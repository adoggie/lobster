/***
 * @file feed_tlive.h
 * @brief 接收来自通联的websock 行情数据
 * @author scott
*/

#ifndef _FEED_LIVE_H
#define _FEED_LIVE_H

#include "feed.h"
#include "lob.h"
#include <atomic>
#include <thread>

class Mdl_Live_Feed : public FeedBase{
public:
    struct DataDecoder:IFeedDataDecoder{
        Message* decode(lob_data_t* data);
    };
public:
    Mdl_Live_Feed(const zmq_feed_setting_t & config);
    ~Mdl_Live_Feed();
    bool start();
    void stop();
protected:

private:
    std::atomic<bool> stopped_;
    mdl_live_feed_setting_t config_;
    std::thread  thread_;
};


#endif // 