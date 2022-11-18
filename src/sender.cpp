#include"globaltype.h"
#include"sender.h"
#include"sockutil.h"
#include"ticktimer.h"
#include"director.h"
#include"msgtype.h"


Sender::Sender(Director* director) : m_director(director) {
    m_timer = NULL;
}

Sender::~Sender() {
}

Int32 Sender::init() {
    Int32 ret = 0;

    ret = TaskPool::init();
    if (0 != ret) {
        return ret;
    }

    I_NEW(TickTimer, m_timer);
    m_timer->setDealer(this);

    return ret;
}

Void Sender::finish() { 
    if (NULL != m_timer) {
        I_FREE(m_timer);
    }
    
    TaskPool::finish();
}

Void Sender::doTick(Uint32 cnt) {
    m_timer->tick(cnt); 
}

void Sender::addTimer(struct TimerEle* ele, 
    Int32 type, Uint32 interval) {
    
    ele->m_type = type;
    ele->m_interval = interval;
    
    m_timer->addTimer(ele);
}

unsigned int Sender::procTask(struct Task* task) {
    Uint32 ret = 0;
    NodeBase* base = NULL;

    base = list_entry(task, NodeBase, m_wr_task);
    
    ret = procNodeQue(base); 
    return ret;  
}

void Sender::procTaskEnd(struct Task* task) {
    NodeBase* base = NULL;

    base = list_entry(task, NodeBase, m_wr_task);

    if (!base->m_wr_err) {
        procNodeQue(base);
    }

    LOG_INFO("proc_task_end| fd=%d| type=%d| name=sender|",
        base->m_fd, base->m_node_type);

    m_director->notify(ENUM_THREAD_DIRECTOR, base, 
        ENUM_MSG_SYSTEM_STOP, ENUM_FD_CLOSE_WR);
    return;
}

int Sender::setup() {
    Int32 ret = 0;

    ret = TaskPool::setup();
    if (0 != ret) {
        return ret;
    }

    return ret;
}

void Sender::teardown() {
    if (NULL != m_timer) {
        m_timer->stop();
    }
    
    TaskPool::teardown();
}

Void Sender::doTimeout(struct TimerEle*) {
}

Uint32 Sender::procNodeQue(NodeBase* base) {
    Int32 ret = 0;
    Bool bOk = TRUE;

    bOk = m_director->lock(base);
    if (bOk) {
        if (!list_empty(&base->m_wr_que_tmp)) {
            list_splice_back(&base->m_wr_que_tmp, &base->m_wr_que);
        }
        
        m_director->unlock(base);
    }

    ret = m_director->writeNode(base); 

    return ret;
}


