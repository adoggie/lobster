
#ifndef _FEED_CSV_H
#define _FEED_CSV_H

#include "feed.h"
#include "lob.h"
#include <atomic>
#include <thread>

class Mdl_Csv_Feed : public FeedBase{
public:
    struct DataDecoder:IFeedDataDecoder{
        Message* decode(lob_data_t * data);
    };
public:
    Mdl_Csv_Feed(const mdl_csv_feed_setting_t & config);
    ~Mdl_Csv_Feed();
    bool start();
    void stop();
protected:
    Message::Ptr parse(const std::string & line);

private:
    std::atomic<bool> stopped_;
    mdl_csv_feed_setting_t config_;
    std::thread  thread1_,thread2_;
};


#endif // !_FEED_CSV_H