#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "client.h"
#include "worldview.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setSimulationParameters(const std::string &world,
                                 const std::string &bug1,
                                 const std::string &bug2,
                                 int ticksPerFrame,
                                 int fps);

private slots:
    void on_sendButton_clicked();

private:
    Ui::MainWindow *ui;
    Client client;
    WorldView *view;
    QTimer *timer = nullptr;
    std::vector<Bug> mapToBugs(const std::vector<std::string>& map);
    std::string world;
    std::string bug1;
    std::string bug2;
    int ticks_per_frame;
    int fps;
};

#endif