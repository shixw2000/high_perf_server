#include<string.h>
#include"globaltype.h"
#include"ticktimer.h"


TickTimer::TickTimer() {
    m_dealer = NULL;
    m_size = 0;
    m_tick = 0;
    m_time = 0;

    memset(m_tv1, 0, sizeof(m_tv1));
    memset(m_tv2, 0, sizeof(m_tv2));
}

TickTimer::~TickTimer() {
}

Void TickTimer::stop() {
    TimerEle* ele = NULL;
    hlist_node* pos = NULL;
    hlist_node* n = NULL; 
    
    for (int i=0; i<TV_1_SIZE; ++i) {
        hlist_for_each_entry_safe(ele, pos, n, &m_tv1[i], m_node) {
            hlist_del(pos);
            --m_size;
        }
    }

    for (int i=0; i<TV_2_SIZE; ++i) {
        hlist_for_each_entry_safe(ele, pos, n, &m_tv2[i], m_node) {
            hlist_del(pos);
            --m_size;
        }
    }
}

Void TickTimer::addTimer(struct TimerEle* ele) {     
    modTimer(ele, ele->m_interval);
}

Void TickTimer::delTimer(struct TimerEle* ele) {
    if (!hlist_unhashed(&ele->m_node)) {
        hlist_del(&ele->m_node);

        if (ele->m_base == this) {
            --m_size;
        }
    }
}

Void TickTimer::modTimer(struct TimerEle* ele, Uint32 timeout) {
    Int32 index = 0;
    
    this->delTimer(ele);

    ele->m_interval = timeout;
    ele->m_expires = m_tick + ele->m_interval;

    if (TV_1_SIZE > timeout) { 
        index = ele->m_expires & TV_1_MASK;
        hlist_add(&ele->m_node, &m_tv1[index]);
    } else { 
        index = (ele->m_expires >> TV_1_BITS) & TV_2_MASK; 
        hlist_add(&ele->m_node, &m_tv2[index]);
    }

    if (ele->m_base != this) {
        ele->m_base = this;
    }
    
    ++m_size;
}

void TickTimer::tick(Uint32 cnt) {
    Int32 i1 = 0;
    Int32 i2 = 0;
    Int32 next = 0;
    hlist_head list;
    TimerEle* ele = NULL;
    hlist_node* pos = NULL;
    hlist_node* n = NULL; 

    ++m_tick;
    m_time += cnt;

    INIT_HLIST_HEAD(&list);
    
    i1 = m_tick & TV_1_MASK;
    
    if (0 == i1) {
        i2 = (m_tick >> TV_1_BITS) & TV_2_MASK;

        if (!hlist_empty(&m_tv2[i2])) { 
            hlist_for_each_entry_safe(ele, pos, n, &m_tv2[i2], m_node) {
                hlist_del(pos);
                
                next = ele->m_expires & TV_1_MASK;
                hlist_add(pos, &m_tv1[next]);
            }
        }
    } 

    if (!hlist_empty(&m_tv1[i1])) {
        hlist_replace(&m_tv1[i1], &list); 
    
        hlist_for_each_entry_safe(ele, pos, n, &list, m_node) {
            hlist_del(pos); 
            --m_size;
            
            doTimer(ele); 
        }
    }
}

Void TickTimer::doTimer(struct TimerEle* ele) {
    m_dealer->doTimeout(ele);
}

void updateTimer(struct TimerEle* ele) {
    if (NULL != ele->m_base) {
        ele->m_base->addTimer(ele);
    }
}

void delTimer(struct TimerEle* ele) {
    if (NULL != ele->m_base) {
        ele->m_base->delTimer(ele);
    }
}

