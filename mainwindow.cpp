#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QMessageBox>
#include <QDebug>
#include <QTime>
#include <QSound>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    serialPort = new QSerialPort;
    serialBuffer = new QByteArray;
    serialReaded = new QString;
    results = 0;
    resultsLast=&results;
    on_pushButton_update_serialport_list_released();
    ui->start_button->setDisabled(true);
    // Связываем обработчики с событиями в последлвательном порте:
//    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::handleNewSerialData);
    //connect(serialPort, SIGNAL(readyRead()), this, SLOT(handleNewSerialData(QByteArray *)));
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(handleNewSerialData()));
    connect(serialPort, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),this, &MainWindow::handleSerialError);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(startLap()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::parseSerialData(QString *data)
{
    if(data->at(data->length()-1) == '\n')
    {
        resultData **cur=resultsLast;
        if(*resultsLast)
        {
            // не первый элемент (в cur - адрес next-элемента в последнем элементе списка)
            cur=&((*cur)->next);
        }
        *cur = new resultData;
        memset(*cur,0,sizeof(resultData));
        (*cur)->prev=*resultsLast;
        (*cur)->status = new QString;
        resultsLast=cur;

        QStringList items = data->split(";");
        for(int index=0; index<items.size(); ++index)
        {
            qDebug() << "item[" << index << "]" << items.at(index);
            QStringList values = items.at(index).split(":");
            QString key = values.at(0);
            QString val = values.at(1);

            // заполняем структуру:
            if(key=="trace")
            {
                (*resultsLast)->trace=val.toInt();
            }
            else if (key == "time_ms")
            {
                (*resultsLast)->time=val.toInt();
            }
            else if (key == "result")
            {
                (*(*resultsLast)->status)=val;
            }
        }
        qDebug() << "== New result: ==\ntrace:" << (*resultsLast)->trace << "\nstatus: " << (*(*resultsLast)->status) << "\nTime: " << (*resultsLast)->time;
        return true;
    }
    return false;
}
int MainWindow::appendResult(resultData *data)
{
    QTableWidget *result=ui->result_table0;
    QLCDNumber *lcd=ui->trace0_time_lcd;
    if (data->trace==1)
    {
        result=ui->result_table1;
        lcd=ui->trace1_time_lcd;
    }
    int rowNum=result->rowCount();
    result->setRowCount(rowNum+1);

    result->scrollToBottom();

    QTableWidgetItem *item = new QTableWidgetItem();
    //item->setText(tr("Участник %1").arg(data->trace));
    item->setText(tr("Участник %1").arg(rowNum+1));
    result->setItem(rowNum,0,item);

    item = new QTableWidgetItem();
    item->setText(*(data->status));
    item->setFlags(Qt::NoItemFlags);// Qt::ItemIsEditable);
    result->setItem(rowNum,1,item);

    item = new QTableWidgetItem();
    item->setText(tr("%1 сек.").arg(((float)data->time)/1000));
    item->setFlags(Qt::NoItemFlags);// Qt::ItemIsEditable);
    result->setItem(rowNum,2,item);

    // lcd

    //lcd->display(data->time);
    lcd->display(tr("%1").arg(((float)data->time)/1000));

    return true;
}
void MainWindow::handleNewSerialData()
{
    serialBuffer->append(serialPort->readAll());
    serialReaded->clear();
    serialReaded->append(*serialBuffer);
    qDebug() << QTime::currentTime() << ": " << "read new data from serial port: " << *serialReaded;
    if(parseSerialData(serialReaded))
    {
        // данные обработаны - очищаем буферы:
        serialBuffer->clear();
        serialReaded->clear();
        // добавляем запись в отчёт:
        appendResult(*resultsLast);

    }
}
void MainWindow::handleSerialError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError) {
        //m_standardOutput << QObject::tr("An I/O error occurred while reading the data from port %1, error: %2").arg(m_serialPort->portName()).arg(m_serialPort->errorString()) << endl;
        qDebug() << QTime::currentTime() << ": " << "error read from serial port: " << serialPort->errorString();

        QMessageBox *pmbx = new QMessageBox;
        pmbx->setText(QObject::tr("Ошибка чтения из порта : %1").arg(serialPort->errorString())) ;
        //pmbx->setInformativeText("Пожалуйста, выбирите рабочий порт");
        pmbx->setIcon(QMessageBox::Critical);
        pmbx->setStandardButtons(QMessageBox::Ok | QMessageBox::Escape);
        pmbx->setDefaultButton(QMessageBox::Ok);
        pmbx->exec();
        delete pmbx;
    }
}

void MainWindow::on_pushButton_update_serialport_list_released()
{
    ui->comboBox_serialport->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
            QString s = info.portName();
/*
                        + QObject::tr("Location: ") + info.systemLocation() + "\n"
                        + QObject::tr("Description: ") + info.description() + "\n"
                        + QObject::tr("Manufacturer: ") + info.manufacturer() + "\n"
                        + QObject::tr("Serial number: ") + info.serialNumber() + "\n"
                        + QObject::tr("Vendor Identifier: ") + (info.hasVendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString()) + "\n"
                        + QObject::tr("Product Identifier: ") + (info.hasProductIdentifier() ? QString::number(info.productIdentifier(), 16) : QString()) + "\n"
                        + QObject::tr("Busy: ") + (info.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) + "\n";
*/
            //QVariant *qv = new QVariant(s);
            ui->comboBox_serialport->addItem(s,s);
    }
}

void MainWindow::on_pushButton_connect_to_serialport_released()
{
    QString serialPortName = ui->comboBox_serialport->currentText();
    initSerial(serialPortName);
}

void MainWindow::initSerial(QString serialPortName)
{
    serialPort->setPortName(serialPortName);

    if (!serialPort->open(QIODevice::ReadWrite))
    {
        QMessageBox *pmbx = new QMessageBox;
        pmbx->setText("Ошибка подключения к порту: " + serialPortName);
        pmbx->setInformativeText("Пожалуйста, выбирите рабочий порт");
        pmbx->setIcon(QMessageBox::Critical);
        pmbx->setStandardButtons(QMessageBox::Ok | QMessageBox::Escape);
        pmbx->setDefaultButton(QMessageBox::Ok);
        pmbx->exec();
        delete pmbx;
        qDebug() << "Error open port: " << serialPortName;
        return;
    }
    serialPort->setBaudRate( QSerialPort::Baud9600 );
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    ui->pushButton_connect_to_serialport->setDisabled(true);
    ui->start_button->setDisabled(false);
    qDebug() << QTime::currentTime() << ": " << "Open is normal";
    qDebug() << "= Connected with parameters =";
    qDebug() << "Device name            : " << serialPort->portName();
    qDebug() << "Baud rate              : " << serialPort->baudRate();
    qDebug() << "Data bits              : " << serialPort->dataBits();
    qDebug() << "Parity                 : " << serialPort->parity();
    qDebug() << "Stop bits              : " << serialPort->stopBits();
    qDebug() << "Flow                   : " << serialPort->flowControl();
}

void MainWindow::on_comboBox_serialport_currentIndexChanged(const QString &serialPortName)
{
    ui->pushButton_connect_to_serialport->setEnabled(true);
    serialPort->close();
}

void MainWindow::on_start_button_released()
{
    QSound::play("ring1.wav");

    timer->start(3000);
    // TODO send внимание to arduino
}

void MainWindow::startLap()
{
    timer->stop();
    QSound::play("ring2.wav");
    // TODO send start to arduino
}
