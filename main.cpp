#include "mainwindow.h"
#include "login.h"
#include "game.h"

#include <QApplication>
#include <QTcpSocket>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString ip = "localhost";
    qint16 port = 8080;
    QTcpSocket* sock {nullptr};

    Login loginWindow(sock, port, ip);
    MainWindow lobby(sock);
    Game gameWindow(sock);


    loginWindow.show();
    if (a.exec() < 0)  return -1;

    lobby.show();
    if (a.exec() < 0) return -1;

    while(true) {
        gameWindow.show();
        if (a.exec() == 1) {
            lobby.show();
            if (a.exec() < 0) return -1;
        } else break;
    }

    if(sock) sock->close();
    return 0;
}
