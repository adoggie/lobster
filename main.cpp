#include <QCoreApplication>
#include <QByteArray>
#include <iostream>
#include <vector>
#include <ranges>
#include <thread>
#include "signal.h"

#include "service.h"


bool isEven(int num) {
    return num % 2 == 0;
}

// int __main(int argc, char *argv[])
// {
//     // qCompress(QByteArray("hello world"));
//     std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//     QString s;
//     QStringList list = QString("1,2,3,4,5,6,7,8,9,10").split(",");
//     for (QString str : list) {
//         std::cout << str.toStdString() << " ";
//     }
//     std::cout << std::endl;
//     auto evenNumbers = numbers | std::views::filter(isEven);

//     for (int num : evenNumbers) {
//         std::cout << num << " ";
//     }
//     std::cout << std::endl;

//     QCoreApplication a(argc, argv);
//     qDebug("hello!");
//     return a.exec();
// }


void signalHandler(int signal) {
    if (signal == SIGINT) {        
        LobService::instance().stop();
    }
}

int main(int argc, char *argv[]){
    LobService::instance().init("settings.json");
    LobService::instance().start();
    LobService::instance().waitForShutdown();
}