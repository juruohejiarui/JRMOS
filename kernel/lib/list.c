#include <lib/list.h>
#include <interrupt/api.h>

void SafeList_insBehind(SafeList *list, ListNode *node, ListNode *pos) {
	// disable interrupt if needed
	SpinLock_lockMask(&list->lck);
	List_insBehind(node, pos);
	SpinLock_unlockMask(&list->lck);
}

void SafeList_insBefore(SafeList *list, ListNode *node, ListNode *pos) {
	SpinLock_lockMask(&list->lck);
	List_insBefore(node, pos);
	SpinLock_unlockMask(&list->lck);
}

int SafeList_isEmpty(SafeList *list) {
	SpinLock_lockMask(&list->lck);
	register int res = List_isEmpty(&list->head);
	SpinLock_unlockMask(&list->lck);
	return res;
}

void SafeList_del(SafeList *list, ListNode *node) {
	SpinLock_lockMask(&list->lck);
	List_del(node);
	SpinLock_unlockMask(&list->lck);
}

void SafeList_delHead(SafeList *list) {
	SpinLock_lockMask(&list->lck);
	List_del(list->head.next);
	SpinLock_unlockMask(&list->lck);

}

void SafeList_delTail(SafeList *list) {
	SpinLock_lockMask(&list->lck);
	List_del(list->head.prev);
	SpinLock_unlockMask(&list->lck);
}

ListNode *SafeList_getHead(SafeList *list) {
	SpinLock_lockMask(&list->lck);
	register ListNode *node = list->head.next;
	SpinLock_unlockMask(&list->lck);
	return node;
}

ListNode *SafeList_getTail(SafeList *list) {
	SpinLock_lockMask(&list->lck);
	register ListNode *node = list->head.prev;
	SpinLock_unlockMask(&list->lck);
	return node;
}


