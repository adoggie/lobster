/***
 * @file feed_tonglian_mdl_redis.h
 * @brief 接收来自通联数据规格的L2行情数据 redis
 * @author scott
*/

#ifndef _FEED_TONGLIAN_REDIS_H
#define _FEED_TONGLIAN_REDIS_H

#include "feed_tonglian_mdl.h"
#include "lob.h"
#include <atomic>
#include <thread>
#include <QJsonObject>
#include <sw/redis++/redis++.h>


class Mdl_Redis_Feed : public FeedBase{
public:
   
    typedef tonglian::OrdTrdType OrdTrdType;
    
    struct DataDecoder:tonglian::DataDecoder{
        // Message* decode(lob_data_t* data);
    };
     struct Settings{
        std::string server_addr; // 
        std::string auth;       //  
        std::vector< std::string > subs; // 订阅主题
    }; 

public:    
    bool init(const QJsonObject& settings) ;   
    bool start();
    void stop();
    Mdl_Redis_Feed() = default;
    ~Mdl_Redis_Feed() = default;
protected:

private:
    std::atomic<bool> stopped_;
    Settings config_;
    std::thread  thread_;
    std::shared_ptr<sw::redis::Redis> redis_;
};


#endif // 