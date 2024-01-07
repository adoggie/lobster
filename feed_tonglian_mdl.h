#ifndef _FEED_TONGLIANG_MDL_H
#define _FEED_TONGLIANG_MDL_H


#include "feed.h"

namespace tonglian{
    enum OrdTrdType{
        ORD = 0,
        TRD = 1,
        ORDTRD = 2
    };

    struct DataDecoder:IFeedDataDecoder{
        virtual Message* decode(lob_data_t* data);
        virtual Message * decodeSH(const std::vector<std::string>& ss, OrdTrdType ordtrd);
        virtual Message * decodeSZ(const std::vector<std::string>& ss, OrdTrdType ordtrd);
    };
}

#endif