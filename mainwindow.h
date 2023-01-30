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
    MainWindow(QTcpSocket *socket, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    QTcpSocket* sock;
    void PushBtnDisconnect();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
