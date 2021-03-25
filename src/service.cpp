#include"sharedheader.h"
#include"cthread.h"
#include"statemachine.h"
#include"service.h"
#include"socktool.h"


CService::CService() {
    m_door = NULL;
}

CService::~CService() {
}

int CService::init() {
    m_door = new BinaryDoor;
    if (NULL != m_door) {
        return 0;
    } else {
        return -1;
    }
}

void CService::finish() {
    if (NULL != m_door) {
        delete m_door;
        m_door = NULL;
    }
}

void CService::resume() {
    bool needFire = false;

    needFire = m_door->login();
    if (needFire) {
        this->alarm();
    }
}

int CService::run() { 
    bool hasFinished = true;
    bool isOut = true;

    m_door->lock();
    while (isRun()) {
        hasFinished = doService();
        if (hasFinished) {
            isOut = m_door->logout();
            if (isOut) {
                LOG_VERB("==service_run| name=%s| msg=start to pause|", 
                    m_name);
                
                this->pause();
            } else { 
                check();
            }

            /* restart the door state */
            m_door->lock();
        } else {
            /* go to the next round */
            check();
        }
    }

    return 0;
}

int SigService::init() {
    int ret = 0;
    
    threadMaskSig();

    ret = CService::init();
    if (0 != ret) {
        return ret;
    }
    
    return 0;
}

void SigService::finish() {
    CService::finish();
}

void SigService::alarm() {
    LOG_VERB("===SigService_alarm| name=%s|", m_name);
    testkill();
}

void SigService::pause() {
    threadSuspend();
    //sleepSec(1);
}

EvService::EvService() {
    m_event = NULL;
}

EvService::~EvService() {
    if (NULL != m_event) {
        delete m_event;
    }
}

int EvService::init() {
    int ret = 0;

    do {
        ret = CService::init();
        if (0 != ret) {
            break;
        }
        
        m_event = new CEvent(true);
        if (NULL == m_event) {
            ret = -1;
            break;
        }
        
        ret = m_event->create();
        if (0 != ret) {
            break;
        } 

        return 0;
    } while (false);
    
    return ret;
}

void EvService::finish() { 
    if (NULL != m_event) {
        delete m_event;
        m_event = NULL;
    }

    CService::finish();
}

void EvService::pause() {
    m_event->waitEvent(-1);
}

void EvService::alarm() {
    LOG_VERB("===EvService_alarm| name=%s|", m_name);
    m_event->putEvent();
}



