
#include <algorithm>
#include <fstream>
#include <iostream>
#include <tuple>
#include <ctime>
#include <sstream>
#include <functional>
// #include <json/json.h>
#include <qcoreapplication.h>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtCore>



#include "service.h"

#include "feed_csv.h"
#include "feed_tlive.h"
#include "feed_zmq.h"
#include "feed_tonglian_mdl_redis.h"

#include "fanout_file.h"
#include "fanout_zmq.h"
#include "fanout_redis.h"



bool LobService::init(const std::string& configfile) {
    logger_ = log4cplus::Logger::getInstance(
        LOG4CPLUS_TEXT("service"));
    LOG4CPLUS_INFO(logger_, LOG4CPLUS_TEXT("LobService::init.."));

    // std::ifstream ifs(configfile);
    QFile ifs(configfile.c_str());
    // Json::Value json;
    // Json::CharReaderBuilder builder;
    // std::string errs;

    // if (!ifs.is_open()) {
    if(!ifs.open(QIODevice::ReadOnly | QIODevice::Text) ){
        QString ss;
        ss =  QCoreApplication::applicationDirPath() + "/settings.json" ;
        // ifs = QFile (ss);
        ifs.setFileName(ss);
        if( !ifs.open(QIODevice::ReadOnly | QIODevice::Text)){
            // std::cerr << "Failed to open config file: " << configfile << std::endl;
            LOG4CPLUS_ERROR(logger_, LOG4CPLUS_TEXT("Failed to open config file: ") << configfile);
            return false;
        }
    }

    QByteArray jsonData = ifs.readAll();
    ifs.close();
    
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull()) {
        std::cerr <<  "Failed to parse JSON data" << std::endl;
        return false;
    }

    // Access specific values from the JSON object
    QJsonObject json = jsonDoc.object();
    config_.lobnum = json.value("lobnum").toInt(1000000);
    config_.feed_type = json.value("feed_type").toString("mdl_csv").toStdString();
    config_.save_pxlist_infile = json.value("save_pxlist_infile").toBool();
    config_.save_in_dir = json.value("save_in_dir").toString().toStdString();
    config_.workers = json.value("workers").toInt(1);
    config_.symbol_price_limit_file = json.value("symbol_price_limit_file").toString("symbol_price_limit_%1.csv").toStdString();    
    config_.symbol_list_file = json["symbol_list_file"].toString("").toStdString();
    config_.sample_px_depth = json["sample_px_depth"].toInt(10);
    config_.sample_interval_ms = json["sample_interval_ms"].toInt(1000);

      {
        loadSymbolList();
    }

    /// load price limit data 
    QDateTime currentDateTime = QDateTime::currentDateTime();
    // QString str = QString( config_.symbol_price_limit_file.c_str() ).arg(currentDateTime.toString("yyyy-MM-dd"));
    QString str = QString( config_.symbol_price_limit_file.c_str() ).arg(currentDateTime.toString("yyyy"));
    std::string filename = str.toStdString();
    QFile file (filename.c_str());
    if( !file.open(QIODevice::ReadOnly | QIODevice::Text)){
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return false;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if( line.size() == 0 || line.startsWith("#")){
            continue;
        }
        QStringList list = line.split(",");
        if(list.size() < 3){
            std::cerr << "limit price data error: " << line.toStdString() << std::endl;
            continue;
        }
        // symbol,low,high
        bool ok;
        symbolid_t symbolid = list[0].toLong(&ok);
        float low = list[1].toFloat(&ok);
        float high = list[2].toFloat(&ok);
        if( !ok){
            std::cerr << "limit price data error: " << line.toStdString() << std::endl;
            continue;
        }
        limit_pxlist_[symbolid] = std::make_pair(low,high);
    }
    file.close();
    

    /// parse
    config_.symnum_of_worker = config_.lobnum / config_.workers;
    if( config_.symnum_of_worker * config_.workers < config_.lobnum){
        config_.symnum_of_worker ++;
    }
    pxlive_.resize(config_.lobnum);
    pxhistory_.resize(config_.lobnum);
    workers_.resize(config_.workers);

    // 直接读取mdl csv文件记录
    if (config_.feed_type == "mdl_csv") {
        decoder_ = std::make_shared<Mdl_Csv_Feed::DataDecoder>();
        feeder_ = std::make_shared<Mdl_Csv_Feed>();
        
    }else if( config_.feed_type == "mdl_zmq"){  // 从zmq 接收
        decoder_ = std::make_shared<Mdl_Zmq_Feed::DataDecoder>();
        feeder_ = std::make_shared<Mdl_Zmq_Feed>();
        feeder_->init(json["mdl_zmq"].toObject());

    }else if( config_.feed_type == "mdl_redis"){ // 从redis接收实时行情
        decoder_ = std::make_shared<Mdl_Redis_Feed::DataDecoder>();
        feeder_ = std::make_shared<Mdl_Redis_Feed>();
        feeder_->init(json["mdl_redis"].toObject());
    
    }else{
        return false;
    }
    feeder_->setFeedUser(this);
    for (auto& worker : workers_) {
        worker = std::make_shared<Worker>();
        worker->setDataDecoder(decoder_.get());
    }

    for(symbolid_t& n : symbol_list_ ){
        pxlive_[n] = createPxList(n);
        pxhistory_[n] = createPxHistory(n);
    }

    // init fanouts
    auto array = json["fanout"].toArray();
    for ( const auto & obj : array){
        if( obj.isObject() ){
            auto enable = obj.toObject().value("enable").toBool(false);

            if( enable){
                LobRecordFanout::Ptr fanout;
                if( obj.toObject().value("name") == "zmq"){
                    fanout = std::make_shared<LobRecordFanoutZMQ>();                    
                }else if( obj.toObject().value("name") == "file"){
                    fanout = std::make_shared<LobRecordFanoutFile>();
                }else if( obj.toObject().value("name") == "redis"){
                    fanout = std::make_shared<LobRecordFanoutRedis>();
                }
                if( fanout ) {
                    fanout->init(obj.toObject());
                    fanouts_.push_back(fanout);
                }
            }
        }
    }
    return true;
}

bool LobService::start() {
    for (auto& worker : workers_) {
        worker->start();
    }
    feeder_->start();
    for( auto & fanout : fanouts_){
        fanout->start();
    }
    if( fanouts_.size()){
        timer_.start( config_.sample_interval_ms, std::bind(&LobService::onFanoutTimed,this) );
    }
    stopped_.store(false);
    
    // save pxlist in file
    // if (config_.save_pxlist_infile) {
    //     thread_ = std::thread([this]() {
            // while (stopped_.load() == false) {
            //     auto r = queue_.pop();
            //     std::string filename = config_.save_in_dir + "/" + std::to_string(symbolid) + ".pxlist";
            //     FILE* fp = fopen(filename.c_str(), "wb");
            //     if (fp) {
            //         auto& pxlist = pxhistory_[r.symbolid];
            //         fwrite(pxlist->data, pxlist->size, 1, fp);
            //         fclose(fp);
            //     }
            // }
    //         });
    // }

    return true;
}

void LobService::stop() {
    feeder_->stop();
    for (auto& worker : workers_) {
        worker->stop();
    }
    for(auto & fanout : fanouts_){
        fanout->stop();
    }
    stopped_.store(true);
    thread_.join();


    cond_.notify_all();
}


void LobService::savePxList(symbolid_t symbolid) {

    lob_px_list_t* pxlist = pxlive_[symbolid];
    if( pxlist->bid1 == nullptr || pxlist->ask1 == nullptr ){
        return;
    }

    lob_px_record_ptr recorder = std::make_shared<lob_px_record_t>();
    size_t bidnum = 0 , asknum = 0;
    lob_px_t* cur = pxlist->bid1;
    while( cur >= pxlist->bids ){
        recorder->bids.push_back( std::make_tuple(cur->px,cur->qty.load()));
        cur --;
    }
    cur = pxlist->ask1;
    while( cur < pxlist->asks+ pxlist->get_px_span()){
        recorder->asks.push_back( std::make_tuple(cur->px,cur->qty.load()));
        cur ++;
    }

    lob_px_history_t* pxhistory = pxhistory_[symbolid];
    {
        std::unique_lock<RwMutex> lock(pxhistory->mtx);
        pxhistory->list.emplace_back(recorder);
    }

}

void LobService::waitForShutdown(){
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]{ return stopped_.load(); });
}

void LobService::onOrder(const  OrderMessage* m)
{
    // 上交委托
    if(m->SecurityIDSource == Message::Source::SH){        
        onOrderSH(m);
        
    }else if( m->SecurityIDSource == Message::Source::SZ){ // 深交所
        onOrderSZ(m);        
    }
    // 处理撤单
}

void LobService::onTrade(const  TradeMessage* m){
    // 上交委托
    if(m->SecurityIDSource == Message::Source::SH){
        onTradeSH(m);

    }else if( m->SecurityIDSource == Message::Source::SZ){ // 深交所
        onTradeSZ(m);
    }
}

void LobService::onMessage(Message * msg){
    symbolid_t symbolid = msg->SecurityID;
    if(symbolid >= pxlive_.size()){
        return ;
    }
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    if(pxlist == nullptr){
        qWarning() << "symbolid: " << symbolid << ", not in the worklist!";
        return;
    }

    switch(msg->MsgType){
        case Message::Order:
           onOrder((OrderMessage*)msg);
           break;
        case Message::Trade:
           onTrade((TradeMessage*)msg);
           break;
    }
}

void LobService::onFeedRawData(lob_data_t* data)
{
    symbolid_t symbolid = data->symbolid;
    uint32_t worker_idx = symbolid / config_.symnum_of_worker;
    if (worker_idx >= workers_.size()) {
        return;
    }
    workers_[worker_idx]->enqueue(data);
}

lob_px_list_t* LobService::createPxList(symbolid_t symbolid)
{
    lob_px_list_t* pxlist = lob_px_list_alloc(); // new lob_px_list_t;
    pxlist->symbolid = symbolid;
    float high, low;
    if (getPxHighLow(symbolid, high, low)) {
        pxlist->high = high * 100;
        pxlist->low = low * 100;
    }
    uint32_t span = pxlist->get_px_span();
    pxlist->bid1 = nullptr;
    pxlist->ask1 = nullptr;
    pxlist->asks = new lob_px_t[span];
    pxlist->bids = new lob_px_t[span];
    return pxlist;
}

bool LobService::getPxHighLow(symbolid_t symbolid, float & high, float& low)
{
    auto itr = limit_pxlist_.find(symbolid);
    if(itr!=limit_pxlist_.end()){
        high = itr->second.second;
        low = itr->second.first;
        return true;
    }
    return false;
}

bool LobService::loadSymbolList(){
    std::string filename = config_.symbol_list_file;
    QFile file (filename.c_str());
    if( !file.open(QIODevice::ReadOnly | QIODevice::Text)){
        std::cerr << "Failed to open config file: " << config_.symbol_list_file << std::endl;
        return false;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if( line.size() == 0 || line.startsWith("#")){
            continue;
        }
        // symbol,low,high
        bool ok;
        symbolid_t symbolid = line.toLong(&ok);        
        if( !ok){
            std::cerr << "symbolid  error: " << line.toStdString() << std::endl;
            continue;
        }
        symbol_list_.push_back(symbolid);
    }
    file.close();
    return true;
}

// sampling & fanout
void LobService::onFanoutTimed(){
    for(auto & symbolid : symbol_list_){
        // savePxList(symbolid);
        lob_px_record_t::Ptr pxr = std::make_shared<lob_px_record_t>();
        if( !getPxList(symbolid,config_.sample_px_depth,*pxr) ){
            continue ;
        }
        for( auto & fout : fanouts_){
            fout->fanout(pxr);
        }
    }
}

lob_px_history_t * LobService::createPxHistory(symbolid_t symbolid){
    lob_px_history_t* pxhistory = new lob_px_history_t;
    return pxhistory;
}


