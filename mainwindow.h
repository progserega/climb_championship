#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtSerialPort/QSerialPort>
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void handleSerialError(QSerialPort::SerialPortError error);

private slots:
    void handleNewSerialData();

    void on_pushButton_update_serialport_list_released();

    void on_pushButton_connect_to_serialport_released();

    void on_comboBox_serialport_currentIndexChanged(const QString &arg1);

private:
    void initSerial(QString serialPortName);

    Ui::MainWindow *ui;
    QSerialPort *serialPort;
    QByteArray *serialBuffer;

};

#endif // MAINWINDOW_H
