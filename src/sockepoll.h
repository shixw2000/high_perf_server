#ifndef __SOCKEPOLL_H__
#define __SOCKEPOLL_H__
#include"globaltype.h"
#include"listnode.h"


struct NodeBase;
struct epoll_event;
class Director;

class SockEpoll {
public:
    SockEpoll(int capacity, Director* director);
    virtual ~SockEpoll();

    virtual int init();
    virtual void finish();

    int addEvent(Int32 ev_type, NodeBase* data);
    int delEvent(Int32 ev_type, NodeBase* data);
    int waitEvent(Int32 timeout);

    inline Int32 getSize() const {
        return m_size;
    }

private:
    int createEpoll();
    
    int addEpoll(Int32 epfd, int fd, Int32 type, NodeBase* data);
    int delEpoll(Int32 epfd, int fd);
    
    int waitWr();

    Void readFd(NodeBase* data);
    Void writeFd(NodeBase* data);
    Void failFd(NodeBase* data, Uint32 ev);

private:
    const Int32 m_capacity;
    
    struct epoll_event* m_evs;
    Director* m_director;
    NodeBase* m_epfd_node;

    Int32 m_size;
    Int32 m_epfd_rd;
};

#endif

