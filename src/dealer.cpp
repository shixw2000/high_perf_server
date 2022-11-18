#include"dealer.h"
#include"ticktimer.h"
#include"director.h"
#include"msgcenter.h"


Dealer::Dealer(Director* director) : m_director(director) {
    m_timer = NULL;
}

Dealer::~Dealer() {
}

Int32 Dealer::init() {
    Int32 ret = 0;

    ret = TaskPool::init();
    if (0 != ret) {
        return ret;
    }

    I_NEW(TickTimer, m_timer);
    m_timer->setDealer(this);

    return ret;
}

Void Dealer::finish() {
    if (NULL != m_timer) {
        I_FREE(m_timer);
    }
    
    TaskPool::finish();
}

Void Dealer::doTick(Uint32 cnt) {
    m_timer->tick(cnt); 
}

void Dealer::addTimer(struct TimerEle* ele, 
    Int32 type, Uint32 interval) {
    
    ele->m_type = type;
    ele->m_interval = interval;

    m_timer->addTimer(ele);
} 

unsigned int Dealer::procTask(struct Task* task) {
    NodeBase* base = NULL;

    base = list_entry(task, NodeBase, m_deal_task);

    procNodeQue(base);
    
    return BIT_EVENT_NORM; 
}

void Dealer::procTaskEnd(struct Task* task) {
    NodeBase* base = NULL;

    base = list_entry(task, NodeBase, m_deal_task);

    LOG_INFO("proc_task_end| fd=%d| type=%d| name=dealer|",
        base->m_fd, base->m_node_type);

    m_director->notify(ENUM_THREAD_DIRECTOR, base, 
        ENUM_MSG_SYSTEM_STOP, ENUM_FD_CLOSE_DEALER);
    return;
}

Void Dealer::procNodeQue(NodeBase* base) { 
    Bool bOk = TRUE;
    list_head* list = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    MsgHdr* msg = NULL;

    bOk = m_director->lock(base);
    if (bOk) {
        if (!list_empty(&base->m_deal_que_tmp)) {
            list_splice_back(&base->m_deal_que_tmp, &base->m_deal_que);
        }
        
        m_director->unlock(base);
    }
    
    list = &base->m_deal_que;
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            msg = MsgCenter::cast(pos);
            
            m_director->dealMsg(base, msg);
        }
    }
}

int Dealer::setup() {
    Int32 ret = 0;

    ret = TaskPool::setup();
    if (0 != ret) {
        return ret;
    }

    return ret;
}

void Dealer::teardown() {
    if (NULL != m_timer) {
        m_timer->stop();
    }
    
    TaskPool::teardown();
}

Void Dealer::doTimeout(struct TimerEle*) {
}

