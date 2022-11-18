#include<signal.h>
#include<pthread.h>
#include"lock.h"
#include"cthread.h"
#include"sockutil.h"
#include"taskpool.h"


I_Waiter* WaiterFactory::creat(int type) {
    I_Waiter* waiter = NULL;

    switch (type) {
    case ENUM_WAITER_EVENT:
        waiter = new EventWaiter;
        break;

    case ENUM_WAITER_SIG:
        waiter = new SigWaiter;
        break;

    case ENUM_WAITER_MUTEX:
    default:
        waiter = new MutexWaiter;
        break;
    }

    return waiter;
}

EventWaiter::EventWaiter() {
    m_event_fd = -1;
}

EventWaiter::~EventWaiter() {
}

int EventWaiter::init() {
    int ret = 0;
    
    ret = creatEventFd();
    if (0 <= ret) {
        m_event_fd = ret;
        
        return 0;
    } else {
        return -1;
    } 
}

void EventWaiter::finish() { 
    if (0 <= m_event_fd) {
        closeHd(m_event_fd);

        m_event_fd = -1;
    }
}

void EventWaiter::alarm() {
    writeEvent(m_event_fd); 
}
 
void EventWaiter::wait() {
    waitEvent(m_event_fd, -1);
}


struct SigWaiter::_intern {
    pthread_t m_thr;
    sigset_t m_sigset;
    int m_sig;
};

SigWaiter::SigWaiter() {
    m_intern = NULL; 
}

SigWaiter::~SigWaiter() {
}

int SigWaiter::init() {
    int ret = 0;

    m_intern = new struct _intern;
    memset(m_intern, 0, sizeof(*m_intern));

    sigemptyset(&m_intern->m_sigset);
    m_intern->m_sig = SIGUSR1; 
    
    return ret;
}

void SigWaiter::finish() { 
    if (NULL != m_intern) {        
        delete m_intern;
        m_intern = NULL;
    }
}

void SigWaiter:: prepare() {
    if (NULL != m_intern) {
        m_intern->m_thr = pthread_self();
    }
}

void SigWaiter::alarm() { 
    if (0 != m_intern->m_thr) {
        pthread_kill(m_intern->m_thr, m_intern->m_sig);
    }
}
 
void SigWaiter::wait() {    
    sigsuspend(&m_intern->m_sigset);
}


struct MutexWaiter::_intern {
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

MutexWaiter::MutexWaiter() {
    m_intern = NULL; 
}

MutexWaiter::~MutexWaiter() {
}

int MutexWaiter::init() {
    int ret = 0;
    
    m_intern = new struct _intern;
    memset(m_intern, 0, sizeof(*m_intern));

    ret = pthread_mutex_init(&m_intern->m_mutex, NULL);
    if (0 != ret) {
        return -1;
    }
    
    ret = pthread_cond_init(&m_intern->m_cond, NULL);
    if (0 != ret) {
        return -1;
    }

    return ret;
}

void MutexWaiter::finish() { 
    if (NULL != m_intern) {   

        pthread_cond_destroy(&m_intern->m_cond);
        pthread_mutex_destroy(&m_intern->m_mutex);
        
        delete m_intern;
        m_intern = NULL;
    }
}

void MutexWaiter::alarm() {
    /* if there are no thread currently blocked on on cond,
        * there have no effects */
    int ret = 0;

    ret = pthread_mutex_lock(&m_intern->m_mutex);
    if (0 == ret) {
        pthread_cond_signal(&m_intern->m_cond);
        
        pthread_mutex_unlock(&m_intern->m_mutex);
    } 
}
 
void MutexWaiter::wait() {
    int ret = 0;

    ret = pthread_mutex_lock(&m_intern->m_mutex);
    if (0 == ret) {
        pthread_cond_wait(&m_intern->m_cond, &m_intern->m_mutex);
        
        pthread_mutex_unlock(&m_intern->m_mutex);
    }
}

LockPooler::LockPooler() {
    m_lock = NULL;
}

LockPooler::~LockPooler() {
}

int LockPooler::init() {
    int ret = 0;
    
    m_lock = new SpinLock; 
    ret = m_lock->init();
    if (0 != ret) {
        return ret;
    }
    
    return 0;
}

void LockPooler::finish() { 
    if (NULL != m_lock) {
        m_lock->finish();
        
        delete m_lock;
        m_lock = NULL;
    }
}

void LockPooler::__add(struct Task* task) {
    bool bOk = true;

    bOk = m_lock->lock();
    if (bOk) {
        Pooler::__add(task);
        
        m_lock->unlock();
    }
}

void LockPooler::__splice() {
    bool bOk = true;

    bOk = m_lock->lock();
    if (bOk) {
        Pooler::__splice();
        
        m_lock->unlock();
    }
}

TaskWorker::TaskWorker() {
    m_pooler = NULL; 
}

TaskWorker::~TaskWorker() {
}

int TaskWorker::init() {
    int ret = 0;

    m_pooler = new LockPooler; 
    ret = m_pooler->init();
    if (0 != ret) {
        return ret;
    }
    
    return 0;
}

void TaskWorker::finish() { 
    if (NULL != m_pooler) {
        m_pooler->finish();
        
        delete m_pooler;
        m_pooler = NULL;
    }
}

bool TaskWorker::endTask(struct Task* task) {
    bool added = true;

    added = m_pooler->add(task, BIT_EVENT_END);
    if (added) {
        resume();
    }
    
    return added;
}

bool TaskWorker::addTask(struct Task* task, unsigned int ev) {
    bool added = false;

    if (BIT_EVENT_NORM <= ev && BIT_EVENT_END > ev) {
        added = m_pooler->add(task, ev);
        if (added) {
            resume();
        }
    }

    return added;
 }

bool TaskWorker::doTasks() {
    bool done1 = true;
    bool done2 = true;

    preTasks();
    
    done1 = m_pooler->consume(this); 
    done2 = postTasks();

    return done1 && done2;
}


TaskThread::TaskThread() {
    m_thread = NULL;
}

TaskThread::~TaskThread() {
}

int TaskThread::init() {
    int ret = 0;

    ret = TaskWorker::init();
    if (0 != ret) {
        return ret;
    }

    m_thread = new CThread;

    return 0;
}

void TaskThread::finish() { 
    if (NULL != m_thread) {
        delete m_thread;
        m_thread = NULL;
    }

    TaskWorker::finish();
}

int TaskThread::start(const char* name) {
    int ret = 0; 
    
    ret = m_thread->start(name, this);
    if (0 != ret) {
        return ret;
    }

    return ret;
}

void TaskThread::join() {
    if (NULL != m_thread) {
        m_thread->join();
    }
}

void TaskThread::stop() {
    stopRun();
}


TaskPool::TaskPool(int type) : m_type(type) {
    m_waiter = NULL;
}

TaskPool::~TaskPool() {
}

int TaskPool::init() {
    int ret = 0;
    WaiterFactory fctry;

    ret = TaskThread::init();
    if (0 != ret) {
        return ret;
    }

    m_waiter = fctry.creat(m_type); 
    ret = m_waiter->init();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void TaskPool::finish() { 
    if (NULL != m_waiter) {
        m_waiter->finish();
        
        delete m_waiter;
        m_waiter = NULL;
    } 

    TaskThread::finish();
}

int TaskPool::setup() {
    m_waiter->prepare();

    return 0;
}

