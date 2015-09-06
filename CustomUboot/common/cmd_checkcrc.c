/*
 * cmd_checkcrc.c 
 *
 * Copyright 2011 Amazon Technologies, Inc. All Rights Reserved.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <command.h>
#include <mmc.h>

#include <flash_part.h>

#ifdef CONFIG_CMD_CHECKCRC

struct crc_table_t {
	u32 size;
	u32 crc;
};

int mmc_crc32_read (u32 src, uchar *dst, u32 size)
{

	debug("read src(mmc)=0x%08x, dst=0x%08x, size=0x%08x\n", src, (u32)dst, size);
	mmc_read(CONFIG_MMC_BOOTFLASH, src, dst, size);

	return 0;
}

int mmc_crc32_write (uchar *src, u32 dst, u32 size)
{

	debug("write src=0x%08x, dst(mmc)=0x%08x, size=0x%08x\n", (u32)src, dst, size);
	mmc_write(CONFIG_MMC_BOOTFLASH, src, dst, size); 

	return 0;
}

int mmc_crc32_test(int start, int size, uint crc, int read_mmc_crc)
{

	uint checkaddr;
	int i, runsize, checksize;
	struct crc_table_t crc_table;
	uint32_t crc_cac = ~0U; /* initial */

	checkaddr = start;
	checksize = size;

	debug("checkaddr=0x%08x, checksize=0x%08x\n", checkaddr, checksize);

	if (read_mmc_crc) {
		/* read the CRC embedded in image */
		mmc_crc32_read(checkaddr + checksize - 512, (unsigned char*)CRC32_BUFFER, 512); //mmc->read_bl_len
		memcpy(&crc_table, (unsigned char*)CRC32_BUFFER + 512 - (2 * sizeof(u32)), 2 * sizeof(u32));
		size = crc_table.size;  	/* actual image size */
		crc = crc_table.crc;
	}

	debug("part=0x%08x, start=0x%08x, size=0x%08x, crc=0x%08x\n", part, start, size, crc);

	if (start != checkaddr || (unsigned)size > checksize) {
		printf("ERROR: section parameters incorrect\n");
		return -1;
	}

	runsize = size;
	if (size > CRC32_BUFFER_SIZE)
		runsize = CRC32_BUFFER_SIZE;  

	debug("start=0x%08x, runsize=0x%08x\n", start, runsize);
	mmc_crc32_read(start, (unsigned char*)CRC32_BUFFER, runsize); 

	crc_cac=crc32_no_comp(crc_cac, (unsigned char*)CRC32_BUFFER, runsize);  
	debug("runsize=0x%08x, crc(cac)=0x%08x\n", runsize, crc_cac);
	printf(".");

	/* allow image size bigger than buffer size */
	for (i = CRC32_BUFFER_SIZE; i < (size - CRC32_BUFFER_SIZE); i += CRC32_BUFFER_SIZE) {
		mmc_crc32_read(start + i, (unsigned char*)CRC32_BUFFER, CRC32_BUFFER_SIZE); 
		crc_cac=crc32_no_comp(crc_cac,(unsigned char*)CRC32_BUFFER, CRC32_BUFFER_SIZE);  
		debug("runleft=0x%08x, crc(cac)=0x%08x\n", size - i, crc_cac);
		printf(".");
	}

	runsize  = size - i;
	if (runsize  > 0) {
		/*  the remaining of the data which is less than the buffer size */
		mmc_crc32_read(start + i, (unsigned char*)CRC32_BUFFER, runsize); 
		crc_cac=crc32_no_comp(crc_cac, (unsigned char*)CRC32_BUFFER, runsize);  
		debug("runsize=0x%08x, crc(cac)=0x%08x\n", runsize, crc_cac);
		printf(".");
	}

	crc_cac=~crc_cac; /* we invert it at the end */ 
	debug("size=0x%08x, crc(cac)=0x%08x\n", size, crc_cac);

	if (crc == crc_cac) {
		printf("PASS\n");
		debug("start=0x%08x, size=0x%08x, crc(mmc)=0x%08x\n", start, size, crc);

		crc_table.crc = crc;
		crc_table.size = size;
		memset((unsigned char*)CRC32_BUFFER, 0xFF, 512);
		memcpy((unsigned char*)CRC32_BUFFER + 512 - (2 * sizeof(u32)), &crc_table, 2 * sizeof(u32));
		mmc_crc32_write((unsigned char*)CRC32_BUFFER, checkaddr + checksize - 512, 512); //mmc->read_bl_len
		return 0;
	} else {
		printf("FAIL\n");
		debug("start=0x%08x, checksize=0x%08x, crc(mmc)=0x%08x, crc(cac)=0x%08x\n",
		      start, size, crc, crc_cac);
		return -1;
	}

}

int do_check_crc (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int size, i;
	uint start, crc;
	static unsigned char data[512];
	static struct efi_entry entry[4];  // mmc_read 4 entry per read
	int n, m, r;
	int partition_found = 0;
	debug("argc=[%d], argv=[0,%s],[1,%s],[2,%s],[3,%s],flag=0x%08x\n",
	      argc, argv[0], argv[1], argv[2], argv[3], flag);

	if (argc != 4){
		printf("usage: check image start size crc\n");
		return -1;
	}

	r = mmc_read(1, 1, data, 512);
	if (r != 1){
		printf("error reading partition table\n");
		return -1;
	}
	if (memcmp(data, "EFI PART", 8)) {
		printf("efi partition table not found\n");
		return -1;
	}

	/* walk through the parition table */
	for (n = 0; n < (128/4) && !partition_found; n++) {
		r = mmc_read(1, 1 + n, (void*) entry, 512);
		if (r != 1) {
			printf("partition read failed\n");
			return 1;
		}
		for (m = 0; m < 4; m ++){
			printf("%s: 0x%x -- 0x%x\n", entry[m].name, entry[m].first_lba,
			       entry[m].last_lba);
			if (strcmp(entry[m].name, argv[1]) == 0){
				printf("partition match found\n");
				partition_found = 1;
				break;
			}
			continue;
		}
	}
    
	if (!partition_found){
		printf("partition: %s is  not found!\n", argv[1]);
		return -1;
	}
  
	/* compute the size */
	size = (uint)simple_strtoul(argv[2], NULL, 16);
	if ((entry[m].last_lba - entry[m].first_lba) < size) {
		printf("error, size is exceeds the partition size\n");
		return -1;
	}

	crc = (uint)simple_strtoul(argv[3], NULL, 16);

	i = 0;
	if (crc == 1 && size == 1) {
		/* using the partition size first */
		size = entry[m].last_lba - entry[m].first_lba;
		i = 1;  			/* read the crc from mmc */
	}
  
	return mmc_crc32_test(start, size, crc, i);

}
/***************************************************/

U_BOOT_CMD(
	   check,5,0,do_check_crc,
	   "perform MMC CRC32 check",
	   "image size crc\n"
	   "    - images:\n"
	   "    - bootloader comes from u-boot.bin\n"
	   "    - kernel comes from uImage\n"
	   "    - system comes from rootfs.img"
	   );

#endif /* CONFIG_CMD_CHECKCRC */
