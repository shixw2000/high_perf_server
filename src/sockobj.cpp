#include"sockobj.h"
#include"msgcenter.h"
#include"msgtype.h"
#include"sockutil.h"
#include"director.h"


int MsgReader::read(NodeBase* node, Director* director) {
    Int32 ret = 0;
    int maxlen = 0;
    int rdlen = 0;
    Int32 fd = node->m_fd; 

    maxlen = DEF_TCP_RCV_BUF_SIZE;
    rdlen = recvTcp(fd, m_buf, maxlen);
    if (0 < rdlen) {
        ret = parse(node, m_buf, rdlen, director);
        if (0 == ret) {
            /* may have data to read */
            return 1;
        } else {
            return -1;
        }
    } else if (0 == ret) {
        return 0;
    } else {
        return -1;
    }
}

Bool MsgReader::chkHeader(const MsgHdr* hdr) {
    if (MAX_MSG_SIZE > hdr->m_size 
        && DEF_MSG_HEADER_SIZE < hdr->m_size
        && DEF_MSG_VER == hdr->m_version
        && !hdr->m_crc) {
        
        return true;
    } else {
        LOG_INFO("chk_msg_hdr| cmd=0x%x| ver=0x%x|"
            " crc=%u| size=%d| msg=invalid msg|",
            hdr->m_cmd, hdr->m_version, 
            hdr->m_crc, hdr->m_size);
        
        return false;
    }
}

int MsgReader::parse(NodeBase* node, const Byte buf[], int len, 
    Director* director) {
    Int32 ret = 0;
    Int32 left = 0;
    SockBase* base = getSockBase(node);
    
    if (base->m_rdlen < DEF_MSG_HEADER_SIZE) {
        
        left = DEF_MSG_HEADER_SIZE - base->m_rdlen; 
        if (left <= len) {
            memcpy(&base->m_buf[base->m_rdlen], buf, left);

            base->m_rdlen += left;
            buf += left;
            len -= left;

            ret = parseHeader(base, &base->m_hdr);
            if (0 == ret) {
                CacheCenter::append(base->m_rcv_dummy, 
                    base->m_buf, DEF_MSG_HEADER_SIZE); 

                ret = parseBody(node, buf, len, director); 
                return ret;
            } else {
                return ret;
            }
        } else {
            memcpy(&base->m_buf[base->m_rdlen], buf, len);

            base->m_rdlen += len;

            return 0;;
        }

    } else { 
        ret = parseBody(node, buf, len, director); 
        return ret;
    } 
}

int MsgReader::parseHeader(SockBase* base, const MsgHdr* hdr) {
    Bool bOk = TRUE;
    Int32 msglen = 0;
    MsgHdr* msg = NULL;

    bOk = chkHeader(hdr);
    if (bOk) {
        msglen = hdr->m_size;
        
        msg = MsgCenter::creat(msglen);
        if (NULL != msg) {
            base->m_rcv_dummy = msg;

            return 0;
        } else {
            return -2;
        }
    } else {
        return -1;
    }
}

int MsgReader::parseBody(NodeBase* node, const Byte buf[],
    Int32 len, Director* director) {
    Int32 ret = 0;
    Int32 left = 0;
    SockBase* sock = getSockBase(node);

    left = CacheCenter::left(sock->m_rcv_dummy);
    if (0 < left) {
        if (left <= len) {
            CacheCenter::append(sock->m_rcv_dummy, buf, left); 

            buf += left;
            len -= left;
                        
            dispatch(director, node, sock); 
            
            ret = parseNextBody(node, buf, len, director);
            return ret;
        } else {
            CacheCenter::append(sock->m_rcv_dummy, buf, len);

            return 0;
        }
    } else {
        dispatch(director, node, sock);

        ret = parseNextBody(node, buf, len, director);
        return ret;
    }
}

int MsgReader::parseNextBody(NodeBase* node, const Byte buf[],
    Int32 len, Director* director) {
    Int32 ret = 0;
    Int32 left = 0;
    const MsgHdr* hdr = NULL;
    SockBase* sock = getSockBase(node);

    while (0 < len) {
        if (DEF_MSG_HEADER_SIZE <= len) {
            hdr = (const MsgHdr*)buf;
            
            ret = parseHeader(sock, hdr);
            if (0 == ret) {
                left = CacheCenter::left(sock->m_rcv_dummy); 
                if (left <= len) {
                    CacheCenter::append(sock->m_rcv_dummy, buf, left);
                    buf += left;
                    len -= left;

                    dispatch(director, node, sock); 
                } else { 
                    CacheCenter::append(sock->m_rcv_dummy, buf, len); 
                    sock->m_rdlen = DEF_MSG_HEADER_SIZE;
                    
                    len = 0;
                } 
            } else {
                return ret;
            } 
        } else {
            memcpy(sock->m_buf, buf, len);
            sock->m_rdlen = len;

            len = 0;
        }
    }

    return 0;
}

Void MsgReader::dispatch(Director* director, NodeBase* base, 
    SockBase* sock) {
    if (NULL != sock->m_rcv_dummy) {
        CacheCenter::flip(sock->m_rcv_dummy);
        
        director->dispatch(base, sock->m_rcv_dummy);

        sock->m_rcv_dummy = NULL;
        sock->m_rdlen = 0;
    }
}

int MsgWriter::write(NodeBase* node) {
    Int32 ret = 0;
    Int32 fd = node->m_fd; 
    list_head* list = &node->m_wr_que;

    ret = sendQue(fd, list);
    return ret;
}

int MsgWriter::sendQue(Int32 fd, list_head* list) {
    Int32 sndlen = 0;
    Int32 total = 0;
    Int32 cnt = 0;
    Int32 maxcnt = DEF_VEC_SIZE;
    Int32 maxlen = DEF_TCP_SEND_BUF_SIZE;
    Bool done = TRUE;
    Void* msg = NULL;
    list_node* pos = NULL;

    if (!list_empty(list)) {
        list_for_each(pos, list) {
            msg = CacheCenter::cast(pos);

            if (cnt < maxcnt && total < maxlen) {
                m_vec[cnt].iov_base = CacheCenter::buffer(msg);
                m_vec[cnt].iov_len = CacheCenter::remain(msg);
                
                total += m_vec[cnt].iov_len;
                ++cnt;
            } else {
                break;
            }
        }

        sndlen = sendVec(fd, m_vec, cnt, total);
        if (0 < sndlen) {
            done = prune(list, sndlen);
            if (done) {
                /* send all data in que */
                return 0;
            } else {
                return 2;
            }
        } else if (0 == sndlen) {
            /* send blocked */
            return 1;
        } else {
            return -1;
        }
    } else {
        /* empty list */
        return 0;
    }
}

Bool MsgWriter::prune(list_head* list, Int32 maxlen) {
    Int32 res = 0;
    Bool done = TRUE;
    Void* msg = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    
    list_for_each_safe(pos, n, list) {
        msg = CacheCenter::cast(pos);
        
        res = CacheCenter::remain(msg);
        if (res <= maxlen) {
            /* all of this msg has been sent */
            list_del(pos, list);

            /* release msg ptr */
            CacheCenter::free(msg);

            maxlen -= res;
        } else {
            CacheCenter::skip(msg, maxlen);

            maxlen = 0;
            done = FALSE;
            break;
        }
    }

    return done;
}


SockCommObj::SockCommObj(Director* director) : m_director(director) {
    I_NEW(MsgReader, m_reader);
    I_NEW(MsgWriter, m_writer);
}

SockCommObj::~SockCommObj() {
    I_FREE(m_reader);
    I_FREE(m_writer);
}

Uint32 SockCommObj::readNode(struct NodeBase* base) {
    Int32 ret = 0;

    ret = m_reader->read(base, m_director);
    if (0 == ret) {
        return BIT_EVENT_NORM;
    } else if (1 == ret) {
        return BIT_EVENT_IN;
    } else {
        base->m_rd_err = TRUE;
        m_director->close(base);

        return BIT_EVENT_ERROR;
    }
}

Uint32 SockCommObj::writeNode(struct NodeBase* base) {
    Int32 ret = 0;

    ret = m_writer->write(base);
    if (0 == ret) {
        /* write all */
        return BIT_EVENT_NORM;
    } else if (1 == ret) {
        /* write blocked */
        return BIT_EVENT_WRITE;
    } else if (2 == ret) {
        /* can write and has more */
        return BIT_EVENT_IN;
    } else {
        base->m_wr_err = TRUE;
        m_director->close(base);

        return BIT_EVENT_ERROR;
    }
}

Int32 SockCommObj::procMsg(struct NodeBase* base, struct MsgHdr* msg) {
    Int32 ret = 0;
    
    ret = m_director->getCb()->process(base, msg);
    return ret;
}

Void SockCommObj::procCmd(struct NodeBase*, struct MsgHdr* msg) {
    MsgCenter::freeMsg(msg);
}

Void SockCommObj::eof(struct NodeBase* base) { 
    m_director->getCb()->onClosed(base);
    
    shutdownHd(base->m_fd);
    m_director->delNode(base);
}


Uint32 SockConnector::readNode(struct NodeBase*) {
    return BIT_EVENT_NORM;
}

Uint32 SockConnector::writeNode(struct NodeBase* base) {
    Int32 ret = 0;

    ret = chkConnStatus(base->m_fd);
    if (0 == ret) { 
        LOG_DEBUG("chk_connection| fd=%d| msg=ok|", base->m_fd); 

        m_director->notify(ENUM_THREAD_DIRECTOR,
            base, ENUM_MSG_SOCK_CONNECT_STATUS, 0);
        
        return BIT_EVENT_NORM;
    } else {
        LOG_ERROR("chk_connection| fd=%d| msg=invalid|", base->m_fd);

        m_director->notify(ENUM_THREAD_DIRECTOR,
            base, ENUM_MSG_SOCK_CONNECT_STATUS, 1);
        
        base->m_wr_err = TRUE;
        return BIT_EVENT_ERROR;
    }
}

Int32 SockConnector::procMsg(struct NodeBase*, struct MsgHdr* msg) {
    MsgCenter::freeMsg(msg);

    return 0;
}

Void SockConnector::procCmd(struct NodeBase* base, struct MsgHdr* msg) {
    switch (msg->m_cmd) {
    case ENUM_MSG_SOCK_CONNECT_STATUS: 
        procConnect(base, msg);
        break;
    
    default:
        MsgCenter::freeMsg(msg);
        break;
    } 
}

Void SockConnector::eof(struct NodeBase* base) {        
    m_director->delNode(base);
}

Void SockConnector::procConnect(NodeBase* base, struct MsgHdr* msg) {
    Int32 ret = 0;
    MsgNotify* body = MsgCenter::body<MsgNotify>(msg);
    
    if (0 == body->m_data) {
        base->m_fd_status = ENUM_FD_NORMAL; 
        base->m_node_type = ENUM_NODE_SOCK;

        ret = m_director->getCb()->onConnected(base);
        if (0 == ret) { 
            m_director->addEvent(EVENT_TYPE_RD, base);
        } else {
            m_director->close(base);
        } 
    } else {
        m_director->getCb()->onFailConnect(base);

        m_director->close(base);
    }

    MsgCenter::freeMsg(msg);
}

Uint32 SockListener::readNode(struct NodeBase* base) {
    m_director->notify(ENUM_THREAD_DIRECTOR,
        base, ENUM_MSG_LISTENER_ACCEPT, 0);
    
    return BIT_EVENT_NORM;
}

Uint32 SockListener::writeNode(struct NodeBase* base) {
    freeMsgQue(&base->m_wr_que);
    return BIT_EVENT_NORM;
}

Int32 SockListener::procMsg(struct NodeBase*, struct MsgHdr* msg) {
    MsgCenter::freeMsg(msg);

    return 0;
}

Void SockListener::procCmd(struct NodeBase* base, struct MsgHdr* msg) {    
    if (ENUM_MSG_LISTENER_ACCEPT == msg->m_cmd) {
        accept(base, msg);
    } else {
        MsgCenter::freeMsg(msg);
    }
}

Void SockListener::eof(struct NodeBase* base) {    
    m_director->delNode(base);
}

Int32 SockListener::accept(struct NodeBase* base, MsgHdr* msg) { 
    Int32 ret = 0;
    Int32 newfd = -1;
    Int32 ok = 0;
    Int32 fail = 0;
    int peer_port = 0;
    Char peer_ip[DEF_IP_SIZE];
    NodeBase* node = NULL;

    newfd = acceptCli(base->m_fd);
    while (0 <= newfd) { 
        getPeerInfo(newfd, peer_ip, &peer_port);

        if (m_director->capacity() > m_director->getSize()) {
            
            node = m_director->creatNode(newfd, ENUM_NODE_SOCK,
                ENUM_FD_NORMAL, peer_ip, peer_port);
            
            ret = m_director->getCb()->onAccepted(node);
            if (0 == ret) { 
                m_director->addEvent(EVENT_TYPE_ALL, node); 
                
                LOG_DEBUG("socket_accept| fd=%d| addr=%s:%d| msg=ok|",
                    newfd, peer_ip, peer_port);
                
                ++ok; 
            } else {
                LOG_ERROR("socket_accept| fd=%d| addr=%s:%d|"
                    " msg=onAccept error|",
                    newfd, peer_ip, peer_port);
                
                ++fail;
                m_director->delNode(node);
            }
          
        } else {
            LOG_ERROR("socket_accept| fd=%d| addr=%s:%d| max_size=%d|"
                " curr_size=%d| error=exceed max sock size|",
                newfd, peer_ip, peer_port,
                m_director->capacity(), m_director->getSize());

            ++fail;
            closeHd(newfd);
        }
        
        newfd = acceptCli(base->m_fd);
    }

    LOG_INFO("listener_accpt| cmd=0x%x| ok_cnt=%d| fail_cnt=%d|",
        msg->m_cmd, ok, fail);

    MsgCenter::freeMsg(msg);

    return 0;
}

