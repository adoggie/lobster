#include "feed_zmq.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>
#include <fstream>
#include <log4cplus/loggingmacros.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <qlogging.h>
#include <QtCore>
#include "utils/logger.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
// #include <czmq.h>
#include <zmq.hpp>

#include "lob.h"
#include "strutils.h"
#include "feed_tonglian_mdl_redis.h"
#include "utils/logger.h"



bool Mdl_Redis_Feed::start() {
    stopped_.store(false);
    redis_ = std::make_shared<sw::redis::Redis>(config_.server_addr);
    thread_ = std::thread([this]() {        
        LOG4CPLUS_INFO(getLogger(), "Mdl_Redis_Feed:: thread start" );
        auto sub = redis_->subscriber();

        sub.on_pmessage([]( std::string pattern,std::string channel, std::string message) {
        });

        sub.on_message([this](std::string channel, std::string message) {
            symbolid_t symbolid;
            message = std::string((char*)message.data(),message.size());
            message = lobutils::trim(message);
            auto ss = lobutils::splitString( message,',');
            if(ss.size() == 0){
                return ;
            }
            try {
                if (channel.find("mdl.4.24") != channel.npos) {
                    symbolid = (symbolid_t) std::stoul(ss[2]);
                    channel = "mdl_4_24";
                } else if (channel.find("mdl.6.33") != channel.npos || channel.find("mdl.6.36")!= channel.npos) {
                    symbolid = (symbolid_t) std::stoul(ss[3]);
                    if(channel.find("mdl.6.33") != channel.npos){
                        channel = "mdl_6_33";
                    }
                    if( channel.find("mdl.6.36") != channel.npos){
                        channel = "mdl_6_36";
                    }
                } else {
                    return;;
                }

            }catch (std::exception& e ) {
                LOG4CPLUS_DEBUG( getLogger("error"), "mdl_data error:" << message.c_str() );
                return;
            }
            message += "," + channel;
            lob_data_t* data = lob_data_alloc2((char*)message.data(), message.size());
            data->symbolid = symbolid;
            onFeedRawData(data);
        });
         sub.subscribe(config_.subs.begin(),config_.subs.end());
//         sub.subscribe("mdl.4.24.*");
//        sub.psubscribe("mdl.4.24.*");
        while (stopped_.load() == false) {
            try {
                sub.consume();
            }catch( sw::redis::Error & e){
                LOG4CPLUS_ERROR(getLogger("error"),"Mdl_Redis_Feed::thread Error: " << e.what());
            }
//            }catch(std::exception& e){
//                LOG4CPLUS_ERROR(getLogger("error"),"Mdl_Redis_Feed::thread Error: " << e.what());
//            }
        }
        std::cout << "thread: mdl zmq  stopped" << std::endl;
    });
    return true;
}


void Mdl_Redis_Feed::stop() {
    stopped_.store(true);
    thread_.join();
}

bool Mdl_Redis_Feed::init(const QJsonObject& json) {
    config_.server_addr = json.value("server_addr").toString("tcp://127.0.0.1:6379").toStdString();
    config_.auth = json.value("auth").toString("").toStdString();   
    for(auto  it : json.value("subs").toArray()){
        config_.subs.push_back( it.toString().toStdString());
    }
    return true;
}