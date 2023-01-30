#include "login.h"
#include "ui_login.h"

Login::Login(QTcpSocket *socket, qint16 port, QString ip, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);

    ui->PBtnEnterGame->setEnabled(false);
    ui->enterName->setEnabled(false);

    connect(ui->enterName, &QLineEdit::returnPressed, ui->PBtnEnterGame, &QPushButton::click);
    connect(ui->PBtnEnterGame, &QPushButton::clicked, this, &Login::PushBtnEnter);

    // Connect to port
    sock = socket;
    sock = new QTcpSocket();
    sock->connectToHost(ip, port);

    connect(sock, &QTcpSocket::connected, this, &Login::socketConnected);
    connect(sock, &QTcpSocket::disconnected, this, &Login::socketDisconnected);
    connect(sock, &QTcpSocket::errorOccurred, this, &Login::socketError);
}

Login::~Login()
{
    delete ui;
}

void Login::PushBtnEnter() {

    QString name = ui->enterName->text().trimmed();
    if (name.size() > 10) {
        QMessageBox::critical(this, "Name Error", "Your chosen name is too long.\nIt should contain 10 or less characters."),
                QMessageBox::Ok;
        ui->enterName->clear();
    } else if (name.size() < 1) {
        QMessageBox::critical(this, "Name Error", "You must chooose a name!"),
                QMessageBox::Ok;
        ui->enterName->clear();
    } else this->close();

    // TODO:
    // ask server if name is taken - if not add name to lobby list

//    QMessageBox::critical(this, "Used Name Error", "This name is already taken./sPease choose another one."), QMessageBox::Ok;
}

void Login::socketConnected(){
    ui->PBtnEnterGame->setEnabled(true);
    ui->enterName->setEnabled(true);
}

void Login::socketDisconnected(){
    QMessageBox::critical(this, "Cconnection Error", "Disconnected"), QMessageBox::Ok;
    if(sock) sock->close();
    QApplication::exit(-1);
}

void Login::socketError(QTcpSocket::SocketError err){
    if(err == QTcpSocket::RemoteHostClosedError) QApplication::exit(-1);
    QMessageBox::critical(this, "Connection Error", sock->errorString());
    if(sock) sock->close();
    QApplication::exit(-1);
}

