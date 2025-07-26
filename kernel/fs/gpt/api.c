#include <fs/gpt/api.h>
#include <fs/fat32/api.h>
#include <fs/api.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include <lib/string.h>
#include <lib/crc32.h>

int fs_gpt_registerPar_efiSys(fs_gpt_Disk *disk, u32 idx, fs_gpt_ParEntry *entry) {
	fs_fat32_BootSector *bs = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);
	disk->disk.device->read(disk->disk.device, entry->stLba, 1, bs);
	printk(screen_log, 
		"\t\toemName:     %s\n"
		"\t\tbytesPerSec: %d\n"
		"\t\tfatSz16:     %d\n"
		"\t\tfatSz32:     %d\n"
		"\t\tbootSigEnd:  %x\n",
		bs->oemName, bs->bytesPerSec, bs->fatSz16, bs->fatSz32, bs->bootSigEnd);
	if (fs_fat32_chk(bs) != res_SUCC) {
		printk(screen_err, "fs: gpt: %p partition %d is not a valid FAT32 partition\n", disk, idx);
		mm_kfree(bs, mm_Attr_Shared);
		return res_FAIL;
	}
	// create fat32 partition manager
	fs_fat32_Partition *par = mm_kmalloc(sizeof(fs_fat32_Partition), mm_Attr_Shared, NULL);
	if (fs_fat32_initPar(par, bs, &disk->disk, entry->stLba, entry->edLba) == res_FAIL) {
		printk(screen_err, "fs: gpt: %p partition %d: failed to initialize.\n", disk, idx);
		mm_kfree(bs, mm_Attr_Shared);
		mm_kfree(par, mm_Attr_Shared);
		return res_FAIL;
	}
	mm_kfree(bs, mm_Attr_Shared);
	return res_SUCC;
}

int fs_gpt_registerPar(fs_gpt_Disk *disk, u32 idx) {
	fs_gpt_ParEntry *entry = &disk->entries[idx];
	// try to determine the file system
	if (fs_gpt_matchGuid(entry->parTypeGuid, fs_gpt_parTypeGuid_EfiSysPar0, fs_gpt_parTypeGuid_EfiSysPar1)) {
		printk(screen_log, "\t\tEFI System\n");
		return fs_gpt_registerPar_efiSys(disk, idx, entry);
	} else if (fs_gpt_matchGuid(entry->parTypeGuid, fs_gpt_parTypeGuid_LinuxFSDt0, fs_gpt_parTypeGuid_LinuxFSDt1))
		printk(screen_log, "\t\tLinux Filesystem\n");
	else if (fs_gpt_matchGuid(entry->parTypeGuid, fs_gpt_parTypeGuid_MsBasicDt0, fs_gpt_parTypeGuid_MsBasicDt1))
		printk(screen_log, "\t\tMicrosoft Basic Data\n");
}

void fs_gpt_scan(hw_DiskDev *dev) {
	fs_gpt_Hdr *hdr = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);
	u8 *entries = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);

	dev->read(dev, 1, 1, hdr);
	printk(screen_log, 
		"fs: gpt: read header from disk %p\n"
		"\tsignature:%s\n"
		"\trevision: %x\n"
		"\thdrSz:    %d\n"
		"\thdrCrc32: %d\n"
		"\tmyLba:    %016lx\n"
		"\talterLba: %016lx\n"
		"\tfirUsableLba: %016lx\n"
		"\tlstUsableLba: %016lx\n"
		"\tdiskGuid:     %016lx %016lx\n"
		"\tparEntryLba:  %016lx\n"
		"\tnumOfParEntries: %d\n"
		"\tszOfParEntry:    %d\n"
		"\tparEntryArrCrc32:%d\n",
		dev, 
		hdr->signature, hdr->revision, 	hdr->hdrSz, 		hdr->hdrCrc32,
		hdr->myLba, 	hdr->alterLba, 	hdr->firUsableLba, 	hdr->lstUsableLba,
		hdr->diskGUID[0], 				hdr->diskGUID[1],
		hdr->parEntryLba, 				hdr->numOfParEntries, 
		hdr->szOfParEntry,              hdr->parEntryArrCrc32);
	u32 oldCrc = hdr->hdrCrc32;
	hdr->hdrCrc32 = 0;
	u32 crc32 = crc32_ieee802((u8 *)hdr, hdr->hdrSz);
	hdr->hdrCrc32 = oldCrc;

	if (memcmp(hdr->signature, "EFI PART", 8) != 0) {
		printk(screen_err, "fs: gpt: invalid signature: %s\n", hdr->signature);
		goto scan_Fail;

	}

	if (crc32 != hdr->hdrCrc32) {
		printk(screen_err, "fs: gpt: crc32 of header does not match: true:%d value on disk:%d\n", crc32, oldCrc);
		goto scan_Fail;
	}

	fs_gpt_Disk *disk = mm_kmalloc(sizeof(fs_gpt_Disk) + sizeof(fs_gpt_ParEntry) * hdr->numOfParEntries, mm_Attr_Shared, NULL);
	fs_Disk_init(&disk->disk, 0);
	disk->disk.sz = (hdr->lstUsableLba - hdr->firUsableLba + 1);
	
	memcpy(hdr, &disk->hdr, sizeof(fs_gpt_Hdr));

	i32 curParLba = 1;

	disk->disk.device = dev;

	for (int i = 0; i < hdr->numOfParEntries; i++) {
		if ((curParLba - hdr->parEntryLba + 1) * hw_diskdev_lbaSz <= i * hdr->szOfParEntry) {
			curParLba++;
			dev->read(dev, curParLba, 1, entries);
		}
		fs_gpt_ParEntry *parEntry = (void *)(entries + (i - (curParLba - hdr->parEntryLba) * hw_diskdev_lbaSz / hdr->szOfParEntry) * hdr->szOfParEntry); 

		if (fs_gpt_matchGuid(parEntry->uniParGuid, fs_gpt_parTypeGuid_None, fs_gpt_parTypeGuid_None))
			continue;
		
		printk(screen_log, 
				"\t#%d: \n"
				"\t\tType:    %016lx %016lx\n"
				"\t\tParGUID: %016lx %016lx\n"
				"\t\tstLba:   %016lx\n"
				"\t\tedLba:   %016lx\n"
				"\t\tattr:    %016lx\n"
				"\t\tparName: %S\n",
				i, 
				parEntry->parTypeGuid[0], parEntry->parTypeGuid[1],
				parEntry->uniParGuid[0], parEntry->uniParGuid[1],
				parEntry->stLba, parEntry->edLba, parEntry->attr,
				parEntry->parName);
		

		memcpy(parEntry, &disk->entries[i], sizeof(fs_gpt_ParEntry));
		if (fs_gpt_registerPar(disk, i) != res_SUCC) {
			printk(screen_err, "fs: gpt: failed to register partition %d\n", i);
			goto scan_Fail;
		}
	}


	scan_Fail:
	mm_kfree(entries, mm_Attr_Shared);
	mm_kfree(hdr, mm_Attr_Shared);
}