#include "feed_csv.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <string_view>
#include <ranges>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "lob.h"
#include "strutils.h"


Mdl_Csv_Feed::Mdl_Csv_Feed(const mdl_csv_feed_setting_t& config) :config_(config)
{

}

Mdl_Csv_Feed::~Mdl_Csv_Feed() {

}

bool Mdl_Csv_Feed::start() {
    stopped_.store(false);

    thread1_ = std::thread([this]() {
        std::cout << "thread1 start" << std::endl;
        // std::this_thread::sleep_for(std::chrono::seconds(1));
        std::string filename = config_.datadir + "/mdl_4.19.csv";
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }
        std::string line;
        while (std::getline(file, line)) {
            if (stopped_.load()) {                
                break;
            }
            if( config_.speed ){
                // std::chrono::duration<double> duration(1/config_.speed);
                std::this_thread::sleep_for(std::chrono::milliseconds( size_t(1000/config_.speed)));
            }
            
            symbolid_t symbolid;
            lob_data_t* data = lob_data_alloc2( (char*)line.c_str(),line.size());
            data->symbolid = symbolid;
            data->user = (void*)"order";
            onFeedRawData( data );           
        }

        file.close();
        std::cout << "thread1: mdl csv  stopped" << std::endl;
    });

    thread2_ = std::thread([this]() {
        std::cout << "thread2 start" << std::endl;
        // std::this_thread::sleep_for(std::chrono::seconds(1));
        std::string filename = config_.datadir + "/mdl_4.18.csv";
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }
        std::string line;
        while (std::getline(file, line)) {
            if (stopped_.load()) {                
                break;
            }
            if( config_.speed ){
                // std::chrono::duration<double> duration(1/config_.speed);
                std::this_thread::sleep_for(std::chrono::milliseconds( size_t(1000/config_.speed)));
            }
            
            symbolid_t symbolid;
            lob_data_t* data = lob_data_alloc2( (char*)line.c_str(),line.size());
            data->symbolid = symbolid;
            data->user = (void*)"trade";
            onFeedRawData( data );           
        }

        file.close();
        std::cout << "thread2: mdl csv  stopped" << std::endl;
    });
    return true;
}

void Mdl_Csv_Feed::stop() {
    stopped_.store(true);
    thread1_.join();
    thread2_.join();
}

Message* Mdl_Csv_Feed::DataDecoder::decode( lob_data_t * data)
{
    Message * msg = nullptr;
    if( data->user == (void*)"order"){
        auto ss = lobutils::splitString( std::string(data->data, data->len),',');
        auto ordmsg = new OrderMessage();
        ordmsg->SecurityID = boost::lexical_cast<uint32_t>(ss[3]);
        msg = ordmsg;

    }
    return msg ;
}
