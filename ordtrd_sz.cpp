
#include "service.h"
#include <QtCore>


void LobService::onOrderSZ(const  OrderMessage* m){

    // std::unique_lock<RwMutex> lock(pxlist->mtx);
    symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    uint32_t p = m->ssz_ord.Price - pxlist->low;
    
    qDebug("onOrderSZ: '%c' Side: %c P:%d", m->ssz_ord.OrdType, m->ssz_ord.Side , p ) ;

    if( m->ssz_ord.Price < pxlist->low || m->ssz_ord.Price > pxlist->high){
        qWarning() << "onOrderSZ: invalid quote data. Price not in rage: ("<< pxlist->low << " , "<< pxlist->high << ")";
        return ;
    }

    // if(m->ssz_ord.OrdType == SBE_SSH_ord_t::OrdType::Add){ // 新增委托        
    {
        if(m->ssz_ord.Side == SBE_SSZ_ord_t::Side::Buy){  // buy
            int64_t seq = m->ssz_ord.OrderNo;
            pxlist->seq_pxs[seq] = std::make_tuple( pxlist->bids + p , lob_bs_t::BUY);

            pxlist->bids[p].qty += m->ssz_ord.OrderQty;
            pxlist->bids[p].ordTime = m->ssz_ord.TransactTime;
            if(pxlist->bid1 == nullptr){
                pxlist->bid1 = pxlist->bids + p;                
            }else{
                if( (pxlist->bids + p) > pxlist->bid1){ // 最新bid 高于上一bid，移动bid游标
                    pxlist->bid1 = pxlist->bids + p;
                }
            }             
        }else if( m->ssz_ord.Side == SBE_SSZ_ord_t::Side::Sell){ // sell
            int64_t seq = m->ssz_ord.OrderNo;
            pxlist->seq_pxs[seq] = std::make_tuple( pxlist->asks + p , lob_bs_t::SELL)  ;

            pxlist->asks[p].qty += m->ssz_ord.OrderQty;
            pxlist->asks[p].ordTime = m->ssz_ord.TransactTime;
            if( pxlist->ask1 == nullptr){
                pxlist->ask1 = pxlist->asks + p;                
            }else{
                if ((pxlist->asks + p) < pxlist->ask1){
                    pxlist->ask1 = pxlist->asks + p;
                }
            }
        }        
    }
     
}

/// @brief 成交回报
/// @param m
void LobService::onTradeSZ(const  TradeMessage* m){
     symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    uint32_t p = m->ssh_trd.LastPx - pxlist->low;
    
    qDebug("onTradeSZ: Side: %c", m->ssz_trd.ExecType);

/**
 2014,32669623,011,0,26362719,000046,102 ,0.0000,20100,52,14:56:40.450,14:56:40.460,72005444,
2014,32671522,011,0,13032875,000046,102 ,0.0000,3800,52,14:56:41.190,14:56:41.201,72009830,
2014,32672944,011,0,22132389,000046,102 ,0.0000,11900,52,14:56:41.870,14:56:41.876,72013521,
2014,32674968,011,0,27740950,000046,102 ,0.0000,24000,52,14:56:42.760,14:56:42.772,72018556,
2014,32676187,011,0,8785785,000046,102 ,0.0000,20000,52,14:56:43.300,14:56:43.312,72021708,
2014,32678469,011,0,21329520,000046,102 ,0.0000,356500,52,14:56:44.180,14:56:44.186,72027085,
2014,32678922,011,0,29965557,000046,102 ,0.0000,1600,52,14:56:44.430,14:56:44.439,72028630,

撤单： 52 时，价格均为0  

 */
    if( m->ssz_trd.ExecType == SBE_SSZ_exe_t::ExecType::Cancel){ // 撤单
        // if(m->ssz_trd.Side ==SBE_SSH_ord_t::Side::Buy){
        if( m->ssz_trd.BidApplSeqNum > 0){ // buy okay
            decltype(pxlist->seq_pxs)::iterator it;
            it = pxlist->seq_pxs.find(m->ssz_trd.BidApplSeqNum);
            if(it != pxlist->seq_pxs.end()){
                auto value = it->second;
                auto px = std::get<0>(value);
                px->qty -= m->ssz_trd.LastQty;
                px->ordTime = m->ssz_trd.TransactTime;
                if( px->qty.load() <= 0){
                    pxlist->seq_pxs.erase(it);
                }
                if( px == pxlist->bid1){
                    pxlist->bid1 = nullptr;
                    while( px >= pxlist->bids){
                        if( px->qty.load() > 0){
                            pxlist->bid1 = px;
                            break;
                        }
                        px --;
                    }             
                }
            }

            // pxlist->bids[p].qty -= m->ssz_trd.BidApplSeqNum;
            // pxlist->bids[p].ordTime = m->ssz_trd.TransactTime;
            // pxlist->bids[p].qty = std::max(0,pxlist->bids[p].qty.load()); //TODO
            // {
            //     if( pxlist->bids + p == pxlist->bid1){ // 删除的是最新bid
            //         if( pxlist->bids[p].qty.load() <= 0){    // TODO bid1.qty is 0 , move bid1 to next bid validable
            //             auto   px = pxlist->bid1 -1;
            //             pxlist->bid1 = nullptr;
            //             while( px >= pxlist->bids){
            //                 if( px->qty.load() > 0){
            //                     pxlist->bid1 = px;
            //                     break;
            //                 }
            //                 px --;
            //             }                        
            //         }
            //     }
            // }
        } 
        if( m->ssz_trd.OfferApplSeqNum > 0 ){
            decltype(pxlist->seq_pxs)::iterator it;
            it = pxlist->seq_pxs.find(m->ssz_trd.OfferApplSeqNum);
            if(it != pxlist->seq_pxs.end()){
                auto value = it->second;
                lob_px_t* px = std::get<0>(value);
                px->qty -= m->ssz_trd.LastQty;
                px->ordTime = m->ssz_trd.TransactTime;
                if( px->qty.load() <= 0){
                    pxlist->seq_pxs.erase(it);
                }
                if( px == pxlist->ask1){

                    px = pxlist->ask1 + 1;
                    pxlist->ask1 = nullptr;
                    while( px < pxlist->asks + pxlist->get_px_span()){
                        if( px->qty.load() > 0){
                            pxlist->ask1 = px;
                            break;
                        }
                        px ++;
                    }            
                }
            }

            // pxlist->asks[p].qty -= m->ssz_trd.OfferApplSeqNum;  
            // pxlist->asks[p].ordTime = m->ssz_trd.TransactTime;
            // pxlist->asks[p].qty = std::max(0,pxlist->asks[p].qty.load()); 
            // {
            //     if( pxlist->asks + p == pxlist->ask1){ // 删除的是最新bid
            //         if( pxlist->asks[p].qty.load() <= 0){    // ask1.qty is 0 , move ask1 to next ask validable
            //             auto   px = pxlist->ask1 +1;             
            //             pxlist->ask1 = nullptr;
            //             while( px < pxlist->asks + pxlist->get_px_span()){
            //                 if( px->qty.load() > 0){
            //                     pxlist->ask1 = px;
            //                     break;
            //                 }
            //                 px ++;
            //             }                        
            //         }
            //     }
            // }
        }
    }


    if( m->ssz_trd.ExecType == SBE_SSZ_exe_t::ExecType::Trade){ // 成交
        if( m->ssz_trd.LastPx < pxlist->low || m->ssz_trd.LastPx > pxlist->high){
            qWarning() << "onTradeSZ: invalid quote data. Price not in rage: ("<< pxlist->low << " , "<< pxlist->high << ")";
            return ;
        }

        decltype(pxlist->seq_pxs)::iterator it;
        if(  m->ssz_trd.BidApplSeqNum > 0){ // buy okay
            it = pxlist->seq_pxs.find(m->ssz_trd.BidApplSeqNum);
            if(it != pxlist->seq_pxs.end()){
                auto value = it->second;
                auto px = std::get<0>(value);
                px->qty -= m->ssz_trd.LastQty;
                px->ordTime = m->ssz_trd.TransactTime;
                if(px->qty <= 0 ){
                    pxlist->seq_pxs.erase(it);                                       
                    pxlist->bid1 = nullptr;
                    while( px >= pxlist->bids){
                        if( px->qty.load() > 0){
                            pxlist->bid1 = px;
                            break;
                        }
                        px --;
                    }                        
                }else{
                    pxlist->bid1 = px;
                }
            }                      
        }
        if( m->ssz_trd.OfferApplSeqNum > 0){
            it = pxlist->seq_pxs.find(m->ssz_trd.OfferApplSeqNum);  
            if(it != pxlist->seq_pxs.end()){
                auto value = it->second;
                auto px = std::get<0>(value);
                px->qty -= m->ssz_trd.LastQty;
                px->ordTime = m->ssz_trd.TransactTime;
                if(px->qty <= 0 ){
                    pxlist->seq_pxs.erase(it);
                    pxlist->ask1 = nullptr; 
                    while( px < pxlist->asks + pxlist->get_px_span()){
                        if( px->qty.load() > 0){
                            pxlist->ask1 = px;
                            break;
                        }
                        px ++;
                    }
                }else{
                    pxlist->ask1 = px;
                }
            }             
        }        
    }
}
