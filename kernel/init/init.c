#include <init/init.h>
#include <fs/vfs/api.h>
#include <fs/api.h>
#include <hardware/pci.h>
#include <hardware/diskdev.h>
#include <hardware/gpu.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>

void init_testFs(u8 *path) {
	u16 buf[fs_vfs_maxPathLen];
	u16 len = toStr16(path, buf);
	printk(screen_log, "fs: test path: %S, len=%d\n", buf, len);
	fs_vfs_Entry *entry = fs_vfs_lookup(buf);
	fs_vfs_Dir *dir = fs_vfs_openDir(entry);
	while ((entry = dir->api->nxt(dir))) {
		printk(screen_log, "%S\n", entry->path);
	}
}

void init_init() {	
	hw_driver_init();

	hw_DiskDev_initMgr();

	fs_init();

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

	task_newTask(init_testFs, (u64)"AA", task_attr_Builtin);
	task_newTask(init_testFs, (u64)"AA/EFI/BOOT", task_attr_Builtin);
}