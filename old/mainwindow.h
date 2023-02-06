#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QMessageBox>
#include <QTcpSocket>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(qint16 port, QString ip, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    QTcpSocket* sock;
    QString userName;
    bool nameAvaiable;

    void socketConnected();
    void socketDisconnected();
    void socketError(QTcpSocket::SocketError);
    void socketReadable();
    void PushBtnGoLobby();
    void PushBtnEnter();
    void submitBtnHit();
    int sendMessage(QString header, QString message);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
