#include <QCoreApplication>
#include <QByteArray>
#include <QtCore>
#include <qlogging.h>

#include <iostream>
#include <qnumeric.h>
#include <vector>
#include <ranges>
#include <thread>
#include <signal.h>
#include <functional>
#include <cstdlib>

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>



#include "service.h"


bool isEven(int num) {
    return num % 2 == 0;
}


void signalHandler(int signal) {
    if (signal == SIGINT) {        
        LobService::instance().stop();
    }
}

//
//void log_route(QtMsgType type, const QMessageLogContext& context, const QString& msg,QTextStream&out ){
//    switch (type) {
//            case QtDebugMsg:
//                out << "Debug: " << msg << endl;
//                break;
//            case QtWarningMsg:
//                out << "Warning: " << msg << endl;
//                break;
//            case QtCriticalMsg:
//                out << "Critical: " << msg << endl;
//                break;
//            case QtFatalMsg:
//                out << "Fatal: " << msg << endl;
//                break;
//        }
//}

void setupQtLogger(){
    using namespace std::placeholders;

    QDate currentDate = QDate::currentDate();
    QString fileName = currentDate.toString("yyyyMMdd") + ".log";
    QFile file(fileName);
    file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append); // 打开文件进行写入
    QTextStream out(&file);

//    qInstallMessageHandler( std::bind(log_route,_1,_2,_3,std::ref(out)));

    qInstallMessageHandler( [](QtMsgType type, const QMessageLogContext& context, const QString& msg){

        switch (type) {
            case QtDebugMsg:
                std::cout << "Debug: " << msg.toStdString() << std::endl;
                break;
            case QtWarningMsg:
                std::cout << "Warning: " << msg.toStdString() << std::endl;
                break;
            case QtCriticalMsg:
                std::cout << "Critical: " << msg.toStdString() << std::endl;
                break;
            case QtFatalMsg:
                std::cout << "Fatal: " << msg.toStdString() << std::endl;
                break;
        }
    });
}

int main(int argc, char *argv[]){
   const  char *env = std::getenv("LOBSTER_CONFIG");
    if( env == nullptr){
        std::cout<<"LOBSTER_CONFIG not set."<<std::endl;
        env = "settings.json";
        // return 1;
    }

    setupQtLogger();
    log4cplus::Initializer initializer;
    log4cplus::BasicConfigurator config;
    config.configure();

    log4cplus::Logger logger = log4cplus::Logger::getInstance(
        LOG4CPLUS_TEXT("main"));
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("AppService[lobster] started.."));
    
    qDebug( "AppService[lobster] started..");
    if( !LobService::instance().init(env)){
        qCritical()<<"LobService init Error.";
        return 1;
    }
    LobService::instance().start();
    LobService::instance().waitForShutdown();
}

// LOBSTER_CONFIG=/opt/lobster/settings.json ./lobster
//