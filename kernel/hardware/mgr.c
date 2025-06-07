#include <hardware/mgr.h>
#include <hardware/hid/parse.h>
#include <hardware/usb/hid/api.h>

SafeList hw_drvLst;

void hw_driver_initDriver(hw_Driver *drv, char *name, 
        int (*check)(hw_Device *), int (*cfg)(hw_Device *), int (*install)(hw_Device *), int (*uninstall)(hw_Device *)) {
    drv->check = check;
    drv->cfg = cfg;
    drv->install = install;
    drv->uninstall = uninstall;
    drv->name = name;
    SafeList_init(&drv->devLst);
}
void hw_driver_init() {
    SafeList_init(&hw_drvLst);

    // register some built-in drivers
    hw_hid_init();
    hw_usb_hid_init();
}

void hw_driver_register(hw_Driver *drv) {
    SafeList_insTail(&hw_drvLst, &drv->lst);
}

void hw_driver_unregister(hw_Driver *drv) {
    SafeList_del(&hw_drvLst, &drv->lst);
    if (drv->uninstall != NULL) {
        SafeList_enum(&drv->devLst, devNd) {
            hw_Device *dev = container(devNd, hw_Device, lst);
            drv->uninstall(dev);
        }
    }
}