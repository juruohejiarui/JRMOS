#include <hardware/usb/hid/api.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>

int hw_usb_hid_setReport(hw_usb_xhci_Device *dev, u8 *report, u32 reportSz, u8 reportId, u8 interId) {
    hw_usb_xhci_Request *req = hw_usb_xhci_makeRequest(3, 0);
    hw_usb_xhci_mkCtrlDataReq(req,
        hw_usb_xhci_mkCtrlReqSetup(0x21, 0x09, 0x0200 | reportId, interId, reportSz),
        hw_usb_xhci_TRB_ctrl_dir_out,
        report, reportSz);
    hw_usb_xhci_request(dev->host, dev->epRing[hw_usb_xhci_DevCtx_CtrlEp], req, dev->slotId, hw_usb_xhci_DbReq_make(hw_usb_xhci_DevCtx_CtrlEp, 0));
    if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
        printk(RED, BLACK, "hw: usb hid: device %p failed to set report, code=%d\n", dev, hw_usb_xhci_TRB_getCmplCode(&req->res));
        hw_usb_xhci_freeRequest(req);
        return res_FAIL;
    }
    hw_usb_xhci_freeRequest(req);
    return res_SUCC;
}