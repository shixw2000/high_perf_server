#include"server.h"
#include"interfobj.h"
#include"msgcenter.h"
#include"director.h"
#include"config.h"
#include"sockutil.h"


MsgHdr* DefSockCb::creatMsg(Uint16 cmd, const Void* data, Int32 size) {
    MsgHdr* hdr = MsgCenter::creatMsg<MsgTest>(cmd, size);
    
    CacheCenter::skip(hdr, DEF_MSG_HEADER_SIZE);
    CacheCenter::append(hdr, data, size);
    CacheCenter::append(hdr, &DEF_DELIM_CHAR, 1);
    
    CacheCenter::flip(hdr);
    
    return hdr;
}

int DefSockCb::onAccepted(struct NodeBase* base) {
    SockBase* sock = getSockBase(base);

    LOG_INFO("on_accept| fd=%d| addr=%s:%d| msg=ok|",
        base->m_fd, sock->m_ip, sock->m_port);

    return 0;
}

int DefSockCb::onConnected(struct NodeBase* base) {
    MsgHdr* msg = NULL;
    SockBase* sock = getSockBase(base);
    
    LOG_INFO("on_connected| fd=%d| addr=%s:%d| msg=ok|",
        base->m_fd, sock->m_ip, sock->m_port);

    for (int i=0; i<m_snd_cnt; ++i) {
        msg = creatMsg(ENUM_MSG_TEST, sock, 1024);
        m_server->sendMsg(base, msg);
    }

    return 0;
}

void DefSockCb::onFailConnect(struct NodeBase* base) {
    SockBase* sock = getSockBase(base);
        
    LOG_INFO("on_connect_fail| fd=%d| addr=%s:%d| msg=error|",
        base->m_fd, sock->m_ip, sock->m_port);
}

void DefSockCb::onClosed(struct NodeBase* base) {
    SockBase* sock = getSockBase(base);
    
    LOG_INFO("on_closed| fd=%d| addr=%s:%d| msg=closed|",
        base->m_fd, sock->m_ip, sock->m_port);
}

Int32 DefSockCb::process(struct NodeBase* base, struct MsgHdr* msg) {
    MsgCenter::printMsg("proc_msg", msg);

    if (m_reply) {
        m_server->sendMsg(base, msg);
    } else {
        MsgCenter::freeMsg(msg);
    } 

    ++m_cnt;
    if (m_cnt >> m_prnt_order) {
        LOG_INFO("proc_msg| mono_time=%u| cnt=%llu|",
            getMonoTime(), m_cnt);

        m_cnt = 0;
    } 

    return 0;
}


Server::Server() {
    m_parser = NULL;
    m_path = NULL;
    m_director = NULL;
    m_cb = NULL;
}

Server::~Server() {
}

Int32 Server::init() {
    Int32 ret = 0;
    Config* conf = NULL;

    I_NEW(Parser, m_parser);
    ret = m_parser->init(m_path);
    if (0 == ret) {
        conf = m_parser->getConf();
    } else {
        return ret;
    } 

    setLogLevel(conf->m_log_level);
    setLogStdin(conf->m_log_stdin);

    if (0 != strncmp(conf->m_passwd, "123456", MAX_PIN_PASSWD_SIZE)) {
        return -1;
    }
    
    I_NEW_1(Director, m_director, DEF_FD_MAX_CAPACITY); 
    ret = m_director->init();
    if (0 != ret)  {
        return ret;
    }

    I_NEW_1(DefSockCb, m_cb, this);
    setSockCb(m_cb);
    m_cb->setParam(conf->m_reply, conf->m_prnt_order, conf->m_snd_cnt);

    return ret;
}

Void Server::finish() {
    if (NULL != m_director) {
        m_director->finish();
        I_FREE(m_director);
    }
    
    I_FREE(m_cb);
    
    if (NULL != m_parser) {
        m_parser->finish();

        I_FREE(m_parser);
    }

    I_FREE(m_cb);
}

Void Server::setSockCb(SockCb* cb) {
    m_director->setSockCb(cb);
}

int Server::start() {
    Int32 ret = 0; 

    ret = m_director->start();

    return ret;
}

void Server::join() {
    if (NULL != m_director) {
        m_director->join();
    }
}

void Server::stop() {
    if (NULL != m_director) {
        m_director->stop();
    }
}

Uint32 Server::getTick() const {
    return m_director->getTick();
}

Uint32 Server::getTime() const {
    return m_director->getTime();
}

Int32 Server::sendMsg(NodeBase* base, MsgHdr* msg) {
    Int32 ret = 0;

    ret = m_director->sendMsg(base, msg);
    return ret;
}

Int32 Server::startServer(const Char ip[], Int32 port) {
    Int32 ret = 0;

    ret = m_director->addListener(ip, port);
    return ret;
}

Int32 Server::startClient(const Char ip[], Int32 port) {
    Int32 ret = 0;
    Int32 fd = -1;
    NodeBase* base = NULL;
    TcpParam param;

    ret = buildParam(ip, port, &param);
    if (0 != ret) { 
        LOG_ERROR("****start_client| addr=%s:%d| ret=%d| msg=parse error|",
            ip, port, ret);

        return ret;
    }

    ret = connSlow(&param, &fd);
    if (0 > ret) {            
        LOG_ERROR("****start_client| addr=%s:%d| ret=%d| msg=connect error|",
            ip, port, ret);

        return ret;
    } 

    base = m_director->creatNode(fd, ENUM_NODE_SOCK_CONNECTOR, 
        ENUM_FD_CONNECTING, param.m_ip, param.m_port); 

    /* just add write event, add read while connect ok */
    m_director->addEvent(EVENT_TYPE_WR, base);

    LOG_INFO("++++add_connect| fd=%d| addr=%s:%d| msg=ok|",
        fd, param.m_ip, param.m_port); 
    
    return ret;
}

Int32 Server::addService(const Char ip[], Int32 port, Bool bServ) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgSockAddr* body = NULL;

    hdr = MsgCenter::creatMsg<MsgSockAddr>(ENUM_MSG_SYSTEM_ADD_ADDRESS);
    body = MsgCenter::body<MsgSockAddr>(hdr);

    body->m_is_serv = bServ;
    body->m_port = port;
    strncpy(body->m_ip, ip, DEF_IP_SIZE - 1);

    ret = m_director->sendCmd(hdr);
    if (0 == ret) {
        LOG_INFO("add_service| is_serv=%d| addr=%s:%d| msg=ok|",
            bServ, ip, port);
    } else {
        LOG_ERROR("add_service| is_serv=%d| addr=%s:%d| ret=%d| msg=error|",
            bServ, ip, port, ret);
    }

    return ret;
}


