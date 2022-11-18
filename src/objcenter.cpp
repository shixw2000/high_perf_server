#include"objcenter.h"
#include"director.h"
#include"msgcenter.h"
#include"timerobj.h"
#include"sockobj.h"
#include"sockutil.h"


Void freeMsgQue(list_head* list) { 
    list_node* pos = NULL;
    list_node* n = NULL;
    MsgHdr* msg = NULL;

    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            msg = MsgCenter::cast(pos);
            
            MsgCenter::freeMsg(msg);
        }
    }
}

static Void resetBase(NodeBase* base) { 
    memset(base, 0, sizeof(NodeBase));

    base->m_fd = -1;
    base->m_fd_status = ENUM_FD_INIT;

    INIT_LIST_NODE(&base->m_node);
    
    INIT_LIST_HEAD(&base->m_cmd_que);
    INIT_LIST_HEAD(&base->m_cmd_que_tmp); 
    INIT_LIST_HEAD(&base->m_deal_que);
    INIT_LIST_HEAD(&base->m_deal_que_tmp);
    INIT_LIST_HEAD(&base->m_wr_que); 
    INIT_LIST_HEAD(&base->m_wr_que_tmp); 

    INIT_TASK(&base->m_rd_task);
    INIT_TASK(&base->m_wr_task);
    INIT_TASK(&base->m_deal_task);
    INIT_TASK(&base->m_director_task); 
}

static Void finishBase(NodeBase* base) { 
    Int32 fd = base->m_fd;

    if (0 <= fd) { 
        base->m_fd = -1; 
        base->m_fd_status = ENUM_FD_END;
        
        freeMsgQue(&base->m_cmd_que);
        freeMsgQue(&base->m_cmd_que_tmp);
        freeMsgQue(&base->m_deal_que);
        freeMsgQue(&base->m_deal_que_tmp);
        freeMsgQue(&base->m_wr_que);
        freeMsgQue(&base->m_wr_que_tmp); 
        
        closeHd(fd); 
    }
}

static NodeBase* buildBase(Int32 fd, Int32 type) {
    NodeBase* base = NULL;
        
    I_NEW(NodeBase, base); 
    if (NULL != base) {
        resetBase(base);

        base->m_fd = fd;
        base->m_node_type = type;

        return base;
    } else {
        return NULL;
    } 
}

static Void freeBase(NodeBase* base) {
    if (NULL != base) {
        finishBase(base);
        I_FREE(base);
    }
}

struct SockNode {
    NodeBase m_base;
    SockBase m_sock;
};

static SockNode* buildSock(Int32 fd, Int32 type) { 
    SockNode* sock = NULL;
    
    I_NEW(SockNode, sock); 
    if (NULL != sock) {
        memset(&sock->m_sock, 0, sizeof(sock->m_sock));
        
        resetBase(&sock->m_base); 

        sock->m_base.m_fd = fd;
        sock->m_base.m_node_type = type; 

        /* extra data */
        sock->m_base.m_data = &sock->m_sock;

        return sock;
    } else {
        return NULL;
    } 
}

static Void freeSock(SockNode* sock) { 
    if (NULL != sock) {
        if (NULL != sock->m_sock.m_rcv_dummy) {
            MsgCenter::freeMsg(sock->m_sock.m_rcv_dummy);

            sock->m_sock.m_rcv_dummy = NULL;
        }
        
        finishBase(&sock->m_base);
        I_FREE(sock); 
    }
}

NodeBase* buildNode(Int32 fd, Int32 type) { 
    NodeBase* node = NULL;
    
    if (ENUM_NODE_SOCK == type || ENUM_NODE_SOCK_CONNECTOR == type) {
        node = (NodeBase*)buildSock(fd, type);    
    } else {
        node = buildBase(fd, type);
    }

    return node;
}

Void freeNode(NodeBase* base) {
    if (ENUM_NODE_SOCK == base->m_node_type 
        || ENUM_NODE_SOCK_CONNECTOR == base->m_node_type) {
        freeSock((SockNode*)base);
    } else {
        freeBase(base);
    }
}

SockBase* getSockBase(NodeBase* node) {
    SockBase* sock = NULL;
    
    if (NULL != node->m_data) {
        sock = (SockBase*)node->m_data;

        return sock;
    } else {
        return NULL;
    }
}

ObjCenter::ObjCenter(Director* director) : m_director(director) {
    memset(m_obj, 0, sizeof(m_obj));
}

ObjCenter::~ObjCenter() {
}

Int32 ObjCenter::init() {
    Int32 ret = 0;
    TimerObj* timer = NULL;
    EventObj* event = NULL;
    SockCommObj* sock = NULL;
    SockConnector* conn = NULL;
    SockListener* listener = NULL;

    I_NEW_1(TimerObj, timer, m_director);
    m_obj[ENUM_NODE_TIMER] = timer;
    
    I_NEW_1(EventObj, event, m_director);
    m_obj[ENUM_NODE_EVENT] = event;
    
    I_NEW_1(SockCommObj, sock, m_director);
    m_obj[ENUM_NODE_SOCK] = sock; 

    I_NEW_1(SockConnector, conn, m_director);
    m_obj[ENUM_NODE_SOCK_CONNECTOR] = conn; 

    I_NEW_1(SockListener, listener, m_director);
    m_obj[ENUM_NODE_LISTENER] = listener;
    
    return ret;
}

Void ObjCenter::finish() {
    for (int i=0; i<ENUM_NODE_OBJ_MAX; ++i) {
        I_FREE(m_obj[i]);
    }
}

I_NodeObj* ObjCenter::findObj(Int32 type) {
    I_NodeObj* obj = NULL;
    
    if (0 <= type && ENUM_NODE_OBJ_MAX > type && NULL != m_obj[type]) {
        obj = m_obj[type];   
    } 

    return obj;
}

Uint32 ObjCenter::readNode(NodeBase* base) {
    Uint32 ret = 0;
    I_NodeObj* obj = NULL;

    obj = findObj(base->m_node_type);
    if (NULL != obj) {
        ret = obj->readNode(base);

        LOG_DEBUG("read_node| fd=%d| type=%d| ret=%u|",
            base->m_fd, base->m_node_type, ret);
    } else {
        ret = BIT_EVENT_ERROR;

        LOG_ERROR("read_node| fd=%d| type=%d| msg=invalid type|",
            base->m_fd, base->m_node_type);
    }
        
    return ret;
}

Uint32 ObjCenter::writeNode(NodeBase* base) {
    Uint32 ret = 0;
    I_NodeObj* obj = NULL;

    obj = findObj(base->m_node_type);
    if (NULL != obj) {
        ret = obj->writeNode(base);

        LOG_DEBUG("write_node| fd=%d| type=%d| ret=%u|",
            base->m_fd, base->m_node_type, ret);
    } else {
        ret = BIT_EVENT_ERROR;

        LOG_ERROR("write_node| fd=%d| type=%d| msg=invalid type|",
            base->m_fd, base->m_node_type);
    }
        
    return ret;
}

Int32 ObjCenter::procMsg(struct NodeBase* base, struct MsgHdr* msg) {
    Int32 ret = 0;
    I_NodeObj* obj = NULL;

    obj = findObj(base->m_node_type);
    if (NULL != obj) {
        ret = obj->procMsg(base, msg);
        if (0 == ret) {
            LOG_DEBUG("deal_node_msg| fd=%d| type=%d| msg=ok|",
                base->m_fd, base->m_node_type);
        } else {
            LOG_INFO("deal_node_msg| fd=%d| type=%d| error=%d|",
                base->m_fd, base->m_node_type, ret);
        }
    } else {
        ret = -1;
        LOG_ERROR("deal_node_msg| fd=%d| type=%d| msg=invalid type|",
            base->m_fd, base->m_node_type);

        MsgCenter::freeMsg(msg);
    }

    return ret;
}

Void ObjCenter::procCmd(struct NodeBase* base, struct MsgHdr* msg) {
    I_NodeObj* obj = NULL;

    obj = findObj(base->m_node_type);
    if (NULL != obj) {
        obj->procCmd(base, msg);

        LOG_DEBUG("proc_node_cmd| fd=%d| type=%d|",
            base->m_fd, base->m_node_type);
    } else {
        LOG_ERROR("proc_node_cmd| fd=%d| type=%d| msg=invalid type|",
            base->m_fd, base->m_node_type);

        MsgCenter::freeMsg(msg);
    }
}

Void ObjCenter::eof(struct NodeBase* base) {
    I_NodeObj* obj = NULL;

    obj = findObj(base->m_node_type);
    if (NULL != obj) {
        obj->eof(base);

        LOG_DEBUG("end_node| fd=%d| type=%d|",
            base->m_fd, base->m_node_type);
    } else {
        LOG_ERROR("end_node| fd=%d| type=%d| msg=invalid type|",
            base->m_fd, base->m_node_type);
    } 
}

