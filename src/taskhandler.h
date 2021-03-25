#ifndef __TASKHANDLER_H__
#define __TASKHANDLER_H__
#include"service.h"
#include"pushpool.h"


class TaskHandler : public EvService, public CPushPool {
public:
    TaskHandler();
    virtual ~TaskHandler();

    virtual int init();
    virtual void finish(); 

protected: 
    virtual int produce(unsigned int ev, struct TaskElement* task);
    
    virtual bool doService(); 
};

#endif

