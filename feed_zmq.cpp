#include "feed_zmq.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>
#include <fstream>
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



bool Mdl_Zmq_Feed::init(const QJsonObject& json) {
    config_.server_addr = json.value("server_addr").toString("tcp://127.0.0.1:6379").toStdString();
    config_.mode = json.value("mode").toString("bind").toStdString();    
    return true;
}


bool Mdl_Zmq_Feed::start() {
    stopped_.store(false);

    thread_ = std::thread([this]() {
        std::cout << "thread1 start" << std::endl;
        zmq::context_t context(1);
        zmq::socket_t socket(context, ZMQ_SUB);
        if( boost::algorithm::to_lower_copy(config_.mode) == "connect"){
            socket.connect(config_.server_addr.c_str());
        }else{
            socket.bind(config_.server_addr.c_str());
        }
        socket.set(zmq::sockopt::subscribe, "");

        // http://czmq.zeromq.org/
    // https://github.com/zeromq/cppzmq

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
                std::string text = std::string((char*)message.data(),message.size());
                auto ss = lobutils::splitString( text,',');
                if(ss.size() == 0){
                    continue;
                }
                try {
                    if( ss.back() == "mdl_4_24"){
                        symbolid = (symbolid_t)std::stoul( ss[2]);
                    }else if( ss.back() == "mdl_6_33" || ss.back() == "mdl_6_36"){
                        symbolid = (symbolid_t) std::stoul(ss[3]);
                    }else{
                        continue;
                    }
                }catch (std::exception& e ) {
                    qWarning() << "mdl_data error:" << text.c_str() ;
                    continue;
                }
                lob_data_t* data = lob_data_alloc2((char*)message.data(), message.size());
                data->symbolid = symbolid;
                onFeedRawData(data);
            }
        }
        std::cout << "thread: mdl zmq  stopped" << std::endl;
    });
    return true;
}

void Mdl_Zmq_Feed::stop() {
    stopped_.store(true);
    thread_.join();
}



Message* Mdl_Zmq_Feed::DataDecoder::decode(lob_data_t* data)
{
    Message* msg = nullptr;
    try {
        std::string text = std::string(data->data, data->len);
        auto ss = lobutils::splitString(text, ',');
        if(text.size() == 0){
            return nullptr;
        }
        std::string &mdl_id = ss.back();

        LOG4CPLUS_DEBUG(getLogger(), text);

        //std::tie(a, b, c, std::ignore) = std::make_tuple(list[0], list[1], list[2], std::vector<int>(list.begin() + 3, list.end()));
        if (mdl_id == std::string("mdl_4_24")) { // SH order  竞价逐笔合并行情 (mdl.4.24)
            // BizIndex,Channel,SecurityID,TickTime,Type,BuyOrderNO,SellOrderNO,Price,Qty,TradeMoney,TickBSFlag,LocalTime,SeqNo,MdlType
            msg = decodeSH(ss, OrdTrdType::ORDTRD);
        } else if (mdl_id == "mdl_6_33") { //逐笔委托行情 (mdl.6.33) 深交所
            msg = decodeSZ(ss, OrdTrdType::ORD);
        } else if (mdl_id == "mdl_6_36") {  // 逐笔成交行情 (mdl.6.36) 深交所
            auto trdmsg = new TradeMessage();
            msg = decodeSZ(ss, OrdTrdType::TRD);
        }

        if (msg) {
            if (mdl_id == "mdl_6_33" || mdl_id == "mdl_6_36") {
                msg->SecurityIDSource = Message::Source::SZ;
                msg->ChannelNo = (uint16_t) std::stoul(ss[1]);
                msg->ApplSeqNum = std::stoull(ss[0]);
            }
        }
    }catch(std::exception& e){
        qWarning() << QString("Mdl_Zmq_Feed::Decode() Error: %1").arg(e.what());
    }
    return msg;
}

