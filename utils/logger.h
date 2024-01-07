//
// Created by zhiyuan on 24-1-1.
//

#ifndef LOBSTER_LOGGER_H
#define LOBSTER_LOGGER_H
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>

log4cplus::Logger& getLogger(const std::string& name="lobster");

#endif //LOBSTER_LOGGER_H
