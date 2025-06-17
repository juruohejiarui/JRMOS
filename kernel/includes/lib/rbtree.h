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
static __always_inline__ void RBTree_insLck(RBTree *tree, RBNode *node) {
	if (tree == NULL || node == NULL) return ;
	SpinLock_lock(&tree->lock);
	RBTree_ins(tree, node);
	SpinLock_unlock(&tree->lock);
}
void RBTree_del(RBTree *tree, RBNode *node);
static __always_inline__ void RBTree_delLck(RBTree *tree, RBNode *node) {
	if (tree == NULL || node == NULL) return ;
	SpinLock_lock(&tree->lock);
	RBTree_del(tree, node);
	SpinLock_unlock(&tree->lock);
}

static __always_inline__ RBNode *RBTree_getLeft(RBTree *tree) {
	return (tree != NULL && tree->left != NULL) ? tree->left : NULL;
}
static __always_inline__ RBNode *RBTree_getLeftLck(RBTree *tree) {
	SpinLock_lock(&tree->lock);
	register RBNode *res = RBTree_getLeft(tree);
	SpinLock_unlock(&tree->lock);
	return res;
}
RBNode *RBTree_getRight(RBTree *tree);

static __always_inline__ RBNode *RBTree_getRightLck(RBTree *tree) {
	SpinLock_lock(&tree->lock);
	register RBNode *res = RBTree_getRight(tree);
	SpinLock_unlock(&tree->lock);
	return res;
}

RBNode *RBTree_getNext(RBTree *tree, RBNode *node);

#endif