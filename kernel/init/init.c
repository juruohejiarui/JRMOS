#include <init/init.h>
#include <mm/mm.h>
#include <fs/vfs/api.h>
#include <fs/api.h>
#include <hardware/pci.h>
#include <hardware/diskdev.h>
#include <hardware/gpu.h>
#include <hardware/usb/xhci/api.h>
#include <screen/screen.h>

Atomic cnt;

void init_testDir(u8 *path) {
	fs_vfs_Entry *entry;
	fs_vfs_Dir *dir;

	{
		u16 buf[fs_vfs_maxPathLen];
		u16 len = toStr16(path, buf);
		printk(screen_log, "fs: test path: %S, len=%d\n", buf, len);
		entry = fs_vfs_lookup(buf);
		dir = fs_vfs_openDir(entry);
	}
	
	while ((entry = dir->api->nxt(dir))) {
		printk(screen_log, "%S\n", entry->path);
	}
	Atomic_inc(&cnt);
}

void init_testFile(u8 *path) {
	fs_vfs_Entry *entry;
	fs_vfs_File *file;

	{
		u16 buf[fs_vfs_maxPathLen];
		u16 len = toStr16(path, buf);
		printk(screen_log, "fs: test path: %S, len=%d\n", buf, len);
		entry = fs_vfs_lookup(buf);
		file = fs_vfs_openFile(entry);
	}

	u8 buf[32];
	memset(buf, 0, sizeof(buf));
	u64 off = fs_vfs_seek(file, -15, fs_vfs_FileAPI_seek_base_End);
	printk(screen_log, "seek off=%lx\n", off);
	u64 bytes = fs_vfs_read(file, buf, 32);
	printk(screen_log, "read %d byte(s) from file %s:\n%s\n", bytes, path, buf + 1);
	off = fs_vfs_seek(file, -19, fs_vfs_FileAPI_seek_base_Cur);
	printk(screen_log, "seek off=%lx\n", off);
	bytes = fs_vfs_read(file, buf, 32);
	printk(screen_log, "read %d byte(s) from file %s:\n%s\n", bytes, path, buf + 1);
	off = fs_vfs_seek(file, 1000, fs_vfs_FileAPI_seek_base_Cur);
	printk(screen_log, "seek off=%lx\n", off);

	Atomic_inc(&cnt);
}

void init_testFs() {
	task_newSubTask(init_testDir, (u64)"AA", task_attr_Builtin);
	task_newSubTask(init_testFile, (u64)"AA/test/test.txt", task_attr_Builtin);
	task_newSubTask(init_testFile, (u64)"AA/EFI/BOOT/test1.txt", task_attr_Builtin);

	while (cnt.value != 3);
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

	mm_dbg(NULL);
	task_newTask(init_testFs, (u64)NULL, task_attr_Builtin);

	

	while (1) {
		mm_dbg("\r");
		task_sche_yield();
	}
}