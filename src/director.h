#ifndef __DIRECTOR_H__
#define __DIRECTOR_H__
#include"interfobj.h"
#include"taskpool.h"
#include"datatype.h"


class TickTimer;
class SockEpoll;
class Receiver;
class Sender;
class Dealer;
class Lock;
class ObjCenter;
class Parser;

class Director : public TaskThread, public I_TimerDealer {
public:
    explicit Director(Int32 capacity);
    virtual ~Director();

    Int32 init();
    Void finish(); 

    int start();
    virtual void join();
    virtual void stop(); 

    inline Void setSockCb(SockCb* cb) {
        m_cb = cb;
    }

    inline SockCb* getCb() {
        return m_cb;
    }

    inline Int32 capacity() {
        return m_capacity;
    }
    
    Int32 getSize() const;

    Int32 sendCmd(MsgHdr* msg);
    Int32 sendMsg(NodeBase* base, MsgHdr* msg);
    Int32 dispatch(NodeBase* base, MsgHdr* msg);

    Uint32 getTick() const;
    Uint32 getTime() const;
    Void procTick(EnumThreadType type, Uint32 cnt);
    Void notify(EnumThreadType type, NodeBase* base, Uint16 cmd, Uint64 data);
      
    NodeBase* creatNode(Int32 fd, Int32 type, Int32 status);
    NodeBase* creatNode(Int32 fd, Int32 type, Int32 status,
        const Char* ip, Int32 port); 
    
    int delNode(NodeBase* node);
    int addEvent(Int32 ev_type, NodeBase* node);
    int delEvent(Int32 ev_type, NodeBase* node);

    Void close(NodeBase* base); 
    
    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task); 

    virtual Void doTimeout(struct TimerEle* ele);
    
    void addTimer(struct TimerEle* ele, Int32 type, Uint32 interval);

    Bool lock(NodeBase* base);
    Bool unlock(NodeBase* base);

    Void addReadable(struct NodeBase* base);
    Void addWriteable(struct NodeBase* base);

    Uint32 readNode(struct NodeBase* base);
    Uint32 writeNode(struct NodeBase* base);
    Void dealMsg(struct NodeBase* base, struct MsgHdr* msg);



    Int32 addListener(TcpParam* param);
    Int32 addConnector(TcpParam* param);
    Int32 addListener(const Char ip[], int port);
    Int32 addConnector(const Char ip[], int port);

protected: 
    virtual int setup();
    virtual void teardown();

    virtual void check();
    virtual void wait();
    virtual void alarm();

private:
    Void doTick(Uint32 cnt);

    Int32 pushMsg(EnumThreadType type, NodeBase* base, MsgHdr* msg);
    Int32 pushWrite(NodeBase* base, MsgHdr* msg);
    Int32 pushDeal(NodeBase* base, MsgHdr* msg);
    Int32 pushCmd(NodeBase* base, MsgHdr* msg);
        
    Void procNodeQue(NodeBase* base);
    Void dealCmd(NodeBase* base, MsgHdr* msg);
    Void procStopMsg(NodeBase* base, MsgHdr* msg);
    
    Int32 creatTimer();
    Int32 creatEvent();    

    Void freeNodes();

    
private:
    const Int32 m_capacity;

    list_head m_node_que;
    
    Lock* m_lock;
    TickTimer* m_timer;
    SockEpoll* m_epoll;
    Receiver* m_receiver;
    Sender* m_sender;
    Dealer* m_dealer;

    NodeBase* m_event_node;
    NodeBase* m_timer_node; 
    ObjCenter* m_obj; 
    
    SockCb* m_cb;
};

#endif

