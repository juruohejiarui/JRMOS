#ifndef __HADEWARE_NVME_H__
#define __HARDWARE_NVME_H__

#include <hardware/pci.h>
#include <hardware/mgr.h>

#define hw_nvme_Host_ctrlCap	0x0
#define hw_nvme_Host_version	0x8
#define hw_nvme_Host_intrMskSet	0xc
#define hw_nvme_Host_intrMskClr	0x10
#define hw_nvme_Host_ctrlCfg	0x14
#define hw_nvme_Host_ctrlStatus	0x1c
// subsystem reset
#define hw_nvme_Host_subsReset	0x20
#define hw_nvme_Host_asQueAttr	0x24
#define hw_nvme_Host_asQueAddr	0x28
#define hw_nvme_Host_acQueAddr	0x30
#define hw_nvme_Host_memBufLoc	0x38
#define hw_nvme_Host_memBufSz	0x3c

// max number of io submission queue for each disk
#define hw_nvme_mxIoSubQueNum	0x2

typedef struct hw_nvme_SubQueEntry {
	// opcode
	u8 opc;
	u8 attr;
	u16 cmdIden;

	// namesapce identifier (NSID)
	u32 nspIden;

	u32 dw2;
	u32 dw3;

	// metadata pointer(MPTR)
	union {
		u32 metaPtr32[2];
		u64 metaPtr;
	};

	// data pointer(DPTR)
	union {
		u32 dtPtr32[4];
		u64 prp[2];
		struct {

		} sgl;
	};
	
	u32 spec[6];
} __attribute__ ((packed)) hw_nvme_SubQueEntry;

typedef struct hw_nvme_cmplQueEntry {
	u32 spec[2];
	u16 subQueHdrPtr;
	u16 subQueIden;
	u16 cmdIden;
	u16 status;
} __attribute__ ((packed)) hw_nvme_cmplQueEntry;

typedef struct hw_nvme_SubQue {
	u32 size;
	u32 head, tail;
	
	hw_nvme_SubQueEntry *entries;
} hw_nvme_SubQue;

typedef struct hw_nvme_Host {
	hw_pci_Dev pci;
	
	u32 dbStride;
	u64 capRegAddr;

	u16 mxQueSz;
	
} hw_nvme_Host;

__always_inline__ u32 hw_nvme_read32(hw_nvme_Host *host, u32 off) { return hal_read32(host->capRegAddr + off); }

__always_inline__ u64 hw_nvme_read64(hw_nvme_Host *host, u32 off) { return hal_read64(host->capRegAddr + off); }

__always_inline__ void hw_nvme_write32(hw_nvme_Host *host, u32 off, u32 val) { hal_write32(host->capRegAddr + off, val); }

__always_inline__ void hw_nvme_write64(hw_nvme_Host *host, u32 off, u64 val) { hal_write64(host->capRegAddr + off, val); }

void hw_nvme_init();
#endif