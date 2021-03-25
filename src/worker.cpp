#include"sharedheader.h"
#include"sockdata.h"
#include"task.h"
#include"worker.h" 


Worker::Worker() {
    m_timer = NULL;
}

Worker::~Worker() {
}

int Worker::init() {
    int ret = 0;
    
    ret = TaskHandler::init();
    if (0 != ret) {
        return ret;
    }

    /* system events */
    registerEvent(WORK_EVENT_IN, WORK_TYPE_LOOP);
    registerEvent(WORK_EVENT_OUT, WORK_TYPE_ONCE);
    registerEvent(WORK_EVENT_END, WORK_TYPE_CUSTOMER);
    registerEvent(EV_FLOW_CONTROL, WORK_TYPE_CUSTOMER);

    m_timer = new TimerList(DEF_TIMER_SIZE_ORDER);
    if (NULL == m_timer) {
        return -1;
    }

    ret = m_timer->init();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void Worker::finish() {   
    if (NULL != m_timer) {
        m_timer->finish();
        delete m_timer;
        m_timer = NULL;
    }
    
    TaskHandler::finish();
}

int Worker::addEndTask(struct TaskElement* task) {
    int ret = 0;

    ret = produce(WORK_EVENT_END, task);
    return ret;
}

int Worker::addNewTask(struct TaskElement* task) {
    int ret = 0;

    ret = produce(WORK_EVENT_IN, task);
    return ret;
}

int Worker::addFlowCtlTask(struct TaskElement* task) {
    int ret = 0;

    ret = produce(EV_FLOW_CONTROL, task);
    return ret;
}

unsigned int Worker::procTask(unsigned int state, 
    struct TaskElement* task) { 
    unsigned int ev = 0U;
    SockData* pData = NULL;

    pData = castTask2Sock(task); 
    if (POLL_TYPE_SOCKET == pData->m_type) {
        if (!isEventSet(WORK_EVENT_END, state)) {
            ev = procSock(pData);
        } else {
            procSockEnd(pData);
            ev = WORK_EVENT_END;
        }
    } else if (POLL_TYPE_TIMER == pData->m_type) {
        procTimer();
        ev = WORK_EVENT_OUT;
    } else {
        LOG_WARN("procTask| state=0x%x| fd=%d| type=%d|"
            " error=invalid type|",
            state, pData->m_fd, pData->m_type);

        ev = WORK_EVENT_END;
    }
    
    return ev;
}

void Worker::procTimeout(struct TimeTask* item) {
    SockData* pData = NULL;

    pData = castTimer2Sock(item);    
    dealTimeout(pData);
}

void Worker::procTimer() {
    m_timer->step();
}

void Worker::setTimeout(struct TimeTask* item, int type, int timeout) {
    TimerList::setTimerParam(item, type, timeout, this);
}

void Worker::upadteTimer(struct TimeTask* item) {
    if (0 < item->m_timeout) {
        m_timer->updateTimer(item);
    }
}

void Worker::deleteTimer(struct TimeTask* item) {
    m_timer->delTimer(item);
}

