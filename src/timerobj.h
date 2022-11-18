#ifndef __TIMEROBJ_H__
#define __TIMEROBJ_H__
#include"globaltype.h"
#include"interfobj.h"
#include"datatype.h"


class Director;

class TimerObj : public I_NodeObj {
public:
    explicit TimerObj(Director* director) : m_director(director) {}
    
    virtual Int32 getType() const {
        return ENUM_NODE_TIMER;
    }
    
    /* return: 0-ok, other: error */
    virtual Uint32 readNode(struct NodeBase* base);

    /* return: 0-ok, 1-blocking write, other: error */
    virtual Uint32 writeNode(struct NodeBase* base);
    
    virtual Int32 procMsg(struct NodeBase* base, struct MsgHdr* msg);

    virtual Void procCmd(struct NodeBase* base, struct MsgHdr* msg);

    /* this function run in the dealer thread for resource free */
    virtual Void eof(struct NodeBase* base);

private:
    Void procTickTimer(EnumThreadType type, struct MsgHdr* msg);

private:
    Director* m_director;
};

class EventObj : public I_NodeObj {
public:
    explicit EventObj(Director* director) : m_director(director) {}
    
    virtual Int32 getType() const {
        return ENUM_NODE_EVENT;
    }
    
    /* return: 0-ok, other: error */
    virtual Uint32 readNode(struct NodeBase* base);

    /* return: 0-ok, 1-blocking write, other: error */
    virtual Uint32 writeNode(struct NodeBase* base);
    
    virtual Int32 procMsg(struct NodeBase* base, struct MsgHdr* msg);

    virtual Void procCmd(struct NodeBase* base, struct MsgHdr* msg);

    /* this function run in the dealer thread for resource free */
    virtual Void eof(struct NodeBase* base);

private:
    Void procAddress(struct NodeBase* base, struct MsgHdr* msg); 

private:
    Director* m_director;
};

#endif


