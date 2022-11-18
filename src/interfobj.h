#ifndef __INTERFOBJ_H__
#define __INTERFOBJ_H__
#include"sysoper.h"


class I_Service {
public:
    virtual ~I_Service() {}

    virtual int run() = 0;
};

struct Task;
class I_Worker {
public:
    virtual ~I_Worker() {} 
    
    virtual void procTaskEnd(struct Task* task) = 0;
    virtual unsigned int procTask(struct Task* task) = 0;
};

class I_Waiter {
public:
    virtual ~I_Waiter() {}

    virtual int init() = 0;
    virtual void finish() = 0;

    virtual void prepare() = 0;
    virtual void check() = 0;
    virtual void wait() = 0; 
    virtual void alarm() = 0;
};

struct TimerEle;
class I_TimerDealer {
public:
    virtual ~I_TimerDealer() {}

    virtual Void doTimeout(struct TimerEle* ele) = 0;
};


struct NodeBase;

class I_NodeObj {
public:
    virtual ~I_NodeObj() {}

    virtual Int32 getType() const = 0;
    
    /* return: 0-ok, other: error */
    virtual Uint32 readNode(struct NodeBase* base) = 0;

    /* return: 0-ok, 1-blocking write, other: error */
    virtual Uint32 writeNode(struct NodeBase* base) = 0;
    
    virtual Int32 procMsg(struct NodeBase* base, struct MsgHdr* msg) = 0;

    virtual Void procCmd(struct NodeBase* base, struct MsgHdr* msg) = 0;

    /* this function run in the dealer thread for resource free */
    virtual Void eof(struct NodeBase* base) = 0;
};

class SockCb {
public:
    virtual ~SockCb() {}
    
    virtual int onAccepted(struct NodeBase* base) = 0;

    virtual int onConnected(struct NodeBase* base) = 0;
    virtual void onFailConnect(struct NodeBase* base) = 0;

    virtual void onClosed(struct NodeBase* base) = 0;

    virtual Int32 process(struct NodeBase* base, struct MsgHdr* msg) = 0;
};


template<typename T>
class Singleton {
public:
    static T* getInst() {
        return m_instance;
    }

    static void set(T* inst) {
        m_instance = inst;
    }

private:
    static T* m_instance;
};

template<typename T>
T* Singleton<T>::m_instance = NULL;

#endif

