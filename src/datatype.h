#ifndef __DATATYPE_H__
#define __DATATYPE_H__
#include"globaltype.h"
#include"listnode.h"
#include"msgtype.h"


struct SockBase { 
    MsgHdr* m_rcv_dummy; // used by recv
    Int32 m_rdlen;
    Int32 m_port;
    Char m_ip[DEF_IP_SIZE];
    union {
        MsgHdr m_hdr;
        Byte m_buf[0];
    };
};

struct TcpParam {
    Int32 m_port;
    Int32 m_addr_len;
    Char m_ip[DEF_IP_SIZE];
    Char m_addr[DEF_ADDR_SIZE];
};

class TickTimer;

struct TimerEle {
    hlist_node m_node;
    TickTimer* m_base;
    Uint32 m_expires;
    Uint32 m_interval; 
    Int32 m_type;
};

static inline void INIT_TIMER_ELE(struct TimerEle* ele) {
    INIT_HLIST_NODE(&ele->m_node);
    ele->m_base = NULL;
    ele->m_expires = 0;
    ele->m_interval = 0;
    ele->m_type = 0;
}

struct NodeBase {
    list_node m_node;
    list_head m_cmd_que;
    list_head m_cmd_que_tmp;
    list_head m_deal_que;
    list_head m_deal_que_tmp;
    list_head m_wr_que;
    list_head m_wr_que_tmp;

    struct Task m_rd_task;
    struct Task m_wr_task;
    struct Task m_deal_task;
    struct Task m_director_task;

    Void* m_data;
    Int32 m_fd;
    Int32 m_node_type;
    Uint32 m_ev_type;
    Int32 m_fd_status;

    Bool m_rd_err;
    Bool m_wr_err;
    Bool m_deal_err;
};

static inline void INIT_NODE_BASE(struct NodeBase* base) {
    INIT_LIST_NODE(&base->m_node);
    base->m_node_type = -1;
    base->m_fd = -1;
}


enum EnumNodeType {
    ENUM_NODE_SOCK,
    ENUM_NODE_SOCK_CONNECTOR,
    
    ENUM_NODE_EVENT,
    ENUM_NODE_TIMER,
    ENUM_NODE_LISTENER, 
    
    ENUM_NODE_OBJ_MAX,
    
    ENUM_NODE_EPOLL_WR,
    ENUM_NODE_OBJ_CENTER,
};

enum EnumFdStatus {
    ENUM_FD_INIT,

    ENUM_FD_CONNECTING,
    ENUM_FD_NORMAL,
    
    ENUM_FD_LOGIN,
    ENUM_FD_LOGOUT,
    
    ENUM_FD_CLOSING,
    ENUM_FD_CLOSE_RD,
    ENUM_FD_CLOSE_WR,
    ENUM_FD_CLOSE_DEALER,
    ENUM_FD_CLOSE_DIRECTOR,

    ENUM_FD_END,
};

extern Void freeMsgQue(list_head* list);

/* check to free the real node */
extern Void freeNode(NodeBase* base);
extern NodeBase* buildNode(Int32 fd, Int32 type);

extern SockBase* getSockBase(NodeBase* node);

#endif

