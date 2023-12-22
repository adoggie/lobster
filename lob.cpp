
#include "lob.h"
#include <cstring>

// void parse_lob_config_from_jsonfile(const char* filename, lob_config_t* config){

// }


lob_px_list_t * lob_px_list_alloc(){
    lob_px_list_t *pxlist = new lob_px_list_t;
    std::memset(pxlist,0,sizeof(lob_px_list_t));
    pxlist->symbolid = 0;
    pxlist->low = 0 ;
    pxlist->high = 0;
    pxlist->ask1 = nullptr;
    pxlist->bid1 = nullptr;
    pxlist->bids = nullptr;
    pxlist->asks = nullptr;
    return pxlist;
}

void lob_px_list_free(lob_px_list_t* pxlist){
    delete pxlist;
}

lob_data_t* lob_data_alloc(size_t len){
    lob_data_t * lob = new lob_data_t;
    lob->data = new char[len];
    lob->len = len ;
    lob->symbolid = 0;
    lob->user = nullptr;
    return lob ;
}

lob_data_t* lob_data_alloc2(char * data ,size_t len){
    lob_data_t * lob = new lob_data_t;
    lob->data = new char[len];
    lob->len = len ;
    std::memcpy(lob->data,data,len);
    lob->symbolid = 0;
    lob->user = nullptr;
    return lob ;
}

void lob_data_free(lob_data_t* data){
    delete [] data->data;
    delete data;
}
