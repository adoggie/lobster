
#include "fanout.h"

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

// LobRecordFanout::LobRecordFanout(const LobRecordFanout::Settings& settings)
// {
//     config_ = settings;
// }

bool LobRecordFanout::init(const QJsonObject& settings){
    return false;
}

bool LobRecordFanout::start() {
    return false;
}

void LobRecordFanout::stop() {
    stopped_.store(true);
    thread_.join();
}

void LobRecordFanout::fanout( lob_px_record_t::Ptr & record){
    queue_.enqueue(record);
}