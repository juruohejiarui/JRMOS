#ifndef __FS_GPT_API_H__
#define __FS_GPT_API_H__

#include <fs/gpt/desc.h>
#include <hardware/diskdev.h>

__always_inline__ int fs_gpt_matchGuid(u64 guid[2], u64 dt1, u64 dt2) { return guid[0] == dt1 && guid[1] == dt2; }

void fs_gpt_scan(hw_DiskDev *dev);

#endif