#ifndef __LIB_LIST_H__
#define __LIB_LIST_H__

#include <lib/dtypes.h>
#include <lib/spinlock.h>

struct ListNode {
	struct ListNode *prev, *next;
};

struct SafeList {
	struct ListNode head;
	SpinLock lck;
};

typedef struct SafeList SafeList;
typedef struct ListNode ListNode;

__always_inline__ void List_init(ListNode *list) {
	list->prev = list->next = list;
}

__always_inline__ void List_insBehind(ListNode *node, ListNode *pos) {
	node->next = pos->next;
	node->prev = pos;
	pos->next->prev = node;
	pos->next = node;
}

__always_inline__ void List_insBefore(ListNode *node, ListNode *pos) {
	node->next = pos;
	node->prev = pos->prev;
	pos->prev->next = node;
	pos->prev = node;
}

__always_inline__ int List_isEmpty(ListNode *node) {
	return node->prev == node && node->next == node;
}

__always_inline__ void List_del(ListNode *node) {
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

__always_inline__ void List_insHead(ListNode *list, ListNode *node) {
	List_insBehind(node, list);
}
__always_inline__ void List_insTail(ListNode *list, ListNode *node) {
	List_insBefore(node, list);
}
__always_inline__ ListNode *List_getHead(ListNode *list) {
	return list->next;
}
__always_inline__ ListNode *List_getTail(ListNode *list) {
	return list->prev;
}

__always_inline__ void SafeList_init(SafeList *list) {
	SpinLock_init(&list->lck);
	List_init(&list->head);
}

void SafeList_insBehind(SafeList *list, ListNode *node, ListNode *pos);

void SafeList_insBefore(SafeList *list, ListNode *node, ListNode *pos);

int SafeList_isEmpty(SafeList *list);

void SafeList_del(SafeList *list, ListNode *node);

void SafeList_delHead(SafeList *list);

void SafeList_delTail(SafeList *list);

ListNode *SafeList_getHead(SafeList *list);

ListNode *SafeList_getTail(SafeList *list);

__always_inline__ void SafeList_insHead(SafeList *list, ListNode *node) {
	SafeList_insBehind(list, node, &list->head);
}

__always_inline__ void SafeList_insTail(SafeList *list, ListNode *node) {
	SafeList_insBefore(list, node, &list->head);
}

// parameter "list" must be a constant expression
#define SafeList_enum(list, node_name) \
	for (ListNode *node_name = (SpinLock_lockMask(&(list)->lck), (list)->head.next) ; \
		(node_name != &(list)->head) ? 1 : (SpinLock_unlockMask(&(list)->lck), 0); \
		node_name = node_name->next)

#define SafeList_exitEnum(list) \
	{ \
		SpinLock_unlockMask(&(list)->lck); \
		break; \
	}
#endif