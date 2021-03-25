#include"sharedheader.h"
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
            LOG_WARN("spin_lock_init| error=init:%s|", ERR_MSG);
            return -1;
        }
    } else {
        LOG_WARN("spin_lock_init| error=malloc:%s|", ERR_MSG);
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
        LOG_WARN("spin_lock| ret=%d| error=%s|", ret, ERR_MSG);
        return false;
    }
}

bool SpinLock::unlock(int n) {
    int ret = 0;

    ret = pthread_spin_unlock(m_lock);
    if (0 == ret) {
        return true;
    } else {
        LOG_WARN("spin_unlock| ret=%d| error=%s|", ret, ERR_MSG);
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
                LOG_WARN("grp_spin_lock_init| capacity=%d|"
                    " cnt=%d| error=init:%s|",
                    m_capacity, i, ERR_MSG);
                return -1;
            } 
        }

        return 0;
    } else {
        LOG_WARN("grp_spin_lock_init| capacity=%d| error=calloc:%s|",
            m_capacity, ERR_MSG);
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
        LOG_WARN("spin_lock| n=%d| mask=%d| ret=%d| error=%s|", 
            n, mask, ret, ERR_MSG);
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
        LOG_WARN("spin_unlock| n=%d| mask=%d| ret=%d| error=%s|", 
            n, mask, ret, ERR_MSG);
        return false;
    }
}


