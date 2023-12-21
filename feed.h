

#ifndef _FEED_H
#define _FEED_H


#include "message.h"
#include <memory.h>
#include <memory>
#include "lob.h"

struct IFeedUser{
    // virtual void onOrder(const  OrderMessage::Ptr m) = 0;
    // virtual void onTrade(const  TradeMessage::Ptr m) = 0;
    // virtual void onFeedData(const char* data, size_t len) = 0;
    virtual void onFeedRawData(lob_data_t * data) = 0;
};

struct IFeedDataDecoder{
    virtual Message* decode( lob_data_t* data) = 0;
};

struct FeedBase{
    typedef std::shared_ptr<FeedBase> Ptr;
    enum{
        MDL_FILE,
        MDL_LIVE, // redis or websocket
        ZMQ_LIVE
    };
public:
    virtual bool start() = 0;
    virtual void stop() = 0;
    void setFeedUser(IFeedUser* user){
        user_ = user;
    }
protected:
    void onFeedRawData(lob_data_t* data){
        if (user_) user_->onFeedRawData(data);
    }
    
private:
    IFeedUser* user_ = nullptr;
};



#endif // !_FEED

