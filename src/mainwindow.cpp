#include "mainwindow.h"
#include <QDebug>
#include <QTimer>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(800, 600);
    setWindowTitle("Bug World Simulator");

    view = new WorldView(this);
    view->setAttribute(Qt::WA_OpaquePaintEvent);
    setCentralWidget(view);

    // ── Trace-length toolbar ──
    QToolBar *toolbar = addToolBar("Trace");
    toolbar->addWidget(new QLabel(" Trace N: "));

    traceSpinBox = new QSpinBox(this);
    traceSpinBox->setRange(0, 50);
    traceSpinBox->setValue(5);
    toolbar->addWidget(traceSpinBox);

    connect(traceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            view, &WorldView::setTraceLength);
}

MainWindow::~MainWindow()
{
    client.stop();
    
}

// mainwindow.cpp
void MainWindow::setSimulationParameters(const std::string &world,
                                         const std::string &bug1,
                                         const std::string &bug2,
                                         int ticksPerFrame,
                                         int fps)
{
    this->world = world;
    this->bug1 = bug1;
    this->bug2 = bug2;
    this->ticks_per_frame = ticksPerFrame;
    this->fps = fps;

    bool ok = client.start(world, bug1, bug2);

    if (!ok) {
        qDebug() << "Failed to start simulator";
        return;
    }else{
        qDebug() << "Simulator started";
    }

    on_sendButton_clicked();
}


void MainWindow::on_sendButton_clicked()
{
    
     if (!timer) {
        timer = new QTimer(this);

        connect(timer, &QTimer::timeout, this, [this]() {
            std::string result = client.step(this->ticks_per_frame);
            client.parseResponse(result);
            view->setMap(client.getCurrentMap(), client.getOffsetRows());
        });
    }
    
    int interval_ms = 1000 / this->fps; // convert FPS to milliseconds per frame
    timer->start(interval_ms); 
}