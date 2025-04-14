#include <init/init.h>
#include <hardware/pci.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>

void init_init() {
	if (hw_pci_init() == res_FAIL) {
		printk(RED, BLACK, "init: failed to initialize pci.\n");
		while (1) ;
	}
	if (hw_usb_xhci_searchInPci() == res_FAIL) {
		printk(RED, BLACK, "init: failed to search xhci in pci.\n");
		while (1) ;
	}
}