#ifndef __SOCKOBJ_H__
#define __SOCKOBJ_H__
#include<sys/uio.h>
#include"globaltype.h"
#include"interfobj.h"
#include"datatype.h"


class Director;

class MsgReader {     
public: 
    Bool chkHeader(const MsgHdr* hdr);
    
    int read(NodeBase* node, Director* director);
    
private: 
    int parse(NodeBase* node, const Byte buf[],
        int len, Director* director);
    
    int parseHeader(SockBase* base, const MsgHdr* hdr);
    
    int parseBody(NodeBase* node, const Byte buf[],
        Int32 len, Director* director);
    
    int parseNextBody(NodeBase* node, const Byte buf[],
        Int32 len, Director* director);

    Void dispatch(Director* director, NodeBase* base, SockBase* sock);
    
private: 
    Byte m_buf[DEF_TCP_RCV_BUF_SIZE]; 
};

class MsgWriter {
public:
    int write(NodeBase* node);

private:
    int sendQue(Int32 fd, list_head* list);
    Bool prune(list_head* list, Int32 maxlen);

private: 
    struct iovec m_vec[DEF_VEC_SIZE];
};

class SockCommObj : public I_NodeObj {
public:
    explicit SockCommObj(Director* director);
    ~SockCommObj();
    
    virtual Int32 getType() const {
        return ENUM_NODE_SOCK;
    }
    
    /* return: 0-ok, other: error */
    virtual Uint32 readNode(struct NodeBase* base);

    /* return: 0-ok, 1-blocking write, other: error */
    virtual Uint32 writeNode(struct NodeBase* base);
    
    virtual Int32 procMsg(struct NodeBase* base, struct MsgHdr* msg);

    virtual Void procCmd(struct NodeBase* base, struct MsgHdr* msg);

    /* this function run in the dealer thread for resource free */
    virtual Void eof(struct NodeBase* base);

private:
    Director* m_director;
    MsgReader* m_reader;
    MsgWriter* m_writer;
};


class SockConnector : public I_NodeObj {
public:
    explicit SockConnector(Director* director) : m_director(director) {}
    
    virtual Int32 getType() const {
        return ENUM_NODE_SOCK_CONNECTOR;
    }
    
    /* return: 0-ok, other: error */
    virtual Uint32 readNode(struct NodeBase* base);

    /* return: 0-ok, 1-blocking write, other: error */
    virtual Uint32 writeNode(struct NodeBase* base);
    
    virtual Int32 procMsg(struct NodeBase* base, struct MsgHdr* msg);

    virtual Void procCmd(struct NodeBase* base, struct MsgHdr* msg);

    /* this function run in the dealer thread for resource free */
    virtual Void eof(struct NodeBase* base);

private:
    Void procConnect(NodeBase* base, struct MsgHdr* msg);

private:
    Director* m_director;
};

class SockListener : public I_NodeObj {
public:
    explicit SockListener(Director* director) : m_director(director) {}
    
    virtual Int32 getType() const {
        return ENUM_NODE_LISTENER;
    }
    
    /* return: 0-ok, other: error */
    virtual Uint32 readNode(struct NodeBase* base);

    /* return: 0-ok, 1-blocking write, other: error */
    virtual Uint32 writeNode(struct NodeBase* base);
    
    virtual Int32 procMsg(struct NodeBase* base, struct MsgHdr* msg);

    virtual Void procCmd(struct NodeBase* base, struct MsgHdr* msg);

    /* this function run in the dealer thread for resource free */
    virtual Void eof(struct NodeBase* base);

private:
    Int32 accept(struct NodeBase* base, MsgHdr* msg);

private:
    Director* m_director;
};

#endif

