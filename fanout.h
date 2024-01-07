#ifndef _LOG_FANOUT_H
#define _LOG_FANOUT_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "lob.h"
#include <atomic>
#include <thread>
#include <QJsonObject>
#include "lockqueue.h"

class  LobRecordFanout{
public:
    typedef std::shared_ptr<LobRecordFanout> Ptr;
    struct Settings{
        std::string server_addr; // zmq fanout server address
        std::string mode;       // bind or connect
        bool enable;
    };
    LobRecordFanout() = default;
    virtual bool init(const QJsonObject& settings) ;
    ~LobRecordFanout() = default;
    virtual bool start();
    virtual void stop();
    virtual void fanout( lob_px_record_t::Ptr & record);
protected:
    std::atomic<bool> stopped_;    
    std::thread  thread_;
    LockQueue<lob_px_record_t::Ptr> queue_;
};

#endif