#include"sharedheader.h"
#include"cthread.h"
#include"lock.h"
#include"hque.h"


ElePool::ElePool(int capacity) : m_capacity(capacity) {
    m_pool = NULL;
    m_oper = NULL;
    clean(&m_root);
    
    m_left = 0;
    m_in_cnt = 0;
    m_out_cnt = 0;
}

ElePool::~ElePool() {
}

int ElePool::init() {
    int ret = 0;
    enum ListType type = DEF_USE_LOCK_TYPE;

    m_oper = QueFactory::create(type); 
    if (NULL == m_oper) {
        LOG_ERROR("init_queoper| capacity=%d| error=create failed|",
            m_capacity);
        return -1;
    }

    m_pool = (struct AtomEle*)calloc(m_capacity, sizeof(struct AtomEle));
    if (NULL == m_pool) {
        LOG_ERROR("init_msgpool| capacity=%d| error=calloc NULL|",
            m_capacity);
        
        return -1;
    }

    for (int i=0; i<m_capacity; ++i) {
        add(&m_root, &m_pool[i].m_list);
    }

    return ret;
}

void ElePool::finish() {
    m_left = 0;
    clean(&m_root);
    
    if (NULL != m_oper) {
        QueFactory::release(m_oper);
        m_oper = NULL;
    }
    
    if (NULL != m_pool) {
        free(m_pool);
        m_pool = NULL;
    }
}

struct AtomEle* ElePool::alloc() {
    int left = 0;
    struct AtomEle* ele = NULL;
    struct hlist* item = NULL;

    item = m_oper->pop(&m_root); 
    if (NULL != item) {
        incOut(); 
        left = ATOMIC_DEC(&m_left);
        ele = containof(item, struct AtomEle, m_list);
        reset(ele); 
        
        if (0 == left % 500000) {
            LOG_DEBUG("alloc| left=%d| in=%lld| out=%lld|",
                left, getInCnt(), getOutCnt());
        }
        
        return ele;
    } else {
        LOG_WARN("alloc| left=%d| in=%lld| out=%lld|"
            " error=hqueue alloc null|", 
            left, getInCnt(), getOutCnt());
        
        return NULL;
    }
}

void ElePool::release(struct hlist* list) {        
    bool bOk = true;
    
    if (NULL != list) { 
        bOk = m_oper->push(&m_root, list); 
        if (bOk) { 
            incIn(); 
            ATOMIC_INC(&m_left);
        }
    }
}

void ElePool::reset(struct AtomEle* ele) {
    static unsigned int g_seq = 0U;
    
    memset(ele, 0, sizeof(*ele));  
    ele->m_type = ELE_DATA_NONE;
    ele->m_seq = ATOMIC_INC(&g_seq);
}

void ElePool::add(struct deque_root* root, struct hlist* ele) {
    ele->next = NULL;

    ++m_in_cnt;
    ++m_left;
    if (NULL != root->tail) {
        root->tail->next = ele;
        root->tail = ele;
    } else {
        root->tail = ele;
        root->head = ele;
    }
}


