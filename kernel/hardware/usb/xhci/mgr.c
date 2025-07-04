#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>

intr_handlerDeclare(hw_usb_xhci_msiHandler) {
	hw_usb_xhci_Host *host = (void *)(param & ~0xfful);
	u32 intrId = param & 0xff;
	hw_usb_xhci_EveRing *eveRing = host->eveRings[intrId];

	// printk(screen_log, "hw: xhci: host %p handle MSI interrupt %d\n", host, intrId);

	// get event trb froma event ring
	hw_usb_xhci_TRB *event;
	hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_status, (1u << 3));
	while (hw_usb_xhci_TRB_getCycBit(event = &eveRing->rings[eveRing->curRingIdx][eveRing->curIdx]) == eveRing->cycBit) {
		if ((++eveRing->curIdx) == eveRing->ringSz) {
			if ((++eveRing->curRingIdx) == eveRing->ringNum)
				eveRing->curRingIdx = 0, eveRing->cycBit ^= 1;
			eveRing->curIdx = 0;
		}
		switch (hw_usb_xhci_TRB_getTp(event)) {
			case hw_usb_xhci_TRB_Tp_CmdCmpl : {
				hw_usb_xhci_Request *req = hw_usb_xhci_response(host->cmdRing, event, *(u64 *)&event->dt1);
				// printk(screen_warn, "response to command request %p with code:%d\n", req, hw_usb_xhci_TRB_getCmplCode(event));
				break;
			}
			case hw_usb_xhci_TRB_Tp_PortStChg :
				hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_status, (1u << 4));
				hw_usb_xhci_portChange(host, event->dt1 >> 24);
				// printk(screen_warn, "response to port status change.\n");
				break;
			case hw_usb_xhci_TRB_Tp_TransEve : {
				hw_usb_xhci_Device *dev = host->dev[hw_usb_xhci_TRB_getSlot(event)];
				hw_usb_xhci_Ring *epRing = dev->epRing[hw_usb_xhci_TRB_getEp(event)];
				hw_usb_xhci_Request *req = hw_usb_xhci_response(epRing, event, *(u64 *)&event->dt1);
				hw_usb_xhci_TRB_copy(event, &req->res);
				req->flags |= hw_usb_xhci_Request_flags_Finished;
				break;
			}
		}
	}
	hw_usb_xhci_IntrReg_write64(host, intrId, hw_usb_xhci_intrReg_DeqPtr, mm_dmas_virt2Phys(event) | (1ul << 3));
}

__always_inline__ void hw_usb_xhci_portConnect(hw_usb_xhci_Host *host, u32 portIdx) {
	// make new device management for this port
	host->portDev[portIdx] = hw_usb_xhci_newDev(host, NULL, portIdx);
	if (!host->portDev[portIdx]) {
		printk(screen_err, "hw: xhci: host %p failed to create device for port %d\n", host, portIdx);
		return;
	}
	printk(screen_log, "finish portConnect\n");
}

__always_inline__ void hw_usb_xhci_portDisconnect(hw_usb_xhci_Host *host, u32 portIdx) {
	hw_usb_xhci_Device *dev = host->portDev[portIdx];
	if (dev == NULL) return ;
	// in interrupt program, we can not directly send interrupt
	// thus, use the interrupt signal
	task_signal_sendFromIntr(dev->mgrTsk, task_Signal_Int);
}

void hw_usb_xhci_portChange(hw_usb_xhci_Host *host, u32 portIdx) {
	hw_usb_xhci_PortReg_write(host, portIdx, hw_usb_xhci_Host_portReg_sc, hw_usb_xhci_Host_portReg_sc_Power | hw_usb_xhci_Host_portReg_sc_AllChg | hw_usb_xhci_Host_portReg_sc_AllEve);
	u32 portSc = hw_usb_xhci_PortReg_read(host, portIdx, hw_usb_xhci_Host_portReg_sc);
	if (portSc & 1) {
		printk(screen_log, "hw: xhci: host %p port %d connect\n", host, portIdx);
		// this port is enabled
		if ((portSc & (1u << 1)) && ((portSc >> 5) & 0xf) == 0) {
			hw_usb_xhci_portConnect(host, portIdx);
		} else hw_usb_xhci_PortReg_write(host, portIdx, hw_usb_xhci_Host_portReg_sc, (1u << 4) | hw_usb_xhci_Host_portReg_sc_Power | hw_usb_xhci_Host_portReg_sc_AllEve);
	} else {
		printk(screen_log, "hw: xhci: host %p port %d disconnect\n", host, portIdx);
		hw_usb_xhci_portDisconnect(host, portIdx);
	}
}

void hw_usb_xhci_uninstallDev(hw_usb_xhci_Device *dev) {
	task_Request_giveUp();
	if (dev->device.drv != NULL) {
		if (dev->device.drv->uninstall(&dev->device) == res_FAIL)
			printk(screen_err, "hw: xhci: failed to uninstall device %p\n", dev);
	}
	if (dev->flag & hw_usb_xhci_Device_flag_Direct) {
		dev->host->portDev[dev->portId] = NULL;
	}
	hw_usb_xhci_Request *req = hw_usb_xhci_makeRequest(1, hw_usb_xhci_Request_flags_Command | hw_usb_xhci_Request_flags_Abort);
	
	// disable endpoint context (except ctrl endpoint)
	if (dev->ctxFlag) {
		printk(screen_log, "hw: xhci: disable bitmap %x\n", dev->ctxFlag);
		hw_usb_xhci_InCtx *inCtx = dev->inCtx;
		void *ctrlCtx = hw_usb_xhci_getCtxEntry(dev->host, inCtx, hw_usb_xhci_InCtx_Ctrl);
		// drop flag
		hw_usb_xhci_writeCtx(ctrlCtx, 0, ~0x0, dev->ctxFlag & ~0b11u);
		// add flag
		hw_usb_xhci_writeCtx(ctrlCtx, 1, ~0x0, 0);

		hw_usb_xhci_TRB_setTp(&req->input[0], hw_usb_xhci_TRB_Tp_EvalCtx);
		hw_usb_xhci_TRB_setSlot(&req->input[0], dev->slotId);
		hw_usb_xhci_request(dev->host, dev->host->cmdRing, req, NULL, 0);
		if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ)
			printk(screen_err, "hw: xhci: device %p failed to disable endpoint(s).\n", dev);
		else printk(screen_succ, "hw: xhci: device %p disable endpoint(s)", dev);
	}

	for (int i = 0; i < 31; i++) {
		if (dev->epRing[i]) {
			printk(screen_log, "hw: xhci: free ring for endpoint %d\n", i);
			hw_usb_xhci_freeRing(dev->host, dev->epRing[i]);
			dev->epRing[i] = NULL;
		}
	}
	
	if (dev->slotId) {
		hw_usb_xhci_TRB_setTp(&req->input[0], hw_usb_xhci_TRB_Tp_DisblSlot);
		hw_usb_xhci_TRB_setSlot(&req->input[0], dev->slotId);
		hw_usb_xhci_request(dev->host, dev->host->cmdRing, req, NULL, 0);
		if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ)
			printk(screen_err, "hw: xhci: device %p failed to disable slot\n", dev);
		else printk(screen_succ, "hw: xhci: device %p disabled slot:%d\n", dev, dev->slotId);
	}
	hw_usb_xhci_freeRequest(req);
	hw_usb_xhci_freeDev(dev);
	printk(screen_succ, "hw: xhci: device %p uninstalled\n", dev);
	task_exit(-1);
}

void hw_usb_xhci_devMgrTsk(hw_usb_xhci_Device *dev) {
	printk(screen_log, "hw: xhci: device manager task %ld for device %p under host %p\n", task_current->pid, dev, dev->host);
	if (hw_usb_xhci_devInit(dev) == res_FAIL) {
		printk(screen_err, "hw: xhci: device manager task for device %p failed to initialize\n", dev);
		hw_usb_xhci_uninstallDev(dev);
		return;
	}
	task_signal_setHandler(task_Signal_Int, (void *)hw_usb_xhci_uninstallDev, (u64)dev);
	printk(screen_succ, "hw: xhci: device manager task for device %p initialized\n", dev);

	// search for drivers
	SafeList_enum(&hw_drvLst, drvNd) {
		hw_Driver *drv = container(drvNd, hw_Driver, lst);
		if (drv->check && drv->check(&dev->device) == res_SUCC) {
			printk(screen_log, "hw: xhci: device %p found driver %s\n", dev, drv->name);
			dev->device.drv = drv;
			SafeList_exitEnum(&hw_drvLst);
		}
	}
	if (dev->device.drv != NULL) {
		if (dev->device.drv->cfg && dev->device.drv->cfg(&dev->device) == res_SUCC)
			dev->device.drv->install(&dev->device);
	}
	
	while (1) hal_hw_hlt();
}