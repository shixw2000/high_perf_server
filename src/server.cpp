#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<unistd.h> 
#include"sockmonitor.h"
#include"cthread.h"
#include"sharedheader.h"
#include"sockdata.h"


extern int clientMain(char name[], char port[], int cnt, 
    int persec, int pkgsize);
int serverMain(char name[], char port[]);

long g_me = 0;

void usage(const char prog[]) {
    LOG_ERROR(
        "usage: server or client|\n"
        " server: %s 1 <local_ip> <local_port> [max_print_cnt] [is_reply]\n"
        " client: %s 0 <srv_ip> <srv_port> <conn_cnt>"
        " <speed> [pkg_size] [max_print_cnt] [is_reply]|",
        prog, prog);
}

int main(int argc, char* argv[]) {
	int ret = 0;
    int conn_cnt = 0;
    int speed = 0;
    bool isReply = false;
    int maxprnt = MAX_PRINT_MSG_CNT;
    int pkgsize = DEF_TEST_MSG_SIZE;
	int isSrv = 0;
	char name[128+1] = {0};
	char port[32+1] = {0};
	 
	if ( 4 > argc ) {
		usage(argv[0]);
		return -1;
	}

    armTerm();
    armUsr1();
    
	isSrv = atoi(argv[1]);
	strncpy(name, argv[2], 128);
	strncpy(port, argv[3], 32);
    
    g_me = getpid();
	if (isSrv) {
        if (5 <= argc) {
            maxprnt = atoi(argv[4]);
            if (0 >= maxprnt) {
                maxprnt = MAX_PRINT_MSG_CNT;
            }
        }

        if (6 <= argc) {
            isReply = atoi(argv[5]);
        } else {
            /* server is replied for default */
            isReply = true;
        }
        
        setPrntMax(maxprnt);
        setReply(isReply);
        
        LOG_INFO("=========>>"
            "server_config| ip=%s| port=%s| max_print_cnt=%d| is_reply=%s|",
            name, port, maxprnt, isReply?"yes":"no");
        
		ret = serverMain(name, port);
	} else {
	    if ( 6 > argc ) {
    		usage(argv[0]);
    		return -1;
    	}
        
	    conn_cnt = atoi(argv[4]);
        speed = atoi(argv[5]);
        
        if (0 >= conn_cnt) {
            conn_cnt = 1;
        }

        if (0 >= speed) {
            speed = 1;
        }

        if (7 <= argc) {
            pkgsize = atoi(argv[6]);
            if (0 > pkgsize) {
                pkgsize = DEF_TEST_MSG_SIZE;
            }
        }

        if (8 <= argc) {
            maxprnt = atoi(argv[7]);
            if (0 >= maxprnt) {
                maxprnt = MAX_PRINT_MSG_CNT;
            }
        }

        if (9 <= argc) {
            isReply = atoi(argv[8]);
        } else {
            /* client is not replied for default */
            isReply = false;
        }
        
        setPrntMax(maxprnt);
        setReply(isReply);

        LOG_INFO("=========>>"
            "client_config| ip=%s| port=%s| conn_cnt=%d| speed=%d|"
            " pkg_size=%d| max_print_cnt=%d| is_reply=%s|",
            name, port, conn_cnt, speed, pkgsize, maxprnt,
            isReply ? "yes": "no");
        
		ret = clientMain(name, port, conn_cnt, speed, pkgsize);
	}
	
	return ret;
}

int serverMain(char name[], char port[]) {
	int ret = 0;
	SockMonitor* pSock = NULL;

	pSock = new SockMonitor;
	ret = pSock->init();
	if (0 != ret) {
		return -1;
	}
    
	ret = pSock->addSvr(name, port); 
	if (0 <= ret) {
		ret = pSock->start("monitor"); 
        pSock->join();
	}

	pSock->finish();
	delete pSock;
	
	return ret;
}


