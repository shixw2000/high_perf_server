#include<sys/epoll.h>
#include"sharedheader.h"
#include"socktool.h"
#include"sockpool.h"


SockPool::SockPool(int capacity) : m_capacity(capacity) {
    epfdwr = epfdrd = -1;
}

SockPool::~SockPool() {
}

int SockPool::init() {
    int ret = 0;

    epfdrd = SockTool::createEpoll(m_capacity);
    if (0 > epfdrd) {
        return -1;
    }

    epfdwr = SockTool::createEpoll(m_capacity);
    if (0 > epfdrd) {
        return -1;
    }

    LOG_DEBUG("eprd=%d| epwr=%d| msg=create epoll ok|", epfdrd, epfdwr); 
	return ret;
}

void SockPool::finish() {
    if (0 <= epfdwr) {
		SockTool::closeFd(epfdwr);
		epfdwr = -1;
	}
    
	if (0 <= epfdrd) {
		SockTool::closeFd(epfdrd);
		epfdrd = -1;
	}
}

int SockPool::addEvent(int fd, int type, void* data) {
    int ret = 0;

    switch (type) {
    case SOCK_READ:
        ret = SockTool::addEpoll(type, epfdrd, fd, data);
        break;

    case SOCK_WRITE:
        ret = SockTool::addEpoll(type, epfdwr, fd, data);
        break;

    default:
        LOG_WARN("add_sock| type=%d| fd=%d| data=%p| error=invalid type|",
            type, fd, data);
        ret = -1;
        break;
    }
    
	return ret;
}

int SockPool::delEvent(int fd, int type, void* data) {
    int ret = 0;

    switch (type) {
    case SOCK_READ:
        ret = SockTool::delEpoll(type, epfdrd, fd, data);
        break;

    case SOCK_WRITE:
        ret = SockTool::delEpoll(type, epfdwr, fd, data);
        break;

    default:
        LOG_WARN("delete_sock| type=%d| fd=%d| data=%p| error=invalid type|",
            type, fd, data);
        ret = -1;
        break;
    }
    
	return ret;
}

int SockPool::chkRdEvent() {
	int ret = 0;
    static struct epoll_event ev[EPOLL_EV_SIZE];

    ret = SockTool::waitEpoll(epfdrd, ev, EPOLL_EV_SIZE, 1000);
    if (0 < ret) {
        for (int i=0; i<ret; ++i) {
            if (SockTool::hasEpollRd(&ev[i])) {
                dealEvent(SOCK_READ, ev[i].data.ptr);
            }

            if (SockTool::hasEpollExcpt(&ev[i])) {
                dealEvent(SOCK_EXCEPTION, ev[i].data.ptr);
            }
        }
    }
    
	return ret;
}

int SockPool::chkWrEvent() {
	int ret = 0;
    static struct epoll_event ev[EPOLL_EV_SIZE];
    
    ret = SockTool::waitEpoll(epfdwr, ev, EPOLL_EV_SIZE, 0);
    if (0 < ret) {
        for (int i=0; i<ret; ++i) {
            if (SockTool::hasEpollWr(&ev[i])) {
                dealEvent(SOCK_WRITE, ev[i].data.ptr);
            }

            if (SockTool::hasEpollExcpt(&ev[i])) {
                dealEvent(SOCK_EXCEPTION, ev[i].data.ptr);
            }
        }
    }
    
	return ret;
}

