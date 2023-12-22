
#ifndef _LOB_H
#define _LOB_H
#include <cstddef>
#include <stdint.h>
#include <cstdlib>
#include <list>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <algorithm>
#include <memory>
#include <vector>
#include <shared_mutex>
#include <string>
#include <cstddef>

typedef std::shared_mutex RwMutex;
typedef void* security_body_addr_t;

#define MAX_ORDER_DEPTH 100  //

typedef uint32_t  symbolid_t;
typedef uint32_t  lob_price_t;
typedef int32_t  lob_qty_t;
struct LobService;


struct tonglian_live_t {
    std::string server_addr; // tcp://192.168.12.2:9900
};

struct mdl_csv_feed_setting_t {
    std::string datadir; //
    uint32_t  speed; // 1 - 100
};

struct mdl_live_feed_setting_t {
    std::string server_addr;
};

struct zmq_feed_setting_t {
    std::string server_addr;
    std::string mode; // bind / connect
};


#pragma pack(push, 1)
struct lob_px_t {
    uint8_t  bs; // buy or sell 0 - buy , 1 - sell
    uint32_t  px;
    uint32_t    ordTime;
    std::atomic<int32_t> qty;
};
#pragma pack(pop)



struct lob_px_list_t {
    symbolid_t   symbolid;
    RwMutex     mtx;
    lob_price_t   low; // 15.21 / 1521
    lob_price_t   high; // 16.50 / 1650 , => (pxlow,pxhigh) => [1521,1522,..,1650]
    // lob_px_t* bid;
    lob_px_t* ask1;
    lob_px_t* bid1;
    // lob_px_t* base;
    lob_px_t* bids;  // data base 
    lob_px_t* asks; // data base 
    inline 
    uint32_t get_px_span() const {
        return (high - low + 1);
    }
};

#include <msgpack.hpp>

struct lob_px_record_t{
    uint8_t     source;     // sh or sz 
    symbolid_t symbolid;
    typedef std::shared_ptr<lob_px_record_t> Ptr;
    std::vector< std::tuple<lob_price_t,lob_qty_t> > asks;
    std::vector<std::tuple<lob_price_t,lob_qty_t> > bids;
    MSGPACK_DEFINE(source, symbolid, asks, bids);
};


typedef std::shared_ptr<lob_px_record_t> lob_px_record_ptr;

struct lob_px_history_t {
    RwMutex mtx;
    std::list< lob_px_record_ptr > list;
};

// offset = 600021 - 0
// LOB_NUM =
// #define  MAX_LOB_NUM  100000
// struct lob_dir_t {
//     uint32_t      version;
//     lob_config_t  config;
//     std::vector<lob_px_list_t*> live; // 0 - 600021
//     std::vector<lob_px_history_t*> history; 
    
// };

struct lob_context_t {
    void* service; // lob_dir_t    
};

struct lob_data_t{
    char * data;
    size_t len;
    symbolid_t symbolid;
    void * user;
};

lob_px_list_t * lob_px_list_alloc();
void lob_px_list_free(lob_px_list_t* pxlist);

lob_data_t* lob_data_alloc(size_t len);
lob_data_t* lob_data_alloc2(char * data ,size_t len);
void lob_data_free(lob_data_t* data);

typedef std::vector<uint8_t> ByteArray;
typedef std::shared_ptr<ByteArray> ByteArrayPtr;

void pxlist_snapshot(const lob_context_t* ctx, lob_px_list_t* pxlist);

void put_px_vol(const lob_context_t* ctx, const char* symbol, uint32_t px, uint32_t vol);

int     get_px_list(lob_context_t* ctx, const char* symbol, lob_px_t** ask, uint32_t* asknum, lob_px_t** bid, uint32_t* bidnum);
void    free_px_list(lob_px_t* list);
lob_px_list_t* create_px_list(lob_context_t* ctx, const char* symbol, uint32_t low, uint32_t high);
int     px_high_low(const char* symbol, uint32_t& low, uint32_t& high);
lob_context_t* init_context(const char* config, int& error);
void    free_context(lob_context_t* ctx);

// void parse_lob_config_from_jsonfile(const char* filename, lob_config_t* config);

#endif

/*

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

// Define the struct lob_px_t
struct lob_px_t {
    uint8_t bs; // buy or sell 0 - buy , 1 - sell
    uint32_t px, vol;
};

// Define the function signature
int get_px_list(void *ctx, const char *symbol, struct lob_px_t **ask, uint32_t *asknum, struct lob_px_t **bid, uint32_t *bidnum);

// Define a wrapper function that converts the ctypes arguments and return value
int get_px_list_wrapper(void *ctx, const char *symbol, void **ask_ptr, uint32_t *asknum, void **bid_ptr, uint32_t *bidnum) {
    struct lob_px_t *ask, *bid;
    int result = get_px_list(ctx, symbol, &ask, asknum, &bid, bidnum);

    // Assign the converted pointers to ctypes pointers
    *ask_ptr = ask;
    *bid_ptr = bid;

    return result;
}


import ctypes

# Define the ctypes structure
class LobPxT(ctypes.Structure):
    _fields_ = [
        ("bs", ctypes.c_uint8),
        ("px", ctypes.c_uint32),
        ("vol", ctypes.c_uint32)
    ]

# Load the shared library
lib = ctypes.CDLL("your_library_name.so")  # Replace "your_library_name.so" with the actual library name

# Define the function signature
lib.get_px_list_wrapper.restype = ctypes.c_int
lib.get_px_list_wrapper.argtypes = [
    ctypes.c_void_p,  # ctx
    ctypes.c_char_p,  # symbol
    ctypes.POINTER(ctypes.POINTER(LobPxT)),  # ask
    ctypes.POINTER(ctypes.c_uint32),  # asknum
    ctypes.POINTER(ctypes.POINTER(LobPxT)),  # bid
    ctypes.POINTER(ctypes.c_uint32)  # bidnum
]

# Define a Python wrapper function
def get_px_list(ctx, symbol):
    ask_ptr = ctypes.POINTER(LobPxT)()
    bid_ptr = ctypes.POINTER(LobPxT)()
    asknum = ctypes.c_uint32()
    bidnum = ctypes.c_uint32()

    result = lib.get_px_list_wrapper(ctypes.c_void_p(ctx), ctypes.c_char_p(symbol.encode()), ctypes.byref(ask_ptr),
                                     ctypes.byref(asknum), ctypes.byref(bid_ptr), ctypes.byref(bidnum))

    # Convert the C arrays to Python lists
    ask_list = [ask_ptr[i] for i in range(asknum.value)]
    bid_list = [bid_ptr[i] for i in range(bidnum.value)]

    # Free the C arrays
    lib.free(ask_ptr)
    lib.free(bid_ptr)

    return ask_list, bid_list


ctx = ...  # Provide the context value
symbol = "ABC"  # Provide the symbol

ask_list, bid_list = get_px_list(ctx, symbol)

# Print the result
print("Ask List:")
for ask in ask_list:
    print(f"BS: {ask.bs}, PX: {ask.px}, VOL: {ask.vol}")

print("Bid List:")
for bid in bid_list:
    print(f"BS: {bid.bs}, PX: {bid.px}, VOL: {bid.vol}")

*/