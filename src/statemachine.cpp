#include"sharedheader.h"
#include"sockdata.h"
#include"statemachine.h"


static const unsigned int WORK_HIGH_FLAG_SHIFT_BIT = 16;

unsigned int BitTool::set(unsigned int n, unsigned int* pstate) {
    unsigned int oldval = 0U;
    unsigned int newval = 0U;
    unsigned int retval = 0U;

    retval = *pstate;

    while (n != (retval & n)) {
        oldval = retval;
        newval = (retval | n);
        retval = CMPXCHG(pstate, oldval, newval);
        if (retval == oldval) {
            /* set ok */
            break;
        }
    } 

    return retval;
}

unsigned int BitTool::clear(unsigned int n, unsigned int* pstate) {
    unsigned int oldval = 0U;
    unsigned int newval = 0U;
    unsigned int retval = 0U;
    unsigned int mask = ~n;

    retval = *pstate; 

    while (0 != (retval & n)) {
        oldval = retval;
        newval = (retval & mask);
        retval = CMPXCHG(pstate, oldval, newval);
        if (retval == oldval) {
            /* set ok */
            break;
        }
    } 

    return retval;
}

unsigned int BitTool::reset(unsigned int n, unsigned int* pstate) {
    unsigned int state = 0U;
    
    state = ATOMIC_SET(pstate, n);
    return state;
}

bool BitTool::isset(unsigned int n, unsigned int state) {
    return (n == (n & state));
}

bool BitTool::cas(unsigned int oldval, 
    unsigned int newval, unsigned int* pstate) {
    bool bOk = true;

    bOk = CAS(pstate, oldval, newval);
    return bOk;
}

#define isCustom(n) (WORK_TYPE_CUSTOMER == m_masks[n].m_type)
#define isWorkLoop(n) (WORK_TYPE_LOOP == m_masks[n].m_type)
#define isWorkOnce(n) (WORK_TYPE_ONCE == m_masks[n].m_type)
#define isUnlock(val) (0U  == val)
#define hasDoor(n, val) (m_masks[n].m_high == ((val) & m_masks[n].m_mask))
#define hasWaiter(n, val) (m_masks[n].m_low == ((val) & m_masks[n].m_mask))


BinaryDoor::BinaryDoor() {
    m_state = 0U;
    
    registerEvent(WORK_EVENT_IN, WORK_TYPE_LOOP);
    registerEvent(WORK_EVENT_OUT, WORK_TYPE_ONCE);
}

bool BinaryDoor::login() {
    bool needFire = false;

    needFire = checkin(WORK_EVENT_IN, &m_state); 
    return needFire;
}

void BinaryDoor::lock() {    
    lockDoor(&m_state);
}

bool BinaryDoor::logout() {
    bool needPause = true;
    
    needPause = checkout(WORK_EVENT_OUT, &m_state);
    return needPause;
}

BitDoor::BitDoor() {
    initEvent();
}

BitDoor::~BitDoor() {
}

bool BitDoor::isEventSet(unsigned int ev, unsigned int state) const {
    bool bSet = false;

    bSet = BitTool::isset(m_masks[ev].m_low, state); 
    return bSet;
}

bool BitDoor::checkin(unsigned int ev, unsigned int* pstate) const {
    bool canIn = false;
    unsigned int state = 0U;

    state = BitTool::set(m_masks[ev].m_low, pstate);
    if (isUnlock(state)) {
        canIn = true;
    } else if (isCustom(ev) && hasDoor(ev, state)) {
         canIn = true;
    } else {
        canIn = false;
    }
    
    return canIn;
}

unsigned int BitDoor::lockDoor(unsigned int* pstate) const {
    unsigned int state = 0U;
    
    state = BitTool::reset(m_masks[WORK_EVENT_LOCK].m_high, pstate);
    return state;
}

bool BitDoor::checkout(unsigned int ev, unsigned int* pstate) const {
    bool canOut = true;
    unsigned int state = 0U;

    if (isWorkOnce(ev)) {
        canOut = BitTool::cas(m_masks[WORK_EVENT_LOCK].m_high, 0, pstate);
    } else if (isWorkLoop(ev)) {
        canOut = false;
    } else if (isCustom(ev)) {
        state = BitTool::set(m_masks[ev].m_high, pstate);
        if (!hasWaiter(ev, state)) {
            canOut = true;
        } else {
            canOut = false;
        }
    } else {
        LOG_WARN("checkout| ev=0x%x| error=invalid event|", ev);
        
        /*error */
        canOut = true;
    } 
  
    return canOut;
}

bool BitDoor::isValid(unsigned int ev) const {
    return ev < (unsigned int)WORK_EVENT_BUTT 
        && m_masks[ev].m_type != WORK_TYPE_INVALID;
}

void BitDoor::initEvent() {
    for (int ev=0; ev<WORK_EVENT_BUTT; ++ev) { 
        m_masks[ev].m_type = WORK_TYPE_INVALID;
        m_masks[ev].m_low = (0x1 << ev);
        m_masks[ev].m_high = (m_masks[ev].m_low << WORK_HIGH_FLAG_SHIFT_BIT);
        m_masks[ev].m_mask = (m_masks[ev].m_low | m_masks[ev].m_high); 
    } 

    /* lock for system used */
    m_masks[WORK_EVENT_LOCK].m_type = WORK_TYPE_CUSTOMER;
}

int BitDoor::registerEvent(unsigned int ev, int type) {    
    /* ev must less than 15 */
    if ((unsigned int)WORK_EVENT_BUTT > ev 
        && WORK_TYPE_INVALID == m_masks[ev].m_type) {
        m_masks[ev].m_type = type;
        
        return 0;
    } else {
        return -1;
    }
}

