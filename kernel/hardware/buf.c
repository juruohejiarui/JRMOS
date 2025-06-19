#include <hardware/buf.h>
#include <lib/string.h>
#include <lib/hash.h>
#include <mm/mm.h>

static RBTree hw_buf_bufTree;

__always_inline__ int hw_buf_getHash(char *iden) {
	u32 crc = crc32_init(); u8 dt;
	while ((dt = *(iden++)) != '\0') crc = crc32_u8(crc, dt);
	return crc32_end(crc);
}

__always_inline__ int hw_buf_bufTreeCompare(RBNode *a, RBNode *b) {
	hw_buf_Desc *da = container(a, hw_buf_Desc, rbNd), *db = container(b, hw_buf_Desc, rbNd);
	return da->idenHash != db->idenHash ? da->idenHash < db->idenHash : strcmp(da->idenPath, da->idenPath) == -1;
}

RBTree_insert(hw_buf_bufTreeInsert, hw_buf_bufTreeCompare);

void hw_buf_init() {
	RBTree_init(&hw_buf_bufTree, hw_buf_bufTreeInsert, hw_buf_bufTreeCompare);
}

hw_buf_Desc *hw_buf_getBuf(char *iden) {
	u32 hash = hw_buf_getHash(iden);
	RBNode *cur = hw_buf_bufTree.root;
	hw_buf_Desc *desc;
	
	SpinLock_lock(&hw_buf_bufTree.lock);
	while (cur != NULL) {
		desc = container(cur, hw_buf_Desc, rbNd);
		if (desc->idenHash == hash && strcmp(iden, desc->idenPath) == 0) {
			SpinLock_unlock(&hw_buf_bufTree.lock);
			return desc;
		}
		if (desc->idenHash < hash || (desc->idenHash == hash && strcmp(desc->idenPath, iden) == -1))
			cur = cur->right;
		else cur = cur->left;
	}
	SpinLock_unlock(&hw_buf_bufTree.lock);
	return NULL;
}
int hw_buf_delBuf(hw_buf_Desc *desc) {
	RBTree_delLck(&hw_buf_bufTree, &desc->rbNd);
	return mm_kfree(desc, mm_Attr_Shared);
}

static void hw_buf_setDesc(hw_buf_Desc *desc, char *iden, int type) {
	desc->idenHash = hw_buf_getHash(iden);
	desc->idenPath = iden;

	desc->bufType = type;
}

hw_buf_ByteBuf *hw_buf_ByteBuf_create(char *iden, int size) {
	hw_buf_ByteBuf *buf = mm_kmalloc(sizeof(hw_buf_ByteBuf) + size * sizeof(u8), mm_Attr_Shared, NULL);
	buf->head = buf->tail = buf->load = 0;
	buf->size = size;
	SpinLock_init(&buf->lck);

	hw_buf_setDesc(&buf->desc, iden, hw_Buf_Type_Byte);
	return buf;
}

hw_buf_ListBuf *hw_buf_ListBuf_create(char *iden) {
	hw_buf_ListBuf *buf = mm_kmalloc(sizeof(hw_buf_ListBuf), mm_Attr_Shared, NULL);
	SafeList_init(&buf->list);

	hw_buf_setDesc(&buf->desc, iden, hw_Buf_Type_List);
	return buf;
}

int hw_buf_ByteBuf_pop(hw_buf_ByteBuf *buf, u8 *output, int size) {
	SpinLock_lockMask(&buf->lck);
	int actSz = 0;
	while (size > 0 && buf->load > 0) {
		*(output++) = buf->data[buf->head++];
		actSz++, buf->load--;
		if (buf->head == buf->size) buf->head = 0;
	}
	buf->load -= actSz;
	SpinLock_unlockMask(&buf->lck);
	return actSz;
}

int hw_buf_ByteBuf_push(hw_buf_ByteBuf *buf, u8 *input, int size) {
	SpinLock_lockMask(&buf->lck);
	if (buf->size - buf->load < size) {
		SpinLock_unlockMask(&buf->lck);
		return res_FAIL;
	}
	while (size > 0) {
		buf->data[buf->tail++] = *(input++);
		if (buf->tail == buf->size) buf->tail = 0;
	}
	buf->load += size;
	SpinLock_unlockMask(&buf->lck);
	return res_SUCC;
}