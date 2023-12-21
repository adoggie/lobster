#ifndef _LOB_SERVICE_H
#define _LOB_SERVICE_H

#include <string>
#include <thread>
#include <atomic>
// #include <pair>
#include <map>
#include <mutex>
#include <condition_variable>
#include "lob.h"
#include "worker.h"
#include "feed.h"


class LobService:public IFeedUser {
public:
    bool init(const std::string& configfile);
    bool start();
    void stop();
    void getPxList(symbolid_t symbolid, size_t depth, std::list< lob_px_record_ptr > & list);
    void savePxList(symbolid_t symbolid);
    static LobService& instance(){
        static LobService s;
        return s;
    }

    void waitForShutdown();
protected:
friend class Worker;
    void onOrder(const  OrderMessage* m);
    void onTrade(const  TradeMessage* m);

    void onOrderSH(const  OrderMessage* m);
    void onTradeSH(const  TradeMessage* m);
    void onOrderSZ(const  OrderMessage* m);
    void onTradeSZ(const  TradeMessage* m);

    void onMessage(Message * message);
    // void onFeedData(const char* data, size_t len) ;
    void onFeedRawData(lob_data_t* data);
    lob_px_list_t* createPxList(symbolid_t symbolid);
    lob_px_history_t * createPxHistory(symbolid_t symbolid);
    bool getPxHighLow(symbolid_t symbolid, float& high, float& low);

    bool loadSymbolList();
private:

    lob_config_t  config_;
    std::vector<lob_px_list_t*> pxlive_; //
    std::vector<lob_px_history_t*> pxhistory_; //
    /*
    000001: 
    600021: [
                [ bid:[], ask:[] ] 
                [ bid:[], ask:[] ] 
                ... 
             ]
    */
    std::vector<Worker::Ptr> workers_;

    FeedBase::Ptr feeder_;
    std::shared_ptr<IFeedDataDecoder> decoder_;
    std::thread   thread_;
    std::atomic<bool> stopped_;

    struct symbol_px_record_t{
        symbolid_t symbolid;
        lob_px_record_ptr record;  
    };

    LockQueue<symbol_px_record_t> queue_;
    std::mutex  mutex_;
    std::condition_variable cond_;
    std::map<symbolid_t, std::pair<float,float > > limit_pxlist_;
    std::vector<symbolid_t>     symbol_list_;
};


#endif