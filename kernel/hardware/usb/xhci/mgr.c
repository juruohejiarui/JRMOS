#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>

intr_handlerDeclare(hw_usb_xhci_msiHandler) {
	hw_usb_xhci_Host *host = (void *)(param & ~0xfful);
	u32 intrId = param & 0xff;
	hw_usb_xhci_EveRing *eveRing = host->eveRings[intrId];
	
	// get event trb froma event ring
	hw_usb_xhci_TRB *event;
	hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_status, (1u << 3));
	while (hw_usb_xhci_TRB_getCycBit(event = &eveRing->rings[eveRing->curRingIdx][eveRing->curIdx]) == eveRing->cycBit) {
		if ((++eveRing->curIdx) == eveRing->ringSz) {
			if ((++eveRing->curRingIdx) == eveRing->ringNum)
				eveRing->curRingIdx = 0, eveRing->cycBit ^= 1;
			eveRing->curIdx = 0;
		}
		switch (hw_usb_xhci_TRB_getType(event)) {
			case hw_usb_xhci_TRB_Type_CmdCmpl :
				hw_usb_xhci_Request *req = hw_usb_xhci_response(host->cmdRing, event, *(u64 *)&event->dt1);
				printk(YELLOW, BLACK, "response to command request %#018lx with code:%d\n", req, hw_usb_xhci_TRB_getCmplCode(event));
				break;
			case hw_usb_xhci_TRB_Type_PortStChg :
				hw_usb_xhci_OpReg_write32(host, hw_usb_xhci_Host_opReg_status, (1u << 4));
				hw_usb_xhci_portChange(host, event->dt1 >> 24);
				printk(YELLOW, BLACK, "response to port status change.\n");
				break;
		}
	}
	hw_usb_xhci_IntrReg_write64(host, intrId, hw_usb_xhci_intrReg_DeqPtr, mm_dmas_virt2Phys(event) | (1ul << 3));
}

static __always_inline__ void hw_usb_xhci_portConnect(hw_usb_xhci_Host *host, u32 portIdx) {
	// make new device management for this port
	host->portDev[portIdx] = hw_usb_xhci_newDev(host, NULL, portIdx);
	if (!host->portDev[portIdx]) {
		printk(RED, BLACK, "hw: xhci: host %#018lx failed to create device for port %d\n", host, portIdx);
		return;
	}
}

static __always_inline__ void hw_usb_xhci_portDisconnect(hw_usb_xhci_Host *host, u32 portIdx) {
	hw_usb_xhci_Device *dev = host->portDev[portIdx];
	if (dev == NULL) return ;
	task_sche_wake(dev->mgrTsk);
	task_signal_send(dev->mgrTsk, task_Signal_Int);
}

void hw_usb_xhci_portChange(hw_usb_xhci_Host *host, u32 portIdx) {
	hw_usb_xhci_PortReg_write(host, portIdx, hw_usb_xhci_Host_portReg_sc, hw_usb_xhci_Host_portReg_sc_Power | hw_usb_xhci_Host_portReg_sc_AllChg | hw_usb_xhci_Host_portReg_sc_AllEve);
	u32 portSc = hw_usb_xhci_PortReg_read(host, portIdx, hw_usb_xhci_Host_portReg_sc);
	if (portSc & 1) {
		printk(WHITE, BLACK, "hw: xhci: host %#018lx port %d connect\n", host, portIdx);
		// this port is enabled
		if ((portSc & (1u << 1)) && ((portSc >> 5) & 0xf) == 0) {
			hw_usb_xhci_portConnect(host, portIdx);
		} else hw_usb_xhci_PortReg_write(host, portIdx, hw_usb_xhci_Host_portReg_sc, (1u << 4) | hw_usb_xhci_Host_portReg_sc_Power | hw_usb_xhci_Host_portReg_sc_AllEve);
	} else {
		printk(WHITE, BLACK, "hw: xhci: host %#018lx port %d disconnect\n", host, portIdx);
		hw_usb_xhci_portDisconnect(host, portIdx);
	}
}

void hw_usb_xhci_uninstallDev(hw_usb_xhci_Device *dev) {
	printk(WHITE, BLACK, "hw: xhci: uninstall device %#018lx\n", dev);
	if (dev->flag & hw_usb_xhci_Device_flag_Direct) {
		dev->host->portDev[dev->portId] = NULL;
	}
	for (int i = 0; i < 31; i++) {
		if (dev->epRing[i]) {
			printk(WHITE, BLACK, "hw: xhci: free ring for endpoint %d\n", i);
			hw_usb_xhci_freeRing(dev->host, dev->epRing[i]);
			dev->epRing[i] = NULL;
		}
	}
	hw_usb_xhci_freeDev(dev);
	task_exit(-1);
}

void hw_usb_xhci_devMgrTsk(hw_usb_xhci_Device *dev) {
	printk(WHITE, BLACK, "hw: xhci: device manager task %ld for device %#018lx under host %#018lx\n", task_current->pid, dev, dev->host);
	int res = hw_usb_xhci_devInit(dev);
	if (res == res_FAIL) {
		printk(RED, BLACK, "hw: xhci: device manager task for device %#018lx failed to initialize\n", dev);
		hw_usb_xhci_uninstallDev(dev);
		return;
	}
	printk(GREEN, BLACK, "hw: xhci: device manager task for device %#018lx initialized\n", dev);
	task_signal_setHandler(task_Signal_Int, (void *)hw_usb_xhci_uninstallDev, (u64)dev);
	while (1) hal_hw_hlt();
}