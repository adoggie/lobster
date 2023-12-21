/***
 * @file feed_zmq.h
 * @brief 接收来自zmq的通联数据规格的L2行情数据
 * @author scott
*/

#ifndef _FEED_ZMQ_H
#define _FEED_ZMQ_H

#include "feed.h"
#include "lob.h"
#include <atomic>
#include <thread>

class Mdl_Zmq_Feed : public FeedBase{
public:
    enum OrdTrdType{
        ORD = 0,
        TRD = 1,
        ORDTRD = 2
    };
    
    struct DataDecoder:IFeedDataDecoder{
        Message* decode(lob_data_t* data);
        Message * decodeSH(const std::vector<std::string>& ss, OrdTrdType ordtrd);
        Message * decodeSZ(const std::vector<std::string>& ss, OrdTrdType ordtrd);
    };
public:
    Mdl_Zmq_Feed(const zmq_feed_setting_t & config);
    ~Mdl_Zmq_Feed() = default;
    bool start();
    void stop();
protected:
    // Message::Ptr parse(const std::string & line);

private:
    std::atomic<bool> stopped_;
    zmq_feed_setting_t config_;
    std::thread  thread_;
};


#endif // 