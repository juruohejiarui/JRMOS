#ifndef __HAL_HARDWARE_HPET_H__
#define __HAL_HARDWARE_HPET_H__

#include <hal/hardware/uefi.h>
#include <hal/hardware/reg.h>
#include <mm/dmas.h>
#include <hal/interrupt/api.h>

typedef struct hal_hw_hpet_UefiAddr {
	u8 addrSpaceId;
	u8 regBitWidth;
	u8 regBitOffset;
	u8 reserved;
	u64 addr;
} __attribute__ ((packed)) hal_hw_hpet_UefiAddr;

typedef struct hal_hw_hpet_XsdtDesc {
    hal_hw_uefi_AcpiHeader hdr;

    u8 hardwareRevID;			// 36
	u8 comparatorCount : 5;		// 37-41
	u8 counterSize : 1;			// 42
	u8 reserved : 1;			// 43
	u8 legacyReplace : 1; 	    // 44
	u16 pciVendorID;			// 45-46
	hal_hw_hpet_UefiAddr addr; 	// 47-63
	u32 hpetNum;				// 64-67
	u16 minTick;			    // 68-69
	u8 pageProtect;			    // 70
} __attribute__((packed)) hal_hw_hpet_XsdtDesc;

#define hal_hw_hpet_capId	0x0
#define hal_hw_hpet_capId_64bit				(1ul << 13)
#define hal_hw_hpet_capId_LegacyReplace		(1ul << 15)

#define hal_hw_hpet_cfg		0x10
#define hal_hw_hpet_cfg_Enable			(1ul << 0)
#define hal_hw_hpet_cfg_LegacyReplace	(1ul << 1)

#define hal_hw_hpet_intrState	0x20

#define hal_hw_hpet_mainCounter	0xf0

#define hal_hw_hpet_timerCfgCap	0x100
// interrupt type of this timer: 0: edge, 1: level
#define hal_hw_hpet_timerCfgCap_IntrT	(1ul << 1)
#define hal_hw_hpet_timerCfgCap_Enable	(1ul << 2)
#define hal_hw_hpet_timerCfgCap_Period	(1ul << 3)
#define hal_hw_hpet_timerCfgCap_PeriodCap	(1ul << 4)
#define hal_hw_hpet_timerCfgCap_64Cap		(1ul << 5)
// 1: allow software to directly set periodic timer's accumulator.
#define hal_hw_hpet_timerCfgCap_SetVal		(1ul << 6)
// 1: force a 64-bit timer to perform as in 32-bit mode. No effect on 32-bit timer
#define hal_hw_hpet_timerCfgCap_32bit		(1ul << 8)
#define hal_hw_hpet_timerCfgCap_fsbEnable	(1ul << 14)
#define hal_hw_hpet_timerCfgCap_fsbCap		(1ul << 15)
#define hal_hw_hpet_timerCfgCap_Irq(x)		(1ul << ((x) + 32))

#define hal_hw_hpet_timerCfgCap_CapPart	(hal_hw_hpet_timerCfgCap_PeriodCap | hal_hw_hpet_timerCfgCap_64Cap | hal_hw_hpet_timerCfgCap_fsbCap)

#define hal_hw_hpet_timerCmpVal		0x108
#define hal_hw_hpet_timerFsbIntr	0x110

extern hal_hw_hpet_XsdtDesc *hal_hw_hpet_xsdtDesc;

static __always_inline__ void hal_hw_hpet_setReg64(u64 offset, u64 val) {
	*(u64 *)mm_dmas_phys2Virt(hal_hw_hpet_xsdtDesc->addr.addr + offset) = val;
	hal_hw_mfence();
}
static __always_inline__ u64 hal_hw_hpet_getReg64(u64 offset) { return *(u64 *)mm_dmas_phys2Virt(hal_hw_hpet_xsdtDesc->addr.addr + offset); }

static __always_inline__ void hal_hw_hpet_setReg32(u64 offset, u32 val) { 
	*(u32 *)mm_dmas_phys2Virt(hal_hw_hpet_xsdtDesc->addr.addr + offset) = val;
	hal_hw_mfence();
}
static __always_inline__ u64 hal_hw_hpet_getReg32(u64 offset) { return *(u32 *)mm_dmas_phys2Virt(hal_hw_hpet_xsdtDesc->addr.addr + offset); }

static __always_inline__ u64 hal_hw_getTimerCfg(u32 id) { return hal_hw_hpet_getReg64(0x20 * id + hal_hw_hpet_timerCfgCap); }
static __always_inline__ void hal_hw_setTimerCfg(u32 id, u64 cfg) {
	u64 cap = hal_hw_getTimerCfg(id) & hal_hw_hpet_timerCfgCap_CapPart;
	hal_hw_hpet_setReg64(0x20 * id + hal_hw_hpet_timerCfgCap, cap | cfg);
}

#define hal_hw_hpet_intrDesc (&hal_intr_desc[hal_intr_vec_Timer - 0x20])

static __always_inline__ void hal_hw_setTimerCmp(u32 id, u32 comparator) {
	hal_hw_hpet_setReg64(0x20 * id + hal_hw_hpet_timerCmpVal, comparator);
}

int hal_hw_hpet_init();

#endif