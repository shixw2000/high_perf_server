#ifndef __STATEMACHINE_H__
#define __STATEMACHINE_H__
#include"sockdata.h"


class BitTool {
public:
    static unsigned int set(unsigned int n, unsigned int* pstate); // |= n
    static unsigned int clear(unsigned int n, unsigned int* pstate); // &= ~n
    static unsigned int reset(unsigned int n, unsigned int* pstate); // = n

    static bool isset(unsigned int n, unsigned int state);
    
    static bool cas(unsigned int oldval, 
        unsigned int newval, unsigned int* pstate); 
};

class BitDoor {    
    struct STBitMask {
        int m_type;
        unsigned int m_low;
        unsigned int m_high;
        unsigned int m_mask; 
    };
        
public:
    BitDoor();
    ~BitDoor(); 

    /* used befor checkout */
    unsigned int lockDoor(unsigned int* pstate) const;

    /* return true if it can in */
    bool checkin(unsigned int ev, unsigned int* pstate) const; 

    /* return true if it can out */
    bool checkout(unsigned int ev, unsigned int* pstate) const;
    
    bool isValid(unsigned int ev) const;

    bool isEventSet(unsigned int ev, unsigned int state) const;

    int registerEvent(unsigned int ev, int type);

private:
    void initEvent();

private:
    STBitMask m_masks[WORK_EVENT_BUTT];
};

class BinaryDoor : private BitDoor {
public:
    BinaryDoor();
    
    /* used befor action */
    void lock();

    /* return true if it is the first one */
    bool login();

    /* return true if it is the last one */
    bool logout();

private:
    unsigned int m_state;
};

#endif

