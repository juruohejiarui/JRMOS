#include <hardware/nvme.h>
#include <mm/mm.h>
#include <screen/screen.h>

hw_nvme_SubQue *hw_nvme_allocSubQue(hw_nvme_Host *host, u32 iden, u32 size) {
	hw_nvme_SubQueEntry *entry = mm_kmalloc(sizeof(hw_nvme_SubQue) + size * sizeof(hw_nvme_SubQueEntry) + size * sizeof(hw_nvme_Request *), mm_Attr_Shared, NULL);
	if (entry == NULL) {
		printk(RED, BLACK, "hw: nvme: %p: failed to allocate submission queue size=%d\n", host, size);
		return NULL;
	}
	memset(entry, 0, sizeof(hw_nvme_SubQueEntry) * size);
	hw_nvme_SubQue *que = (void *)(entry + size);
	que->entries = entry;
	que->iden = iden;

	que->tail = que->head = 0;
	que->size = size;
	que->load = 0;

	SpinLock_init(&que->lck);
	return que;
}

hw_nvme_CmplQue *hw_nvme_allocCmplQue(hw_nvme_Host *host, u32 iden, u32 size) {
	hw_nvme_CmplQueEntry *entry = mm_kmalloc(sizeof(hw_nvme_CmplQue) + size * sizeof(hw_nvme_CmplQueEntry), mm_Attr_Shared, NULL);
	if (entry == NULL) {
		printk(RED, BLACK, "hw: nvme: %p: failed to allocate completion queue size=%d\n", host, size);
		return NULL;
	}
	memset(entry, 0, sizeof(hw_nvme_CmplQueEntry) * size);
	hw_nvme_CmplQue *que = (void *)(entry + size);
	que->entries = entry;
	que->iden = iden;
	que->phase = 1;

	que->pos = 0;
	que->size = size;

	return que;
}

hw_nvme_Request *hw_nvme_makeReq(int inputSz) {
	hw_nvme_Request *req = mm_kmalloc(sizeof(hw_nvme_Request) + inputSz * sizeof(hw_nvme_SubQueEntry), mm_Attr_Shared, NULL);
	if (req == NULL) {
		printk(RED, BLACK, "hw: nvme: failed to allocate request, size=%d\n", inputSz);
		return NULL;
	}
	task_Request_init(&req->req, hw_Request_Flag_Abort);
	memset(req->input, 0, sizeof(hw_nvme_SubQueEntry) * inputSz);
	req->inputSz = inputSz;
	return req;
}

int hw_nvme_freeReq(hw_nvme_Request *req) {
	return mm_kfree(req, mm_Attr_Shared);
}

int hw_nvme_initReq_identify(hw_nvme_Request *req, u32 tp, u32 nspIden, void *buf) {
	hw_nvme_SubQueEntry *entry = &req->input[0];
	entry->opc = 0x6;
	entry->nspIden = nspIden;
	entry->spec[0] = tp;
	entry->prp[0] = mm_dmas_virt2Phys(buf);
}

__always_inline__ int hw_nvme_tryInsReq(hw_nvme_SubQue *subQue, hw_nvme_Request *req) {
	SpinLock_lockMask(&subQue->lck);
	if (subQue->load + req->inputSz > subQue->size) {
		SpinLock_unlockMask(&subQue->lck);
		return res_FAIL;
	}
	subQue->load += req->inputSz;
	// set identifier of each input to the idx in queue
	for (int i = 0; i < req->inputSz; i++) {
		req->input[i].cmdIden = subQue->tail;
		subQue->req[subQue->tail] = req;
		memcpy(&req->input[i], subQue->entries + subQue->tail, sizeof(hw_nvme_SubQueEntry));
		
		if ((++subQue->tail) == subQue->size) subQue->tail = 0;
	}
	SpinLock_unlockMask(&subQue->lck);
	return res_SUCC;
}

int hw_nvme_request(hw_nvme_Host *host, hw_nvme_SubQue *subQue, hw_nvme_Request *req) {
	while (hw_nvme_tryInsReq(subQue, req) != res_SUCC) ;

	hw_nvme_writeSubDb(host, subQue);

	task_Request_send(&req->req);

	if (req->res.status != 0x01) {
		printk(RED, BLACK, "hw: nvme: %p: request %p failed status:%d\n", host, req, req->res.status);
	}
}

int hw_nvme_respone(hw_nvme_Request *req, hw_nvme_CmplQueEntry *entry) {
	// copy the response to request
	memcpy(entry, &req->res, sizeof(hw_nvme_CmplQueEntry));
	// send the request
	task_Request_response(&req->req);
	return res_SUCC;
}