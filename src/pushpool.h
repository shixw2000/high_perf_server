#ifndef __PUSHPOOL_H__
#define __PUSHPOOL_H__
#include"hlist.h"


class Lock;
class BitDoor;
struct TaskElement;

class CPushPool {
public:
    CPushPool();
    virtual ~CPushPool();

    virtual int init();
    virtual void finish(); 
    
    virtual bool consume(); 

    bool isEventSet(unsigned int ev, unsigned int state); 

protected:
    /* ret: 1: in, 0-not in, -1:error */
    virtual int produce(unsigned int ev, struct TaskElement* ele);
    
    int registerEvent(unsigned int ev, int type);
    
    virtual unsigned int procTask(unsigned int state, 
        struct TaskElement* ele) = 0;
    
private:
    ListPool* m_listpool;
    BitDoor* m_door;
    struct hlist* m_tail;
    struct hlist_root m_local;
    struct hlist_root m_global;
};


#endif

