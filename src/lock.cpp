#include<stdlib.h>
#include"globaltype.h"
#include"lock.h"


SpinLock::SpinLock() {
    m_lock = NULL;
}

SpinLock::~SpinLock() {
}

int SpinLock::init() {
    int ret = 0;

    m_lock = (pthread_spinlock_t*)malloc(sizeof(pthread_spinlock_t));
    if (NULL != m_lock) {
        ret = pthread_spin_init(m_lock, PTHREAD_PROCESS_PRIVATE);
        if (0 == ret) {
            return 0;
        } else {
            LOG_ERROR("spin_lock_init| error=init:%s|", ERR_MSG());
            return -1;
        }
    } else {
        LOG_ERROR("spin_lock_init| error=malloc:%s|", ERR_MSG());
        return -1;
    }
}

void SpinLock::finish() {
    if (NULL != m_lock) {
        pthread_spin_destroy(m_lock);
        free((void*)m_lock);
        
        m_lock = NULL;
    }
}

bool SpinLock::lock(int n) {
    int ret = 0;

    ret = pthread_spin_lock(m_lock);
    if (0 == ret) {
        return true;
    } else {
        LOG_ERROR("spin_lock| ret=%d| n=%d| error=%s|",
            ret, n, ERR_MSG());
        return false;
    }
}

bool SpinLock::unlock(int n) {
    int ret = 0;

    ret = pthread_spin_unlock(m_lock);
    if (0 == ret) {
        return true;
    } else {
        LOG_ERROR("spin_unlock| ret=%d| n=%d| error=%s|", 
            ret, n, ERR_MSG());
        return false;
    }
}


GrpSpinLock::GrpSpinLock(int order)
    : m_capacity(0x1<<order), m_mask(m_capacity-1) {
    m_locks = NULL;
}

GrpSpinLock::~GrpSpinLock() {
}

int GrpSpinLock::init() {
    int ret = 0;

    m_locks = (pthread_spinlock_t*)calloc(m_capacity, 
        sizeof(pthread_spinlock_t));
    if (NULL != m_locks) {
        for (int i=0; i<m_capacity; ++i) {
            ret = pthread_spin_init(&m_locks[i], PTHREAD_PROCESS_PRIVATE);
            if (0 != ret) {
                LOG_INFO("grp_spin_lock_init| capacity=%d|"
                    " cnt=%d| error=init:%s|",
                    m_capacity, i, ERR_MSG());
                return -1;
            } 
        }

        return 0;
    } else {
        LOG_INFO("grp_spin_lock_init| capacity=%d| error=calloc:%s|",
            m_capacity, ERR_MSG());
        return -1;
    }
}

void GrpSpinLock::finish() {
    if (NULL != m_locks) {
        for (int i=0; i<m_capacity; ++i) {
            pthread_spin_destroy(&m_locks[i]);
        }
        
        free((void*)m_locks);
        m_locks = NULL;
    }
}

bool GrpSpinLock::lock(int n) {
    int ret = 0;
    int mask = 0;
    
    mask = n & m_mask;
    ret = pthread_spin_lock(&m_locks[mask]);
    if (0 == ret) {
        return true;
    } else {
        LOG_INFO("spin_lock| n=%d| mask=%d| ret=%d| error=%s|", 
            n, mask, ret, ERR_MSG());
        return false;
    }
}

bool GrpSpinLock::unlock(int n) {
    int ret = 0;
    int mask = 0;
    
    mask = n & m_mask;
    ret = pthread_spin_unlock(&m_locks[mask]);
    if (0 == ret) {
        return true;
    } else {
        LOG_INFO("spin_unlock| n=%d| mask=%d| ret=%d| error=%s|", 
            n, mask, ret, ERR_MSG());
        return false;
    }
}


MutexLock::MutexLock() {
    m_mutex = NULL;
}

MutexLock::~MutexLock() {
}

int MutexLock::init() {
    int ret = 0;

    m_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (NULL != m_mutex) {
        ret = pthread_mutex_init(m_mutex, NULL);
        if (0 == ret) {
            return 0;
        } else {
            LOG_ERROR("mutex_lock_init| error=init:%s|", ERR_MSG());
            return -1;
        }
    } else {
        LOG_ERROR("mutex_lock_init| error=malloc:%s|", ERR_MSG());
        return -1;
    }
}

void MutexLock::finish() {
    if (NULL != m_mutex) {
        pthread_mutex_destroy(m_mutex);
        free(m_mutex);
        
        m_mutex = NULL;
    }
}

bool MutexLock::lock(int n) {
    int ret = 0;

    ret = pthread_mutex_lock(m_mutex);
    if (0 == ret) {
        return true;
    } else {
        LOG_ERROR("mutex_lock| ret=%d| n=%d| error=%s|", 
            ret, n, ERR_MSG());
        return false;
    }
}

bool MutexLock::unlock(int n) {
    int ret = 0;

    ret = pthread_mutex_unlock(m_mutex);
    if (0 == ret) {
        return true;
    } else {
        LOG_ERROR("mutex_unlock| ret=%d| n=%d| error=%s|",
            ret, n,ERR_MSG());
        return false;
    }
}

MutexCond::MutexCond() {
    m_cond = NULL;
}

MutexCond::~MutexCond() {
}

int MutexCond::init() {
    int ret = 0;

    ret = MutexLock::init();
    if (0 != ret) {
        return ret;
    }

    m_cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    if (NULL != m_cond) {
        ret = pthread_cond_init(m_cond, NULL);
        if (0 == ret) {
            return 0;
        } else {
            LOG_ERROR("mutex_cond_init| error=init:%s|", ERR_MSG());
            return -1;
        }
    } else {
        LOG_ERROR("mutex_cond_init| error=malloc:%s|", ERR_MSG());
        return -1;
    }
}

void MutexCond::finish() {
    if (NULL != m_cond) {
        pthread_cond_destroy(m_cond);
        free(m_cond);
        
        m_cond = NULL;
    }

    MutexLock::finish();
}

bool MutexCond::signal() {
    int ret = 0;

    ret = pthread_cond_signal(m_cond);
    if (0 == ret) {
        return true;
    } else {
        LOG_ERROR("mutex_cond_signal| ret=%d| error=%s|", ret, ERR_MSG());
        return false;
    }
}

bool MutexCond::broadcast() {
    int ret = 0;

    ret = pthread_cond_broadcast(m_cond);
    if (0 == ret) {
        return true;
    } else {
        LOG_ERROR("mutex_cond_broadcast| ret=%d| error=%s|", ret, ERR_MSG());
        return false;
    }
}

bool MutexCond::wait() {
    int ret = 0;

    ret = pthread_cond_wait(m_cond, m_mutex);
    if (0 == ret) {
        return true;
    } else {
        LOG_ERROR("mutex_cond_wait| ret=%d| error=%s|", ret, ERR_MSG());
        return false;
    }
}
