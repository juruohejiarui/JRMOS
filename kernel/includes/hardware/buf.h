#ifndef __HARDWARE_BUF_H__
#define __HARDWARE_BUF_H__

#include <lib/rbtree.h>
#include <lib/list.h>
#include <lib/spinlock.h>

#define hw_Buf_Type_Byte    0x1
#define hw_Buf_Type_List    0x2

typedef struct hw_buf_Desc {
    RBNode rbNd;
    char *idenPath;
    int idenHash;

    int bufType;

} hw_buf_Desc;

typedef struct hw_buf_ByteBuf {
    hw_buf_Desc desc;

    SpinLock lck;
    
    int size, load;
    int head, tail;
    u8 data[];
} hw_buf_ByteBuf;

// list elements buffer
typedef struct hw_buf_ListBuf {
    hw_buf_Desc desc;
    SafeList list;
} hw_buf_ListBuf;

void hw_buf_init();

hw_buf_Desc *hw_buf_getBuf(char *iden);

int hw_buf_delBuf(hw_buf_Desc *desc);

hw_buf_ByteBuf *hw_buf_ByteBuf_create(char *iden, int size);

int hw_buf_ByteBuf_pop(hw_buf_ByteBuf *buf, u8 *ouput, int size);

hw_buf_ListBuf *hw_buf_ListBuf_create(char *iden);

ListNode *hw_buf_ListBuf_pop(hw_buf_ListBuf *buf);

#endif