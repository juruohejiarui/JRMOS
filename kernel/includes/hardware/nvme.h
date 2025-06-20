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
#define hw_nvme_Host_adQueAttr	0x24
#define hw_nvme_Host_asQueAddr	0x28
#define hw_nvme_Host_acQueAddr	0x30
#define hw_nvme_Host_memBufLoc	0x38
#define hw_nvme_Host_memBufSz	0x3c

// max number of io submission queue for each disk
#define hw_nvme_mxIoSubQueNum	0x2

typedef struct hw_nvme_SubQueEntry {
	// dword 0
	// opcode
	u8 opc;
	u8 attr;
	u16 cmdIden;

	// dword 1
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
	
	// dwords 10 ~ dwords 15
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

typedef struct hw_nvme_CmplQue {
	u32 size, pos;
	u32 phase, iden;

	hw_nvme_CmplQueEntry *entries;
} hw_nvme_CmplQue;

typedef struct hw_nvme_SubQue {
	u32 size, load;
	// since that controller can not guarantee the process order of command in ring,
	// we need to update the load by checking whether the head of queue has been handled.
	u32 tail, head;

	u32 iden;

	SpinLock lck;
	
	hw_nvme_SubQueEntry *entries;
	hw_nvme_CmplQue *trgQue;

	hw_nvme_Request *req[0];
} hw_nvme_SubQue;

typedef struct hw_nvme_Nsp {
	int nspId;
	hw_nvme_SubQue *subQue[hw_nvme_mxIoSubQueNum];
	hw_nvme_CmplQue *cmplQue;
} hw_nvme_Nsp;

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

	u32 subQueIdenCnt;
	u32 cmplQueIdenCnt;
#define hw_nvme_Host_flags_msix	0x1

	hw_nvme_SubQue *adSubQue;
	hw_nvme_CmplQue *adCmplQue;

	u32 nspNum;

	hw_nvme_Nsp *nsp;
	
} hw_nvme_Host;

__always_inline__ u32 hw_nvme_read32(hw_nvme_Host *host, u32 off) { return hal_read32(host->capRegAddr + off); }

__always_inline__ u64 hw_nvme_read64(hw_nvme_Host *host, u32 off) { return hal_read64(host->capRegAddr + off); }

__always_inline__ void hw_nvme_write32(hw_nvme_Host *host, u32 off, u32 val) { hal_write32(host->capRegAddr + off, val); }

__always_inline__ void hw_nvme_write64(hw_nvme_Host *host, u32 off, u64 val) { hal_write64(host->capRegAddr + off, val); }

__always_inline__ int hw_nvme_CmplQue_phaseTag(hw_nvme_CmplQueEntry *entry) { return hal_read16((u64)&entry->status) & 1; }

__always_inline__ void hw_nvme_writeSubDb(hw_nvme_Host *host, hw_nvme_SubQue *subQue) {
	hw_nvme_write32(host, 0x1000 + (subQue->iden << 1) * host->dbStride, subQue->tail);
}
__always_inline__ void hw_nvme_writeCmplDb(hw_nvme_Host *host, hw_nvme_CmplQue *cmplQue) {
	hw_nvme_write32(host, 0x1000 + (cmplQue->iden << 1 | 1) * host->dbStride, cmplQue->pos);
}

__always_inline__ int hw_nvme_getSubQueIden(int nspId, int queIdx) { return (nspId << 2) | queIdx; }

hw_nvme_Request *hw_nvme_makeReq(int inputSz);

int hw_nvme_freeReq(hw_nvme_Request *req);

int hw_nvme_request(hw_nvme_Host *host, hw_nvme_SubQue *subQue, hw_nvme_Request *req);

#define hw_nvme_Request_Identify_type_Nsp		0x0
#define hw_nvme_Request_Identify_type_Ctrl		0x1
#define hw_nvme_Request_Identify_type_NspLst	0x2
int hw_nvme_initReq_identify(hw_nvme_Request *req, u32 type, u32 nspIden, void *buf);

hw_nvme_SubQue *hw_nvme_getSubQue(hw_nvme_Host *host, int nspId, int queIdx);

hw_nvme_SubQue *hw_nvme_allocSubQue(hw_nvme_Host *host, u32 queSz, hw_nvme_CmplQue *trgQue);

hw_nvme_CmplQue *hw_nvme_allocCmplQue(hw_nvme_Host *host, u32 queSz);

void hw_nvme_init();
#endif