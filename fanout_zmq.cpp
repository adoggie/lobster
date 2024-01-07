
#include "fanout.h"

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
// #include <czmq.h>
#include <zmq.hpp>

#include "lob.h"
#include "fanout_zmq.h"
#include "utils/logger.h"


bool LobRecordFanoutZMQ::init(const QJsonObject& json) {
    config_.server_addr = json.value("server_addr").toString("tcp://127.0.0.1:5556").toStdString();
    config_.mode = json.value("mode").toString("bind").toStdString();
    config_.enable = json.value("enable").toBool(false);
    return true;
}


bool LobRecordFanoutZMQ::start() {    
    stopped_.store(false);

    thread_ = std::thread([this]() {
        std::cout << "LobRecordFanoutZMQ::thread start" << std::endl;
        zmq::context_t context(1);
        zmq::socket_t socket(context, ZMQ_PUB);
        if( boost::algorithm::to_lower_copy(config_.mode) == "connect"){
            socket.connect(config_.server_addr.c_str());
        }else{
            socket.bind(config_.server_addr.c_str());
        }
                
        while (stopped_.load() == false) {
            lob_px_record_t::Ptr px_record;
            if (!queue_.dequeue(px_record)) {
                continue;
            }
            msgpack::sbuffer buffer;
            msgpack::pack(buffer, *px_record);

            std::string symbolid = std::to_string(px_record->symbolid);
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(6) << symbolid;
            symbolid = oss.str();

            zmq::message_t topic(symbolid.c_str(),symbolid.size());
            zmq::message_t message(buffer.data(), buffer.size());
            socket.send(topic,zmq::send_flags::sndmore);
            socket.send(message,zmq::send_flags::none);
        }
        std::cout << "LobRecordFanoutZMQ::thread:   stopped" << std::endl;
    });
    return true;
}

void LobRecordFanoutZMQ::stop() {
    stopped_.store(true);
    thread_.join();
}

