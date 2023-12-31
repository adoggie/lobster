
#include "service.h"


void LobService::onOrderSZ(const  OrderMessage* m){

    // std::unique_lock<RwMutex> lock(pxlist->mtx);
    symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    uint32_t p = m->ssz_ord.Price - pxlist->low;
    
    qDebug("onOrderSZ: '%c' Side: %c", m->ssz_ord.OrdType, m->ssz_ord.Side);

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

    if( m->ssz_trd.ExecType == SBE_SSZ_exe_t::ExecType::Cancel){ // 撤单
        // if(m->ssz_trd.Side ==SBE_SSH_ord_t::Side::Buy){
        if( m->ssz_trd.BidApplSeqNum > 0){ // buy okay
            pxlist->bids[p].qty -= m->ssz_trd.BidApplSeqNum;
            pxlist->bids[p].ordTime = m->ssz_trd.TransactTime;
            pxlist->bids[p].qty = std::max(0,pxlist->bids[p].qty.load()); //TODO
            {
                if( pxlist->bids + p == pxlist->bid1){ // 删除的是最新bid
                    if( pxlist->bids[p].qty.load() <= 0){    // TODO bid1.qty is 0 , move bid1 to next bid validable
//                    if( pxlist->bids[p].qty <= 0){    // bid1.qty is 0 , move bid1 to next bid validable
                        auto   px = pxlist->bid1 -1;
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
            }
        } 
        if( m->ssz_trd.OfferApplSeqNum > 0 ){
            pxlist->asks[p].qty -= m->ssz_trd.OfferApplSeqNum;  
            pxlist->asks[p].ordTime = m->ssz_trd.TransactTime;
            pxlist->asks[p].qty = std::max(0,pxlist->asks[p].qty.load()); 
            {
                if( pxlist->asks + p == pxlist->ask1){ // 删除的是最新bid
                    if( pxlist->asks[p].qty.load() <= 0){    // ask1.qty is 0 , move ask1 to next ask validable
                        auto   px = pxlist->ask1 +1;             
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
            }
        }
    }
    if( m->ssz_trd.ExecType == SBE_SSZ_exe_t::ExecType::Trade){ // 成交
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
