#ifndef __HLIST_H__
#define __HLIST_H__ 


struct hlist {
    struct hlist* next;
};

struct hlist_root {
    struct hlist head;
};

struct deque_root {
    int m_size;
    struct hlist* head;
    struct hlist* tail;
};

struct list_node {
    struct list_node* prev;
    struct list_node* next;
};


class ListNodeOper {
public:
    static bool isEmpty(struct list_node* root);
    static void clean_node(struct list_node* node);
    static void addTail(struct list_node* root, struct list_node* node);
    static void delNode(struct list_node* node);
    static struct list_node* delall(struct list_node* root);
};

extern void clean(struct deque_root* root);

class ListPool {
public:
    virtual ~ListPool() {}
    virtual int init() = 0;
    virtual void finish() = 0;
    
    virtual bool push(struct hlist_root* root, struct hlist* list) = 0;
    virtual void popall(struct hlist_root* src, struct hlist_root* dst) = 0;
};

class QuePool {
public:
    virtual ~QuePool() {}
    virtual int init() = 0;
    virtual void finish() = 0;
    virtual bool push(struct deque_root* root, struct hlist* list) = 0;
    virtual struct hlist* pop(struct deque_root* root) = 0;
};

class GrpQuePool {
public:
    virtual ~GrpQuePool() {}
    virtual int init() = 0;
    virtual void finish() = 0;
    virtual bool push(int n, struct deque_root* root, struct hlist* list) = 0;
};

enum ListType {
    ATOM_LIST_TYPE,
    SPIN_LIST_TYPE
};

static const int DEF_LOCK_ORDER = 4; 
static const enum ListType DEF_USE_LOCK_TYPE = SPIN_LIST_TYPE;

class ListFactory {
public:
    static ListPool* create(enum ListType type);
    static void release(ListPool* pool);
};

class QueFactory {
public:
    static QuePool* create(enum ListType type);
    static void release(QuePool* pool);
};

class GrpQueFactory {
public:
    static GrpQuePool* create(enum ListType type);
    static void release(GrpQuePool* pool);
};

#endif

