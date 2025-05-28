#include <hardware/usb/xhci/api.h>
#include <mm/mm.h>
#include <timer/api.h>
#include <screen/screen.h>

void *hw_usb_xhci_nxtExtCap(hw_usb_xhci_Host *host, void *cur) {
    if (!cur) return (void *)(host->capRegAddr + (hw_usb_xhci_CapReg_hccParam(host, 1) >> 16) * 4);
    register u32 off = hal_read8((u64)cur + 1);
    return off ? (void *)((u64)cur + off * 4) : NULL;
}

u8 hw_usb_xhci_getSlotType(hw_usb_xhci_Host * host, u32 portIdx) {
	for (void *expCap = hw_usb_xhci_nxtExtCap(host, NULL); expCap; expCap = hw_usb_xhci_nxtExtCap(host, expCap)) {
        if (hw_usb_xhci_xhci_getExtCapId(expCap) == hw_usb_xhci_Ext_Id_Protocol) {
            if (hw_usb_xhci_Ext_Protocol_contain(expCap, portIdx)) return hw_usb_xhci_Ext_Protocol_slotType(expCap);
        }
    }
    return 0;
}

hw_usb_xhci_InCtx *hw_usb_xhci_allocInCtx(hw_usb_xhci_Host *host) {
    void *ctx = mm_kmalloc((host->flag & hw_usb_xhci_Host_flag_Ctx64) ? sizeof(hw_usb_xhci_InCtx64) : sizeof(hw_usb_xhci_InCtx32), 0, NULL);
    if (!ctx) {
        printk(RED, BLACK, "hw: xhci: alloc input context failed\n");
        return NULL;
    }
    memset(ctx, 0, (host->flag & hw_usb_xhci_Host_flag_Ctx64) ? sizeof(hw_usb_xhci_InCtx64) : sizeof(hw_usb_xhci_InCtx32));
    return container(ctx, hw_usb_xhci_InCtx, ctx32);
}

hw_usb_xhci_Ring *hw_usb_xhci_allocRing(hw_usb_xhci_Host *host, u32 size) {
	hw_usb_xhci_Ring *ring = mm_kmalloc(sizeof(hw_usb_xhci_Ring), mm_Attr_Shared, NULL);
    if (!ring) {
        printk(RED, BLACK, "hw: xhci: alloc ring failed\n");
        return NULL;
    }

    ring->trbs = mm_kmalloc(size * sizeof(hw_usb_xhci_TRB), mm_Attr_Shared, NULL);
    if (!ring->trbs) {
        printk(RED, BLACK, "hw: xhci: alloc ring trbs failed\n");
        mm_kfree(ring, mm_Attr_Shared);
        return NULL;
    }
    memset(ring->trbs, 0, size * sizeof(hw_usb_xhci_TRB));

    ring->curIdx = ring->load = 0;
    ring->size = size;

    ring->reqs = mm_kmalloc(size * sizeof(hw_usb_xhci_Request *), mm_Attr_Shared, NULL);
    if (!ring->reqs) {
        printk(RED, BLACK, "hw: xhci: alloc ring request pointer failed\n");
        mm_kfree(ring->trbs, mm_Attr_Shared);
        mm_kfree(ring, mm_Attr_Shared);
        return NULL;
    }
    SpinLock_init(&ring->lck);
    
    ring->cycBit = 1;

    // set the final TRB to link TRB
    {
        hw_usb_xhci_TRB *lkTrb = ring->trbs + size - 1;
        // point to the first TRB
        hw_usb_xhci_TRB_setData(lkTrb, mm_dmas_virt2Phys(ring->trbs));
        
        hw_usb_xhci_TRB_setType(lkTrb, hw_usb_xhci_TRB_Type_Link);
        hw_usb_xhci_TRB_setToggle(lkTrb, 1);
    }

    // add to the list of rings in host
    SafeList_insTail(&host->ringLst, &ring->lst);

    return ring;
}

int hw_usb_xhci_freeRing(hw_usb_xhci_Host *host, hw_usb_xhci_Ring *ring) {
    if (!ring || ring->load) return res_FAIL;

    SafeList_del(&host->ringLst, &ring->lst);
    if (mm_kfree(ring->trbs, mm_Attr_Shared) == res_FAIL) {
        printk(RED, BLACK, "hw: xhci: free ring trbs failed\n");
        return res_FAIL;
    }
    if (mm_kfree(ring->reqs, mm_Attr_Shared) == res_FAIL) {
        printk(RED, BLACK, "hw: xhci: free ring request pointer failed\n");
        return res_FAIL;
    }
    if (mm_kfree(ring, mm_Attr_Shared) == res_FAIL) {
        printk(RED, BLACK, "hw: xhci: free ring failed\n");
        return res_FAIL;
    }

    return res_SUCC;
}

static int _tryInsReq(hw_usb_xhci_Host *host, hw_usb_xhci_Ring *ring, hw_usb_xhci_Request *req) {
    req->flags &= ~hw_usb_xhci_Request_flags_Finished;
    SpinLock_lockMask(&ring->lck);
    // check if the ring is full
    if (ring->load + req->inputSz > ring->size) {
        SpinLock_unlockMask(&ring->lck);
        printk(RED, BLACK, "hw: xhci: ring %p is full.\n", ring);
        return res_FAIL;
    }
    ring->load += req->inputSz;
    // insert TRBs into the ring
    for (int i = 0; i < req->inputSz; i++) {
        // meet link TRB
        if (ring->curIdx + 1 == ring->size) {
            hw_usb_xhci_TRB_setCycBit(&ring->trbs[ring->curIdx], ring->cycBit);
            ring->curIdx = 0;
            ring->cycBit ^= 1;
            i--;
            continue;
        }
        hw_usb_xhci_TRB_copy(&req->input[i], &ring->trbs[ring->curIdx]);
        hw_usb_xhci_TRB_setCycBit(&ring->trbs[ring->curIdx], ring->cycBit);
        ring->reqs[ring->curIdx] = req;
        ring->curIdx++;
    }
    SpinLock_unlockMask(&ring->lck);
    return res_SUCC;
}

void hw_usb_xhci_InsReq(hw_usb_xhci_Host *host, hw_usb_xhci_Ring *ring, hw_usb_xhci_Request *req) {
    while (_tryInsReq(host, ring, req) != res_SUCC) {
        task_sche_yield();
    }
}

// insert the request, write the doorbell register, then wait for the reply
void hw_usb_xhci_request(hw_usb_xhci_Host *host, hw_usb_xhci_Ring *ring, hw_usb_xhci_Request *req, u32 slot, u32 doorbell) {
    hw_usb_xhci_InsReq(host, ring, req);
    hw_usb_xhci_DbReg_write(host, slot, doorbell);
    // wait for the request to be finished
    task_Request_send(&req->req);
}

hw_usb_xhci_Request *hw_usb_xhci_makeRequest(u32 size, u32 flags) {
	hw_usb_xhci_Request *req = mm_kmalloc(sizeof(hw_usb_xhci_Request) + size * sizeof(hw_usb_xhci_TRB), 0, NULL);
    if (!req) {
        printk(RED, BLACK, "hw: xhci: alloc request failed\n");
        return NULL;
    }
    task_Request_init(&req->req, hw_Request_Flag_Abort);
    memset(req->input, 0, size * sizeof(hw_usb_xhci_TRB));
    req->flags = flags;
    req->inputSz = size;
    return req;
}

int hw_usb_xhci_freeRequest(hw_usb_xhci_Request *req) {
	if (!req || mm_kfree(req, 0) == res_FAIL) {
        printk(RED, BLACK, "hw: xhci: free request failed\n");
        return res_FAIL;
    }
    return res_SUCC;
}

// response the first request in the ring and wake the corresponding task
hw_usb_xhci_Request *hw_usb_xhci_response(hw_usb_xhci_Ring *ring, hw_usb_xhci_TRB *result, u64 trbAddr) {
    SpinLock_lockMask(&ring->lck);
    if (!ring->load) {
        SpinLock_unlockMask(&ring->lck);
        printk(RED, BLACK, "hw: xhci: ring %p has no request waiting for responese", ring);
        return NULL;
    }

    u64 idx = (trbAddr - mm_dmas_virt2Phys(ring->trbs)) / sizeof(hw_usb_xhci_TRB);
    hw_usb_xhci_Request *req = ring->reqs[idx];

    ring->load -= req->inputSz;
    SpinLock_unlockMask(&ring->lck);
    hw_usb_xhci_TRB_copy(result, &req->res);
    req->flags |= hw_usb_xhci_Request_flags_Finished;
    task_Request_response(&req->req);
    return req;
}

hw_usb_xhci_EveRing *hw_usb_xhci_allocEveRing(hw_usb_xhci_Host *host, u16 ringNum, u32 ringSz) {
    hw_usb_xhci_EveRing *eveRing = mm_kmalloc(sizeof(hw_usb_xhci_EveRing), mm_Attr_Shared, NULL);
    if (!eveRing) {
        printk(RED, BLACK, "hw: xhci: alloc event ring failed\n");
        return NULL;
    }

    eveRing->rings = mm_kmalloc(ringNum * sizeof(hw_usb_xhci_TRB *), mm_Attr_Shared, NULL);
    if (!eveRing->rings) {
        printk(RED, BLACK, "hw: xhci: alloc event ring trbs failed\n");
        mm_kfree(eveRing, mm_Attr_Shared);
        return NULL;
    }

    for (u16 i = 0; i < ringNum; i++) {
        eveRing->rings[i] = mm_kmalloc(ringSz * sizeof(hw_usb_xhci_TRB), mm_Attr_Shared, NULL);
        if (!eveRing->rings[i]) {
            printk(RED, BLACK, "hw: xhci: alloc event ring trbs failed\n");
            for (u16 j = 0; j < i; j++) {
                mm_kfree(eveRing->rings[j], mm_Attr_Shared);
            }
            mm_kfree(eveRing->rings, mm_Attr_Shared);
            mm_kfree(eveRing, mm_Attr_Shared);
            return NULL;
        }
        memset(eveRing->rings[i], 0, ringSz * sizeof(hw_usb_xhci_TRB));
    }

    eveRing->curIdx = eveRing->curRingIdx = 0;
    eveRing->ringNum = ringNum;
    eveRing->ringSz = ringSz;
    eveRing->cycBit = 1;
    return eveRing;
}

int hw_usb_xhci_reset(hw_usb_xhci_Host *host) {
    int timeout = 500;
    hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_cmd, (1u << 1));
    while ((hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_cmd) & (1u << 1)) && timeout--) {
        timer_mdelay(1);
        timeout--;
    }
    if ((hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_cmd) & (1u << 1)) && (~hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_status) & (1u << 0)))
        return res_FAIL;
    return res_SUCC;
}

int hw_usb_xhci_waitReady(hw_usb_xhci_Host *host) {
    int timeout = 500;
    while (hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_status) & (1u << 11) && timeout--) {
        timer_mdelay(1);
    }
    if (hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_status) & (1u << 11)) return res_FAIL;
    return res_SUCC;
}

hw_usb_xhci_Device *hw_usb_xhci_newDev(hw_usb_xhci_Host *host, hw_usb_xhci_Device *parent, u32 portIdx) {
    hw_usb_xhci_Device *dev = mm_kmalloc(sizeof(hw_usb_xhci_Device), mm_Attr_Shared, NULL);
    if (!dev) {
        printk(RED, BLACK, "hw: xhci: alloc device failed\n");
        return NULL;
    }
    memset(dev, 0, sizeof(hw_usb_xhci_Device));
    dev->flag = parent == NULL ? hw_usb_xhci_Device_flag_Direct : 0;
    dev->portId = portIdx;

    dev->parent = parent;
    dev->host = host;

    // make new task for the device
    dev->mgrTsk = task_newTask(hw_usb_xhci_devMgrTsk, (u64)dev, task_attr_Builtin);

    SafeList_insTail(&host->devLst, &dev->lst);
    return dev;
}

int hw_usb_xhci_freeDev(hw_usb_xhci_Device *dev) {
    if (!dev) return res_FAIL;
    if (mm_kfree(dev, mm_Attr_Shared) == res_FAIL) {
        printk(RED, BLACK, "hw: xhci: free device failed\n");
        return res_FAIL;
    }
    return res_SUCC;
}