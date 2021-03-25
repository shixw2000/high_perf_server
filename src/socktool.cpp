#include<sys/types.h> 
#include<sys/socket.h>
#include<netdb.h>
#include<unistd.h> 
#include<string.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/uio.h>
#include<sys/un.h>
#include<sys/timerfd.h>
#include<sys/eventfd.h>
#include<poll.h>
#include<sys/epoll.h> 
#include<netinet/in.h>
#include<netinet/tcp.h>
#include"sharedheader.h"
#include"sockconf.h"
#include"socktool.h"


static const unsigned long long DEF_EVENT_VAL = 1ULL;
static const int evsize = sizeof(DEF_EVENT_VAL);

int SockTool::createInetTcp() {
    int fd = -1;

    fd = createSock(AF_INET, SOCK_STREAM, 0);
    return fd;
}

int SockTool::createUnixTcp(const char name[], const char port[]) {
    int fd = -1;
	int ret = 0;
	int len = 0;
	struct sockaddr_storage addr;

    len = sizeof(addr);
	memset(&addr, 0, len); 
	ret = createAddrUnix(name, port, &addr, &len);
	if (0 != ret) {
		return -1;
	}

    fd = createSock(AF_UNIX, SOCK_STREAM, 0);
    if (0 > fd) {
        return fd;
    }
		
	ret = bindAddr(fd, (struct sockaddr*)&addr, len);
	if (0 != ret) {
		closeFd(fd);
    	return -1;
	}
    
    return fd;
}

int SockTool::createAddrUnix(const char name[], const char port[], 
	void* addr, int* plen) {
	struct sockaddr_un* pAddr = NULL;

    pAddr = (struct sockaddr_un*)addr;
    memset(pAddr, 0, *plen);
    pAddr->sun_family = AF_UNIX;
    strcpy(pAddr->sun_path, name);
    strcat(pAddr->sun_path, ":");
    strcat(pAddr->sun_path, port);
    
    *plen = sizeof(pAddr->sun_family) + strlen(pAddr->sun_path) + 1;
	return 0;
}

int SockTool::parseAddrUnix(const void* sa, int len,
	char name[], char port[]) {
	const struct sockaddr_un* pAddr = NULL;

	pAddr = (const struct sockaddr_un*)sa;
    strcpy(name, pAddr->sun_path);
    port[0] = '\0';
    return 0;
}

int SockTool::createSvrUnix(const char name[], const char port[]) {
	int fd = -1;
	int ret = 0;

    fd = createUnixTcp(name, port);
	if (0 > fd) {
		return -1;
	}

	setNonBlock(fd);
    setSockBuffer(fd);
    ret = listenSrv(fd, MAX_LISTEN_SRV_SIZE);
	if (0 == ret) {
        return fd;
	} else {
		closeFd(fd);
    	return -1;
	}
}

int SockTool::parseAddr(const void* sa, int len,
	char name[], char port[]) {
	int ret = 0;
	const struct sockaddr* addr = NULL;

	addr = (const struct sockaddr*)sa;
    if (addr->sa_family == AF_INET) {
        ret = parseAddrInet(sa, len, name, port);
    } else if (addr->sa_family == AF_UNIX) {
        ret = parseAddrUnix(sa, len, name, port);
    } else {
        LOG_ERROR("sa_family=%d| error=invalid socket family|", 
            addr->sa_family);
        ret = -1; 
    }

    return ret;
}

int SockTool::createAddrInet(const char name[], const char port[], 
	void* addr, int* len) {
	int ret = 0;
	struct addrinfo hints;
	struct addrinfo *result = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
	ret = getaddrinfo(name, port, &hints, &result);
	if (0 != ret) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	*len = result->ai_addrlen;
	memcpy(addr, result->ai_addr, *len);
	freeaddrinfo(result);
	return 0;
}

int SockTool::parseAddrInet(const void* sa, int len,
	char name[], char port[]) {
	int ret = 0;
	const struct sockaddr* addr = NULL;

	addr = (const struct sockaddr*)sa;
	ret = getnameinfo(addr, len, name, 32, port, 32, 
		NI_NUMERICHOST|NI_NUMERICSERV);
	if (0 == ret) {
		return 0;
	} else {
		fprintf(stderr, "getnameinfo err:%s\n", 
			gai_strerror(ret));
		return -1;
	}
}

void SockTool::setNoTimewait(int fd) {
    int ret = 0;
    struct linger val = { .l_onoff=1, .l_linger=0 };
	int len = sizeof(val);
    
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, &val, len);
    if (0 == ret) {
        LOG_DEBUG("set_no_timewait| fd=%d|", fd);
    } else {
        LOG_WARN("set_no_timewait| fd=%d| ret=%d| error=%s|", 
            fd, ret, ERR_MSG);
    }
}

void SockTool::setNoDelay(int fd) {
    int ret = 0;
    int val = 1;
	int len = sizeof(val);
    
    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, len);
    if (0 == ret) {
        LOG_DEBUG("set_no_delay| fd=%d|", fd);
    } else {
        LOG_WARN("set_no_delay| fd=%d| ret=%d| error=%s|", 
            fd, ret, ERR_MSG);
    }
}

void SockTool::setCork(int fd) {
    int ret = 0;
    int val = 1;
	int len = sizeof(val);
    
    ret = setsockopt(fd, IPPROTO_TCP, TCP_CORK, &val, len);
    if (0 == ret) {
        LOG_DEBUG("set_tcp_cork| fd=%d|", fd);
    } else {
        LOG_WARN("set_tcp_cork| fd=%d| ret=%d| error=%s|", 
            fd, ret, ERR_MSG);
    }
}

void SockTool::setSockOption(int fd) {
    setNoTimewait(fd);
    //setNoDelay(fd);
}

void SockTool::setSockBuffer(int fd) {
    /* socket buffer must be set before listen or accept */
    SockTool::getSndBufferSize(fd);
    SockTool::setSndBufferSize(fd, MAX_SOCKET_BUFFER_SIZE);
    SockTool::getSndBufferSize(fd);
    
    SockTool::getRcvBufferSize(fd);
    SockTool::setRcvBufferSize(fd, MAX_SOCKET_BUFFER_SIZE);
    SockTool::getRcvBufferSize(fd);
}

void SockTool::setReuse(int fd) {
    int ret = 0;
    int val = 1;
	int len = sizeof(val);
    
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, len);
    if (0 == ret) {
        LOG_DEBUG("set_reuse| fd=%d|", fd);
    } else {
        LOG_WARN("set_reuse| fd=%d| ret=%d| error=%s|", 
            fd, ret, ERR_MSG);
    }
}

int SockTool::createSock(int family, int type, int proto) {
    int fd = -1;
    
    fd = socket(family, type, proto);
	if (0 <= fd) {        
        return fd;
	} else {
		LOG_ERROR("create_socket| family=%d| type=%d| proto=%d|"
            " ret=%d| error=%s|", 
            family, type, proto, fd, ERR_MSG);
		return SOCK_CREATE_FD_ERR;
	}
}

int SockTool::bindAddr(int fd, struct sockaddr* addr, int len) {
    int ret = 0;

    ret = bind(fd, addr, len);
	if (0 == ret) {
        return 0;
	} else {
		LOG_ERROR("bind_addr| fd=%d| ret=%d| err=%s|", 
            fd, ret, ERR_MSG);
		return SOCK_BIND_ADDR_ERR;
	}
}

int SockTool::listenSrv(int fd, int size) {
    int ret = 0;

    ret = listen(fd, size);
	if (0 == ret) {
        return 0;
	} else {
		LOG_ERROR("listen_srv| fd=%d| ret=%d| err=%s|", 
            fd, ret, ERR_MSG);
		return SOCK_LISTEN_SRV_ERR;
	}
}

int SockTool::connAddr(int fd, struct sockaddr* addr, int len) {
    int ret = 0;
    int errcode = 0;

    setSockOption(fd);
    setSockBuffer(fd);
    setNonBlock(fd);
    
    ret = connect(fd, addr, len);
	if (0 == ret) {
        return 0;
	} else {
	    errcode = errno;
    	if (EINPROGRESS == errcode) {
            ret = SOCK_CONN_SRV_PROGRESS;
        } else if (EINTR == errcode) {
            LOG_DEBUG("connect_srv| fd=%d| ret=%d| err=interrupted|", 
                fd, ret);
            
            ret = COMMON_ERR_INTR;
    	} else {
    		LOG_ERROR("connect_srv| fd=%d| ret=%d| err=%s|", 
                fd, ret, strerror(errcode));

            ret = SOCK_CONN_SRV_ERR;
    	}

        return ret;
	}
}

int SockTool::connSrvInet(int fd, const char name[],
    const char port[]) {
	int ret = 0;
	int len = 0;
	struct sockaddr_storage addr;

    len = sizeof(addr);
	memset(&addr, 0, len); 
	ret = createAddrInet(name, port, &addr, &len);
	if (0 != ret) {
		return -1;
	}

    ret = connAddr(fd, (struct sockaddr*)&addr, len);
    return ret;
}

int SockTool::connSrvUnix(int fd, const char name[], 
    const char port[]) {
	int ret = 0;
	int len = 0;
	struct sockaddr_storage addr;

    len = sizeof(addr);
	memset(&addr, 0, len); 
	ret = createAddrUnix(name, port, &addr, &len);
	if (0 != ret) {
		return -1;
	}

    ret = connAddr(fd, (struct sockaddr*)&addr, len);
    return ret;
}

int SockTool::setNonBlock(int fd) {
    int ret = 0;

    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (0 == ret) {
        return 0;
	} else {
		LOG_ERROR("set_nonblock| fd=%d| ret=%d| err=%s|", 
            fd, ret, ERR_MSG);
		return SOCK_SET_FLAG_ERR;
	}
}

int SockTool::createSvrInet(const char name[], const char port[]) {
	int fd = -1;
	int ret = 0;
	int len = 0;
	struct sockaddr_storage addr;

    len = sizeof(addr);
	memset(&addr, 0, len); 
	ret = createAddrInet(name, port, &addr, &len);
	if (0 != ret) {
		return -1;
	}
	
	fd = createInetTcp();
	if (0 > fd) {
		return fd;
	}
	
	setNonBlock(fd);
    setReuse(fd); 
    setSockBuffer(fd);
    
	ret = bindAddr(fd, (struct sockaddr*)&addr, len);
	if (0 != ret) {
		closeFd(fd);
    	return -1;
	}

	ret = listenSrv(fd, MAX_LISTEN_SRV_SIZE);
	if (0 != ret) {
		closeFd(fd);
    	return -1;
	}

	return fd; 
}

int SockTool::waitPoll(struct pollfd* fds, int size, int timeout) {
    int ret = 0;
    int ready = 0;

    ready = poll(fds, size, timeout);
    if (0 < ready) {
        LOG_VERB("wait_poll| size=%d| ready=%d| timeout=%d| msg=ok|", 
            size, ready, timeout);

        ret = ready;
    } else if (0 == ready || EINTR == errno) {
        LOG_VERB("wait_poll| size=%d| ready=%d| timeout=%d|", 
            size, ready, timeout);
        
		ret = 0;
	} else {
	    LOG_WARN("wait_poll| size=%d| ready=%d| timeout=%d| error=%s|", 
            size, ready, timeout, ERR_MSG);
		ret = SOCK_WAIT_POLL_ERR;
	}

    return ret;
}

bool SockTool::hasPollRd(const struct pollfd* ev) {
    if (POLLIN & ev->revents) {
        return true;
	} else {
        return false;
	}
}

bool SockTool::hasPollWr(const struct pollfd* ev) {
    if (POLLOUT & ev->revents) {
        return true;
	} else {
        return false;
	}
}

bool SockTool::hasPollErr(const struct pollfd* ev) {
    static const int EV_POLL_ERROR = POLLERR|POLLHUP|POLLNVAL;
    
    if (EV_POLL_ERROR & ev->revents) {
        return true;
	} else {
        return false;
	}
}

int SockTool::pollSock(int fd, int ev, int timeout) {
	int ret = 0;
	struct pollfd fds;

	fds.fd = fd;
	fds.revents = 0;
	switch (ev) {
	case SOCK_READ:
		fds.events = POLLIN;
		break;

	case SOCK_WRITE:
		fds.events = POLLOUT;
		break;

	default:
		return -1;
	}

    ret = waitPoll(&fds, 1, timeout);
	if (0 < ret) {
		if (!hasPollErr(&fds)) {
			return 1;
		} else {
			LOG_WARN("revent=0x%x| error=flag error|", fds.revents);
			return -1;
		}
	} else if (0 == ret) {
		LOG_VERB("poll timeout|");
		return 0;
	} else {
		LOG_WARN("poll err: %s|", ERR_MSG);
		return -1;
	}
}

int SockTool::acceptSock(int listenfd, char name[], char port[]) {
	int newfd = 0;
	socklen_t socklen = 0;
	struct sockaddr_storage addr;

	socklen = sizeof(addr);
	memset(&addr, 0, socklen);
	newfd = accept(listenfd, (struct sockaddr*)&addr, &socklen);
	if (0 <= newfd) {
        setNonBlock(newfd);
        setSockOption(newfd);
		
		parseAddr(&addr, (int)socklen, name, port);
		LOG_DEBUG("accept new sock[%s:%s]|", name, port);
		return newfd;
	} else if (EAGAIN == errno || EINTR == errno) {
	    LOG_VERB("accept err: again or interrupt|");
		return -1;
	} else {
		LOG_ERROR("accept err:%s|", ERR_MSG);
		return -1;
	}
}

int SockTool::sendSock(int fd, const void* buf, int len) {
	int ret = 0;
    int flag = MSG_NOSIGNAL;

    if (0 < len) {
    	ret = send(fd, buf, len, flag);
    	if (0 <= ret) {
    	} else if (-1 == ret && EAGAIN == errno) {
    		ret = 0;
    	} else {
    		LOG_WARN("send err:%s\n", ERR_MSG);
    		ret = -1;
    	}
    }
	
	return ret;
}

int SockTool::sendVec(int fd, struct iovec* iov, int len) {
    int ret = 0;
    int flag = MSG_NOSIGNAL;
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = len;

    ret = sendmsg(fd, &msg, flag);
    if (0 <= ret) {
	} else if (-1 == ret && EAGAIN == errno) {
		ret = 0;
	} else {
		LOG_WARN("sendVec err:%s\n", ERR_MSG);
		ret = -1;
	}

    return ret;
}


int SockTool::recvSock(int fd, void* buf, int len) {
	int ret = 0;

	if (0 < len) {
		ret = recv(fd, buf, len, 0);
    	if (0 < ret) {
            /* read bytes */
    	} else if (-1 == ret && EAGAIN == errno) {
    		ret = 0; 
    	} else if (0 == ret) { 
            LOG_WARN("peer close socket\n");
    		ret = -1;
    	} else {
    		LOG_WARN("recv err:%s\n", ERR_MSG);
    		ret = -1;
    	}
	}

	return ret;
}

int SockTool::recvVec(int fd, struct iovec* iov, int len) {
    int ret = 0;
    int flag = 0;
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = len;

    ret = recvmsg(fd, &msg, flag);
    if (0 < ret) {
        /* read bytes */
	} else if (-1 == ret && EAGAIN == errno) {
		ret = 0; 
	} else if (0 == ret) { 
        LOG_WARN("recvVec peer close socket\n");
		ret = -1;
	} else {
		LOG_WARN("recvVec err:%s\n", ERR_MSG);
		ret = -1;
	}

    return ret;
}

int SockTool::shutdownSock(int fd, int flag) {
    if (SOCK_READ == flag) {
        shutdown(fd, SHUT_RD);
    } else if (SOCK_WRITE ==  flag) {
        shutdown(fd, SHUT_WR);
    } else {
        shutdown(fd, SHUT_RDWR);
    }

    return 0;
}

int SockTool::getConnCode(int fd) {
    int ret = 0;
    int errcode = 0;
	socklen_t socklen = 0;

    socklen = sizeof(errcode);
    ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &socklen);
    if (0 == ret && 0 == errcode) {
        return 0;
    } else {
        LOG_ERROR("connect check err:%s|", strerror(errcode));
        return SOCK_CONNECT_CHK_ERR;
    }
}

int SockTool::getSndBufferSize(int fd) {
    int ret = 0;
    int size = 0;
    socklen_t socklen = 0;

    socklen = sizeof(size);
    ret = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, &socklen);
    if (0 == ret) {
        LOG_DEBUG("get_snd_buffer_size| fd=%d| size=%d|", fd, size);
    } else {
        LOG_WARN("get_snd_buffer_size| fd=%d| error=%s|", fd, ERR_MSG);
        size = 0;
    }

    return size;
}

int SockTool::setSndBufferSize(int fd, int size) {
    int ret = 0;
    socklen_t socklen = 0;

    socklen = sizeof(size);
    ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, socklen);
    if (0 == ret) {
        LOG_DEBUG("set_snd_buffer_size| fd=%d| size=%d|", fd, size);
    } else {
        LOG_WARN("set_snd_buffer_size| fd=%d| size=%d| error=%s|", 
            fd, size, ERR_MSG);
    }

    return ret;
}

int SockTool::getRcvBufferSize(int fd) {
    int ret = 0;
    int size = 0;
    socklen_t socklen = 0;

    socklen = sizeof(size);
    ret = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, &socklen);
    if (0 == ret) {
        LOG_DEBUG("get_rcv_buffer_size| fd=%d| size=%d|", fd, size);
    } else {
        LOG_WARN("get_rcv_buffer_size| fd=%d| error=%s|", fd, ERR_MSG);
        size = 0;
    }

    return size;
}

int SockTool::setRcvBufferSize(int fd, int size) {
    int ret = 0;
    socklen_t socklen = 0;

    socklen = sizeof(size);
    ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, socklen);
    if (0 == ret) {
        LOG_DEBUG("set_rcv_buffer_size| fd=%d| size=%d|", fd, size);
    } else {
        LOG_WARN("set_rcv_buffer_size| fd=%d| size=%d| error=%s|", 
            fd, size, ERR_MSG);
    }

    return ret;
}

int SockTool::closeFd(int fd) {
    int ret = 0;
    
    if (0 < fd) {
        ret = close(fd);
    }

    return ret;
}

void SockTool::pause() {
    ::pause();

    return;
}

int SockTool::createTimerFd() {
	int fd = -1;
	
	fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (0 <= fd) {
		setNonBlock(fd);

        return fd;
	} else {
		LOG_ERROR("timerfd_create| ret=%d| err=%s|", fd, ERR_MSG);
		return -1;
	}
}

int SockTool::setTimerVal(int fd, int timeout) {
    long msec = 0;
    int ret = 0;
    struct itimerspec value;

	/* start a timer at once here */
	value.it_value.tv_sec = 0;
	value.it_value.tv_nsec = 1;

	/* interval time as 100ms*/
    msec = timeout % 1000;
    msec *= 1000000;
	value.it_interval.tv_sec = timeout / 1000;
	value.it_interval.tv_nsec = msec;
    
    ret = timerfd_settime(fd, 0, &value, NULL);
	if (0 == ret) {
		return 0;
	} else {
		LOG_ERROR("timerfd_settime| fd=%d| ret=%d| err=%s|",
            fd, ret, ERR_MSG);
		return -1;
	}
}

int SockTool::createEventFd() {
	int fd = -1;

	/* start a event loop here */
	fd = eventfd(1, 0);
	if (0 <= fd) {
		setNonBlock(fd);

		return fd;
	} else {
		LOG_ERROR("eventfd_create| ret=%d| err=%s|", fd, ERR_MSG);
		return -1;
	}
}

int SockTool::createEpoll(int size) {
    int fd = -1;
    
    fd = epoll_create(size);
	if (0 <= fd) {
        return fd;
	} else {
		LOG_ERROR("epoll_create| ret=%d| err=%s|", fd, ERR_MSG);
		return -1;
	}
}

int SockTool::readTimerOrEvent(int fd, unsigned long long& ullVal) {
    int ret = 0;
    int len = 0;

    ullVal = 0;
    len = read(fd, &ullVal, evsize);
    if (len == evsize) {
        ret = 1;
	} else if (EAGAIN == errno) {
		ret = 0;
	} else {
		LOG_ERROR("read_timer_event| fd=%d| ret=%d| err=%s|", 
            fd, len, ERR_MSG);
		ret = -1;
	}

    return ret;
}

int SockTool::writeTimerOrEvent(int fd, 
    const unsigned long long& ullVal) {
    int ret = 0;
    
    ret = write(fd, &ullVal, evsize);
    if (ret == evsize) {
		ret = 1;
	} else if (EAGAIN == errno) {
	    ret = 0;
	} else {
	    LOG_ERROR("write_timer_event| fd=%d| ret=%d| err=%s|", 
            fd, ret, ERR_MSG);
		ret = -1;
	}

    return ret;
}

int SockTool::addEpoll(int type, int epfd, int fd, void* data) {
    int ret = 0;
    struct epoll_event ev;
    
    if (SOCK_READ == type) {
        ev.events = EPOLLET | EPOLLIN;
        ev.data.ptr = data;
		ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
        if (0 == ret) { 
            LOG_VERB("add_epoll_rd| type=%d| epfd=%d| fd=%d| data=%p|"
                " msg=ok|", type, epfd, fd, data);
        } else {
    		LOG_WARN("add_epoll_rd| type=%d| epfd=%d| fd=%d| data=%p|"
                " error=%s|", type, epfd, fd, data, ERR_MSG);
    	}
    } else if (SOCK_WRITE == type) {
        ev.events = EPOLLET | EPOLLOUT;
        ev.data.ptr = data;
        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
        if (0 == ret) {
            LOG_VERB("add_epoll_wr| type=%d| epfd=%d| fd=%d| data=%p|"
                " msg=ok|", type, epfd, fd, data);
        } else {
            LOG_WARN("add_epoll_wr| type=%d| epfd=%d| fd=%d| data=%p|"
                " error=%s|", type, epfd, fd, data, ERR_MSG);
		}
    } else {
        LOG_WARN("add_epoll| type=%d| epfd=%d| fd=%d| data=%p|"
            " error=invalid type|", type, epfd, fd, data);

        ret = -1;
    }

	return ret;
}

int SockTool::delEpoll(int type, int epfd, int fd, void* data) {
	int ret = 0;

    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    if (0 == ret) { 
        LOG_VERB("del_epoll| type=%d| epfd=%d| fd=%d| data=%p|"
            " msg=ok|", type, epfd, fd, data);
    } else {
        LOG_WARN("del_epoll| type=%d| epfd=%d| fd=%d| data=%p|"
            " error=%s|", type, epfd, fd, data, ERR_MSG);
	}

	return ret;
}

int SockTool::waitEpoll(int epfd, struct epoll_event ev[],
    int size, int timeout) {    
    int ret = 0;
    int ready = 0;
    
    ready = epoll_wait(epfd, ev, size, timeout);
    if (0 < ready) {
        LOG_VERB("wait_epoll| epfd=%d| ready=%d| timeout=%d| msg=ok|", 
            epfd, ready, timeout);

        ret = ready;
    } else if (0 == ready || EINTR == errno) {
        LOG_VERB("wait_epoll| epfd=%d| ready=%d| timeout=%d|", 
            epfd, ready, timeout);
        
		ret = 0;
	} else {
	    LOG_WARN("wait_epoll| epfd=%d| ready=%d| timeout=%d| error=%s|", 
            epfd, ready, timeout, ERR_MSG);
		ret = SOCK_WAIT_EPOLL_ERR;
	}

    return ret;
}

bool SockTool::hasEpollRd(const struct epoll_event* ev) {
    if (EPOLLIN & ev->events) {
        return true;
	} else {
        return false;
	}
}

bool SockTool::hasEpollWr(const struct epoll_event* ev) {
    if (EPOLLOUT & ev->events) {
        return true;
	} else {
        return false;
	}
}

bool SockTool::hasEpollExcpt(const struct epoll_event* ev) {
        static const unsigned int EV_EPOLL_ERR = EPOLLERR | EPOLLHUP;
        
    if (EV_EPOLL_ERR & ev->events) {
        return true;
	} else {
        return false;
	}
}

CEvent::CEvent(bool bDel) : m_bDel(bDel) {
    m_fd = -1;
}

CEvent::~CEvent() {
    if (0 <= m_fd) {
        if (m_bDel) {
            SockTool::closeFd(m_fd);
        }
        
        m_fd = -1;
    }
}

int CEvent::create() { 
	/* start a event loop here */
	m_fd = SockTool::createEventFd();
	if (0 <= m_fd) {
		return 0;
	} else {
		return -1;
	}
}

int CEvent::putEvent() {
    int ret = 0;

    ret = SockTool::writeTimerOrEvent(m_fd, DEF_EVENT_VAL);
	return ret;
}

int CEvent::getEvent() {
    unsigned long long ullVal = 0;
	int ret = 0;

    ret = SockTool::readTimerOrEvent(m_fd, ullVal);
	if (0 < ret) {        
        if (1ULL == ullVal) {
		    LOG_VERB("get_event| val=%llu|", ullVal);
        } else {
            LOG_DEBUG("=======get_event| val=%llu|", ullVal);
        }
	} 

    return ret;
}

int CEvent::waitEvent(int timeout) {
    int ret = 0;

    ret = SockTool::pollSock(m_fd, SOCK_READ, timeout);
    if (0 < ret) {
        ret = getEvent();
    }

    return ret;
}

