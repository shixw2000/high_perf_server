#ifndef __DEALER_H__
#define __DEALER_H__
#include"interfobj.h"
#include"taskpool.h"
#include"datatype.h"


class TickTimer;
class Director;

class Dealer : public TaskPool, public I_TimerDealer {
public:
    Dealer(Director* director);
    virtual ~Dealer();

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
    Void procNodeQue(NodeBase* base);
    
private:
    TickTimer* m_timer;
    Director* m_director;
};


#endif

