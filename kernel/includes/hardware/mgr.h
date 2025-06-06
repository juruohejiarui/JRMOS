#ifndef __HARDWARE_MGR_H__
#define __HARDWARE_MGR_H__

#include <lib/dtypes.h>
#include <lib/list.h>

typedef struct hw_Device {

    struct hw_Device *parent;
    struct hw_Driver *drv;
    ListNode lst;
} hw_Device;
typedef struct hw_Driver {
    int (*check)(hw_Device *dev);
    // use this drvier to initialize the device
    int (*cfg)(hw_Device *dev);
    int (*install)(hw_Device *dev);
    int (*uninstall)(hw_Device *dev);
    SafeList devLst;
    ListNode lst;

    char *name;
} hw_Driver;

void hw_driver_initDriver(hw_Driver *drv, char *name, 
    int (*check)(hw_Device *), int (*init)(hw_Device *), int (*install)(hw_Device *), int (*uninstall)(hw_Device *));

void hw_driver_init();

// Register a driver, adding it to the driver list
void hw_driver_register(hw_Driver *drv);

// Unregister a driver, removing it from the driver list
// uninstalls all devices using this driver
void hw_driver_unregister(hw_Driver *drv);

extern SafeList hw_drvLst;

#endif