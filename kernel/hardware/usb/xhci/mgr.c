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
				printk(YELLOW, BLACK, "response to port status change.\n");
				break;
		}
	}
	hw_usb_xhci_IntrReg_write64(host, intrId, hw_usb_xhci_intrReg_DeqPtr, mm_dmas_virt2Phys(event) | (1ul << 3));
}