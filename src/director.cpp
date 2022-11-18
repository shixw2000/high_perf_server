#include"director.h"
#include"sockutil.h"
#include"sockepoll.h"
#include"receiver.h"
#include"sender.h"
#include"dealer.h"
#include"ticktimer.h"
#include"msgcenter.h"
#include"lock.h"
#include"msgtype.h"
#include"objcenter.h"
#include"config.h"


Director::Director(Int32 capacity) : m_capacity(capacity) {
    INIT_LIST_HEAD(&m_node_que);
    
    m_lock = NULL;
    m_timer = NULL;
    m_epoll = NULL;
    m_receiver = NULL;
    m_sender = NULL;
    m_dealer = NULL;

    m_event_node = NULL;
    m_timer_node = NULL;
    m_obj = NULL;
    m_cb = NULL;
}

Director::~Director() {
}

Int32 Director::init() {
    Int32 ret = 0;

    ret = TaskThread::init();
    if (0 != ret) {
        return ret;
    }

    I_NEW_1(GrpSpinLock, m_lock, DEF_LOCK_ORDER);
    ret = m_lock->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW(TickTimer, m_timer);
    m_timer->setDealer(this);

    I_NEW_2(SockEpoll, m_epoll, m_capacity, this);
    ret = m_epoll->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW_1(Receiver, m_receiver, this);
    ret = m_receiver->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW_1(Sender, m_sender, this);
    ret = m_sender->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW_1(Dealer, m_dealer, this);
    ret = m_dealer->init();
    if (0 != ret) {
        return ret;
    }

    ret = creatEvent();
    if (0 != ret) {
        return ret;
    }

    ret = creatTimer();
    if (0 != ret) {
        return ret;
    }

    I_NEW_1(ObjCenter, m_obj, this);
    ret = m_obj->init();
    if (0 != ret) {
        return ret;
    }

    return ret;
}

Void Director::finish() {
    freeNodes();
    
    if (NULL != m_dealer) {
        m_dealer->finish();
        I_FREE(m_dealer);
    }

    if (NULL != m_sender) {
        m_sender->finish();
        I_FREE(m_sender);
    }

    if (NULL != m_receiver) {
        m_receiver->finish();
        I_FREE(m_receiver);
    }

    if (NULL != m_epoll) {
        m_epoll->finish();
        I_FREE(m_epoll);
    } 

    if (NULL != m_event_node) {
        freeNode(m_event_node);
        m_event_node = NULL;
    }

    if (NULL != m_timer_node) {
        freeNode(m_timer_node);
        m_timer_node = NULL;
    }
    
    if (NULL != m_timer) {
        I_FREE(m_timer);
    }

    if (NULL != m_obj) {
        m_obj->finish();
        I_FREE(m_obj);
    }

    if (NULL != m_lock) {
        m_lock->finish();
        I_FREE(m_lock);
    }
    
    TaskThread::finish();
}

int Director::start() {
    Int32 ret = 0; 

    if (NULL == m_cb) {
        LOG_ERROR("***start_director| sock_cb=NULL|"
            " error=you must set sock_cb first!!");
        return -1;
    }

    ret = m_dealer->start("dealer");
    if (0 != ret) {
        return ret;
    }

    ret = m_sender->start("sender");
    if (0 != ret) {
        return ret;
    }

    ret = m_receiver->start("receiver");
    if (0 != ret) {
        return ret;
    }

    ret = TaskThread::start("director");
    if (0 != ret) {
        return ret;
    }

    return 0;
}

void Director::join() {
    TaskThread::join();

    m_dealer->stop();
    m_sender->stop();
    m_receiver->stop();

    m_dealer->join();
    m_sender->join();
    m_receiver->join();
}

void Director::stop() {
    if (isRunning()) {
        TaskThread::stop();
        m_dealer->stop();
        m_sender->stop();
        m_receiver->stop();
    }
}

Int32 Director::getSize() const {
    return m_epoll->getSize();
}

Uint32 Director::getTick() const {
    return m_timer->monoTick();
}

Uint32 Director::getTime() const {
    return m_timer->now();
}

Void Director::procTick(EnumThreadType type, Uint32 cnt) {
    switch (type) {
    case ENUM_THREAD_READ:
        m_receiver->doTick(cnt);
        break;

    case ENUM_THREAD_WRITE:
        m_sender->doTick(cnt);
        break;

    case ENUM_THREAD_DEALER:
        m_dealer->doTick(cnt);
        break;

    case ENUM_THREAD_DIRECTOR:
        doTick(cnt);
        break;

    default:
        break;
    }
}

Int32 Director::sendCmd(MsgHdr* msg) {
    Int32 ret = 0;

    ret = pushCmd(m_event_node, msg);
    return ret;
}

Int32 Director::sendMsg(NodeBase* base, MsgHdr* msg) {
    Int32 ret = 0;

    ret = pushWrite(base, msg);
    return ret;
}

Int32 Director::dispatch(NodeBase* base, MsgHdr* msg) {
    Int32 ret = 0;

    ret = pushDeal(base, msg);
    return ret;
}

Int32 Director::pushMsg(EnumThreadType type, NodeBase* base, MsgHdr* msg) {
    Int32 ret = 0;
    
    switch (type) { 
    case ENUM_THREAD_WRITE:
        ret = pushWrite(base, msg);
        break;

    case ENUM_THREAD_DEALER:
        ret = pushDeal(base, msg);
        break;

    case ENUM_THREAD_DIRECTOR:
        ret = pushCmd(base, msg);
        break;

    case ENUM_THREAD_READ:
    default:
        MsgCenter::freeMsg(msg);
        ret = -1;
        break;
    }

    return ret;
}

Int32 Director::pushWrite(NodeBase* base, MsgHdr* msg) {
    Bool bOk = TRUE;

    bOk = lock(base);
    if (bOk) {
        MsgCenter::queue(msg, &base->m_wr_que_tmp);
        
        unlock(base);

        m_sender->addTask(&base->m_wr_task, BIT_EVENT_NORM);

        return 0;
    } else {
        MsgCenter::freeMsg(msg);

        return -1;
    }
}

Int32 Director::pushDeal(NodeBase* base, MsgHdr* msg) {
    Bool bOk = TRUE;

    bOk = lock(base);
    if (bOk) {
        MsgCenter::queue(msg, &base->m_deal_que_tmp);
        
        unlock(base);

        m_dealer->addTask(&base->m_deal_task, BIT_EVENT_NORM);

        return 0;
    } else {
        MsgCenter::freeMsg(msg);

        return -1;
    }
}

Int32 Director::pushCmd(NodeBase* base, MsgHdr* msg) {
    Bool bOk = TRUE;

    bOk = lock(base);
    if (bOk) {
        MsgCenter::queue(msg, &base->m_cmd_que_tmp);
        
        unlock(base);

        addTask(&base->m_director_task, BIT_EVENT_NORM);

        return 0;
    } else {
        MsgCenter::freeMsg(msg);

        return -1;
    }
}

Void Director::notify(EnumThreadType type, NodeBase* base, 
    Uint16 cmd, Uint64 data) {
    MsgHdr* hdr = NULL;
    MsgNotify* body = NULL;

    hdr = MsgCenter::creatMsg<MsgNotify>(cmd);
    if (NULL != hdr) {
        body = MsgCenter::body<MsgNotify>(hdr);

        body->m_data = data;

        pushMsg(type, base, hdr);
    }
}

NodeBase* Director::creatNode(Int32 fd, Int32 type, Int32 status) {
    NodeBase* node = NULL;

    node = buildNode(fd, type);
    node->m_fd_status = status;

    list_add_back(&node->m_node, &m_node_que); 
    return node;
}

NodeBase* Director::creatNode(Int32 fd, Int32 type, Int32 status,
    const Char* ip, Int32 port) {
    NodeBase* node = NULL;
    SockBase* sock = NULL;

    node = creatNode(fd, type, status);
    sock = getSockBase(node);
    if (NULL != sock) {
        sock->m_port = port;
        strncpy(sock->m_ip, ip, DEF_IP_SIZE-1);
    }

    return node;
}

int Director::delNode(NodeBase* node) {
    list_del(&node->m_node, &m_node_que); 
    freeNode(node);

    return 0;
}

int Director::addEvent(Int32 ev_type, NodeBase* node) {
    Int32 ret = 0; 
    
    ret = m_epoll->addEvent(ev_type, node); 
    return ret;
}

int Director::delEvent(Int32 ev_type, NodeBase* node) {
    Int32 ret = 0; 

    ret = m_epoll->delEvent(ev_type, node); 
    return ret;
}

int Director::setup() { 
    return 0;
}

void Director::teardown() {
    if (NULL != m_timer) {
        m_timer->stop();
    }
}

Int32 Director::creatEvent() {
    int fd = -1;
    
    fd = creatEventFd(); 
    if (0 <= fd) { 
        m_event_node = buildNode(fd, ENUM_NODE_EVENT);
        m_event_node->m_fd_status = ENUM_FD_NORMAL;

        m_epoll->addEvent(EVENT_TYPE_RD, m_event_node);

        LOG_INFO("++++creat_event| fd=%d| msg=ok|", fd);

        return 0;
    } else {
        LOG_ERROR("****creat_event| msg=error|");
        
        return -1;
    }
}

Int32 Director::creatTimer() {
    int fd = -1;
    
    fd = creatTimerFd(1000); 
    if (0 <= fd) { 
        m_timer_node = buildNode(fd, ENUM_NODE_TIMER);
        m_timer_node->m_fd_status = ENUM_FD_NORMAL;

        m_epoll->addEvent(EVENT_TYPE_RD, m_timer_node);

        LOG_INFO("++++creat_timer| fd=%d| msg=ok|", fd); 
        
        return 0;
    } else {
        LOG_ERROR("****creat_timer| msg=error|");
        
        return -1;
    }
}

Void Director::doTimeout(struct TimerEle*) {
}

Void Director::alarm() {
    writeEvent(m_event_node->m_fd);
}

Void Director::wait() {
    m_epoll->waitEvent(DEF_EPOLL_WAIT_MILI_SEC);
}

Void Director::check() {
    m_epoll->waitEvent(0);
}

unsigned int Director::procTask(struct Task* task) {
    NodeBase* base = NULL;

    base = list_entry(task, NodeBase, m_director_task);

    procNodeQue(base);
    
    return BIT_EVENT_NORM; 
}

void Director::procTaskEnd(struct Task* task) {
    NodeBase* base = NULL;

    base = list_entry(task, NodeBase, m_director_task);

    base->m_fd_status = ENUM_FD_CLOSE_DIRECTOR;

    LOG_INFO("proc_task_end| fd=%d| type=%d| msg=finished|",
        base->m_fd, base->m_node_type);

    m_obj->eof(base); 
    return;
}

Void Director::procNodeQue(NodeBase* base) {
    list_head* list = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    MsgHdr* msg = NULL;
    Bool bOk = TRUE;

    bOk = lock(base);
    if (bOk) {
        if (!list_empty(&base->m_cmd_que_tmp)) {
            list_splice_back(&base->m_cmd_que_tmp, &base->m_cmd_que);
        }
        
        unlock(base);
    }
    
    list = &base->m_cmd_que;
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            msg = MsgCenter::cast(pos);
            dealCmd(base, msg);
        }
    }
}

Void Director::dealCmd(NodeBase* base, MsgHdr* msg) {
    if (ENUM_MSG_SYSTEM_STOP != msg->m_cmd) {
        m_obj->procCmd(base, msg);
    } else {
        procStopMsg(base, msg);
    } 
}

Void Director::procStopMsg(NodeBase* base, MsgHdr* msg) {
    Int32 status = 0;
    MsgNotify* body = MsgCenter::body<MsgNotify>(msg);

    status = (Int32)body->m_data;
    if (ENUM_FD_CLOSING == status) {
        if (base->m_fd_status < status) {
            base->m_fd_status = ENUM_FD_CLOSING;
        
            delEvent(EVENT_TYPE_ALL, base);
            
            m_receiver->endTask(&base->m_rd_task);
        }
    } else if (ENUM_FD_CLOSE_RD == status) {
        base->m_fd_status = status;
        
        m_sender->endTask(&base->m_wr_task);
    } else if (ENUM_FD_CLOSE_WR == status) {
        base->m_fd_status = status;
            
        m_dealer->endTask(&base->m_deal_task);
    } else if (ENUM_FD_CLOSE_DEALER == status) {
        base->m_fd_status = status;
        
        /* this go to myself end */
        endTask(&base->m_director_task);
    } else {
        /* invalid */
    }

    MsgCenter::freeMsg(msg);
}

Void Director::close(NodeBase* base) {
    /*do some thing */

    notify(ENUM_THREAD_DIRECTOR, base, ENUM_MSG_SYSTEM_STOP, ENUM_FD_CLOSING);
}

Bool Director::lock(NodeBase* base) {
    int idx = base->m_fd;

    return m_lock->lock(idx);
}

Bool Director::unlock(NodeBase* base) {
    int idx = base->m_fd;

    return m_lock->unlock(idx);
}

Void Director::addReadable(struct NodeBase* base) {
    m_receiver->addTask(&base->m_rd_task, BIT_EVENT_READ);
}

Void Director::addWriteable(struct NodeBase* base) {
    m_sender->addTask(&base->m_wr_task, BIT_EVENT_WRITE);
}

Void Director::doTick(Uint32 cnt) {
    m_timer->tick(cnt); 
}

void Director::addTimer(struct TimerEle* ele, 
    Int32 type, Uint32 interval) {
    
    ele->m_type = type;
    ele->m_interval = interval;

    m_timer->addTimer(ele);
} 

Uint32 Director::readNode(struct NodeBase* base) {
    Uint32 ret = 0;

    if (!base->m_rd_err) {
        ret = m_obj->readNode(base);

        return ret;
    } else {
        close(base);
        return BIT_EVENT_ERROR;
    } 
}

Uint32 Director::writeNode(struct NodeBase* base) {
    Uint32 ret = 0;

    if (!base->m_wr_err) {
        ret = m_obj->writeNode(base);

        return ret;
    } else {
        close(base);
        return BIT_EVENT_ERROR;
    } 
}

Void Director::dealMsg(struct NodeBase* base, struct MsgHdr* msg) {
    Int32 ret = 0;
    
    if (!base->m_deal_err) {
        ret = m_obj->procMsg(base, msg);
        if (0 != ret) {
            base->m_deal_err = TRUE;
            close(base);
        }
    } else {
        MsgCenter::freeMsg(msg);
    }
}

Int32 Director::addListener(TcpParam* param) {
    Int32 ret = 0;
    Int32 fd = -1;
    NodeBase* base = NULL; 

    do { 
        fd = creatTcpSrv(param);
        if (0 > fd) {
            ret = -2;
            break;
        } 

        base = creatNode(fd, ENUM_NODE_LISTENER, ENUM_FD_NORMAL);
        addEvent(EVENT_TYPE_RD, base);

        LOG_INFO("++++add_listener| fd=%d| addr=%s:%d| msg=ok|",
            fd, param->m_ip, param->m_port);

        return 0;
    } while (0);

    LOG_ERROR("****add_listener| addr=%s:%d| ret=%d| msg=error|",
        param->m_ip, param->m_port, ret);
    
    return ret;
}

Int32 Director::addConnector(TcpParam* param) {
    Int32 ret = 0;
    Int32 fd = -1;
    NodeBase* base = NULL;

    do { 
        ret = connFast(param, &fd);
        if (0 > ret) {            
            ret = -1;
            break;
        } 

        base = creatNode(fd, ENUM_NODE_SOCK_CONNECTOR, 
            ENUM_FD_CONNECTING, param->m_ip, param->m_port); 

        /* just add write event, add read while connect ok */
        addEvent(EVENT_TYPE_WR, base);


        LOG_INFO("++++add_connect| fd=%d| addr=%s:%d| msg=ok|",
            fd, param->m_ip, param->m_port);

        return 0;
    } while (0);

    LOG_ERROR("****add_listener| addr=%s:%d| ret=%d| msg=error|",
        param->m_ip, param->m_port, ret);
    
    return ret;
}

Int32 Director::addListener(const Char ip[], int port) {
    Int32 ret = 0;
    TcpParam param;

    ret = buildParam(ip, port, &param);
    if (0 != ret) { 
        LOG_ERROR("****add_listener| addr=%s:%d| ret=%d| msg=parse error|",
            ip, port, ret);

        return ret;
    }

    ret = addListener(&param);
    return ret;
}

Int32 Director::addConnector(const Char ip[], int port) {
    Int32 ret = 0;
    TcpParam param;

    ret = buildParam(ip, port, &param);
    if (0 != ret) { 
        LOG_ERROR("****add_connect| addr=%s:%d| ret=%d| msg=parse error|",
            ip, port, ret);

        return ret;
    }

    ret = addConnector(&param);
    return ret;
}

Void Director::freeNodes() {
    list_node* pos = NULL;
    list_node* n = NULL;
    NodeBase* node = NULL;
    list_head* list = &m_node_que;

    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            node = list_entry(pos, NodeBase, m_node);
            
            freeNode(node);
        }
    }
}

