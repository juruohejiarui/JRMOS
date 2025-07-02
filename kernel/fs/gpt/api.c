#include <fs/gpt/api.h>
#include <fs/desc.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include <lib/string.h>
#include <lib/crc32.h>

void fs_gpt_registerPar(fs_gpt_ParEntry *entry, hw_DiskDev *dev) {
	
}

void fs_gpt_scan(hw_DiskDev *dev) {
	fs_gpt_Hdr *hdr = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);
	u8 *entries = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);

	dev->read(dev, 1, 1, hdr);
	printk(WHITE, BLACK, 
		"fs: gpt: read header from disk %p\n"
		"\tsignature:	%s\n"
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
	u32 crc32 = crc32_noReflect((u8 *)hdr, hdr->hdrSz);
	hdr->hdrCrc32 = oldCrc;

	if (crc32 != hdr->hdrCrc32) {
		printk(RED, BLACK, "fs: gpt: crc32 of header does not match: true:%d value on disk:%d\n",
			crc32, oldCrc);
		return ;
	}

	fs_Disk *disk = mm_kmalloc(sizeof(fs_Disk) + sizeof(fs_gpt_ParEntry) * hdr->numOfParEntries, mm_Attr_Shared, NULL);
	
	memcpy(hdr, &disk->gptTbl.hdr, sizeof(fs_gpt_Hdr));

	i32 curParLba = 1;

	for (int i = 0; i < hdr->numOfParEntries; i++) {
		if ((curParLba - hdr->parEntryLba + 1) * hw_diskdev_lbaSz <= i * hdr->szOfParEntry) {
			curParLba++;
			dev->read(dev, curParLba, 1, entries);
		}
		fs_gpt_ParEntry *parEntry = (void *)(entries + (i - (curParLba - hdr->parEntryLba) * hw_diskdev_lbaSz / hdr->szOfParEntry) * hdr->szOfParEntry); 

		if (fs_gpt_matchGuid(parEntry->uniParGuid, fs_gpt_parTypeGuid_None, fs_gpt_parTypeGuid_None))
			continue;
		
		printk(WHITE, BLACK, 
				"\t#%d: \n"
				"\t\tType:    %016lx %016lx\n"
				"\t\tParGUID: %016lx %016lx\n"
				"\t\tstLba:   %016lx\n"
				"\t\tedLba:   %016lx\n"
				"\t\tattr:    %016lx\n"
				"\t\tparName: %s\n",
				i, 
				parEntry->parTypeGuid[0], parEntry->parTypeGuid[1],
				parEntry->uniParGuid[0], parEntry->uniParGuid[1],
				parEntry->stLba, parEntry->edLba, parEntry->attr,
				parEntry->parName);
		if (fs_gpt_matchGuid(parEntry->parTypeGuid, fs_gpt_parTypeGuid_EfiSysPar0, fs_gpt_parTypeGuid_EfiSysPar1))
			printk(WHITE, BLACK, "\t\tEFI System\n");
		else if (fs_gpt_matchGuid(parEntry->parTypeGuid, fs_gpt_parTypeGuid_LinuxFSDt0, fs_gpt_parTypeGuid_LinuxFSDt1))
			printk(WHITE, BLACK, "\t\tLinux Filesystem\n");

		memcpy(parEntry, &disk->gptTbl.entry[i], sizeof(fs_gpt_ParEntry));
	}
	mm_kfree(entries, mm_Attr_Shared);
	mm_kfree(hdr, mm_Attr_Shared);

	disk->diskDev = dev;
}