#ifndef __GLOBALTYPE_H__
#define __GLOBALTYPE_H__
/*************************
README FIRST
@author: shixw 2022-11

1. You should modify the linux system config:
/proc/sys/net/core/wmem_max >=10485760
/proc/sys/net/core/rmem_max >=10485760
to reach the better socket buffer

2.1 to test connect & listen speed, the env need to be modified:
net.core.somaxconn = 10000
net.ipv4.tcp_max_syn_backlog = 10000

2.2 modify the nofile in file /etc/security/limits.conf
*               soft    nofile       1000000
*               hard    nofile       1000000

2. usage: 
    server: prog 1 <local_ip> <local_port> 
    client:  prog 0 <srv_ip>   <srv_port> <conn_cnt>

***************************/
#include"sysoper.h"
#include"misc.h"


static const Int32 DEF_EPOLL_WAIT_MILI_SEC = 1000;
static const Int32 DEF_VEC_SIZE = 100;
static const Int32 DEF_TCP_SEND_BUF_SIZE = 0x100000;
static const Int32 DEF_TCP_RCV_BUF_SIZE = 0x100000;
static const Int32 MAX_MSG_SIZE = 0x500000;

static const Uint16 DEF_MSG_VER = 0x0814;
static const Byte DEF_DELIM_CHAR = 0xAC;

static const Int32 DEF_IP_SIZE = 32;
static const Int32 DEF_ADDR_SIZE = 128; 

static const Int32 DEF_LOCK_ORDER = 3;
static const Int32 DEF_FD_MAX_CAPACITY = 200000;

enum EnumThreadType {
    ENUM_THREAD_DIRECTOR,
    ENUM_THREAD_READ,
    ENUM_THREAD_WRITE,
    ENUM_THREAD_DEALER,
};

enum EventType {
    EVENT_TYPE_RD = 0x1,
    EVENT_TYPE_WR = 0x2,
    EVENT_TYPE_ALL = EVENT_TYPE_WR | EVENT_TYPE_RD,
};

#endif

