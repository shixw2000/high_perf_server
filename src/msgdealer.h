#ifndef __MSGDEALER_H__
#define __MSGDEALER_H__
#include"sockdata.h"
#include"worker.h"
#include"msgpool.h"


class SockDirector;
class MsgBaseQue;

class MsgDealer : public Worker, public CalcListener {
    typedef int (MsgDealer::*PDealer)(SockData* pData);
    
    enum MSG_STEP_STATE {
        MSG_STEP_START = 0,
        MSG_STEP_PROC,
        MSG_STEP_END,

        MSG_STEP_BUTT
    };
    
public:
    MsgDealer();
    ~MsgDealer();
    
    int init(SockDirector* director, MsgBaseQue* msgOper);
    void finish();

    int startDealWork(SockData* pData);

    void calcMsgIn(int reason, int size);
    
    void calcMsg(int reason, int size);

protected:    
    virtual SockData* castTask2Sock(struct TaskElement* task);

    virtual SockData* castTimer2Sock(struct TimeTask* task);
    
    virtual unsigned int procSock(SockData* pData);

    virtual void procSockEnd(SockData* pData);

    virtual void dealTimeout(SockData* pData);

    void postRun();

private:
    int startTask(SockData* pData);
    int procTask(SockData* pData);
    int endTask(SockData* pData);
    void failMsg(int reason, SockData* pData);
    
private:
    static const PDealer m_funcs[MSG_STEP_BUTT];
    SockDirector* m_director;
    MsgBaseQue* m_msgOper;
    unsigned long long m_len;
    unsigned long long m_leftsize;
    unsigned long long m_max;
    unsigned long long m_total; 
    unsigned long long m_success;
    unsigned long long m_fail;
};

#endif

