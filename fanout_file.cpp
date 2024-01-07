
#include "fanout.h"

#include <QtCore>

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

#include "fanout_file.h"

bool LobRecordFanoutFile::init(const QJsonObject& settings){
    config_.store_dir = settings.value("store_dir").toString("data").toStdString();

    return true ;
}

bool LobRecordFanoutFile::start() {
    stopped_.store(false);

    thread_ = std::thread([this]() {
        std::cout << "LobRecordFanoutFile::thread start" << std::endl;
        while( !stopped_.load()){
            lob_px_record_t::Ptr px_record;
            if (!queue_.dequeue(px_record)) {
                continue;
            }
            auto itr = files_.find(px_record->symbolid);
            std::shared_ptr<QFile> file;
//            if( itr == files_.end()){
//                std::string filename = config_.store_dir + "/" + std::to_string(px_record->symbolid) + ".lob";
//                file = std::make_shared<QFile>(filename.c_str());
//                if( file->open(QIODevice::WriteOnly)){
//                    files_.insert( std::make_pair(px_record->symbolid, file));
//                }else{
//                    std::cout << "LobRecordFanoutFile::thread:   open file failed" << std::endl;
//                    continue;
//                }
//            }else{
//                file = itr->second;
//                if( !file->isOpen()){
//                    std::cout << "LobRecordFanoutFile::thread:   file is not open" << std::endl;
//                    continue;
//                }
//            }
            std::string symbolid = std::to_string(px_record->symbolid);
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(6) << symbolid;
            symbolid = oss.str();

            std::string filename = config_.store_dir + "/" + symbolid + ".lob";
            file = std::make_shared<QFile>(filename.c_str());
            if(!file->open(QIODevice::WriteOnly)){
                qWarning() << QString("Fanout_File open file failed! Detail:") << filename.c_str() ;
                continue;
            }
            // format px_record->asks to string
            std::string line;
            size_t num = px_record->asks.size();
            for(auto itr = px_record->asks.rbegin();itr != px_record->asks.rend();itr++){
                line += "ask" + std::to_string(num--) + " \t " + std::to_string(std::get<0>(*itr)) + ":" + std::to_string( std::get<1>(*itr)) + "\n";
            }
            line += "----------------\n";
            num = 1;
            for(auto itr = px_record->bids.begin();itr != px_record->bids.end();itr++){
                line += "bid:" + std::to_string(num ++ ) + " \t " + std::to_string(std::get<0>(*itr)) + ":" + std::to_string( std::get<1>(*itr)) + "\n";
            }
            file->write(line.c_str(), line.size());
            file->flush();
        }
        std::cout << "LobRecordFanoutFile::thread:   stopped" << std::endl;
    });
    return true;
}

void LobRecordFanoutFile::stop() {
    stopped_.store(true);
    thread_.join();
}
