#include"hlist.h"
#include"sharedheader.h"
#include"taskhandler.h"


TaskHandler::TaskHandler() {
}

TaskHandler::~TaskHandler() {
}

int TaskHandler::init() {
    int ret = 0;
    
    ret = EvService::init();
    if (0 != ret) {
        return ret;
    }

    ret = CPushPool::init();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void TaskHandler::finish() {    
    CPushPool::finish();
    EvService::finish(); 
}

int TaskHandler::produce(unsigned int ev, struct TaskElement* task) {
    int ret = 0;
    
    ret = CPushPool::produce(ev, task);
    if (0 < ret) {
        resume();
    }

    return ret;
}

bool TaskHandler::doService() {
    bool done = false;
    
    done = consume(); 
    return done;
}


