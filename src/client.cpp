#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include"sockmonitor.h"
#include"cthread.h"
#include"sharedheader.h"
#include"transhandler.h"


int* g_fds = NULL;
int g_clifd = -1;
int g_cnt = 0;
char g_sndmsg[DEF_RECV_MSG_SIZE_ONCE] = {0};
char g_rcvmsg[DEF_RECV_MSG_SIZE_ONCE] = {0};


static int initBuffer(int len) {
    struct MsgHeader* pHd = NULL;
    char* psz = NULL;

    psz = g_sndmsg;
    pHd = (struct MsgHeader*)psz;
    
    for (int i=0; i<DEF_RECV_MSG_SIZE_ONCE; ++i) {
        psz[i] = (char)i;
    } 
       
    pHd->m_cmd = 0x4444;
    pHd->m_ver = DEF_MSG_VERSION;
    pHd->m_crc = DEF_MSG_CRC;
    pHd->m_size = len;
    pHd->m_csum = mkcrc(pHd+1, len - DEF_MSG_HEADER_SIZE);
    
    return 0;
}

void daemon(int cnt) {
    int status = 0;
    pid_t* pids = NULL;
    pid_t me = 0;
    
    pids = new pid_t[cnt];

    for (int i=0; i<cnt; ++i) {
        pids[i] = CThread::forkSelf();
        me = pids[i];
        
        if (0 < me) {
            continue;
        } else {
            g_me = getpid();
            return;
        }
    }

    /* in parent process */
    do {
        me = wait(&status);
        if (0 < me) {
            LOG_INFO("wait| status=%d| process=0x%x| msg=child exit now|",
                status, me);
        } else if (-1 == me && EINTR == errno) {
            continue;
        } else {
            LOG_WARN("wait| ret=0x%x| error=%s|", me, ERR_MSG);
            break;
        }
    } while (true);

    LOG_INFO("====pid=%ld| parent exit now|", g_me);
    exit(0);
}

int batchAddCli(char name[], char port[], 
    int cnt, int persec, SockMonitor* pSock) {
    int ret = 0;
    g_fds = new int[cnt];

    for (int i=0; i<cnt; ++i) {
        //ret = pSock->addCliSync(name, port);
        ret = pSock->addCliAsync(name, port);
        if (0 < ret) {
            g_fds[i] = ret;
            ++g_cnt;
        } else {
            g_fds[i] = -1;
            break;
        }
    }

    LOG_INFO("=======create_conn_cnt=%d| require_cnt=%d|", g_cnt, cnt);

    return g_cnt;
}

int clientMain(char name[], char port[], int conn_cnt, 
    int speed, int pkgsize) {
	int ret = 0;
	SockMonitor* pSock = NULL;
	TransHandler* ptest = NULL;   

    //daemon(cnt);

    initBuffer(pkgsize);
        
	pSock = new SockMonitor;
	ret = pSock->init();
	if (0 != ret) {
		return -1;
	}

    ptest = new TransHandler;
    ret = ptest->init(pSock, name, port, speed, pkgsize);
    if (0 != ret) {
		return -1;
	}
    
	if (0 < conn_cnt) {
        ret = batchAddCli(name, port, conn_cnt, speed, pSock);
        if (0 < ret) { 
    		ret = pSock->start("client"); 
            ret = ptest->start("test");

            ptest->join();
            pSock->join(); 
        } else {
            LOG_ERROR("add client(%s:%s) error|", name, port);
        }
	}

	pSock->finish();
	delete pSock;
    delete ptest; 

	return ret;
}

