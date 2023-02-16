#include "mainwindow.h"

#include <QApplication>
#include <QTcpSocket>
#include <unistd.h>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    std::string  configurationPath = "config.txt";

//    QString ip = "localhost";
//    qint16 port = 8080;

    MainWindow lobby(configurationPath);

    lobby.show();
    if (a.exec() < 0) return -1;

    return 0;
}
