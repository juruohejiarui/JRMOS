#ifndef __HADEWARE_NVME_H__
#define __HARDWARE_NVME_H__

#include <hardware/pci.h>
#include <hardware/mgr.h>
#include <task/request.h>

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

typedef struct hw_nvme_CmplQueEntry {
	u32 spec[2];
	u16 subQueHdrPtr;
	u16 subQueIden;
	u16 cmdIden;
	u16 status;
} __attribute__ ((packed)) hw_nvme_CmplQueEntry;

#define hw_nvme_subQueSz	(Page_4KSize / sizeof(hw_nvme_SubQueEntry))
#define hw_nvme_cmplQueSz	(Page_4KSize / sizeof(hw_nvme_CmplQueEntry))

typedef struct hw_nvme_Request {
	task_Request req;
	int inputSz;
	hw_nvme_CmplQueEntry res;
	hw_nvme_SubQueEntry input[0];
} hw_nvme_Request;

typedef struct hw_nvme_SubQue {
	u32 size, load;
	u32 head, tail;

	u32 iden;

	SpinLock lck;
	
	hw_nvme_SubQueEntry *entries;
	hw_nvme_Request req[0];
} hw_nvme_SubQue;

typedef struct hw_nvme_CmplQue {
	u32 size, pos;

	hw_nvme_CmplQueEntry *entries;
} hw_nvme_CmplQue;

typedef struct hw_nvme_Host {
	hw_pci_Dev pci;
	
	u32 dbStride;

	u16 mxQueSz;

	u16 intrNum;

	u64 capRegAddr;

	union {
		hw_pci_MsiCap *msi;
		hw_pci_MsixCap *msix;
	};

	intr_Desc *intr;

	u64 flags;
#define hw_nvme_Host_flags_msix	0x1

	hw_nvme_SubQue *adSubQue;
	hw_nvme_CmplQue *adCmplQue;
	
} hw_nvme_Host;

__always_inline__ u32 hw_nvme_read32(hw_nvme_Host *host, u32 off) { return hal_read32(host->capRegAddr + off); }

__always_inline__ u64 hw_nvme_read64(hw_nvme_Host *host, u32 off) { return hal_read64(host->capRegAddr + off); }

__always_inline__ void hw_nvme_write32(hw_nvme_Host *host, u32 off, u32 val) { hal_write32(host->capRegAddr + off, val); }

__always_inline__ void hw_nvme_write64(hw_nvme_Host *host, u32 off, u64 val) { hal_write64(host->capRegAddr + off, val); }

__always_inline__ int hw_nvme_CmplQue_phaseTag(hw_nvme_CmplQueEntry *entry) { return hal_read16((u64)&entry->status) & 1; }

hw_nvme_SubQue *hw_nvme_allocSubQue(hw_nvme_Host *host, u32 queSz);

hw_nvme_CmplQue *hw_nvme_allocCmplQue(hw_nvme_Host *host, u32 queSz);


void hw_nvme_init();
#endif