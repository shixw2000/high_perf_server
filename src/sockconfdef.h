#ifndef __SOCKCONFDEF_H__
#define __SOCKCONFDEF_H__
#include"sockconf.h"


typedef struct _SockData SockData;
class SockMonitor;

class SockConfDef : public SockConf {
public:
    explicit SockConfDef(SockMonitor* monitor);
    virtual ~SockConfDef();
    
    virtual int onAccepted(const SockData* pData);

    virtual int onConnected(const SockData* pData);

    virtual void onFailConnect(const SockData* pData);

    virtual void onClosed(const SockData* pData);
    
    virtual int process(SockData* pData, struct msg_type* pMsg);

private:
    int chkBody(const SockData* pData, const struct msg_type* pMsg);
    int chkLogin(const SockData* pData, struct msg_type* pMsg);

private:
    SockMonitor* m_monitor;
};


#endif

