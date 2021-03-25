#ifndef __SENDER_H__
#define __SENDER_H__
#include<sys/uio.h>
#include"sockdata.h"
#include"worker.h"
#include"msgpool.h"


class Sender;
class MsgBaseQue;
class SockDirector;

class MsgWriter {     
public:
    MsgWriter(Sender* sender, MsgBaseQue* msgOper);
    ~MsgWriter();

    int writeMsg(SockData* pData); 
    void finishWrite(SockData* pData); 

private: 

    /* return cnt of msgs */
    int peek_deque(int maxsize, int offset,
        struct hlist* head, struct hlist* tail);

    /* return true: has more */
    bool del_deque(SockData* pData, int wlen,
        struct hlist* head, struct hlist* tail);

private:
    const bool m_needFlowctl;
    Sender* m_sender;
    MsgBaseQue* m_msgOper;
    int m_total;
    struct iovec m_vec[DEF_SEND_VEC_SIZE];
};


class Sender : public Worker, public CalcListener {
    /* return 0: continue, >0: stop, <0: error */
    typedef int (Sender::*PDealer)(SockData* pData); 

    enum MSG_STEP_STATE {
        MSG_STEP_PREPARE_WR = 0,
        MSG_STEP_CONNECT,
        MSG_STEP_ESTABLISH,
        MSG_STEP_SEND_MSG,
        MSG_STEP_END,

        MSG_STEP_BUTT
    };
    
public:
    Sender();
    virtual ~Sender();

    int init(SockDirector* director, MsgBaseQue* msgOper);
    void finish();

    int startSendWork(SockData* pData);
    int startConnWork(SockData* pData);

    int addWriteTask(struct TaskElement* task);

    void calcMsgIn(int reason, int size);
    void calcMsg(int reason, int size);

protected:      
    virtual SockData* castTask2Sock(struct TaskElement* task);

    virtual SockData* castTimer2Sock(struct TimeTask* task);
    
    virtual unsigned int procSock(SockData* pData);

    virtual void procSockEnd(SockData* pData);

    virtual void dealTimeout(SockData* pData);
    
    int prepareWr(SockData* pData);
    int chkConn(SockData* pData);
    int establish(SockData* pData);
    int sendQueue(SockData* pData);
    int endTask(SockData* pData);

    void failMsg(int reason, SockData* pData);

    virtual void postRun();
    
private:
    static const PDealer m_funcs[MSG_STEP_BUTT];
    MsgWriter* m_writer;
    MsgBaseQue* m_msgOper;
    SockDirector* m_director;
    unsigned long long m_leftsize; 
    unsigned long long m_max;
    unsigned long long m_len;
    unsigned long long m_total;
    unsigned long long m_success;
    unsigned long long m_fail;
};

#endif

