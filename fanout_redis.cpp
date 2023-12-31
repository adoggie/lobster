

#include <QtCore>

#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>


#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "fanout_redis.h"

//https://github.com/sewenew/redis-plus-plus

bool LobRecordFanoutFile::init(const QJsonObject& settings){
    config_.server_addr = settings.value("server_addr").toString("tcp://127.0.0.1:6379").toStdString();

    return true ;
}

bool LobRecordFanoutFile::start() {
    stopped_.store(false);
    redis_ = sw::redis::Redis(config_.server_addr);

    thread_ = std::thread([this]() {
        std::cout << "LobRecordFanoutRedis::thread start" << std::endl;

        while( !stopped_.load()){
            lob_px_record_t::Ptr px_record;
            if (!queue_.dequeue(px_record)) {
                continue;
            }
            std::unordered_map<std::string, std::string> m ;
            
            std::string line;
            size_t num = px_record->asks.size();
            for(auto itr = px_record->asks.begin();itr != px_record->asks.end();itr++){
                line +=  std::to_string(std::get<0>(*itr)) + "," + std::to_string( std::get<1>(*itr)) ;
                if( itr !=std::prev(px_record->asks.end())){
                    line += ",";
                }
            }
            m["asks"] = line;

            line = "";            
            for(auto itr = px_record->bids.begin();itr != px_record->bids.end();itr++){
                line +=  std::to_string(std::get<0>(*itr)) + "," + std::to_string( std::get<1>(*itr)) ;
                if( itr !=std::prev(px_record->asks.end())){
                    line += ",";
                }
            }
            m["bids"] = line;
            redis_.hmset(std::to_string(px_record->symbolid), m.begin(), m.end());
        }
        std::cout << "LobRecordFanoutRedis::thread:   stopped" << std::endl;
    });
    return true;
}

void LobRecordFanoutFile::stop() {
    stopped_.store(true);
    thread_.join();
}
