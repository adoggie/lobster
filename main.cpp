#include <QCoreApplication>
#include <QByteArray>
#include <iostream>
#include <qnumeric.h>
#include <vector>
#include <ranges>
#include <thread>
#include <signal.h>

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>



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

    log4cplus::Initializer initializer;
    log4cplus::BasicConfigurator config;
    config.configure();

    log4cplus::Logger logger = log4cplus::Logger::getInstance(
        LOG4CPLUS_TEXT("main"));
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("AppService[lobster] started.."));
    
    qDebug( "AppService[lobster] started..");
    LobService::instance().init("settings.json");
    LobService::instance().start();
    LobService::instance().waitForShutdown();
}