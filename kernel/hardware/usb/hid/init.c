#include <hardware/usb/hid/api.h>
#include <hardware/usb/xhci/api.h>
#include <hardware/hid/parse.h>
#include <mm/mm.h>
#include <screen/screen.h>
#include <lib/algorithm.h>

static hw_usb_hid_Driver hw_usb_hid_driver;

int hw_usb_hid_check(hw_Device *dev) {
    // check if the device is a USB device by searching the xhci host list
    if (hw_usb_xhci_isXhciDev(dev) == res_SUCC) goto Found;
    return res_FAIL;
    Found:
    hw_usb_xhci_Device *usbDev = container(dev, hw_usb_xhci_Device, device);
    for (hw_usb_devdesc_Hdr *hdr = &usbDev->cfgDesc[0]->hdr; hdr; hdr = hw_usb_devdesc_getNxt(usbDev->cfgDesc[0], hdr)) {
        // check interface descriptor
        if (hdr->tp == hw_usb_devdesc_Tp_Inter) {
            hw_usb_devdesc_Inter *inter = container(hdr, hw_usb_devdesc_Inter, hdr);
            if (inter->bInterCls == 0x03) {
                printk(screen_succ, "hw: usb hid: device %p is a USB HID device\n", dev);
                return res_SUCC;
            }
        }
    }
    return res_FAIL;
}

int hw_usb_hid_config(hw_Device *dev) {
    // set configure
    hw_usb_devdesc_Inter *interDesc = NULL;
    hw_usb_xhci_Device *usbDev = container(dev, hw_usb_xhci_Device, device);

    // find the most suitable interface descriptor
    for (hw_usb_devdesc_Hdr *hdr = &usbDev->cfgDesc[0]->hdr; hdr; hdr = hw_usb_devdesc_getNxt(usbDev->cfgDesc[0], hdr)) {
        // check interface descriptor
        if (hdr->tp == hw_usb_devdesc_Tp_Inter) {
            hw_usb_devdesc_Inter *inter = container(hdr, hw_usb_devdesc_Inter, hdr);
            if (inter->bInterCls == 0x03) {
                if (interDesc == NULL || interDesc->bInterSubCls != 0x01) interDesc = inter;
                break;
            }
        }
    }
    printk(screen_log, "hw: usb hid: device %p interface descriptor: %p, class %02x, subclass %02x, protocol %02x\n", 
           dev, interDesc, interDesc->bInterCls, interDesc->bInterSubCls, interDesc->bInterProtocol);
    // read hid descriptor & report descriptor for parsing
    hw_usb_devdesc_Hid *hidDesc = NULL;
    hw_usb_devdesc_Ep **epDesc = mm_kmalloc(sizeof(hw_usb_devdesc_Ep *) * interDesc->bNumEp, mm_Attr_Shared, NULL);
    // read descriptors
    {
        int epDescIdx = 0;
        for (hw_usb_devdesc_Hdr *hdr = hw_usb_devdesc_getNxt(usbDev->cfgDesc[0], &interDesc->hdr); 
                hdr && hdr->tp != hw_usb_devdesc_Tp_Inter; 
                hdr = hw_usb_devdesc_getNxt(usbDev->cfgDesc[0], hdr)) {
            switch (hdr->tp) {
                case hw_usb_devdesc_Tp_HID: hidDesc = container(hdr, hw_usb_devdesc_Hid, hdr); break;
                case hw_usb_devdesc_Tp_Ep: 
                    epDesc[epDescIdx++] = container(hdr, hw_usb_devdesc_Ep, hdr);
                    break;
            }
        }
    }
    {
        hw_usb_xhci_InCtx *inCtx = usbDev->inCtx;
        void *slotCtx = hw_usb_xhci_getCtxEntry(usbDev->host, inCtx, hw_usb_xhci_InCtx_Slot);
        hw_usb_xhci_writeCtx(inCtx, 1, ~0x0u, 1);
        for (int i = 0; i < interDesc->bNumEp; i++) {
            int epId = hw_usb_devdesc_Ep_epId(epDesc[i]), epTp = hw_usb_devdesc_Ep_epTp(epDesc[i]);
            printk(screen_log, "hw: usb hid: device %p ep %d: id:%d tp:%d\n", usbDev, i, epId, epTp);
            // write addflags of ctrl context
            bit_set1_32(&usbDev->ctxFlag, epId);
            hw_usb_xhci_writeCtx(inCtx, 1, (1u << epId), 1);

            // write slot context
            hw_usb_xhci_writeCtx(slotCtx, 0, hw_usb_xhci_SlotCtx_ctxEntries, 
                max(hw_usb_xhci_readCtx(slotCtx, 0, hw_usb_xhci_SlotCtx_ctxEntries), epId));

            void *epCtx = hw_usb_xhci_getCtxEntry(usbDev->host, inCtx, epId + 1);
            hw_usb_xhci_writeCtx(epCtx, 0, hw_usb_xhci_EpCtx_interval, hw_usb_xhci_EpCtx_calcInterval(usbDev, epTp, epDesc[i]->bInterval));
            hw_usb_xhci_writeCtx(epCtx, 1, hw_usb_xhci_EpCtx_epTp, epTp);
            hw_usb_xhci_writeCtx(epCtx, 1, hw_usb_xhci_EpCtx_CErr, 3);
            hw_usb_xhci_writeCtx(epCtx, 1, hw_usb_xhci_EpCtx_mxPackSize, epDesc[i]->wMxPktSz & 0x07ff);
            hw_usb_xhci_writeCtx(epCtx, 1, hw_usb_xhci_EpCtx_mxBurstSize, (epDesc[i]->wMxPktSz & 0x1800) >> 11);
            
            usbDev->epRing[epId] = hw_usb_xhci_allocRing(usbDev->host, hw_usb_xhci_Ring_mxSz);
            hw_usb_xhci_writeCtx64(epCtx, 2, mm_dmas_virt2Phys(usbDev->epRing[epId]->trbs));

            hw_usb_xhci_writeCtx(epCtx, 2, hw_usb_xhci_EpCtx_dcs, 1);
            
            hw_usb_xhci_EpCtx_writeESITPay(epCtx, hw_usb_xhci_readCtx(epCtx, 1, hw_usb_xhci_EpCtx_mxPackSize) * (hw_usb_xhci_readCtx(epCtx, 1, hw_usb_xhci_EpCtx_mxBurstSize) + 1));

            printk(screen_log, "hw: usb hid: device %p ep %d : interval=%d epTp=%d CErr=%d mxPkSz=%d mxBurstSz=%d mxESITPay=%ld\n", 
                   usbDev, epId, 
                   hw_usb_xhci_readCtx(epCtx, 0, hw_usb_xhci_EpCtx_interval), 
                   hw_usb_xhci_readCtx(epCtx, 1, hw_usb_xhci_EpCtx_epTp), 
                   hw_usb_xhci_readCtx(epCtx, 1, hw_usb_xhci_EpCtx_CErr), 
                   hw_usb_xhci_readCtx(epCtx, 1, hw_usb_xhci_EpCtx_mxPackSize), 
                   hw_usb_xhci_readCtx(epCtx, 1, hw_usb_xhci_EpCtx_mxBurstSize),
                   hw_usb_xhci_EpCtx_readESITPay(epCtx));
        }
    }
    
    // set "configure endpoint command"
    hw_usb_xhci_Request *req = hw_usb_xhci_makeRequest(1, hw_usb_xhci_Request_flags_Command | hw_usb_xhci_Request_flags_Abort), 
        *req2 = hw_usb_xhci_makeRequest(2, hw_usb_xhci_Request_flags_Abort);
    if (req == NULL || req2 == NULL) {
        printk(screen_err, "hw: usb hid: device %p failed to allocate request\n", dev);
        if (req) hw_usb_xhci_freeRequest(req);
        if (req2) hw_usb_xhci_freeRequest(req2);
        return res_FAIL;
    }
    hw_usb_xhci_TRB_setData(&req->input[0], mm_dmas_virt2Phys(usbDev->inCtx));
    hw_usb_xhci_TRB_setSlot(&req->input[0], usbDev->slotId);
    hw_usb_xhci_TRB_setTp(&req->input[0], hw_usb_xhci_TRB_Tp_CfgEp);

    hw_usb_xhci_request(usbDev->host, usbDev->host->cmdRing, req, NULL, 0);
    if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
        printk(screen_err, "hw: usb hid: device %p failed to configure endpoint\n", dev);
        goto Fail;
    }
    printk(screen_succ, "hw: usb hid: device %p configured endpoint\n", dev);

    hw_usb_xhci_mkCtrlReq(req2, hw_usb_xhci_mkCtrlReqSetup(0x00, 0x09, usbDev->cfgDesc[0]->bCfgVal, 0, 0), hw_usb_xhci_TRB_ctrl_dir_out);
    hw_usb_xhci_request(usbDev->host, usbDev->epRing[hw_usb_xhci_DevCtx_CtrlEp], req2, usbDev, hw_usb_xhci_DbReq_make(hw_usb_xhci_DevCtx_CtrlEp, 0));
    if (hw_usb_xhci_TRB_getCmplCode(&req2->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
        printk(screen_err, "hw: usb hid: device %p failed to set configuration\n", dev);
        goto Fail;
    }
    printk(screen_succ, "hw: usb hid: device %p set configuration\n", dev);

    hw_usb_xhci_freeRequest(req2);
    req2 = hw_usb_xhci_makeRequest(3, hw_usb_xhci_Request_flags_Abort);

    u8 *report = NULL; u32 reportSz = 0;
    // get the first report descriptor
    for (int i = 0; i < hidDesc->bNumDesc; i++) {
        if (hidDesc->desc[i].bDescTp == hw_usb_devdesc_Tp_Report) {
            reportSz = hidDesc->desc[i].wDescLen;
            report = mm_kmalloc(reportSz, mm_Attr_Shared, NULL);
            if (report == NULL) {
                printk(screen_err, "hw: usb hid: device %p failed to allocate report descriptor\n", dev);
                goto Fail;
            }
            // read report descriptor
            hw_usb_xhci_mkCtrlDataReq(req2,
                hw_usb_xhci_mkCtrlReqSetup(0x81, 0x06, 0x2200, interDesc->bIntrNum, reportSz), 
                hw_usb_xhci_TRB_ctrl_dir_in, 
                report, reportSz);
            hw_usb_xhci_request(usbDev->host, usbDev->epRing[hw_usb_xhci_DevCtx_CtrlEp], req2, usbDev, hw_usb_xhci_DbReq_make(hw_usb_xhci_DevCtx_CtrlEp, 0));
            if (hw_usb_xhci_TRB_getCmplCode(&req2->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
                printk(screen_err, "hw: usb hid: device %p failed to read report descriptor\n", dev);
                goto Fail;
            }
            printk(screen_succ, "hw: usb hid: device %p read report descriptor, size %d\n", dev, reportSz);
            goto FoundReport;
        }
    }
    printk(screen_err, "hw: usb hid: device %p has no report descriptor\n", dev);
    goto Fail;
    FoundReport:
    // parse report
    hw_hid_Parser *parser = hw_hid_getParser(dev, 1);
    if (parser == NULL) {
        printk(screen_err, "hw: usb hid: device %p failed to allocate parser\n", dev);
        goto Fail;
    }

    if (hw_hid_parse(report, reportSz, parser) == res_FAIL) {
        printk(screen_err, "hw: usb hid: device %p failed to parse report descriptor\n", dev);
        hw_hid_delParser(parser);
        mm_kfree(report, mm_Attr_Shared);
        goto Fail;
    }

    mm_kfree(report, mm_Attr_Shared);

    hw_hid_printParser(parser);

    // decide parser type
    if (interDesc->bInterSubCls == 0x01)
        parser->tp = interDesc->bInterProtocol;
    else {
        printk(screen_err, "hw: usb hid: device %p unsupported custom device.\n", dev);
        hw_hid_delParser(parser);
        goto Fail;
    }
    usbDev->inter = interDesc;

    return res_SUCC;

    Fail:
    hw_usb_xhci_freeRequest(req);
    hw_usb_xhci_freeRequest(req2);
    return res_FAIL;
}

int hw_usb_hid_install(hw_Device *dev) {
    hw_usb_xhci_Device *usbDev = container(dev, hw_usb_xhci_Device, device);
    hw_hid_Parser *parser = hw_hid_getParser(dev, 0);
    if (parser == NULL) {
        printk(screen_err, "hw: usb hid: faileed to find parser for device %p.\n", dev);
        return res_FAIL;
    }
    // set_protocol & set_idle
    hw_usb_xhci_Request *req = hw_usb_xhci_makeRequest(2, hw_usb_xhci_Request_flags_Abort);
    hw_usb_xhci_mkCtrlReq(req, hw_usb_xhci_mkCtrlReqSetup(0x21, 0x0b, 1, usbDev->inter->bIntrNum, 0), hw_usb_xhci_TRB_ctrl_dir_out);
    hw_usb_xhci_request(usbDev->host, usbDev->epRing[hw_usb_xhci_DevCtx_CtrlEp], req, usbDev, hw_usb_xhci_DbReq_make(hw_usb_xhci_DevCtx_CtrlEp, 0));
    if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
        printk(screen_err, "hw: usb hid: device %p failed on SET_PROTOCOL request\n", usbDev);
        return res_FAIL;
    }
    
    hw_usb_xhci_mkCtrlReq(req, hw_usb_xhci_mkCtrlReqSetup(0x21, 0x0a, 1, usbDev->inter->bIntrNum, 0), hw_usb_xhci_TRB_ctrl_dir_out);
    hw_usb_xhci_request(usbDev->host, usbDev->epRing[hw_usb_xhci_DevCtx_CtrlEp], req, usbDev, hw_usb_xhci_DbReq_make(hw_usb_xhci_DevCtx_CtrlEp, 0));
    if (hw_usb_xhci_TRB_getCmplCode(&req->res) != hw_usb_xhci_TRB_CmplCode_Succ) {
        printk(screen_err, "hw: usb hid: device %p failed on SET_IDLE request\n", usbDev);
        return res_FAIL;
    }
    // get endpoints
    int inEp = -1, outEp = -1;
    for (hw_usb_devdesc_Hdr *hdr = hw_usb_devdesc_getNxt(usbDev->cfgDesc[0], &usbDev->inter->hdr);
            hdr != NULL && hdr->tp != hw_usb_devdesc_Tp_Inter;
            hdr = hw_usb_devdesc_getNxt(usbDev->cfgDesc[0], hdr)) {
        if (hdr->tp == hw_usb_devdesc_Tp_Ep) {
            hw_usb_devdesc_Ep *ep = container(hdr, hw_usb_devdesc_Ep, hdr);
            int epId = hw_usb_devdesc_Ep_epId(ep);
            if (epId & 1) inEp = epId;
            else outEp = epId;
        }
    }
    printk(screen_log, "hw: usb hid: device %p inEp:%d outEp:%d\n", usbDev, inEp, outEp);
    hw_usb_xhci_freeRequest(req);
    switch (parser->tp) {
        case hw_hid_Parser_type_Keyboard :
            hw_usb_hid_keyboard(usbDev, parser, inEp, outEp);
            break;
        case hw_hid_Parser_type_Mouse :
            hw_usb_hid_mouse(usbDev, parser, inEp);
    }
    return res_SUCC;
}

int hw_usb_hid_uninstall() {
    return 0;
}

void hw_usb_hid_init() {
    hw_driver_initDriver(   &hw_usb_hid_driver.drv, 
                            "hw: usb hid",
                            hw_usb_hid_check, 
                            hw_usb_hid_config,
                            hw_usb_hid_install, 
                            hw_usb_hid_uninstall);
    hw_driver_register(&hw_usb_hid_driver.drv);
}

