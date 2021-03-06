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
    // Главный виджет на всё окно:
    window = new QWidget();
    window->setLayout(ui->verticalLayout_3);
    setCentralWidget(window);

    serialPort = new QSerialPort;
    serialBuffer = new QByteArray;
    serialReaded = new QString;
    results = 0;
    resultsLast=&results;
    on_pushButton_update_serialport_list_released();
    ui->start_button->setDisabled(true);
    // Связываем обработчики с событиями в последлвательном порте:
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(handleNewSerialData()));
    connect(serialPort, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),this, &MainWindow::handleSerialError);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(startLap()));

    // масштабирование таблицы:
    ui->result_table0->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->result_table0->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->result_table0->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    ui->result_table1->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->result_table1->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->result_table1->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete window;
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
    item->setText(tr("Sportsmen %1").arg(rowNum+1));
    result->setItem(rowNum,0,item);

    item = new QTableWidgetItem();
    item->setText(*(data->status));
    item->setFlags(Qt::NoItemFlags);// Qt::ItemIsEditable);
    result->setItem(rowNum,1,item);

    item = new QTableWidgetItem();
    item->setText(tr("%1 sec.").arg(((float)data->time)/1000));
    item->setFlags(Qt::NoItemFlags);// Qt::ItemIsEditable);
    result->setItem(rowNum,2,item);

    // lcd

    //lcd->display(data->time);
    lcd->display(QString("%1").arg(((float)data->time)/1000));

    return true;
}
int MainWindow::getNextCommand(QString *data, QString *cmd)
{
    int index = data->indexOf("\n");
    if (index == -1)
    {
        return false;
    }
    else
    {
        QString tmp;
        *cmd = data->left(index+1);
        tmp = data->right(data->size()-(index+1));
        *data = tmp;
        return true;
    }
}
int MainWindow::parseSerialData(QString *data)
{
    QString cmd;
    qDebug() << __FUNCTION__ << ":" << __LINE__ << "input data: '" << *data << "'";
    bool successParseCommand=false;

    while(getNextCommand(data,&cmd))
    {
        qDebug() << __FUNCTION__ << ":" << __LINE__ << "current data: '" << *data << "'";
        qDebug() << "process command: '" << cmd << "'";
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

            QStringList items = cmd.split(";");
            for(int index=0; index<items.size(); ++index)
            {
                qDebug() << "item[" << index << "]" << items.at(index);
                QStringList values = items.at(index).split(":");
                if(values.size()<2)
                {
                    // неверная команда (нет пары ключ-значение) - пропуск:
                    qDebug() << "error command. No pars ket:value. Skip command.";
                    continue;
                }
                QString key = values.at(0);
                QString val = values.at(1);

                // заполняем структуру:
                if(key=="trace")
                {
                    (*resultsLast)->trace=val.toInt();
                    successParseCommand=true;
                }
                else if (key == "time_ms")
                {
                    (*resultsLast)->time=val.toInt();
                    successParseCommand=true;
                }
                else if (key == "result")
                {
                    // берём перевод состояний:
                    if (val == "success")
                    {
                        (*(*resultsLast)->status)=tr("success","success trace by sportsmen");
                    }
                    else if (val == "falsh_start")
                    {
                        (*(*resultsLast)->status)=tr("falsh start","falsh start trace by sportsmen");
                    }
                    else if (val == "not_on_start_button")
                    {
                        (*(*resultsLast)->status)=tr("not on start button","spotrtsmen not stay on start position, when start command sayed");
                    }
                    else
                    {
                        (*(*resultsLast)->status)=val;
                    }
                    successParseCommand=true;
                }
                else if (key == "current_log_ms")
                {
                    (*resultsLast)->change_time_arduino=val.toInt();
                    successParseCommand=true;
                }
            }
            qDebug() << "== New result: ==\ntrace:" << (*resultsLast)->trace << "\nstatus: " << (*(*resultsLast)->status) << "\nTime: " << (*resultsLast)->time << "\nChange time arduino:" << (*resultsLast)->change_time_arduino << "\n==============";
            // добавляем запись в отчёт:
            if(successParseCommand)
                appendResult(*resultsLast);
        }
    if(*data == "")
    {
        return true;
    }
    else
    {
        // ещё есть необработанные даныне в буфере
        return false;
    }
}

void MainWindow::handleNewSerialData()
{
    serialBuffer->clear();
    serialBuffer->append(serialPort->readAll());
    //serialReaded->clear();
    serialReaded->append(*serialBuffer);
    //qDebug() << QTime::currentTime() << ": " << "read new data from serial port: " << *serialReaded;
    if(parseSerialData(serialReaded))
    {
        // данные обработаны - очищаем буферы:
        //serialBuffer->clear();
        serialReaded->clear();
    }
}
void MainWindow::handleSerialError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError) {
        //m_standardOutput << QObject::tr("An I/O error occurred while reading the data from port %1, error: %2").arg(m_serialPort->portName()).arg(m_serialPort->errorString()) << endl;
        qDebug() << QTime::currentTime() << ": " << "error read from serial port: " << serialPort->errorString();

        QMessageBox *pmbx = new QMessageBox;
        pmbx->setText(QObject::tr("Error read from port: %1").arg(serialPort->errorString())) ;
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
        pmbx->setText(tr("Error connect to port: ") + serialPortName);
        pmbx->setInformativeText(tr("Please, select work serial port"));
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
    ui->start_button->setEnabled(false);
    serialPort->close();
}

void MainWindow::on_start_button_released()
{
    // send внимание to arduino
    serialPort->write("vnimanie\n");

    // временно блокируем кнопку старта:
    ui->start_button->setEnabled(false);

    // "Внимание":
    QSound::play("ring1.wav");

    timer->start(3000);
}

void MainWindow::startLap()
{
    timer->stop();
    // send start to arduino
    serialPort->write("start\n");
    QSound::play("ring2.wav");

    // разблокируем кнопку старта:
    ui->start_button->setEnabled(true);
}
