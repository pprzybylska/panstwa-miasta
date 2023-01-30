#include "game.h"
#include "ui_game.h"

Game::Game(QTcpSocket *socket, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Game)
{
    ui->setupUi(this);
    sock = socket;

    connect(ui->leaveBtn, &QPushButton::clicked, this, &Game::PushBtnDisconnect);

}

Game::~Game()
{
    delete ui;
}

void Game::PushBtnDisconnect() {
    QMessageBox::warning(this, "Connection Warning", "You are going back to lobby"),
            QMessageBox::Ok;
    this->close();
    QApplication::exit(1);
}
