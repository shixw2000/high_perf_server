#ifndef __SOCKMONITOR_H__
#define __SOCKMONITOR_H__
#include"cthread.h"


struct msg_type;
class SockDirector;
class SockDeal;
class SockConf;

struct _SockData;
typedef struct _SockData SockData; 

class SockMonitor : public CThread {     
public:
    SockMonitor();
    virtual ~SockMonitor();

    int init();
    void finish();

    int addSvr(const char name[], const char port[]);
	int addCliAsync(const char name[], const char port[]);
    int addCliSync(const char name[], const char port[]);
    
    int sendMsg(SockData* pData, struct msg_type* pMsg);
    int sendData(SockData* pData, char* psz, int len);

    struct msg_type* allocMsg(int len);
    bool freeMsg(struct msg_type* pMsg);

    bool isSrv(const SockData* pData) const;
    
    int testSock(int opt, int cnt, int len);
    
private:
    int prepareRun();
    void postRun();

    virtual int run();

    int connSync(const char name[], const char port[]);
    
    int test_1(int cnt, int len);
    int test_2(int cnt, int len);
    int test_3(int cnt);
    int test_4(int cnt, int len);

private:
    SockDirector* m_director;
    SockDeal* m_deal;
    SockConf* m_sockConf;
};

#endif

