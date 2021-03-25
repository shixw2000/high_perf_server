#ifndef __RECEIVER_H__
#define __RECEIVER_H__
#include<sys/uio.h>
#include"sockdata.h"
#include"worker.h"


class SockDirector;
class Receiver;

class MsgReader { 
    typedef int (MsgReader::*PDealer)(SockData* pData);
    
public:
    explicit MsgReader(Receiver* receiver);
    ~MsgReader(); 
    
    int readHeader(SockData* pData); 
    int readBody(SockData* pData);
    void finishRead(SockData* pData); 
    
private: 
    int readMsg(int cnt, SockData* pData, PDealer cb);
    int parseHeader(SockData* pData);
    int parseBody(SockData* pData);
    int parseNextBody(SockData* pData);
    
    int fillMsg(SockData* pData);
    int prepareHeader(SockData* pData);
    int prepareBody(SockData* pData);
    
private:
    static const int DEF_RECV_VEC_SIZE_TWO = 2;
    struct iovec m_vec[DEF_RECV_VEC_SIZE_TWO];
    char m_buffer[DEF_RECV_MSG_SIZE_ONCE]; 
    const bool m_needFlowctl;
    Receiver* m_receiver;
    int m_total;
};


class Receiver : public Worker {
    /* return 0: continue, >0: stop, <0: error */
    typedef int (Receiver::*PDealer)(SockData* pData);    

    enum MSG_STEP_STATE {
        MSG_STEP_PREPARE = 0,
        MSG_STEP_READ_HEADER,
        MSG_STEP_READ_BODY,
        MSG_STEP_END,

        MSG_STEP_BUTT
    };

    friend class MsgReader;
    
public:
    Receiver();
    virtual ~Receiver();
    
    int init(SockDirector* director);
    void finish(); 

    int startReceiveWork(SockData* pData);

    int chkHeader(const struct MsgHeader* pHd);     

    int dispatchMsg(SockData* pData, struct msg_type* pMsg);

    int addReadTask(struct TaskElement* task);
    
protected: 
    void calcMsg(int reason, int size);
    
    virtual SockData* castTask2Sock(struct TaskElement* task);

    virtual SockData* castTimer2Sock(struct TimeTask* task);
    
    virtual unsigned int procSock(SockData* pData);

    virtual void procSockEnd(SockData* pData);

    virtual void dealTimeout(SockData* pData);

    void failMsg(int reason, SockData* pData);

    int prepare(SockData* pData);
    int readHeader(SockData* pData);
    int readBody(SockData* pData);
    int endTask(SockData* pData);

    void postRun();
    
private:
    static const PDealer m_funcs[MSG_STEP_BUTT];
    SockDirector* m_director;
    MsgReader* m_reader;
    unsigned long long m_len;
    unsigned long long m_total; 
    unsigned long long m_success;
    unsigned long long m_fail;
    unsigned long long m_max;
};

#endif

