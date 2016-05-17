#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // загрузка переводов:
    QTranslator myTranslator;
    myTranslator.load("climb_championship_" + QLocale::system().name());
    a.installTranslator(&myTranslator);

    MainWindow w;
    w.show();

    return a.exec();
}
