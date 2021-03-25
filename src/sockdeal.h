#ifndef __SOCKDEAL_H__
#define __SOCKDEAL_H__
#include"sharedheader.h"
#include"sockpool.h"


class SockDirector;
struct _SockData;
typedef struct _SockData SockData;

class SockDeal : public SockPool {
    typedef int (SockDeal::*PFUNC)(SockData* pData); 
    
public:
    explicit SockDeal(int capacity);
    virtual ~SockDeal();
    
    virtual int init(SockDirector* director);
    virtual void finish(); 

    int monitor();

    int addSvr(const char name[], const char port[]);
	int addCliAsync(const char name[], const char port[]);
    int addCliSync(const char name[], const char port[]);

private: 
    virtual void dealEvent(int type, void* data);

    int addData(int fd, int cmd);
    int connSync(const char name[], const char port[]);
    
    int dealWrEpoll(SockData* pWrEpoll);
    int dealListenSock(SockData* pData);
    int dealTimerFd(SockData* pData);
	int dealEventFd(SockData* pData);
    int dealException(SockData* pData);
    int dealRdSock(SockData* pData); 
	int dealWrSock(SockData* pData);
    int dealTimerPerSec(unsigned long long tick, SockData* pData);

protected:
    static const PFUNC m_funcs[POLL_TYPE_NONE][SOCK_ALL];
    SockDirector* m_director;
    int m_listenfd;
    int m_eventfd;
    int m_timerfd;
};

#endif

