#ifndef _LOG_FANOUT_REDIS_H
#define _LOG_FANOUT_REDIS_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "lob.h"
#include <atomic>
#include <thread>
#include <QJsonObject>
#include <QFile>

#include <sw/redis++/redis++.h>

#include "fanout.h"



class  LobRecordFanoutRedis:public LobRecordFanout{
public:
    typedef std::shared_ptr<LobRecordFanoutRedis> Ptr;
    struct Settings{
        std::string server_addr; //  fanout server address        
        bool enable;
    };

    bool init(const QJsonObject& settings) ;
//    LobRecordFanoutRedis() = default;
//    ~LobRecordFanoutRedis() = default;
    bool start();
    void stop();
    
private:
    LobRecordFanoutRedis::Settings  config_;
    std::shared_ptr<sw::redis::Redis> redis_;
};

#endif