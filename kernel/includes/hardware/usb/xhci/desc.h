#ifndef __HARDWARD_USB_XHCI_DESC_H__
#define __HARDWARE_USB_XHCI_DESC_H__

#include <hal/hardware/reg.h>
#include <hardware/pci.h>
#include <hardware/usb/devdesc.h>
#include <interrupt/desc.h>
#include <task/api.h>


typedef struct hw_usb_xhci_TRB {
    u32 dt1, dt2, status, ctrl;
} __attribute__ ((packed)) hw_usb_xhci_TRB;

typedef struct hw_usb_xhci_Request {
    u32 flags, inputSz;
    #define hw_usb_xhci_Request_flags_Finished  (1u << 0)
    #define hw_usb_xhci_Request_flags_Command   (1u << 1)
    hw_usb_xhci_TRB res;
    hw_usb_xhci_TRB input[];
} __attribute__ ((packed)) hw_usb_xhci_Request;

// evaluate next TRB
#define hw_usb_xhci_TRB_ctrl_ent    (1u << 1)
// interrupt on short packet
#define hw_usb_xhci_TRB_ctrl_isp    (1u << 2)
// no snoop
#define hw_usb_xhci_TRB_ctrl_noSnoop    (1u << 3)
// chain
#define hw_usb_xhci_TRB_ctrl_chain  (1u << 4)
// inerrupt on completion
#define hw_usb_xhci_TRB_ctrl_ioc    (1u << 5)
// immediate data
#define hw_usb_xhci_TRB_ctrl_idt    (1u << 6)
// block event interrupt
#define hw_usb_xhci_TRB_ctrl_bei    (1u << 9)

#define hw_usb_xhci_TRB_ctrl_allBit (0x0000027eu)

#define hw_usb_xhci_TRB_ctrl_dir_in 1
#define hw_usb_xhci_TRB_ctrl_dir_out 0
#define hw_usb_xhci_TRB_trt_No  0
#define hw_usb_xhci_TRB_trt_Out 2
#define hw_usb_xhci_TRB_trt_In  3

typedef enum hw_usb_xhci_TRB_Type {
	hw_usb_xhci_TRB_Type_Normal = 1, 	hw_usb_xhci_TRB_Type_SetupStage, 	hw_usb_xhci_TRB_Type_DataStage,	    hw_usb_xhci_TRB_Type_StatusStage,
	hw_usb_xhci_TRB_Type_Isoch, 		hw_usb_xhci_TRB_Type_Link, 		    hw_usb_xhci_TRB_Type_EventData,	    hw_usb_xhci_TRB_Type_NoOp,
	hw_usb_xhci_TRB_Type_EnblSlot,		hw_usb_xhci_TRB_Type_DisblSlot,	    hw_usb_xhci_TRB_Type_AddrDev,		hw_usb_xhci_TRB_Type_CfgEp,
	hw_usb_xhci_TRB_Type_EvalCtx,		hw_usb_xhci_TRB_Type_ResetEp,		hw_usb_xhci_TRB_Type_StopEp,		hw_usb_xhci_TRB_Type_SetTrDeq,
	hw_usb_xhci_TRB_Type_ResetDev,		hw_usb_xhci_TRB_Type_ForceEvent,	hw_usb_xhci_TRB_Type_DegBand,		hw_usb_xhci_TRB_Type_SetLatToler,
	hw_usb_xhci_TRB_Type_GetPortBand,	hw_usb_xhci_TRB_Type_ForceHdr,		hw_usb_xhci_TRB_Type_NoOpCmd,
	hw_usb_xhci_TRB_Type_TransEve = 32, hw_usb_xhci_TRB_Type_CmdCmpl,		hw_usb_xhci_TRB_Type_PortStChg,	    hw_usb_xhci_TRB_Type_BandReq,
	hw_usb_xhci_TRB_Type_DbEve,		    hw_usb_xhci_TRB_Type_HostCtrlEve,	hw_usb_xhci_TRB_Type_DevNotfi,		hw_usb_xhci_TRB_Type_MfidxWrap
} hw_usb_xhci_TRB_Type;

typedef enum hw_usb_xhci_TRB_CmplCode {
    hw_usb_xhci_TRB_CmplCode_Succ = 1,			hw_usb_xhci_TRB_CmplCode_DtBufErr,			hw_usb_xhci_TRB_CmplCode_BadDetect,	    hw_usb_xhci_TRB_CmplCode_TransErr,
	hw_usb_xhci_TRB_CmplCode_TrbErr,			hw_usb_xhci_TRB_CmplCode_StallErr,			hw_usb_xhci_TRB_CmplCode_ResrcErr,		hw_usb_xhci_TRB_CmplCode_BandErr,
	hw_usb_xhci_TRB_CmplCode_NoSlotErr,		    hw_usb_xhci_TRB_CmplCode_InvalidStreamT,	hw_usb_xhci_TRB_CmplCode_SlotNotEnbl,	hw_usb_xhci_TRB_CmplCode_EpNotEnbl,
	hw_usb_xhci_TRB_CmplCode_ShortPkg = 13,	    hw_usb_xhci_TRB_CmplCode_RingUnderRun,		hw_usb_xhci_TRB_CmplCode_RingOverRun,	hw_usb_xhci_TRB_CmplCode_VFEveRingFull,
	hw_usb_xhci_TRB_CmplCode_ParamErr,			hw_usb_xhci_TRB_CmplCode_BandOverRun,		hw_usb_xhci_TRB_CmplCode_CtxStsErr,	    hw_usb_xhci_TRB_CmplCode_NoPingResp,
	hw_usb_xhci_TRB_CmplCode_EveRingFull,		hw_usb_xhci_TRB_CmplCode_IncompDev,		    hw_usb_xhci_TRB_CmplCode_MissServ,		hw_usb_xhci_TRB_CmplCode_CmdRingStop,
	hw_usb_xhci_TRB_CmplCode_CmdAborted,		hw_usb_xhci_TRB_CmplCode_Stop,				hw_usb_xhci_TRB_CmplCode_StopLenErr,	hw_usb_xhci_TRB_CmplCode_Reserved,
	hw_usb_xhci_TRB_CmplCode_IsochBufOverRun,	hw_usb_xhci_TRB_CmplCode_EvernLost = 32,	hw_usb_xhci_TRB_CmplCode_Undefined,	    hw_usb_xhci_TRB_CmplCode_InvalidStreamId,
	hw_usb_xhci_TRB_CmplCode_SecBand,			hw_usb_xhci_TRB_CmplCode_SplitTrans
} hw_usb_xhci_TRB_CmplCode;

typedef struct hw_usb_xhci_SlotCtx32 {
    u32 dw0, dw1, dw2, dw3;
    #define hw_usb_xhci_SlotCtx_routeStr	0x000fffffu
	#define hw_usb_xhci_SlotCtx_speed		0x00f00000u
	#define hw_usb_xhci_SlotCtx_mtt		    0x02000000u
	#define hw_usb_xhci_SlotCtx_hub		    0x04000000u
	#define hw_usb_xhci_SlotCtx_ctxEntries	0xf8000000u

    #define hw_usb_xhci_SlotCtx_maxExitLatency	0x0000ffffu
	#define hw_usb_xhci_SlotCtx_rootPortNum	    0x00ff0000u
	#define hw_usb_xhci_SlotCtx_numOfPorts		0xff000000u

    #define hw_usb_xhci_SlotCtx_ttHubSlotId	0x000000ffu
	#define hw_usb_xhci_SlotCtx_ttPortNum	0x0000ff00u
	#define hw_usb_xhci_SlotCtx_ttt			0x00030000u
	#define hw_usb_xhci_SlotCtx_intrTarget	0xffc00000u

    #define hw_usb_xhci_SlotCtx_devAddr	    0x000000ffu
	#define hw_usb_xhci_SlotCtx_slotState	0xff000000u
    u32 reserved[4];
} __attribute__ ((packed)) hw_usb_xhci_SlotCtx32;

typedef struct hw_usb_xhci_EpCtx32 {
    u32 dw0, dw1;
    union {
        u64 depPtr;
        struct {
            u32 dw2, dw3;
        } __attribute__ ((packed));
    };
    u32 dw4;
    u32 reserved[3];

    #define hw_usb_xhci_EpCtx_epState		0x00000007u
	#define hw_usb_xhci_EpCtx_multi		    0x00000300u
	#define hw_usb_xhci_EpCtx_mxPStreams	0x00007C00u
	#define hw_usb_xhci_EpCtx_lsa			0x00008000u
	#define hw_usb_xhci_EpCtx_interval		0x00ff0000u
	#define hw_usb_xhci_EpCtx_mxESITPayH	0xff000000u

    #define hw_usb_xhci_EpCtx_CErr			0x00000006u
	#define hw_usb_xhci_EpCtx_epType		0x00000038u
	#define hw_usb_xhci_EpCtx_hid			0x00000080u
	#define hw_usb_xhci_EpCtx_mxBurstSize	0x0000ff00u
	#define hw_usb_xhci_EpCtx_mxPackSize	0xffff0000u

    #define hw_usb_xhci_EpCtx_dcs	        0x00000001u

    #define hw_usb_xhci_EpCtx_aveTrbLen	    0x0000ffffu
	#define hw_usb_xhci_EpCtx_mxESITPayL	0xffff0000u
} __attribute__ ((packed)) hw_usb_xhci_EpCtx32;

typedef struct hw_usb_xhci_SlotCtx64 {
    u32 dw0, dw1, dw2, dw3;
    u32 reserved[12];
} __attribute__ ((packed)) hw_usb_xhci_SlotCtx64;

typedef struct hw_usb_xhci_EpCtx64 {
    u32 dw0, dw1;
    union {
        u64 depPtr;
        struct {
            u32 dw2, dw3;
        } __attribute__ ((packed));
    };
    u32 dw4;
    u32 reserved[11];
} __attribute__ ((packed)) hw_usb_xhci_EpCtx64;

#define hw_usb_xhci_EpCtx_epType_IsochOut   1
#define hw_usb_xhci_EpCtx_epType_BulkOut    2
#define hw_usb_xhci_EpCtx_epType_IntrOut    3
#define hw_usb_xhci_EpCtx_epType_Ctrl       4
#define hw_usb_xhci_EpCtx_epType_IsochIn    5
#define hw_usb_xhci_EpCtx_epType_BulkIn     6
#define hw_usb_xhci_EpCtx_epType_IntrIn     7

typedef struct hw_usb_xhci_DevCtx32 {
    hw_usb_xhci_SlotCtx32 slot;
    hw_usb_xhci_EpCtx32 ep[31];
} __attribute__ ((packed)) hw_usb_xhci_DevCtx32;

typedef struct hw_usb_xhci_DevCtx64 {
    hw_usb_xhci_SlotCtx64 slot;
    hw_usb_xhci_EpCtx64 ep[31];
} __attribute__ ((packed)) hw_usb_xhci_DevCtx64;

typedef union hw_usb_xhci_DevCtx {
    hw_usb_xhci_DevCtx32 *ctx32;
    hw_usb_xhci_DevCtx64 *ctx64;
} __attribute__ ((packed)) hw_usb_xhci_DevCtx;

typedef struct hw_usb_xhci_InCtrlCtx32 {
    u32 drop, add, reserved[5], dw7;
} __attribute__ ((packed)) hw_usb_xhci_InCtrlCtx32;

typedef struct hw_usb_xhci_InCtrlCtx64 {
    u32 drop, add, reserved[5], dw7, reserved1[8];
} __attribute__ ((packed)) hw_usb_xhci_InCtrlCtx64;

typedef struct hw_usb_xhci_InCtx32 {
    hw_usb_xhci_InCtrlCtx32 ctrl;
    hw_usb_xhci_SlotCtx32 slot;
    hw_usb_xhci_EpCtx32 ep[31];
} __attribute__ ((packed)) hw_usb_xhci_InCtx32;

typedef struct hw_usb_xhci_InCtx64 {
    hw_usb_xhci_InCtrlCtx64 ctrl;
    hw_usb_xhci_SlotCtx64 slot;
    hw_usb_xhci_EpCtx64 ep[31];
} __attribute__ ((packed)) hw_usb_xhci_InCtx64;

typedef union hw_usb_xhci_InCtx {
    hw_usb_xhci_InCtx32 *ctx32;
    hw_usb_xhci_InCtx64 *ctx64;
} __attribute__ ((packed)) hw_usb_xhci_InCtx;

#define hw_usb_xhci_InCtx_Ctrl 0x0
#define hw_usb_xhci_InCtx_Slot 0x1
#define hw_usb_xhci_InCtx_Ep(x, dir) ((((x) << 1) | dir) + 1)

#define hw_usb_xhci_DevCtx_Slot 0x0
#define hw_usb_xhci_DevCtx_Ep(x, dir) (((x) << 1) | dir)

#define hw_usb_xhci_Speed_Full  1
#define hw_usb_xhci_Speed_Low   2
#define hw_usb_xhci_Speed_High  3
#define hw_usb_xhci_Speed_Super 4

typedef struct hw_usb_xhci_Ring {
    SpinLock lck;
    u32 curIdx, size;
    u32 cycBit, load;
    ListNode lst;
    hw_usb_xhci_Request **reqs;
    hw_usb_xhci_TRB *trbs;
} __attribute__ ((packed)) hw_usb_xhci_Ring;

typedef struct hw_usb_xhci_EveRing {
    hw_usb_xhci_TRB **rings;
    u16 curRingIdx, ringNum; 
    u32 curIdx, ringSz;
    u32 cycBit;
} __attribute__ ((packed)) hw_usb_xhci_EveRing;

#define hw_usb_xhci_Ring_mxSz (Page_4KSize * 16 / sizeof(hw_usb_xhci_TRB))

typedef struct hw_usb_xhci_Device {
    struct hw_usb_xhci_Host *host;
    struct hw_usb_xhci_Device *parent;
    u32 slotId;
    u16 portId, speed;
    u32 flag;

    #define hw_usb_xhci_Device_flag_Enbl    (1ul << 0)
    // whether the device is directly connected to the host
    #define hw_usb_xhci_Device_flag_Direct  (1ul << 1)
    
    task_TaskStruct *mgrTsk;

    hw_usb_xhci_DevCtx *ctx;
    hw_usb_xhci_InCtx *inCtx;

    hw_usb_xhci_Ring *epRing[32];

    hw_usb_devdesc_Device *devDesc;
    hw_usb_devdesc_Cfg *cfgDesc;

    ListNode lst;
} hw_usb_xhci_Device;

typedef struct hw_usb_xhci_Port {
    u8 flags;
    u8 pirOffset;
    u8 offset;
    u8 portId;
    hw_usb_xhci_Device *dev;
} hw_usb_xhci_Port;

typedef struct hw_usb_xhci_Host {
    hw_pci_Dev *pci;

    union {
        hw_pci_MsiCap *msiCap;
        hw_pci_MsixCap *msixCap;
    };

    intr_Desc *intr;

    u64 intrNum;

    u64 flag;

    #define hw_usb_xhci_Host_flag_Msix   (1ul << 0)
    #define hw_usb_xhci_Host_flag_Ctx64  (1ul << 1)

    ListNode lst;
    // device that attached to this controller
    SafeList devLst;
    SafeList ringLst;

    u64 capRegAddr;
    u64 opRegAddr;
    u64 rtRegAddr;
    u64 dbRegAddr;

    hw_usb_xhci_Port *ports;
    hw_usb_xhci_Ring *cmdRing;
    hw_usb_xhci_EveRing **eveRings;

    hw_usb_xhci_DevCtx **devCtx;

    hw_usb_xhci_Device **dev;
    hw_usb_xhci_Device **portDev;

    
} hw_usb_xhci_Host;

#define hw_usb_xhci_Host_capReg_capLen      0x0
#define hw_usb_xhci_Host_capReg_hciVer      0x2
#define hw_usb_xhci_Host_capReg_hcsParam1   0x4
#define hw_usb_xhci_Host_capReg_hcsParam2   0x8
#define hw_usb_xhci_Host_capReg_hcsParam3   0xc
#define hw_usb_xhci_Host_capReg_hccParam1   0x10
#define hw_usb_xhci_Host_capReg_hccParam2   0x1c
#define hw_usb_xhci_Host_capReg_dbOff	    0x14
#define hw_usb_xhci_Host_capReg_rtsOff	    0x18

#define hw_usb_xhci_Host_opReg_cmd		0x0
#define hw_usb_xhci_Host_opReg_status	0x4
#define hw_usb_xhci_Host_opReg_pgSize	0x8
// device notification control
#define hw_usb_xhci_Host_opReg_dnCtrl	0x14
// command ring control
#define hw_usb_xhci_Host_opReg_crCtrl	0x18
// device context base address array pointer
#define hw_usb_xhci_Host_opReg_dcbaa	0x30
#define hw_usb_xhci_Host_opReg_cfg		0x38

#define hw_usb_xhci_Ext_Id_Legacy		0x1

#define hw_usb_xhci_Ext_Id_Protocol     0x2

#define hw_usb_xhci_Host_portReg_baseOffset	    0x400
#define hw_usb_xhci_Host_portReg_offset	        0x10
// port status and control
#define hw_usb_xhci_Host_portReg_sc		        0x00
#define hw_usb_xhci_Host_portReg_sc_AllEve		(0xe000000u)
#define hw_usb_xhci_Host_portReg_sc_AllChg		(0xfe0000u)
#define hw_usb_xhci_Host_portReg_sc_Power		(1u << 9)
// port power management staatus and control
#define hw_usb_xhci_Host_portReg_pwsc	        0x04
// port link info
#define hw_usb_xhci_Host_portReg_lk		        0x08

#define hw_usb_xhci_intrReg_IMan	    0x00
#define hw_usb_xhci_intrReg_IMod	    0x04
#define hw_usb_xhci_intrReg_TblSize	    0x08
#define hw_usb_xhci_intrReg_TblAddr	    0x10
#define hw_usb_xhci_intrReg_DeqPtr		0x18

#endif