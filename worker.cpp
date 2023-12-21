#include <memory>
#include "worker.h"
#include "service.h"

void Worker::setDataDecoder(IFeedDataDecoder* decoder){
    decoder_ = decoder;
}

void Worker::enqueue( lob_data_t * data){
    dataqueue_.enqueue(data);
}

// here we can do some filter
void Worker::onOrder(const  OrderMessage::Ptr& m){

}

void Worker::onTrade(const  TradeMessage::Ptr& m){

}

void Worker::start(){  // NOLINT
    stopped_.store(false);
    thread_ = std::thread([this](){
        while(stopped_.load() == false){
            lob_data_t* data = nullptr;
            if(dataqueue_.dequeue(data) == false){
                continue;
            }
            Message* msg = decoder_->decode(data);            
            if(msg != nullptr){ // NOLINT
                LobService::instance().onMessage(msg); 
                delete msg ;      
            }
            lob_data_free(data);
        }   
    });

}

void Worker::stop(){
    stopped_.store(true);
    thread_.join();
}
