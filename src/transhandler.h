#ifndef __TRANSHANDLER_H__
#define __TRANSHANDLER_H__
#include"cthread.h"
#include"sockdata.h"
#include"sockmonitor.h"


class TransHandler : public CThread {
public:
    TransHandler();
    ~TransHandler();

    int init(SockMonitor* pool, const char name[], const char port[],
        int cnt, int len);

protected:
    int run();

private:
    int m_cnt;
    int m_len;
    char m_name[128];
    char m_port[32];
    SockMonitor* m_pool;
};

#endif

