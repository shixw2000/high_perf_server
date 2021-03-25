#ifndef __SOCKTOOL_H__
#define __SOCKTOOL_H__


struct iovec;
struct sockaddr;
struct epoll_event;
struct pollfd;

class SockTool {
public:
    static int createInetTcp();
    static int createUnixTcp(const char name[], const char port[]);

    static int parseAddr(const void* sa, int len, 
        char name[], char port[]); 

    static int connSrvInet(int fd, const char name[], const char port[]);
    static int connSrvUnix(int fd, const char name[], const char port[]);
    
    static int createSvrInet(const char name[], const char port[]);
    static int createSvrUnix(const char name[], const char port[]);

    static int waitPoll(struct pollfd* fds, int size, int timeout);
    static bool hasPollRd(const struct pollfd* ev);
    static bool hasPollWr(const struct pollfd* ev);
    static bool hasPollErr(const struct pollfd* ev);
    
	static int pollSock(int fd, int ev, int timeout);

	static int acceptSock(int listenfd, char name[], char port[]);

	static int sendSock(int fd, const void* buf, int len);
    static int sendVec(int fd, struct iovec* iov, int len);
    
	static int recvSock(int fd, void* buf, int len);
    static int recvVec(int fd, struct iovec* iov, int len);

    static int shutdownSock(int fd, int flag); 
        
    static int getSndBufferSize(int fd);
    static int setSndBufferSize(int fd, int size);
    static int getRcvBufferSize(int fd);
    static int setRcvBufferSize(int fd, int size); 

    static int createEventFd(); 

    static int createTimerFd();
    static int setTimerVal(int fd, int timeout);

    /* -1: err, 0: nodata, <0: ok */
    static int readTimerOrEvent(int fd, unsigned long long& ullVal);
    static int writeTimerOrEvent(int fd, 
        const unsigned long long& ullVal) ;

    static int createEpoll(int size);
    static int addEpoll(int type, int epfd, int fd, void* data);
    static int delEpoll(int type, int epfd, int fd, void* data); 
    static int waitEpoll(int epfd, struct epoll_event ev[],
        int size, int timeout);

    static bool hasEpollRd(const struct epoll_event* ev);
    static bool hasEpollWr(const struct epoll_event* ev);
    static bool hasEpollExcpt(const struct epoll_event* ev);

    static int getConnCode(int fd);
    
    static int closeFd(int fd);

    static void pause();

private:
    static int createSock(int family, int type, int proto);
    static int connAddr(int fd, struct sockaddr* addr, int len);
    
    static int createAddrInet(const char name[], const char port[], 
		void* addr, int* len);
    static int createAddrUnix(const char name[], const char port[], 
		void* addr, int* len);

    static int parseAddrInet(const void* addr, int len,
		char name[], char port[]);

    static int parseAddrUnix(const void* addr, int len,
		char name[], char port[]);
    
    static int bindAddr(int fd, struct sockaddr* addr, int len);
    static int listenSrv(int fd, int size);

    static int setNonBlock(int fd);
    static void setSockBuffer(int fd);
    static void setSockOption(int fd);
    static void setNoTimewait(int fd);
    static void setNoDelay(int fd);
    static void setCork(int fd);
    static void setReuse(int fd); 
};

class CEvent {
public:
    explicit CEvent(bool bDel);
    ~CEvent();
    
    int create();
    
    int putEvent();
    int waitEvent(int timeout);

    inline int getFd() const { return m_fd; }

    int getEvent();
    
private:
    const bool m_bDel;
    int m_fd; 
};

#endif

