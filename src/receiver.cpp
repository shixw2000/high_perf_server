#include"sharedheader.h"
#include"hlist.h"
#include"socktool.h"
#include"director.h"
#include"msgpool.h"
#include"receiver.h"


MsgReader::MsgReader(Receiver* receiver) : m_needFlowctl(false) {
    m_total = 0;
    m_receiver = receiver;

    memset(m_buffer, 0, sizeof(m_buffer));
    memset(m_vec, 0, sizeof(m_vec));
}

MsgReader::~MsgReader() {
}

void MsgReader::finishRead(SockData* pData) {
    struct MsgRcvState* pState = &pData->m_curRcv;
    
    if (NULL != pState->m_msg) {
        MsgQueOper::freeMsg(pState->m_msg);

        pState->m_msg = NULL;
    }
} 

int MsgReader::prepareHeader(SockData* pData) {
    int maxlen = 0;
    int cnt = 0;
    struct MsgRcvState* pState = &pData->m_curRcv;

    maxlen = pState->m_maxlenOnce;
    m_total = 0;
    
    m_vec[cnt].iov_base = pState->m_end + pState->m_size;
    m_vec[cnt].iov_len = pState->m_capacity - pState->m_size;
    maxlen -= m_vec[cnt].iov_len;
    
    m_total += m_vec[cnt].iov_len;
    ++cnt;

    m_vec[cnt].iov_base = m_buffer;
    if (maxlen <= DEF_RECV_MSG_SIZE_ONCE) {
        m_vec[cnt].iov_len = maxlen;
        maxlen = 0;
    } else {
        m_vec[cnt].iov_len = DEF_RECV_MSG_SIZE_ONCE;
        maxlen -= DEF_RECV_MSG_SIZE_ONCE;
    }

    m_total += m_vec[cnt].iov_len;
    ++cnt;
    return cnt;
}

int MsgReader::prepareBody(SockData* pData) {
    int maxlen = 0;
    int cnt = 0;
    struct MsgRcvState* pState = &pData->m_curRcv;

    maxlen = pState->m_maxlenOnce;
    m_total = 0;
    
    if (maxlen + pState->m_size > pState->m_capacity) {
        m_vec[cnt].iov_base = pState->m_msg->m_buf + pState->m_size;
        m_vec[cnt].iov_len = pState->m_capacity - pState->m_size;
        
        maxlen -= m_vec[cnt].iov_len;
        m_total += m_vec[cnt].iov_len;
        ++cnt;

        m_vec[cnt].iov_base = m_buffer;
        if (maxlen <= DEF_RECV_MSG_SIZE_ONCE) {
            m_vec[cnt].iov_len = maxlen;
            maxlen = 0;
        } else {
            m_vec[cnt].iov_len = DEF_RECV_MSG_SIZE_ONCE;
            maxlen -= DEF_RECV_MSG_SIZE_ONCE;
        }

        m_total += m_vec[cnt].iov_len;
        ++cnt;
    } else {
        m_vec[cnt].iov_base = pState->m_msg->m_buf + pState->m_size;
        m_vec[cnt].iov_len = maxlen;

        m_total = m_vec[cnt].iov_len;
        ++cnt;
    } 
    
    return cnt; 
}

int MsgReader::readMsg(int cnt, SockData* pData, PDealer cb) {
    int ret = 0;
    int rlen = 0;
    struct MsgRcvState* pState = &pData->m_curRcv;
    
    rlen = SockTool::recvVec(pData->m_fd, m_vec, cnt); 
    if (0 < rlen) { 
        pState->m_size += rlen;
        
        if (pState->m_size >= pState->m_capacity) {
            ret = (this->*cb)(pData);
        } 

        if (0 == ret) {
            if (rlen == m_total) {
                return WORK_EVENT_IN;
            } else {
                return EV_SOCK_READABLE;
            }
        } else {
            return ret;
        }
    } else if (0 == rlen) {
        return EV_SOCK_READABLE;
    } else {
        return RECV_ERR_SOCK_FAIL;
    }
}

int MsgReader::readHeader(SockData* pData) {
    int ret = 0;
    int cnt = 0;

    cnt = prepareHeader(pData);
    ret = readMsg(cnt, pData, &MsgReader::parseHeader);
    return ret;
} 

int MsgReader::readBody(SockData* pData) {
    int ret = 0;
    int cnt = 0;

    cnt = prepareBody(pData);
    ret = readMsg(cnt, pData, &MsgReader::parseBody);
    return ret;
}

int MsgReader::parseHeader(SockData* pData) {
    int ret = 0;
    int cplen = 0;
    struct MsgRcvState* pState = &pData->m_curRcv;
    struct MsgHeader* pHd = &pState->m_hd;
    struct msg_type* pMsg = NULL;

    ret = m_receiver->chkHeader(pHd);
    if (0 == ret) { 
        pMsg = MsgQueOper::allocMsg(pHd->m_size);
        if (NULL != pMsg) { 
            if (pState->m_size >= pHd->m_size) {
                MY_COPY(pMsg->m_buf, pState->m_end, DEF_MSG_HEADER_SIZE);
                
                cplen = pHd->m_size - DEF_MSG_HEADER_SIZE;
                MY_COPY(pMsg->m_buf + DEF_MSG_HEADER_SIZE, m_buffer, cplen); 

                pState->m_size -= DEF_MSG_HEADER_SIZE;
                pState->m_capacity = cplen;

                ret = m_receiver->dispatchMsg(pData, pMsg);
                if (0 == ret) { 
                    ret = parseNextBody(pData);
                } 
            } else {
                MY_COPY(pMsg->m_buf, pState->m_end, DEF_MSG_HEADER_SIZE);
            
                cplen = pState->m_size - DEF_MSG_HEADER_SIZE;
                MY_COPY(pMsg->m_buf + DEF_MSG_HEADER_SIZE, m_buffer, cplen); 
                
                pState->m_msg = pMsg;
                /* size is equal read byte, cannot substract header size */
                pState->m_capacity = pHd->m_size; 
                
                pState->m_step = Receiver::MSG_STEP_READ_BODY; 
            }
        } else {
            ret = RECV_ERR_NO_MEMORY;
        }
    } 

    return ret;
}

int MsgReader::parseBody(SockData* pData) {
    int ret = 0;
    struct MsgRcvState* pState = &pData->m_curRcv;

    pState->m_size -= pState->m_capacity;
    pState->m_capacity = 0;
    
    ret = m_receiver->dispatchMsg(pData, pState->m_msg);
    if (0 == ret) {
        pState->m_msg = NULL; 
        ret = parseNextBody(pData);
    } else {
        pState->m_msg = NULL;
    }
    
    return ret;
}

int MsgReader::parseNextBody(SockData* pData) {
    int ret = 0;
    int cplen = 0;
    struct MsgRcvState* pState = &pData->m_curRcv;

    while (pState->m_size >= pState->m_capacity + DEF_MSG_HEADER_SIZE) {
        ret = fillMsg(pData);
        if (0 != ret) {
            return ret;
        }
    } 

    if (pState->m_size >= pState->m_capacity) {
        cplen = pState->m_size - pState->m_capacity; 
        if (0 < cplen) {
            memcpy(pState->m_end, m_buffer + pState->m_capacity, cplen);
        }
        
        pState->m_size = cplen;
        pState->m_capacity = DEF_MSG_HEADER_SIZE;
        pState->m_step = Receiver::MSG_STEP_READ_HEADER; 
    } else {
        pState->m_step = Receiver::MSG_STEP_READ_BODY; 
    }

    return 0;
}

int MsgReader::fillMsg(SockData* pData) {
    int ret = 0;
    int cplen = 0;
    struct MsgRcvState* pState = &pData->m_curRcv;
    struct MsgHeader* pHd = NULL;
    struct msg_type* pMsg = NULL;

    pHd = (struct MsgHeader*)(m_buffer + pState->m_capacity); 
    ret = m_receiver->chkHeader(pHd);
    if (0 == ret) {
        pMsg = MsgQueOper::allocMsg(pHd->m_size);
        if (NULL != pMsg) {
            /* has full pkg */
            if (pHd->m_size + pState->m_capacity <= pState->m_size) {
                cplen = pHd->m_size;
                MY_COPY(pMsg->m_buf, m_buffer + pState->m_capacity, cplen);

                pState->m_capacity += cplen;
                ret = m_receiver->dispatchMsg(pData, pMsg);
            } else {
                cplen = pState->m_size - pState->m_capacity;
                MY_COPY(pMsg->m_buf, m_buffer + pState->m_capacity, cplen);

                pState->m_msg = pMsg;
                pState->m_capacity = pHd->m_size;
                pState->m_size = cplen;
            }
        } else {
            ret = RECV_ERR_NO_MEMORY;
        }
    } 

    return ret;
}


const Receiver::PDealer Receiver::m_funcs[Receiver::MSG_STEP_BUTT] = {
    &Receiver::prepare,
    &Receiver::readHeader,
    &Receiver::readBody,
    &Receiver::endTask
};

Receiver::Receiver() {
    m_reader = NULL;
    m_director = NULL; 
    
    m_max = 0ULL;
    m_len = 0ULL;
    m_total = 0ULL;
    m_success = 0ULL;
    m_fail = 0ULL;
}

Receiver::~Receiver() {
}

int Receiver::prepare(SockData* pData) {
    struct MsgRcvState* pState = &pData->m_curRcv;
    
    pState->m_maxlenOnce = DEF_RECV_MSG_SIZE_ONCE;
    pState->m_capacity = DEF_MSG_HEADER_SIZE; 

    setTimeout(&pData->m_rcv_timer, 
        SOCK_TIMER_TIMEOUT, DEF_RECV_TIMEOUT_TICK);
    upadteTimer(&pData->m_rcv_timer);

    pState->m_step = MSG_STEP_READ_HEADER; 
    return 0; 
}

int Receiver::readHeader(SockData* pData) {
    int ret = 0;
    unsigned int queuesize = 0U;

    queuesize = MsgQueOper::size(&pData->m_rcvMsgQue);
    if (m_max < queuesize) {
        m_max = queuesize;
    }

    ret = m_reader->readHeader(pData);
    if (0 < ret) {
        upadteTimer(&pData->m_rcv_timer);
    }
    
    return ret;
}

int Receiver::readBody(SockData* pData) {
    int ret = 0;
    unsigned int queuesize = 0U;

    queuesize = MsgQueOper::size(&pData->m_rcvMsgQue);
    if (m_max < queuesize) {
        m_max = queuesize;
    }

    ret = m_reader->readBody(pData);
    if (0 < ret) {
        upadteTimer(&pData->m_rcv_timer);
    }
    
    return ret;
}

int Receiver::endTask(SockData* pData) {
    return WORK_EVENT_END;
}

int Receiver::dispatchMsg(SockData* pData, struct msg_type* pMsg) {
    int ret = 0;
    int size = pMsg->m_size; 

    ret = m_director->dispatchMsg(pData, pMsg);
    if (0 == ret) {
        calcMsg(ret, size); 
    } else {
        ret = RECV_ERR_DISPATCH_MSG;
        calcMsg(ret, size); 
    }
    
    return ret;
}

void Receiver::calcMsg(int reason, int size) {    
    ++m_total;
    if (0 == reason) {
        ++m_success;
    } else {
        ++m_fail;
    }
    
    m_len += size;

    if (needPrnt(m_success)) {        
        LOG_INFO("+++++receiver| success_cnt=%llu| fail_cnt=%llu|"
            " max_que_size=%llu|"
            " total_cnt=%llu| "
            " total_len=%llu|",
            m_success, m_fail, m_max, m_total, m_len); 
        m_success = 0ULL;
        m_len = 0ULL;
        m_max = 0ULL;
    }
}

SockData* Receiver::castTask2Sock(struct TaskElement* task) { 
    SockData* pData = NULL;

    pData = containof(task, SockData, m_rcvlist); 
    return pData;
}

SockData* Receiver::castTimer2Sock(struct TimeTask* task) {
    SockData* pData = NULL;

    pData = containof(task, SockData, m_rcv_timer); 
    return pData;
}

unsigned int Receiver::procSock(SockData* pData) {
    int ret = 0;
    struct MsgRcvState* pState = &pData->m_curRcv;
    

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

void Receiver::procSockEnd(SockData* pData) {
    LOG_DEBUG("proc_receive_end| fd=%d| state=%d| msg=closing socket|", 
        pData->m_fd, pData->m_state);
    
    deleteTimer(&pData->m_rcv_timer);
    m_reader->finishRead(pData);

    SockTool::shutdownSock(pData->m_fd, SOCK_READ);
    m_director->stopSock(pData);
    
    return;
}

void Receiver::dealTimeout(SockData* pData) {
    struct TimeTask* item = &pData->m_rcv_timer;
    
    LOG_WARN("<<<<<<read_timeout| fd=%d| type=%d| timeout=%d|",
        pData->m_fd, item->m_type, item->m_timeout);

    m_director->closeSock(RECV_ERR_TIMEOUT, pData);
}

void Receiver::failMsg(int reason, SockData* pData) {
    struct MsgRcvState* pState = &pData->m_curRcv;
    
    LOG_WARN("<<<<<<read_fail| reason=%d| fd=%d|", reason, pData->m_fd);   

    deleteTimer(&pData->m_rcv_timer);
    pState->m_step = MSG_STEP_END; 
    m_director->closeSock(reason, pData);
}

int Receiver::init(SockDirector* director) {
    int ret = 0;

    m_director = director;
    
    ret = Worker::init();
    if (0 != ret) {
        return ret;
    }

    ret = registerEvent(EV_SOCK_READABLE, WORK_TYPE_CUSTOMER);
    if (0 != ret) {
        return ret;
    }

    m_reader = new MsgReader(this);
    if (NULL == m_reader) {
        return -1;
    }
    
    return 0;
}

void Receiver::finish() {     
    if (NULL != m_reader) {
        delete m_reader;
        m_reader = NULL;
    }

    Worker::finish();
}

void Receiver::postRun() {
    m_director->stopSvc();
    
    Worker::postRun();
}

int Receiver::startReceiveWork(SockData* pData) {
    struct MsgRcvState* pState = &pData->m_curRcv;

    pState->m_step = MSG_STEP_PREPARE;
    addNewTask(&pData->m_rcvlist);

    return 0;
}

int Receiver::addReadTask(struct TaskElement* task) { 
    int ret = 0;
    
    ret = produce(EV_SOCK_READABLE, task); 
    return ret;
}

int Receiver::chkHeader(const struct MsgHeader* pHd) { 
    int ret = 0;
    
    if (DEF_MSG_VERSION == pHd->m_ver) {
        if (MAX_PKG_MSG_SIZE >= pHd->m_size
            && DEF_MSG_HEADER_SIZE <= pHd->m_size) {
            ret = 0;
        } else {
            ret = RECV_ERR_HEADER_SIZE;
        } 
    } else {
        ret = RECV_ERR_HEAER_PROTOCOL;
    }

    return ret;
}


