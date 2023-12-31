#include "service.h"


// 获取指定symbolid 的depth 长度的 订单簿 
bool LobService::getPxList(symbolid_t symbolid, size_t depth, lob_px_record_t &pxr){
    lob_px_list_t* pxlist = pxlive_[symbolid];
    std::shared_lock<RwMutex> lock(pxlist->mtx);
    // size_t asknum = (size_t) std::min(depth, static_cast<size_t>(pxlist->list.size()));
    pxr.symbolid = symbolid;
    if( pxlist->ask1 != nullptr){
        auto px = pxlist->ask1;
        px->px = pxlist->get_px(pxlist->ask1,pxlist->asks);
        while( px < pxlist->asks + pxlist->get_px_span()){
            if( px->qty.load() > 0){
                px->px = pxlist->get_px(px,pxlist->asks);
                pxr.asks.push_back( std::make_tuple(px->px, px->qty.load()));
                if( pxr.asks.size() >= depth){
                    break;
                }
            }
            px ++;
        }
    }

    if( pxlist->bid1!=nullptr){
        auto px = pxlist->bid1;
        px->px = pxlist->get_px(pxlist->bid1,pxlist->bids);
        while( px >= pxlist->bids){
            if( px->qty.load() > 0){
                px->px = pxlist->get_px(px,pxlist->bids);
                pxr.bids.push_back( std::make_tuple(px->px, px->qty.load()));
                if( pxr.bids.size() >= depth){
                    break;
                }
            }
            px --;
        }
    }
    
    return true;
}

// 获取指定symbolid 的depth 长度的 订单簿 
// bool LobService::getPxList(symbolid_t symbolid, size_t depth, lob_px_record_t &pxr){
    // lob_px_history_t* pxlist = pxhistory_[symbolid];
    // std::shared_lock<RwMutex> lock(pxlist->mtx);
    // int32_t shift = (int32_t) std::min(depth, static_cast<size_t>(pxlist->list.size()));
    // std::reverse_copy(pxlist->list.rbegin(), pxlist->list.rbegin() + shift, 
    //         std::back_inserter(list));
    // std::reverse_copy(pxlist->list.rbegin(), pxlist->list.rbegin() + (int32_t)1, 
    //         std::back_inserter(list));
    // std::transform( pxlist->list.rbegin(), pxlist->list.rbegin() + 
    //         std::min(depth, static_cast<int>(pxlist->list.size())), 
    //         std::back_inserter(list), [](lob_px_record_ptr& ptr) {
    //             return ptr;
    //         });
    // return true;
// }   
