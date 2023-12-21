#ifndef _LOG_FANOUT_H
#define _LOG_FANOUT_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "lob.h"
#include <atomic>
#include <thread>

#include "lockqueue.h"

class  LobRecordFanout{
public:
    struct Settings{
        std::string server_addr; // zmq fanout server address
        std::string mode;       // bind or connect
    };

    LobRecordFanout(const LobRecordFanout::Settings& settings) ;
    ~LobRecordFanout() = default;
    bool start();
    void stop();
    void fanout( lob_px_record_t::Ptr & record);
private:
    // std::shared_ptr<LogFanoutImpl> impl_;
    LobRecordFanout::Settings  config_;
    std::atomic<bool> stopped_;    
    std::thread  thread_;
    LockQueue<lob_px_record_t::Ptr> queue_;
};

#endif