#ifndef __LIB_RBTREE_H__
#define __LIB_RBTREE_H__

#include <lib/spinlock.h>

#define RBTree_Color_Red 0
#define RBTree_Color_Black 1

typedef struct RBNode {
	u64 unionParCol;
	struct RBNode *left, *right;
} __attribute__((aligned(sizeof(long)))) RBNode;

struct RBTree;

// the comparator for RBTree, return 1 if a < b, 0 if a > b; invalid for a == b
typedef void (*RBTree_Insert)(struct RBTree *tree, RBNode *node, RBNode ***tgr, RBNode **par);

typedef int (*RBTree_Comparator)(RBNode *a, RBNode *b);

typedef struct RBTree {
	RBNode *root, *left;
	SpinLock lock;
	RBTree_Insert insert;
	RBTree_Comparator cmp;
} RBTree;

#define RBTree_Col_Red		0
#define RBTree_Col_Black	1

#define RBTree_insert(name, comparator) \
static void name(RBTree *tree, RBNode *node, RBNode ***tgr, RBNode **par) { \
	if (tree->left == NULL || comparator(node, tree->left)) tree->left = node; \
	RBNode **src = &tree->root, *lst; \
	while (*src) { \
		lst = *src; \
		if (comparator(node, lst)) \
			src = &(*src)->left; \
		else src = &(*src)->right; \
	} \
	*tgr = src, *par = lst; \
}

void RBTree_init(RBTree *tree, RBTree_Insert insert, RBTree_Comparator cmp);

void RBTree_ins(RBTree *tree, RBNode *node);
void RBTree_del(RBTree *tree, RBNode *node);

RBNode *RBTree_getLeft(RBTree *tree);
RBNode *RBTRee_getRight(RBTree *tree);
RBNode *RBTree_getNext(RBTree *tree, RBNode *node);

#endif