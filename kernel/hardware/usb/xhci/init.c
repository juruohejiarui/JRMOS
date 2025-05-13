#include <hardware/usb/xhci/api.h>
#include <lib/algorithm.h>
#include <mm/mm.h>
#include <timer/api.h>
#include <screen/screen.h>

SafeList hw_usb_xhci_devLst;

// search xhci device in pci list
static int _searchInPci() {
	SafeList_init(&hw_usb_xhci_devLst);
	for (ListNode *pciDevNd = SafeList_getHead(&hw_pci_devLst); pciDevNd != &hw_pci_devLst.head; pciDevNd = pciDevNd->next) {
		hw_pci_Dev *dev = container(pciDevNd, hw_pci_Dev, lst);
		if (dev->cfg->class == 0x0c && dev->cfg->subclass == 0x03 && dev->cfg->progIf == 0x30) {
			hw_usb_xhci_Host *mgr = mm_kmalloc(sizeof(hw_usb_xhci_Host), mm_Attr_Shared, NULL);
			if (mgr == NULL) {
				printk(RED, BLACK, "xhci: failed to allocate xhci manager structure.\n");
				return res_FAIL;
			}
			mgr->pci = dev;
			mgr->intrNum = 0;
			mgr->flag = 0;
			SafeList_insTail(&hw_usb_xhci_devLst, &mgr->lst);
			
			SafeList_init(&mgr->devLst);
			SafeList_init(&mgr->ringLst);
		}
	}

	// list xhci controller
	for (ListNode *xhciDevNd = SafeList_getHead(&hw_usb_xhci_devLst); xhciDevNd != &hw_usb_xhci_devLst.head; xhciDevNd = xhciDevNd->next) {
		hw_usb_xhci_Host *mgr = container(xhciDevNd, hw_usb_xhci_Host, lst);
		printk(WHITE, BLACK, "hw: xhci: find controller: %#018lx pci: %02x:%02x:%02x\n", mgr, mgr->pci->busId, mgr->pci->devId, mgr->pci->funcId);
	}

	return res_SUCC;
}

static __always_inline__ void _disablePartnerCtrl(hw_usb_xhci_Host *host) {
	if (host->pci->cfg->vendorId == 0x8086 && host->pci->cfg->deviceId == 0x1e31 && host->pci->cfg->revisionId == 4) {
		*(u32 *)((u64)host->pci->cfg + 0xd8) = 0xffffffffu;
		*(u32 *)((u64)host->pci->cfg + 0xd0) = 0xffffffffu;
	}
}

static __always_inline__ int _getRegAddr(hw_usb_xhci_Host *host) {
	u64 phyAddr = ((host->pci->cfg->type0.bar[0] | (((u64)host->pci->cfg->type0.bar[1]) << 32))) & ~0xful;
	host->capRegAddr = (u64)mm_dmas_phys2Virt(phyAddr);

	printk(WHITE, BLACK, "hw: xhci: host %#018lx: capRegAddr:%#018lx\n", host, phyAddr);
	
	// map this address in dmas
	if (mm_dmas_map(phyAddr) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to map cap regs\n", host);
		return res_FAIL;
	}
	host->opRegAddr = host->capRegAddr + hw_usb_xhci_CapReg_capLen(host);
	host->rtRegAddr = host->capRegAddr + hw_usb_xhci_CapReg_rtsOff(host);
	host->dbRegAddr = host->capRegAddr + hw_usb_xhci_CapReg_dbOff(host);

	// map address in dmas
	if (mm_dmas_map(mm_dmas_virt2Phys(host->opRegAddr)) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to map op regs\n", host);
		return res_FAIL;
	}
	if (mm_dmas_map(mm_dmas_virt2Phys(host->rtRegAddr)) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to map rt regs\n", host);
		return res_FAIL;
	}
	if (mm_dmas_map(mm_dmas_virt2Phys(host->dbRegAddr)) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to map hcc regs\n", host);
		return res_FAIL;
	}
	return res_SUCC;
}

static int _initHost(hw_usb_xhci_Host *host) {
	// check the capability list
	if (!(host->pci->cfg->status & (1u << 4))) {
		printk(RED, BLACK, "hw: xhci: pci device %02x:%02x:%02x has no capability list\n", host->pci->busId, host->pci->devId, host->pci->funcId);
		return res_FAIL;
	}

	hw_pci_MsixCap *msix = NULL;
	hw_pci_MsiCap *msi = NULL;
	// search for msi/msix capability descriptor
	for (hw_pci_CapHdr *hdr = hw_pci_getNxtCap(host->pci->cfg, NULL); hdr; hdr = hw_pci_getNxtCap(host->pci->cfg, hdr)) {
		switch (hdr->capId) {
			case hw_pci_CapHdr_capId_MSI:
				msi = container(hdr, hw_pci_MsiCap, hdr);
				break;
			case hw_pci_CapHdr_capId_MSIX:
				msix = container(hdr, hw_pci_MsixCap, hdr);
				break;
		}
	}
	if (!msix && !msi) {
		printk(RED, BLACK, "hw: xhci: controller %#018lx has no msi/msix descriptor\n", host);
		return res_FAIL;
	}

	if (msix) host->flag |= hw_usb_xhci_Host_flag_Msix;

	printk(WHITE, BLACK, "hw: xhci: initialize device %#018lx: vendor:%04x device:%04x revision:%04x\n",
		host, host->pci->cfg->vendorId, host->pci->cfg->deviceId, host->pci->cfg->revisionId);

	_disablePartnerCtrl(host);

	// get address of registers
	if (_getRegAddr(host) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to get register address\n", host);
		return res_FAIL;
	}

	// read information from capability registers
	printk(WHITE, BLACK, "hw: xhci: host %#018lx: capReg:%#018lx opReg:%#018lx rtReg:%#018lx dbReg:%#018lx\n",
		host, host->capRegAddr, host->opRegAddr, host->rtRegAddr, host->dbRegAddr);
	printk(WHITE, BLACK, "hw: xhci: host %#018lx: mxslot:%d mxintr:%d mxport:%d mxerst:%d mxscr:%d\n",
		host, hw_usb_xhci_mxSlot(host), hw_usb_xhci_mxIntr(host), hw_usb_xhci_mxPort(host), hw_usb_xhci_mxERST(host), hw_usb_xhci_mxScrSz(host));

	// check if the controller support neccessary features: 4K page, 64bit address, port power control
	if (~hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_pgSize) & 0x1) {
		printk(RED, BLACK, "hw: xhci: host %#018lx does not support 4K page\n", host);
		return res_FAIL;
	}
	if (~hw_usb_xhci_CapReg_hccParam(host, 1) & 0x1) {
		printk(RED, BLACK, "hw: xhci: host %#018lx does not support 64bit address\n", host);
		return res_FAIL;
	}
	if (hw_usb_xhci_CapReg_hccParam(host, 1) & (1u << 3)) {
		printk(RED, BLACK, "hw: xhci: host %#018lx doest not support port power control\n", host);
		return res_FAIL;
	}
	// check the context size
	if (hw_usb_xhci_CapReg_hccParam(host, 1) & (1u << 2)) {
		printk(YELLOW, BLACK, "hw: xhci: host %#018lx use 64 byte context\n", host);
		host->flag |= hw_usb_xhci_Host_flag_Ctx64;
	}

	// stop the host
	hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_cmd, hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_cmd) & ~1u);
	// wait for the host to stop
	{
		int timeout = 30;
		do {
			if (hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_status) & (1u << 0)) break;
			timer_mdelay(1);
		} while (--timeout);
		if (~hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_status) & (1u << 0)) {
			printk(RED, BLACK, "hw: xhci: host %#018lx failed to stop\n", host);
			return res_FAIL;
		}
		printk(GREEN, BLACK, "hw: xhci: host %#018lx stopped\n", host);
	}

	// reset the host
	if (hw_usb_xhci_reset(host) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to reset\n", host);
		return res_FAIL;
	}
	printk(GREEN, BLACK, "hw: xhci: host %#018lx reset\n", host);

	// wait for ready of the host
	if (hw_usb_xhci_waitReady(host) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to wait ready\n", host);
		return res_FAIL;
	}
	printk(GREEN, BLACK, "hw: xhci: host %#018lx ready\n", host);

	// set max slot field of config register
	hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_cfg,
		(hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_cfg) & ((1u << 10) - 1)) | (1u << 8) | hw_usb_xhci_mxSlot(host));
	
	// determine the number of interrupts
	host->intrNum = min(4, hw_usb_xhci_mxIntr(host));

	host->intr = mm_kmalloc(sizeof(intr_Desc) * host->intrNum, mm_Attr_Shared, NULL);
	if (host->intr == NULL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate interrupt descriptor\n", host);
		return res_FAIL;
	}

	if (host->flag & hw_usb_xhci_Host_flag_Msix) {
		host->msixCap = msix;
		printk(WHITE, BLACK, "hw: xhci: host %#018lx use msix\n", host);

		int vecNum = hw_pci_MsixCap_vecNum(host->msixCap);

		hw_pci_MsixTbl *tbl = hw_pci_getMsixTbl(msix, host->pci->cfg);
		
		printk(WHITE, BLACK, "hw: xhci: host %#018lx msixtbl:%#018lx vecNum:%d msgCtrl:%#010x\n", host, tbl, vecNum + 1, msix->msgCtrl);

		for (int i = 0; i < host->intrNum; i++) {
			intr_initDesc(host->intr + i, hw_usb_xhci_msiHandler, (u64)host | i, "xhci msi", &hw_pci_intrCtrl);
		}
		if (hw_pci_allocMsix(host->msixCap, host->pci->cfg, host->intr, host->intrNum) == res_FAIL) {
			printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate msix interrupt\n", host);
			return res_FAIL;
		}
	} else {
		host->msiCap = msi;
		printk(WHITE, BLACK, "hw: xhci: host %#018lx use msi\n", host);

		int vecNum = hw_pci_MsiCap_vecNum(host->msiCap);
		host->intrNum = vecNum = min(vecNum, host->intrNum);

		printk(WHITE, BLACK, "hw: xhci: host %#018lx msi: vecNum:%d mgsCtrl:%#010x\n", host, vecNum, msi->msgCtrl);

		for (int i = 0; i < vecNum; i++) intr_initDesc(host->intr + i, hw_usb_xhci_msiHandler, (u64)host | i, "xhci msi", &hw_pci_intrCtrl);
		if (hw_pci_allocMsi(host->msiCap, host->intr, host->intrNum) == res_FAIL) {
			printk(RED, BLACK, "hw: xhci: host %#018lx failed to set msi interrupt\n", host);
			return res_FAIL;
		}
	}
	hw_pci_disableIntx(host->pci->cfg);
	printk(GREEN, BLACK, "hw: xhci: host %#018lx msi/msix set\n", host);

	{
		int res;
		// enable interrupt
		if (host->flag & hw_usb_xhci_Host_flag_Msix) res = hw_pci_enableMsixAll(host->msixCap, host->pci->cfg, host->intr, host->intrNum);
		else res = hw_pci_enableMsiAll(host->msiCap, host->intr, host->intrNum);

		if (res == res_SUCC) printk(GREEN, BLACK, "hw: xhci: host %#018lx msi/msix enabled\n", host);
		else {
			printk(RED, BLACK, "hw: xhci: host %#018lx msi/msix enable failed\n", host);
			return res_FAIL;
		}
	}
	
	// allocate device context
	u64 *dcbaa = mm_kmalloc(2048, mm_Attr_Shared, NULL);
	if (dcbaa == NULL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate dcbaa\n", host);
		return res_FAIL;
	}
	hw_usb_xhci_OpReg_write64(host, hw_usb_xhci_Host_opReg_dcbaa, mm_dmas_virt2Phys(dcbaa));

	// allocate scratchpad array and items
	{
		u32 mxScrSz = max(63, hw_usb_xhci_mxScrSz(host));
		u64 *scrArr = mm_kmalloc(upAlign((mxScrSz + 1) * sizeof(u64), Page_4KSize), mm_Attr_Shared, NULL);
		if (scrArr == NULL) {
			printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate scratchpad array\n", host);
			return res_FAIL;
		}
		for (u32 i = 0; i <= mxScrSz; i++) {
			void *item = mm_kmalloc(Page_4KSize, mm_Attr_Shared, NULL);
			if (item == NULL) {
				printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate scratchpad item\n", host);
				return res_FAIL;
			}
			memset(item, 0, Page_4KSize);
			scrArr[i] = mm_dmas_virt2Phys(item);
		}
		// the address of scratchpad array is stored in dcbaa[0]
		dcbaa[0] = mm_dmas_virt2Phys(scrArr);
	}

	// allocate device context
	{
		host->devCtx = mm_kmalloc(max(64, sizeof(hw_usb_xhci_DevCtx *) * (hw_usb_xhci_mxSlot(host) + 1)), mm_Attr_Shared, NULL);
		if (host->devCtx == NULL) {
			printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate device context array\n", host);
			return res_FAIL;
		}
		u64 ctxSize = host->flag & hw_usb_xhci_Host_flag_Ctx64 ? sizeof(hw_usb_xhci_DevCtx64) : sizeof(hw_usb_xhci_DevCtx32);
		for (u32 i = hw_usb_xhci_mxSlot(host); i; i--) {
			host->devCtx[i] = mm_kmalloc(ctxSize, mm_Attr_Shared, NULL);
			if (host->devCtx[i] == NULL) {
				printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate device context\n", host);
				return res_FAIL;
			}
			dcbaa[i] = mm_dmas_virt2Phys(host->devCtx[i]);
		}
	}

	// release this host from BIOS
	for (void *extCap = hw_usb_xhci_nxtExtCap(host, NULL); extCap; extCap = hw_usb_xhci_nxtExtCap(host, extCap)) {
		if (hw_usb_xhci_xhci_getExtCapId(extCap) == 0x01) {
			printk(WHITE, BLACK, "hw: xhci: host %#018lx legacy support previous value: %#010x\n", host, hal_read32((u64)extCap));
			hal_write32((u64)extCap, hal_read32((u64)extCap) | (1u << 24));
			int timeout = 10;
			while (timeout--) {
				u32 cur = hal_read32((u64)extCap) & ((1u << 24) | (1u << 16));
				if (cur == (1u << 24)) break;
				timer_mdelay(1);
			}
			if (hal_read32((u64)extCap) & (1u << 16)) {
				printk(RED, BLACK, "hw: xhci: host %#018lx failed to release legacy support\n", host);
				return res_FAIL;
			}
			printk(GREEN, BLACK, "hw: xhci: host %#018lx release legacy support\n", host);
		}
	}

	// allocate command ring
	host->cmdRing = hw_usb_xhci_allocRing(host, hw_usb_xhci_Ring_mxSz);
	if (host->cmdRing == NULL) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate command ring\n", host);
		return res_FAIL;
	}

	hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_dnCtrl, (1u << 1) | (hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_dnCtrl) & ~0xffffu));

	// allocate event rings
	{	
		int tblSize = min(4, hw_usb_xhci_mxERST(host));
		host->eveRings = mm_kmalloc(sizeof(hw_usb_xhci_EveRing *) * host->intrNum, mm_Attr_Shared, NULL);
		for (int i = 0; i < host->intrNum; i++) {
			host->eveRings[i] = hw_usb_xhci_allocEveRing(host, tblSize, hw_usb_xhci_Ring_mxSz);
			if (host->eveRings[i] == NULL) {
				printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate event ring\n", host);
				return res_FAIL;
			}
			u64 *ringTbl = mm_kmalloc(max(128, 2 * sizeof(u64) * tblSize), mm_Attr_Shared, NULL);
			if (ringTbl == NULL) {
				printk(RED, BLACK, "hw: xhci: host %#018lx failed to allocate event ring table\n", host);
				return res_FAIL;
			}
			for (int j = 0; j < tblSize; j++)
				ringTbl[(j << 1) | 0] = mm_dmas_virt2Phys(host->eveRings[i]->rings[j]),
				ringTbl[(j << 1) | 1] = hw_usb_xhci_Ring_mxSz;
			
			hw_usb_xhci_IntrReg_write32(host, i, hw_usb_xhci_intrReg_IMod, 0);
			hw_usb_xhci_IntrReg_write32(host, i, hw_usb_xhci_intrReg_TblSize, tblSize | (hw_usb_xhci_IntrReg_read32(host, i, hw_usb_xhci_intrReg_TblSize) & ~0xffffu));
			hw_usb_xhci_IntrReg_write64(host, i, hw_usb_xhci_intrReg_TblAddr, mm_dmas_virt2Phys(ringTbl) | (hw_usb_xhci_IntrReg_read64(host, i, hw_usb_xhci_intrReg_TblAddr) & 0x3ful));
			// set dequeue pointer to the first trb of the first ring
			hw_usb_xhci_IntrReg_write64(host, i, hw_usb_xhci_intrReg_DeqPtr, ringTbl[0] | (1ul << 3));
			hw_usb_xhci_IntrReg_write32(host, i, hw_usb_xhci_intrReg_IMan, (1ul << 1) | (1ul << 0) | (hw_usb_xhci_IntrReg_read32(host, i, hw_usb_xhci_intrReg_IMan) & ~0x3u));
		}
	}

	// create manager task

	// power off all the ports and restart them
	for (u32 i = hw_usb_xhci_mxPort(host); i > 0; i--) hw_usb_xhci_PortReg_write(host, i, hw_usb_xhci_Host_portReg_sc, 0);

	for (u32 i = hw_usb_xhci_mxPort(host); i > 0; i--) hw_usb_xhci_PortReg_write(host, i, hw_usb_xhci_Host_portReg_sc, hw_usb_xhci_Host_portReg_sc_Power | hw_usb_xhci_Host_portReg_sc_AllEve);

	// set crcr
	hw_usb_xhci_OpReg_write64(host, hw_usb_xhci_Host_opReg_crCtrl, mm_dmas_virt2Phys(host->cmdRing->trbs) | 1);
	// restart the host
	hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_cmd, (1u << 0) | (1u << 2) | (1u << 3));

	printk(GREEN, BLACK, "hw: xhci: host %#018lx initialized. cmd:%#010x state:%#010x\n", host, 
		hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_cmd), hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_status));

	// set empty command to test
	{
		hw_usb_xhci_Request *req = hw_usb_xhci_makeRequest(1, hw_usb_xhci_Request_flags_Command);
		hw_usb_xhci_TRB_setType(&req->input[0], hw_usb_xhci_TRB_Type_NoOpCmd);
		for (int i = 0; i < hw_usb_xhci_Ring_mxSz * 3 + 10; i++) {
			printk(WHITE, BLACK, "[%d ]", i);
			hw_usb_xhci_request(host, host->cmdRing, req, 0, 0);
		}
	}

	return res_SUCC;
}

int hw_usb_xhci_init() {
	if (_searchInPci() == res_FAIL) return res_FAIL;
	for (ListNode *xhciDevNd = SafeList_getHead(&hw_usb_xhci_devLst); xhciDevNd != &hw_usb_xhci_devLst.head; xhciDevNd = xhciDevNd->next) {
		if (_initHost(container(xhciDevNd, hw_usb_xhci_Host, lst)) == res_FAIL) return res_FAIL;
	}
	return res_SUCC;
}

