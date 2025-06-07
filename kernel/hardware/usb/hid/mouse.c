#include <hardware/usb/hid/api.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>
#include <mm/mm.h>

int hw_usb_hid_mouse(hw_usb_xhci_Device *dev, hw_hid_Parser *parser, int inEp) {
    printk(WHITE, BLACK, "hw: usb hid: device %p is a mouse with parser %p.\n", dev, parser);

    hw_usb_xhci_Request *req = hw_usb_xhci_makeRequest(2, 0);

    hw_usb_xhci_mkCtrlReq(req, hw_usb_xhci_mkCtrlReqSetup(0x21, 0x0a, 0xff00, dev->inter->bIntrNum, 0), hw_usb_xhci_TRB_ctrl_dir_out);
    hw_usb_xhci_request(dev->host, dev->epRing[hw_usb_xhci_DevCtx_CtrlEp], req, dev->slotId, hw_usb_xhci_DbReq_make(hw_usb_xhci_DevCtx_CtrlEp, 0));
    if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
        printk(RED, BLACK, "hw: usb hid: mouse device %p failed to set correct idle.\n");
        hw_usb_xhci_freeRequest(req);
        return res_FAIL;
    }
    hw_usb_xhci_freeRequest(req);
    req = hw_usb_xhci_makeRequest(1, 0);

    u32 sz = parser->reportEnum[hw_hid_ReportType_Input].report[0]->sz / 8;

    u8 *rawReport = mm_kmalloc(sz, 0, NULL);

    hw_usb_xhci_TRB_setData(&req->input[0], mm_dmas_virt2Phys(rawReport));
    hw_usb_xhci_TRB_setStatus(&req->input[0], hw_usb_xhci_TRB_mkStatus(sz, 0x0, 0));
    hw_usb_xhci_TRB_setType(&req->input[0], hw_usb_xhci_TRB_Type_Normal);
    hw_usb_xhci_TRB_setCtrlBit(&req->input[0], hw_usb_xhci_TRB_ctrl_ioc | hw_usb_xhci_TRB_ctrl_isp);

    hw_hid_MouseInput input;

    while (1) {
        hw_usb_xhci_request(dev->host, dev->epRing[inEp], req, dev->slotId, hw_usb_xhci_DbReq_make(inEp, 0));
        if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
            printk(RED, BLACK, "hw: usb hid: device %p failed to get input, code=%d\n", dev, hw_usb_xhci_TRB_getCmplCode(&req->res));
            while (1) hal_hw_hlt();
        }
        printk(WHITE, BLACK, "Mouse: %x %x %x %x\r", rawReport[0], rawReport[1], rawReport[2], rawReport[3]);
    }
}