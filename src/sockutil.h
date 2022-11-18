#ifndef __SOCKUTIL_H__
#define __SOCKUTIL_H__
#include"globaltype.h"


struct TcpParam;
struct iovec;

Int32 creatSock();
Int32 creatTcpSrv(const TcpParam* param); 
Int32 connCli(Int32 fd, const TcpParam* param);
Int32 chkConnStatus(Int32 fd);

Int32 creatTcpCli(const Char peer_ip[], int peer_port);

Int32 setNonBlock(Int32 fd);
Int32 setReuse(Int32 fd);
Void setNoTimewait(int fd);

Int32 creatAddr(const Char ip[], int port, Void* addr, Int32* plen);
Int32 buildParam(const Char ip[], Int32 port, TcpParam* param);
Int32 parseAddr(const Void* addr, Char ip[], int* port);
Int32 getLocalInfo(Int32 fd, Char ip[], int* port);
Int32 getPeerInfo(Int32 fd, Char ip[], int* port);

Int32 connAddr(Int32 fd, const Void* addr, Int32 len);

Int32 closeHd(Int32 fd);
Int32 shutdownHd(Int32 fd);

Int32 sendTcp(Int32 fd, const Void* buf, Int32 len);
Int32 recvTcp(Int32 fd, Void* buf, Int32 maxlen);
Int32 peekTcp(Int32 fd, Void* buf, Int32 maxlen);
Int32 acceptCli(Int32 fd);

int recvVec(int fd, struct iovec* iov, int size, int maxlen);
int sendVec(int fd, struct iovec* iov, int size, int maxlen);

Int32 creatTimerFd(Int32 ms);
Int32 creatEventFd();

Int32 writeEvent(int fd);
Int32 readEvent(Int32 fd, Uint32* pVal = NULL);

Void sysPause();
Uint32 sysRand();

Void waitEvent(Int32 fd, Int32 millsec);

Int32 connFast(const TcpParam* param, Int32* pfd);
Int32 connSlow(const TcpParam* param, Int32* pfd);

#endif

