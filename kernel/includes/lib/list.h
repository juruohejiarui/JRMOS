#ifndef __LIB_LIST_H__
#define __LIB_LIST_H__

#include <lib/dtypes.h>

struct List {
	struct List *prev, *next;
};

typedef struct List List;

static __always_inline__ void List_init(List *list) { list->prev = list->next = list; }

static __always_inline__ void List_insBehind(List *list, List *pos) {
    list->next = pos->next;
	list->prev = pos;
    pos->next->prev = list;
    pos->next = list;
}

static __always_inline__ void List_insBefore(List *list, List *pos) {
	list->next = pos;
	list->prev = pos->prev;
    pos->prev->next = list;
    pos->prev = list;
}

static __always_inline__ int List_isEmpty(List *list) { return list->prev == list && list->next == list; }

static __always_inline__ void List_del(List *list) {
    list->next->prev = list->prev;
    list->prev->next = list->next;
    list->prev = list->next = list;
}

#endif