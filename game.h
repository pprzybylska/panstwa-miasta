#ifndef GAME_H
#define GAME_H

#include <QDialog>
#include <QMessageBox>
#include <QTcpSocket>

namespace Ui {
class Game;
}

class Game : public QDialog
{
    Q_OBJECT

public:
    explicit Game(QTcpSocket *socket, QWidget *parent = nullptr);
    ~Game();

protected:
    QTcpSocket* sock;
    void PushBtnDisconnect();

private:
    Ui::Game *ui;
};

#endif // GAME_H
