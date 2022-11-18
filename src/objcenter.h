#ifndef __OBJCENTER_H__
#define __OBJCENTER_H__
#include"globaltype.h"
#include"interfobj.h"
#include"datatype.h"


class Director;

class ObjCenter : public I_NodeObj { 
public:
    explicit ObjCenter(Director* director);
    ~ObjCenter();

    virtual Int32 init();
    virtual Void finish();
    
    virtual Int32 getType() const {
        return ENUM_NODE_OBJ_CENTER;
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
    I_NodeObj* findObj(Int32 type);

private:
    Director* m_director;
    I_NodeObj* m_obj[ENUM_NODE_OBJ_MAX];
};


#endif

