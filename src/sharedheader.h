/*************************
README FIRST
@author: shixw 2021-03-17

1. You should modify the linux system config:
/proc/sys/net/core/wmem_max >=10485760
/proc/sys/net/core/rmem_max >=10485760
to reach the better socket buffer

2. to test connect & listen speed, the env need to be modified:
net.core.somaxconn = 10000
net.ipv4.tcp_max_syn_backlog = 10000


2. usage: 
    server: prog 1 <local_ip> <local_port> [max_print_cnt]
    client:  prog 0 <srv_ip>   <srv_port> \
                     <conn_cnt> <speed> [pkg_size] [max_print_cnt]

***************************/
#ifndef __SHAREDHEADER_H__
#define __SHAREDHEADER_H__
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<pthread.h>


#if 0
#define LOG(level,format,args...) \
    fprintf(stdout, "[%s:%d]<%llu>: "format"|\n", \
	__func__, __LINE__, getTime(), ##args)

#else

#define LOG(level,format,args...) \
    fprintf(stdout, "[%llu]: "format"|\n", getTime(), ##args)
#endif

#ifndef __LOG_LEVEL__
#define __LOG_LEVEL__ 3
#endif

#if (__LOG_LEVEL__ >= 4)
#define LOG_VERB(format,args...) LOG(4,format, ##args)
#else 
#define LOG_VERB(format,args...) 
#endif

#if (__LOG_LEVEL__ >= 3)
#define LOG_DEBUG(format,args...) LOG(3,format, ##args)
#else 
#define LOG_DEBUG(format,args...) 
#endif

#if (__LOG_LEVEL__ >= 2)
#define LOG_INFO(format,args...) LOG(2,format, ##args)
#else
#define LOG_INFO(format,args...)
#endif

#if (__LOG_LEVEL__ >= 1)
#define LOG_WARN(format,args...) LOG(1,format, ##args)
#else 
#define LOG_WARN(format,args...)
#endif

#define LOG_ERROR(format,args...) LOG(-1,format, ##args)

#define ERR_MSG strerror(errno)
#define CHK_EXPR(ret,opt,n) if (!((ret)opt(n))) \
	{LOG(-1, "chk not equal[%s:%s]", #opt, ERR_MSG); return -1;}
#define CHK_EQ(ret,n) CHK_EXPR(ret,==,n)

/**
bool __sync_bool_compare_and_swap(type* ptr, type old, type new)
type __sync_val_compare_and_swap(type* ptr, type old, type new)

type __sync_fetch_and_add(type* ptr, type val, ...)
type __sync_fetch_and_sub(type* ptr, type val, ...)
type __sync_fetch_and_or(type* ptr, type val, ...)
type __sync_fetch_and_and(type* ptr, type val, ...)
type __sync_fetch_and_xor(type* ptr, type val, ...)
type __sync_fetch_and_nand(type* ptr, type val, ...)

type __sync_add_and_fetch(type* ptr, type val, ...)
type __sync_sub_and_fetch(type* ptr, type val, ...)
type __sync_or_and_fetch(type* ptr, type val, ...)
type __sync_and_and_fetch(type* ptr, type val, ...)
type __sync_xor_and_fetch(type* ptr, type val, ...)
type __sync_nand_and_fetch(type* ptr, type val, ...)


//full barrier
void __sync_synchronize()

//acqure barrier: it means references after the builtin
// cannot move to before the builtin,
// but previous memory stores may not e globally visible yet,
// and previous memory loads may not yet be satisfied.
type __sync_lock_test_and_set(type* ptr, type val, ...)

//release barrier: it means all previous  memory stores
// are globally visible, and all previous memory loads
// have been satisfied, but following memory reads are 
// not prevented from being speculated to before the barrier
void __sync_lock_release(type* ptr, ...)

**/

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

#define CAS(ptr,test,val) __sync_bool_compare_and_swap((ptr), (test), (val))
#define CMPXCHG(ptr,test,val) __sync_val_compare_and_swap((ptr), (test), (val)) 
#define ATOMIC_ADD(ptr, val) __sync_add_and_fetch((ptr), val)
#define ATOMIC_SUB(ptr, val) __sync_sub_and_fetch((ptr), val)
#define ATOMIC_INC(ptr) __sync_add_and_fetch((ptr), 1)
#define ATOMIC_DEC(ptr) __sync_sub_and_fetch((ptr), 1)
#define ATOMIC_LOCK(ptr) __sync_lock_test_and_set((ptr), 1)
#define ATOMIC_UNLOCK(ptr) __sync_lock_release(ptr)

#define ATOMIC_SET(ptr,val) __sync_lock_test_and_set((ptr), (val))

#define barrier() __sync_synchronize()

#ifdef __USE_MB__
#define smp_mb() asm volatile ("mfence": : :"memory")
#define smp_wmb() asm volatile ("sfence": : :"memory")
#define smp_rmb() asm volatile ("lfence": : :"memory")
#else
#define smp_mb()
#define smp_wmb()
#define smp_rmb()
#endif

#define offsetof(type,mem) ((size_t)(&((type*)0)->mem))
#define containof(ptr,type,mem) ({ \
    const typeof( ((type*)0)->mem )* _ptr = (ptr); \
    (type*)( (char*)_ptr - offsetof(type,mem) ); })
#define countof(array) ((int)(sizeof(array)/sizeof((array)[0])))

#define __IN    
#define __OUT   

#if 1
#define MY_COPY(dst,src,n) memcpy(dst,src,n)
#define MY_STR_COPY(dst,src,n) strncpy(dst, src, n)
#define MY_PRINT_STR(dst,n,format,args...) snprintf(dst, n, format, ##args)
#else
#define MY_COPY(dst,src,n)
#endif

enum PollType {
    POLL_TYPE_WRITE_EPOLL = 0,
    POLL_TYPE_SOCKET,
	POLL_TYPE_LISTENER,
	POLL_TYPE_TIMER,
	POLL_TYPE_EVENTER, 

	POLL_TYPE_NONE
};

enum SOCK_EVENT {
	SOCK_READ = 0,
	SOCK_WRITE = 1,
	SOCK_EXCEPTION = 2,

	SOCK_ALL
};

enum SOCK_ERROR_REASON { 
    RECV_ERR_INIT_QUE = -10000,
    RECV_ERR_NO_MEMORY,
    RECV_ERR_HEAER_PROTOCOL,
    RECV_ERR_HEADER_SIZE,
    RECV_ERR_SOCK_FAIL,
    RECV_ERR_DISPATCH_MSG,
    RECV_ERR_BODY_CSUM,
    RECV_ERR_TIMEOUT,

    SEND_ERR_INIT_QUE = -20000, 
    SEND_ERR_SOCK_FAIL,
    SEND_ERR_TIMEOUT,

    DEAL_ERR_TIMEOUT = -30000,
    DEAL_LOGIN_CHK_ERR,
    DEAL_PROC_MSG_ERR,

    COMMON_ERR_BASE_LINE = -90000,
    COMMON_ERR_SOCK_EXCEPTION,
    COMMON_ERR_SOCK_END,
    COMMON_ERR_INTR,

    SOCK_ADD_ALLOC_ERR = -110000,
    SOCK_ADD_INIT_ERR,
    SOCK_ADD_TYPE_ERR,
    SOCK_CREATE_EPOLL_ERR,
    SOCK_BIND_ADDR_ERR,
    SOCK_LISTEN_SRV_ERR,
    SOCK_CONN_SRV_ERR,
    SOCK_CONN_SRV_PROGRESS,
    SOCK_CREATE_FD_ERR,
    
    SOCK_SET_FLAG_ERR,
    SOCK_WAIT_POLL_ERR,
    SOCK_WAIT_EPOLL_ERR,
    SOCK_CONNECT_CHK_ERR,
    SOCK_CONNECT_TIMEOUT_ERR,
    SOCK_CONNECT_POST_ERR,
};

static const int MAX_MSG_POOL_SIZE = 10000000;
static const int MAX_SOCK_FD_SIZE = 300000;
static const int MAX_EPOLL_POOL_SIZE = MAX_SOCK_FD_SIZE;
static const int MAX_LISTEN_SRV_SIZE = 100000;
static const int EPOLL_EV_SIZE = 100000;
static const int MAX_SOCKET_BUFFER_SIZE = 1024 * 1024 * 5;


static const int DEF_SEND_VEC_SIZE = 1000;
static const int MAX_PKG_MSG_SIZE = 1024 * 1024 * 5; 
static const int DEF_RECV_MSG_SIZE_ONCE = 1024 * 1024 * 20;
static const int DEF_SEND_MSG_SIZE_ONCE = DEF_RECV_MSG_SIZE_ONCE;
static const int DEF_TEST_MSG_SIZE = 1024;
static const int MAX_PRINT_MSG_CNT = 2000000;


extern long g_me;
extern int* g_fds;
extern int g_cnt;
extern int g_clifd;
extern char g_sndmsg[];
extern char g_rcvmsg[];
extern unsigned long long getTime();
extern void setPrntMax(int max);
extern bool needPrnt(int n);


/****************
    return: true: reply response msg, false: free response msg
        test sock delay case: [server-reply, client-reply]
        default case: [server-reply, client-free]
*************/
extern bool needReply();
extern void setReply(bool isReply);

#endif

