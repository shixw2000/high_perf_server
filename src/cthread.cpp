#include<unistd.h>
#include<pthread.h>
#include<signal.h>
#include<sys/types.h>
#include"sharedheader.h"
#include"cthread.h"


CThread::CThread() {
    m_thr = 0;
    m_isRun = false;
    memset(m_name, 0, sizeof(m_name));
}

CThread::~CThread() {
}

int CThread::start(const char name[]) {
    int ret = 0;
    pthread_attr_t attr;

    if (NULL != name) {
        strncpy(m_name, name, 30);
    }
    
    if (0 < m_thr) {
        LOG_ERROR("pthread_start| m_thr=0x%lx|"
            " msg=thread is already running|", m_thr);
        return -1;
    }

    ret = prepareRun();
    if (0 != ret) {
        LOG_ERROR("pthread_start| ret=%d|"
            " msg=fail to prepareRun|", ret);
        return ret;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    ret = pthread_attr_setstacksize(&attr, 5*1024*1024);
    if (0 != ret) {
        LOG_ERROR("pthread_attr_setstacksize| ret=%d|", ret);
    }

    ATOMIC_SET(&m_isRun, true);
    ret = pthread_create(&m_thr, &attr, &CThread::activate, this);
    if (0 == ret) {
        LOG_DEBUG("pthread_create| thrid=0x%lx| name=%s|", m_thr, m_name);
    } else {
        ATOMIC_SET(&m_isRun, false);
        LOG_ERROR("pthread_create| ret=%d| error=%s|", ret, ERR_MSG);
        ret =  -1;
    }
    
    pthread_attr_destroy(&attr); 
    return ret;
}

void CThread::stop() {
    ATOMIC_SET(&m_isRun, false);
}

bool CThread::isRun() {
    return ACCESS_ONCE(m_isRun);
}

int CThread::join() {
    int ret = 0;
    void* err = NULL;

    if (0 < m_thr) { 
        ret = pthread_join(m_thr, &err);
        if (0 == ret) {
            m_thr = 0;

            if (err == PTHREAD_CANCELED) {
               LOG_INFO("pthread_join| thread was canceled\n");
            } else {
                LOG_INFO("pthread_join| ret=%p|\n", err);
            }
            
        } else {
            LOG_ERROR("pthread_join| ret=%d| error=%s|", ret, ERR_MSG);
            ret = -1;
        }
    }

    return ret;
}

int CThread::cancel() {
    int ret = 0;

    if (0 < m_thr) {
        ret = pthread_cancel(m_thr);
        if (0 != ret) {
            LOG_ERROR("pthread_cancel| ret=%d| error=%s|", ret, ERR_MSG);
        }
    } else {
        ret = -1;
    }

    return ret;
}

int CThread::kill(int sig) {
    int ret = 0;

    if (0 < m_thr) {
        ret = pthread_kill(m_thr, sig);
        if (0 == ret) {
            LOG_DEBUG("pthread_kill| my=0x%lx| thr=0x%lx| sig=%d| ret=%d|",
                pthread_self(), m_thr, sig, ret);
        } else {
            LOG_WARN("pthread_kill| thr=0x%lx| sig=%d| ret=%d| error=%s|", 
                m_thr, sig, ret, ERR_MSG);
        }
    } else {
        ret = -1;
    }

    return ret;
}

void CThread::testkill() {
    this->kill(SIGUSR1);
}

void* CThread::activate(void* arg) {
    int ret = 0;
    int state = 0;
    int type = 0;
    CThread* pthr = NULL;

    pthr = (CThread*)arg;
    
    //state = PTHREAD_CANCEL_DISABLE; 
    state = PTHREAD_CANCEL_ENABLE;
    pthread_setcancelstate(state, NULL);

    type = PTHREAD_CANCEL_DEFERRED; 
    //type = PTHREAD_CANCEL_ASYNCHRONOUS;
    pthread_setcanceltype(type, NULL);

    LOG_INFO("enter_thread| thrid=0x%lx| state=%d| type=%d| name=%s|",
        pthread_self(), state, type, pthr->m_name); 
    
    ret = pthr->run();
    
    LOG_DEBUG("enter_postRun| thrid=0x%lx| ret=%d| name=%s|", 
        pthread_self(), ret, pthr->m_name);
    pthr->postRun();

    LOG_INFO("exit| thrid=0x%lx| ret=%d| name=%s|", 
        pthread_self(), ret, pthr->m_name);
    
    return (void*)(size_t)ret;
}

int CThread::sleepSec(int sec) {
    int ret = 0;
    
    ret = (int)sleep(sec);
    return ret;
}

int CThread::sleepMiliSec(int milisec) {
    int ret = 0;
    unsigned int tm = 0;

    tm = (unsigned int)milisec * 1000;
    ret = usleep(tm);
    if (0 != ret) {
        LOG_INFO("sleep| milisec=%d| ret=%d| error=%s|", 
            milisec, ret, ERR_MSG);
    }
    
    return ret;
}

pid_t CThread::forkSelf() {
    pid_t pid = 0;

    pid = fork();
    if (0 < pid) {
        LOG_INFO("this is in parent(%d ==> %d)",
            getpid(), pid);
    } else if (0 == pid) {
        LOG_INFO("this is in child(%d ==> %d)",
            pid, getpid());
    } else {
        LOG_ERROR("forkSelf| ret=%d| error=%s|",
            pid, ERR_MSG);
    }

    return pid;
}

static void sigdealer(int sig) {
    LOG_INFO("===signal=%d|\n", sig);
}

static void sigdealerinfo(int sig, siginfo_t* info, void* ctx) {
    LOG_VERB("signal=%d| pid=%d| thrid=0x%lx| si_code=%d:%d| val=%p|\n", 
        sig, info->si_pid, 
        pthread_self(),
        SI_QUEUE, info->si_code, 
        info->si_value.sival_ptr);
}

void armSig(int sig) {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);

    #if 1
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    act.sa_sigaction = &sigdealerinfo;
    #else
    act.sa_flags = SA_RESTART;
    act.sa_handler = sigdealer;
    //act.sa_handler = SIG_IGN;
    //act.sa_handler = SIG_DFL;
    #endif

    sigaction(sig, &act, NULL);
}

void armTerm() {
    armSig(15);
}

static int g_sig = SIGUSR1;

void armUsr1() {
    armSig(g_sig);
}

void sendsig() {
    int sig = g_sig;
    union sigval val;

    val.sival_ptr = (void*)0x12345678;
    sigqueue(getpid(), sig, val);
}

void sendselfsig() {
    int sig = g_sig;
    
    pthread_kill(pthread_self(), sig);
}

void threadSuspend() {
    int ret = 0;
    int sig = g_sig;
    sigset_t sets;

    pthread_sigmask(SIG_BLOCK, NULL, &sets);

    sigdelset(&sets, sig);
    ret = sigsuspend(&sets);
    if (EINTR != errno) {
        LOG_WARN("sigsuspend| ret=%d| error=%s|",
            ret, ERR_MSG);
    }

    return;
}

void threadMaskSig() {
    int ret = 0;
    int sig = g_sig;
    sigset_t sets;

    sigemptyset(&sets);
    sigaddset(&sets, sig);
    ret = pthread_sigmask(SIG_BLOCK, &sets, NULL);
    if (0 != ret) {
        LOG_WARN("pthread_sigmask| ret=%d| error=%s|",
            ret, ERR_MSG);
    }

    return;
}
    