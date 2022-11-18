#ifndef __LISTNODE_H__
#define __LISTNODE_H__ 
#include"sysoper.h"


typedef struct _list_node {
    struct _list_node* m_prev;
    struct _list_node* m_next; 
} list_node;

typedef struct _list_head {
    list_node m_root;
    int m_size;
} list_head;

#define LIST_ADDR(head) (&(head)->m_root)
#define LIST_FIRST(head) ((head)->m_root.m_next)
#define LIST_LAST(head) ((head)->m_root.m_prev)
#define LIST_SIZE(head) ((head)->m_size)

static inline void INIT_LIST_HEAD(list_head* head) {
    LIST_FIRST(head) = LIST_LAST(head) = LIST_ADDR(head);
    
    LIST_SIZE(head) = 0;
} 

static inline void INIT_LIST_NODE(list_node* h) {
    h->m_prev = h->m_next = h;
}

static inline int list_empty(list_head* head) {
    return LIST_FIRST(head) == LIST_ADDR(head);
}

static inline void __list_add(list_node* added, 
    list_node* prev, list_node* next) {
    added->m_prev = prev;
    added->m_next = next;
    
    prev->m_next = added;
    next->m_prev = added;
}

static inline void list_add_front(list_node* added, list_head* head) {
    __list_add(added, LIST_ADDR(head), LIST_FIRST(head));
    ++LIST_SIZE(head);
}

static inline void list_add_back(list_node* added, list_head* head) {
    __list_add(added, LIST_LAST(head), LIST_ADDR(head));
    ++LIST_SIZE(head);
}

static inline void __list_del(list_node* node) {
    node->m_prev->m_next = node->m_next;
    node->m_next->m_prev = node->m_prev;

    INIT_LIST_NODE(node);
}

static inline void list_del(list_node* node, list_head* head) {
    if (node->m_next != node) {
        __list_del(node);
        --LIST_SIZE(head);
    }
}

/* replace all of a not empty list 'from' to an empty dest list 'to' */
static inline void list_replace(list_head* from, list_head* to) {
    if (!list_empty(from)) {
        LIST_FIRST(to) = LIST_FIRST(from);
        LIST_FIRST(to)->m_prev = LIST_ADDR(to);
        LIST_LAST(to) = LIST_LAST(from);
        LIST_LAST(to)->m_next = LIST_ADDR(to);

        LIST_SIZE(to) = LIST_SIZE(from); 
        INIT_LIST_HEAD(from);
    }
}

/* splice an not empty list between prev and next: prev -> h:list -> next */
static void __list_splice(list_head* h, list_node* prev, list_node* next) {
    prev->m_next = LIST_FIRST(h);
    LIST_FIRST(h)->m_prev = prev;
    next->m_prev = LIST_LAST(h);
    LIST_LAST(h)->m_next = next; 
}

/* arrange all of list 'from' at head of list 'to' */
static inline void list_splice_front(list_head* from, list_head* to) { 
    if (!list_empty(from)) {
        __list_splice(from, LIST_ADDR(to), LIST_FIRST(to));

        LIST_SIZE(to) += LIST_SIZE(from); 
        INIT_LIST_HEAD(from);
    }
}

/* append all of list 'from' at tail of list 'to' */
static inline void list_splice_back(list_head* from, list_head* to) { 
    if (!list_empty(from)) {
        __list_splice(from, LIST_LAST(to), LIST_ADDR(to));

        LIST_SIZE(to) += LIST_SIZE(from); 
        INIT_LIST_HEAD(from);
    }
}

#define list_entry(ptr, type, member) containof(ptr, type, member)
#define list_first_entry(head, type, member) \
	list_entry(LIST_FIRST(head), type, member)

#define list_for_each(pos, head) \
	for (pos = LIST_FIRST(head); pos != LIST_ADDR(head); pos = pos->m_next)

#define list_for_each_prev(pos, head) \
	for (pos = LIST_LAST(head); pos != LIST_ADDR(head); pos = pos->m_prev)
        	
#define list_for_each_safe(pos, n, head) \
	for (pos = LIST_FIRST(head), n = pos->m_next; \
        pos != LIST_ADDR(head); pos = n, n = pos->m_next)

#define list_for_each_prev_safe(pos, n, head) \
	for (pos = LIST_LAST(head), n = pos->m_prev; \
        pos != LIST_ADDR(head); pos = n, n = pos->m_prev)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
	     &pos->member != LIST_ADDR(head); 	\
	     pos = list_entry(pos->member.m_next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		n = list_entry(pos->member.m_next, typeof(*pos), member);	\
	     &pos->member != LIST_ADDR(head); 					\
	     pos = n, n = list_entry(n->member.m_next, typeof(*n), member))
    

typedef struct _hlist_node {
    struct _hlist_node* m_next, **m_pprev;
} hlist_node;
    
typedef struct _hlist_head {
    struct _hlist_node* m_first;
} hlist_head;

#define INIT_HLIST_HEAD(ptr) ((ptr)->m_first = NULL)

static inline void INIT_HLIST_NODE(hlist_node *h) {
    h->m_next = NULL;
    h->m_pprev = NULL;
}

static inline int hlist_unhashed(const hlist_node *h) {
	return !h->m_pprev;
}

static inline int hlist_empty(const hlist_head *h) {
	return !h->m_first;
}

static inline void hlist_del(hlist_node *n) {
	hlist_node *next = n->m_next;
	hlist_node **pprev = n->m_pprev;
    
	*pprev = next;
	if (next) {
		next->m_pprev = pprev;
	}

    INIT_HLIST_NODE(n);
}

static inline void hlist_add(hlist_node *n, hlist_head *h) {
	hlist_node *first = h->m_first;
	
	if (first) {
		first->m_pprev = &n->m_next;
	}

    n->m_next = first;
    n->m_pprev = &h->m_first;
    
	h->m_first = n; 
}

/* replace all of a not empty list 'from' to an empty dest list 'to' */
static inline void hlist_replace(hlist_head *from, hlist_head *to) {
	to->m_first = from->m_first;
    
	if (to->m_first) {
		to->m_first->m_pprev = &to->m_first;
	}
    
	from->m_first = NULL;
}
	
#define hlist_for_each(pos, head) \
	for (pos = (head)->m_first; pos; pos = pos->m_next)

#define hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->m_first; pos && ({ n = pos->m_next; 1; }); \
	     pos = n)

#define hlist_for_each_entry(tpos, pos, head, member)			 \
	for (pos = (head)->m_first;					 \
	     pos && ({ tpos = list_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->m_next)

#define hlist_for_each_entry_safe(tpos, pos, n, head, member) 		 \
	for (pos = (head)->m_first;					 \
	     pos && ({ n = pos->m_next; 1; }) && 				 \
		({ tpos = list_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = n)


typedef struct _order_list_head {
    list_node m_root;
    int m_size;
    int (*m_cmp)(list_node* n1, list_node* n2);
} order_list_head;

#define ORDER_LIST_CMP(head) ((head)->m_cmp)

static inline void INIT_ORDER_LIST_HEAD(
    order_list_head* head,
    int (*cmp)(list_node* n1, list_node* n2)) {
    LIST_FIRST(head) = LIST_LAST(head) = LIST_ADDR(head);
    
    LIST_SIZE(head) = 0;
    ORDER_LIST_CMP(head) = cmp;
}

static inline int order_list_empty(order_list_head* head) {
    return LIST_FIRST(head) == LIST_ADDR(head);
}

static inline void order_list_add(list_node* node, order_list_head* head) {
    int ret = 0;
    list_node* curr = NULL;

    list_for_each_prev(curr, head) {
        ret = ORDER_LIST_CMP(head)(curr, node);
        if (0 >= ret) {
            break;
        }
    }

    __list_add(node, curr, curr->m_next);
    ++LIST_SIZE(head);
}

static inline void order_list_del(list_node* node, order_list_head* head) {
    __list_del(node);
    --LIST_SIZE(head);
}

static inline list_node* order_list_find(list_node* node, order_list_head* head) {
    int ret = 0;
    list_node* pos = NULL;
    
    list_for_each(pos, head) {
        ret = ORDER_LIST_CMP(head)(pos, node);
        if (0 > ret) {
            continue;
        } else if (0 == ret) {
            return pos;
        } else {
            break;
        }
    }

    return NULL;
}


struct node {
    struct node* m_next;
};

struct root {
    struct node* m_tail;
    struct node m_head;
    int m_size;
};

struct Task {
    struct node m_node;
    unsigned int m_state;
};

static inline void INIT_ROOT_NODE(struct node* node) {
    node->m_next = NULL;
}

static inline void INIT_ROOT(struct root* root) {
    root->m_head.m_next = NULL;
    root->m_tail = &root->m_head;

    root->m_size = 0;
}

static inline void INIT_TASK(struct Task* task) {
    INIT_ROOT_NODE(&task->m_node);
    task->m_state = 0U;
}

static inline bool isRootEmpty(struct root* root) {
    return !root->m_head.m_next;
}

static inline void pushNode(struct root* root, struct node* node) {    
    INIT_ROOT_NODE(node);
    
    root->m_tail->m_next = node;
    root->m_tail = node; 
    ++root->m_size;    
}

static inline struct node* popNode(struct root* root) {
    struct node* node = NULL;

    if (!isRootEmpty(root)) {
        node = root->m_head.m_next; 
        
        if (NULL != node->m_next) {
            root->m_head.m_next = node->m_next;
        } else {
            root->m_head.m_next = NULL;
            root->m_tail = &root->m_head;
        }

        --root->m_size;
        INIT_ROOT_NODE(node);
    }
    
    return node;    
}

static inline void spliceRoot(struct root* from, struct root* to) {
    if (!isRootEmpty(from)) {
        to->m_tail->m_next = from->m_head.m_next;
        to->m_tail = from->m_tail;
        to->m_size += from->m_size;

        INIT_ROOT(from);
    } 
}

static inline void replaceRoot(struct root* from, struct root* to) {
    to->m_head.m_next = from->m_head.m_next;
    to->m_tail = from->m_tail;
    to->m_size = from->m_size; 

    INIT_ROOT(from);
}

#define ROOT_FIRST(root) ((root)->m_head.m_next)

#define root_for_each(pos, root) \
	for (pos = ROOT_FIRST(root); pos; pos = pos->m_next)

#define root_for_each_safe(pos, n, root) \
	for (pos = ROOT_FIRST(root); pos && ({ n = pos->m_next; 1; }); \
	     pos = n)
	     
#endif

