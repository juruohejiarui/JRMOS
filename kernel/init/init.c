#include <init/init.h>
#include <hardware/pci.h>
#include <hardware/gpu.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>

void init_init() {	
	hw_driver_init();
	if (hw_pci_init() == res_FAIL) {
		printk(RED, BLACK, "init: failed to initialize pci.\n");
		while (1) ;
	}
	if (hw_driver_registerBuiltin() == res_FAIL) {
		printk(RED, BLACK, "init: failed to register builtin drivers.\n");
		while (1) ;
	}

	hw_pci_lstDev();

	hw_pci_chk();
}