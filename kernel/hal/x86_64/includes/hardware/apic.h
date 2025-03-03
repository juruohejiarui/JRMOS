#ifndef __HAL_HARDWARE_APIC_H__
#define __HAL_HARDWARE_APIC_H__

#include <lib/dtypes.h>
#include <mm/dmas.h>
#include <hal/hardware/reg.h>

// I/O APIC Redirection Table Entry Descriptor
typedef struct hal_hw_apic_RteDesc {
	u32 vector 			: 8; // 0-7
	u32 deliveryMode 	: 3; // 8-10
	u32 destMode 		: 1; // 11
	u32 deliveryStatus 	: 1; // 12
	u32 pinPolarity 	: 1; // 13
	u32 remoteIRR 		: 1; // 14
	u32 triggerMode 	: 1; // 15
	u32 mask 			: 1; // 16
	u32 reserved 		: 15;// 17-31

	union {
		struct {
			u32 reserved1 	: 24; // 0-23
			u32 dest 		: 8;  // 24-31
		} __attribute__ ((packed)) logical;

		struct {
			u32 reserved2 	: 24; // 0-23
			u32 dest 		: 4;  // 24-27
			u32 reserved3 	: 4;  // 28-31
		} __attribute__ ((packed)) physical;
	} destDesc;

} __attribute__ ((packed)) hal_hw_apic_RteDesc;

typedef struct hal_hw_apic_IcrDesc {
	u32 vector 			: 8;
	u32 deliverMode 	: 3;
	u32 destMode 		: 1;
	u32 deliverStatus	: 1;
	u32 reserved1		: 1;
	u32 level			: 1;
	u32 triggerMode		: 1;
	u32 reserved2		: 2;
	u32 DestShorthand	: 2;
	u32 reserved3		: 12;
	union {
		struct {
			u32 reserved : 24;
			u8 apic : 8;
		} __attribute__ ((packed));
		u32 x2Apic;
	} __attribute__ ((packed)) dest;
} __attribute__ ((packed)) hal_hw_apic_IcrDesc;

// delivery mode
#define hal_hw_apic_DeliveryMode_Fixed 			0x0
#define hal_hw_apic_DeliveryMode_LowestPriority 0x1
#define hal_hw_apic_DeliveryMode_SMI 			0x2
#define hal_hw_apic_DeliveryMode_NMI 			0x4
#define hal_hw_apic_DeliveryMode_INIT 			0x5
#define hal_hw_apic_DeliveryMode_Startup 		0x6
#define hal_hw_apic_DeliveryMode_ExtINT 		0x7

// mask
#define hal_hw_apic_Mask_Unmasked 	0x0
#define hal_hw_apic_Mask_Masked 	0x1

// trigger mode
#define hal_hw_apic_TriggerMode_Edge 	0x0
#define hal_hw_apic_TriggerMode_Level 	0x1

// delivery status
#define hal_hw_apic_DeliveryStatus_Idle 	0x0
#define hal_hw_apic_DeliveryStatus_Pending 	0x1

// destination shorthand
#define hal_hw_apic_DestShorthand_None 			0x0
#define hal_hw_apic_DestShorthand_Self 			0x1
#define hal_hw_apic_DestShorthand_AllIncludingSelf 0x2
#define hal_hw_apic_DestShorthand_AllExcludingSelf 0x3

// destination mode
#define hal_hw_apic_DestMode_Physical 	0x0
#define hal_hw_apic_DestMode_Logical 	0x1

// level
#define hal_hw_apic_Level_Deassert	0x0
#define hal_hw_apic_Level_Assert 	0x1

// remote IRR
#define hal_hw_apic_RemoteIRR_Reset 	0x0
#define hal_hw_apic_RemoteIRR_Accept 	0x1

// pin polarity
#define hal_hw_apic_PinPolarity_High 0x0
#define hal_hw_apic_PinPolarity_Low  0x1

extern u64 hal_hw_apic_supportFlag;

#define hal_hw_apic_supportFlag_X2Apic			(1ul << 0)
#define hal_hw_apic_supportFlag_EOIBroadcase	(1ul << 1)

static __always_inline__ void hal_hw_apic_setIcrDest(hal_hw_apic_IcrDesc *icr, u32 apicId, u32 x2ApicId) {
	(hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_X2Apic) ? (icr->dest.x2Apic = x2ApicId) : (icr->dest.apic = apicId);
}

static __always_inline__ u32 hal_hw_apic_getApicId() {
	return (hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_X2Apic) ? (u32)hal_hw_readMsr(0x802) : *(u32 *)mm_dmas_phys2Virt(0xfee00020);
}


static __always_inline__ u64 hal_hw_apic_getApicBase() { return hal_hw_readMsr(0x1b); }
static __always_inline__ void hal_hw_apic_setApicBase(u64 base) { hal_hw_writeMsr(0x1b, base | hal_hw_readMsr(0x1b)); }

void hal_hw_apic_writeRte(u8 idx, u64 val);
u64 hal_hw_apic_readRte(u8 idx);

void hal_hw_apic_install(u8 intrId, void *arg);
void hal_hw_apic_uninstall(u8 intrId);

void hal_hw_apic_enable(u8 intrId);
void hal_hw_apic_disable(u8 intrId);

void hal_hw_apic_edgeAck(u8 intrId);

int hal_hw_apic_init();

int hal_hw_apic_initSmp();

#endif