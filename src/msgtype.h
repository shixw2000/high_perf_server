#ifndef __MSGTYPE_H__
#define __MSGTYPE_H__
#include"sysoper.h"
#include"globaltype.h"


#pragma pack(push, 1)

struct MsgHdr {
    Int32 m_size;
    Uint32 m_seq;
    Uint32 m_crc;
    Uint32 m_retcode;
    Uint16 m_version;
    Uint16 m_cmd;
};

struct MsgNotify {
    Uint64 m_data;
};

struct MsgSockAddr {
    Int32 m_is_serv;   // 1-server, 0-client
    Int32 m_port;
    Char m_ip[DEF_IP_SIZE];
};

struct MsgTest {
    Byte m_data[1];
};

#pragma pack(pop)


static const Int32 DEF_MSG_HEADER_SIZE = sizeof(struct MsgHdr);

enum EnumMsgCmd {
    ENUM_MSG_CMD_NULL = 0,

    ENUM_MSG_SYSTEM_STOP,

    ENUM_MSG_SYSTEM_TICK_TIMER,

    ENUM_MSG_SYSTEM_ADD_ADDRESS,
    
    ENUM_MSG_SYSTEM_EXIT = 100,   // system cmds max

    ENUM_MSG_SOCK_CONNECT_STATUS,
    ENUM_MSG_LISTENER_ACCEPT,

    ENUM_MSG_TEST,
};

#endif

