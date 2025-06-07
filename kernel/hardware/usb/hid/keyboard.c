#include <hardware/usb/hid/api.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>
#include <mm/mm.h>

int hw_usb_hid_keyboard(hw_usb_xhci_Device *dev, hw_hid_Parser *parser, int inEp, int outEp) {
    printk(WHITE, BLACK, "hw: usb hid: device %p is a keyboard with parser %p.\n", dev, parser);
    
    u8 *rawReport = mm_kmalloc(parser->reportEnum[hw_hid_ReportType_Input].report[0]->sz / 8, 0, NULL);

    hw_hid_KeyboardInput input;
    hw_hid_KeyboardOutput output;

    hw_usb_xhci_Request *req = hw_usb_xhci_makeRequest(1, 0);
    
    hw_usb_xhci_TRB_setData(&req->input[0], mm_dmas_virt2Phys(rawReport));
    hw_usb_xhci_TRB_setType(&req->input[0], hw_usb_xhci_TRB_Type_Normal);
    hw_usb_xhci_TRB_setStatus(&req->input[0], hw_usb_xhci_TRB_mkStatus(parser->reportEnum[hw_hid_ReportType_Input].report[0]->sz / 8, 0, 0));
    hw_usb_xhci_TRB_setCtrlBit(&req->input[0], hw_usb_xhci_TRB_ctrl_ioc | hw_usb_xhci_TRB_ctrl_isp);

    memset(rawReport, 0, parser->reportEnum[hw_hid_ReportType_Input].report[0]->sz / 8);
    rawReport[0] = (1 << 4);
    hw_usb_hid_setReport(dev, rawReport, parser->reportEnum[hw_hid_ReportType_Output].report[0]->sz / 8, 0, dev->inter->bIntrNum);

    while (1) {
        hw_usb_xhci_request(dev->host, dev->epRing[inEp], req, dev->slotId, hw_usb_xhci_DbReq_make(inEp, 0));
        if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
            printk(RED, BLACK, "hw: usb hid: device %p failed to get input, code=%d\n", dev, hw_usb_xhci_TRB_getCmplCode(&req->res));
            while (1) hal_hw_hlt();
        }
        hw_hid_parseKeyboardInput(parser, rawReport, &input);
        printk(WHITE, BLACK, "modi: %x k:", input.modifier);
        for (int i = 0; i < 6; i++) printk(WHITE, BLACK, "%x ", input.keys[i]);
        printk(WHITE, BLACK, "\r");
    }
}