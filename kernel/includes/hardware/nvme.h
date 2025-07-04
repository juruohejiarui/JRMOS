#ifndef __HADEWARE_NVME_H__
#define __HARDWARE_NVME_H__

#include <hardware/pci.h>
#include <hardware/diskdev.h>
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

	// dword 4 ~ 5
	// metadata pointer(MPTR)
	union {
		u32 metaPtr32[2];
		u64 metaPtr;
	};

	// dword 6 ~ 7
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
	u16 size, pos;
	u16 phase, iden;

	hw_nvme_CmplQueEntry *entries;
} hw_nvme_CmplQue;

typedef struct hw_nvme_SubQue {
	u16 size, load;
	// since that controller can not guarantee the process order of command in ring,
	// we need to update the load by checking whether the head of queue has been handled.
	u16 tail, head;

	u16 iden;

	SpinLock lck;
	
	hw_nvme_SubQueEntry *entries;
	hw_nvme_CmplQue *trg;

	hw_nvme_Request *req[0];
} hw_nvme_SubQue;

typedef struct hw_nvme_Nsp {
	u64 nspSz;
	u64 nspCap;
	u64 nspUtil;
	u8 nspFeature;
	u8 numOfLbaFormat;
	// formatted lba size
	u8 formattedLbaSz;
	// metadata capabilities
	u8 metaCap;
	// end-to-end data protection capabilites
	u8 dtProtectCap;
	// end-to-end data protection tp settings
	u8 dtProtectTpSet;

	// unecessary properties
	// namespace multi-path i/o and namespace sharing capabilities
	u8 nmic;
	// reservation capability
	u8 rescap;
	// format progress indicator
	u8 fpi;
	// dealocate logical block features
	u8 dlfeat;
	// namespace atomic write unit normal
	u16 nawun;
	// namespace atomic write unit power fail
	u16 nawupf;
	// namespace atomic compare & write unit
	u16 nacwu;
	// namespace atomic boundary size normal
	u16 nabsn;
	// namespace atomic boundary offset
	u16 nabo;
	// namespace atomic boundary size power fail
	u16 nabspf;
	// namespace optimal i/o boundary
	u16 noiob;
	
	// NVM capacity
	u64 nvmcap[2];

	// namespace preferred write granularity
	u16 npwg;
	// namespace preferred write alignment
	u16 npwa;
	// namespace preferred dealocate granularity
	u16 npdg;
	// namespace preferred deallocate alignment
	u16 npda;
	// namespace optimal write size
	u16 nows;
	// maximum single source range length (used for Copy command)
	u16 mssrl;
	// maximum copy length
	u32 mcl;
	// maximum source range count
	u8 msrc;
	// key per i/o status
	u8 kpios;
	// number of unique attribute lba formats
	u8 nulbaf;
	
	u8 reserved;
	// key pre i/o data access alignment and granularity
	u32 kpiodaag;
	
	u32 reserved2;
	// anagroup identifier
	u32 anagrpid;
	
	u8 reserved3[3];
	
	// namespace attributes
	u8 nsattr;
	// NVM set identifier
	u16 nvmsetid;
	// endurance group identifier
	u16 endgid;
	// namespace globally unique identifier
	u64 nguid[2];
	// ieee extended unique identifier
	u64 eui64;
	// support lba format, this structure use bit field to reduce function (doge)
	struct {
		// metadata size 
		u16 metaSz;
		// lba data size
		u8 lbaDtSz;
		// relative performance
		u8 rp : 2;
		u32 reserved : 6;
	} __attribute__ ((packed)) lbaFormat[64];
	u8 vendorSpec[Page_4KSize - 384];
} __attribute__ ((packed)) hw_nvme_Nsp;

typedef struct hw_nvme_Dev {
	int nspId;
	u64 size;

	struct hw_nvme_Host *host;

	hw_DiskDev diskDev;

	SpinLock lck;
	u64 reqBitmap;
	hw_nvme_Request *reqs[64]; 
} hw_nvme_Dev;

typedef struct hw_nvme_Host {
	hw_pci_Dev pci;
	
	u32 dbStride;

	u16 mxQueSz;

#define hw_nvme_Host_mxIntrNum 4
	u16 intrNum;

	u16 devNum;

	u64 capRegAddr;

	union {
		hw_pci_MsiCap *msi;
		hw_pci_MsixCap *msix;
	};

	intr_Desc *intr;

	u64 flags;
#define hw_nvme_Host_flags_msix	0x1

	// subQue[0]: admin submission queue
	hw_nvme_SubQue **subQue;

	// cmplQue[0]: admin completion queue
	hw_nvme_CmplQue **cmplQue;

	hw_nvme_Dev *dev;
	
} hw_nvme_Host;

__always_inline__ u32 hw_nvme_read32(hw_nvme_Host *host, u32 off) { return hal_read32(host->capRegAddr + off); }

__always_inline__ u64 hw_nvme_read64(hw_nvme_Host *host, u32 off) { return hal_read64(host->capRegAddr + off); }

__always_inline__ void hw_nvme_write32(hw_nvme_Host *host, u32 off, u32 val) { hal_write32(host->capRegAddr + off, val); }

__always_inline__ void hw_nvme_write64(hw_nvme_Host *host, u32 off, u64 val) { hal_write64(host->capRegAddr + off, val); }

__always_inline__ int hw_nvme_CmplQueEntry_phaseTag(hw_nvme_CmplQueEntry *entry) { return hal_read16((u64)&entry->status) & 1; }

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

int hw_nvme_respone(hw_nvme_Request *req, hw_nvme_CmplQueEntry *entry);

#define hw_nvme_Request_Identify_type_Nsp		0x0
#define hw_nvme_Request_Identify_type_Ctrl		0x1
#define hw_nvme_Request_Identify_type_NspLst	0x2
int hw_nvme_initReq_identify(hw_nvme_Request *req, u32 tp , u32 nspIden, void *buf);

int hw_nvme_initReq_createSubQue(hw_nvme_Request *req, hw_nvme_SubQue *subQue);

int hw_nvme_initReq_createCmplQue(hw_nvme_Request *req, hw_nvme_CmplQue *cmplQue);

hw_nvme_SubQue *hw_nvme_allocSubQue(hw_nvme_Host *host, u32 iden, u32 queSz, hw_nvme_CmplQue *trg);

hw_nvme_CmplQue *hw_nvme_allocCmplQue(hw_nvme_Host *host, u32 iden, u32 queSz);

// no chk() for nvme device driver, always return res_FAIL
int hw_nvme_devChk(hw_Device *dev);

// configure device, register file system etc
int hw_nvme_devCfg(hw_Device *dev);

// install device, enable file system etc
int hw_nvme_devInstall(hw_Device *dev);

int hw_nvme_devUninstall(hw_Device *dev);

u64 hw_nvme_devSize(hw_DiskDev *dev);

u64 hw_nvme_devRead(hw_DiskDev *dev, u64 offset, u64 size, void *buf);

u64 hw_nvme_devWrite(hw_DiskDev *dev, u64 offset, u64 size, void *buf);

void hw_nvme_init();
#endif