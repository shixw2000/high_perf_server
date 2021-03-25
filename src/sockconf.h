#ifndef __SOCKCONF_H__
#define __SOCKCONF_H__


struct msg_type;
struct _SockData;

class SockConf {
public:
    virtual ~SockConf() {}
    
    virtual int onAccepted(const struct _SockData* pData) = 0;

    virtual int onConnected(const struct _SockData* pData) = 0;
    virtual void onFailConnect(const struct _SockData* pData) = 0;

    virtual void onClosed(const struct _SockData* pData) = 0;

    virtual int process(struct _SockData* pData, struct msg_type* pMsg) = 0;
};


#endif

