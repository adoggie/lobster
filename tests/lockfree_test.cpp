
#include <thread>
#include <iostream>
#include "../lockqueue.h"
#include "../lob.h"



int main(int argc ,char** argv){
    LockQueue< lob_data_t* > q;
    std::thread thread = std::thread([&]{
        while(true){
            lob_data_t *  v = nullptr;
            if( q.dequeue(v) == false){
                continue;
            }
            std::cout<< "get v:" << v->len << std::endl;
            lob_data_free(v);
        }
    });
    uint64_t s = 0 ;

    while(true){
        std::string s = "15536363,3,688299,15:00:00.910,T,9599201,9617949,14.820,1400,20748.000,N,15:00:05.735,89753155,mdl_4_24";
        lob_data_t *data = lob_data_alloc2((char*)s.c_str(),s.size());
        q.enqueue(data);
    }
    return 0;
}