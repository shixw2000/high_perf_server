#ifndef __MSGPOOL_H__
#define __MSGPOOL_H__
#include"hlist.h"
#include"hque.h"


struct msg_type; 

class MsgQueOper {
public:        
    static int size(struct deque_root* root); 
    static bool verify(const struct msg_type* pMsg);
    static struct msg_type* allocMsg(int len);
    static bool freeMsg(struct msg_type* pMsg);
    static void resetMsg(struct msg_type* pMsg);
    static void refMsg(struct msg_type* pMsg);
};

class CalcListener {
public:
    virtual ~CalcListener() {}
    virtual void calcMsg(int ret, int size) = 0; 
};

/* multiple push, one pop */ 
class MsgBaseQue {
public:
    MsgBaseQue();
    virtual ~MsgBaseQue();

    virtual int init();
    virtual void finish(); 

    int initQue(struct deque_root* root);
    void finishQue(struct deque_root* root);

    bool pushMsg(int n, struct deque_root* root, struct msg_type* pMsg);
    bool pushData(int n, struct deque_root* root, char* data, int len);

    struct msg_type* pop(int n, struct deque_root* root); 

    void releaseData(struct AtomEle* ele);
    void releaseEle(struct hlist* list);
    void deleteAllMsg(int n, struct deque_root* root, CalcListener* cb);

    struct hlist* head(int n, struct deque_root* root);
    struct hlist* tail(int n, struct deque_root* root);
    int setHead(int n, int cnt, struct hlist* list, 
        struct deque_root* root);

protected:
    bool push(int n, struct deque_root* root, AtomEle* ele);

protected:
    ElePool* m_pool;
    GrpQuePool* m_oper;
};


#endif

