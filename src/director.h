#ifndef __DIRECTOR_H__
#define __DIRECTOR_H__
#include"pushpool.h"


class Receiver;
class Sender;
class MsgDealer;
class MsgBaseQue;
class SockDeal;
class SockCache;
class SockConf;
struct msg_type;

struct _SockData;
typedef struct _SockData SockData; 

class SockDirector : public CPushPool { 
public:
    SockDirector();
    ~SockDirector();

    int init();
    
    void finish(); 

    void setCenter(SockDeal* center);
    void setConf(SockConf* sockConf);

    int startSvc();

    void waitSvc();

    void stopSvc();

    bool hasSvc() const;
    
    void dealTimerSec(SockData* pTimer);
    int dealWriteSock(SockData* pData);   
    int dealReadSock(SockData* pData);
        
    int sendMsg(SockData* pData, struct msg_type* pMsg);
    int sendData(SockData* pData, char* psz, int len);
    int dispatchMsg(SockData* pData, struct msg_type* pMsg);    
  
    void stopSock(SockData* pData);
    void closeSock(int reason, SockData* pData);

    SockData* allocData(int fd, int cmd);
    void releaseData(SockData* pData);
    
     int startEstabSock(int fd, bool isSrv);
     int startConnSock(int fd);

     int addWrEvent(SockData* pData);
     int addRdEvent(SockData* pData);

     int asyncConnOk(SockData* pData);
    
    SockData* getSock(int fd); 

    int process(SockData* pData, struct msg_type* pMsg);

protected: 
    virtual unsigned int procTask(unsigned int state, 
        struct TaskElement* task);
    
    unsigned int procStopTask(SockData* pData); 

    int initSock(SockData* pData, bool isSrv);

    void finishSock(SockData* pData);

    SockData* allocSock(int fd, bool isSrv); 
    void releaseSock(SockData* pData);    

    int prepareSockSync(SockData* pData, bool isSrv);
    int prepareSockAsync(SockData* pData);

private:
    Receiver* m_receiver;
    Sender* m_sender;
    MsgDealer* m_dealer;
    MsgBaseQue* m_msgOper;
    SockCache* m_cache;
    SockDeal* m_center;
    SockConf* m_sockConf;
    bool m_hasSvc;
};


#endif

