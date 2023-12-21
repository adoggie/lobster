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


#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
// #include <czmq.h>
#include <zmq.hpp>

#include "lob.h"
#include "strutils.h"



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
                onFeedRawData(data);
                // std::cout << "Received: " << data << std::endl;
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


Message * Mdl_Zmq_Feed::DataDecoder::decodeSH(const std::vector<std::string>& ss,OrdTrdType ordtrd){
    Message* msg = nullptr;
    auto& TickBSFlag = ss[10];
    auto& BuyOrderNO = ss[5];
    auto& SellOrderNO = ss[6];
    auto& Price = ss[7];
    auto& Qty = ss[8];
    auto& TradeMoney = ss[9];

    if( ss[4] == "A" || ss[4] == "D"){
        OrderMessage* ordmsg = new OrderMessage();
        ordmsg->ssh_ord.OrdType = ss[4] == "A" ? 'A' : 'D';
        if( ss[4] == "A"){ //若为产品状态订单、删除订单无意义
            ordmsg->ssh_ord.Price = std::stof(Price)*100;
        }
        ordmsg->ssh_ord.OrderQty = std::stoll(Qty);
        if( TickBSFlag == "B" ){ // buy
            ordmsg->ssh_ord.Side = 'B';
            ordmsg->ssh_ord.OrderNo = std::stoll(BuyOrderNO);
        }else if (TickBSFlag == "S"){
            ordmsg->ssh_ord.Side = 'S';
            ordmsg->ssh_ord.OrderNo = std::stoll(SellOrderNO);
        }
        msg = ordmsg;
    }else if (ss[4] == "T") {
        TradeMessage* trdmsg = new TradeMessage();
        trdmsg->ssh_trd.LastPx = std::stof(Price)*100;
        trdmsg->ssh_trd.LastQty = std::stoll(Qty);
        trdmsg->ssh_trd.TradeMoney = std::stod(TradeMoney);
        trdmsg->ssh_trd.TradeBSFlag = TickBSFlag[0];
        trdmsg->ssh_trd.TradeBuyNo = std::stoll(BuyOrderNO);                
        trdmsg->ssh_trd.TradeSellNo = std::stoll(SellOrderNO);                            
        msg = trdmsg;
    }

    msg->SecurityIDSource = Message::Source::SH;
    msg->ChannelNo = (uint16_t)std::stoul(ss[1]);
    msg->ApplSeqNum = std::stoull(ss[0]);
    return msg ;
}

Message * Mdl_Zmq_Feed::DataDecoder::decodeSZ(const std::vector<std::string>& ss,OrdTrdType ordtrd){
    Message * msg = nullptr;
    if( ordtrd == OrdTrdType::ORD){
        auto ordmsg = new OrderMessage();

        msg = ordmsg;
    }else if (ordtrd == OrdTrdType::TRD){
        auto trdmsg = new TradeMessage();
        msg = trdmsg;
    }
    return msg;
}

Message* Mdl_Zmq_Feed::DataDecoder::decode(lob_data_t* data)
{
    Message* msg = nullptr;

    auto ss = lobutils::splitString(std::string(data->data, data->len), ',');
    std::string& mdl_id = ss.back();
    
    //std::tie(a, b, c, std::ignore) = std::make_tuple(list[0], list[1], list[2], std::vector<int>(list.begin() + 3, list.end()));
    if (mdl_id == "mdl_4_24") { // SH order  竞价逐笔合并行情 (mdl.4.24)
        // BizIndex,Channel,SecurityID,TickTime,Type,BuyOrderNO,SellOrderNO,Price,Qty,TradeMoney,TickBSFlag,LocalTime,SeqNo,MdlType
        msg = decodeSH(ss,OrdTrdType::ORDTRD);
    }
    else if (mdl_id ==  "mdl_6_33"  ){ //逐笔委托行情 (mdl.6.33) 深交所
        msg = decodeSZ(ss,OrdTrdType::ORD);
    }else if (mdl_id == "mdl_6_36"){  // 逐笔成交行情 (mdl.6.36) 深交所
        auto trdmsg = new TradeMessage();
        msg = decodeSZ(ss,OrdTrdType::TRD);
    }

    if (msg) {
        if( mdl_id == "mdl_6_33" || mdl_id == "mdl_6_36"){
            msg->SecurityIDSource = Message::Source::SZ;
            msg->ChannelNo = (uint16_t)std::stoul(ss[1]);
            msg->ApplSeqNum = std::stoull(ss[0]);
        }        
    }
    return msg;
}

Mdl_Zmq_Feed::Mdl_Zmq_Feed(const zmq_feed_setting_t & config)
{
    config_ = config;
}
