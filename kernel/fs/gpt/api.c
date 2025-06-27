#include <fs/gpt/api.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include <lib/string.h>
#include <lib/crc32.h>

void fs_gpt_scan(hw_DiskDev *dev) {
	fs_gpt_Hdr *hdr = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL), *alterHdr = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);

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
	printk(WHITE, BLACK, "fs: gpt: header crc32: %d, expected: %d\n", crc32, hdr->hdrCrc32);

	u32 entrySz = hdr->szOfParEntry;

	// read alterHdr
	dev->read(dev, hdr->alterLba, 1, alterHdr);
	// compare two headers
	if (memcmp(hdr, alterHdr, sizeof(fs_gpt_Hdr)) != 0) {
		printk(RED, BLACK, "fs: gpt: header and alter header are not equal.\n");
		// printk alter header
		printk(WHITE, BLACK, 
			"fs: gpt: read alter header from disk %p\n"
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
			alterHdr->signature,	alterHdr->revision, alterHdr->hdrSz, 		alterHdr->hdrCrc32,
			alterHdr->myLba, 		alterHdr->alterLba,	alterHdr->firUsableLba, alterHdr->lstUsableLba,
			alterHdr->diskGUID[0], 	alterHdr->diskGUID[1],
			alterHdr->parEntryLba, 	alterHdr->numOfParEntries, 
			alterHdr->szOfParEntry,	alterHdr->parEntryArrCrc32);
	} else {
		printk(GREEN, BLACK, "fs: gpt: header and alter header are equal.\n");
	}

	mm_kfree(hdr, mm_Attr_Shared);
	mm_kfree(alterHdr, mm_Attr_Shared);
}