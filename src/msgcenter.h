#ifndef __MSGCENTER_H__
#define __MSGCENTER_H__
#include"globaltype.h"
#include"listnode.h"
#include"msgtype.h"


class CacheCenter { 
    friend class MsgCenter;
    
    struct _internal;
    
public: 
   static Void* prepend(Int32 capacity);
   static Void free(Void* dummy);

    static void push_back(Void* dummy, list_head*);
    static void push_front(Void* dummy, list_head*);

    /* reset pos and size of this buffer */
    static Void rewind(Void* dummy);

    /* set size=pos, and pos=0 for the next read */
    static Void flip(Void* dummy);

    /* add n with pos */
    static Void skip(Void* dummy, Int32 n);

    /* set pos = n */
    static Void set(Void* dummy, Int32 n);

    static Void* cast(list_node* node);

    /* left space to fill data */
    static Int32 left(Void* dummy);

    /* data not used */
    static Int32 remain(Void* dummy);
        
    static Int32 pos(Void* dummy);
    static Int32 size(Void* dummy);
    static Int32 capacity(Void* dummy);

    static Void* buffer(Void* dummy);

    static Void append(Void* dummy, const Void* buf, Int32 len);
    static Void copy(Void* dummy, const Void* buf, Int32 len);
    
private: 
    static _internal* to(Void* dummy); 
    static Void reset(_internal* intern);
};


struct MsgHdr;

class MsgCenter {
public:
    template<typename T>
    static MsgHdr* creatMsg(Uint16 cmd, Int32 extlen = 0) {
        MsgHdr* msg = NULL;
        Int32 size = 0;

        size = sizeof(MsgHdr) + sizeof(T) + extlen;
        msg = creat(size);
        msg->m_cmd = cmd;
        
        return msg;
    }

    static MsgHdr* creat(Int32 size);

    static Void freeMsg(MsgHdr* msg);

    template<typename T>
    static T* body(MsgHdr* hdr) {
        Void* msg = up(hdr);
        
        return (T*)msg;
    }

    static MsgHdr* cast(list_node* node);

    static void queue(MsgHdr* msg, list_head* list);

    static Void printMsg(const Char* prompt, MsgHdr* msg);

private: 
    static Void* up(MsgHdr* msg);
};

#endif

