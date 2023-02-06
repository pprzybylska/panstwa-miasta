#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(qint16 port, QString ip, QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    userName = "";
    this->nameAvaiable = false;

    ui->GameGroup->hide();
    ui->LobbyGroup->hide();

    ui->PBtnEnterGame->setEnabled(false);
    ui->enterName->setEnabled(false);

    // create conection
    sock = new QTcpSocket();
    sock->connectToHost(ip, port);

    connect(sock, &QTcpSocket::connected, this, &MainWindow::socketConnected);
    connect(sock, &QTcpSocket::disconnected, this, &MainWindow::socketDisconnected);
    connect(sock, &QTcpSocket::errorOccurred, this, &MainWindow::socketError);
    connect(sock, &QTcpSocket::readyRead, this, &MainWindow::socketReadable);

    connect(ui->enterName, &QLineEdit::returnPressed, ui->PBtnEnterGame, &QPushButton::click);
    connect(ui->PBtnEnterGame, &QPushButton::clicked, this, &MainWindow::PushBtnEnter);

    connect(ui->disconnect, &QPushButton::clicked, this, &MainWindow::socketDisconnected);
    connect(ui->leaveBtn, &QPushButton::clicked, this, &MainWindow::PushBtnGoLobby);
    connect(ui->submitBtn, &QPushButton::clicked, this, &MainWindow::submitBtnHit);

    // TODO: populate users in game (ask server for list of strings with user names)
    QList<QString> usersL;
    usersL << "test" << "chuj" << "twoja" << "stara";
    ui->usersList->addItems(usersL);

    // TODO: populate temp users in lobby (ask server for list of strings with user names)
    QList<QString> usersLobby;
    usersLobby << userName;
    usersLobby << "tmp";
    ui->listLobby->addItems(usersLobby);
}

MainWindow::~MainWindow() {
    if(sock) sock->close();
    delete ui;
}

void MainWindow::PushBtnEnter() {

    QString name = ui->enterName->text().trimmed();
    if (name.size() > 10) {
        QMessageBox::critical(this, "Name Error", "Your chosen name is too long.\nIt should contain 10 or less characters."),
                QMessageBox::Ok;
    } else if (name.size() < 1) {
        QMessageBox::critical(this, "Name Error", "You must chooose a name!"),
                QMessageBox::Ok;
    } else if (this->nameAvaiable == true) {
        userName = name;
        ui->LoginGroup->hide();
        ui->LobbyGroup->show();
    }
}

void MainWindow::PushBtnGoLobby() {
    QMessageBox::warning(this, "Connection Warning", "You are going back to lobby"),
            QMessageBox::Ok;
    ui->GameGroup->hide();
    ui->LobbyGroup->show();
}

void MainWindow::socketConnected(){
    ui->PBtnEnterGame->setEnabled(true);
    ui->enterName->setEnabled(true);
}

void MainWindow::socketDisconnected(){
    if(sock) {
        sock->close();
        QMessageBox::critical(this, "Connection", "Disconnected"), QMessageBox::Ok;
    }
    QApplication::exit(-1);
}

void MainWindow::socketError(QTcpSocket::SocketError err){
    if(err == QTcpSocket::RemoteHostClosedError) QApplication::exit(-1);
    QMessageBox::critical(this, "Connection Error", sock->errorString());
    if(sock) sock->close();
    QApplication::exit(-1);
}

void MainWindow::socketReadable(){
    QByteArray buf = sock->readLine();

    if (buf.length() == 0)  {
        QMessageBox::critical(this, "Connection Error", "Error occured. Aborting..."),
                QMessageBox::Ok;
        if(sock) sock->close();
        QApplication::exit(-1);
    }

    QByteArray headBuf = buf.first(4);
    QByteArray message =  buf.sliced(4);
    int head = headBuf.toInt();

    switch (head) {
    case 10:    // Info
        QMessageBox::information(this, "Server Info", message), QMessageBox::Ok;
        break;
    case 11:    // Letter
        if (! ui->GameGroup->isHidden()) ui->letterEdit->setText(message.toUpper());
        break;
    case 12:    // timer lobby
        if (! ui->LobbyGroup->isHidden()) ui->showTimer->setText(message);
        break;
    case 13:    // timer game
        if (! ui->GameGroup->isHidden()) ui->timerEdit->setText(message);
        break;
    case 14:    // error
        QMessageBox::critical(this, "Server Error", message), QMessageBox::Ok;
        if(sock) sock->close();
        QApplication::exit(-1);
        break;
    case 15:    // name avaiable
        if (message.contains("false")) {
            QMessageBox::critical(this, "Used Name Error", "This name is already taken./nPlease choose another one."),
                    QMessageBox::Ok;
        } else if (message.contains("true")) {
            nameAvaiable = true;
            ui->LoginGroup->hide();
            ui->LobbyGroup->show();
        }
        break;
    case 50:    // go lobby -> game
        ui->LobbyGroup->hide();
        ui->GameGroup->show();
        QMessageBox::information(this, "New Game", "New game will start now."), QMessageBox::Ok;
        break;
    default:
        break;
    }
}

void MainWindow::submitBtnHit(){
    auto country = ui->imieLine->text().trimmed();
    if (country.isEmpty()) country = " ";
    auto city = ui->miastoLine->text().trimmed();
    if (city.isEmpty()) city = " ";
    auto name = ui->panstwoLine->text().trimmed();
    if (name.isEmpty()) name = " ";
    auto animal = ui->rzeczLine->text().trimmed();
    if (animal.isEmpty()) animal = " ";
    auto job = ui->zawodLine->text().trimmed();
    if (job.isEmpty()) job = " ";
    auto object = ui->zwierzeLine->text().trimmed();
    if (object.isEmpty()) object = " ";

    ui->enterAnswer->setEnabled(false);
}

int MainWindow::sendMessage(QString header, QString message) {
    auto txt = header + message;
    if (sock->write(txt.toUtf8()) == -1) {
        QMessageBox::critical(this, "Error", "Error occured while communicationg with server."), QMessageBox::Ok;
        return 1;
    }
    return 0;
}

/*
100 recive nickname
101 country
102 city
103 name
104 animal
105 job
106 object
*/
