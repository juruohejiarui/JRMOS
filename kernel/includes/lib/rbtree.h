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

typedef void (*RBTree_InsFunc)(struct RBTree *tree, RBNode *node, RBNode ***tgr, RBNode **par);

// the comparator for RBTree, return 1 if a < b, 0 if a > b; invalid for a == b
typedef int (*RBTree_Comparator)(RBNode *a, RBNode *b);

typedef RBNode *(*RBTree_FindFunc)(struct RBTree *tree, void *data);

// return -1 if node < data; 0 if node == data; 1 otherwise
typedef int (*RBTree_Match)(RBNode *node, void *data);

typedef struct RBTree {
	RBNode *root, *left;
	SpinLock lock;
	RBTree_InsFunc insert;
	RBTree_FindFunc find;
} RBTree;

#define RBTree_Col_Red		0
#define RBTree_Col_Black	1

// define insert function for RBTree
#define RBTree_insertDef(name, comparator) \
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

// find node s.t. <= requirement and closest to requirement
#define RBTree_findDef(name, match) \
static RBNode *name(RBTree *tree, void *data) { \
	RBNode *node = tree->root, *bst = NULL; \
	while (node) { \
		register int res = match(node, data); \
		if (res > 0) { \
			bst = node; \
			node = node->left; \
		} else if (res > 0) { \
			node = node->right; \
		} else return node; \
	} \
	return bst; \
}

void RBTree_init(RBTree *tree, RBTree_InsFunc insert, RBTree_FindFunc find);

void RBTree_ins(RBTree *tree, RBNode *node);

__always_inline__ void RBTree_lck(RBTree *tree) { SpinLock_lock(&tree->lock); }
__always_inline__ void RBTree_unlck(RBTree *tree) { SpinLock_unlock(&tree->lock); }

__always_inline__ void RBTree_insLck(RBTree *tree, RBNode *node) {
	SpinLock_lock(&tree->lock);
	RBTree_ins(tree, node);
	SpinLock_unlock(&tree->lock);
}

__always_inline__ RBNode *RBTree_find(RBTree *tree, void *data) {
	return tree->find(tree, data);
}

__always_inline__ RBNode *RBTree_findLck(RBTree *tree, void *data) {
	SpinLock_lock(&tree->lock);
	RBNode *res = RBTree_find(tree, data);
	SpinLock_unlock(&tree->lock);
	return res;
}

void RBTree_del(RBTree *tree, RBNode *node);

__always_inline__ void RBTree_delLck(RBTree *tree, RBNode *node) {
	SpinLock_lock(&tree->lock);
	RBTree_del(tree, node);
	SpinLock_unlock(&tree->lock);
}

__always_inline__ RBNode *RBTree_getLeft(RBTree *tree) { return tree->left; }

__always_inline__ RBNode *RBTree_getLeftLck(RBTree *tree) {
	SpinLock_lock(&tree->lock);
	register RBNode *res = RBTree_getLeft(tree);
	SpinLock_unlock(&tree->lock);
	return res;
}
RBNode *RBTree_getRight(RBTree *tree);

__always_inline__ RBNode *RBTree_getRightLck(RBTree *tree) {
	SpinLock_lock(&tree->lock);
	register RBNode *res = RBTree_getRight(tree);
	SpinLock_unlock(&tree->lock);
	return res;
}

RBNode *RBTree_getNext(RBTree *tree, RBNode *node);

RBNode *RBTree_getNextLck(RBTree *tree, RBNode *node);

void RBTree_debug(RBTree *tree);

#endif