#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/timerfd.h>
#include<sys/eventfd.h>
#include<errno.h>
#include<poll.h>
#include<stdlib.h>
#include<time.h>
#include"sockutil.h"
#include"datatype.h"


Int32 creatTcpSrv(const TcpParam* param) {
    Int32 ret = 0;
    Int32 fd = -1; 

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > fd) {
        LOG_ERROR("create_tcp| ret=%d| error=%s|",
            fd, ERR_MSG());

        return -1;
    }

    do {
        setNonBlock(fd); 
        setReuse(fd);
        
        ret = bind(fd, (const struct sockaddr*)param->m_addr, param->m_addr_len);
        if (0 != ret) {
            LOG_ERROR("bind_tcp| ret=%d| fd=%d| ip=%s| port=%d| error=%s|",
                ret, fd, param->m_ip, param->m_port, ERR_MSG());
            
            break; 
        }

        ret = listen(fd, 10000);
        if (0 != ret) {
            LOG_ERROR("listen_tcp| ret=%d| fd=%d| ip=%s| port=%d| error=%s|",
                ret, fd, param->m_ip, param->m_port, ERR_MSG());
            
            break; 
        } 

        LOG_INFO("listen_tcp| fd=%d| ip=%s| port=%d| msg=ok|", 
            fd, param->m_ip, param->m_port);
        
        return fd;
    } while (0);

    closeHd(fd);
    return -1;
}

Int32 creatSock() {
    Int32 fd = -1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 <= fd) { 
        return fd;
    } else {
        LOG_ERROR("create_sock| ret=%d| error=%s|", fd, ERR_MSG());

        return -1;
    }
}

Int32 connCli(Int32 fd, const TcpParam* param) {
    Int32 ret = 0;
    
    ret = connect(fd, (const struct sockaddr*)param->m_addr, param->m_addr_len);
    if (0 == ret) {
        return 0;
    } else if (EINPROGRESS == errno) {
        return 1;
    } else {
        LOG_ERROR("conn_fast| fd=%d| ip=%s| port=%d| error=%s|", 
            fd, param->m_ip, param->m_port, ERR_MSG());
        return -1;
    } 
}

Int32 chkConnStatus(Int32 fd) {
    int ret = 0;
    int errcode = 0;
	socklen_t socklen = 0;

    socklen = sizeof(errcode);
    ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &socklen);
    if (0 == ret && 0 == errcode) {
        return 0;
    } else {
        LOG_ERROR("chk_conn_status| ret=%d| errcode=%d| error=%s|",
            ret, errcode, strerror(errcode));
        return -1;
    } 
}

Int32 creatTcpCli(const Char peer_ip[], int peer_port) {
    Int32 ret = 0;
    Int32 fd = -1;
    Int32 addr_len = 0;
    struct sockaddr_storage addr;
    
    addr_len = sizeof(struct sockaddr_storage);
    ret = creatAddr(peer_ip, peer_port, &addr, &addr_len);
    if (0 != ret) {
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > fd) {
        LOG_INFO("create_tcp| ret=%d| error=%s|",
            fd, ERR_MSG());

        return -1;
    }

    ret = connAddr(fd, &addr, addr_len);
    if (0 == ret) {
        setNonBlock(fd); 
        
        LOG_INFO("connect_tcp| peer_ip=%s| peer_port=%d| msg=ok|",
            peer_ip, peer_port);

        return fd;
    } else {
        LOG_INFO("connect_tcp| ret=%d| peer_ip=%s| peer_port=%d| error=%s|",
            ret, peer_ip, peer_port, ERR_MSG());
        
        closeHd(fd);
        return -1;
    } 
}

Int32 setNonBlock(Int32 fd) {
    Int32 ret = 0;

    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    return ret;
}

Int32 setReuse(Int32 fd) {
    Int32 ret = 0;
    Int32 val = 1;
	Int32 len = sizeof(val);
    
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, len);
    if (0 != ret) {
        LOG_INFO("set_reuse| fd=%d| ret=%d| error=%s|", 
            fd, ret, ERR_MSG());
    }

    return ret;
}

Void setNoTimewait(int fd) {
    int ret = 0;
    struct linger val;
	int len = sizeof(val);
    
    val.l_onoff=1;
    val.l_linger=0;
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, &val, len);
    if (0 != ret) {
        LOG_INFO("set_no_timewait| fd=%d| ret=%d| error=%s|", 
            fd, ret, ERR_MSG());
    }
}

Int32 creatAddr(const Char ip[], int port, Void* addr, Int32* plen) {
    Int32 ret = 0;
    struct sockaddr_in* dst = (struct sockaddr_in*)addr;

    memset(addr, 0, *plen);

    if (0 < port && 0x10000 > port) {
        dst->sin_family = AF_INET;
        dst->sin_port = htons(port);
        ret = inet_pton(AF_INET, ip, &dst->sin_addr);
        if (1 == ret) {
            *plen = sizeof(struct sockaddr_in);
            return 0;
        } else {
            *plen = 0;
            return -1;
        }
    } else {
        *plen = 0;
        return -1;
    }
}

Int32 buildParam(const Char ip[], Int32 port, TcpParam* param) {
    Int32 ret = 0;

    memset(param, 0, sizeof(TcpParam));
    
    strncpy(param->m_ip, ip, DEF_IP_SIZE-1);
    param->m_port = port;

    param->m_addr_len = (Int32)sizeof(param->m_addr);
    ret = creatAddr(param->m_ip, param->m_port, 
        param->m_addr, &param->m_addr_len);
    
    return ret;
}

Int32 parseAddr(const Void* addr, Char ip[], int* port) {
    const char* psz = NULL;
    const struct sockaddr_in* src = (const struct sockaddr_in*)addr;
    
    psz = inet_ntop(AF_INET, &src->sin_addr, ip, DEF_IP_SIZE);
    if (NULL != psz) {
        *port = ntohs(src->sin_port);
        return 0;
    } else {
        *port = 0;
        return -1;
    }
}

Int32 getLocalInfo(Int32 fd, Char ip[], int* port) {
    Int32 ret = 0;
    socklen_t addr_len = 0;
    struct sockaddr_storage addr;

    addr_len = sizeof(addr);
    ret = getsockname(fd, (struct sockaddr*)&addr, &addr_len);
    if (0 != ret) {
        return -1;
    }

    ret = parseAddr(&addr, ip, port);
    if (0 != ret) {
        return -1;
    }

    return 0;
}

Int32 getPeerInfo(Int32 fd, Char ip[], int* port) {
    Int32 ret = 0;
    socklen_t addr_len = 0;
    struct sockaddr_storage addr;

    addr_len = sizeof(addr);
    ret = getpeername(fd, (struct sockaddr*)&addr, &addr_len);
    if (0 != ret) {
        return -1;
    }

    ret = parseAddr(&addr, ip, port);
    if (0 != ret) {
        return -1;
    }

    return 0;
}

Int32 closeHd(Int32 fd) {
    Int32 ret = 0;
    
    if (0 <= fd) {
        ret = close(fd);
    }

    return ret;
}

Int32 shutdownHd(Int32 fd) {
    Int32 ret = 0;
    
    if (0 <= fd) {
        ret = shutdown(fd, SHUT_RDWR);
    }

    return ret;
}

Int32 connAddr(Int32 fd, const Void* addr, Int32 len) {
    Int32 ret = 0;

    ret = connect(fd, (struct sockaddr*)addr, len);
    if (0 == ret) {
        return 0;
    } else {
        LOG_ERROR("connect| fd=%d| ret=%d| error=%s|",
            fd, ret, ERR_MSG());
        return -1;
    }
}

Int32 sendTcp(Int32 fd, const Void* buf, Int32 len) {
    Int32 sndlen = 0;

    if (0 < len) {
        sndlen = send(fd, buf, len, MSG_NOSIGNAL);
        if (0 <= sndlen) {
            LOG_DEBUG("sendTcp| fd=%d| len=%d| sndlen=%d|"
                " msg=ok|",
                fd, len, sndlen);
            
            return sndlen;
        } else if (EAGAIN == errno || EINTR == errno) {
            return 0;
        }  else {
            LOG_ERROR("sendTcp| fd=%d| len=%d| ret=%d| error=%s|",
                fd, len, sndlen, ERR_MSG());
            return -1;
        }
    } else {
        return 0;
    }
}

Int32 recvTcp(Int32 fd, Void* buf, Int32 maxlen) {
    Int32 rdlen = 0;

    if (0 < maxlen) { 
        rdlen = recv(fd, buf, maxlen, 0);
        if (0 < rdlen) {
            LOG_DEBUG("recvTcp| fd=%d| maxlen=%d| rdlen=%d| msg=ok|",
                fd, maxlen, rdlen);
            
            return rdlen;
        } else if (0 == rdlen) {
            /* end of read */
            LOG_ERROR("recvTcp| fd=%d| maxlen=%d| error=eof|",
                fd, maxlen);
            return -2;
        } else if (EAGAIN == errno || EINTR == errno) {
            return 0;
        } else {
            LOG_ERROR("recvTcp| fd=%d| maxlen=%d| error=%s|",
                fd, maxlen, ERR_MSG());
            
            return -1;
        }
    } else {
        return 0;
    }
}

int recvVec(int fd, struct iovec* iov, int size, int maxlen) {
    int rdlen = 0;
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = size;

    rdlen = recvmsg(fd, &msg, 0);
    if (0 < rdlen) {
        LOG_DEBUG("recvVec| fd=%d| maxlen=%d| rdlen=%d| msg=ok|",
            fd, maxlen, rdlen);
	} else if (-1 == rdlen && EAGAIN == errno) {
		rdlen = 0; 
	} else if (0 == rdlen) { 
	    LOG_ERROR("recvVec| fd=%d| maxlen=%d| error=eof|",
            fd, maxlen);
		rdlen = -2;
	} else {
		LOG_ERROR("recvVec| fd=%d| maxlen=%d| error=%s|",
            fd, maxlen, ERR_MSG());
		rdlen = -1;
	}

    return rdlen;
}

int sendVec(int fd, struct iovec* iov, int size, int maxlen) {
    int sndlen = 0;
    int flag = MSG_NOSIGNAL;
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = size;

    sndlen = sendmsg(fd, &msg, flag);
    if (0 < sndlen) {
        LOG_DEBUG("sendVec| fd=%d| size=%d| maxlen=%d| sndlen=%d| msg=ok|",
            fd, size, maxlen, sndlen);
	} else if (0 == sndlen || EAGAIN == errno) {
		sndlen = 0;
	} else {
		LOG_ERROR("sendVec| fd=%d| size=%d| maxlen=%d| error=%s|",
            fd, size, maxlen, ERR_MSG());
		sndlen = -1;
	}

    return sndlen;
}


/* return >=0: ok fd, -1: error, -2: empty */
Int32 acceptCli(Int32 fd) {
    Int32 newFd = -1;

    newFd = accept(fd, NULL, NULL);
    if (0 <= newFd) {
        setNonBlock(newFd);
        setNoTimewait(newFd);
        
        return newFd;
    } else if (EAGAIN == errno || EINTR == errno) {
        return -2;
    } else {
        return -1;
    }
}

Int32 peekTcp(Int32 fd, Void* buf, Int32 maxlen) {
    Int32 rdlen = 0;

    rdlen = recv(fd, buf, maxlen, MSG_PEEK);
    if (0 < rdlen) {
        LOG_INFO("peekTcp| fd=%d| maxlen=%d| rdlen=%d| msg=ok|",
            fd, maxlen, rdlen);
        
        return rdlen;
    } else if (0 == rdlen) {
        /* end of read */
        LOG_INFO("peekTcp| fd=%d| maxlen=%d| error=eof|",
            fd, maxlen);
        return -2;
    } else if (EAGAIN == errno || EINTR == errno) {
        return 0;
    } else {
        LOG_INFO("peekTcp| fd=%d| maxlen=%d| error=%s|",
            fd, maxlen, ERR_MSG());
        
        return -1;
    }
}


Int32 writeEvent(int fd) {
    Int32 ret = 0;
    Int32 len = sizeof(Uint64);
    Uint64 val = 1;

    ret = write(fd, &val, len);
    if (ret == len) {
        return 0;
    } else {
        return -1;
    }
}

Int32 readEvent(int fd, Uint32* pVal) {
    Int32 ret = 0;
    Int32 len = sizeof(Uint64);
    Uint64 val = 1;

    ret = read(fd, &val, len);
    if (ret == len) {
        *pVal = (Uint32)val;
        
        return 0;
    } else {
        *pVal = 0;
        
        return -1;
    }
}

Int32 creatTimerFd(Int32 ms) {
    Int32 ret = 0;
    Int32 sec = 0;
    Int32 fd = -1;
    struct itimerspec value;

    if (1000 <= ms) {
        sec = ms / 1000;
        ms = ms % 1000;
    }

    fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (0 > fd) {
        return -1;
    }
    
    value.it_value.tv_sec = 0;
	value.it_value.tv_nsec = 1;
    value.it_interval.tv_sec = sec;
	value.it_interval.tv_nsec = ms * 1000000;
    ret = timerfd_settime(fd, 0, &value, NULL);
    if (0 != ret) {
        close(fd);
        return -1;
    }

    return fd;
}

Int32 creatEventFd() {
    Int32 fd = -1; 

    fd = eventfd(1, EFD_NONBLOCK);
    if (0 > fd) {
        return -1;
    } 
    
    return fd;
}

Void sysPause() {
    pause();
}

Uint32 sysRand() {
    Uint32 n = rand();

    return n;
}

Void waitEvent(Int32 fd, Int32 millsec) {
    Int32 ret = 0;
    Uint32 cnt = 0;
    struct pollfd fds;

    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;
    
    ret = poll(&fds, 1, millsec);
    if (0 < ret && (POLLIN & fds.revents)) {
        readEvent(fd, &cnt);
    }
}

Int32 connFast(const TcpParam* param, Int32* pfd) {
    Int32 ret = 0;
    Int32 fd = -1; 
    
    fd = creatSock();
    if (0 > fd) {
        *pfd = -1;
        return -1;
    }

    setNonBlock(fd);
    setNoTimewait(fd);

    ret = connCli(fd, param);
    if (1 == ret) {
        /* in progress */
        *pfd = fd;
    } else if (0 == ret) {
        /* conn ok */
        *pfd = fd;
    } else {
        closeHd(fd);

        *pfd = -1;
        ret = -1;
    } 

    return ret;
}

Int32 connSlow(const TcpParam* param, Int32* pfd) {
    Int32 ret = 0;
    Int32 fd = -1; 
    
    fd = creatSock();
    if (0 > fd) {
        *pfd = -1;
        return -1;
    } 

    ret = connCli(fd, param);
    if (0 == ret) {
        setNonBlock(fd);
        setNoTimewait(fd);
        
        /* conn ok */
        *pfd = fd;
    } else {
        closeHd(fd);

        *pfd = -1;
        ret = -1;
    } 

    return ret;
}


