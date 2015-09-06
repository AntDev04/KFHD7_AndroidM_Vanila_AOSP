/*
 * Copyright (c) 2010, The Android Open Source Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Neither the name of The Android Open Source Project nor the names
 *    of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <common.h>
#include <mmc.h>
#include <fastboot.h>
static int do_format(void);

#include <flash_part.h>

static void init_mbr(u8 *mbr, u32 blocks)
{
	mbr[0x1be] = 0x00; // nonbootable
	mbr[0x1bf] = 0xFF; // bogus CHS
	mbr[0x1c0] = 0xFF;
	mbr[0x1c1] = 0xFF;

	mbr[0x1c2] = 0xEE; // GPT partition
	mbr[0x1c3] = 0xFF; // bogus CHS
	mbr[0x1c4] = 0xFF;
	mbr[0x1c5] = 0xFF;

	mbr[0x1c6] = 0x01; // start
	mbr[0x1c7] = 0x00;
	mbr[0x1c8] = 0x00;
	mbr[0x1c9] = 0x00;

	memcpy(mbr + 0x1ca, &blocks, sizeof(u32));

	mbr[0x1fe] = 0x55;
	mbr[0x1ff] = 0xaa;
}

static void start_ptbl(struct ptable *ptbl, unsigned blocks)
{
	struct efi_header *hdr = &ptbl->header;

	memset(ptbl, 0, sizeof(*ptbl));

	init_mbr(ptbl->mbr, blocks - 1);

	memcpy(hdr->magic, "EFI PART", 8);
	hdr->version = EFI_VERSION;
	hdr->header_sz = sizeof(struct efi_header);
	hdr->header_lba = 1;
	hdr->backup_lba = blocks - 1;
	hdr->first_lba = 34;
	hdr->last_lba = blocks - 1;
	memcpy(hdr->volume_uuid, random_uuid, 16);
	hdr->entries_lba = 2;
	hdr->entries_count = EFI_ENTRIES;
	hdr->entries_size = sizeof(struct efi_entry);
}

static void end_ptbl(struct ptable *ptbl)
{
	struct efi_header *hdr = &ptbl->header;
	u32 n;

	n = crc32(0, 0, 0);
	n = crc32(n, (void*) ptbl->entry, sizeof(ptbl->entry));
	hdr->entries_crc32 = n;

	n = crc32(0, 0, 0);
	n = crc32(0, (void*) &ptbl->header, sizeof(ptbl->header));
	hdr->crc32 = n;
}

int add_ptn(struct ptable *ptbl, u64 first, u64 last, const char *name)
{
	struct efi_header *hdr = &ptbl->header;
	struct efi_entry *entry = ptbl->entry;
	unsigned n;

	if (first < 34) {
		printf("partition '%s' overlaps partition table\n", name);
		return -1;
	}

	if (last > hdr->last_lba) {
		printf("partition '%s' does not fit\n", name);
		return -1;
	}
	for (n = 0; n < EFI_ENTRIES; n++, entry++) {
		if (entry->last_lba)
			continue;
		memcpy(entry->type_uuid, partition_type, 16);
		memcpy(entry->uniq_uuid, random_uuid, 16);
		entry->uniq_uuid[0] = n;
		entry->first_lba = first;
		entry->last_lba = last;
		for (n = 0; (n < EFI_NAMELEN) && *name; n++)
			entry->name[n] = *name++;
		return 0;
	}
	printf("out of partition table entries\n");
	return -1;
}

void import_efi_partition(struct efi_entry *entry)
{
	struct fastboot_ptentry e;
	int n;
	unsigned int p_size;
	if (memcmp(entry->type_uuid, partition_type, sizeof(partition_type)))
		return;
	for (n = 0; n < (sizeof(e.name)-1); n++)
		e.name[n] = entry->name[n];
	e.name[n] = 0;
	e.start = entry->first_lba;
	e.length = (entry->last_lba - entry->first_lba + 1) * 512;
	p_size = (entry->last_lba - entry->first_lba + 1) >>11;
	e.flags = 0;

	if (!strcmp(e.name,"environment"))
		e.flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_ENV;
	fastboot_flash_add_ptn(&e);

	if (e.length > 0x100000)
		printf("%8d %7ldM %s\n", e.start, p_size, e.name);
	else
		printf("%8d %7dK %s\n", e.start, (int)e.length/0x400, e.name);
}

static int load_ptbl(void)
{
	static unsigned char data[512];
	static struct efi_entry entry[4];
	int n,m,r;
	r = mmc_read(1, 1, data, 512);
	if (r != 1) {
		printf("error reading partition table\n");
		return -1;
	}
	if (memcmp(data, "EFI PART", 8)) {
		printf("efi partition table not found\n");
		printf("===============================\n");
		printf("            format             \n");
		printf("===============================\n");
		do_format();
		return -1;
	}
	for (n = 0; n < (128/4); n++) {
		r = mmc_read(1, 1 + n, (void*) entry, 512);
		if (r != 1) {
			printf("partition read failed\n");
			return 1;
		}
		for (m = 0; m < 4; m ++)
			import_efi_partition(entry + m);
	}
	return 0;
}

/* use the partition table form otter */
static struct partition partitions[] = {
        { "-",          128 },
        { "xloader",    128 },
        { "bootloader", 256 },
        { "idme",       64  },         /* 64K for idme */
        { "crypto",     16  },         /* 16K for crypto */
        { "misc",       2   },
        { "-",          430 },         /* <- 1 MB */
        { "dkernel",    10*1024 },
        { "dfs",        192*1024 },
        { "efs",        16*1024  },
        { "recovery",   8*1024 },
        { "boot",       8*1024 },
        { "system",     886*1024 },
        { "cache",      650*1024},
        { "userdata",   0 },
        { 0, 0 },
};

static struct ptable the_ptable;

static int do_format(void)
{
	struct ptable *ptbl = &the_ptable;
	unsigned sector_sz, blocks;
	unsigned next;
	int n;

	if (mmc_init(1)) {
		printf("mmc init failed?\n");
		return -1;
	}

	mmc_info(1, &sector_sz, &blocks);
	printf("blocks %d\n", blocks);

	start_ptbl(ptbl, blocks);
	n = 0;
	next = 0;
	for (n = 0, next = 0; partitions[n].name; n++) {
		unsigned sz = partitions[n].size_kb * 2;
		if (!strcmp(partitions[n].name,"-")) {
			next += sz;
			continue;
		}
		if (sz == 0)
			sz = blocks - next;
		if (add_ptn(ptbl, next, next + sz - 1, partitions[n].name))
			return -1;
		next += sz;
	}
	end_ptbl(ptbl);

	fastboot_flash_reset_ptn();
	if (mmc_write(1, (void*) ptbl, 0, sizeof(struct ptable)) != 1)
		return -1;

	printf("\nnew partition table:\n");
	load_ptbl();

	return 0;
}

int fastboot_oem(const char *cmd)
{
	if (!strcmp(cmd,"format"))
		return do_format();
	return -1;
}

void board_mmc_init(void)
{
        /* nothing to do this early */
}

#ifdef CONFIG_ENABLE_IDME
extern int idme_update_vars_ex(void);
#endif


int board_late_init(void)
{
        int ret = 0;
	if (mmc_init(1)) {
		printf("mmc init failed?\n");
		return 1;
	}
	printf("\nefi partition table:\n");
#ifdef CONFIG_ENABLE_IDME
        idme_update_vars_ex();
#endif
	ret = load_ptbl();

        return ret;
}


