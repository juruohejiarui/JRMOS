#include <hardware/usb/xhci/api.h>
#include <mm/mm.h>
#include <screen/screen.h>

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

    ring->cur = ring->trbs;
    ring->curIdx = 0;
    ring->size = size;

    List_init(&ring->reqLst);
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
    if (!ring || !List_isEmpty(&ring->reqLst)) return res_FAIL;

    SafeList_del(&host->ringLst, &ring->lst);
    if (mm_kfree(ring->trbs, mm_Attr_Shared) == res_FAIL) {
        printk(RED, BLACK, "hw: xhci: free ring trbs failed\n");
        return res_FAIL;
    }
    if (mm_kfree(ring, mm_Attr_Shared) == res_FAIL) {
        printk(RED, BLACK, "hw: xhci: free ring failed\n");
        return res_FAIL;
    }

    return res_SUCC;
}

int hw_usb_xhci_tryInsReq(hw_usb_xhci_Host *host, hw_usb_xhci_Ring *ring, hw_usb_xhci_Request *req) {
    
}