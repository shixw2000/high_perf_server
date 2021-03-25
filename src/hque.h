#ifndef __HQUE_H__
#define __HQUE_H__
#include"hlist.h"


enum ELE_DATA_TYPE {
    ELE_DATA_NONE = 100,
    ELE_DATA_SOCK = 200,
    ELE_DATA_MSG = 300,
    ELE_DATA_BUFFER = 400,
};

struct AtomEle {
    int m_size;
    int m_type;
    unsigned int m_seq;
    unsigned char m_res[4];
    struct hlist m_list;
    void* m_data;
};

class Lock;

class ElePool {
    explicit ElePool(int capacity);
    ~ElePool();

    int init();
    void finish(); 

    friend class MsgBaseQue;

public:
    
    struct AtomEle* alloc(); 
    void release(struct hlist* list);

    void reset(struct AtomEle* ele); 
    
    inline void setType(struct AtomEle* ele, int type) {
        ele->m_type = type;
    }
    
    inline void setData(struct AtomEle* ele, void* data, int len){
        ele->m_data = data;
        ele->m_size = len;
    } 

    inline void* getData(const struct AtomEle* ele) const {
        return ele->m_data;
    }

    inline int getType(const struct AtomEle* ele) const {
        return ele->m_type;
    }

    inline long long getInCnt() const { return m_in_cnt; }
    inline long long getOutCnt() const { return m_out_cnt; }
    inline void incIn() { ++m_in_cnt; }
    inline void incOut() { ++m_out_cnt; }

private:
    void add(struct deque_root* root, struct hlist* ele);
    
private:
    const int m_capacity;
    struct AtomEle* m_pool;
    QuePool* m_oper;
    struct deque_root m_root;
    long long m_in_cnt;
    long long m_out_cnt;
    int m_left;
};


#endif

