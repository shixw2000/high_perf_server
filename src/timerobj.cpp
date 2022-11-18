#include"timerobj.h"
#include"ticktimer.h"
#include"msgcenter.h"
#include"director.h"
#include"sockutil.h"


Uint32 TimerObj::readNode(NodeBase* base) {
    Int32 ret = 0;
    Uint32 val = 0;

    ret = readEvent(base->m_fd, &val); 
    if (0 == ret) {
        m_director->notify(ENUM_THREAD_DIRECTOR, base, 
            ENUM_MSG_SYSTEM_TICK_TIMER, val);
        
        m_director->notify(ENUM_THREAD_WRITE, base, 
            ENUM_MSG_SYSTEM_TICK_TIMER, val);
        
        m_director->notify(ENUM_THREAD_DEALER, base, 
            ENUM_MSG_SYSTEM_TICK_TIMER, val); 
        
        m_director->procTick(ENUM_THREAD_READ, val);
    }

    return BIT_EVENT_NORM;
}

Uint32 TimerObj::writeNode(NodeBase* base) {
    list_head* list = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    MsgHdr* msg = NULL;

    list = &base->m_wr_que;
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            msg = MsgCenter::cast(pos);
            procTickTimer(ENUM_THREAD_WRITE, msg);
        }
    }

    return BIT_EVENT_NORM;
}

Void TimerObj::procTickTimer(EnumThreadType type, struct MsgHdr* msg) { 
    if (ENUM_MSG_SYSTEM_TICK_TIMER == msg->m_cmd) {
        Uint32 cnt = 0;
        MsgNotify* body = MsgCenter::body<MsgNotify>(msg);

        cnt = (Uint32)body->m_data;
        
        m_director->procTick(type, cnt); 
    } 
    
    MsgCenter::freeMsg(msg);
}

Int32 TimerObj::procMsg(struct NodeBase*, struct MsgHdr* msg) {
    procTickTimer(ENUM_THREAD_DEALER, msg);
    
    return 0;
}

Void TimerObj::procCmd(struct NodeBase*, struct MsgHdr* msg) {
    procTickTimer(ENUM_THREAD_DIRECTOR, msg);
}

Void TimerObj::eof(struct NodeBase* ) {
    //do not free here 
    m_director->stop();
}


Uint32 EventObj::readNode(NodeBase* base) {
    Uint32 val = 0;

    readEvent(base->m_fd, &val); 

    return BIT_EVENT_NORM;
}

Uint32 EventObj::writeNode(NodeBase* base) { 
    
    freeMsgQue(&base->m_wr_que);
    return BIT_EVENT_NORM;
}

Int32 EventObj::procMsg(struct NodeBase*, struct MsgHdr* msg) {
    MsgCenter::freeMsg(msg); 

    return 0;
}

Void EventObj::eof(struct NodeBase*) {    
    //do not free here 
    m_director->stop();
}

Void EventObj::procCmd(struct NodeBase* base, struct MsgHdr* msg) {    
    if (ENUM_MSG_SYSTEM_ADD_ADDRESS == msg->m_cmd) {
        procAddress(base, msg);
    } else {
        MsgCenter::freeMsg(msg);
    } 
}

Void EventObj::procAddress(struct NodeBase*, struct MsgHdr* msg) {
    Int32 ret = 0;
    MsgSockAddr* body = MsgCenter::body<MsgSockAddr>(msg);

    if (body->m_is_serv) {
        ret = m_director->addListener(body->m_ip, body->m_port);
    } else {
        ret = m_director->addConnector(body->m_ip, body->m_port);
    }

    LOG_DEBUG("proc_address| cmd=0x%x| is_server=%d|"
        " ip=%s| port=%d| ret=%d|",
        msg->m_cmd, body->m_is_serv,
        body->m_ip, body->m_port, ret);

    MsgCenter::freeMsg(msg);
} 

