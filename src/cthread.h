#ifndef __CTHREAD_H__
#define __CTHREAD_H__
#include<pthread.h>
#include<unistd.h>

class CThread {
public:
    CThread();
    virtual ~CThread();

    int start(const char name[]);
    void stop();
    
    int join();
    int cancel();
    int kill(int sig);
    void testkill();
    
    pthread_t getThrid() const { return m_thr; }

    static int sleepSec(int sec);
    
    static int sleepMiliSec(int milisec);
    
    static pid_t forkSelf();

protected:
    virtual int prepareRun() { return 0; }
    virtual void postRun() {}
    virtual int run() = 0;
    bool isRun();

private:
    static void* activate(void* arg);
    
protected:
    pthread_t m_thr;
    bool m_isRun;
    char m_name[32];
};


extern void armSig(int sig);
extern void armUsr1();
extern void armTerm();
extern void sendsig();
extern void sendselfsig();
extern void threadSuspend();
extern void threadMaskSig();


#endif

