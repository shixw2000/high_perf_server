#ifndef __SERVICE_H__
#define __SERVICE_H__
#include"task.h" 
#include"cthread.h"


class BinaryDoor;

class CService : public CThread {
public:
    CService();
    virtual ~CService();
    virtual int init();
    virtual void finish();
    
    void resume();
    
protected: 
    virtual int run();

    /* return true if all of the services may have been done */
    virtual bool doService() = 0; 
    
    virtual void pause() = 0; 
    virtual void alarm() = 0;
    virtual void check() = 0;

private:
    BinaryDoor* m_door;
};

class SigService : public CService {
public:
    SigService() {}
    virtual ~SigService(){}

    virtual int init(); 
    virtual void finish();
    
protected:
    virtual void pause(); 
    virtual void alarm();
    virtual void check() {}
};

class CEvent;

class EvService : public CService {
public:
    EvService();
    ~EvService();

    virtual int init();
    virtual void finish();

protected:    
    virtual void alarm();
    virtual void pause(); 
    virtual void check() {}

private:
    CEvent* m_event;
};

#endif

