#include <hardware/usb/xhci/api.h>
#include <lib/algorithm.h>
#include <mm/mm.h>
#include <timer/api.h>
#include <screen/screen.h>

SafeList hw_usb_xhci_hostLst;
hw_Driver hw_usb_xhci_drv;

static int hw_usb_xhci_chk(hw_Device *dev) {
	if (dev->parent != &hw_pci_rootDev) return res_FAIL;
	hw_pci_Dev *pciDev = container(dev, hw_pci_Dev, device);
	if (pciDev->cfg->class != 0x0c || pciDev->cfg->subclass != 0x03 || pciDev->cfg->progIf != 0x30) return res_FAIL;

	printk(WHITE, BLACK, "hw: xhci: find controller: %02x:%02x:%01x vendor=%04x,device=%04x\n", 
		pciDev->busId, pciDev->devId, pciDev->funcId, pciDev->cfg->vendorId, pciDev->cfg->deviceId);
	return res_SUCC;
}

__always_inline__ void _disablePartnerCtrl(hw_usb_xhci_Host *host) {
	if (host->pci.cfg->vendorId == 0x8086 && host->pci.cfg->deviceId == 0x1e31 && host->pci.cfg->revisionId == 4) {
		*(u32 *)((u64)host->pci.cfg + 0xd8) = 0xffffffffu;
		*(u32 *)((u64)host->pci.cfg + 0xd0) = 0xffffffffu;
	}
}

__always_inline__ int _getRegAddr(hw_usb_xhci_Host *host) {
	u64 phyAddr = hw_pci_Cfg_getBar(&host->pci.cfg->tp0.bar[0]);

	host->capRegAddr = (u64)mm_dmas_phys2Virt(phyAddr);

	printk(WHITE, BLACK, "hw: xhci: host %p: %08x %08x capRegAddr:%p\n", host, host->pci.cfg->tp0.bar[0], host->pci.cfg->tp0.bar[1], host->capRegAddr);
	
	// map this address in dmas
	if (mm_dmas_map(phyAddr) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to map cap regs\n", host);
		return res_FAIL;
	}
	host->opRegAddr = host->capRegAddr + hw_usb_xhci_CapReg_capLen(host);
	host->rtRegAddr = host->capRegAddr + hw_usb_xhci_CapReg_rtsOff(host);
	host->dbRegAddr = host->capRegAddr + hw_usb_xhci_CapReg_dbOff(host);

	// map address in dmas
	if (mm_dmas_map(mm_dmas_virt2Phys(host->opRegAddr)) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to map op regs\n", host);
		return res_FAIL;
	}
	if (mm_dmas_map(mm_dmas_virt2Phys(host->rtRegAddr)) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to map rt regs\n", host);
		return res_FAIL;
	}
	if (mm_dmas_map(mm_dmas_virt2Phys(host->dbRegAddr)) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to map hcc regs\n", host);
		return res_FAIL;
	}
	return res_SUCC;
}

__always_inline__ int _initHost(hw_usb_xhci_Host *host) {
	// check the capability list
	if (!(host->pci.cfg->status & (1u << 4))) {
		printk(RED, BLACK, "hw: xhci: pci device %02x:%02x:%02x has no capability list\n", host->pci.busId, host->pci.devId, host->pci.funcId);
		return res_FAIL;
	}

	hw_pci_MsixCap *msix = NULL;
	hw_pci_MsiCap *msi = NULL;
	// search for msi/msix capability descriptor
	for (hw_pci_CapHdr *hdr = hw_pci_getNxtCap(host->pci.cfg, NULL); hdr; hdr = hw_pci_getNxtCap(host->pci.cfg, hdr)) {
		switch (hdr->capId) {
			case hw_pci_CapHdr_capId_MSI:
				msi = container(container(hdr, hw_pci_MsiCap32, hdr), hw_pci_MsiCap, cap32);
				break;
			case hw_pci_CapHdr_capId_MSIX:
				msix = container(hdr, hw_pci_MsixCap, hdr);
				break;
		}
	}
	if (!msix && !msi) {
		printk(RED, BLACK, "hw: xhci: controller %p has no msi/msix descriptor\n", host);
		return res_FAIL;
	}
	printk(WHITE, BLACK, "hw: xhci: controller %p msi:%p msix:%p\n", host, msi, msix);

	if (msix) host->flag |= hw_usb_xhci_Host_flag_Msix;

	printk(WHITE, BLACK, "hw: xhci: initialize device %p: vendor:%04x device:%04x revision:%04x\n",
		host, host->pci.cfg->vendorId, host->pci.cfg->deviceId, host->pci.cfg->revisionId);

	_disablePartnerCtrl(host);

	// get address of registers
	if (_getRegAddr(host) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to get register address\n", host);
		return res_FAIL;
	}

	// read information from capability registers
	printk(WHITE, BLACK, "hw: xhci: host %p: capReg:%p opReg:%p rtReg:%p dbReg:%p\n",
		host, host->capRegAddr, host->opRegAddr, host->rtRegAddr, host->dbRegAddr);
	printk(WHITE, BLACK, "hw: xhci: host %p: mxslot:%d mxintr:%d mxport:%d mxerst:%d mxscr:%d\n",
		host, hw_usb_xhci_mxSlot(host), hw_usb_xhci_mxIntr(host), hw_usb_xhci_mxPort(host), hw_usb_xhci_mxERST(host), hw_usb_xhci_mxScrSz(host));

	// check if the controller support neccessary features: 4K page, 64bit address, port power control
	if (~hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_pgSize) & 0x1) {
		printk(RED, BLACK, "hw: xhci: host %p does not support 4K page\n", host);
		return res_FAIL;
	}
	if (~hw_usb_xhci_CapReg_hccParam(host, 1) & 0x1) {
		printk(RED, BLACK, "hw: xhci: host %p does not support 64bit address\n", host);
		return res_FAIL;
	}
	if (hw_usb_xhci_CapReg_hccParam(host, 1) & (1u << 3)) {
		printk(RED, BLACK, "hw: xhci: host %p doest not support port power control\n", host);
		return res_FAIL;
	}
	// check the context size
	if (hw_usb_xhci_CapReg_hccParam(host, 1) & (1u << 2)) {
		printk(YELLOW, BLACK, "hw: xhci: host %p use 64 byte context\n", host);
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
			printk(RED, BLACK, "hw: xhci: host %p failed to stop\n", host);
			return res_FAIL;
		}
		printk(GREEN, BLACK, "hw: xhci: host %p stopped cmd:%#010x status:%#010x\n", 
			host, hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_cmd), hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_status));
	}

	// reset the host
	if (hw_usb_xhci_reset(host) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to reset\n", host);
		return res_FAIL;
	}
	printk(GREEN, BLACK, "hw: xhci: host %p reset\n", host);

	// wait for ready of the host
	if (hw_usb_xhci_waitReady(host) == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to wait ready\n", host);
		return res_FAIL;
	}
	printk(GREEN, BLACK, "hw: xhci: host %p ready\n", host);

	// set max slot field of config register
	hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_cfg,
		(hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_cfg) & ((1u << 10) - 1)) | (1u << 8) | hw_usb_xhci_mxSlot(host));
	
	// determine the number of interrupts
	host->intrNum = min(4, hw_usb_xhci_mxIntr(host));

	host->intr = mm_kmalloc(sizeof(intr_Desc) * host->intrNum, mm_Attr_Shared, NULL);
	if (host->intr == NULL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to allocate interrupt descriptor\n", host);
		return res_FAIL;
	}

	if (host->flag & hw_usb_xhci_Host_flag_Msix) {
		host->msixCap = msix;
		printk(WHITE, BLACK, "hw: xhci: host %p use msix\n", host);

		int vecNum = hw_pci_MsixCap_vecNum(host->msixCap);
		vecNum = host->intrNum = min(vecNum, host->intrNum);

		hw_pci_MsixTbl *tbl = hw_pci_getMsixTbl(msix, host->pci.cfg);
		
		printk(WHITE, BLACK, "hw: xhci: host %p msixtbl:%p vecNum:%d msgCtrl:%#010x\n", host, tbl, vecNum, msix->msgCtrl);

		for (int i = 0; i < host->intrNum; i++) {
			hw_pci_initIntr(host->intr + i, hw_usb_xhci_msiHandler, (u64)host | i, "xhci msix");
		}
		if (hw_pci_allocMsix(host->msixCap, host->pci.cfg, host->intr, host->intrNum) == res_FAIL) {
			printk(RED, BLACK, "hw: xhci: host %p failed to allocate msix interrupt\n", host);
			return res_FAIL;
		}
	} else {
		host->msiCap = msi;
		printk(WHITE, BLACK, "hw: xhci: host %p use msi\n", host);

		int vecNum = hw_pci_MsiCap_vecNum(host->msiCap);
		host->intrNum = vecNum = min(vecNum, host->intrNum);

		printk(WHITE, BLACK, "hw: xhci: host %p msi: %s vecNum:%d mgsCtrl:%#010x\n", 
				host, hw_pci_MsiCap_is64(msi) ? "64B" : "32B", vecNum, *hw_pci_MsiCap_msgCtrl(msi));

		for (int i = 0; i < vecNum; i++)
			hw_pci_initIntr(host->intr + i, hw_usb_xhci_msiHandler, (u64)host | i, "xhci msi");
		if (hw_pci_allocMsi(host->msiCap, host->intr, host->intrNum) == res_FAIL) {
			printk(RED, BLACK, "hw: xhci: host %p failed to set msi interrupt\n", host);
			return res_FAIL;
		}
	}
	hw_pci_disableIntx(host->pci.cfg);
	printk(GREEN, BLACK, "hw: xhci: host %p msi/msix set\n", host);

	{
		int res;
		// enable interrupt
		if (host->flag & hw_usb_xhci_Host_flag_Msix) res = hw_pci_enableMsixAll(host->msixCap, host->pci.cfg, host->intr, host->intrNum);
		else res = hw_pci_enableMsiAll(host->msiCap, host->intr, host->intrNum);

		if (res == res_SUCC) printk(GREEN, BLACK, "hw: xhci: host %p msi/msix enabled\n", host);
		else {
			printk(RED, BLACK, "hw: xhci: host %p msi/msix enable failed\n", host);
			return res_FAIL;
		}
	}
	
	// allocate device context
	u64 *dcbaa = mm_kmalloc(2048, mm_Attr_Shared, NULL);
	if (dcbaa == NULL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to allocate dcbaa\n", host);
		return res_FAIL;
	}
	hw_usb_xhci_OpReg_write64(host, hw_usb_xhci_Host_opReg_dcbaa, mm_dmas_virt2Phys(dcbaa));

	// allocate scratchpad array and items
	{
		u32 mxScrSz = max(63, hw_usb_xhci_mxScrSz(host));
		u64 *scrArr = mm_kmalloc(upAlign((mxScrSz + 1) * sizeof(u64), Page_4KSize), mm_Attr_Shared, NULL);
		if (scrArr == NULL) {
			printk(RED, BLACK, "hw: xhci: host %p failed to allocate scratchpad array\n", host);
			return res_FAIL;
		}
		for (u32 i = 0; i <= mxScrSz; i++) {
			void *item = mm_kmalloc(Page_4KSize, mm_Attr_Shared, NULL);
			if (item == NULL) {
				printk(RED, BLACK, "hw: xhci: host %p failed to allocate scratchpad item\n", host);
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
			printk(RED, BLACK, "hw: xhci: host %p failed to allocate device context array\n", host);
			return res_FAIL;
		}
		u64 ctxSize = host->flag & hw_usb_xhci_Host_flag_Ctx64 ? sizeof(hw_usb_xhci_DevCtx64) : sizeof(hw_usb_xhci_DevCtx32);
		for (u32 i = hw_usb_xhci_mxSlot(host); i; i--) {
			host->devCtx[i] = mm_kmalloc(ctxSize, mm_Attr_Shared, NULL);
			if (host->devCtx[i] == NULL) {
				printk(RED, BLACK, "hw: xhci: host %p failed to allocate device context\n", host);
				return res_FAIL;
			}
			dcbaa[i] = mm_dmas_virt2Phys(host->devCtx[i]);
		}
	}

	// allocate device task array
	host->dev = mm_kmalloc(sizeof(hw_usb_xhci_Device *) * (hw_usb_xhci_mxSlot(host) + 1), mm_Attr_Shared, NULL);
	if (host->dev == NULL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to allocate device array\n", host);
		return res_FAIL;
	}
	memset(host->dev, 0, sizeof(hw_usb_xhci_Device *) * (hw_usb_xhci_mxSlot(host) + 1));
	
	host->portDev = mm_kmalloc(sizeof(hw_usb_xhci_Device *) * (hw_usb_xhci_mxPort(host) + 1), mm_Attr_Shared, NULL);
	if (host->portDev == NULL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to allocate port device array\n", host);
		return res_FAIL;
	}
	memset(host->portDev, 0, sizeof(hw_usb_xhci_Device *) * (hw_usb_xhci_mxPort(host) + 1));

	// release this host from BIOS
	for (void *extCap = hw_usb_xhci_nxtExtCap(host, NULL); extCap; extCap = hw_usb_xhci_nxtExtCap(host, extCap)) {
		if (hw_usb_xhci_xhci_getExtCapId(extCap) == hw_usb_xhci_Ext_Id_Legacy) {
			printk(WHITE, BLACK, "hw: xhci: host %p legacy support previous value: %#010x\n", host, hal_read32((u64)extCap));
			hal_write32((u64)extCap, hal_read32((u64)extCap) | (1u << 24));
			int timeout = 10;
			while (timeout--) {
				u32 cur = hal_read32((u64)extCap) & ((1u << 24) | (1u << 16));
				if (cur == (1u << 24)) break;
				timer_mdelay(1);
			}
			if (hal_read32((u64)extCap) & (1u << 16)) {
				printk(RED, BLACK, "hw: xhci: host %p failed to release legacy support\n", host);
				return res_FAIL;
			}
			printk(GREEN, BLACK, "hw: xhci: host %p release legacy support\n", host);
		}
	}

	// allocate command ring
	host->cmdRing = hw_usb_xhci_allocRing(host, hw_usb_xhci_Ring_mxSz);
	if (host->cmdRing == NULL) {
		printk(RED, BLACK, "hw: xhci: host %p failed to allocate command ring\n", host);
		return res_FAIL;
	}

	hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_dnCtrl, (1u << 1) | (hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_dnCtrl) & ~0xffffu));

	// allocate event rings
	{	
		int tblSize = min(4, hw_usb_xhci_mxERST(host));
		printk(WHITE, BLACK, "hw: xhci: host %p event ring tbl size:%d\n", host, tblSize);
		host->eveRings = mm_kmalloc(sizeof(hw_usb_xhci_EveRing *) * host->intrNum, mm_Attr_Shared, NULL);
		for (int i = 0; i < host->intrNum; i++) {
			host->eveRings[i] = hw_usb_xhci_allocEveRing(host, tblSize, hw_usb_xhci_Ring_mxSz);
			if (host->eveRings[i] == NULL) {
				printk(RED, BLACK, "hw: xhci: host %p failed to allocate event ring\n", host);
				return res_FAIL;
			}
			u64 *ringTbl = mm_kmalloc(max(128, 2 * sizeof(u64) * tblSize), mm_Attr_Shared, NULL);
			if (ringTbl == NULL) {
				printk(RED, BLACK, "hw: xhci: host %p failed to allocate event ring table\n", host);
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

	printk(GREEN, BLACK, "hw: xhci: host %p initialized. cmd:%#010x state:%#010x\n", host, 
		hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_cmd), hw_usb_xhci_OpReg_read32(host, hw_usb_xhci_Host_opReg_status));

	// for qemu xhci, call portChange manually
	if (host->pci.cfg->vendorId == 0x1b36 && host->pci.cfg->deviceId == 0x000d) {
		for (int i = hw_usb_xhci_mxPort(host); i > 0; i--)
			if (hw_usb_xhci_PortReg_read(host, i, hw_usb_xhci_Host_portReg_sc) & 1)
				hw_usb_xhci_portChange(host, i);
	}
 
	return res_SUCC;
}

static int hw_usb_xhci_cfg(hw_Device *dev) {
	hw_usb_xhci_Host *host = mm_kmalloc(sizeof(hw_usb_xhci_Host), mm_Attr_Shared, NULL);
	memset(host, 0, sizeof(hw_usb_xhci_Host));

	hw_pci_extend(container(dev, hw_pci_Dev, device), &host->pci);

	SafeList_init(&host->devLst);
	SafeList_init(&host->ringLst);

	SafeList_insTail(&hw_usb_xhci_hostLst, &host->lst);

	return _initHost(host);
}

int hw_usb_xhci_init() {
	SafeList_init(&hw_usb_xhci_hostLst);

	hw_driver_initDriver(&hw_usb_xhci_drv, "hw: usb: xhci", hw_usb_xhci_chk, hw_usb_xhci_cfg, NULL, NULL);
	hw_driver_register(&hw_usb_xhci_drv);
	return res_SUCC;
}

__always_inline__ u32 _EpCtx_getDefaultMxPackSz0(u32 speed) {
	switch (speed) {
		case hw_usb_xhci_Speed_Full :
		case hw_usb_xhci_Speed_High : return 64;
		case hw_usb_xhci_Speed_Super : return 512;
		case hw_usb_xhci_Speed_Low : return 8;
	}
	return 512;
}

int hw_usb_xhci_devInit(hw_usb_xhci_Device *dev) {
	// get slot ID
	hw_usb_xhci_Host *host = dev->host;

	hw_usb_xhci_Request *req = hw_usb_xhci_makeRequest(1, hw_usb_xhci_Request_flags_Command), *req2 = hw_usb_xhci_makeRequest(3, 0);
	hw_usb_xhci_TRB_setTp(&req->input[0], hw_usb_xhci_TRB_Tp_EnblSlot);
	
	// get slot Type
	if (dev->flag & hw_usb_xhci_Device_flag_Direct) {
		// read supported protocol capability
		printk(WHITE, BLACK, "hw: xhci: dev %p slot tp:%d\n", dev, hw_usb_xhci_getSlotTp(host, dev->portId));
		hw_usb_xhci_TRB_setSlotTp(&req->input[0], hw_usb_xhci_getSlotTp(host, dev->portId));
		dev->speed = (hw_usb_xhci_PortReg_read(host, dev->portId, hw_usb_xhci_Host_portReg_sc) >> 10) & ((1u << 4) - 1);
	} else hw_usb_xhci_TRB_setSlotTp(&req->input[0], 0), dev->speed = 0;

	hw_usb_xhci_request(host, host->cmdRing, req, NULL, 0);
	if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "hw: xhci: dev %p failed to enable slot\n", dev);
		hw_usb_xhci_freeRequest(req);
		return res_FAIL;
	}

	printk(GREEN, BLACK, "hw: xhci: dev %p enabled slot:%d speed:%d\n", dev, req->res.ctrl >> 24, dev->speed);
	dev->slotId = req->res.ctrl >> 24;
	host->dev[dev->slotId] = dev;

	dev->ctx = host->devCtx[dev->slotId];
	dev->inCtx = hw_usb_xhci_allocInCtx(host);

	if (dev->ctx == NULL || dev->inCtx == NULL) {
		printk(RED, BLACK, "hw: xhci: dev %p failed to allocate context\n", dev);
		hw_usb_xhci_freeRequest(req);
		return res_FAIL;
	}

	memset(&req->input[0], 0, sizeof(hw_usb_xhci_TRB));
	hw_usb_xhci_TRB_setTp(&req->input[0], hw_usb_xhci_TRB_Tp_AddrDev);
	hw_usb_xhci_TRB_setSlot(&req->input[0], dev->slotId);
	hw_usb_xhci_TRB_setData(&req->input[0], mm_dmas_virt2Phys(dev->inCtx));

	{
		void *ctrlCtx = hw_usb_xhci_getCtxEntry(host, dev->inCtx, hw_usb_xhci_InCtx_Ctrl);
		hw_usb_xhci_writeCtx(ctrlCtx, 1, ~0x0u, (1u << 0) | (1u << 1));
	}

	{
		void *slotCtx = hw_usb_xhci_getCtxEntry(host, dev->inCtx, hw_usb_xhci_InCtx_Slot);
		hw_usb_xhci_writeCtx(slotCtx, 0, hw_usb_xhci_SlotCtx_ctxEntries, 1);
		hw_usb_xhci_writeCtx(slotCtx, 0, hw_usb_xhci_SlotCtx_speed, dev->speed);
		hw_usb_xhci_writeCtx(slotCtx, 1, hw_usb_xhci_SlotCtx_rootPortNum, dev->portId);

		// set interrupt target to slotId % host->intrNum
		dev->intrTrg = dev->slotId % host->intrNum;
		hw_usb_xhci_writeCtx(slotCtx, 2, hw_usb_xhci_SlotCtx_intrTarget, dev->intrTrg);
		printk(WHITE, BLACK, "hw: xhci: dev %p interrupt target=%d\n", dev, dev->intrTrg);
	}

	{
		void *ep0 = hw_usb_xhci_getCtxEntry(host, dev->inCtx, hw_usb_xhci_InCtx_CtrlEp);
		hw_usb_xhci_writeCtx(ep0, 1, hw_usb_xhci_EpCtx_epTp, hw_usb_xhci_EpCtx_epTp_Ctrl);
		hw_usb_xhci_writeCtx(ep0, 1, hw_usb_xhci_EpCtx_CErr, 3);
		hw_usb_xhci_writeCtx(ep0, 1, hw_usb_xhci_EpCtx_mxPackSize, _EpCtx_getDefaultMxPackSz0(dev->speed));
		
		dev->epRing[hw_usb_xhci_DevCtx_CtrlEp] = hw_usb_xhci_allocRing(host, hw_usb_xhci_Ring_mxSz >> 1);
		if (dev->epRing[hw_usb_xhci_DevCtx_CtrlEp] == NULL) {
			printk(RED, BLACK, "hw: xhci: dev %p failed to allocate ep ring\n", dev);
			goto Fail_To_Initialize;
		}

		hw_usb_xhci_writeCtx64(ep0, 2, mm_dmas_virt2Phys(dev->epRing[hw_usb_xhci_DevCtx_CtrlEp]->trbs));
		hw_usb_xhci_writeCtx(ep0, 2, hw_usb_xhci_EpCtx_dcs, 1);

		hw_usb_xhci_writeCtx(ep0, 4, hw_usb_xhci_EpCtx_aveTrbLen, 8);
	}

	hw_usb_xhci_request(host, host->cmdRing, req, NULL, 0);
	if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "hw: xhci: dev %p failed to address device\n", dev);
		goto Fail_To_Initialize;
	}
	// printk(GREEN, BLACK, "hw: xhci: dev %p addressed\n", dev);
	
	// get device descriptor
	dev->devDesc = mm_kmalloc(0xff, 0, NULL);

	// make data ctrl request
	hw_usb_xhci_mkCtrlDataReq(req2, hw_usb_xhci_mkCtrlReqSetup(0x80, 0x6, 0x0100, 0x0, 0x8), hw_usb_xhci_TRB_ctrl_dir_in, dev->devDesc, 8);

	hw_usb_xhci_request(host, dev->epRing[hw_usb_xhci_DevCtx_CtrlEp], req2, dev, hw_usb_xhci_DbReq_make(hw_usb_xhci_DevCtx_CtrlEp, 0));
	if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "hw: xhci: dev %p failed to get first 8 bytes of device descriptor, code=%d\n", dev, hw_usb_xhci_TRB_getCmplCode(&req->res));
		goto Fail_To_Initialize;
	}

	// set max package size 0
	{
		u32 mxPkgSz0 = (dev->speed >= 4 ? (1u << (dev->devDesc->bMxPackSz0)) : dev->devDesc->bMxPackSz0);
		hw_usb_xhci_writeCtx(hw_usb_xhci_getCtxEntry(host, dev->inCtx, hw_usb_xhci_InCtx_CtrlEp), 1, hw_usb_xhci_EpCtx_mxPackSize, mxPkgSz0);
	}

	hw_usb_xhci_TRB_setTp(&req->input[0], hw_usb_xhci_TRB_Tp_EvalCtx);
	hw_usb_xhci_request(host, host->cmdRing, req, 0, 0);
	if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "hw: xhci: dev %p failed to evaluate context, code=%d\n", dev, hw_usb_xhci_TRB_getCmplCode(&req->res));
		goto Fail_To_Initialize;
	}
	hw_usb_xhci_mkCtrlDataReq(req2, hw_usb_xhci_mkCtrlReqSetup(0x80, 0x6, 0x0100, 0x0, 0xff), hw_usb_xhci_TRB_ctrl_dir_in, dev->devDesc, 0xff);
	hw_usb_xhci_request(host, dev->epRing[hw_usb_xhci_DevCtx_CtrlEp], req2, dev, hw_usb_xhci_DbReq_make(hw_usb_xhci_DevCtx_CtrlEp, 0));
	if (hw_usb_xhci_TRB_getCmplCode(&req2->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
		printk(RED, BLACK, "hw: xhci: dev %p failed to get device description, code=%d\n", dev, hw_usb_xhci_TRB_getCmplCode(&req->res));
		goto Fail_To_Initialize;
	}
	printk(GREEN, BLACK, "hw: xhci: dev %p get device descriptor.\n", dev);

	dev->cfgDesc = mm_kmalloc(sizeof(void *) & dev->devDesc->bNumCfg, 0, NULL);
	for (int i = 0; i < dev->devDesc->bNumCfg; i++) {
		dev->cfgDesc[i] = mm_kmalloc(0xff, 0, NULL);
		hw_usb_xhci_mkCtrlDataReq(req2, hw_usb_xhci_mkCtrlReqSetup(0x80, 0x6, 0x0200 | i, 0x0, 0xff), hw_usb_xhci_TRB_ctrl_dir_in, dev->cfgDesc[i], 0xff);
		hw_usb_xhci_request(host, dev->epRing[hw_usb_xhci_DevCtx_CtrlEp], req2, dev, hw_usb_xhci_DbReq_make(hw_usb_xhci_DevCtx_CtrlEp, 0));
		if (hw_usb_xhci_TRB_getCmplCode(&req2->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
			printk(RED, BLACK, "hw: xhci: dev %p failed to get configuration description #%d, code=%d\n",
				dev, i, hw_usb_xhci_TRB_getCmplCode(&req->res));
			goto Fail_To_Initialize;
		}
	}
	printk(GREEN, BLACK, "hw: xhci: dev %p get configuration descriptor.\n", dev);

	hw_usb_xhci_freeRequest(req);
	hw_usb_xhci_freeRequest(req2);

	return res_SUCC;
	Fail_To_Initialize:
	hw_usb_xhci_freeRequest(req);
	hw_usb_xhci_freeRequest(req2);
	return res_FAIL;
} 