#ifndef __TASK_H__
#define __TASK_H__


struct TimeTask;
struct list_node;

enum SockTimerType {
    SOCK_TIMER_TIMEOUT = 1,
    SOCK_TIMER_CONNECT_WR,
    SOCK_TIMER_FLOWCTL,
};

class TimerCallback {
public:
    virtual ~TimerCallback() {}
    virtual void procTimeout(struct TimeTask* item) = 0;
};

class TimerList {
public:
    explicit TimerList(int order);
    ~TimerList();
    
    int init();
    void finish(); 
    
    void updateTimer(struct TimeTask* item);
    void delTimer(struct TimeTask* item);

    void step();

    static void initTimerParam(struct TimeTask* item);

    static void setTimerParam(struct TimeTask* item, int type,
        int timeout, TimerCallback* cb);
    
private: 
    void procTimeout();
    
private:
    const int m_order;
    const int m_capacity;
    const int m_mask;
    int m_cur_pos;
    struct list_node* m_timeout_list;
};

#endif

