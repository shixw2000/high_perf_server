#ifndef __WORKER_H__
#define __WORKER_H__
#include"taskhandler.h"
#include"task.h"


class TimerList;

class Worker : public TaskHandler, public TimerCallback {    
public:
    Worker();
    ~Worker();

    virtual int init();
    virtual void finish(); 

    int addEndTask(struct TaskElement* task);
    int addNewTask(struct TaskElement* task);
    int addFlowCtlTask(struct TaskElement* task);

protected: 
    void setTimeout(struct TimeTask* item, int type, int timeout);
    void deleteTimer(struct TimeTask* item);
    void upadteTimer(struct TimeTask* item);
    
    virtual unsigned int procTask(unsigned int state, 
        struct TaskElement* task);

    virtual SockData* castTask2Sock(struct TaskElement* task) = 0;

    virtual SockData* castTimer2Sock(struct TimeTask* task) = 0;
    
    virtual unsigned int procSock(SockData* pData) = 0;

    virtual void procSockEnd(SockData* pData) = 0;

    virtual void dealTimeout(SockData* pData) = 0;
        
private: 
    void procTimeout(struct TimeTask* item);
        
    void procTimer();
    
private:
    TimerList* m_timer;
};


#endif

