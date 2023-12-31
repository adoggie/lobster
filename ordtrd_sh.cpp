
#include "service.h"


void LobService::onOrderSH(const  OrderMessage* m){

    // std::unique_lock<RwMutex> lock(pxlist->mtx);
    symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    uint32_t p = m->ssh_ord.Price - pxlist->low;

    qDebug("onOrderSH: '%c' Side: %c", m->ssh_ord.OrdType, m->ssh_ord.Side);

    if(m->ssh_ord.OrdType == SBE_SSH_ord_t::OrdType::Add){ // 新增委托        
        if(m->ssh_ord.Side == SBE_SSH_ord_t::Side::Buy){  // buy
            int64_t seq = m->ssh_ord.OrderNo;
            pxlist->seq_pxs[seq] = std::make_tuple( pxlist->bids + p , lob_bs_t::BUY);

            pxlist->bids[p].qty += m->ssh_ord.OrderQty;
            pxlist->bids[p].ordTime = m->ssh_ord.OrderTime;
            if(pxlist->bid1 == nullptr){
                pxlist->bid1 = pxlist->bids + p;                
            }else{
                if( (pxlist->bids + p) > pxlist->bid1){ // 最新bid 高于上一bid，移动bid游标
                    pxlist->bid1 = pxlist->bids + p;
                }
            }             
        }else if( m->ssh_ord.Side == SBE_SSH_ord_t::Side::Sell){ // sell
            int64_t seq = m->ssh_ord.OrderNo;
            pxlist->asks[p].qty += m->ssh_ord.OrderQty;
            pxlist->seq_pxs[seq] = std::make_tuple( pxlist->asks + p , lob_bs_t::SELL) ;


            pxlist->asks[p].ordTime = m->ssh_ord.OrderTime;
            if( pxlist->ask1 == nullptr){
                pxlist->ask1 = pxlist->asks + p;                
            }else{
                if ((pxlist->asks + p) < pxlist->ask1){
                    pxlist->ask1 = pxlist->asks + p;
                }
            }
        }        
    }else if( m->ssh_ord.OrdType == SBE_SSH_ord_t::OrdType::Del){ // 删除委托单
        if(m->ssh_ord.Side ==SBE_SSH_ord_t::Side::Buy){
            pxlist->bids[p].qty -= m->ssh_ord.OrderQty;
            pxlist->bids[p].ordTime = m->ssh_ord.OrderTime;
            pxlist->bids[p].qty = std::max(0,pxlist->bids[p].qty.load());
            {
                if( pxlist->bids + p == pxlist->bid1){ // 删除的是最新bid
                    if( pxlist->bids[p].qty.load() <= 0){    // bid1.qty is 0 , move bid1 to next bid validable
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
        }else if( m->ssh_ord.Side == SBE_SSH_ord_t::Side::Sell){
            pxlist->asks[p].qty -= m->ssh_ord.OrderQty;  
            pxlist->asks[p].ordTime = m->ssh_ord.OrderTime;
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
}

/// @brief 成交回报"T" ,消除委托单的委托量
/// @param m
void LobService::onTradeSH(const  TradeMessage* m){
     symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    // uint32_t p = m->ssh_trd.LastPx - pxlist->low;
    
    qDebug("onTradeSH: 'T' Side: %c", m->ssh_trd.TradeBSFlag);
    
    if( m->ssh_trd.TradeBSFlag == 'B' || m->ssh_trd.TradeBSFlag == 'S'){ // buy
        decltype(pxlist->seq_pxs)::iterator it;
        if( m->ssh_trd.TradeBuyNo > 0){
            it = pxlist->seq_pxs.find(m->ssh_trd.TradeBuyNo);
            if(it != pxlist->seq_pxs.end()){
                auto value = it->second;
                auto px = std::get<0>(value);
                px->qty -= m->ssh_trd.LastQty;
                px->ordTime = m->ssh_trd.TradeTime;
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
        if( m->ssh_trd.TradeSellNo > 0){
            it = pxlist->seq_pxs.find(m->ssh_trd.TradeSellNo);  
            if(it != pxlist->seq_pxs.end()){
                auto value = it->second;
                auto px = std::get<0>(value);
                px->qty -= m->ssh_trd.LastQty;
                px->ordTime = m->ssh_trd.TradeTime;
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
