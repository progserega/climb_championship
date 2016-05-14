#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QMessageBox>
#include <QDebug>
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    serialPort = new QSerialPort;
    serialBuffer = new QByteArray;
    on_pushButton_update_serialport_list_released();
    // Связываем обработчики с событиями в последлвательном порте:
//    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::handleNewSerialData);
    //connect(serialPort, SIGNAL(readyRead()), this, SLOT(handleNewSerialData(QByteArray *)));
    connect(serialPort, SIGNAL(readyRead()), this, SLOT(handleNewSerialData()));
    connect(serialPort, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),this, &MainWindow::handleSerialError);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::handleNewSerialData()
{
    serialBuffer->append(serialPort->readAll());

    QString str;
    str.append(*serialBuffer);

    qDebug() << QTime::currentTime() << ": " << "read new data from port: " << serialBuffer;
    qDebug() << QTime::currentTime() << ": " << "read new data from port2: " << str;
/*
    QMessageBox *pmbx = new QMessageBox;
    pmbx->setText("new data!: ");
    pmbx->setInformativeText(str);
    pmbx->setIcon(QMessageBox::Information);
    pmbx->setStandardButtons(QMessageBox::Ok | QMessageBox::Escape);
    pmbx->setDefaultButton(QMessageBox::Ok);
    pmbx->exec();
    delete pmbx;
*/
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
    qDebug() << QTime::currentTime() << ": " << "Open is normal";
}

void MainWindow::on_comboBox_serialport_currentIndexChanged(const QString &serialPortName)
{
    ui->pushButton_connect_to_serialport->setEnabled(true);
    serialPort->close();
}
