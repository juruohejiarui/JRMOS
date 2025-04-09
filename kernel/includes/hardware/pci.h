#ifndef __HARDWARE_PCI_H__
#define __HARDWARE_PCI_H__

#include <lib/dtypes.h>
#include <lib/list.h>

typedef struct pci_Cfg {
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
} __attribute__ ((packed)) pci_Cfg;

typedef struct pci_Dev {
    u8 busId, devId, funcId;
    pci_Cfg *cfg;
    ListNode list;
} pci_Dev;

int hw_pci_init();

#endif