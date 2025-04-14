#ifndef __HARDWARE_PCI_H__
#define __HARDWARE_PCI_H__

#include <lib/dtypes.h>
#include <lib/list.h>
#include <lib/bit.h>
#include <mm/dmas.h>
#include <interrupt/api.h>

typedef struct hw_pci_Cfg {
    u16 vendorId, deviceId;
    u16 command, status;
    u8 revisionId, progIf, subclass, class;
    u8 cacheLineSz, latencyTimer, hdrType, bist;
    union {
        struct {
            u32 bar[6];
            u16 subsysVendorId, subsysId;
            u32 expRomBaseAddr;
            u8 capPtr;
            u8 reserved1[7];
            u8 intrLine, intrPin, minGrant, mxMatency;
        } __attribute__ ((packed)) type0;
        struct {
            u32 bar[2];
            u8 primBusNum, secBusNum, subBusNum, secLatencyTimer;
            u16 memBase, memLmt;
            u16 prefetchableMemBase, prefetchableMemLmt;
            u32 prefetchableBaseUpper32;
            u32 prefetchableLmtUpper32;
            u16 ioBaseUpper16, ioLmtUpper16;
            u8 capPtr;
            u8 reserved1[3];
            u32 expRomBaseAddr;
            u8 intrLine, intrPin;
            u16 bridgeCtrl;
        } __attribute__ ((packed)) type1;
        struct {
            u32 cardBusBaseAddr;
            u8 offCapLst, reserved1;
            u16 secondaryStatus;
            u8 pciBusNum, cardBusNum, SubordinateBusNum, cardBusLatencyTimer;
            u32 memBaseAddr0;
            u32 memLmt0;
            u32 memBaseAddr1;
            u32 memLmt1;
            u32 ioBaseAddr0;
            u32 ioLmt0;
            u32 ioBaseAddr1;
            u32 ioLmt1;
            u8 intrLine, intrPin;
            u16 bridgeCtrl;
            u16 subsysDevId, subsysVendorId;
            u32 cardLegacyBaseAddr;
        } __attribute__ ((packed)) type2;
    };
} __attribute__ ((packed)) hw_pci_Cfg;

typedef struct hw_pci_Dev {
    u8 busId, devId, funcId;
    hw_pci_Cfg *cfg;
    ListNode list;
} hw_pci_Dev;

typedef struct hw_pci_CapHdr {
    u8 capId;
    u8 nxtPtr;
} __attribute__ ((packed)) hw_pci_CapHdr;

typedef struct hw_pci_MsiCap {
    hw_pci_CapHdr hdr;
    u16 msgCtrl;
    #define hw_pci_MsiCap_vecNum(cap) ((1ul) << (((cap)->msgCtrl >> 1) & 0x7))
    u64 msgAddr;
    u16 msgData;
    u16 reserved;
    u32 msk;
    u32 pending;
} __attribute__ ((packed)) hw_pci_MsiCap;

typedef struct hw_pci_MsixCap {
    hw_pci_CapHdr hdr;
    u16 msgCtrl;
    #define hw_pci_MsixCap_vecNum(cap) (((cap)->msgCtrl & ((1u << 11) - 1)) + 1)
    u32 dw1;
    #define hw_pci_MsixCap_bir(cap) ((cap)->dw1 & 0x7)
    #define hw_pci_MsixCap_tblOff(cap) ((cap)->dw1 & ~0x7u)
    u32 dw2;
    #define hw_pci_MsixCap_pendingBir(cap) ((cap)->dw2 & 0x7)
    #define hw_pci_MsixCap_pendingTblOff(cap) ((cap)->dw2 & ~0x7u)
} __attribute__ ((packed)) hw_pci_MsixCap;

typedef struct hw_pci_MsixTbl {
    u64 msgAddr;
    u32 msgData;
    u32 vecCtrl;
} __attribute__ ((packed)) hw_pci_MsixTbl;

extern SafeList hw_pci_devLst;

extern intr_Ctrl hw_pci_intrCtrl;

static __always_inline__ hw_pci_Cfg *hw_pci_getCfg(u64 baseAddr, u8 bus, u8 dev, u8 func) {
    return (hw_pci_Cfg *)mm_dmas_phys2Virt(baseAddr | ((u64)bus << 20) | ((u64)dev << 15) | ((u64)func << 12));
}

void hw_pci_chkBus(u64 baseAddr, u16 bus);

int hw_pci_init();

void hw_pci_initIntr(intr_Desc *desc, void (*handler)(u64), u64 param, char *name);

static __always_inline__ hw_pci_MsixTbl *hw_pci_getMsixTbl(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg) {
    return mm_dmas_phys2Virt((*(u64 *)&cfg->type0.bar[hw_pci_MsixCap_bir(cap)] & ~0xf) + hw_pci_MsixCap_tblOff(cap));
}

int hw_pci_allocMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum);

int hw_pci_allocMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum);

int hw_pci_enableMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrIdx);

int hw_pci_enableMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrIdx);

int hw_pci_disableMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrIdx);

int hw_pci_disableMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrIdx);

int hw_pci_enableMsiAll(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum);

int hw_pci_enableMsixAll(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum);

int hw_pci_disableMsiAll(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum);

int hw_pci_disableMsixAll(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum);

int hw_pci_freeMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum);

int hw_pci_freeMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum);

#endif