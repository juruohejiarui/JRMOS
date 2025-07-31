#include <hardware/nvme.h>
#include <mm/mm.h>
#include <screen/screen.h>
#include <lib/algorithm.h>

hw_nvme_SubQue *hw_nvme_allocSubQue(hw_nvme_Host *host, u32 iden, u32 size, hw_nvme_CmplQue *trg) {
	hw_nvme_SubQueEntry *entry = mm_kmalloc(sizeof(hw_nvme_SubQue) + size * sizeof(hw_nvme_SubQueEntry) + size * sizeof(hw_nvme_Request *), mm_Attr_Shared, NULL);
	if (entry == NULL) {
		printk(screen_err, "hw: nvme: %p: failed to allocate submission queue size=%d\n", host, size);
		return NULL;
	}
	memset(entry, 0, sizeof(hw_nvme_SubQueEntry) * size);
	hw_nvme_SubQue *que = (void *)(entry + size);
	que->entries = entry;
	que->iden = iden;

	que->tail = que->head = 0;
	que->size = size;
	que->load = 0;

	que->trg = trg;

	SpinLock_init(&que->lck);
	return que;
}

hw_nvme_CmplQue *hw_nvme_allocCmplQue(hw_nvme_Host *host, u32 iden, u32 size) {
	hw_nvme_CmplQueEntry *entry = mm_kmalloc(sizeof(hw_nvme_CmplQue) + size * sizeof(hw_nvme_CmplQueEntry), mm_Attr_Shared, NULL);
	if (entry == NULL) {
		printk(screen_err, "hw: nvme: %p: failed to allocate completion queue size=%d\n", host, size);
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
		printk(screen_err, "hw: nvme: failed to allocate request, size=%d\n", inputSz);
		return NULL;
	}
	task_Request_init(&req->req, task_Request_Flag_Abort);
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
	return res_SUCC;
}

int hw_nvme_initReq_crtSubQue(hw_nvme_Request *req, hw_nvme_SubQue *subQue) {
	hw_nvme_SubQueEntry *entry = &req->input[0];
	entry->opc = 0x01;
	entry->prp[0] = mm_dmas_virt2Phys(subQue->entries);
	entry->spec[0] = subQue->iden | (((u32)subQue->size - 1) << 16);
	// flag[0] : contiguous
	entry->spec[1] = 0b1 | (((u32)subQue->trg->iden) << 16);
	return res_SUCC;
}

int hw_nvme_initReq_crtCmplQue(hw_nvme_Request *req, hw_nvme_CmplQue *cmplQue) {
	hw_nvme_SubQueEntry *entry = &req->input[0];
	entry->opc = 0x05;
	entry->prp[0] = mm_dmas_virt2Phys(cmplQue->entries);
	entry->spec[0] = cmplQue->iden | ((u32)(cmplQue->size - 1) << 16);
	// flag[0] : contiguous
	// flag[1] : enable interrupt
	entry->spec[1] = 0b11 | ((u32)(cmplQue->iden) << 16);
	return res_SUCC;
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
		printk(screen_err, "hw: nvme: %p: request %p failed status:%x\n", host, req, req->res.status);
	}
}

int hw_nvme_respone(hw_nvme_Request *req, hw_nvme_CmplQueEntry *entry) {
	// copy the response to request
	memcpy(entry, &req->res, sizeof(hw_nvme_CmplQueEntry));
	// send the request
	task_Request_response(&req->req);
	return res_SUCC;
}

int hw_nvme_devChk(hw_Device *dev) { return res_FAIL; }

__always_inline__ hw_nvme_Dev *_toNvmeDev(hw_Device *dev) {
	return container(container(dev, hw_DiskDev, device), hw_nvme_Dev, diskDev);
}

int hw_nvme_devCfg(hw_Device *dev) {
	hw_nvme_Dev *nvmeDev = _toNvmeDev(dev);

	SpinLock_init(&nvmeDev->lck);

	hw_DiskDev_init(&nvmeDev->diskDev, hw_nvme_devSize, hw_nvme_devRead, hw_nvme_devWrite);

	hw_DiskDev_register(&nvmeDev->diskDev);

	return res_SUCC;
}

// install device, enable file system etc
int hw_nvme_devInstall(hw_Device *dev) {
	// allocate request buffer for this dev
	hw_nvme_Dev *nvmeDev = _toNvmeDev(dev);

	for (int i = 0; i < 64; i++) {
		if ((nvmeDev->reqs[i] = hw_nvme_makeReq(1)) == NULL) {
			printk(screen_err, "hw: nvme: %p: device %p: failed to allocate request block #%d\n", nvmeDev->host, nvmeDev, i);
			return res_FAIL;
		}
	}
	nvmeDev->reqBitmap = 0;
	return res_SUCC;
}

int hw_nvme_devUninstall(hw_Device *dev) {
	hw_nvme_Dev *nvmeDev = _toNvmeDev(dev);

	// some tasks are not finished
	if (nvmeDev->reqBitmap)
		printk(screen_warn, "hw: nvme: %p: device %p: some task not finished when uninstalling.\n", nvmeDev->host, nvmeDev);

	for (int i = 0; i < 64; i++) {
		if (hw_nvme_freeReq(nvmeDev->reqs[i]) == res_FAIL) {
			printk(screen_err, "hw: nvme: %p: device %p: failed to free request block #%d\n", nvmeDev->host, nvmeDev, i);
			return res_FAIL;
		}
	}
	return res_SUCC;
}

u64 hw_nvme_devSize(hw_DiskDev *dev) { return container(dev, hw_nvme_Dev, diskDev)->size; }

static int _getSpareReq(hw_nvme_Dev *dev) {
	int idx = -1;
	while (idx == -1) {
		SpinLock_lock(&dev->lck);
		// find a spare request
		if (dev->reqBitmap != ~0x0ul) {
			for (int i = 0; i < 64; i++) if (~dev->reqBitmap & (1ul << i)) {
				bit_set1(&dev->reqBitmap, idx = i);
				break;
			}
		}
		SpinLock_unlock(&dev->lck);
		task_sche_yield();
	}
	return idx;
}

__always_inline__ void _releaseReq(hw_nvme_Dev *dev, int idx) {
	// no need to lock
	bit_set0(&dev->reqBitmap, idx);
}

static hw_nvme_SubQue *_findIOSubQue(hw_nvme_Host *host) {
	hw_nvme_SubQue *que = host->subQue[1];
	for (int i = 2; i < host->intrNum; i++)
		if (que->load > host->subQue[i]->load) que = host->subQue[i];
	return que;
}

u64 hw_nvme_devRead(hw_DiskDev *dev, u64 offset, u64 size, void *buf) {
	hw_nvme_Dev *nvmeDev = container(dev, hw_nvme_Dev, diskDev);
	int reqIdx = _getSpareReq(nvmeDev);
	hw_nvme_Request *req = nvmeDev->reqs[reqIdx];
	hw_nvme_SubQueEntry *entry = &req->input[0];
	hw_nvme_SubQue *subQue;

	// make request and send
	u64 res = 0;
	memset(entry, 0, sizeof(hw_nvme_SubQueEntry));
	entry->opc = 0x02;
	entry->nspIden = nvmeDev->nspId;
	while (size != res) {
		entry->prp[0] = mm_dmas_virt2Phys(buf) + res * hw_diskdev_lbaSz;
		*(u64 *)&entry->spec[0] = offset + res;
		u32 blkSz = min(size - res, (1ul << 16));
		entry->spec[2] = blkSz - 1;

		// find a io submission queue & submit this request
		subQue = _findIOSubQue(nvmeDev->host);
		hw_nvme_request(nvmeDev->host, subQue, req);
		if (req->res.status != 0x01) {
			printk(screen_err, "hw: nvme: %p: device %p: failed to read blk: %ld~%ld\n", 
				nvmeDev->host, nvmeDev, 
				offset + res, offset + res + blkSz - 1);
			break;
		}
		res += blkSz;
	}

	// release request
	_releaseReq(nvmeDev, reqIdx);
	return res;
}

u64 hw_nvme_devWrite(hw_DiskDev *dev, u64 offset, u64 size, void *buf) {
	hw_nvme_Dev *nvmeDev = container(dev, hw_nvme_Dev, diskDev);
	int reqIdx = _getSpareReq(nvmeDev);
	hw_nvme_Request *req = nvmeDev->reqs[reqIdx];
	hw_nvme_SubQueEntry *entry = &req->input[0];
	hw_nvme_SubQue *subQue;

	// make request and send
	u64 res = 0;
	memset(entry, 0, sizeof(hw_nvme_SubQueEntry));
	entry->opc = 0x01;
	entry->nspIden = nvmeDev->nspId;
	while (size != res) {
		entry->prp[0] = mm_dmas_virt2Phys(buf) + res * hw_diskdev_lbaSz;
		*(u64 *)&entry->spec[0] = offset + res;
		u32 blkSz = min(size - res, (1ul << 16));
		entry->spec[2] = blkSz - 1;

		// find a io submission queue & submit this request
		subQue = _findIOSubQue(nvmeDev->host);
		hw_nvme_request(nvmeDev->host, subQue, req);
		if (req->res.status != 0x01) {
			printk(screen_err, "hw: nvme: %p: device %p: failed to read blk: %ld~%ld\n", 
				nvmeDev->host, nvmeDev, 
				offset + res, offset + res + blkSz - 1);
			break;
		}
		res += blkSz;
	}

	// release request
	_releaseReq(nvmeDev, reqIdx);
	return res;
}