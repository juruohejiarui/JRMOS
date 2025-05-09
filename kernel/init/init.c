#include <init/init.h>
#include <hardware/pci.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>

void init_init() {
	if (hw_pci_init() == res_FAIL) {
		printk(RED, BLACK, "init: failed to initialize pci.\n");
		while (1) ;
	}
	if (hw_usb_xhci_init() == res_FAIL) {
		printk(RED, BLACK, "init: failed to initialize xhci\n");
		while (1) ;
	}
}