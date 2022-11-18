#ifndef __SERVER_H__
#define __SERVER_H__
#include"globaltype.h"
#include"interfobj.h"


struct MsgHdr;
struct NodeBase;
class Parser;
class Director; 
class SockCb;
class Server;

class DefSockCb : public SockCb {
public:
    explicit DefSockCb(Server* server) : m_server(server) {
        m_cnt = 0;
        m_reply = FALSE;
        m_prnt_order = 20;
        m_snd_cnt = 1;
    }

    void setParam(Bool reply, Int32 prnt_order, Int32 snd_cnt) {
        m_reply = reply;
        m_prnt_order = prnt_order;
        m_snd_cnt = snd_cnt;
    }
    
    virtual int onAccepted(struct NodeBase* );

    virtual int onConnected(struct NodeBase* );
    virtual void onFailConnect(struct NodeBase* );

    virtual void onClosed(struct NodeBase* base);

    virtual Int32 process(struct NodeBase* base, struct MsgHdr* msg);

private:
    MsgHdr* creatMsg(Uint16 cmd, const Void* data, Int32 size);

private:
    Server* m_server;
    Bool m_reply;
    Int32 m_prnt_order;
    Int32 m_snd_cnt;
    Uint64 m_cnt;
};

class Server {
public:
    Server();
    ~Server();

    Int32 init();
    Void finish();

    int start();
    void join();
    void stop(); 

    inline Void setConf(const Char* path) {
        m_path = path;
    } 
    
    Void setSockCb(SockCb* cb);

    Uint32 getTick() const;
    Uint32 getTime() const;

    Int32 sendMsg(NodeBase* base, MsgHdr* msg);

    Int32 startServer(const Char ip[], Int32 port);
    Int32 startClient(const Char ip[], Int32 port);

    Int32 addService(const Char ip[], Int32 port, Bool bServ);

private: 
    Parser* m_parser;
    const Char* m_path;
    Director* m_director;
    DefSockCb* m_cb;
};

#endif

