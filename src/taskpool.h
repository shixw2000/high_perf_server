#ifndef __TASKPOOL_H__
#define __TASKPOOL_H__
#include"tasker.h" 
#include"interfobj.h" 


class CThread;

class WaiterFactory {
public:
    I_Waiter* creat(int type);
};

class EventWaiter : public I_Waiter {
public:
    EventWaiter();
    ~EventWaiter();

    int init();
    void finish();
    
    virtual void prepare() {}
    virtual void check() {}
    virtual void wait(); 
    virtual void alarm();

private:
    int m_event_fd;
};


class SigWaiter : public I_Waiter {
    struct _intern;
    
public:
    SigWaiter();
    ~SigWaiter();

    int init();
    void finish();

    virtual void prepare(); 
    virtual void check() {}
    virtual void wait(); 
    virtual void alarm();

private:
    struct _intern* m_intern;
};

/* this class cannot used here, it just act as a mistake */
class MutexWaiter : public I_Waiter {
    struct _intern;
    
public:
    MutexWaiter();
    ~MutexWaiter();
    
    int init();
    void finish();

    virtual void prepare() {}
    virtual void check() {}
    virtual void wait(); 
    virtual void alarm();

private:
    struct _intern* m_intern;
};

enum ENUM_WAITER_TYPE {
    ENUM_WAITER_EVENT = 0,
    ENUM_WAITER_SIG,

    /* attention: this mutex used here may be blocked by a lost signal
        * between alarm and wait
        */
    ENUM_WAITER_MUTEX,

    ENUM_WAITER_TYPE_MAX
};


class Lock;
class LockPooler : public Pooler {
public:
    LockPooler();
    virtual ~LockPooler();

    virtual int init();
    virtual void finish();

protected:
    virtual void __add(struct Task* task);
    virtual void __splice();
    
private:
    Lock* m_lock;
};


class TaskWorker : public Tasker, public I_Worker {
public:
    TaskWorker();
    virtual ~TaskWorker(); 

    virtual int init();
    virtual void finish(); 
    
    bool endTask(struct Task* task);
    bool addTask(struct Task* task, unsigned int ev); 

protected: 
    virtual void preTasks() {}
    virtual bool postTasks() { return true; }
    virtual bool doTasks();

private:
    LockPooler* m_pooler;
    
};

class TaskThread : public TaskWorker {
public:
    TaskThread();
    virtual ~TaskThread();
    
    virtual int init();
    virtual void finish();

    virtual int start(const char* name);
    virtual void join();
    virtual void stop(); 

private:
    CThread* m_thread;
};

class TaskPool : public TaskThread {
public:
    explicit TaskPool(int type = ENUM_WAITER_EVENT);
    virtual ~TaskPool();

    virtual int init();
    virtual void finish();

protected:
    virtual int setup();
    virtual void teardown() {}

    virtual void check() { m_waiter->check(); }
    virtual void wait() { m_waiter->wait(); }
    virtual void alarm() { m_waiter->alarm(); }

private:
    I_Waiter* m_waiter;
    int m_type;
};


#endif

