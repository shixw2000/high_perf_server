#include"sharedheader.h"
#include"hlist.h"
#include"director.h"
#include"msgdealer.h"
#include"msgpool.h"


const MsgDealer::PDealer MsgDealer::m_funcs[MsgDealer::MSG_STEP_BUTT] = {
    &MsgDealer::startTask,
    &MsgDealer::procTask,
    &MsgDealer::endTask
};

MsgDealer::MsgDealer() {
    m_director = NULL;
    m_msgOper = NULL;
    
    m_leftsize = 0ULL;
    m_max = 0ULL;
    m_len = 0ULL;
    m_total = 0ULL;
    m_success = 0ULL;
    m_fail = 0ULL;
}

MsgDealer::~MsgDealer() {
}

int MsgDealer::init(SockDirector* director, MsgBaseQue* msgOper) {
    int ret = 0;

    m_director = director;
    m_msgOper = msgOper;

    ret = Worker::init();
    if (0 != ret) {
        return -1;
    }
    
    return 0;
}

void MsgDealer::finish() {
    m_director = NULL;
    m_msgOper = NULL;
    
    Worker::finish();
}

void MsgDealer::postRun() {
    m_director->stopSvc();
    
    Worker::postRun();
}

int MsgDealer::startDealWork(SockData* pData) {
    struct MsgDealState* pState = &pData->m_curDeal;

    pState->m_step = MSG_STEP_START;
    addNewTask(&pData->m_deallist);

    return 0;
}

int MsgDealer::startTask(SockData* pData) {
    struct MsgDealState* pState = &pData->m_curDeal;
    
    setTimeout(&pData->m_deal_timer, 
        SOCK_TIMER_TIMEOUT, DEF_LOGIN_TIMEOUT_TICK); 
    //upadteTimer(&pData->m_deal_timer);
    
    pState->m_step = MSG_STEP_PROC;
    return 0;
}

int MsgDealer::procTask(SockData* pData) {
    int ret = 0;
    int size = 0;
    int n = pData->m_fd;
    struct deque_root* msgque = &pData->m_rcvMsgQue;
    struct msg_type* pMsg = NULL; 
    
    pMsg = m_msgOper->pop(n, msgque); 
    if (NULL != pMsg) {
        do {
            size = pMsg->m_size;
            
            ret = m_director->process(pData, pMsg);
            if (0 == ret) {
                calcMsg(ret, size); 
            } else {
                calcMsg(ret, size);
                
                /* error */
                return DEAL_PROC_MSG_ERR;
            }

            pMsg = m_msgOper->pop(n, msgque); 
        } while (NULL != pMsg);
    }     
    
    return WORK_EVENT_OUT;
}

int MsgDealer::endTask(SockData* pData) {    
    return COMMON_ERR_SOCK_END;
}

void MsgDealer::failMsg(int reason, SockData* pData) {
    LOG_WARN("<<<<<<deal_fail| reason=%d| fd=%d|", reason, pData->m_fd);   

    deleteTimer(&pData->m_deal_timer);
    
    m_director->closeSock(reason, pData); 
}

void MsgDealer::calcMsgIn(int reason, int size) {
    ATOMIC_INC(&m_leftsize);
    ATOMIC_INC(&m_total);
}

void MsgDealer::calcMsg(int reason, int size) { 
    if (0 == reason) {
        ++m_success;
    } else {
        ++m_fail;
    }

    m_len += size;
    ATOMIC_DEC(&m_leftsize); 
    
    if (needPrnt(m_success)) {
        LOG_INFO("+++++dealer| success_cnt=%llu| fail_cnt=%llu|"
            " left_size=%llu|"
            " total_cnt=%llu| total_len=%llu|",
            m_success, m_fail, m_leftsize, m_total, m_len); 
        m_success = 0ULL;
        m_len = 0ULL;
        m_max = 0ULL;
    } 
}

unsigned int MsgDealer::procSock(SockData* pData) { 
    int ret = 0;
    struct MsgDealState* pState = &pData->m_curDeal;

    do {
        ret = (this->*(m_funcs[pState->m_step]))(pData); 
    } while (0 == ret);

    if (0 < ret) {
        return ret;
    } else { 
        failMsg(ret, pData); 
        return WORK_EVENT_END;
    } 
}

SockData* MsgDealer::castTask2Sock(struct TaskElement* task) { 
    SockData* pData = NULL;

    pData = containof(task, SockData, m_deallist); 
    return pData;
}

SockData* MsgDealer::castTimer2Sock(struct TimeTask* task) {
    SockData* pData = NULL;

    pData = containof(task, SockData, m_deal_timer); 
    return pData;
}

void MsgDealer::procSockEnd(SockData* pData) { 
    LOG_DEBUG("proc_deal_end| fd=%d| state=%d| msg=closing socket|", 
        pData->m_fd, pData->m_state);
    
    deleteTimer(&pData->m_deal_timer);

    m_msgOper->deleteAllMsg(pData->m_fd, &pData->m_rcvMsgQue, this);

    m_director->stopSock(pData);
    return;
}

void MsgDealer::dealTimeout(SockData* pData) {
    struct TimeTask* item = &pData->m_deal_timer;
    
    LOG_WARN("<<<<<<deal_timeout| fd=%d| type=%d| timeout=%d|",
        pData->m_fd, item->m_type, item->m_timeout);
    
    m_director->closeSock(DEAL_ERR_TIMEOUT, pData);
}


