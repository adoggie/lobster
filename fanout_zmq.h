#ifndef _LOG_FANOUT_ZMQ_H
#define _LOG_FANOUT_ZMQ_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "lob.h"
#include <atomic>
#include <thread>
#include <QJsonObject>
#include "lockqueue.h"
#include "fanout.h"

class  LobRecordFanoutZMQ:public LobRecordFanout{
public:
    typedef std::shared_ptr<LobRecordFanoutZMQ> Ptr;
    struct Settings{
        std::string server_addr; // zmq fanout server address
        std::string mode;       // bind or connect
        bool enable;
    };

    bool init(const QJsonObject& settings) ;    
    LobRecordFanoutZMQ() = default;
    ~LobRecordFanoutZMQ() = default;
    bool start();
    void stop();

private:
    LobRecordFanoutZMQ::Settings  config_;
};

#endif