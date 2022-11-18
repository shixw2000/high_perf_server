#ifndef __SENDER_H__
#define __SENDER_H__
#include"interfobj.h"
#include"taskpool.h"
#include"datatype.h"


class TickTimer;
class Director;

class Sender : public TaskPool, public I_TimerDealer {
public:
    explicit Sender(Director* director);
    virtual ~Sender();

    Int32 init();
    Void finish(); 

    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task); 

    virtual Void doTimeout(struct TimerEle* ele);
    Void doTick(Uint32 cnt);
    void addTimer(struct TimerEle* ele, Int32 type, Uint32 interval);

protected: 
    virtual int setup();
    virtual void teardown();


private:
    Uint32 procNodeQue(NodeBase* base);
    
private:
    TickTimer* m_timer;
    Director* m_director;
};

#endif

