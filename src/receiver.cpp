#include"globaltype.h"
#include"receiver.h"
#include"ticktimer.h"
#include"director.h"
#include"msgtype.h"


Receiver::Receiver(Director* director) : m_director(director) {
    m_timer = NULL;
}

Receiver::~Receiver() {
}

Int32 Receiver::init() {
    Int32 ret = 0;

    ret = TaskPool::init();
    if (0 != ret) {
        return ret;
    }

    I_NEW(TickTimer, m_timer);
    m_timer->setDealer(this);

    return ret;
}

Void Receiver::finish() {
    if (NULL != m_timer) {
        I_FREE(m_timer);
    }
    
    TaskPool::finish();
}

Void Receiver::doTick(Uint32 cnt) {
    m_timer->tick(cnt); 
}

void Receiver::addTimer(struct TimerEle* ele, 
    Int32 type, Uint32 interval) {
    
    ele->m_type = type;
    ele->m_interval = interval;
    
    m_timer->addTimer(ele);
}

unsigned int Receiver::procTask(struct Task* task) {
    Uint32 ret = 0;
    NodeBase* base = NULL;

    base = list_entry(task, NodeBase, m_rd_task);

    ret = readNode(base); 
    return ret; 
}

void Receiver::procTaskEnd(struct Task* task) {
    NodeBase* base = NULL;

    base = list_entry(task, NodeBase, m_rd_task);

    LOG_INFO("proc_task_end| fd=%d| type=%d| name=recver|",
        base->m_fd, base->m_node_type);
    
    m_director->notify(ENUM_THREAD_DIRECTOR, base,
        ENUM_MSG_SYSTEM_STOP, ENUM_FD_CLOSE_RD);
    return;
}

int Receiver::setup() {
    Int32 ret = 0;

    ret = TaskPool::setup();
    if (0 != ret) {
        return ret;
    }

    return ret;
}

void Receiver::teardown() {
    if (NULL != m_timer) {
        m_timer->stop();
    }
    
    TaskPool::teardown();
}

Void Receiver::doTimeout(struct TimerEle*) {
}

Uint32 Receiver::readNode(NodeBase* base) {
    Uint32 ret = 0;
    
    ret = m_director->readNode(base); 
    return ret; 
}

