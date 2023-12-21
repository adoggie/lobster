
#include <algorithm>
#include <fstream>
#include <iostream>
#include <tuple>
#include <ctime>
#include <sstream>
// #include <json/json.h>
#include <qcoreapplication.h>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>

#include "service.h"
#include "feed_csv.h"
#include "feed_tlive.h"
#include "feed_zmq.h"



bool LobService::init(const std::string& configfile) {
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
            std::cerr << "Failed to open config file: " << configfile << std::endl;
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
    // QString name = jsonObj.value("name").toString();
    // int age = jsonObj.value("age").toInt();

    // ifs >> json;
    config_.lobnum = json.value("lobnum").toInt(1000000);
    config_.feed_type = json.value("feed_type").toString("mdl_csv").toStdString();
    config_.save_pxlist_infile = json.value("save_pxlist_infile").toBool();
    config_.save_in_dir = json.value("save_in_dir").toString().toStdString();
    config_.workers = json.value("workers").toInt(1);
    config_.symbol_price_limit_file = json.value("symbol_price_limit_file").toString("symbol_price_limit_%1.csv").toStdString();    
    config_.csv_setting.datadir = json.value("mdl_csv").toObject().value("datadir").toString("./").toStdString();
    config_.csv_setting.speed = json.value("mdl_csv").toObject().value("speed").toInt(1);
    config_.live_setting.server_addr = json["mdl_live"].toObject().value("server_addr").toString("tcp://127.0.0.1:5555").toStdString();
    config_.zmq_setting.server_addr = json["mdl_zmq"].toObject().value("server_addr").toString("tcp://127.0.0.1:5555").toStdString();
    config_.zmq_setting.mode = json["mdl_zmq"].toObject().value("mode").toString("bind").toStdString();
    config_.symbol_list_file = json["symbol_list_file"].toString("").toStdString();

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
        std::cerr << "Failed to open config file: " << config_.symbol_price_limit_file << std::endl;
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
    pxlive_.resize(config_.lobnum);
    pxhistory_.resize(config_.lobnum);
    workers_.resize(config_.workers);
    if (config_.feed_type == "mdl_csv") {
        decoder_ = std::make_shared<Mdl_Csv_Feed::DataDecoder>();
        feeder_ = std::make_shared<Mdl_Csv_Feed>(config_.csv_setting);
        // }else if( config_.feed_type == "mdl_live"){
        //     decoder_ = std::make_shared<Mdl_Live_Feed::DataDecoder>();
    }else if( config_.feed_type == "mdl_zmq"){
        decoder_ = std::make_shared<Mdl_Zmq_Feed::DataDecoder>();
        feeder_ = std::make_shared<Mdl_Zmq_Feed>(config_.zmq_setting);
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
    return true;
}

bool LobService::start() {
    for (auto& worker : workers_) {
        worker->start();
    }
    feeder_->start();
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
    stopped_.store(true);
    thread_.join();
    cond_.notify_all();
}

void LobService::getPxList(symbolid_t symbolid, size_t depth, std::list< lob_px_record_ptr >& list){
    lob_px_history_t* pxlist = pxhistory_[symbolid];
    std::shared_lock<RwMutex> lock(pxlist->mtx);
    int32_t shift = (int32_t) std::min(depth, static_cast<size_t>(pxlist->list.size()));
    // std::reverse_copy(pxlist->list.rbegin(), pxlist->list.rbegin() + shift, 
    //         std::back_inserter(list));
    // std::reverse_copy(pxlist->list.rbegin(), pxlist->list.rbegin() + (int32_t)1, 
    //         std::back_inserter(list));
    // std::transform( pxlist->list.rbegin(), pxlist->list.rbegin() + 
    //         std::min(depth, static_cast<int>(pxlist->list.size())), 
    //         std::back_inserter(list), [](lob_px_record_ptr& ptr) {
    //             return ptr;
    //         });
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
    symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    
    // 上交委托
    if(m->SecurityIDSource == Message::Source::SH){        
        onOrderSH(m);
        
    }else if( m->SecurityIDSource == Message::Source::SZ){ // 深交所
        onOrderSZ(m);        
    }
    // 处理撤单
}

void LobService::onTrade(const  TradeMessage* m){
    
}

void LobService::onMessage(Message * msg){
    symbolid_t symbolid = msg->SecurityID;
    if(symbolid >= pxlive_.size()){
        return ;
    }
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    if(pxlist == nullptr){
        return;
    }

    switch(msg->MsgType){
        case Message::Order:
           onOrder((OrderMessage*)msg);                        
        case Message::Trade:
           onTrade((TradeMessage*)msg);                        
    }
}

void LobService::onFeedRawData(lob_data_t* data)
{
    symbolid_t symbolid = data->symbolid;
    uint32_t worker_idx = symbolid / config_.workers;
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

lob_px_history_t * LobService::createPxHistory(symbolid_t symbolid){
    lob_px_history_t* pxhistory = new lob_px_history_t;
    return pxhistory;
}


void LobService::onOrderSH(const  OrderMessage* m){

    // std::unique_lock<RwMutex> lock(pxlist->mtx);
    symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    uint32_t p = m->ssh_ord.Price - pxlist->low;
    if(m->ssh_ord.OrdType == 'A'){ // 新增委托
        if(m->ssh_ord.Side =='B'){  // buy
            pxlist->bids[p].qty += m->ssh_ord.OrderQty;
        }else if( m->ssh_ord.Side == 'S'){ // sell
            pxlist->asks[p].qty += m->ssh_ord.OrderQty;
        }
    }else if( m->ssh_ord.OrdType == 'D'){ // 删除委托单
        if(m->ssh_ord.Side =='B'){
            pxlist->bids[p].qty -= m->ssh_ord.OrderQty;
            // pxlist->bids[p].qty = std::max(0,pxlist->bids[p].qty.load());
        }else if( m->ssh_ord.Side == 'S'){
            pxlist->asks[p].qty -= m->ssh_ord.OrderQty;  
            // pxlist->asks[p].qty = std::max(0,pxlist->asks[p].qty.load());      
        }
    }        
}

/// @brief 成交回报"T" ,消除委托单的委托量
/// @param m
void LobService::onTradeSH(const  TradeMessage* m){
    symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
}

void LobService::onOrderSZ(const  OrderMessage* m){
    symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
    uint32_t p = m->ssz_ord.Price - pxlist->low;
    if(m->ssz_ord.Side =='1'){ //buy
        pxlist->bids[p].qty += m->ssz_ord.OrderQty;
    }else if( m->ssz_ord.Side == '2'){ // sell
        pxlist->asks[p].qty += m->ssz_ord.OrderQty;
    }
}

void LobService::onTradeSZ(const  TradeMessage* m){
    symbolid_t symbolid = m->SecurityID;
    lob_px_list_t*  pxlist =  pxlive_[symbolid];
}