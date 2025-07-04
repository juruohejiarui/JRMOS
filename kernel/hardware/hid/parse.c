#include <hardware/hid/parse.h>
#include <screen/screen.h>
#include <lib/string.h>
#include <lib/algorithm.h>
#include <mm/mm.h>

static SafeList _parserLst;

static SpinLock _printLck;

void hw_hid_init() {
    SafeList_init(&_parserLst);
    SpinLock_init(&_printLck);
}

static void _initParser(hw_hid_Parser *parser, hw_Device *dev) {
    memset(parser, 0, sizeof(hw_hid_Parser));
    for (int i = 0; i < hw_hid_Parser_ReportTps; i++) {
        List_init(&parser->reportEnum[i].reportLst);
    }
    parser->colCap = hw_hid_Parser_DefaultColCap;
    parser->col = mm_kmalloc(sizeof(struct hw_hid_Collection) * parser->colCap, mm_Attr_Shared, NULL);
    if (!parser->col) {
        printk(screen_err, "hw: hid: parser %p: collection allocation failed\n", parser);
        return;
    }
    parser->dev = dev;
    memset(parser->col, 0, sizeof(struct hw_hid_Collection) * parser->colCap);
}


hw_hid_Parser *hw_hid_getParser(hw_Device *dev, int create) {
    hw_hid_Parser *par = NULL;
    SafeList_enum(&_parserLst, parserNd) {
        hw_hid_Parser *parser = container(parserNd, hw_hid_Parser, lst);
        if (parser->dev == dev) {
            par = parser;
            SafeList_exitEnum(&_parserLst);
        }
    }
    if (par == NULL && create) {
        par = mm_kmalloc(sizeof(hw_hid_Parser), mm_Attr_Shared, NULL);
        if (par == NULL) {
            printk(screen_err, "hw: hid: failed to create new parser.\n");
            return NULL;
        }
        _initParser(par, dev);
        SafeList_insTail(&_parserLst, &par->lst);
    }
    return par;
}

int hw_hid_delParser(hw_hid_Parser *parser) {
    SafeList_del(&_parserLst, &parser->lst);
    return mm_kfree(parser->col, mm_Attr_Shared) == res_SUCC && mm_kfree(parser, mm_Attr_Shared) == res_SUCC ? res_SUCC : res_FAIL;
}

static u32 hw_hid_Item_udata(struct hw_hid_Item *item) {
    switch (item->size) {
        case 0 : return 0;
        case 1 : return item->u8Dt;
        case 2 : return item->u16Dt;
        case 4 : return item->u32Dt;
    }
}

static i32 hw_hid_Item_sdata(struct hw_hid_Item *item) {
    switch (item->size) {
        case 0 : return 0;
        case 1 : return (i32)*(i8 *)&item->u8Dt;
        case 2 : return (i32)*(i16 *)&item->u16Dt;
        case 4 : return *(i32 *)&item->u32Dt;
    }
    return 0; // should not happen
}

static u8 *_fetchItem(u8 *st, u8 *ed, struct hw_hid_Item *item) {
    if ((ed - st) <= 0) return NULL;
    u8 b = *st++;

    item->tp = (b >> 2) & 0x3, item->tag = (b >> 4) & 0xf;

    if (item->tag == hw_hid_Item_tag_Long) {
        item->format = hw_hid_Item_format_Long;
        if ((ed - st) < 2) return NULL;

        item->size = *st++, item->tag = *st++;
        if ((ed - st) < item->size) return NULL;

        item->longDt = st;
        st += item->size;
        return st;
    }
    item->format = hw_hid_Item_format_Short;
    item->size = b & 0x3;
    switch (item->size) {
        case 0 : return st;
        case 1 : {
            if ((ed - st) < 1) return NULL;
            item->u8Dt = *st++;
            return st;
        }
        case 2 : {
            if ((ed - st) < 2) return NULL;
            item->u16Dt = *(u16 *)st;
            st += 2;
            return st;
        }
        case 3 : {
            if ((ed - st) < 4) return NULL;
            item->u32Dt = *(u32 *)st;
            st += 4;
            item->size = 4;
            return st;
        }
    }
    return NULL;
}

static int _parseGlobal(struct hw_hid_Parser *parser, struct hw_hid_Item *item) {
    switch (item->tag) {
        case hw_hid_Item_tag_Push :
            if (parser->gloStkPtr == hw_hid_ParserGlobal_StkSize) {
                printk(screen_err, "hw: hid: parser %p global stack overflow\n", parser);
                return res_FAIL;
            }
            memcpy(&parser->glo, parser->gloStk + (parser->gloStkPtr++), sizeof(struct hw_hid_ParserGlobal));
            return res_SUCC;
        case hw_hid_Item_tag_Pop :
            if (!parser->gloStkPtr) {
                printk(screen_err, "hw: hid: parser %p global stack underflow\n", parser);
                return res_FAIL;
            }
            memcpy(parser->gloStk + (--parser->gloStkPtr), &parser->glo, sizeof(struct hw_hid_ParserGlobal));
            return res_SUCC;
        case hw_hid_Item_tag_UsagePage : 
            parser->glo.usagePg = hw_hid_Item_udata(item);
            return res_SUCC;
        case hw_hid_Item_tag_LogicalMinimum :
            parser->glo.logicalMin = hw_hid_Item_sdata(item);
            return res_SUCC;
        case hw_hid_Item_tag_LogicalMaximum :
            parser->glo.logicalMax = (parser->glo.logicalMin < 0 ? hw_hid_Item_sdata(item) : hw_hid_Item_udata(item));
            return res_SUCC;
        case hw_hid_Item_tag_PhysicalMinumum :
            parser->glo.physicalMin = hw_hid_Item_sdata(item);
            return res_SUCC;
        case hw_hid_Item_tag_PhysicalMaximum :
            parser->glo.physicalMax = (parser->glo.physicalMin < 0 ? hw_hid_Item_sdata(item) : hw_hid_Item_udata(item));
            return res_SUCC;
        case hw_hid_Item_tag_UnitExponent :
            parser->glo.unitExponent = hw_hid_Item_sdata(item);
            return res_SUCC;
        case hw_hid_Item_tag_Unit :
            parser->glo.unit = hw_hid_Item_udata(item);
            return res_SUCC;
        case hw_hid_Item_tag_ReportSize :
            parser->glo.reportSz = hw_hid_Item_udata(item);
            if (parser->glo.reportSz > 32) {
                printk(screen_err, "hw: hid: parser %p: invalid report size: %d\n", parser, parser->glo.reportSz);
                return res_FAIL;
            }
            return res_SUCC;
        case hw_hid_Item_tag_ReportCount :
            parser->glo.reportCnt = hw_hid_Item_udata(item);
            if (parser->glo.reportCnt > 256) {
                printk(screen_err, "hw: hid: parser %p: invalid report count: %d\n", parser, parser->glo.reportCnt);
                return res_FAIL;
            }
            return res_SUCC;
        case hw_hid_Item_tag_ReportId :
            parser->glo.reportId = hw_hid_Item_udata(item);
            if (parser->glo.reportId == 0) {
                printk(screen_err, "hw: hid: parser %p: invalid report id: %d\n", parser, parser->glo.reportId);
                return res_FAIL;
            }
            return res_SUCC;
        default:
            printk(screen_err, "hw: hid: parser %p: unknown global item tag: %d\n", parser, item->tag);
            return res_FAIL;
    }
}

static int _completeUsage(struct hw_hid_Parser *parser, u32 idx) {
    parser->loc.usage[idx] &= 0xffff;
    parser->loc.usage[idx] |= (parser->glo.usagePg << 16);
}
static int _addUsage(struct hw_hid_Parser *parser, u32 usage) {
    if (parser->loc.usageIdx >= hw_hid_ParserLocal_UsageMx) {
        printk(screen_err, "hw: hid: parser %p: usage index overflow\n", parser);
        return res_FAIL;
    }
    parser->loc.usage[parser->loc.usageIdx] = usage;
    parser->loc.colIdx[parser->loc.usageIdx] = 
        parser->colStkPtr ? parser->colStk[parser->colStkPtr - 1] : 0;
    parser->loc.usageIdx++;
    return res_SUCC;
}

static int _parseLocal(struct hw_hid_Parser *parser, struct hw_hid_Item *item) {
    u32 data; u32 n;
    if (item->size == 0) {
        printk(screen_err, "hw: hid: parser %p: invalid local item size: %d\n", parser, item->size);
        return res_FAIL;
    }
    data = hw_hid_Item_udata(item);
    switch (item->tag) {
        case hw_hid_Item_tag_Delimiter :
            if (data) {
                if (parser->loc.delimiterDep != 0) { 
                    printk(screen_err, "hw: hid: parser %p: nested delimiter %d\n", parser, data);
                    return res_FAIL;
                }
                parser->loc.delimiterDep++;
                parser->loc.delimiterBranch++;
            } else {
                if (parser->loc.delimiterDep < 1) {
                    printk(screen_err, "hw: hid: parser %p: bogus close delimiter\n", parser);
                    return res_FAIL;
                }
                parser->loc.delimiterDep--;
            }
            return res_SUCC;
        case hw_hid_Item_tag_Usage :
            if (parser->loc.delimiterBranch > 1) {
                printk(screen_warn, "hw: hid: parser %p: alternative usage ignored\n", parser);
                return res_SUCC;
            }
            if (item->size <= 2) data = (parser->glo.usagePg << 16) + data;
            return _addUsage(parser, data);
        case hw_hid_Item_tag_UsageMinimum :
            if (parser->loc.delimiterBranch > 1) {
                printk(screen_warn, "hw: hid: parser %p: alternative usage minimum ignored\n", parser);
                return res_SUCC;
            }
            if (item->size <= 2) data = (parser->glo.usagePg << 16) + data;
            parser->loc.usageMin = data;
            return res_SUCC;
        case hw_hid_Item_tag_UsageMaximum :
            if (parser->loc.delimiterBranch > 1) {
                printk(screen_warn, "hw: hid: parser %p: alternative usage minimum ignored\n", parser);
                return res_SUCC;
            }
            if (item->size <= 2) data = (parser->glo.usagePg << 16) + data;
            for (n = parser->loc.usageMin; n <= data; n++) 
                if (_addUsage(parser, n) != res_SUCC) {
                    printk(screen_err, "hw: hid: parser %p: add usage %d failed\n", parser, n);
                    return res_FAIL;
                }
            return res_SUCC;
        default :
            printk(screen_err, "hw: hid: parser %p: unknown local item tag: %d\n", parser, item->tag);
            return res_FAIL;
    }
}

static int _collection(struct hw_hid_Parser *parser, u32 tp ) {
    struct hw_hid_Collection *coll;
    u32 usage = parser->loc.usage[0];

    if (parser->colStkPtr == hw_hid_Parser_CollectStkSz) {
        printk(screen_err, "hw: hid: parser %p: collection stack overflow\n", parser);
        return res_FAIL;
    }
    if (parser->colSz == parser->colCap) {
        coll = mm_kmalloc(sizeof(struct hw_hid_Collection) * parser->colCap * 2, mm_Attr_Shared, NULL);
        if (!coll) {
            printk(screen_err, "hw: hid: parser %p: collection allocation failed\n", parser);
            return res_FAIL;
        }
        memcpy(parser->col, coll, sizeof(struct hw_hid_Collection) * parser->colSz);
        memset(coll + parser->colSz, 0, sizeof(struct hw_hid_Collection) * parser->colCap);
        mm_kfree(parser->col, mm_Attr_Shared);
        parser->col = coll; 
        parser->colCap <<= 1;
    }

    // put the idx of collection into the stack
    parser->colStk[parser->colStkPtr++] = parser->colSz;

    coll = parser->col + parser->colSz++;
    coll->tp = tp;
    coll->usage = usage;
    coll->lvl = parser->colStkPtr - 1;

    if (tp == hw_hid_Collection_Application) parser->mxApp++;

    return res_SUCC;
}

static int _endCollection(struct hw_hid_Parser *parser) {
    if (!parser->colStkPtr) {
        printk(screen_err, "hw: hid: parser %p: collection stack underflow\n", parser);
        return res_FAIL;
    }
    parser->colStkPtr--;
    return res_SUCC;
}

static u32 _lookupCol(struct hw_hid_Parser *parser, u32 tp ) {
    struct hw_hid_Collection *col = parser->col;
    for (int i = parser->colStkPtr - 1; i >= 0; i--) {
        u32 idx = parser->colStk[i];
        if (col[idx].tp == tp) return col[idx].usage;
    }
    return 0;
}

static struct hw_hid_Report *_registerReport(struct hw_hid_Parser *parser, u32 tp , u32 id, u32 app) {
    struct hw_hid_ReportEnum *repEnum = parser->reportEnum + tp ;
    if (id >= hw_hid_ReportEnum_MxReports) {
        printk(screen_err, "hw: hid: parser %p: report id %d too high\n", parser, id);
        return NULL;
    }
    if (repEnum->report[id]) return repEnum->report[id];
    struct hw_hid_Report *report = mm_kmalloc(sizeof(struct hw_hid_Report), mm_Attr_Shared, NULL);
    if (!report) {
        printk(screen_err, "hw: hid: parser %p: report allocation failed\n", parser);
        return NULL;
    }
    memset(report, 0, sizeof(struct hw_hid_Report));
    if (id) repEnum->numbered = 1;
    report->id = id;
    report->tp = tp;
    report->sz = 0;
    report->app = app;
    repEnum->report[id] = report;
    List_insTail(&repEnum->reportLst, &report->lst);
    List_init(&report->fieldLst);

    printk(screen_log, "hw: hid: parser %p: new report %p, tp=%d id=%d\n", parser, report, tp , id);

    return report;
}

static struct hw_hid_Field *_registerField(struct hw_hid_Report *report, u32 usage) {
    if (report->fieldCnt >= hw_hid_Parser_MxFields) {
        printk(screen_err, "hw: hid: report %p: field count overflow\n", report);
        return NULL;
    }
    struct hw_hid_Field *field = mm_kmalloc(
        sizeof(struct hw_hid_Field) + usage * sizeof(struct hw_hid_Usage), mm_Attr_Shared, NULL);
    if (!field) {
        printk(screen_err, "hw: hid: report %p: field allocation failed\n", report);
        return NULL;
    }

    field->idx = report->fieldCnt++;
    report->field[field->idx] = field;
    field->usage = (struct hw_hid_Usage *)(field + 1);

    return field;
}

static int _addField(struct hw_hid_Parser *parser, u32 tp , u32 flags) {
    struct hw_hid_Report *report;
    struct hw_hid_Field *field;

    u32 mxBufSz = hw_hid_Parser_MxBufSz;

    u32 app = _lookupCol(parser, hw_hid_Collection_Application);

    report = _registerReport(parser, tp , parser->glo.reportId, app);

    if (!report) {
        printk(screen_err, "hw: hid: parser %p: failed to register report\n", parser);
        return res_FAIL;
    }

    if ((parser->glo.logicalMin < 0 && parser->glo.logicalMax < parser->glo.logicalMin)
            || (parser->glo.logicalMin >= 0 && (u32)parser->glo.logicalMax < (u32)parser->glo.logicalMin)) {
        printk(screen_err, "hw: hid: parser %p: invalid logical range: %d ~ %d\n", 
               parser, parser->glo.logicalMin, parser->glo.logicalMax);
        return res_FAIL;
    }
    u32 off = report->sz;
    report->sz += parser->glo.reportSz * parser->glo.reportCnt;

    if (report->sz > (mxBufSz - 1) << 3) {
        printk(screen_err, "hw: hid: parser %p: report too long.", parser);
        return res_FAIL;
    }

    if (parser->loc.usageIdx == 0) {
        return res_SUCC;
    }

    u32 usages = max(parser->loc.usageIdx, parser->glo.reportCnt);

    field = _registerField(report, usages);

    if (field == NULL) {
        printk(screen_err, "hw: hid: parser %p: failed to register field\n", parser);
        return res_FAIL;
    }

    field->physical = _lookupCol(parser, hw_hid_Collection_Physical);
    field->logical = _lookupCol(parser, hw_hid_Collection_Logical);
    field->app = app;

    for (int i = 0; i < usages; i++) {
        u32 j = i;
        if (j >= parser->loc.usageIdx) j = parser->loc.usageIdx - 1;
        field->usage[i].hid = parser->loc.usage[j];
        field->usage[i].colIdx = parser->loc.colIdx[j];
        field->usage[i].usageIdx = i;
        field->usage[i].resoMulti = 1; // default resolution multiplier
    }
    field->mxUsage = usages;
    field->flag = flags;
    field->reportOff = off;
    field->reportTp = tp;
    field->reportSz = parser->glo.reportSz;
    field->reportCnt = parser->glo.reportCnt;
    field->logicalMin = parser->glo.logicalMin;
    field->logicalMax = parser->glo.logicalMax;
    field->physicalMin = parser->glo.physicalMin;
    field->physicalMax = parser->glo.physicalMax;
    field->unitExponent = parser->glo.unitExponent;
    field->unit = parser->glo.unit;    
    return res_SUCC;
}

static int _parseMain(struct hw_hid_Parser *parser, struct hw_hid_Item *item) {
    u32 data; int ret;
    data = hw_hid_Item_udata(item);

    switch (item->tag) {
        case hw_hid_Item_tag_Collection :
            ret = _collection(parser, data & 0xff);
            break;
        case hw_hid_Item_tag_EndCollection :
            ret = _endCollection(parser);
            break;
        case hw_hid_Item_tag_Input :
            ret = _addField(parser, hw_hid_ReportTp_Input, data);
            break;
        case hw_hid_Item_tag_Output :
            ret = _addField(parser, hw_hid_ReportTp_Output, data);
            break;
        case hw_hid_Item_tag_Feature :
            ret = _addField(parser, hw_hid_ReportTp_Feature, data);
            break;
        default :
            printk(screen_err, "hw: hid: parser %p: unknown main item tag: %d\n", parser, item->tag);
            return res_FAIL;
    }
    memset(&parser->loc, 0, sizeof(struct hw_hid_ParserLocal));
    return ret;
}
int hw_hid_parse(u8 *report, u32 reportLen, hw_hid_Parser *parser) {
    u8 *st = report, *ed = report + reportLen;
    struct hw_hid_Item item;
    u8 *nxt;
    
    static int (*tsk[])(hw_hid_Parser *parser, struct hw_hid_Item *item) = {
        _parseMain, _parseGlobal, _parseLocal
    };
    while ((nxt = _fetchItem(st, ed, &item)) != NULL) {
        st = nxt;
        if (item.format != hw_hid_Item_format_Short) {
            printk(screen_err, "hw: hid: parser %p: no support for long item.\n", parser);
            goto Fail;
        }
        if (tsk[item.tp](parser, &item) != res_SUCC) {
            printk(screen_err, "hw: hid: parser %p: item %u %u %u %u\n", 
                parser, item.format, (u32)item.tp , (u32)item.size, (u32)item.tag);
            goto Fail;
        }
    }
    return res_SUCC;
    Fail:
    return res_FAIL;
}

static void _printReportEnum(struct hw_hid_ReportEnum *repEnum) {
    int repIdx = 0;
    for (ListNode *repNd = repEnum->reportLst.next; repNd != &repEnum->reportLst; repNd = repNd->next, repIdx++) {
        printk(screen_log, "Report #%d:\n", repIdx);
        struct hw_hid_Report *rep = container(repNd, struct hw_hid_Report, lst);
        for (int i = 0; i < rep->fieldCnt; i++) {
            struct hw_hid_Field *field = rep->field[i];
            printk(screen_log, "\tphy:[%d %d] logical:[%d %d] off:%d sz:%d cnt:%d flag:%d", 
                field->physicalMin, field->physicalMax,
                field->logicalMin, field->logicalMax, 
                field->reportOff, field->reportSz, field->reportCnt,
                field->flag);
            printk(screen_log, "\n");
        }
    }
}

void hw_hid_printParser(hw_hid_Parser *parser) {
    SpinLock_lock(&_printLck);
    printk(screen_log, "parser %p:\nInput:\n", parser);
    struct hw_hid_ReportEnum *repEnum = &parser->reportEnum[hw_hid_ReportTp_Input];
    _printReportEnum(repEnum);

    printk(screen_log, "Output:\n", parser);
    repEnum = &parser->reportEnum[hw_hid_ReportTp_Output];
    _printReportEnum(repEnum);
    SpinLock_unlock(&_printLck);
}

u32 _getReportUData(u8 *report, struct hw_hid_Field *field, int idx) {
    u32 off = field->reportSz * idx + field->reportOff;
    u32 mask = (1u << field->reportSz) - 1;
    mask <<= (off & 0x7);
    return (*(u32 *)(report + (off / 8)) & mask) >> (off & 0x7);
}

i32 _getReportSData(u8 *report, struct hw_hid_Field *field, int idx) {
    u32 off = field->reportSz * idx + field->reportOff;
    u32 mask = (1u << field->reportSz) - 1;
    mask <<= (off & 0x7);
    u32 rawData = (*(u32 *)(report + (off / 8)) & mask) >> (off & 0x7);
    i32 data = (rawData << (32 - field->reportSz));
    return data >> (32 - field->reportSz);
}

int hw_hid_parseKeyboardInput(hw_hid_Parser *parser, u8 *report, hw_hid_KeyboardInput *input) {
    struct hw_hid_ReportEnum *repEnum = &parser->reportEnum[hw_hid_ReportTp_Input];
    struct hw_hid_Report *rep = repEnum->report[0];
    // get modify key status
    input->modifier = 0;
    for (int i = 0; i < rep->field[0]->reportCnt; i++) input->modifier |= (_getReportUData(report, rep->field[0], i) << i);
    for (int i = 0; i < 6; i++) input->keys[i] = _getReportUData(report, rep->field[1], i);
    return res_SUCC;
}

int hw_hid_parseMouseInput(hw_hid_Parser *parser, u8 *report, hw_hid_MouseInput *input) {
    struct hw_hid_ReportEnum *repEnum = &parser->reportEnum[hw_hid_ReportTp_Input];
    struct hw_hid_Report *rep = repEnum->report[0];
    input->buttons = 0;
    for (int i = 0; i < rep->field[0]->reportCnt; i++) input->buttons |= (_getReportUData(report, rep->field[0], i) << i);
    input->x = _getReportSData(report, rep->field[1], 0);
    input->y = _getReportSData(report, rep->field[1], 1);
    input->wheel = (rep->fieldCnt == 3 ? _getReportSData(report, rep->field[2], 0) : _getReportSData(report, rep->field[1], 2));
    return res_SUCC;
}