#include <hardware/usb/hid/api.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>
#include <mm/mm.h>

int hw_usb_hid_mouse(hw_usb_xhci_Device *dev, hw_hid_Parser *parser, int inEp) {
    printk(WHITE, BLACK, "hw: usb hid: device %p is a mouse with parser %p.\n", dev, parser);

    hw_usb_xhci_Request *req = hw_usb_xhci_makeRequest(1, 0);

    u32 sz = parser->reportEnum[hw_hid_ReportType_Input].report[0]->sz / 8;

    u8 *rawReport = mm_kmalloc(sz, 0, NULL);

    hw_usb_xhci_TRB_setData(&req->input[0], mm_dmas_virt2Phys(rawReport));
    hw_usb_xhci_TRB_setStatus(&req->input[0], hw_usb_xhci_TRB_mkStatus(sz, 0x0, 0));
    hw_usb_xhci_TRB_setType(&req->input[0], hw_usb_xhci_TRB_Type_Normal);
    hw_usb_xhci_TRB_setCtrlBit(&req->input[0], hw_usb_xhci_TRB_ctrl_ioc | hw_usb_xhci_TRB_ctrl_isp);

    hw_hid_MouseInput input;

    while (1) {
        hw_usb_xhci_request(dev->host, dev->epRing[inEp], req, dev, hw_usb_xhci_DbReq_make(inEp, 0));
        if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
            printk(RED, BLACK, "hw: usb hid: device %p failed to get input, code=%d\n", dev, hw_usb_xhci_TRB_getCmplCode(&req->res));
            while (1) hal_hw_hlt();
        }
        hw_hid_parseMouseInput(parser, rawReport, &input);
        printk(WHITE, BLACK, "Mouse: btn:%2x (%4d,%4d,%4d)\r", input.buttons, input.x, input.y, input.wheel);
    }
}