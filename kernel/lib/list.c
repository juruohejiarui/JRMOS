#include <lib/list.h>
#include <interrupt/api.h>

void SafeList_insBehind(SafeList *list, ListNode *node, ListNode *pos) {
	// disable interrupt if needed
	register int state = intr_state();
	if (state) intr_mask();
	SpinLock_lock(&list->lck);
	List_insBehind(node, pos);
	SpinLock_unlock(&list->lck);
	if (state) intr_unmask();
}

void SafeList_insBefore(SafeList *list, ListNode *node, ListNode *pos) {
	// disable interrupt if needed
	register int state = intr_state();
	if (state) intr_mask();
	SpinLock_lock(&list->lck);
	List_insBefore(node, pos);
	SpinLock_unlock(&list->lck);
	if (state) intr_unmask();
}

int SafeList_isEmpty(SafeList *list) {
	// disable interrupt if needed
	register int state = intr_state();
	if (state) intr_mask();
	register int res;
	SpinLock_lock(&list->lck);
	res = List_isEmpty(&list->head);
	SpinLock_unlock(&list->lck);
	if (state) intr_unmask();
	return res;
}

void SafeList_del(SafeList *list, ListNode *node) {
	register int state = intr_state();
	if (state) intr_mask();
	SpinLock_lock(&list->lck);
	List_del(node);
	SpinLock_unlock(&list->lck);
	if (state) intr_unmask();
}

void SafeList_delHead(SafeList *list) {
	register int state = intr_state();
	if (state) intr_mask();
	SpinLock_lock(&list->lck);
	List_del(list->head.next);
	SpinLock_unlock(&list->lck);
	if (state) intr_unmask();
}

void SafeList_delTail(SafeList *list) {
	register int state = intr_state();
	if (state) intr_mask();
	SpinLock_lock(&list->lck);
	List_del(list->head.prev);
	SpinLock_unlock(&list->lck);
	if (state) intr_unmask();
}

ListNode *SafeList_getHead(SafeList *list) {
	register int state = intr_state();
	if (state) intr_mask();
	register ListNode *node;
	SpinLock_lock(&list->lck);
	node = list->head.next;
	SpinLock_unlock(&list->lck);
	if (state) intr_unmask();
	return node;
}

ListNode *SafeList_getTail(SafeList *list) {
	register int state = intr_state();
	if (state) intr_mask();
	register ListNode *node;
	SpinLock_lock(&list->lck);
	node = list->head.prev;
	SpinLock_unlock(&list->lck);
	if (state) intr_unmask();
	return node;
}


