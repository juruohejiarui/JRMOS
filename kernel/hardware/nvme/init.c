#include <hardware/nvme.h>
#include <timer/api.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include <lib/algorithm.h>

hw_Driver hw_nvme_drv;

intr_handlerDeclare(hw_nvme_msiHandler) {

}

int hw_nvme_chk(hw_Device *dev) {
	if (dev->parent != &hw_pci_rootDev) return res_FAIL;
	hw_pci_Dev *pciDev = container(dev, hw_pci_Dev, device);
	if (pciDev->cfg->class != 0x01 || pciDev->cfg->subclass != 0x08) return res_FAIL;
	printk(WHITE, BLACK, "hw: nvme: find controller: %02x:%02x:%01x vendor=%04x device=%04x\n",
		pciDev->busId, pciDev->devId, pciDev->funcId, pciDev->cfg->vendorId, pciDev->cfg->deviceId);
	return res_SUCC;
}

__always_inline__ int _getRegAddr(hw_nvme_Host *host) {
	host->capRegAddr = (u64)mm_dmas_phys2Virt(hw_pci_Cfg_getBar(&host->pci.cfg->type0.bar[0]));

	// map regsters
	return mm_dmas_map(mm_dmas_virt2Phys(host->capRegAddr));
}

__always_inline__ int hw_nvme_reset(hw_nvme_Host *host) {
	// write 0 to ctrlCfg[0]
	hw_nvme_write32(host, hw_nvme_Host_ctrlCfg, 0);

	int timeout = 30;
	while (timeout > 0) {
		if (hw_nvme_read32(host, hw_nvme_Host_ctrlStatus) & 1) {
			timeout--;
			timer_mdelay(1);
		} else break;
	}
	if (hw_nvme_read32(host, hw_nvme_Host_ctrlStatus) & 1) {
		printk(RED, BLACK, "hw: nvme: %p: failed to reset.\n", host);
		return res_FAIL;
	}
	printk(GREEN, BLACK, "hw: nvme %p: succ reset.\n", host);
	return res_SUCC;
}

__always_inline__ int hw_nvme_initIntr(hw_nvme_Host *host) {
	host->msi = NULL;
	host->flags &= ~hw_nvme_Host_flags_msix;

	host->intrNum = 4;

	for (hw_pci_CapHdr *hdr = hw_pci_getNxtCap(host->pci.cfg, NULL); hdr != NULL; hdr = hw_pci_getNxtCap(host->pci.cfg, hdr)) {
		switch (hdr->capId) {
			case hw_pci_CapHdr_capId_MSI :
				host->msi = container(container(hdr, hw_pci_MsiCap32, hdr), hw_pci_MsiCap, cap32);
				break;
			case hw_pci_CapHdr_capId_MSIX :
				host->msix = container(hdr, hw_pci_MsixCap, hdr);
				host->flags |= hw_nvme_Host_flags_msix;
				break;
		}
		if (host->flags & hw_nvme_Host_flags_msix) break;
	}
	if (host->msi == NULL) {
		printk(RED, BLACK, "hw: nvme: %p: no MSI/MSI-X support\n", host);
		return res_FAIL;
	}
	
	// allocate interrupt descriptor
	host->intr = mm_kmalloc(sizeof(intr_Desc) * host->intrNum, mm_Attr_Shared, NULL);
	if (host->intr == NULL) {
		printk(RED, BLACK, "hw: nvme: %p failed to allocate interrupt descriptor\n", host);
		return res_FAIL;
	}

	if (host->flags & hw_nvme_Host_flags_msix) {
		printk(WHITE, BLACK, "hw: nvme: %p: use msix\n", host);
		
		int vecNum = hw_pci_MsixCap_vecNum(host->msix);
		vecNum = host->intrNum = min(vecNum, host->intrNum);

		hw_pci_MsixTbl *tbl = hw_pci_getMsixTbl(host->msix, host->pci.cfg);

		printk(WHITE, BLACK, "hw: nvme: %p msixtbl:%p vecNum:%d msgCtrl:%#010x\n", host, tbl, vecNum, host->msix->msgCtrl);

		for (int i = 0; i < host->intrNum; i++)
			hw_pci_initIntr(host->intr + i, hw_nvme_msiHandler, (u64)host | i, "nvme msix");
		if (hw_pci_allocMsix(host->msix, host->pci.cfg, host->intr, host->intrNum) == res_FAIL) {
			printk(RED, BLACK, "hw: nvme: %p failed to allocate msix interrupt\n", host);
			return res_FAIL;
		}
	} else {
		printk(WHITE, BLACK, "hw: nvme: %p: use msi\n", host);

		int vecNum = hw_pci_MsiCap_vecNum(host->msi);
		host->intrNum = vecNum = min(vecNum, host->intrNum);

		printk(WHITE, BLACK, "hw: nvme: %p msi: %s vecNum:%d mgsCtrl:%#010x\n", 
				host, hw_pci_MsiCap_is64(host->msi) ? "64B" : "32B", vecNum, *hw_pci_MsiCap_msgCtrl(host->msi));

		for (int i = 0; i < vecNum; i++)
			hw_pci_initIntr(host->intr + i, hw_nvme_msiHandler, (u64)host | i, "nvme msi");
		if (hw_pci_allocMsi(host->msi, host->intr, host->intrNum) == res_FAIL) {
			printk(RED, BLACK, "hw: nvme: host %p failed to set msi interrupt\n", host);
			return res_FAIL;
		}
	}

	hw_pci_disableIntx(host->pci.cfg);
	printk(GREEN, BLACK, "hw: nvme: %p msi/msix set\n", host);
	return res_SUCC;
}

__always_inline__ int hw_nvme_initAdQue(hw_nvme_Host *host) {
	host->adCmplQue = hw_nvme_allocCmplQue(host, hw_nvme_cmplQueSz);
	host->adSubQue = hw_nvme_allocSubQue(host, hw_nvme_subQueSz, host->adCmplQue);

	if (host->adSubQue == NULL || host->adCmplQue == NULL) {
		printk(RED, BLACK, "hw: nvme: %p: failed to allocate admin queue\n");
		return res_FAIL;
	}

	{
		u32 ada = hw_nvme_subQueSz | (hw_nvme_cmplQueSz << 16);
		hw_nvme_write32(host, hw_nvme_Host_adQueAttr, ada);
	}

	hw_nvme_write64(host, hw_nvme_Host_asQueAddr, mm_dmas_virt2Phys(host->adSubQue->entries));
	hw_nvme_write64(host, hw_nvme_Host_acQueAddr, mm_dmas_virt2Phys(host->adCmplQue->entries));
	
	return res_SUCC;
}

__always_inline__ int hw_nvme_initNsp(hw_nvme_Host *host) {
	u32 *nspLst = mm_kmalloc(sizeof(u32) * 1024, mm_Attr_Shared, NULL);
	hw_nvme_Request *req = hw_nvme_makeReq(1);
	hw_nvme_initReq_identify(req, hw_nvme_Request_Identify_type_NspLst, 0, nspLst);
}

__always_inline__ int _initHost(hw_nvme_Host *host) {
	if (_getRegAddr(host) == res_FAIL) return res_FAIL;

	if (hw_nvme_reset(host) == res_FAIL) return res_FAIL;

	// read doorbell stride
	host->dbStride = 1 << (2 + ((hw_nvme_read64(host, hw_nvme_Host_ctrlCap) >> 32) & 0xf));

	u64 cap = hw_nvme_read64(host, hw_nvme_Host_ctrlCap);
	// set page size to 4K, set correct io command set
	{
		u32 mxPgSz, mnPgSz;
		mnPgSz = (cap >> 48) & 0xf;
		mxPgSz = (cap >> 52) & 0xf;
		if (mnPgSz > 0) {
			printk(RED, BLACK, "hw: nvme %p: no support for 4K page.\n");
			return res_FAIL;
		}
	}
	u32 cfg = 0;
	
	{
		u32 ioCmdSet = 0, ams = 0, ioSubEntrySz, ioCmplEntrySz;
		// noiocss
		if (cap & (1ul << 44)) ioCmdSet = 0b111;
		// iocss
		else if (cap & (1ul << 43)) ioCmdSet = 0b110;
		else if ((~cap & (1ul << 43)) && (cap & (1ul << 37))) ioCmdSet = 0b000;

		ams = (cap >> 17) & 0b11;
		ioSubEntrySz = bit_ffs32(sizeof(hw_nvme_SubQueEntry)) - 1;
		ioCmplEntrySz = bit_ffs32(sizeof(hw_nvme_CmplQueEntry)) - 1;

		cfg = (ioCmplEntrySz << 20) | (ioSubEntrySz << 16) | (ams << 11) | (0 << 7);

		printk(WHITE, BLACK, "hw: nvme %p: ioCmdSet:%x->%1x ams:%1x ioSubEntrySz:%d ioCmplEntrySz:%d cfg:%08x\n",
			host, (cap >> 37) & 0xff, ioCmdSet, ams, ioSubEntrySz, ioCmplEntrySz, cfg);
	}
	
	hw_nvme_write32(host, hw_nvme_Host_ctrlCfg, cfg);

	host->subQueIdenCnt = host->cmplQueIdenCnt = 0;
	host->intrNum = 4;

	printk(WHITE, BLACK, "hw: nvme: %p: capAddr:%p cap:%#018lx version:%x dbStride:%d\n", 
		host, host->capRegAddr, hw_nvme_read64(host, hw_nvme_Host_ctrlCap), hw_nvme_read32(host, hw_nvme_Host_version), host->dbStride);

	if (hw_nvme_initIntr(host) == res_FAIL) return res_FAIL;

	if (hw_nvme_initAdQue(host) == res_FAIL) return res_FAIL;

	// launch nvme and get namespace information
	hw_nvme_write32(host, hw_nvme_Host_ctrlCfg, hw_nvme_read32(host, hw_nvme_Host_ctrlCfg) | 1);
	int timeout = 30;
	while (timeout > 30) {
		if (~hw_nvme_read32(host, hw_nvme_Host_ctrlStatus) & 1) {
			timeout--;
			timer_mdelay(1);
		} else break;
	}
	if (hw_nvme_read32(host, hw_nvme_Host_ctrlStatus) & 1) {
		printk(GREEN, BLACK, "hw: nvme %p: succ to enable, status=%08x\n", host, hw_nvme_read32(host, hw_nvme_Host_ctrlStatus));
	} else {
		printk(RED, BLACK, "hw: nvme: %p: failed to enable, status=%08x\n", host, hw_nvme_read32(host, hw_nvme_Host_ctrlStatus));
		return res_FAIL;
	}

	if (hw_nvme_initNsp(host) == res_FAIL) return res_FAIL;
	
	return res_SUCC;
}

int hw_nvme_cfg(hw_Device *dev) {
	hw_pci_Dev *pciDev = container(dev, hw_pci_Dev, device);

	hw_nvme_Host *host = mm_kmalloc(sizeof(hw_nvme_Host), mm_Attr_Shared, NULL);
	
	hw_pci_extend(pciDev, &host->pci);

	return _initHost(host);
}

void hw_nvme_init() {
	hw_driver_initDriver(&hw_nvme_drv, "hw: nvme", hw_nvme_chk, hw_nvme_cfg, NULL, NULL);

	hw_driver_register(&hw_nvme_drv);
}