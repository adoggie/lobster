
#ifndef _FEED_CSV_H
#define _FEED_CSV_H

#include "feed.h"
#include "lob.h"
#include <atomic>
#include <thread>

class Mdl_Csv_Feed : public FeedBase{
public:
    struct Settings {
        std::string datadir; //
        uint32_t  speed; // 1 - 100
    };

    struct DataDecoder:IFeedDataDecoder{
        Message* decode(lob_data_t * data);
    };
public:
    bool init(const QJsonObject& settings) ; 
    bool start();
    void stop();
    Mdl_Csv_Feed() = default;
protected:
    Message::Ptr parse(const std::string & line);

private:
    std::atomic<bool> stopped_;
    Settings config_;
    std::thread  thread1_,thread2_;
};


#endif // !_FEED_CSV_H