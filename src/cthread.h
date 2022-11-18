#ifndef __CTHREAD_H__
#define __CTHREAD_H__
#include"interfobj.h"


class CThread : public I_Service {
    struct _intern;
    
public:
    CThread();
    virtual ~CThread();

    unsigned long getThr() const;

    int start(const char name[], I_Service* service = NULL);
    
    void join(); 

    virtual int run() {
        int ret = 0;
        
        if (NULL != m_service) {
            ret = m_service->run();
        }

        return ret;
    }

private:
    static void* activate(void* arg);
    
private:
    struct _intern* m_intern;
    I_Service* m_service;
};


#endif

