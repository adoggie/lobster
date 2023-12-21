
#ifndef WORKER_H
#define WORKER_H

#include <thread>
#include <atomic>
#include <vector>
#include <memory>

#include "lockqueue.h"
#include "lob.h"
#include "feed.h"

class Worker {
public:
    typedef std::shared_ptr<Worker> Ptr;
    Worker()=default;
    ~Worker()=default;
    void start();
    void stop();
    void setDataDecoder(IFeedDataDecoder* decoder);
    void enqueue(lob_data_t* data);
protected:
    void onOrder(const  OrderMessage::Ptr& m);
    void onTrade(const  TradeMessage::Ptr& m);
private:
    LockQueue<lob_data_t*> dataqueue_;
    IFeedDataDecoder* decoder_ = nullptr;
    std::thread   thread_;
    std::atomic<bool> stopped_;
};

#endif 
