#include <hardware/usb/xhci/api.h>
#include <mm/mm.h>
#include <screen/screen.h>

SafeList hw_usb_xhci_devLst;

// search xhci device in pci list
int hw_usb_xhci_searchInPci() {
	SafeList_init(&hw_usb_xhci_devLst);
	for (ListNode *pciDevNd = SafeList_getHead(&hw_pci_devLst); pciDevNd != &hw_pci_devLst.head; pciDevNd = pciDevNd->next) {
		hw_pci_Dev *dev = container(pciDevNd, hw_pci_Dev, lst);
		if (dev->cfg->class == 0x0c && dev->cfg->subclass == 0x03 && dev->cfg->progIf == 0x30) {
			hw_usb_xhci_Mgr *mgr = mm_kmalloc(sizeof(hw_usb_xhci_Mgr), mm_Attr_Shared, NULL);
			if (mgr == NULL) {
				printk(RED, BLACK, "xhci: failed to allocate xhci manager structure.\n");
				return res_FAIL;
			}
			mgr->pci = dev;
			mgr->intrNum = 0;
			mgr->flag = 0;
			SafeList_insTail(&hw_usb_xhci_devLst, &mgr->lst);
		}
	}

	// list xhci controller
	for (ListNode *xhciDevNd = SafeList_getHead(&hw_usb_xhci_devLst); xhciDevNd != &hw_usb_xhci_devLst.head; xhciDevNd = xhciDevNd->next) {
		hw_usb_xhci_Mgr *mgr = container(xhciDevNd, hw_usb_xhci_Mgr, lst);
		printk(WHITE, BLACK, "hw: xhci: find controller: %#018lx pci: %02x:%02x:%02x\n", mgr, mgr->pci->busId, mgr->pci->devId, mgr->pci->funcId);
	}

	return res_SUCC;
}