#include<stdlib.h>#include<stdio.h>#include<string.h>#include<errno.h>#include"sharedheader.h"#include"sockdata.h"SockCache::SockCache(unsigned int capacity) 	: m_capacity(capacity) {	m_size = 0U;	m_pdata = NULL;}
SockCache::~SockCache() {}
int SockCache::createCache() {    int ret = 0;    m_pdata = (SockData*)calloc(m_capacity, sizeof(SockData));    return ret;}int SockCache::freeCache() {    int ret = 0;    m_size = 0U;    if (NULL != m_pdata) {        free(m_pdata);                m_pdata = NULL;    }    return ret;}SockData* SockCache::allocData(int fd, int type, void* data) {    static unsigned int seq = 0;    SockData* pData = NULL;    
	    if ((unsigned int)fd < m_capacity) {        pData = &m_pdata[fd];        memset(pData, 0, sizeof(*pData));        
	        pData->m_state = (short)SOCK_STATE_NONE;        pData->m_seq = ATOMIC_INC(&seq);        pData->m_fd = fd;        pData->m_type = type;        pData->m_data = data;     }        return pData;}SockData* SockCache::getData(int fd) {    SockData* pData = NULL;    
	    if ((unsigned int)fd < m_capacity) {        pData = &m_pdata[fd];        if (fd == pData->m_fd) {            return pData;        } else {            return NULL;        }    } else {        return NULL;    }}
int SockCache::freeData(SockData* data) {    memset(data, 0, sizeof(*data));    data->m_fd = -1;    data->m_state = SOCK_STATE_NONE;    return 0;}unsigned char mkcrc(const void* data, int size) {    unsigned char sum = 0;    const unsigned char* psz = (const unsigned char*)data;        for (int i=0; i<size; ++i) {        sum += psz[i];    }    return ~sum;}