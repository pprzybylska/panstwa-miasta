#include "mainwindow.h"

#include <QApplication>
#include <QTcpSocket>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString ip = "localhost";
    qint16 port = 8080;

    MainWindow lobby(port, ip);

    lobby.show();
    if (a.exec() < 0) return -1;

    return 0;
}
