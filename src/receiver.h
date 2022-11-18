#ifndef __RECEIVER_H__
#define __RECEIVER_H__
#include"interfobj.h"
#include"taskpool.h"
#include"datatype.h"


class TickTimer;
class Director;

class Receiver : public TaskPool, public I_TimerDealer {
public:
    explicit Receiver(Director* director);
    virtual ~Receiver();

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
    Uint32 readNode(struct NodeBase* base);
    
private:
    TickTimer* m_timer;
    Director* m_director;
};

#endif

