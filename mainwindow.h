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

    struct resultData {
        int trace;
        QString *status;
        long unsigned int time;
        resultData *next;
        resultData *prev;
    } *results, **resultsLast;

    Ui::MainWindow *ui;
    QSerialPort *serialPort;
    QByteArray *serialBuffer;
    QString *serialReaded;

    void initSerial(QString serialPortName);
    int parseSerialData(QString *data);
    int ShowResult(resultData *item);
};

#endif // MAINWINDOW_H
