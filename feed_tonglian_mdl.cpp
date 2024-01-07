#include "feed_tonglian_mdl.h"
#include "strutils.h"
#include "utils/logger.h"

namespace tonglian{


Message* DataDecoder::decode(lob_data_t* data)
{
    Message* msg = nullptr;
    try {
        std::string text = std::string(data->data, data->len);
        auto ss = lobutils::splitString(text, ',');
        if(text.size() == 0){
            return nullptr;
        }
        std::string &mdl_id = ss.back();

        LOG4CPLUS_DEBUG(getLogger(), text.c_str());

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
        LOG4CPLUS_DEBUG(getLogger("error") ,"Mdl_Zmq_Feed::Decode() Error: " << e.what());
    }
    return msg;
}





Message * DataDecoder::decodeSH(const std::vector<std::string>& ss,OrdTrdType ordtrd){
    Message* msg = nullptr;
    auto& TickBSFlag = ss[10];
    auto& BuyOrderNO = ss[5];
    auto& SellOrderNO = ss[6];
    auto& Price = ss[7];
    auto& Qty = ss[8];
    auto& TradeMoney = ss[9];

    if( ss[4][0] == SBE_SSH_ord_t::OrdType::Add || ss[4][0] == SBE_SSH_ord_t::OrdType::Del){
        OrderMessage* ordmsg = new OrderMessage();
        ordmsg->ssh_ord.OrdType = ss[4][0] ; // == "A" ? 'A' : 'D';
//        if( ss[4] == "A"){ //若为产品状态订单、删除订单无意义
            ordmsg->ssh_ord.Price = std::stof(Price)*100;
//        }
        ordmsg->ssh_ord.OrderQty = std::stoll(Qty);
        if( TickBSFlag[0] == SBE_SSH_ord_t::Side::Buy ){ // buy
            ordmsg->ssh_ord.Side = TickBSFlag[0];
            ordmsg->ssh_ord.OrderNo = std::stoll(BuyOrderNO);
        }else if (TickBSFlag[0] == SBE_SSH_ord_t::Side::Sell){
            ordmsg->ssh_ord.Side = TickBSFlag[0];
            ordmsg->ssh_ord.OrderNo = std::stoll(SellOrderNO);
        }
        msg = ordmsg;
    }else if (ss[4][0] == SBE_SSH_ord_t::OrdType::Trade ) {
        TradeMessage* trdmsg = new TradeMessage();
        trdmsg->ssh_trd.LastPx = std::stof(Price)*100;
        trdmsg->ssh_trd.LastQty = std::stoll(Qty);
        trdmsg->ssh_trd.TradeMoney = std::stod(TradeMoney);
        trdmsg->ssh_trd.TradeBSFlag = TickBSFlag[0];
        trdmsg->ssh_trd.TradeBuyNo = std::stoll(BuyOrderNO);                
        trdmsg->ssh_trd.TradeSellNo = std::stoll(SellOrderNO);                            
        msg = trdmsg;
    }
    if(msg) {
        msg->SecurityID = std::stoul(ss[2]);
        msg->SecurityIDSource = Message::Source::SH;
        msg->ChannelNo = (uint16_t) std::stoul(ss[1]);
        msg->ApplSeqNum = std::stoull(ss[0]);
    }
    return msg ;
}

Message * DataDecoder::decodeSZ(const std::vector<std::string>& ss,OrdTrdType ordtrd){
    Message * msg = nullptr;
    if( ordtrd == OrdTrdType::ORD){
        auto ordmsg = new OrderMessage();
        ordmsg->ssz_ord.OrderNo = std::stoll(ss[1]);
        ordmsg->ssz_ord.Price = std::stof(ss[5])*100;
        ordmsg->ssz_ord.OrderQty = std::stoll(ss[6]);
        ordmsg->ssz_ord.Side = (int8_t)std::stol(ss[7]);
        ordmsg->ssz_ord.TransactTime = std::stoul(ss[8]);
        ordmsg->ssz_ord.OrdType = (int8_t)std::stol(ss[9]);        
        msg = ordmsg;
    }else if (ordtrd == OrdTrdType::TRD){
        auto trdmsg = new TradeMessage();
        trdmsg->ssz_trd.BidApplSeqNum = std::stoll(ss[3]);
        trdmsg->ssz_trd.OfferApplSeqNum = std::stoll(ss[4]);
        trdmsg->ssz_trd.LastPx = std::stof(ss[7])*100;
        trdmsg->ssz_trd.LastQty = std::stoll(ss[8]);
        trdmsg->ssz_trd.ExecType = (int8_t)std::stol(ss[9]);
        trdmsg->ssz_trd.TransactTime = std::stoul(ss[10]);
        msg = trdmsg;
    }
    if(msg) {
        if( ordtrd == OrdTrdType::ORD) {
            msg->SecurityID = std::stoul(ss[3]);
        }else if( ordtrd == OrdTrdType::TRD){
            msg->SecurityID = std::stoul(ss[5]);
        }
        msg->SecurityIDSource = Message::Source::SZ;
        msg->ChannelNo = (uint16_t) std::stoul(ss[0]);
        msg->ApplSeqNum = std::stoull(ss[1]);
    }
    return msg;
}

}