#ifndef __HARDWARE_BUF_H__
#define __HARDWARE_BUF_H__

#include <lib/rbtree.h>

typedef struct hw_Buf {
    RBNode rbNd;
    char *idenPath;
    int idenHash;
    
} hw_Buf;

#endif