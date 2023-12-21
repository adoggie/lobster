
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

LobRecordFanout::LobRecordFanout(const LobRecordFanout::Settings& settings)
{
    config_ = settings;
}

bool LobRecordFanout::start() {
    stopped_.store(false);

    thread_ = std::thread([this]() {
        std::cout << "thread1 start" << std::endl;
        zmq::context_t context(1);
        zmq::socket_t socket(context, ZMQ_PUB);
        if( boost::algorithm::to_lower_copy(config_.mode) == "connect"){
            socket.connect(config_.server_addr.c_str());
        }else{
            socket.bind(config_.server_addr.c_str());
        }
        
        zmq::pollitem_t items[] = { {socket, 0, ZMQ_POLLIN, 0} };
        while (stopped_.load() == false) {
            zmq::poll(items, 1, std::chrono::milliseconds(1));

            if (items[0].revents & ZMQ_POLLIN) {
                // Data is available to be received
                zmq::message_t message;
                auto optret = socket.recv(message, zmq::recv_flags::dontwait); // .value_or((size_t)0);
                if (optret.value() <= 0) {
                    continue;
                }
                symbolid_t symbolid;
                lob_data_t* data = lob_data_alloc2((char*)message.data(), message.size());

                char* token;
                size_t num = 0;

                // mdl_id,UpdateTime,MDStreamID,SecurityID[3],SecurityIDSource, ...
                token = std::strtok(data->data, ",");
                while (token != NULL) {
                    // printf("%s\n", token);
                    num++;
                    if (num == 3) {
                        break;
                    }
                    token = strtok(NULL, ",");
                    
                }
                if (num != 3) {
                    continue;
                }
                try {
                    // symbolid = boost::lexical_cast<symbolid_t>(token);
                    symbolid = (symbolid_t)std::stoul(token);
                }catch (...) {
                    continue;
                }
                data->symbolid = symbolid;
                // data->user = (void*)"order";
                
                // std::cout << "Received: " << data << std::endl;
            }
        }
        std::cout << "thread: mdl zmq  stopped" << std::endl;
    });
    return true;
}

void LobRecordFanout::stop() {
    stopped_.store(true);
    thread_.join();
}

void LobRecordFanout::fanout( lob_px_record_t::Ptr & record){
    queue_.enqueue(record);
}