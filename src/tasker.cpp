#include"tasker.h"


unsigned int BitTool::addBit(unsigned int* pstate, unsigned int n) {
    unsigned int oldval = 0U;
    unsigned int newval = 0U;
    unsigned int retval = 0U;
    
    do {
        oldval = retval;
        
        if (n != (oldval & n)) {
            newval = (oldval | n);
        } else {
            /* already has the flag */
            break;
        }
        
        retval = CMPXCHG(pstate, oldval, newval);
    } while (retval != oldval);

    return oldval;
}

unsigned int BitTool::setBit(unsigned int* pstate, unsigned int n) {
    unsigned int state = 0U;
    
    state = ATOMIC_SET(pstate, n);
    return state;
}

bool BitTool::isMember(unsigned int state, unsigned int n) {
    return (n == (n & state));
}

bool BitTool::casBit(unsigned int* pstate, 
    unsigned int newval, unsigned int oldval) {
    bool bOk = true;

    bOk = CAS(pstate, oldval, newval);
    return bOk;
}

BinaryDoor::BinaryDoor() {
    m_state = 0U;
}

bool BinaryDoor::login() {
    unsigned int state = 0U;

    state = BitTool::addBit(&m_state, DEF_LOGIN_FLAG);
    if (0 == state) {
        return true;
    } else {
        return false;
    }
}

void BinaryDoor::lock() {    
    (void)BitTool::setBit(&m_state, DEF_LOCK_FLAG);
}

bool BinaryDoor::release() {
    bool bOk = true; 
    
    bOk = BitTool::casBit(&m_state, 0, DEF_LOCK_FLAG); 
    return bOk;
}


Tasker::Tasker() { 
    m_running = false; 
}

Tasker::~Tasker() {
}

void Tasker::resume() {
    bool needFire = false;

    needFire = m_door.login();
    if (needFire && m_running) {
        alarm();
    }
}

int Tasker::run() { 
    int ret = 0;
    bool done = true;

    ret = setup();
    if (0 == ret) {
        m_running = true;
        
        m_door.lock();
        while (m_running) {
            done = doTasks();
            if (done) {
                done = m_door.release();
                if (done) {
                    /* nothing to do, then wait here */
                    wait();
                } else {
                    check();
                } 
            } else {
                check();
            }

            /* restart the door state */
            m_door.lock();
        } 

        teardown(); 
    } 
    
    return ret;
}

void Tasker::stopRun() {
    if (m_running) {
        m_running = false;

        alarm();
    }
}


BitDoor::BitDoor() {
    for (int i=0; i<BIT_EVENT_MAX; ++i) {
        m_low_bits[i] = (0x1 << i);
        m_high_bits[i] = (m_low_bits[i] << BIT_EVENT_MAX);
        m_mask_bits[i] = (m_low_bits[i] | m_high_bits[i]);
    }
}

unsigned int BitDoor::lock(unsigned int* pstate) {
    unsigned int old_state = 0U;

    old_state = BitTool::setBit(pstate, m_low_bits[BIT_EVENT_LOCK]);
    return old_state;
}

bool BitDoor::release(unsigned int* pstate) {
    bool bOk = true; 
    
    bOk = BitTool::casBit(pstate, 0, m_low_bits[BIT_EVENT_LOCK]); 
    return bOk;
}

bool BitDoor::checkin(unsigned int* pstate, unsigned int ev) {    
    unsigned int old_state = 0U;
    unsigned int old_high = 0U;

    if (BIT_EVENT_NORM <= ev && BIT_EVENT_END > ev) {
        old_state = BitTool::addBit(pstate, m_low_bits[ev]);
        if (0 == old_state || (m_high_bits[ev] == (old_state & m_mask_bits[ev]))) {
            return true;
        } else {
            return false;
        }
    } else if (BIT_EVENT_END == ev) {
        /* to quick end */
        old_state = BitTool::setBit(pstate, 0xFFFF);
        if (0 == old_state) {
            return true;
        } else {
            old_high = (old_state >> BIT_EVENT_MAX);

            /* get correspending bit between high and low mask */
            if (0 != (old_high ^ (old_state & old_high))) {
                return true;
            } else {
                return false;
            }
        } 
    } else {
        /* invalid */
        return false;
    }
}

bool BitDoor::checkout(unsigned int* pstate, unsigned int ev) {
    unsigned int old_state = 0U;

    old_state = BitTool::addBit(pstate, m_high_bits[ev]);
    if (m_low_bits[ev] == (old_state & m_mask_bits[ev])) {
        return false;
    } else {
        return true;
    }
}

bool BitDoor::isEof(unsigned int state) {
    bool isEnd = false;
    
    isEnd = BitTool::isMember(state, m_low_bits[BIT_EVENT_END]);
    return isEnd;
} 

Pooler::Pooler() {
    INIT_ROOT(&m_safe_root);
    INIT_ROOT(&m_lock_root);
}

Pooler::~Pooler() {
}

bool Pooler::add(struct Task* task, unsigned int ev) {
    bool added = true; 
    
    added = m_door.checkin(&task->m_state, ev);
    if (added) {
        __add(task); 
    } 

    return added;
}

void Pooler::__add(struct Task* task) {    
    pushNode(&m_lock_root, &task->m_node);
}

void Pooler::__splice() {
    if (!isRootEmpty(&m_lock_root)) {
        spliceRoot(&m_lock_root, &m_safe_root);
    }
}

bool Pooler::consume(I_Worker* worker) {
    struct node* prev = NULL;
    struct node* curr = NULL;
    struct node* next = NULL;
    struct Task* task = NULL;
    unsigned int old_state = 0U;
    unsigned int ret = 0;
    bool done = true;

    __splice(); 
    
    for (prev=&m_safe_root.m_head, curr=prev->m_next; !!curr; curr=next) {
        next = curr->m_next;

        task = containof(curr, struct Task, m_node);
        
        old_state = m_door.lock(&task->m_state);

        ret = doTask(worker, task, old_state);
        if (BIT_EVENT_NORM <= ret && BIT_EVENT_CUSTOM > ret) {
            done = m_door.release(&task->m_state);
        } else if (BIT_EVENT_CUSTOM <= ret && BIT_EVENT_END > ret) {
            done = m_door.checkout(&task->m_state, ret);
        } else if (BIT_EVENT_IN == ret) {
            /* 1 means to keep in the queue */
            done = false;
        } else {
            /* others mean out ot queue */
            done = true;
        }

        if (done) {
            /* erase from the queue */
            prev->m_next = next;

            --m_safe_root.m_size;
        } else {
            /* keep in the queue */
            prev = curr;
        }
    }

    m_safe_root.m_tail = prev;

    return isRootEmpty(&m_safe_root);
}

unsigned int Pooler::doTask(I_Worker* worker,
    struct Task* task, unsigned int oldstate) {
    unsigned int ret = 0;
    bool isEnd = false;
    
    isEnd = m_door.isEof(oldstate);
    if (!isEnd) {
        ret = worker->procTask(task);

        return ret;
    } else {
        /* the end of task */
        worker->procTaskEnd(task);
        
        return BIT_EVENT_END;
    }
}


