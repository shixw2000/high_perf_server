#include"sharedheader.h"
#include"hlist.h"
#include"sender.h"
#include"socktool.h"
#include"director.h"
#include"msgpool.h"


MsgWriter::MsgWriter(Sender* sender, MsgBaseQue* msgOper) 
    : m_needFlowctl(false) { 
    m_sender = sender;
    m_msgOper = msgOper;
    m_total = 0;
}

MsgWriter::~MsgWriter() {
}

int MsgWriter::peek_deque(int maxsize, int offset,
    struct hlist* head, struct hlist* tail) {
    int cnt = 0;
    int maxlen = maxsize;
    struct hlist* item = NULL;
    struct AtomEle* ele = NULL;

    for (; head != tail && 0 < maxlen && cnt < DEF_SEND_VEC_SIZE; 
        head = item, ++cnt) {
        item = head->next;

        ele = containof(item, struct AtomEle, m_list); 
        if (0 == offset) {
            if (maxlen > ele->m_size) {
                m_vec[cnt].iov_base = ele->m_data;
                m_vec[cnt].iov_len = ele->m_size;
                maxlen -= ele->m_size; 
            } else { 
                m_vec[cnt].iov_base = ele->m_data;
                m_vec[cnt].iov_len = maxlen;
                maxlen = 0;
            } 
        } else {
            if (maxlen + offset > ele->m_size) {
                m_vec[cnt].iov_base = (char*)ele->m_data + offset;
                m_vec[cnt].iov_len = (ele->m_size - offset);
                maxlen -= m_vec[cnt].iov_len; 

                offset = 0;
            } else { 
                m_vec[cnt].iov_base = (char*)ele->m_data + offset;
                m_vec[cnt].iov_len = maxlen;
                maxlen = 0;
            }
        } 
    }

    m_total = maxsize - maxlen;    
    return cnt;
}

int MsgWriter::writeMsg(SockData* pData) {
    int wlen = 0;
    int cnt = 0;
    bool hasMore = false;
    struct MsgSndState* pState = &pData->m_curSnd; 
    struct deque_root* msgque = &pData->m_sndMsgQue;
    struct hlist* head = NULL;
    struct hlist* tail = NULL;
    int n = pData->m_fd; 

    head = m_msgOper->head(n, msgque);
    tail = m_msgOper->tail(n, msgque);
    if (head != tail) {
        cnt = peek_deque(pState->m_maxlenOnce, pState->m_pos, head, tail);
        wlen = SockTool::sendVec(pData->m_fd, m_vec, cnt);
        if (0 < wlen) {
            pState->m_totalSnd += wlen;
            
            hasMore = del_deque(pData, wlen, head, tail);
            if (wlen == m_total) {
                if (!hasMore) {
                    return WORK_EVENT_OUT;
                } else {
                    return WORK_EVENT_IN;
                } 
            } else {
                LOG_DEBUG("sendmsg| cnt=%d| total=%d| wlen=%d|"
                    " total_send=%lld| msg=wait writable|",
                    cnt, m_total, wlen, pState->m_totalSnd);
                
                return EV_SOCK_WRITABLE;
            }
        } else if (0 == wlen) {
            LOG_DEBUG("sendmsg| cnt=%d| total=%d| wlen=%d| total_send=%lld|"
                " msg=wait writable|",
                cnt, m_total, wlen, pState->m_totalSnd);

            return EV_SOCK_WRITABLE;
        } else {
            return SEND_ERR_SOCK_FAIL;
        }
    } else { 
        return WORK_EVENT_OUT;
    }
}

/* true: has more */
bool MsgWriter::del_deque(SockData* pData, int wlen,
    struct hlist* head, struct hlist* tail) {
    struct MsgSndState* pState = &pData->m_curSnd; 
    struct deque_root* msgque = &pData->m_sndMsgQue;
    struct hlist* item = NULL;
    struct AtomEle* ele = NULL;
    int cnt = 0;
    int n = pData->m_fd;

    for (; 0<wlen; head=item, ++cnt) {
        item = head->next;

        ele = containof(item, struct AtomEle, m_list); 
        if (0 == pState->m_pos) {
            if (wlen >= ele->m_size) {
                wlen -= ele->m_size;

                m_sender->calcMsg(0, ele->m_size);
                m_msgOper->releaseData(ele);
                m_msgOper->releaseEle(head);
                
            } else {
                pState->m_pos = wlen;
                break;
            }
        } else {
            if (wlen + pState->m_pos >= ele->m_size) { 
                wlen -= (ele->m_size - pState->m_pos);
                pState->m_pos = 0;
                
                m_sender->calcMsg(0, ele->m_size);
                m_msgOper->releaseData(ele);
                m_msgOper->releaseEle(head);
            } else {
                pState->m_pos += wlen;
                break;
            } 
        }
    } 

    if (0 < cnt) {
        m_msgOper->setHead(n, cnt, head, msgque);
    }

    return head != tail;
}

void MsgWriter::finishWrite(SockData* pData) {
}


const Sender::PDealer Sender::m_funcs[Sender::MSG_STEP_BUTT] = {
    &Sender::prepareWr,
    &Sender::chkConn,
    &Sender::establish,
    &Sender::sendQueue,
    &Sender::endTask,
};

Sender::Sender() { 
    m_writer = NULL;
    m_msgOper = NULL;
    m_director = NULL;

    m_max = 0;
    m_leftsize = 0ULL;

    m_len = 0ULL;
    m_total = 0ULL;
    m_success = 0ULL;
    m_fail = 0ULL;
}

Sender::~Sender() {
}

unsigned int Sender::procSock(SockData* pData) { 
    int ret = 0;
    struct MsgSndState* pState = &pData->m_curSnd; 

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

int Sender::init(SockDirector* director, MsgBaseQue* msgOper) {
    int ret = 0;

    m_director = director;
    m_msgOper = msgOper;
    
    ret = Worker::init();
    if (0 != ret) {
        return -1;
    } 

    ret = registerEvent(EV_SOCK_WRITABLE, WORK_TYPE_CUSTOMER);
    if (0 != ret) {
        return ret;
    }

    m_writer = new MsgWriter(this, msgOper);
    if (NULL == m_writer) {
        return -1;
    }
    
    return 0;
}

void Sender::finish() {
    if (NULL != m_writer) {
        delete m_writer;
        m_writer = NULL;
    }
    
    Worker::finish(); 
}

void Sender::postRun() {
    m_director->stopSvc();
    
    Worker::postRun();
}

int Sender::startSendWork(SockData* pData) {
    struct MsgSndState* pState = &pData->m_curSnd; 

    pData->m_state = (short)SOCK_STATE_ESTABLISHED; 
    pState->m_step = MSG_STEP_ESTABLISH;
    addNewTask(&pData->m_sndlist);

    m_director->addWrEvent(pData); 
    m_director->addRdEvent(pData);

    return 0;
}

int Sender::startConnWork(SockData* pData) {
    struct MsgSndState* pState = &pData->m_curSnd; 

    pData->m_state = (short)SOCK_STATE_CONNECTING; 
    pState->m_step = MSG_STEP_PREPARE_WR;
    addNewTask(&pData->m_sndlist);

    return 0;
}

void Sender::calcMsgIn(int reason, int size) {
    ATOMIC_INC(&m_leftsize);
    ATOMIC_INC(&m_total);
}

void Sender::calcMsg(int reason, int size) {
    if (0 == reason) {
        ++m_success;
    } else {
        ++m_fail;
    }
    
    m_len += size;
    ATOMIC_DEC(&m_leftsize); 

    if (needPrnt(m_success)) {
        LOG_INFO("++++sender| success_cnt=%llu| fail_cnt=%llu|"
            " max_que_size=%llu|"
            " left_size=%llu| total_cnt=%llu|  total_len=%llu|",
            m_success, m_fail,
            m_max, m_leftsize, 
            m_total, m_len); 
        m_success = 0ULL;
        m_len = 0ULL;
        m_max = 0ULL;
    } 
}

void Sender::failMsg(int reason, SockData* pData) { 
    struct MsgSndState* pState = &pData->m_curSnd; 
    
    LOG_WARN("<<<<<<write_fail| reason=%d| fd=%d|", reason, pData->m_fd);

    deleteTimer(&pData->m_snd_timer);
    pState->m_step = MSG_STEP_END; 
    m_director->closeSock(reason, pData);
}

int Sender::addWriteTask(struct TaskElement* task) { 
    int ret = 0;
    
    ret = produce(EV_SOCK_WRITABLE, task); 
    return ret;
}

SockData* Sender::castTask2Sock(struct TaskElement* task) { 
    SockData* pData = NULL;

    pData = containof(task, SockData, m_sndlist); 
    return pData;
}

SockData* Sender::castTimer2Sock(struct TimeTask* task) {
    SockData* pData = NULL;

    pData = containof(task, SockData, m_snd_timer); 
    return pData;
}

void Sender::procSockEnd(SockData* pData) { 
    LOG_DEBUG("proc_send_end| fd=%d| state=%d| msg=closing socket|", 
        pData->m_fd, pData->m_state);
    
    deleteTimer(&pData->m_snd_timer);  
    m_writer->finishWrite(pData);
    m_msgOper->deleteAllMsg(pData->m_fd, &pData->m_sndMsgQue, this);
    
    SockTool::shutdownSock(pData->m_fd, SOCK_WRITE);
    m_director->stopSock(pData);
    return;
}

void Sender::dealTimeout(SockData* pData) {
    struct TimeTask* item = &pData->m_snd_timer;
    
    LOG_WARN("<<<<<<write_timeout| fd=%d| type=%d| timeout=%d|",
        pData->m_fd, item->m_type, item->m_timeout);
    
    if (SOCK_TIMER_TIMEOUT == item->m_type) {
        m_director->closeSock(SEND_ERR_TIMEOUT, pData);
    } else if (SOCK_TIMER_CONNECT_WR == item->m_type) {
        m_director->closeSock(SOCK_CONNECT_TIMEOUT_ERR, pData);
    }
}

int Sender::prepareWr(SockData* pData) {
    struct MsgSndState* pState = &pData->m_curSnd; 
    
    setTimeout(&pData->m_snd_timer, 
        SOCK_TIMER_CONNECT_WR, DEF_CONNECT_TIMEOUT_TICK); 
    
    upadteTimer(&pData->m_snd_timer);

    pState->m_step = MSG_STEP_CONNECT;
    m_director->addWrEvent(pData); 
    return WORK_EVENT_OUT;
}

int Sender::chkConn(SockData* pData) {
    int ret = 0;
    struct MsgSndState* pState = &pData->m_curSnd; 

    deleteTimer(&pData->m_snd_timer);
    
    ret = SockTool::getConnCode(pData->m_fd);
    if (0 == ret) { 
        ret = m_director->asyncConnOk(pData);
        if (0 == ret) {
            pData->m_state = (short)SOCK_STATE_ESTABLISHED; 
            pState->m_step = MSG_STEP_ESTABLISH; 
            
            m_director->addRdEvent(pData);

            return 0;
        } else {
            return SOCK_CONNECT_POST_ERR;
        }
        
    } else {
        return SOCK_CONNECT_CHK_ERR;
    }
}

int Sender::establish(SockData* pData) {
    struct MsgSndState* pState = &pData->m_curSnd; 

    pState->m_maxlenOnce = DEF_SEND_MSG_SIZE_ONCE; 
    
    setTimeout(&pData->m_snd_timer, 
        SOCK_TIMER_TIMEOUT, DEF_SEND_TIMEOUT_TICK); 
    
    upadteTimer(&pData->m_snd_timer);

    pState->m_step = MSG_STEP_SEND_MSG;
    return 0;
}

int Sender::sendQueue(SockData* pData) {
    int ret = 0;
    unsigned int queuesize = 0;

    queuesize = MsgQueOper::size(&pData->m_sndMsgQue);
    if (m_max < queuesize) {
        m_max = queuesize;
    }
    
    ret = m_writer->writeMsg(pData); 
    if (0 < ret) {
        upadteTimer(&pData->m_snd_timer);
    }
    
    return ret;
}

int Sender::endTask(SockData* pData) {
    return WORK_EVENT_END;
}

