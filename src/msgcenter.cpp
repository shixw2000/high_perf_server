#include"globaltype.h"
#include"listnode.h"
#include"msgcenter.h"


struct CacheCenter::_internal {
    list_node m_node;
    Int32 m_capacity;
    Int32 m_size;
    Int32 m_pos;
};

Void* CacheCenter::prepend(Int32 capacity) { 
    _internal* intern = NULL;
    Byte* buf = NULL;
    Int32 total = 0;

    total = sizeof(_internal) + capacity;
    ARR_NEW(Byte, total, buf);
    
    if (NULL != buf) {         
        intern = (_internal*)buf; 
        
        reset(intern);
        intern->m_capacity = capacity;

        return intern + 1;
    } else {
        return NULL;
    }
}

void CacheCenter::free(Void* dummy) { 
    if (NULL != dummy) {
        Byte* buf = (Byte*)dummy;
        
        buf -= sizeof(_internal); 
        ARR_FREE(buf);
    }
}

Void CacheCenter::reset(_internal* intern) { 
    memset(intern, 0, sizeof(_internal));

    INIT_LIST_NODE(&intern->m_node);
}

Void* CacheCenter::cast(list_node* node) {
    _internal* intern = NULL;

    intern = list_entry(node, _internal, m_node);
    return intern + 1;
}

CacheCenter::_internal* CacheCenter::to(Void* dummy) {
    Byte* buf = (Byte*)dummy;
        
    buf -= sizeof(_internal); 
    return (_internal*)buf;
}

void CacheCenter::push_back(Void* dummy, list_head* head) {
    _internal* intern = NULL;

    intern = to(dummy); 
    list_add_back(&intern->m_node, head); 
}

void CacheCenter::push_front(Void* dummy, list_head* head) {
    _internal* intern = NULL;

    intern = to(dummy); 
    list_add_front(&intern->m_node, head);
}

Void CacheCenter::rewind(Void* dummy) {
    _internal* intern = NULL;

    intern = to(dummy); 
    intern->m_pos = 0;
    intern->m_size = 0;
}

Void CacheCenter::flip(Void* dummy) {
    _internal* intern = NULL;

    intern = to(dummy);
    intern->m_size = intern->m_pos;
    intern->m_pos = 0;
}

Void CacheCenter::skip(Void* dummy, Int32 n) {
    _internal* intern = NULL;

    intern = to(dummy);
    intern->m_pos += n;
}

Void CacheCenter::set(Void* dummy, Int32 n) {
    _internal* intern = NULL;

    intern = to(dummy);
    intern->m_pos = n;
}

Void* CacheCenter::buffer(Void* dummy) {
    Byte* buf = (Byte*)dummy;
    _internal* intern = NULL;

    intern = to(dummy);
    return &buf[intern->m_pos];
}

Int32 CacheCenter::left(Void* dummy) {
    _internal* intern = NULL;

    intern = to(dummy);
    return intern->m_capacity - intern->m_pos;
}

Int32 CacheCenter::remain(Void* dummy) {
    _internal* intern = NULL;

    intern = to(dummy);
    return intern->m_size - intern->m_pos;
}

Int32 CacheCenter::pos(Void* dummy) {
    _internal* intern = NULL;

    intern = to(dummy);
    return intern->m_pos;
}

Int32 CacheCenter::size(Void* dummy) {
    _internal* intern = NULL;

    intern = to(dummy);
    return intern->m_size;
}

Int32 CacheCenter::capacity(Void* dummy) {
    _internal* intern = NULL;

    intern = to(dummy);
    return intern->m_capacity;
}

Void CacheCenter::append(Void* dummy, const Void* buf, Int32 len) {
    Byte* psz = (Byte*)dummy;
    _internal* intern = NULL;

    intern = to(dummy);
    memcpy(&psz[intern->m_pos], buf, len);

    intern->m_pos += len;
}

Void CacheCenter::copy(Void* dummy, const Void* buf, Int32 len) {
    Byte* psz = (Byte*)dummy;
    _internal* intern = NULL;

    intern = to(dummy);
    memcpy(&psz[intern->m_pos], buf, len);
}


MsgHdr* MsgCenter::creat(Int32 size) {
    MsgHdr* msg = NULL;
    
    msg = (MsgHdr*)CacheCenter::prepend(size);
    if (NULL != msg) {
        memset(msg, 0, sizeof(*msg));
        
        msg->m_version = DEF_MSG_VER;
        msg->m_size = size;

        return msg;
    } else {
        return NULL;
    } 
}

Void MsgCenter::freeMsg(MsgHdr* msg) {
    CacheCenter::free(msg);
}

Void* MsgCenter::up(MsgHdr* msg) {
    return (msg+1);
}

void MsgCenter::queue(MsgHdr* msg, list_head* list) { 
    CacheCenter::push_back(msg, list);
}

MsgHdr* MsgCenter::cast(list_node* node) {
    Void* msg = CacheCenter::cast(node);

    return (MsgHdr*)msg;
}

Void MsgCenter::printMsg(const Char* prompt, MsgHdr* msg) {
    CacheCenter::_internal* intern = NULL;

    intern = CacheCenter::to(msg);
    
    LOG_DEBUG("<%s>_print_msg| capacity=%d| size=%d| pos=%d|"
        " hdr_size=%d| hdr_seq=0x%x| hdr_crc=0x%x|"
        " hdr_version=0x%x| hdr_cmd=0x%x|",
        prompt,
        intern->m_capacity, intern->m_size, intern->m_pos,
        msg->m_size, msg->m_seq, msg->m_crc, 
        msg->m_version, msg->m_cmd);
}

