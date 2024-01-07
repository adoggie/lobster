#ifndef _LOB_SERVICE_H
#define _LOB_SERVICE_H

#include <string>
#include <thread>
#include <atomic>
// #include <pair>
#include <map>
#include <mutex>
#include <condition_variable>

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include "lob.h"
#include "worker.h"
#include "feed.h"
#include "fanout.h"
#include "utils/timer.h"


struct lob_config_t {
    uint32_t  lobnum; // 100000
    std::string feed_type;   // feed name: mdl_csv , mdl_live ,
    bool  save_pxlist_infile; // true or false
    std::string save_in_dir; 
    std::string symbol_price_limit_file;
    std::string symbol_list_file;
    // uint32_t  symnum_in_worker =1; // worker threads 
    uint32_t  workers; // 1 - 100
    uint32_t  symnum_of_worker;
    // mdl_live_feed_setting_t live_setting;
    // mdl_csv_feed_setting_t csv_setting;
    // zmq_feed_setting_t zmq_setting;
    // LobRecordFanout::Settings fanout_setting;
    uint32_t     sample_interval_ms; // orb price list 采样时间
    uint32_t     sample_px_depth;   // lob px depth
};


class LobService:public IFeedUser {
public:
    bool init(const std::string& configfile);
    bool start();
    void stop();
    bool getPxList(symbolid_t symbolid, size_t depth, lob_px_record_t &pxr);
    void savePxList(symbolid_t symbolid);
    static LobService& instance(){
        static LobService s;
        return s;
    }

    void waitForShutdown();
    log4cplus::Logger& getLogger()  { return logger_;}
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
    void onFanoutTimed();
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
    
    std::vector<LobRecordFanout::Ptr> fanouts_;
    log4cplus::Logger logger_;
    LobTimer timer_;
};


#endif