#include<pthread.h>
#include<string.h>
#include"globaltype.h"
#include"cthread.h"


struct CThread::_intern {
    pthread_t m_thr;
    char m_name[32];
};

CThread::CThread() {
    m_intern = NULL;
    m_service = NULL;
}

CThread::~CThread() {
    if (NULL != m_intern) {
        delete m_intern;
    }
}

unsigned long CThread::getThr() const {
    if (NULL != m_intern) {
        return m_intern->m_thr;
    } else {
        return 0;
    }
}

int CThread::start(const char name[], I_Service* service) {
    int ret = 0;
    pthread_attr_t attr;

    m_intern = new struct _intern;
    memset(m_intern, 0, sizeof(*m_intern));
    
    if (NULL != name) {
        strncpy(m_intern->m_name, name, sizeof(m_intern->m_name));
    }

    if (NULL != service) {
        m_service = service;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&m_intern->m_thr, &attr, &CThread::activate, this);
    if (0 == ret) {        
        LOG_DEBUG("pthread_create| thrid=0x%lx| name=%s|", 
            m_intern->m_thr, m_intern->m_name);
    } else {
        LOG_ERROR("pthread_create| ret=%d| error=%s|", ret, ERR_MSG());

        ret =  -1;
    }
    
    pthread_attr_destroy(&attr); 
    return ret;
}

void CThread::join() {
    if (NULL != m_intern && 0 != m_intern->m_thr) { 
        pthread_join(m_intern->m_thr, NULL);
        m_intern->m_thr = 0;
    }
}

void* CThread::activate(void* arg) {
    int ret = 0;
    CThread* pthr = (CThread*)arg;
    
    ret = pthr->run(); 

    LOG_INFO("exit| ret=%d| thrid=0x%lx| name=%s|", 
        ret, pthr->m_intern->m_thr, pthr->m_intern->m_name); 
    
    return (void*)(Uint64)ret;
}


