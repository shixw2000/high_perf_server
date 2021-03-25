#include"sharedheader.h"
#include"sockdata.h"
#include"sockmonitor.h"
#include"socktool.h"
#include"sockconfdef.h"


SockConfDef::SockConfDef(SockMonitor* monitor) {
    m_monitor = monitor;
}

SockConfDef::~SockConfDef() {
}

int SockConfDef::onAccepted(const SockData* pData) {
    static int g_total_accpt_cnt = 0;
    int fd = pData->m_fd;
    int rcvsize = 0;
    int sndsize = 0;

    if (1 == (++g_total_accpt_cnt % 5000)) {
        rcvsize = SockTool::getRcvBufferSize(fd);
        sndsize = SockTool::getSndBufferSize(fd);
            
        LOG_INFO("onAccepted| total_accept_cnt=%d|"
            " rcv_buf_size=%d| snd_buf_size=%d|",
            g_total_accpt_cnt, rcvsize, sndsize);
    }
    
    return 0;
}

int SockConfDef::onConnected(const SockData* pData) {
    static int g_total_conn_cnt = 0;
    int fd = pData->m_fd;
    int rcvsize = 0;
    int sndsize = 0;

    if (1 == (++g_total_conn_cnt % 5000)) {
        rcvsize = SockTool::getRcvBufferSize(fd);
        sndsize = SockTool::getSndBufferSize(fd);
            
        LOG_INFO("onConnected| total_conn_cnt=%d|"
            " rcv_buf_size=%d| snd_buf_size=%d|", 
            g_total_conn_cnt, rcvsize, sndsize);
    }
    
    return 0;
}

void SockConfDef::onFailConnect(const SockData* pData) {
    LOG_ERROR("onFailConnect| fd=%d| state=%d| data=%p|", 
        pData->m_fd, pData->m_state, pData);
} 

void SockConfDef::onClosed(const SockData* pData) {
    LOG_DEBUG("onClosed| fd=%d| id=%s| state=%d| data=%p|", 
        pData->m_fd, pData->m_id, pData->m_state, pData);
    return;
}

int SockConfDef::chkLogin(const SockData* pData, struct msg_type* pMsg) {    
    return 0;
}

int SockConfDef::process(SockData* pData, struct msg_type* pMsg) {
    int ret = 0;
    struct MsgHeader* pHd = NULL;
    int len = pMsg->m_size; 
 
    pHd = (struct MsgHeader*)pMsg->m_buf;
    LOG_DEBUG("====procMsg| fd=%d| ver=0x%x| crc=0x%x| id=%u| size=%d:%d|",
        pData->m_fd, pHd->m_ver, pHd->m_crc, 
        pHd->m_id, pHd->m_size, len);

    //ret = chkBody(pData, pMsg);
    if (0 == ret) {
        if (SOCK_STATE_ESTABLISHED == pData->m_state) {
            ret = chkLogin(pData, pMsg);
            if (0 == ret) {
                pData->m_state = SOCK_STATE_LOGIN;
            }
        }
            
        if (needReply()) {
            ret = m_monitor->sendMsg(pData, pMsg);
        } else {
            m_monitor->freeMsg(pMsg);
        } 
    } else {
        LOG_WARN("===processMsg| fd=%d| state=%d|"
            " len=%d| msg=chkbody error|",
            pData->m_fd, pData->m_state, len); 

        m_monitor->freeMsg(pMsg);
    }

    return ret;
}

int SockConfDef::chkBody(const SockData* pData, 
    const struct msg_type* pMsg) { 
    unsigned char csum = 0;
    const struct MsgHeader* pHd = (const struct MsgHeader*)pMsg->m_buf;
    
    csum = mkcrc(pHd+1, pMsg->m_size - DEF_MSG_HEADER_SIZE);
    if (pHd->m_csum == csum) {
        return 0;
    } else {
        LOG_WARN("chkBody| fd=%d| pkgsize=%d| csum=0x%x|",
            pData->m_fd, pMsg->m_size, csum);
        
        return RECV_ERR_BODY_CSUM;
    }
}
