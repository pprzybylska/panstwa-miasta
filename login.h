#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include <QMessageBox>
#include <QTcpSocket>

namespace Ui {
class Login;
}

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QTcpSocket* socket, qint16 port, QString ip, QWidget *parent = nullptr);
    ~Login();

protected:
    void PushBtnEnter();
    QTcpSocket* sock;
    void socketConnected();
    void socketDisconnected();
    void socketError(QTcpSocket::SocketError);

private:
    Ui::Login *ui;
};

#endif // LOGIN_H
