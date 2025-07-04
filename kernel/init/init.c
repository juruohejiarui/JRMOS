#include <init/init.h>
#include <hardware/pci.h>
#include <hardware/diskdev.h>
#include <hardware/gpu.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>

void init_init() {	
	hw_driver_init();

	hw_DiskDev_initMgr();

	if (hw_pci_init() == res_FAIL) {
		printk(screen_err, "init: failed to initialize pci.\n");
		while (1) ;
	}
	if (hw_driver_registerBuiltin() == res_FAIL) {
		printk(screen_err, "init: failed to register builtin drivers.\n");
		while (1) ;
	}

	hw_pci_lstDev();

	hw_pci_chk();
}