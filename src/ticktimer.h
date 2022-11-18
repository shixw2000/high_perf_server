#ifndef __TICKTIMER_H__
#define __TICKTIMER_H__
#include"sysoper.h"
#include"listnode.h"
#include"interfobj.h"
#include"datatype.h"


#define TV_1_BITS 8
#define TV_1_SIZE (1 << TV_1_BITS)
#define TV_1_MASK (TV_1_SIZE-1)
#define TV_2_BITS 8
#define TV_2_SIZE (1 << TV_2_BITS)
#define TV_2_MASK (TV_2_SIZE-1)

struct TimerEle;

class TickTimer { 
public:
    TickTimer();
    ~TickTimer(); 

    Void stop();
    
    inline Uint32 monoTick() const {
        return m_tick;
    }

    inline Uint32 now() const {
        return m_time;
    }

    Void setDealer(I_TimerDealer* dealer) {
        m_dealer = dealer;
    }

    Void addTimer(struct TimerEle* ele);

    Void delTimer(struct TimerEle* ele);

    Void modTimer(struct TimerEle* ele, Uint32 timeout);

    void tick(Uint32 cnt);

private:
    void doTimer(struct TimerEle* ele);

private:
    I_TimerDealer* m_dealer;
    Uint32 m_size;
    Uint32 m_tick;
    Uint32 m_time;
    hlist_head m_tv1[TV_1_SIZE];
    hlist_head m_tv2[TV_2_SIZE];
};

extern void updateTimer(struct TimerEle* ele);

extern void delTimer(struct TimerEle* ele);

#endif

