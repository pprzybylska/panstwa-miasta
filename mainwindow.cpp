#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QTcpSocket *socket, QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    sock = socket;

    connect(ui->disconnect, &QPushButton::clicked, this, &MainWindow::PushBtnDisconnect);

    // TODO: populate users in game (ask server for list of strings with user names)
    QList<QString> usersL;
    usersL << "test" << "chuj" << "twoja" << "stara";
    ui->usersList->addItems(usersL);

    // TODO: populate temp users in lobby (ask server for list of strings with user names)
    QList<QString> usersLobby;
    usersLobby << "tmp";
    ui->listLobby->addItems(usersLobby);

    //TODO: get timer from server live
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::PushBtnDisconnect() {
    QMessageBox::warning(this, "Connection Warning", "You have been disconnected."),
            QMessageBox::Ok;
    if(sock) sock->close();
    QApplication::exit(-1);
}
