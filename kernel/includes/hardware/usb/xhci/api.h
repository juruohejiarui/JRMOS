#ifndef __HARDWARE_USB_XHCI_API_H__
#define __HARDWARE_USB_XHCI_API_H__

#include <hal/hardware/reg.h>
#include <hardware/usb/xhci/desc.h>

extern SafeList hw_usb_xhci_devLst;

static __always_inline__ u32 hw_usb_xhci_TRB_getType(hw_usb_xhci_TRB *trb) {
	return (hal_read32((u64)&trb->ctrl) >> 10) & ((1 << 6) - 1);
}
static __always_inline__ void hw_usb_xhci_TRB_setType(hw_usb_xhci_TRB *trb, u32 type) {
	hal_write32((u64)&trb->ctrl, (hal_read32((u64)&trb->ctrl) & (0xffff03ffu)) | (type << 10));
}
static __always_inline__ void hw_usb_xhci_TRB_setSlotType(hw_usb_xhci_TRB *trb, u32 type) {
	hal_write32((u64)&trb->ctrl, (hal_read32((u64)&trb->ctrl) & (0xfff0ffffu)) | (type << 16));
}
static __always_inline__ u32 hw_usb_xhci_TRB_getCmplCode(hw_usb_xhci_TRB *trb) {
	return (hal_read32((u64)&trb->status) >> 24) & 0xff;
}
static __always_inline__ void hw_usb_xhci_TRB_setCycBit(hw_usb_xhci_TRB *trb, u32 val) {
	hal_write32((u64)&trb->ctrl, (hal_read32((u64)&trb->ctrl) & ~0x1u) | val);
}
static __always_inline__ u32 hw_usb_xhci_TRB_getCycBit(hw_usb_xhci_TRB *trb) {
	return hal_read32((u64)&trb->ctrl) & 1;
}
static __always_inline__ void hw_usb_xhci_TRB_setToggle(hw_usb_xhci_TRB *trb, u32 val) {
	hal_write32((u64)&trb->ctrl, (hal_read32((u64)&trb->ctrl) & ~0x2u) | (val << 1));
}
static __always_inline__ u32 hw_usb_xhci_TRB_getToggle(hw_usb_xhci_TRB *trb) {
	return (hal_read32((u64)&trb->ctrl) >> 1) & 1;
}
static __always_inline__ void hw_usb_xhci_TRB_setCtrlBit(hw_usb_xhci_TRB *trb, u32 val) {
	hal_write32((u64)&trb->ctrl, val | (hal_read32((u64)&trb->ctrl) & ~hw_usb_xhci_TRB_ctrl_allBit));
}
static __always_inline__ u32 hw_usb_xhci_TRB_getCtrlBit(hw_usb_xhci_TRB *trb) {
	return hal_read32((u64)&trb->ctrl) & hw_usb_xhci_TRB_ctrl_allBit;
}
static __always_inline__ void hw_usb_xhci_TRB_setTRT(hw_usb_xhci_TRB *trb, u32 val) {
	hal_write32((u64)&trb->ctrl, (val << 16) | (hal_read32((u64)&trb->ctrl) & 0xfffcffffu));
}
static __always_inline__ void hw_usb_xhci_TRB_setDir(hw_usb_xhci_TRB *trb, u32 val) {
	hw_usb_xhci_TRB_setTRT(trb, val & 0x1);
}
static __always_inline__ u64 hw_usb_xhci_TRB_getData(hw_usb_xhci_TRB *trb) {
	return hal_read64((u64)&trb->dt1);
}
static __always_inline__ void hw_usb_xhci_TRB_setData(hw_usb_xhci_TRB *trb, u64 data) {
	hal_write64((u64)&trb->dt1, data);
}
static __always_inline__ u32 hw_usb_xhci_TRB_getIntrTarget(hw_usb_xhci_TRB *trb) {
	// interrupt target and completion code are in the same field
	return hw_usb_xhci_TRB_getCmplCode(trb);
}
static __always_inline__ void hw_usb_xhci_TRB_setIntrTarget(hw_usb_xhci_TRB *trb, u32 val) {
	hal_write32((u64)&trb->status, (hal_read32((u64)&trb->status) & 0xffffffu) | (val << 24));
}
static __always_inline__ u64 hw_usb_xhci_TRB_setStatus(hw_usb_xhci_TRB *trb, u32 val) {
	hal_write32((u64)&trb->status, val);
}
static __always_inline__ u64 hw_usb_xhci_TRB_mkSetup(u8 bmReqT, u8 bReq, u16 wVal, u16 wIdx, u16 wLen) {
	return (u64)bmReqT | (((u64)bReq) << 8) | (((u64)wVal) << 16) | (((u64)wIdx) << 32) | (((u64)wLen) << 48);
}
static __always_inline__ u32 hw_usb_xhci_TRB_mkStatus(u32 trbTransLen, u32 tdSize, u32 intrTarget) {
	return trbTransLen | (tdSize << 17) | (intrTarget << 22);
}
static __always_inline__ u32 hw_usb_xhci_TRB_getSlot(hw_usb_xhci_TRB *trb) {
	return hal_read32((u64)&trb->ctrl) >> 24;
}
static __always_inline__ u32 hw_usb_xhci_TRB_setSlot(hw_usb_xhci_TRB *trb, u32 val) {
	hal_write32((u64)&trb->ctrl, (hal_read32((u64)&trb->ctrl) & 0xffffffu) | (val << 24));
}
static __always_inline__ void hw_usb_xhci_TRB_setBSR(hw_usb_xhci_TRB *trb, u32 val) {
	hal_write32((u64)&trb->ctrl, (hal_read32((u64)&trb->ctrl) & ~(1u << 9)) | (val << 9));
}
static __always_inline__ int hw_usb_xhci_TRB_getPos(hw_usb_xhci_TRB *trb) {
	return ((u64)trb - ((u64)trb & ~0xfffful)) / sizeof(hw_usb_xhci_TRB);
}

static __always_inline__ void hw_usb_xhci_TRB_copy(hw_usb_xhci_TRB *src, hw_usb_xhci_TRB *dst) {
	hal_write64((u64)dst, *(u64 *)src);
	hal_write64((u64)dst + sizeof(u64), *(u64 *)((u64)src + sizeof(u64)));
}

static __always_inline__ u8 hw_usb_xhci_CapReg_capLen(hw_usb_xhci_Host *host) { return hal_read8(host->capRegAddr + hw_usb_xhci_Host_capReg_capLen); }

static __always_inline__ u16 hw_usb_xhci_CapReg_hciVer(hw_usb_xhci_Host *host) { return hal_read16(host->capRegAddr + hw_usb_xhci_Host_capReg_hciVer); }

static __always_inline__ u32 hw_usb_xhci_CapReg_rtsOff(hw_usb_xhci_Host *host) { return hal_read32(host->capRegAddr + hw_usb_xhci_Host_capReg_rtsOff) & ~0x1fu; }

static __always_inline__ u32 hw_usb_xhci_CapReg_dbOff(hw_usb_xhci_Host *host) { return hal_read32(host->capRegAddr + hw_usb_xhci_Host_capReg_dbOff) & ~0x03u; }

static __always_inline__ void hw_usb_xhci_OpReg_write32(hw_usb_xhci_Host *host, u32 offset, u32 val) {
	hal_write32(host->opRegAddr + offset, val);
}
static __always_inline__ void hw_usb_xhci_OpReg_write64(hw_usb_xhci_Host *host, u32 offset, u64 val) {
	hal_write64(host->opRegAddr + offset, val); 
}

static __always_inline__ u32 hw_usb_xhci_OpReg_read32(hw_usb_xhci_Host *host, u32 offset) {
	return hal_read32(host->opRegAddr + offset);
}

static __always_inline__ void hw_usb_xhci_PortReg_write(hw_usb_xhci_Host *host, u32 portIdx, u32 offset, u32 val) {
	hal_write32(host->opRegAddr + 0x400 + (portIdx - 1) * 0x10 + offset, val);
}
static __always_inline__ u32 hw_usb_xhci_PortReg_read(hw_usb_xhci_Host *host, u32 portIdx, u32 offset) {
	return hal_read32(host->opRegAddr + 0x400 + (portIdx - 1) * 0x10 + offset);
}

static __always_inline__ void hw_usb_xhci_IntrReg_write64(hw_usb_xhci_Host *host, u32 intrId, u32 offset, u64 val) {
	hal_write64(host->rtRegAddr + 0x20 + intrId * 0x20 + offset, val);
}
static __always_inline__ void hw_usb_xhci_IntrReg_write32(hw_usb_xhci_Host *host, u32 intrId, u32 offset, u32 val) {
	hal_write32(host->rtRegAddr + 0x20 + intrId * 0x20 + offset, val);
}
static __always_inline__ u64 hw_usb_xhci_IntrReg_read64(hw_usb_xhci_Host *host, u32 intrId, u32 offset) {
	return hal_read64(host->rtRegAddr + 0x20 + intrId * 0x20 + offset);
}
static __always_inline__ u32 hw_usb_xhci_IntrReg_read32(hw_usb_xhci_Host *host, u32 intrId, u32 offset) {
	return hal_read32(host->rtRegAddr + 0x20 + intrId * 0x20 + offset);
}

static __always_inline__ u32 hw_usb_xhci_DbReq_make(u32 epId, u32 tskId) { return epId | (tskId << 16); }

static __always_inline__ void hw_usb_xhci_DbReg_write(hw_usb_xhci_Host *host, u32 slotId, u32 value) {
	hal_write32(host->dbRegAddr + slotId * 0x4, value);
}

#define hw_usb_xhci_CapReg_hcsParam(host, id) \
	hal_read32((host)->capRegAddr + hw_usb_xhci_Host_capReg_hcsParam##id)
#define hw_usb_xhci_CapReg_hccParam(host, id) \
	hal_read32((host)->capRegAddr + hw_usb_xhci_Host_capReg_hccParam##id)

static __always_inline__ u32 hw_usb_xhci_mxSlot(hw_usb_xhci_Host *host) { return hw_usb_xhci_CapReg_hcsParam(host, 1) & ((1u << 8) - 1); }

static __always_inline__ u32 hw_usb_xhci_mxIntr(hw_usb_xhci_Host *host) { return (hw_usb_xhci_CapReg_hcsParam(host, 1) >> 8) & ((1u << 11) - 1); }

static __always_inline__ u32 hw_usb_xhci_mxPort(hw_usb_xhci_Host *host) { return (hw_usb_xhci_CapReg_hcsParam(host, 1) >> 24) & ((1u << 8) - 1); }

// maximum value of event ring segment table size
static __always_inline__ u32 hw_usb_xhci_mxERST(hw_usb_xhci_Host *host) { return (1u << ((hw_usb_xhci_CapReg_hcsParam(host, 2) >> 4) & 0xf)); }

static __always_inline__ u32 hw_usb_xhci_mxScrSz(hw_usb_xhci_Host *host) {
	register u32 vl = hw_usb_xhci_CapReg_hcsParam(host, 2);
	return ((vl >> 16) & (((1ul << 5) - 1) << 5)) | ((vl >> 27) & ((1ul << 5) - 1));
}

void *hw_usb_xhci_nxtExtCap(hw_usb_xhci_Host *host, void *cur);
static __always_inline__ u32 hw_usb_xhci_xhci_getExtCapId(void *cur) { return hal_read8((u64)cur);}

static __always_inline__ u8 hw_usb_xhci_Ext_Protocol_PortOff(void *extCap) { return hal_read8((u64)extCap + 8); }
static __always_inline__ u8 hw_usb_xhci_Ext_Protocol_PortCnt(void *extCap) { return hal_read8((u64)extCap + 9); }

static __always_inline__ u8 hw_usb_xhci_Ext_Protocol_contain(void *extCap, u32 port) {
	register u32 st = hw_usb_xhci_Ext_Protocol_PortOff(extCap), 
		cnt = hw_usb_xhci_Ext_Protocol_PortCnt(extCap);
	return (port >= st && port < st + cnt);
}


static __always_inline__ u8 hw_usb_xhci_Ext_Protocol_slotType(void *extCap) { return hal_read8((u64)extCap + 12); }

u8 hw_usb_xhci_getSlotType(hw_usb_xhci_Host *host, u32 portIdx);

hw_usb_xhci_InCtx *hw_usb_xhci_allocInCtx(hw_usb_xhci_Host *host);

static __always_inline__ void *hw_usb_xhci_getCtxEntry(hw_usb_xhci_Host *host, void *ctx, u32 ctxId) {
	return (void *)((u64)ctx + (ctxId << ((host->flag & hw_usb_xhci_Host_flag_Ctx64) ? 6 : 5)));
}

static __always_inline__ void hw_usb_xhci_writeCtx(void *ctx, int dwIdx, u32 mask, u32 val) {
	register u64 addr = (u64)ctx + dwIdx * sizeof(u32);
	hal_write32(addr, (hal_read32(addr) & ~mask) | ((val << (bit_ffs32(mask) - 1) & mask)));
}
static __always_inline__ void hw_usb_xhci_writeCtx64(void *ctx, int dwIdx, u64 val) {
	register u64 addr = (u64)ctx + dwIdx * 8;
	hal_write64(addr, val);
}

hw_usb_xhci_Ring *hw_usb_xhci_allocRing(hw_usb_xhci_Host *host, u32 size);

int hw_usb_xhci_freeRing(hw_usb_xhci_Host *host, hw_usb_xhci_Ring *ring);

void hw_usb_xhci_InsReq(hw_usb_xhci_Host *host, hw_usb_xhci_Ring *ring, hw_usb_xhci_Request *req);

void hw_usb_xhci_request(hw_usb_xhci_Host *host, hw_usb_xhci_Ring *ring, hw_usb_xhci_Request *req, u32 slot, u32 doorbell);

hw_usb_xhci_Request *hw_usb_xhci_response(hw_usb_xhci_Ring *ring, hw_usb_xhci_TRB *result, u64 trbAddr);

hw_usb_xhci_Request *hw_usb_xhci_makeRequest(u32 size, u32 flags);

int hw_usb_xhci_freeRequest(hw_usb_xhci_Request *req);

hw_usb_xhci_EveRing *hw_usb_xhci_allocEveRing(hw_usb_xhci_Host *host, u16 ringNum, u32 ringSz);

int hw_usb_xhci_reset(hw_usb_xhci_Host *host);

int hw_usb_xhci_waitReady(hw_usb_xhci_Host *host);

hw_usb_xhci_Device *hw_usb_xhci_newDev(hw_usb_xhci_Host *host, hw_usb_xhci_Device *parent, u32 portIdx);

int hw_usb_xhci_freeDev(hw_usb_xhci_Device *dev);

intr_handlerDeclare(hw_usb_xhci_msiHandler);

void hw_usb_xhci_portChange(hw_usb_xhci_Host *host, u32 portIdx);

void hw_usb_xhci_devMgrTsk(hw_usb_xhci_Device *dev);

int hw_usb_xhci_init();

int hw_usb_xhci_devInit(hw_usb_xhci_Device *dev);

#endif