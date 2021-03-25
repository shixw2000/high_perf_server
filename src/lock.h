#ifndef __LOCK_H__
#define __LOCK_H__
#include<pthread.h>


class Lock {
public:
    virtual ~Lock() {}

    virtual int init() { return 0; }
    virtual void finish() {}
    
    virtual bool lock(int n = -1) = 0;
    virtual bool unlock(int n = -1) = 0;
};

class EmptyLock : public Lock {
public:
    virtual bool lock(int n) { return true; }
    virtual bool unlock(int n) { return true; }
};

class SpinLock : public Lock {
public:
    SpinLock();
    ~SpinLock();

    int init();
    void finish();

    virtual bool lock(int n);
    virtual bool unlock(int n);

private:
    pthread_spinlock_t* m_lock;
};

class GrpSpinLock : public Lock {
public:
    explicit GrpSpinLock(int order);
    virtual ~GrpSpinLock();

    int init();
    void finish();
    
    virtual bool lock(int n);
    virtual bool unlock(int n);

private:
    const int m_capacity;
    const int m_mask;
    pthread_spinlock_t* m_locks;
};

#endif

