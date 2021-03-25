#include"sharedheader.h"
#include"socktool.h"
#include"transhandler.h"


TransHandler::TransHandler() {
}

TransHandler::~TransHandler() {
}

int TransHandler::init(SockMonitor* pool, 
    const char name[], const char port[],
    int cnt, int len) {
    m_pool = pool;
    m_cnt = cnt;
    m_len = len; 

    strcpy(m_name, name);
    strcpy(m_port, port);
    return 0;
}

int TransHandler::run() {
    int ret = 0;
    int sec = 1;
    int opt = 1;

#if 0
    LOG_INFO(">>>>enter your option[1,2,3]:");
    scanf("%d", &opt);
    LOG_INFO("<<<<your option is: [%d]", opt);
#endif

    if (3 == opt) {
        g_clifd = m_pool->addCliSync(m_name, m_port);
    }

    sleepSec(3);

    /* send msgs per second*/
    if (!needReply()) {
        while (0 == ret) {        
            sleepSec(sec);

            ret = m_pool->testSock(opt, m_cnt, m_len);
        }
    } else {
        /* just send msgs once, then replied */
        ret = m_pool->testSock(opt, m_cnt, m_len);
        SockTool::pause();
    }

    m_pool->stop();
    
    if (0 < g_clifd) {
        SockTool::closeFd(g_clifd);
    }

    return 0;
}

