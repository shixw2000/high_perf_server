#include<sys/epoll.h>
#include"sockepoll.h"
#include"datatype.h"
#include"ticktimer.h"
#include"sockutil.h"
#include"director.h"


SockEpoll::SockEpoll(int capacity, Director* director) 
    : m_capacity(capacity), m_director(director) { 
    m_evs = NULL;
    m_epfd_node = NULL; 
    
    m_size = 0;
    m_epfd_rd = -1; 
}

SockEpoll::~SockEpoll() {
}

int SockEpoll::init() {
    Int32 ret = 0;
    
    ARR_NEW(struct epoll_event, m_capacity, m_evs);
    memset(m_evs, 0, sizeof(struct epoll_event) * m_capacity); 

    ret = createEpoll();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void SockEpoll::finish() {     
    if (NULL != m_epfd_node) {
        freeNode(m_epfd_node);
        m_epfd_node = NULL;
    }

    if (0 <= m_epfd_rd) {
        closeHd(m_epfd_rd);
        m_epfd_rd = -1;
    }

    if (NULL != m_evs) {
        ARR_FREE(m_evs);
    }
}

int SockEpoll::createEpoll() {
    Int32 ret = 0;
    int fd = -1;

    do {
        fd = epoll_create(m_capacity);
    	if (0 <= fd) {        
            m_epfd_rd = fd;;
    	} else {
    		LOG_ERROR("epoll_create_rd| capacity=%d| err=%s|", 
                m_capacity, ERR_MSG());
    		ret = -1;
            break;
    	}

        fd = epoll_create(m_capacity);
    	if (0 <= fd) {        
            m_epfd_node = buildNode(fd, ENUM_NODE_EPOLL_WR);
            
            addEvent(EVENT_TYPE_RD, m_epfd_node);
            
    	} else {
    		LOG_ERROR("epoll_create_wr| capacity=%d| err=%s|", 
                m_capacity, ERR_MSG());
    		ret = -1;
            break;
    	}        
        
        ret = 0;
    } while (0);

    return ret;
}

int SockEpoll::addEvent(Int32 ev_type, NodeBase* data) {
    Int32 ret = 0;
    
    if ((EVENT_TYPE_RD & ev_type) && !(EVENT_TYPE_RD & data->m_ev_type)) {
        ret = addEpoll(m_epfd_rd, data->m_fd, EVENT_TYPE_RD, data);
        if (0 == ret) {
            if (0 == data->m_ev_type) {
                ++m_size;
            }
            
            data->m_ev_type |= EVENT_TYPE_RD;
        }
    }

    if ((EVENT_TYPE_WR & ev_type) && !(EVENT_TYPE_WR & data->m_ev_type)) {
        ret = addEpoll(m_epfd_node->m_fd, data->m_fd, EVENT_TYPE_WR, data); 
        if (0 == ret) {
            if (0 == data->m_ev_type) {
                ++m_size;
            }
            
            data->m_ev_type |= EVENT_TYPE_WR;
        }
    } 

    return 0;
}

int SockEpoll::delEvent(Int32 ev_type, NodeBase* data) {
    if ((EVENT_TYPE_RD & ev_type) && (EVENT_TYPE_RD & data->m_ev_type)) {
        delEpoll(m_epfd_rd, data->m_fd);

        data->m_ev_type &= ~EVENT_TYPE_RD;

        if (0 == data->m_ev_type) {
            --m_size;
        }
    }

    if ((EVENT_TYPE_WR & ev_type) && (EVENT_TYPE_WR & data->m_ev_type)) {
        delEpoll(m_epfd_node->m_fd, data->m_fd);

        data->m_ev_type &= ~EVENT_TYPE_WR;

        if (0 == data->m_ev_type) {
            --m_size;
        }
    }

    return 0;
}

int SockEpoll::addEpoll(Int32 epfd, int fd, Int32 type, NodeBase* data) {
    int ret = 0;
    struct epoll_event ev; 
    
    ev.data.ptr = data;
    ev.events = 0;
    
    if (EVENT_TYPE_RD == type) {
        ev.events = EPOLLET | EPOLLIN;
    } else {
        ev.events = EPOLLET | EPOLLOUT;
    } 
    
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    if (0 == ret) {        
        LOG_DEBUG("add_epoll| epfd=%d| fd=%d|"
            " data=%p| type=%d| msg=ok|", 
            epfd, fd, data, type);
    } else {
        LOG_ERROR("add_epoll| epfd=%d| fd=%d|"
            " data=%p| type=%d| error=%s|", 
            epfd, fd, data, type, ERR_MSG());
	}

	return ret;
}

int SockEpoll::delEpoll(Int32 epfd, int fd) {
	int ret = 0;

    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    if (0 == ret) {         
        LOG_DEBUG("del_epoll| epfd=%d| fd=%d| msg=ok|", 
            epfd, fd);
    } else {
        LOG_ERROR("del_epoll| epfd=%d| fd=%d| error=%s|",
            epfd, fd, ERR_MSG());
	}

	return ret;
}

int SockEpoll::waitEvent(Int32 timeout) {    
    static const Uint32 ERR_FLAG = EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    int ret = 0;
    Bool need_wr = FALSE;
    NodeBase* data = NULL;
    
    ret = epoll_wait(m_epfd_rd, m_evs, m_size, timeout);
    if (0 < ret) {
        LOG_DEBUG("wait_epoll_rd| epfd=%d| size=%d| timeout=%d|"
            " ret=%d| msg=ok|", 
            m_epfd_rd, m_size, timeout, ret);
        
        for (int i=0; i<ret; ++i) {
            data = (NodeBase*)m_evs[i].data.ptr;
            
            if (EPOLLIN & m_evs[i].events) {
                if (ENUM_NODE_EPOLL_WR != data->m_node_type) {
                    readFd(data); 
                } else {
                    need_wr = TRUE;
                }
            }

            if (ERR_FLAG & m_evs[i].events) {
                if (ENUM_NODE_EPOLL_WR != data->m_node_type) {
                    failFd(data, m_evs[i].events);
                } else {
                    need_wr = TRUE;
                } 
            }
        } 

        if (need_wr) {
            ret = waitWr();
        } else {
            ret = 0;
        }
    } else if (0 == ret || EINTR == errno) { 
		ret = 0;
	} else {
	    LOG_ERROR("wait_epoll| epfd=%d| size=%d| timeout=%d|"
            " ret=%d| error=%s|", 
            m_epfd_rd, m_size, timeout, ret, ERR_MSG());
	}

    return ret;
}

int SockEpoll::waitWr() {    
    static const Uint32 ERR_FLAG = EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    int ret = 0;
    NodeBase* data = NULL;
    
    ret = epoll_wait(m_epfd_node->m_fd, m_evs, m_size, 0);
    if (0 < ret) {
        LOG_DEBUG("wait_epoll_wr| epfd=%d| size=%d|"
            " ret=%d| msg=ok|", 
            m_epfd_node->m_fd, m_size, ret);
        
        for (int i=0; i<ret; ++i) {
            data = (NodeBase*)m_evs[i].data.ptr;
            
            if (EPOLLOUT & m_evs[i].events) {
                writeFd(data); 
            }

            if (ERR_FLAG & m_evs[i].events) {
                failFd(data, m_evs[i].events);
            }
        } 

        ret = 0;
    } else if (0 == ret || EINTR == errno) { 
		ret = 0;
	} else {
	    LOG_ERROR("wait_epoll_wr| epfd=%d| size=%d|"
            " ret=%d| error=%s|", 
            m_epfd_node->m_fd, m_size, ret, ERR_MSG());
	}

    return ret;
}

Void SockEpoll::readFd(NodeBase* data) {    
    LOG_DEBUG("sock_read| fd=%d|", data->m_fd);

    m_director->addReadable(data);
}

Void SockEpoll::writeFd(NodeBase* data) {
    LOG_DEBUG("sock_write| fd=%d|", data->m_fd);

    m_director->addWriteable(data);
}

Void SockEpoll::failFd(NodeBase* data, Uint32 ev) {
    LOG_ERROR("sock_err| ev=0x%x| fd=%d|", ev, data->m_fd);

    data->m_rd_err = TRUE;
    data->m_wr_err = TRUE;
    
    m_director->close(data);
}

