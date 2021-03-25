#include"sharedheader.h"
#include"statemachine.h"
#include"lock.h"
#include"sockdata.h"
#include"pushpool.h"


CPushPool::CPushPool() { 
    m_listpool = NULL;
    m_door = NULL;
    m_tail = &m_local.head;
    m_local.head.next = NULL;
    m_global.head.next = NULL;
}

CPushPool::~CPushPool() {
}

int CPushPool::init() {
    int ret = 0;
    enum ListType type = DEF_USE_LOCK_TYPE;

    m_door = new BitDoor;
    if (NULL == m_door) {
        return -1;
    }

    m_listpool = ListFactory::create(type);
    if (NULL == m_listpool) {
        return -1;
    }
    
    return ret;
}

void CPushPool::finish() {    
    if (NULL != m_listpool) {
        ListFactory::release(m_listpool);
        m_listpool = NULL;
    }

    if (NULL != m_door) {
        delete m_door;
        m_door = NULL;
    }
}

int CPushPool::produce(unsigned int ev, struct TaskElement* task) { 
    int ret = 0;
    bool bOk = false; 
    struct hlist* ele = NULL;

    if (m_door->isValid(ev)) {
        ele = &task->m_list;
        bOk = m_door->checkin(ev, &task->m_state);
        if (bOk) {
            /* let ele in queue */
            bOk = m_listpool->push(&m_global, ele);
            if (bOk) { 
                LOG_VERB("===produce| ev=%u| task=%p| msg=add ele|", 
                    ev, task);
                ret = 1;
            } else {
                LOG_WARN("produce| ev=%u| task=%p| error=push fail|", 
                    ev, task);
                ret = -1;
            }
        } else { 
            LOG_VERB("===produce| ev=%u| task=%p| msg=neednot add ele|", 
                ev, task);
        }
    } else {
        LOG_WARN("produce| ev=%u| task=%p| error=invalid event|",
            ev, task);
        ret = -1;
    } 
    
    return ret;
}

bool CPushPool::consume() {
    bool isOut = true; 
    unsigned int state = 0U;
    unsigned int ev = 0U;
    struct hlist* prev = NULL;
    struct hlist* cur = NULL;
    struct hlist* next = NULL;
    struct TaskElement* task = NULL;
    struct hlist_root root;

    /* append new tasks to tail */
    m_listpool->popall(&m_global, &root);
    if (NULL != root.head.next) {
        m_tail->next = root.head.next;
    }

    prev = &m_local.head;
    for (cur = prev->next; NULL != cur; cur = next) {
        next = cur->next;

        task = containof(cur, struct TaskElement, m_list);
    
        state = m_door->lockDoor(&task->m_state);
        ev = procTask(state, task);
        if (m_door->isValid(ev)) {
            isOut = m_door->checkout(ev, &task->m_state); 
            if (isOut) {
                /* kick out current ele */
                prev->next = next; 
            } else {
                prev = cur;
            }
        } else {
            LOG_WARN("consume| ev=%u| task=%p| error=invalid event|",
                ev, task);

            /* kick out the task */
            prev->next = next; 
        }
    } 
    
    /* record the tail */
    m_tail = prev;
    
    return prev == &m_local.head;
}

bool CPushPool::isEventSet(unsigned int ev, unsigned int state) {
    bool bSet = false;

    bSet = m_door->isEventSet(ev, state);
    return bSet;
}

int CPushPool::registerEvent(unsigned int ev, int type) {
    int ret = 0;

    ret = m_door->registerEvent(ev, type); 
    return ret;
}

