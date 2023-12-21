
#ifndef _FEED_MESSAGE_H
#define _FEED_MESSAGE_H

#include <stdint.h>
#include <memory>

struct Message {
    enum Type{
        SnapShot = 111 , 
        Order = 191,
        Trade = 192 
    };
    
    enum Source{
        SH =101,
        SZ = 102,
    };

    typedef std::shared_ptr<Message> Ptr;
    
    uint8_t     SecurityIDSource;
    uint8_t     MsgType; 
    uint16_t    ChannelNo; 
    uint64_t    ApplSeqNum; 
    uint32_t    SecurityID;

    Message* next = nullptr;
};

struct SBE_header_t //24B
{
    uint8_t     SecurityIDSource;   //交易所代码:102=深交所;101=上交所.
    uint8_t     MsgType;            //消息类型:111=快照行情;191=逐笔成交;192=逐笔委托.
    uint16_t    MsgLen;             //消息总字节数，含消息头.
    char        SecurityID[9];      //证券代码，6或8字符后加'\0'
    uint16_t    ChannelNo;          //通道号
    uint64_t    ApplSeqNum;         //消息序列号，仅对逐笔成交和逐笔委托有效.
    uint8_t     TradingPhase;       //交易阶段代码映射，仅对行情快照有效（深交所和上交所具体映射方式不同）.
};

//对应上交所消息类型UA5801
struct SBE_SSH_ord_t  //56B
{
    // SBE_header_t  Header;    //msgType=192

    int64_t         OrderNo;            //原始订单号 *
    int32_t         Price;              //委托价格（元）, 3位小数
    int64_t         OrderQty;           //委托数量, 3位小数
    int8_t          OrdType;            //订单类型: 'A'=新增委托订单, 'D'=删除委托订单
    int8_t          Side;               //买卖单标志: 'B'=买单, 'S'=卖单
    uint32_t        OrderTime;          //委托时间(百分之一秒), 14302506表示14:30:25.06
    uint8_t         Resv[6];
};


//上交所逐笔成交
//对应上交所消息类型UA3201
struct SBE_SSH_exe_t  //64B
{
    // SBE_header_t  Header;    //msgType=191

    int64_t         TradeBuyNo;         //买方订单号
    int64_t         TradeSellNo;        //卖方订单号
    int32_t         LastPx;             //成交价格（元）, 3位小数
    int64_t         LastQty;            //成交数量, 3位小数 *
    int8_t          TradeBSFlag;        //内外盘标志: 'B'=外盘，主动买; 'S'=内盘，主动卖; 'N'=未知 **
    uint32_t        TradeTime;          //委托时间(百分之一秒), 14302506表示14:30:25.06
    double          TradeMoney;         // 成交金额
    uint8_t         Resv[7];
};
// *  股票单位：股; 债券分销单位：千元面额; 基金单位：份
// ** 目前看集合竞价结束时的成交标志为'N'



// 深交所逐笔委托

//对应深交所协议消息类型300192
struct SBE_SSZ_ord_t //48B
{
    struct SBE_header_t  Header;    //msgType=192

    int32_t         Price;          //委托价格, Price,N13(4)
    int64_t         OrderQty;       //委托数量, Qty,N15(2)
    int8_t          Side;           //买卖方向: '1'=买, '2'=卖, 'G'=借入, 'F'=出借
    int8_t          OrdType;        //订单类别: '1'=市价, '2'=限价, 'U'=本方最优
    uint64_t        TransactTime;   //YYYYMMDDHHMMSSsss(毫秒) 实际以10ms为变化单位
    uint8_t         Resv[2];
};


//深交所逐笔成交
//对应深交所协议消息类型300191
struct SBE_SSZ_exe_t //64B
{
    SBE_header_t  Header;    //msgType=191

    int64_t         BidApplSeqNum;  //买方委托索引 *
    int64_t         OfferApplSeqNum;//卖方委托索引 *
    int32_t         LastPx;         //成交价格, Price,N13(4)
    int64_t         LastQty;        //成交数量, Qty,N15(2)
    int8_t          ExecType;       //成交类别: '4'=撤销, 'F'=成交
    uint64_t        TransactTime;   //YYYYMMDDHHMMSSsss(毫秒) 实际以10ms为变化单位
    uint8_t         Resv[3];
};
// * 委托索引从 1 开始计数， 0 表示无对应委托

struct OrderMessage :Message {
    typedef std::shared_ptr<OrderMessage> Ptr;
    OrderMessage(){ MsgType = Message::Order;}
    // SBE_header_t  Header;
    union {
        SBE_SSH_ord_t ssh_ord;
        SBE_SSZ_ord_t ssz_ord;
    };
};

struct TradeMessage :Message {
    typedef std::shared_ptr<TradeMessage> Ptr;
    // SBE_header_t  Header;
    union {
        SBE_SSH_exe_t ssh_trd;
        SBE_SSZ_exe_t ssz_trd;
    };
};

#endif // !_FEED_MESSAGE_H
