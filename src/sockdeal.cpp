#include"socktool.h"
#include"director.h"
#include"sockdata.h"
#include"sockdeal.h"


static unsigned long long g_cursec = 0ULL;
static unsigned long long g_lastsec = 0ULL;
static int g_max_prnt_cnt = MAX_PRINT_MSG_CNT;
static bool g_need_reply = false;

unsigned long long getTime() {
    unsigned long long ullTm = 0ULL;
    
    ullTm =  ACCESS_ONCE(g_cursec);
    return ullTm;
}

void setPrntMax(int max) {
    g_max_prnt_cnt = max;
}

bool needPrnt(int n) {
    if (g_max_prnt_cnt > n) {
        return false;
    } else {
        return true;
    }
}

void setReply(bool isReply) {
    g_need_reply = isReply;
}

bool needReply() {
    return g_need_reply;
}

const SockDeal::PFUNC SockDeal::m_funcs[POLL_TYPE_NONE][SOCK_ALL] = {
    /* read         write           exception*/
    {&SockDeal::dealWrEpoll, NULL, NULL },
    {&SockDeal::dealRdSock, &SockDeal::dealWrSock, &SockDeal::dealException},
        
    {&SockDeal::dealListenSock, NULL, NULL},
    {&SockDeal::dealTimerFd, NULL, NULL },
    {&SockDeal::dealEventFd, NULL, NULL }, 
};

SockDeal::SockDeal(int capacity) : SockPool(capacity) {
    m_director = NULL;

    m_eventfd = m_timerfd = m_listenfd = -1;
}

SockDeal::~SockDeal() {
}

int SockDeal::init(SockDirector* director) {
    int ret = 0;

    m_director = director;

    do {
        ret = SockPool::init();
        if (0 != ret) {
            break;
        }

        ret = addData(epfdwr, POLL_TYPE_WRITE_EPOLL);
        if (0 != ret) {
            epfdwr = -1;
			break;
		}
        
        m_eventfd = SockTool::createEventFd();
    	if (0 <= m_eventfd) {
            ret = addData(m_eventfd, POLL_TYPE_EVENTER);
    		if (0 != ret) {
                m_eventfd= -1;
    			break;
    		}
    	} else {
    		break;
    	}

    	m_timerfd = SockTool::createTimerFd();
    	if (0 <= m_timerfd) {
            SockTool::setTimerVal(m_timerfd, 100);
            
    		ret = addData(m_timerfd, POLL_TYPE_TIMER);
    		if (0 != ret) {
                m_timerfd = -1;
    			break;
    		}
    	} else {
    		break;
    	}

        LOG_DEBUG("eventfd=%d| timefd=%d| msg=init msgdeal ok|",
            m_eventfd, m_timerfd); 

        return 0;
    } while (false);
    
    return -1;
}

void SockDeal::finish() {
    if (0 <= m_listenfd) {
		SockTool::closeFd(m_listenfd);
		m_listenfd = -1;
	}
    
    if (0 <= m_timerfd) {
		SockTool::closeFd(m_timerfd);
		m_timerfd = -1;
	}

	if (0 <= m_eventfd) {
		SockTool::closeFd(m_eventfd);
		m_eventfd = -1;
	}
    
    SockPool::finish(); 
}

int SockDeal::monitor() {
    int ret = 0;

    ret = chkRdEvent();
    return ret;
}

int SockDeal::addData(int fd, int cmd) {
    SockData* pData = NULL;

    pData = m_director->allocData(fd, cmd);
    if (NULL != pData) {
        addEvent(fd, SOCK_READ, pData);
        
        return 0;
    } else {
        SockTool::closeFd(fd);
        return SOCK_ADD_ALLOC_ERR;
    }
}

int SockDeal::addSvr(const char name[], const char port[]) {
	int ret = 0;
    int listenfd = -1;
    int rcvsize = 0;
    int sndsize = 0;
    
    listenfd = SockTool::createSvrInet(name, port);
	if (0 <= listenfd) { 
        ret = addData(listenfd, POLL_TYPE_LISTENER);
		if (0 == ret) {
            rcvsize = SockTool::getRcvBufferSize(listenfd);
            sndsize = SockTool::getSndBufferSize(listenfd);
    
            LOG_INFO("add_srv| listenfd=%d| name=%s| port=%s|"
                " rcv_buff_size=%d| send_buff_size=%d|"
                " msg=create server ok|", 
                listenfd, name, port, rcvsize, sndsize);
            
            return listenfd;
		} else {
		    LOG_WARN("add_srv| listenfd=%d| name=%s| port=%s| ret=%d|"
                " error=add server failed|", 
                listenfd, name, port, ret);
            
			return -1;
		}
	} else {
		LOG_WARN("add_srv| name=%s| port=%s| ret=%d"
            " error=create server failed|", 
            name, port, listenfd);

        return -1;
	}
}

int SockDeal::addCliSync(const char name[], const char port[]) {
	int ret = 0;
    int fd = -1;

	fd = connSync(name, port);
	if (0 <= fd) {
        ret = m_director->startEstabSock(fd, false);
		if (0 == ret) {
            LOG_INFO("add_cli_sync| fd=%d| name=%s| port=%s|"
                " msg=create client ok|", 
                fd, name, port);
            
            return fd;
		} else {
		    LOG_WARN("add_cli_sync| fd=%d| name=%s| port=%s| ret=%d|"
                " error=add sock failed|", 
                fd, name, port, ret);
            
			return -1;
		}
	} else {
	    LOG_WARN("add_cli_sync| name=%s| port=%s| ret=%d|"
            " error=connect failed|", 
            name, port, fd);
        
		return -1;
	}
}

int SockDeal::addCliAsync(const char name[], const char port[]) {
	int ret = 0;
    int fd = -1;

    fd = SockTool::createInetTcp();
    if (0 > fd) {
        return -1;
    }

    ret = SockTool::connSrvInet(fd, name, port);
    if (0 == ret) {
        ret = m_director->startEstabSock(fd, false);
        if (0 == ret) {
            LOG_DEBUG("add_cli_async| fd=%d| name=%s| port=%s|"
                " msg=create client ok|", 
                fd, name, port);
            
            return fd;
		} else {
		    LOG_WARN("add_cli_async| fd=%d| name=%s| port=%s| ret=%d|"
                " error=add sock failed|", 
                fd, name, port, ret);
            
			return -1;
		}
    } else if (SOCK_CONN_SRV_PROGRESS == ret) {
        ret = m_director->startConnSock(fd);
        if (0 == ret) {
            LOG_DEBUG("add_cli_async| fd=%d| name=%s| port=%s|"
                " msg=start async connect ok|", 
                fd, name, port);
            
            return fd;
        } else {
            LOG_WARN("add_cli_async| fd=%d| name=%s| port=%s| ret=%d|"
                " error=start async connect failed|", 
                fd, name, port, ret);
            
        	return -1;
        }
    } else {
        LOG_WARN("add_cli_async| fd=%d| name=%s| port=%s| ret=%d|"
            " error=connect failed|", 
            fd, name, port, ret);

        SockTool::closeFd(fd);
		return -1;
    }
}

/* return fd */
int SockDeal::connSync(const char name[], const char port[]) {
    int ret = 0;
    int fd = -1;

    fd = SockTool::createInetTcp();
    if (0 > fd) {
        return -1;
    }

    do {
        ret = SockTool::connSrvInet(fd, name, port);
        if (0 == ret) {
            /* ok */
        } else if (SOCK_CONN_SRV_PROGRESS == ret) {
    		ret = SockTool::pollSock(fd, SOCK_WRITE, 3000);
    		if (0 < ret) {
    			ret = SockTool::getConnCode(fd);
    			if (0 == ret) {
                    /* ok*/
    			} else {
    			    break;
                }
    		} else {
                break;
    		}
    	} else {
    	    break;
    	}

        LOG_DEBUG("conn_sync| fd=%d| name=%s| port=%s|"
            " msg=create client ok|", 
            fd, name, port); 
        
        return fd;
    } while (false);

    LOG_WARN("conn_sync| fd=%d| name=%s| port=%s|"
        " msg=create client failed|", 
        fd, name, port); 
    
    SockTool::closeFd(fd);
    return -1;
}

void SockDeal::dealEvent(int type, void* data) {
    SockData* pData = NULL;
    int ret = 0;
    int cmd = 0;

    pData = (SockData*)data;
    cmd = pData->m_type;
    if (NULL != (m_funcs[cmd][type])) {
        ret = (this->*(m_funcs[cmd][type]))(pData);
        LOG_VERB("dealEvent| fd=%d| cmd=%d| type=%d| ret=%d|",
            pData->m_fd, cmd, type, ret);
    } else {
        LOG_ERROR("dealEvent| fd=%d| cmd=%d| type=%d| error=null function|",
            pData->m_fd, cmd, type);
    }
    
    return;
}

int SockDeal::dealWrEpoll(SockData* pWrEpoll) {
    int ret = 0;

    ret = chkWrEvent();
    return ret;
}

int SockDeal::dealTimerFd(SockData* pData) {
	unsigned long long ullVal = 0;
	int ret = 0;

    ret = SockTool::readTimerOrEvent(pData->m_fd, ullVal);
	if (0 < ret) {
        g_cursec += ullVal;

        if (g_lastsec + 10 <= g_cursec) {
            dealTimerPerSec(ullVal, pData); 

            g_lastsec = g_cursec;
        } 
	} 
	
	return ret;
}

int SockDeal::dealEventFd(SockData* pData) {
	unsigned long long ullVal = 0;
	int ret = 0;

    ret = SockTool::readTimerOrEvent(pData->m_fd, ullVal);
	if (0 < ret) {
		LOG_DEBUG("deal_event| val=%llu|", ullVal);
	} 
	
	return ret;
}

int SockDeal::dealRdSock(SockData* pData) { 
    int ret = 0;
    
	ret = m_director->dealReadSock(pData);

    LOG_VERB("dealRdSock| fd=%d| ret=%d|", pData->m_fd, ret); 
	return ret;
}

int SockDeal::dealWrSock(SockData* pData) {
	int ret = 0;
    
	ret = m_director->dealWriteSock(pData);

    LOG_VERB("dealWrSock| fd=%d| ret=%d|", pData->m_fd, ret); 
	return ret;
}

int SockDeal::dealException(SockData* pData) {
    LOG_WARN("dealException| fd=%d|", pData->m_fd);

    m_director->closeSock(COMMON_ERR_SOCK_EXCEPTION, pData);
    
	return 0;
}

int SockDeal::dealListenSock(SockData* pListener) {
	int ret = 0;
	char name[32] = {0};
	char port[32] = {0};
    int listenfd = -1;
    int clifd = -1;

    listenfd = pListener->m_fd;
    LOG_DEBUG("listenfd=%d| msg=new socket is connecting|", listenfd);

	clifd = SockTool::acceptSock(listenfd, name, port);
	while (0 <= clifd) {
        ret = m_director->startEstabSock(clifd, true);
        if (0 == ret) { 
            LOG_DEBUG("accept_sock| clifd=%d| name=%s| port=%s|"
                " msg=accept new client|", clifd, name, port);
        } else {
            LOG_WARN("accept_sock| clifd=%d| name=%s| port=%s| ret=%d|"
                " error=add sock failed|", clifd, name, port, ret);
        }
		
		clifd = SockTool::acceptSock(listenfd, name, port);
	}

	return 0;
}

int SockDeal::dealTimerPerSec(unsigned long long tick, 
    SockData* pData) {
    static unsigned long long ullTm = 0ULL;
   
    if (10 <= ++ullTm) {
        LOG_INFO("print_time| tick=%llu| time=%llu|", tick, g_cursec);
        ullTm = 0ULL;
    } 

    m_director->dealTimerSec(pData);
	
	return 0;
}
