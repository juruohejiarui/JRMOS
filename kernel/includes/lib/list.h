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

static __always_inline__ void List_init(ListNode *list) {
	list->prev = list->next = list;
}

static __always_inline__ void List_insBehind(ListNode *node, ListNode *pos) {
	node->next = pos->next;
	node->prev = pos;
	pos->next->prev = node;
	pos->next = node;
}

static __always_inline__ void List_insBefore(ListNode *node, ListNode *pos) {
	node->next = pos;
	node->prev = pos->prev;
	pos->prev->next = node;
	pos->prev = node;
}

static __always_inline__ int List_isEmpty(ListNode *node) {
	return node->prev == node && node->next == node;
}

static __always_inline__ void List_del(ListNode *node) {
	node->next->prev = node->prev;
	node->prev->next = node->next;
	node->prev = node->next = node;
}

static __always_inline__ void List_insHead(ListNode *list, ListNode *node) {
	List_insBehind(node, list);
}
static __always_inline__ void List_insTail(ListNode *list, ListNode *node) {
	List_insBefore(node, list);
}
static __always_inline__ ListNode *List_getHead(ListNode *list) {
	return list->next;
}
static __always_inline__ ListNode *List_getTail(ListNode *list) {
	return list->prev;
}

static __always_inline__ void SafeList_init(SafeList *list) {
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

static __always_inline__ void SafeList_insHead(SafeList *list, ListNode *node) {
	SafeList_insBehind(list, node, &list->head);
}

static __always_inline__ void SafeList_insTail(SafeList *list, ListNode *node) {
	SafeList_insBefore(list, node, &list->head);
}

#endif