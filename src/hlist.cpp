#include"sharedheader.h"
#include"lock.h"
#include"hlist.h"


void ListNodeOper::clean_node(struct list_node* node) {
    node->prev = node;
    node->next = node;
}

bool ListNodeOper::isEmpty(struct list_node* root) {
    return root->next == root;
}

void ListNodeOper::addTail(struct list_node* root, struct list_node* node) {
    struct list_node* prev = NULL;

    prev = root->prev;
    
    root->prev = node;
    node->next = root;
    
    prev->next = node;
    node->prev = prev;
}

void ListNodeOper::delNode(struct list_node* node) {
    struct list_node* prev = NULL;
    struct list_node* next = NULL;

    prev = node->prev;
    next = node->next;
    prev->next = next;
    next->prev = prev;
    clean_node(node);
}

struct list_node* ListNodeOper::delall(struct list_node* root) {
    struct list_node* head = NULL;
    
    if (root != root->next) { 
        root->prev->next = NULL;
        root->next->prev = NULL;

        head = root->next;
        clean_node(root);
    }

    return head;
}

void clean(struct deque_root* root) {
    root->m_size = 0;
    root->head = NULL;
    root->tail = NULL;
} 


class AtomListPool : public ListPool {
public:
    virtual int init() { return 0; }
    virtual void finish() {}
    virtual bool push(struct hlist_root* root, struct hlist* list);
    virtual void popall(struct hlist_root* src, struct hlist_root* dst);
};

bool AtomListPool::push(struct hlist_root* root, struct hlist* list) {
    struct hlist* head = NULL;
    struct hlist* old = NULL;

    old = ACCESS_ONCE(root->head.next);
    
    do {
        head = old;
        list->next = head;
        smp_mb();
        old = CMPXCHG(&root->head.next, head, list);
    } while (old != head);

    return true;
}

void AtomListPool::popall(struct hlist_root* src, struct hlist_root* dst) {
    struct hlist* head = NULL;
    struct hlist* old = NULL;

    old = ACCESS_ONCE(src->head.next);
    
    while (NULL != old && old != head) {
        head = old;
        smp_mb();
        old = CMPXCHG(&src->head.next, head, NULL); 
    }

    dst->head.next = old;
}

class SpinListPool : public ListPool {
public:
    virtual int init();
    virtual void finish();
    virtual bool push(struct hlist_root* root, struct hlist* list);
    virtual void popall(struct hlist_root* src, struct hlist_root* dst); 

private:
    Lock* m_lock;
};

int SpinListPool::init() {
    int ret = 0;

    m_lock = new SpinLock;
    if (NULL == m_lock) {
        return -1;
    }

    ret = m_lock->init();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void SpinListPool::finish() {
    if (NULL != m_lock) {
        m_lock->finish();
        delete m_lock;
        m_lock = NULL;
    }
}

bool SpinListPool::push(struct hlist_root* root, struct hlist* list) {
    bool bOk = true;
   
    bOk = m_lock->lock(0);
    if (bOk) {
        list->next = root->head.next;
        root->head.next = list;
        
        m_lock->unlock(0);
    } 

    return bOk;
}

void SpinListPool::popall(struct hlist_root* src, struct hlist_root* dst) {
    bool bOk = true;
   
    bOk = m_lock->lock(0);
    if (bOk) {
        dst->head.next = src->head.next;
        src->head.next = NULL;
        
        m_lock->unlock(0);
    } else {
        dst->head.next = NULL;
    }
}

ListPool* ListFactory::create(enum ListType type) {
    int ret = 0;
    ListPool* pool = NULL;

    switch (type) {
    case ATOM_LIST_TYPE:
        pool = new AtomListPool;
        break;

    case SPIN_LIST_TYPE:
    default:
        pool = new SpinListPool;
        break;
    }

    if (NULL != pool) {
        ret = pool->init();
        if (0 == ret) {
            /* create ok and return */
        } else {
            release(pool);
            pool = NULL;
        }
    }
    
    return pool;
}

void ListFactory::release(ListPool* pool) {
    if (NULL != pool) {
        pool->finish();
        delete pool;
    }
}


class AtomQuePool : public QuePool{
public:
    virtual int init() { return 0; }
    virtual void finish() {}
    virtual bool push(struct deque_root* root, struct hlist* list);
    virtual struct hlist* pop(struct deque_root* root);
};

bool AtomQuePool::push(struct deque_root* root, struct hlist* list) {
    struct hlist* tail = NULL;
    struct hlist* prev = NULL;
    struct hlist* curr = NULL;
    struct hlist* next = NULL;

    list->next = NULL;
    ATOMIC_INC(&root->m_size);
    
    /* two fold level mechanism */
    tail = ACCESS_ONCE(root->tail);
    
    do {
        prev = tail; 
        smp_rmb();
        tail = CMPXCHG(&prev->next, NULL, list);
    } while (NULL != tail);

    smp_wmb();
    
    /* forward a step for tail prt */
    do {
        curr = prev;
        smp_rmb();
        next = curr->next; 
        prev = CMPXCHG(&root->tail, curr, next);
    } while (prev != curr);

    return true;
}

struct hlist* AtomQuePool::pop(struct deque_root* root) {
    struct hlist* tail = NULL;
    struct hlist* head = NULL;
    struct hlist* item = NULL;
    struct hlist* next = NULL;

    head = ACCESS_ONCE(root->head);
    tail = ACCESS_ONCE(root->tail);
    smp_rmb();
    
    while (head != tail && head != item) {
        item = head;
        next = head->next;
        head = CMPXCHG(&root->head, item, next);
    }

    smp_mb();
    if (head != tail) {
        /* pop ok */
        ATOMIC_DEC(&root->m_size);
        return head;
    } else {
        return NULL;
    }
}

class SpinQuePool : public QuePool{
public:
    virtual int init();
    virtual void finish();
    virtual bool push(struct deque_root* root, struct hlist* list);
    virtual struct hlist* pop(struct deque_root* root);

private:
    Lock* m_rlock;
    Lock* m_wlock;
};

int SpinQuePool::init() {
    int ret = 0;

    m_rlock = new SpinLock;
    if (NULL == m_rlock) {
        return -1;
    }

    ret = m_rlock->init();
    if (0 != ret) {
        return ret;
    }

    m_wlock = new SpinLock;
    if (NULL == m_wlock) {
        return -1;
    }

    ret = m_wlock->init();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void SpinQuePool::finish() {
    if (NULL != m_rlock) {
        m_rlock->finish();
        delete m_rlock;
        m_rlock = NULL;
    }

    if (NULL != m_wlock) {
        m_wlock->finish();
        delete m_wlock;
        m_wlock = NULL;
    }
}

bool SpinQuePool::push(struct deque_root* root, struct hlist* list) {
    bool bOk = true;

    list->next = NULL;
    
    bOk = m_wlock->lock(0);
    if (bOk) {
        root->tail->next = list;
        root->tail = list;
        
        m_wlock->unlock(0); 

        ATOMIC_INC(&root->m_size);
    }

    return bOk;
}

struct hlist* SpinQuePool::pop(struct deque_root* root) {
    bool bOk = true;
    struct hlist* tail = NULL;
    struct hlist* item = NULL;

    tail = ACCESS_ONCE(root->tail);
    bOk = m_rlock->lock(0);
    if (bOk) { 
        if (tail != root->head) {
            item = root->head;
            root->head = item->next;
        } 
        
        m_rlock->unlock(0);
    }

    if (NULL != item) {
        ATOMIC_DEC(&root->m_size);
        return item;
    } else {
        return NULL;
    }
}

QuePool* QueFactory::create(enum ListType type) {
    int ret = 0;
    QuePool* pool = NULL;

    switch (type) {
    case ATOM_LIST_TYPE:
        pool = new AtomQuePool;
        break;

    case SPIN_LIST_TYPE:
    default:
        pool = new SpinQuePool;
        break;
    }

    if (NULL != pool) {
        ret = pool->init();
        if (0 == ret) {
            /* create ok and return */
        } else {
            release(pool);
            pool = NULL;
        }
    }
    
    return pool;
}

void QueFactory::release(QuePool* pool) {
    if (NULL != pool) {
        pool->finish();
        delete pool;
    }
}


class AtomGrpQuePool : public GrpQuePool{
public:
    virtual int init() { return 0; }
    virtual void finish() {}
    virtual bool push(int n, struct deque_root* root, struct hlist* list);
};

/* same as atomquepool. push */
bool AtomGrpQuePool::push(int n, struct deque_root* root,
    struct hlist* list) {
    struct hlist* tail = NULL;
    struct hlist* prev = NULL;
    struct hlist* curr = NULL;
    struct hlist* next = NULL;

    list->next = NULL;
    ATOMIC_INC(&root->m_size);
    
    /* two fold level mechanism */
    tail = ACCESS_ONCE(root->tail);
    do {
        prev = tail;
        smp_mb();
        tail = CMPXCHG(&prev->next, NULL, list);
    } while (NULL != tail);

    /* forward a step for tail prt */
    do {
        curr = prev;
        next = curr->next;
        smp_mb();
        prev = CMPXCHG(&root->tail, curr, next);
    } while (prev != curr);

    return true;
}


class SpinGrpQuePool : public GrpQuePool{
public:
    virtual int init();
    virtual void finish();
    virtual bool push(int n, struct deque_root* root, struct hlist* list);

private:
    Lock* m_lock;
};

int SpinGrpQuePool::init() {
    int ret = 0;

    m_lock = new GrpSpinLock(DEF_LOCK_ORDER);
    if (NULL == m_lock) {
        return -1;
    }

    ret = m_lock->init();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void SpinGrpQuePool::finish() {
    if (NULL != m_lock) {
        m_lock->finish();
        delete m_lock;
        m_lock = NULL;
    }
}

bool SpinGrpQuePool::push(int n, struct deque_root* root,
    struct hlist* list) {
    bool bOk = true;

    list->next = NULL;
    
    bOk = m_lock->lock(n);
    if (bOk) {
        root->tail->next = list;
        root->tail = list;
        
        m_lock->unlock(n); 

        ATOMIC_INC(&root->m_size);
    }

    return bOk;
}

GrpQuePool* GrpQueFactory::create(enum ListType type) {
    int ret = 0;
    GrpQuePool* pool = NULL;

    switch (type) {
    case ATOM_LIST_TYPE:
        pool = new AtomGrpQuePool;
        break;

    case SPIN_LIST_TYPE:
    default:
        pool = new SpinGrpQuePool;
        break;
    }

    if (NULL != pool) {
        ret = pool->init();
        if (0 == ret) {
            /* create ok and return */
        } else {
            release(pool);
            pool = NULL;
        }
    }
    
    return pool;
}

void GrpQueFactory::release(GrpQuePool* pool) {
    if (NULL != pool) {
        pool->finish();
        delete pool;
    }
}


