#include"sharedheader.h"
#include"sockdata.h"
#include"msgpool.h"
#include"lock.h"


struct msg_type* MsgQueOper::allocMsg(int len) {
    int capacity = 0;
    struct msg_type* pMsg = NULL;

    capacity = len + sizeof(struct msg_type);
    pMsg = (struct msg_type*)malloc(capacity);
    if (NULL != pMsg) {
        resetMsg(pMsg);
        pMsg->m_capacity = len;
        pMsg->m_size = len;
        pMsg->m_buf[len] = DEF_MSG_TAIL_CHAR;

        LOG_VERB("==allocMsg| data=%p| size=%d| msg=ok|", pMsg, pMsg->m_size);
        return pMsg;
    } else {
        return NULL;
    }
}

bool MsgQueOper::freeMsg(struct msg_type* pMsg) {
    bool bOk = true;
    short ref =0;    

    ref = ATOMIC_DEC(&pMsg->m_ref);
    if (0 == ref) {
        bOk = verify(pMsg);
        if (bOk) {
            free(pMsg);
        } else {
            LOG_WARN("!!!!!!!!!!!!!freeMsg| data=%p| len=%d|"
                " msg=verify invalid msg|", 
                pMsg, pMsg->m_size);
        }
    } else {
        LOG_WARN("!!!!!!!!!!!!!freeMsg| data=%p| size=%d| ref=%d| msg=ok|", 
            pMsg, pMsg->m_size, ref);
    }

    return bOk;
}

void MsgQueOper::resetMsg(struct msg_type* pMsg) {
    memset(pMsg, 0, sizeof(*pMsg));

    pMsg->m_ref = (short)1;
}

void MsgQueOper::refMsg(struct msg_type* pMsg) {
    ATOMIC_INC(&pMsg->m_ref);
}

bool MsgQueOper::verify(const struct msg_type* pMsg) {
    if (DEF_MSG_TAIL_CHAR == pMsg->m_buf[ pMsg->m_size ]) {
        return true;
    } else {
        return false;
    }
}

int MsgQueOper::size(struct deque_root* root) {
    return ACCESS_ONCE(root->m_size);
}

MsgBaseQue::MsgBaseQue() {
    m_pool = NULL;
    m_oper = NULL;
}

MsgBaseQue::~MsgBaseQue() {
}

int MsgBaseQue::init() {
    int ret = 0;

    do {
        m_pool = new ElePool(MAX_MSG_POOL_SIZE);
        if (NULL == m_pool) {
            ret = -1;
            break;
        }

        ret = m_pool->init();
        if (0 != ret) {
    		break;
    	}
            
        m_oper = GrpQueFactory::create(DEF_USE_LOCK_TYPE); 
        if (NULL == m_oper) {
            ret = -1;
            break;
        }

        return 0;
    } while (false);
    
    return ret;
}

void MsgBaseQue::finish() {    
    if (NULL != m_oper) {
        GrpQueFactory::release(m_oper);
        m_oper = NULL;;
    }

    if (NULL != m_pool) {
        m_pool->finish();
        
        delete m_pool;
        m_pool = NULL;
    }
}

int MsgBaseQue::initQue(struct deque_root* root) {
    struct AtomEle* ele = NULL;
    struct hlist* list = NULL;

    ele = m_pool->alloc();
    if (NULL != ele) {
        m_pool->setType(ele, ELE_DATA_NONE);

        list = &ele->m_list;
        list->next = NULL;

        root->head = list;
        root->tail = list; 
        return 0;
    } else {
        LOG_WARN("init_msg_que| error=alloc fail|");
        return SOCK_ADD_INIT_ERR;
    }
}

void MsgBaseQue::finishQue(struct deque_root* root) {
    if (NULL != root->head && root->head == root->tail) {
        releaseEle(root->head);
        root->head = NULL;
        root->tail = NULL;
    }
}

void MsgBaseQue::releaseEle(struct hlist* list) {
    m_pool->release(list);
}

void MsgBaseQue::releaseData(struct AtomEle* ele) {
    struct msg_type* pMsg = NULL;

    if (ELE_DATA_MSG == m_pool->getType(ele)) {
        if (NULL != ele->m_data) {
            pMsg = containof((char*)(ele->m_data), struct msg_type, m_beg);

            MsgQueOper::freeMsg(pMsg);
            ele->m_data = NULL;
        } else {
            LOG_WARN("delete_send_msg| ele=%p| data=%p| size=%d|"
                " error=delete null msg|",
                ele, ele->m_data, ele->m_size);
        } 
    }
}

void MsgBaseQue::deleteAllMsg(int n, 
    struct deque_root* root, CalcListener* cb) {
    int cnt = 0;
    struct hlist* head = NULL;
    struct hlist* tail = NULL;
    struct hlist* item = NULL;
    struct AtomEle* ele = NULL;
  
    head = this->head(n, root);
    tail = this->tail(n, root);

    if (head != tail) {
        for (; head!=tail; head=item) {
            item = head->next;

            ele = containof(item, struct AtomEle, m_list);

            cb->calcMsg(COMMON_ERR_SOCK_END, ele->m_size);
            this->releaseData(ele);
            this->releaseEle(head);
            ++cnt;
        }

        this->setHead(n, cnt, head, root);
    }
}

bool MsgBaseQue::push(int n, struct deque_root* root, struct AtomEle* ele) {
    bool bOk = true;
    struct hlist* list = NULL;
        
    list = &ele->m_list;
    list->next = NULL;
    bOk = m_oper->push(n, root, list);
    
    return bOk;
}

bool MsgBaseQue::pushMsg(int n, struct deque_root* root,
    struct msg_type* pMsg) {
    bool bOk = true;
    struct AtomEle* ele = NULL;

    ele = m_pool->alloc();
    if (NULL != ele) { 
        m_pool->setType(ele, ELE_DATA_MSG);
        m_pool->setData(ele, pMsg->m_buf, pMsg->m_size);
        
        bOk = push(n, root, ele);
        if (!bOk) {
            LOG_WARN("push_msg| msg=%p| len=%d| error=push fail|",
                pMsg, pMsg->m_size);

            releaseEle(&ele->m_list);
            MsgQueOper::freeMsg(pMsg); 
        }
    } else { 
        LOG_WARN("push_msg| msg=%p| len=%d| error=alloc fail|",
            pMsg, pMsg->m_size); 
        bOk = false;

        MsgQueOper::freeMsg(pMsg);
    }

    return bOk;
}

bool MsgBaseQue::pushData(int n, struct deque_root* root, 
    char* data, int len) {
    bool bOk = true;
    struct AtomEle* ele = NULL;

    ele = m_pool->alloc();
    if (NULL != ele) { 
        m_pool->setType(ele, ELE_DATA_BUFFER);
        m_pool->setData(ele, data, len);
        
        bOk = push(n, root, ele);
        if (!bOk) {
            LOG_WARN("push_data| data=%p| len=%d| error=push fail|",
                data, len); 

            releaseEle(&ele->m_list);
        }
    } else { 
        LOG_WARN("push_data| data=%p| len=%d| error=alloc fail|",
            data, len); 
        bOk = false;
    }

    return bOk;
}

struct hlist* MsgBaseQue::head(int n, struct deque_root* root) {
    return root->head;
}

struct hlist* MsgBaseQue::tail(int n, struct deque_root* root) {
    struct hlist* tail = NULL;

    tail = ACCESS_ONCE(root->tail);
    return tail;
}

int MsgBaseQue::setHead(int n, int cnt, struct hlist* list,
    struct deque_root* root) {
    root->head = list;
    ATOMIC_SUB(&root->m_size, cnt);
    return 0;
}

struct msg_type* MsgBaseQue::pop(int n, struct deque_root* root) {
    struct hlist* tail = NULL;
    struct hlist* cur = NULL;
    struct hlist* prev = NULL;
    struct msg_type* pMsg = NULL;
    struct AtomEle* ele = NULL;

    tail = ACCESS_ONCE(root->tail);
    
    if (tail != root->head) {
        prev = root->head;
        cur = prev->next;
        root->head = cur;

        releaseEle(prev);
        ATOMIC_DEC(&root->m_size);
        
        ele = containof(cur, struct AtomEle, m_list);
        if (ELE_DATA_MSG == m_pool->getType(ele)) {
            pMsg = containof((char*)(ele->m_data), struct msg_type, m_beg);

            return pMsg;
        } else {
            LOG_WARN("pop_recv_que| ele=%p| data=%p| type=%d| size=%d|"
                " error=invalid type to pop|",
                ele, ele->m_data, ele->m_type, ele->m_size);

            return NULL;
        }
    } else {
        return NULL;
    }
}

