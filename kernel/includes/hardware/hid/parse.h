#ifndef __HARDWARE_USB_HID_PARSE_H__
#define __HARDWARE_USB_HID_PARSE_H__

#include <lib/dtypes.h>
#include <lib/list.h>

// global item tags
#define hw_hid_Item_tag_UsagePage          0x0
#define hw_hid_Item_tag_LogicalMinimum     0x1
#define hw_hid_Item_tag_LogicalMaximum     0x2
#define hw_hid_Item_tag_PhysicalMinumum    0x3
#define hw_hid_Item_tag_PhysicalMaximum    0x4
#define hw_hid_Item_tag_UnitExponent       0x5
#define hw_hid_Item_tag_Unit               0x6
#define hw_hid_Item_tag_ReportSize         0x7
#define hw_hid_Item_tag_ReportId           0x8
#define hw_hid_Item_tag_ReportCount        0x9
#define hw_hid_Item_tag_Push               0xa
#define hw_hid_Item_tag_Pop                0xb

// local item tags
#define hw_hid_Item_tag_Usage             0x0
#define hw_hid_Item_tag_UsageMinimum      0x1
#define hw_hid_Item_tag_UsageMaximum      0x2
#define hw_hid_Item_tag_DesignatorIndex   0x3
#define hw_hid_Item_tag_DesignatorMinimum 0x4
#define hw_hid_Item_tag_DesignatorMaximum 0x5
#define hw_hid_Item_tag_StringIndex       0x7
#define hw_hid_Item_tag_StringMinimum     0x8
#define hw_hid_Item_tag_StringMaximum     0x9
#define hw_hid_Item_tag_Delimiter         0xa

// main item tags
#define hw_hid_Item_tag_Input      0x8
#define hw_hid_Item_tag_Output     0x9
#define hw_hid_Item_tag_Feature    0xb
#define hw_hid_Item_tag_Collection 0xa
#define hw_hid_Item_tag_EndCollection 0xc

#define hw_hid_Item_tag_Long 0xf

#define hw_hid_Item_format_Long    0x3
#define hw_hid_Item_format_Short   0x2

#define hw_hid_ReportType_Input     0x0
#define hw_hid_ReportType_Output    0x1
#define hw_hid_ReportType_Feature   0x2

#define hw_hid_Collection_Physical     0x0
#define hw_hid_Collection_Application  0x1
#define hw_hid_Collection_Logical      0x2
#define hw_hid_Collection_Report       0x3
#define hw_hid_Collection_NamedArray   0x4
#define hw_hid_Collection_UsageSwitch  0x5
#define hw_hid_Collection_UsageModifier 0x6

struct hw_hid_Item {
    u8 type;
    u8 tag;
    u8 format;
    u8 size;
    union {
        u8 *longDt;
        u8 u8Dt;
        u16 u16Dt;
        u32 u32Dt;
    };
};

#define hw_hid_ParserGlobal_StkSize 256

struct hw_hid_ParserGlobal {
    u32 usagePg;
    i32 logicalMin, physicalMin;
    i32 logicalMax, physicalMax;
    i32 unitExponent;
    u32 unit;
    u32 reportSz, reportCnt, reportId;
};

#define hw_hid_ParserLocal_UsageMx 8192

struct hw_hid_ParserLocal {
    u32 usage[hw_hid_ParserLocal_UsageMx];
    u32 usageMin;
    u32 usageIdx;
    u32 delimiterDep, delimiterBranch;
    u32 colIdx[hw_hid_ParserLocal_UsageMx];
};

struct hw_hid_Collection {
    u32 type;
    u32 usage;
    u32 lvl; // level in the collection stack
};

#define hw_hid_Parser_MxFields 256
#define hw_hid_Parser_CollectStkSz 256
#define hw_hid_Parser_MxBufSz 16384
#define hw_hid_Parser_ReportTypes 3
#define hw_hid_Parser_DefaultColCap 1

struct hw_hid_Usage {
    u32 hid; // usage id
    u32 colIdx;
    u32 usageIdx;
    i8 resoMulti;
};

struct hw_hid_Field {
    u32 physical, logical, app;
    struct hw_hid_Usage *usage;
    u32 flag;
    u32 reportOff;
    u32 reportSz;
    u32 reportCnt;
    u32 reportType;
    i32 logicalMin, logicalMax;
    i32 physicalMin, physicalMax;
    i32 unitExponent;
    u32 unit;
    u32 hid;
    u32 mxUsage;
    u32 idx;
};

struct hw_hid_Report {
    ListNode lst, fieldLst;
    u32 id, type;
    struct hw_hid_Field *field[256];
    u32 fieldCnt;
    u32 sz;
    u32 app;
};

#define hw_hid_ReportEnum_MxReports 256

struct hw_hid_ReportEnum {
    u32 numbered;
    ListNode reportLst;
    struct hw_hid_Report *report[hw_hid_ReportEnum_MxReports];
};

struct hw_hid_Parser {
    struct hw_hid_ParserGlobal gloStk[hw_hid_ParserGlobal_StkSize], glo;
    struct hw_hid_ReportEnum reportEnum[hw_hid_Parser_ReportTypes];
    int gloStkPtr;
    struct hw_hid_ParserLocal loc;
    u32 colCap, colSz, colStkPtr;
    u32 colStk[hw_hid_Parser_CollectStkSz];
    struct hw_hid_Collection *col;
    u32 mxApp;
};

typedef struct hw_hid_KeyboardInput {
    u8 modifier;
    u8 keys[6];
} __attribute__ ((packed)) hw_hid_KeyboardInput;

typedef struct hw_hid_MouseInput {
    i8 x, y, wheel;
    u8 buttons;
} __attribute__ ((packed)) hw_hid_MouseInput;

typedef struct hw_hid_KeyboardOutput {
    u8 led;
} __attribute__ ((packed)) hw_hid_KeyboardOutput;

typedef struct hw_hid_Parser hw_hid_Parser;

void hw_hid_initParser(hw_hid_Parser *parser);

int hw_hid_parse(u8 *report, u32 reportLen, hw_hid_Parser *parser);

int hw_hid_parseKeyboardInput(hw_hid_Parser *parser, u8 *report, hw_hid_KeyboardInput *input);

int hw_hid_parseMouseInput(hw_hid_Parser *parser, u8 *report, hw_hid_MouseInput *input);

int hw_hid_parseKeyboardOutput(hw_hid_Parser *parser, u8 *report, hw_hid_KeyboardOutput *output);

#endif