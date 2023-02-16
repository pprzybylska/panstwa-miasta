#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QMessageBox>
#include <QTcpSocket>
#include <unistd.h>
#include <fstream>
#include <string>
#include <algorithm>
#include <iostream>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(std::string configurationPath, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    qint16 port;
    QString ip;
    int roundTime;
    int gameTime;
    int breakTime;

    QTcpSocket* sock;
    QString userName;
    bool nameAvaiable;

    int numUsersLobby;
    int numRanking;
    QList<QString> usersLobby;
    QList<QString> ranking;
    QTimer * infoTimer{nullptr};

    void socketConnected();
    void socketDisconnected();
    void socketError(QTcpSocket::SocketError);
    void socketReadable();
    void PushBtnGoLobby();
    void PushBtnEnter();
    void submitBtnHit();
    int sendMessage(QString header, QString message);
    void getConfig(std::string configPath);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
