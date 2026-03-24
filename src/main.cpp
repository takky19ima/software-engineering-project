#include "client.h"
#include "mainwindow.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <QApplication>
using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 4) {
        std::cerr << "Usage:\n";
        std::cerr << "client <world_map> <bug1> <bug2> [ticks_per_frame] [fps]\n";
        return 1;
    }

    std::string world = argv[1];
    std::string bug1 = argv[2];
    std::string bug2 = argv[3];

    
    int ticks_per_frame = 50;
    int fps = 10;

    //Optional change if client wants to specify ticks per frame and frames per second
    if (argc >= 5)
        ticks_per_frame = std::stoi(argv[4]);

    if (argc >= 6)
        fps = std::stoi(argv[5]);

    //If there are more than 6 arguments, abort
    if (argc >=7){
        std::cerr << "Usage:\n";
        std::cerr << "client <world_map> <bug1> <bug2> [ticks_per_frame] [fps]\n";
        return 1;
    }

    Client client;

    //Initialize Qt application
    QApplication app(argc, argv);

    //Create main window and set simulation parameters
    MainWindow w;

    //Initializing the simulation parameters and starting the simulation
    w.setSimulationParameters(world, bug1, bug2, ticks_per_frame, fps);
    
    //Display the main window
    w.show();

    //Enter the Qt event loop and wiats until exit is called
    int result = app.exec();

    client.stop();
    return result;
}