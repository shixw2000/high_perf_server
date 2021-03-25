#ifndef __SOCKDATA_H__
#define __SOCKDATA_H__
#include"hlist.h"


typedef struct _SockData SockData;

struct TaskElement {
    unsigned int m_state; 
    struct hlist m_list;
};

#define INIT_TASK_ELE(x) ({(x)->m_state=0U; \
    (x)->m_list.next=NULL;})


enum SockState {  
    SOCK_STATE_NONE,
    SOCK_STATE_CONNECTING,
    SOCK_STATE_ESTABLISHED,
    SOCK_STATE_LOGIN,
    
    SOCK_STATE_CLOSING,
    SOCK_STATE_CLOSING_SEND,
    SOCK_STATE_CLOSING_RECV,
    SOCK_STATE_CLOSING_DEAL,
    
    SOCK_STATE_CLOSED, 
};

enum WORK_EVENT { 
    WORK_EVENT_LOCK = 0, //never used by user
    
    WORK_EVENT_IN,
    WORK_EVENT_OUT,
    WORK_EVENT_END,
    
    EV_FLOW_CONTROL, 
    
    EV_SOCK_READABLE,
    EV_SOCK_WRITABLE,

    DIRECT_STOP_SOCK,
    
    WORK_EVENT_BUTT = 16 
};

enum WORK_EV_TYPE {
    WORK_TYPE_ONCE = 1,
    WORK_TYPE_CUSTOMER,
    WORK_TYPE_LOOP,

    WORK_TYPE_INVALID = -1
};

#pragma pack(4)
struct MsgHeader {
    int m_size;
    unsigned char m_ver;
    unsigned char m_crc;
    unsigned short m_cmd;
    unsigned int m_id;
    unsigned char m_csum;
    unsigned char m_res[3];
};
#pragma pack()

struct msg_type {
    int m_capacity;
    int m_size;
    short m_ref;
    union {
        char m_beg;
        char m_buf[2];
    };
};

struct MsgSndState {
    long long m_totalSnd;
    int m_pos; 
    int m_step;
    int m_maxlenOnce;
};

struct MsgRcvState {
    long long m_totalRcv;
    struct msg_type* m_msg;
    int m_capacity;
    int m_size;
    int m_step;
    int m_maxlenOnce;    
    
    union {
        char m_end[1];
        struct MsgHeader m_hd;
    };
};

struct MsgDealState {
    int m_step;
};

class TimerCallback;
struct TimeTask {
    struct list_node m_node;
    int m_type;
    int m_timeout; 
    TimerCallback* m_cb; 
};


static const int DEF_MSG_HEADER_SIZE = sizeof(struct MsgHeader);
static const unsigned char DEF_MSG_VERSION = 0xBE;
static const unsigned char DEF_MSG_CRC = 0xCE;
static const char DEF_MSG_TAIL_CHAR = 0xCA;

static const int DEF_CONNECT_TIMEOUT_TICK = 3;
static const int DEF_RECV_TIMEOUT_TICK = 30;
static const int DEF_SEND_TIMEOUT_TICK = 30;
static const int DEF_LOGIN_TIMEOUT_TICK = 20;
static const int DEF_TIMER_SIZE_ORDER = 6;
static const int MAX_SOCK_ID_SIZE = 32;


struct _SockData {
    short m_state;
    short m_isSrv;
	unsigned int m_seq; // unique sequence
	int m_fd;
	int m_type; // kinds of socket 
    void* m_data;  
    char m_id[MAX_SOCK_ID_SIZE];
    struct MsgRcvState m_curRcv;
    struct MsgSndState m_curSnd; 
    struct MsgDealState m_curDeal;
    struct deque_root m_sndMsgQue;
    struct deque_root m_rcvMsgQue;
    struct TaskElement m_deallist;
	struct TaskElement m_rcvlist;
    struct TaskElement m_sndlist;
    struct TaskElement m_monlist;
    struct TimeTask m_rcv_timer;
    struct TimeTask m_snd_timer;
    struct TimeTask m_deal_timer;
};

#define isSockValid(s) (SOCK_STATE_CLOSING > (s)->m_state)

class SockCache {
public:
	explicit SockCache(unsigned int capacity);
	~SockCache();

	int createCache();
	int freeCache();

	SockData* allocData(int fd, int type, void* data);
    SockData* getData(int fd);
	int freeData(SockData* data); 

private: 
	const unsigned int m_capacity;
	unsigned int m_size;
	SockData* m_pdata;
};

extern unsigned char mkcrc(const void* data, int size);

#endif

