#include <hardware/nvme.h>
#include <mm/mm.h>
#include <screen/screen.h>

hw_nvme_SubQue *hw_nvme_allocSubQue(hw_nvme_Host *host, u32 size) {
	hw_nvme_SubQueEntry *entry = mm_kmalloc(sizeof(hw_nvme_SubQue) + size * sizeof(hw_nvme_SubQueEntry) + size * sizeof(hw_nvme_Request *), mm_Attr_Shared, NULL);
	if (entry == NULL) {
		printk(RED, BLACK, "hw: nvme: %p: failed to allocate submission queue size=%d\n", host, size);
		return NULL;
	}
	hw_nvme_SubQue *que = (void *)(entry + size);
	que->entries = entry;
	return que;
}

hw_nvme_CmplQue *hw_nvme_allocCmplQue(hw_nvme_Host *host, u32 size) {
	hw_nvme_CmplQueEntry *entry = mm_kmalloc(sizeof(hw_nvme_CmplQue) + size * sizeof(hw_nvme_CmplQueEntry), mm_Attr_Shared, NULL);
	if (entry == NULL) {
		printk(RED, BLACK, "hw: nvme: %p: failed to allocate completion queue size=%d\n", host, size);
		return NULL;
	}
	hw_nvme_CmplQue *que = (void *)(entry + size);
	que->entries = entry;
	return que;
}