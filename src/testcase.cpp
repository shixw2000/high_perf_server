#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include"testcase.h"
#include"ticktimer.h"
#include"sockepoll.h"
#include"sockutil.h"
#include"director.h"
#include"msgcenter.h"
#include"server.h"


class TestTimer : public I_TimerDealer {
public:
    virtual Void doTimeout(struct TimerEle* ele) {
        LOG_INFO("do_timeout| tick=%u| time=%u|",
            ele->m_base->monoTick(),
            ele->m_base->now());

        updateTimer(ele);
    }
};

void test_timer() {
    TickTimer* timer = NULL;
    TestTimer* dealer = NULL;
    long timeout = 2000;
    int max = 200000;
    struct TimerEle ele;

    timer = new TickTimer;
    dealer = new TestTimer;
    timer->setDealer(dealer);

    INIT_TIMER_ELE(&ele);

    ele.m_type = 0;
    ele.m_interval = timeout;
    timer->addTimer(&ele);

    for (int i=0; i<max; ++i) {
        timer->tick(1);
    }

    timer->delTimer(&ele);
    timer->stop();
    delete timer;
    delete dealer;
}

static Server* g_server = NULL;

Bool g_reply = TRUE;

static Void sigHandler(Int32 sig) {
    LOG_ERROR("*******sig=%d| msg=stop server|", sig);

    if (NULL != g_server) {
        g_server->stop();
    }
}

void test_epoll_cli(const Char* ip, int port, Bool bServ, Int32 conn_cnt) {
    int ret = 0; 
    
    I_NEW(Server, g_server); 
    ret = g_server->init();
    if (0 != ret) {
        return;
    }

    if (bServ) {
        ret = g_server->startServer(ip, port);
    } else {
        for (int i=0; i<conn_cnt; ++i) {
            ret = g_server->addService(ip, port, bServ);
        }
    }
    
    if (0 == ret) { 
        g_server->start(); 
        g_server->join();
    }
    
    g_server->finish();

    I_FREE(g_server);

    sleepSec(3);
    return;
}

int test_main(int argc, char* argv[]) {
    int opt = 0;
    Int32 conn_cnt = 1;

    armSig(SIGHUP, &sigHandler);
    armSig(SIGINT, &sigHandler);
    armSig(SIGQUIT, &sigHandler);
    armSig(SIGTERM, &sigHandler);

    opt = atoi(argv[1]);
    if (0 == opt) {
        if (5 == argc) {
            conn_cnt = atoi(argv[4]);
            test_epoll_cli(argv[2], atoi(argv[3]), FALSE, conn_cnt);
        } 
    } else if (1 == opt) {
        if (4 == argc) {
            /* server */
            test_epoll_cli(argv[2], atoi(argv[3]), TRUE, 0);
        }
    } else if (2 == opt) {
    } else if (3 == opt) {
    } else if (4 == opt) {
    } else if (5 == opt) {
    } else if (6 == opt) {
    } else if (7 == opt) {
    } else if (8 == opt) {
        test_timer();
    } else {
    }

    return 0;
}

