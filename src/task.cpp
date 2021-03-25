#include"sharedheader.h"
#include"hlist.h"
#include"sockdata.h" 
#include"task.h" 


TimerList::TimerList(int order) : m_order(order),
    m_capacity(0x1<<m_order), 
    m_mask(m_capacity-1) {
    m_timeout_list = NULL; 
    m_cur_pos = 0;
}

TimerList::~TimerList() {
}

int TimerList::init() {
    int ret = 0;

    m_timeout_list = (struct list_node*)calloc(m_capacity, 
        sizeof(struct list_node));
    if (NULL == m_timeout_list) {
        return -1;
    }

    for (int i=0; i<m_capacity; ++i) {
        ListNodeOper::clean_node(&m_timeout_list[i]); 
    }

    m_cur_pos = 0;
    return ret;
}

void TimerList::finish() {
    if (NULL != m_timeout_list) {
        free(m_timeout_list);
        m_timeout_list = NULL;
    }
}

void TimerList::initTimerParam(struct TimeTask* item) {
    memset(item, 0, sizeof(*item));
    ListNodeOper::clean_node(&item->m_node);
}

void TimerList::setTimerParam(struct TimeTask* item, int type,
    int timeout, TimerCallback* cb) {
    item->m_type = type;
    item->m_timeout = timeout;
    item->m_cb = cb;
}

void TimerList::updateTimer(struct TimeTask* item) { 
    int nextpos = 0;
    struct list_node* node = &item->m_node;

    if (0 < item->m_timeout) {
        nextpos = (m_cur_pos + item->m_timeout) & m_mask;
        
        ListNodeOper::delNode(node);
        ListNodeOper::addTail(&m_timeout_list[nextpos], node);
    }
}

void TimerList::delTimer(struct TimeTask* item) { 
    item->m_timeout = 0;
    ListNodeOper::delNode(&item->m_node);
}

void TimerList::step() {
    procTimeout();
    
    ++m_cur_pos;    
    m_cur_pos &= m_mask;
}

void TimerList::procTimeout() {
    bool isEmpty = false;
    struct list_node* head = NULL;
    struct TimeTask* item = NULL;

    isEmpty = ListNodeOper::isEmpty(&m_timeout_list[m_cur_pos]);
    if (!isEmpty) {
        head = ListNodeOper::delall(&m_timeout_list[m_cur_pos]);
        for (struct list_node* next=NULL; NULL != head; head = next) {
            next = head->next;

            ListNodeOper::clean_node(head);
            item = containof(head, struct TimeTask, m_node);
            if (NULL != item->m_cb) { 
                LOG_DEBUG("==proc_timeout| type=%d| timeout=%d| cb=%p|",
                    item->m_type, item->m_timeout, item->m_cb);
                
                item->m_cb->procTimeout(item);
            }
        }
    }
}

