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
#include "utils/logger.h"
#include "service.h"

bool isEven(int num) {
    return num % 2 == 0;
}

void signalHandler(int signal) {
    if (signal == SIGINT) {        
        LobService::instance().stop();
    }
}

log4cplus::Logger& getLogger(const std::string& name){
    static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("lobster"));
    static log4cplus::Logger error = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("lobster-error"));
    if( name == "error"){
        return error;
    }
    return logger;
}

int main(int argc, char *argv[]){
//    std::vector<int> n = {1,2,3};
//    for( auto k:n){
//        std::cout<< k << std::endl;
//    }

   const  char *env = std::getenv("LOBSTER_CONFIG");
    if( env == nullptr){
        std::cout<<"LOBSTER_CONFIG not set."<<std::endl;
        env = "settings.json";
        // return 1;
    }

    log4cplus::Initializer initializer;
    log4cplus::PropertyConfigurator::doConfigure("lobsterlog.properties");


    LOG4CPLUS_INFO(getLogger(), LOG4CPLUS_TEXT("AppService[lobster] started.."));

    if( !LobService::instance().init(env)){
        LOG4CPLUS_ERROR_FMT(getLogger(),"LobService init Error.");
        return 1;
    }
    LobService::instance().start();
    LobService::instance().waitForShutdown();
}

// LOBSTER_CONFIG=/opt/lobster/settings.json ./lobster
//