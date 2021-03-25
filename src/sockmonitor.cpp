#include"sharedheader.h"
#include"sockdata.h"
#include"director.h"
#include"socktool.h"
#include"sockdeal.h"
#include"sockconfdef.h"
#include"msgpool.h"
#include"sockmonitor.h"


SockMonitor::SockMonitor() {
    m_director = NULL;
    m_deal = NULL;
    m_sockConf = NULL;
}

SockMonitor::~SockMonitor() {
}

int SockMonitor::init() {
    int ret = 0;
    
    do { 
        m_sockConf = new SockConfDef(this);
        if (NULL == m_sockConf) {
            ret = -1;
            break;
        }
        
        m_director = new SockDirector;
        if (NULL == m_director) {
            ret = -1;
            break;
        } 

        ret = m_director->init();
        if (0 != ret) {
    		break;
    	}

        m_deal = new SockDeal(MAX_EPOLL_POOL_SIZE);
        if (NULL == m_deal) {
            ret = -1;
            break;
        } 

        ret = m_deal->init(m_director);
        if (0 != ret) {
    		break;
    	}

        m_director->setConf(m_sockConf);
        m_director->setCenter(m_deal);
        
        ret = 0;
    } while (false);

    return ret;
}

void SockMonitor::finish() {
    if (NULL != m_director) {
        m_director->finish();
        
        delete m_director;
        m_director = NULL;
    }
    
    if (NULL != m_deal) {
        m_deal->finish();
        
        delete m_deal;
        m_deal = NULL;
    } 

    if (NULL != m_sockConf) {        
        delete m_sockConf;
        m_sockConf = NULL;
    }
}

int SockMonitor::addSvr(const char name[], const char port[]) {
    int fd = 0;

    fd = m_deal->addSvr(name, port);
    return fd;
}

int SockMonitor::addCliAsync(const char name[], const char port[]) {
    int fd = 0;

    fd = m_deal->addCliAsync(name, port);
    return fd;
}

int SockMonitor::addCliSync(const char name[], const char port[]) {
    int fd = 0;

    fd = m_deal->addCliSync(name, port);
    return fd;
}


int SockMonitor::sendMsg(SockData* pData, struct msg_type* pMsg) {
    int ret = 0;

    if (NULL != pData) {
        ret = m_director->sendMsg(pData, pMsg);
    } else {
        ret = -1;
    }
    
    return ret;
}

int SockMonitor::sendData(SockData* pData, char* data, int len) {
    int ret = 0;

    if (NULL != pData) {
        ret = m_director->sendData(pData, data, len);
    } else {
        ret = -1;
    }
    
    return ret;
}

struct msg_type* SockMonitor::allocMsg(int len) {
    struct msg_type* pMsg = NULL;

    pMsg = MsgQueOper::allocMsg(len);
    return pMsg;
}

bool SockMonitor::freeMsg(struct msg_type* pMsg) {
    bool bOk = true;

    bOk = MsgQueOper::freeMsg(pMsg);
    return bOk;
}

bool SockMonitor::isSrv(const SockData* pData) const {
    return pData->m_isSrv; 
}

int SockMonitor::prepareRun() {
    int ret = 0;

    ret = m_director->startSvc();
    return ret;
}

void SockMonitor::postRun() {
    m_director->stopSvc();
    m_director->waitSvc();
}

int SockMonitor::run() {
    int ret = 0;

    do {
        ret = m_deal->monitor();
    } while (0 <= ret && isRun() && m_director->hasSvc());

    return ret;
}

int SockMonitor::testSock(int opt, int cnt, int len) {
    int ret = 0;

    switch (opt) {
    case 1:
        ret = test_1(cnt, len);
        break;

    case 2:
        ret = test_2(cnt, len);
        break;

    case 3:
        ret = test_3(cnt);
        break;

    case 4:
        ret = test_4(cnt, len);
        break;

    default:
        break;
    }

    return ret;
}

int SockMonitor::test_1(int cnt, int len) {
    int ret = 0;
    int fd = 0;
    char* psz = NULL;
    struct MsgHeader* pHd = NULL;
    SockData* pData = NULL; 

    psz = g_sndmsg;
    pHd = (struct MsgHeader*)g_sndmsg;
    
    pHd->m_id = (unsigned int)getTime();
    pHd->m_size = len;
    
    for (int i=0; i<g_cnt; ++i) {
        fd = g_fds[i];
        if (0 < fd) {    
            pData = m_director->getSock(fd);

            for (int i=0; i<cnt; i++) { 
                ret = sendData(pData, psz, len);
                if (0 != ret) {
                    break;
                }
            }
        }
    }

    LOG_DEBUG("testPerf_1| len=%d| speed=%d| usrcnt=%d| id=%u| ret=%d|",
        len, cnt, g_cnt, pHd->m_id, ret);

    return ret;
}

int SockMonitor::test_2(int cnt, int len) {
    int ret = 0;
    int fd = 0;
    char* psz = NULL;
    struct MsgHeader* pHd = NULL;
    SockData* pData = NULL; 

    psz = g_sndmsg;
    pHd = (struct MsgHeader*)g_sndmsg;

    pHd->m_cmd = 0x4444;
    pHd->m_ver = DEF_MSG_VERSION;
    pHd->m_crc = DEF_MSG_CRC;
    pHd->m_size = len;
    pHd->m_id = (unsigned int)getTime();
    
    for (int i=0; i<cnt; ++i) {
        for (int i=0; i<g_cnt; ++i) {
            fd = g_fds[i];
            if (0 < fd) { 
                pData = m_director->getSock(fd);
                ret = sendData(pData, psz, len);
                if (0 != ret) {
                    break;
                }
            }
        }
    }

    LOG_DEBUG("testPerf_2| len=%d| speed=%d| usrcnt=%d| id=%u| ret=%d|",
        len, cnt, g_cnt, pHd->m_id, ret);

    return ret;
}

int SockMonitor::test_3(int cnt) {
	int ret = 0;
	int len = 0;
	int opt = 0;
    int fd = g_clifd;
    char* psz = NULL;
    char* prcv = g_rcvmsg;
    char* psnd = g_sndmsg;
    struct MsgHeader* pHd = NULL;

    if (0 >= fd) {
        return -1;
    }
    
    psz = g_sndmsg;
    pHd = (struct MsgHeader*)psz;
    pHd->m_cmd = 0x4444;
    pHd->m_ver = DEF_MSG_VERSION;
    pHd->m_crc = DEF_MSG_CRC;
    pHd->m_id = (unsigned int)getTime(); 
    
	fflush(stdin);
	fprintf(stderr, "Enter your opt[0-read, 1-write,"
        " 2-down_rd, 3-down_wr, 4-down_rdwr, 5-close, 6-nooop] and len:");
	scanf("%d%d", &opt, &len);

    pHd->m_size = len + DEF_MSG_HEADER_SIZE;
    pHd->m_csum = mkcrc(pHd+1, len);
	
	if (0 == opt) {
		ret = SockTool::recvSock(fd, prcv, len);
	} else if (1 == opt) {
		ret = SockTool::sendSock(fd, psnd, pHd->m_size);
	} else if (2 == opt) {
	    SockTool::shutdownSock(fd, SOCK_READ);
	} else if (3 == opt) {
	    SockTool::shutdownSock(fd, SOCK_WRITE);
	} else if (4 == opt) {
	    SockTool::shutdownSock(fd, SOCK_READ | SOCK_WRITE);
	} else if (5 == opt) {
	    SockTool::closeFd(fd);
        g_clifd = -1;
	} else {
	}
	
	fflush(stdout);
	
	return ret;
}

int SockMonitor::test_4(int cnt, int len) { 
    int ret = 0;
    int fd = 0;
    char* psz = NULL;
    struct MsgHeader* pHd = NULL;
    SockData* pData = NULL; 

    psz = g_sndmsg;
    pHd = (struct MsgHeader*)g_sndmsg;
    
    pHd->m_id = (unsigned int)getTime();
    pHd->m_size = len;
    
    for (int i=0; i<g_cnt; ++i) {
        fd = g_fds[i];
        if (0 < fd) {    
            pData = m_director->getSock(fd);

            for (int i=0; i<cnt; i++) { 
                ret = sendData(pData, psz, len);
                if (0 != ret) {
                    break;
                }
            }
        }
    }

    LOG_DEBUG("testPerf_4| len=%d| speed=%d| usrcnt=%d| id=%u| ret=%d|",
        len, cnt, g_cnt, pHd->m_id, ret);

    SockTool::pause();

    return ret;
}

