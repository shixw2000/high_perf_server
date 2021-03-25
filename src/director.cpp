#include"sharedheader.h"
#include"socktool.h"
#include"receiver.h"
#include"sender.h"
#include"msgdealer.h"
#include"sockdeal.h"
#include"sockdata.h"
#include"sockconf.h"
#include"director.h"


SockDirector::SockDirector() {    
    m_receiver = NULL;
    m_sender = NULL;
    m_dealer = NULL;

    m_msgOper = NULL;
    m_cache = NULL;
    m_center = NULL;
    m_sockConf = NULL;

    m_hasSvc = false;
}

SockDirector::~SockDirector() {
}

int SockDirector::init() {
    int ret = 0;

    do {           
        ret = CPushPool::init();
        if (0 != ret) {
            break;
        }

        registerEvent(DIRECT_STOP_SOCK, WORK_TYPE_CUSTOMER);
        registerEvent(WORK_EVENT_END, WORK_TYPE_CUSTOMER);
        registerEvent(WORK_EVENT_OUT, WORK_TYPE_ONCE);

        m_cache = new SockCache(MAX_SOCK_FD_SIZE);
		if (NULL == m_cache) {
            ret = -1;
			break;
		}

        ret = m_cache->createCache();
		if (0 != ret) {
			break;
		}

        m_msgOper = new MsgBaseQue;
        if (NULL == m_msgOper) {
            ret = -1;
            break;
        }

        ret = m_msgOper->init();
        if (0 != ret) {
    		break;
    	}

        m_sender = new Sender;
        ret = m_sender->init(this, m_msgOper);
        if (0 != ret) {
    		break;
    	}

        m_dealer = new MsgDealer;
        ret = m_dealer->init(this, m_msgOper);
        if (0 != ret) {
    		break;
    	} 

        m_receiver = new Receiver;
        ret = m_receiver->init(this);
        if (0 != ret) {
    		break;
    	}
        
        return 0;
    } while (false);

    return ret;
}

void SockDirector::setCenter(SockDeal* center) {
    m_center = center;
}

void SockDirector::setConf(SockConf* sockConf) {
    m_sockConf = sockConf;
}

void SockDirector::finish() { 
    if (NULL != m_receiver) {
        m_receiver->finish();
        
        delete m_receiver;
        m_receiver = NULL;
    }

    if (NULL != m_dealer) {
        m_dealer->finish();
        
        delete m_dealer;
        m_dealer = NULL;
    }

    if (NULL != m_sender) {
        m_sender->finish();
        
        delete m_sender;
        m_sender = NULL;
    }

    if (NULL != m_msgOper) {
        m_msgOper->finish();
        delete m_msgOper;
        m_msgOper = NULL;
    }

    if (NULL != m_cache) {
        m_cache->freeCache();
        
		delete m_cache;
		m_cache = NULL;
	}
    
    CPushPool::finish();
}

int SockDirector::startSvc() {
    int ret = 0;

    do {
        ret = m_sender->start("sender");
        if (0 != ret) {
    		break;
    	}

        ret = m_dealer->start("dealer");
        if (0 != ret) {
    		break;
    	}

        ret = m_receiver->start("receiver");
        if (0 != ret) {
    		break;
    	}

        m_hasSvc = true;
    } while (false);

    return ret;
}

void SockDirector::waitSvc() {
    m_receiver->join();
    m_dealer->join();
    m_sender->join();       
}

void SockDirector::stopSvc() {
    if (m_hasSvc) {
        m_hasSvc = false;
        
        m_receiver->stop();
        m_receiver->resume();
        
        m_dealer->stop();
        m_dealer->resume();
        
        m_sender->stop(); 
        m_sender->resume();
    }
}

bool SockDirector::hasSvc() const { 
    return ACCESS_ONCE(m_hasSvc); 
}

void SockDirector::closeSock(int reason, SockData* pData) {
    LOG_INFO("close_sock| reason=%d| fd=%d|", reason, pData->m_fd);
    
    produce(WORK_EVENT_END, &pData->m_monlist);
}

void SockDirector::stopSock(SockData* pData) {
    produce(DIRECT_STOP_SOCK, &pData->m_monlist);
}

void SockDirector::dealTimerSec(SockData* pTimer) {
    /* first deal events */
    CPushPool::consume();

    m_receiver->addNewTask(&pTimer->m_rcvlist);
    m_sender->addNewTask(&pTimer->m_sndlist);
    m_dealer->addNewTask(&pTimer->m_deallist);
}

int SockDirector::dealWriteSock(SockData* pData) {
    int ret = 0;

    ret = m_sender->addWriteTask(&pData->m_sndlist);
    return ret;
}

int SockDirector::dealReadSock(SockData* pData) {
    int ret = 0;

    ret = m_receiver->addReadTask(&pData->m_rcvlist);
    return ret;
} 

int SockDirector::sendMsg(SockData* pData, struct msg_type* pMsg) {
    int ret = 0;
    bool bOk = true;
    int fd = pData->m_fd; 
    int size = pMsg->m_size;

    if (DEF_MSG_HEADER_SIZE <= size) {
        if (isSockValid(pData)) {
            bOk = m_msgOper->pushMsg(fd, &pData->m_sndMsgQue, pMsg);
            if (bOk) {
                ret = 0;

                m_sender->calcMsgIn(ret, size);
                m_sender->addNewTask(&pData->m_sndlist);
            } else {
                LOG_WARN("sendMsg| fd=%d| state=%d|"
                    " len=%d| msg=send error|",
                    fd, pData->m_state, size);

                ret = -1;
            }
        } else {
            LOG_DEBUG("sendMsg| fd=%d| state=%d| len=%d|"
                " msg=socket state is invalid|", 
                fd, pData->m_state, size);
            
            ret = -2;        
            MsgQueOper::freeMsg(pMsg);
        }
    } else {
        LOG_WARN("sendMsg| fd=%d| state=%d| len=%d|"
            " msg=invalid msg len([%d:%d])|", 
            fd, pData->m_state, size, 
            DEF_MSG_HEADER_SIZE, MAX_PKG_MSG_SIZE);
        
        ret = -3;
        MsgQueOper::freeMsg(pMsg);
    }

    return ret;
}

int SockDirector::sendData(SockData* pData, char* data, int len) {
    int ret = 0;
    int fd = -1;
    bool bOk = true;
    
    fd = pData->m_fd; 
    if (DEF_MSG_HEADER_SIZE <= len) {
        if (isSockValid(pData)) {
            bOk = m_msgOper->pushData(fd, &pData->m_sndMsgQue, data, len);
            if (bOk) {
                ret = 0;

                m_sender->calcMsgIn(ret, len);
                m_sender->addNewTask(&pData->m_sndlist);
            } else {
                LOG_WARN("sendData| fd=%d| state=%d|"
                    " data=%p| len=%d| msg=send error|",
                    fd, pData->m_state, data, len);
                
                ret = -1;
            }
        } else {
            LOG_DEBUG("sendData| fd=%d| state=%d| data=%p| len=%d|"
                " msg=socket state is invalid|", 
                fd, pData->m_state, data, len);
            
            ret = -2;
        }
    } else {
        LOG_WARN("sendData| fd=%d| state=%d| data=%p| len=%d|"
            " msg=invalid msg len([%d:%d])|", 
            fd, pData->m_state, data, 
            len, DEF_MSG_HEADER_SIZE, MAX_PKG_MSG_SIZE);
        
        ret = -3;
    }

    return ret;
}

int SockDirector::dispatchMsg(SockData* pData, struct msg_type* pMsg) {
    int ret = 0;
    bool bOk = true;
    int fd = pData->m_fd; 
    int n = pData->m_fd;
    int size = pMsg->m_size;
    
    bOk = m_msgOper->pushMsg(n, &pData->m_rcvMsgQue, pMsg);
    if (bOk) { 
        ret = 0;
        m_dealer->calcMsgIn(ret, size); 
        m_dealer->addNewTask(&pData->m_deallist);
    } else {
        LOG_WARN("dispatchMsg| fd=%d| state=%d|"
            " len=%d| msg=pushmsg error|",
            fd, pData->m_state, size); 

        ret = -1;
    }
    
    return ret;
}

unsigned int SockDirector::procTask(unsigned int state, 
    struct TaskElement* task) {
    unsigned int ev = WORK_EVENT_OUT;
    SockData* pData = NULL;

    pData = containof(task, SockData, m_monlist);

    if (isEventSet(WORK_EVENT_END, state)) {
        if (SOCK_STATE_ESTABLISHED == pData->m_state
            || SOCK_STATE_LOGIN == pData->m_state) {
            pData->m_state = SOCK_STATE_CLOSING;
            m_center->delEvent(pData->m_fd, SOCK_READ, pData);
            m_center->delEvent(pData->m_fd, SOCK_WRITE, pData);

            m_sockConf->onClosed(pData);
            
            ev = procStopTask(pData);
        } else if (SOCK_STATE_CONNECTING == pData->m_state) {
            pData->m_state = SOCK_STATE_CLOSING;
            m_center->delEvent(pData->m_fd, SOCK_WRITE, pData);
            
            m_sockConf->onFailConnect(pData);
            ev = procStopTask(pData);
        }
    } 

    if (isEventSet(DIRECT_STOP_SOCK, state)) {
        ev = procStopTask(pData);
    } 
    
    return ev; 
}

unsigned int SockDirector::procStopTask(SockData* pData) {  
    ++pData->m_state;
    
    switch (pData->m_state) {
    case SOCK_STATE_CLOSING_SEND:
        LOG_DEBUG("stop_send_sock| fd=%d| state=%d| msg=notify send end|", 
            pData->m_fd, pData->m_state);
        
        m_sender->addEndTask(&pData->m_sndlist);
        m_sender->addWriteTask(&pData->m_sndlist);
        break;

    case SOCK_STATE_CLOSING_RECV:   
        LOG_DEBUG("stop_receive_sock| fd=%d| state=%d| msg=notify receive end|", 
            pData->m_fd, pData->m_state);
        
        m_receiver->addEndTask(&pData->m_rcvlist);
        m_receiver->addReadTask(&pData->m_rcvlist);
        break;

    case SOCK_STATE_CLOSING_DEAL:
        LOG_DEBUG("stop_deal_sock| fd=%d| state=%d| msg=notify deal end|", 
            pData->m_fd, pData->m_state);
        
        m_dealer->addEndTask(&pData->m_deallist);
        break;

    case SOCK_STATE_CLOSED:
    default:
        LOG_INFO("proc_stop_sock| fd=%d| state=%d| msg=close socket ok|", 
            pData->m_fd, pData->m_state);
        
        if (0 <= pData->m_fd) { 
            releaseSock(pData);
        }

        return WORK_EVENT_END;
    } 
    
	return DIRECT_STOP_SOCK;
}

int SockDirector::initSock(SockData* pData, bool isSrv) {
    int ret = 0;
    
    clean(&pData->m_rcvMsgQue);
    clean(&pData->m_sndMsgQue);

    INIT_TASK_ELE(&pData->m_rcvlist);
    INIT_TASK_ELE(&pData->m_sndlist);
    INIT_TASK_ELE(&pData->m_deallist);
    INIT_TASK_ELE(&pData->m_monlist);
    
    TimerList::initTimerParam(&pData->m_rcv_timer);
    TimerList::initTimerParam(&pData->m_snd_timer);
    TimerList::initTimerParam(&pData->m_deal_timer);

    pData->m_isSrv = isSrv;
    ret = m_msgOper->initQue(&pData->m_sndMsgQue);
    if (0 == ret) {
        ret = m_msgOper->initQue(&pData->m_rcvMsgQue);
        if (0 == ret) { 
            return 0;
        } else {
            m_msgOper->finishQue(&pData->m_sndMsgQue);
        }
    } 

    return ret;
}

void SockDirector::finishSock(SockData* pData) {
    m_msgOper->finishQue(&pData->m_sndMsgQue);
    m_msgOper->finishQue(&pData->m_rcvMsgQue);
}

SockData* SockDirector::allocSock(int fd, bool isSrv) {
    int ret = 0;
    SockData* pData = NULL;

    pData = m_cache->allocData(fd, POLL_TYPE_SOCKET, NULL); 
    if (NULL != pData) {
        ret = initSock(pData, isSrv);
        if (0 == ret) {
            return pData;
        } else {
            m_cache->freeData(pData);
            return NULL;
        }
    } else { 
        return NULL;
    }
}

int SockDirector::prepareSockSync(SockData* pData, bool isSrv) {
    int ret = 0;

    if (isSrv) {
        ret = m_sockConf->onAccepted(pData);
    } else {
        ret = m_sockConf->onConnected(pData);
    }

    if (0 == ret) {        
        m_sender->startSendWork(pData); 
        
        m_dealer->startDealWork(pData);
        m_receiver->startReceiveWork(pData);
    }

    return ret;
}

int SockDirector::prepareSockAsync(SockData* pData) {
    int ret = 0;
    
    ret = m_sender->startConnWork(pData); 
    return ret;
}

SockData* SockDirector::allocData(int fd, int cmd) {
    SockData* pData = NULL;
    
    pData = m_cache->allocData(fd, cmd, NULL);     
    return pData;
}

void SockDirector::releaseData(SockData* pData) {
    int fd = 0;
    
    if (0 <= pData->m_fd) {
        fd = pData->m_fd;
        
        m_cache->freeData(pData);
        SockTool::closeFd(fd);
    }
}

void SockDirector::releaseSock(SockData* pData) {
    int fd = 0;

    if (0 <= pData->m_fd) {
        fd = pData->m_fd;

        finishSock(pData);
        m_cache->freeData(pData);
        SockTool::closeFd(fd);
    }
}

int SockDirector::asyncConnOk(SockData* pData) { 
    int ret = 0;
    
    ret = m_sockConf->onConnected(pData);
    if (0 == ret) {
        m_dealer->startDealWork(pData);
        m_receiver->startReceiveWork(pData);
    } 

    return ret;
}

int SockDirector::addWrEvent(SockData* pData) {
    m_center->addEvent(pData->m_fd, SOCK_WRITE, pData);
    
    return 0;
}

int SockDirector::addRdEvent(SockData* pData) {
    m_center->addEvent(pData->m_fd, SOCK_READ, pData);
    
    return 0;
}

SockData* SockDirector::getSock(int fd) {
    return m_cache->getData(fd);
}

int SockDirector::process(SockData* pData, struct msg_type* pMsg) {
    int ret = 0;

    ret = m_sockConf->process(pData, pMsg); 
    return ret; 
}

int SockDirector::startEstabSock(int fd, bool isSrv) {
    int ret = 0;
    SockData* pData = NULL;

    pData = allocSock(fd, isSrv);
    if (NULL != pData) {
        ret = prepareSockSync(pData, isSrv);
        if (0 == ret) { 
            return 0;
        } else {
            releaseSock(pData);
            return ret;
        } 
    } else {
        SockTool::closeFd(fd);
        return SOCK_ADD_ALLOC_ERR;
    }
}

 int SockDirector::startConnSock(int fd) {
    int ret = 0;
    SockData* pData = NULL;

    pData = allocSock(fd, false);
    if (NULL != pData) {
        ret = prepareSockAsync(pData);
        if (0 == ret) { 
            return 0;
        } else {
            releaseSock(pData);
            return ret;
        } 
    } else {
        SockTool::closeFd(fd);
        return SOCK_ADD_ALLOC_ERR;
    }
 }

