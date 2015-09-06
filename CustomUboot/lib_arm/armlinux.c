/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <zlib.h>
#include <asm/byteorder.h>
#ifdef CONFIG_HAS_DATAFLASH
#include <dataflash.h>
#endif
#include <bootimg.h>

DECLARE_GLOBAL_DATA_PTR;

/*cmd_boot.c*/
extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG) || \
    defined (CONFIG_VFD) || \
    defined (CONFIG_LCD)
static void setup_start_tag (bd_t *bd);

# ifdef CONFIG_SETUP_MEMORY_TAGS
static void setup_memory_tags (bd_t *bd);
# endif
static void setup_commandline_tag (bd_t *bd, char *commandline);

#ifdef CONFIG_MACADDR_TAG
void setup_macaddr_tag (struct tag **params);
#endif

#if 0
static void setup_ramdisk_tag (bd_t *bd);
#endif

# ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start,
			      ulong initrd_end);
# endif
static void setup_end_tag (bd_t *bd);

# if defined (CONFIG_VFD) || defined (CONFIG_LCD)
static void setup_videolfb_tag (gd_t *gd);
# endif


static struct tag *params;
#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */

#ifdef CONFIG_SHOW_BOOT_PROGRESS
# include <status_led.h>
# define SHOW_BOOT_PROGRESS(arg)	show_boot_progress(arg)
#else
# define SHOW_BOOT_PROGRESS(arg)
#endif

extern image_header_t header;	/* from cmd_bootm.c */


void do_bootm_linux (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[],
		     ulong addr, ulong *len_ptr, int verify)
{
	ulong len = 0, checksum;
	ulong initrd_start, initrd_end;
	ulong data;
	void (*theKernel)(int zero, int arch, uint params);
	image_header_t *hdr = &header;
	bd_t *bd = gd->bd;

#ifdef CONFIG_CMDLINE_TAG
	char *commandline = getenv ("bootargs");
#endif

	theKernel = (void (*)(int, int, uint))ntohl(hdr->ih_ep);

	/*
	 * Check if there is an initrd image
	 */
	if (argc >= 3) {
		SHOW_BOOT_PROGRESS (9);

		addr = simple_strtoul (argv[2], NULL, 16);

		printf ("## Loading Ramdisk Image at %08lx ...\n", addr);

		/* Copy header so we can blank CRC field for re-calculation */
#ifdef CONFIG_HAS_DATAFLASH
		if (addr_dataflash (addr)) {
			read_dataflash (addr, sizeof (image_header_t),
					(char *) &header);
		} else
#endif
			memcpy (&header, (char *) addr,
				sizeof (image_header_t));

		if (ntohl (hdr->ih_magic) != IH_MAGIC) {
			printf ("Bad Magic Number\n");
			SHOW_BOOT_PROGRESS (-10);
			do_reset (cmdtp, flag, argc, argv);
		}

		data = (ulong) & header;
		len = sizeof (image_header_t);

		checksum = ntohl (hdr->ih_hcrc);
		hdr->ih_hcrc = 0;

		if (crc32 (0, (unsigned char *) data, len) != checksum) {
			printf ("Bad Header Checksum\n");
			SHOW_BOOT_PROGRESS (-11);
			do_reset (cmdtp, flag, argc, argv);
		}

		SHOW_BOOT_PROGRESS (10);

		print_image_hdr (hdr);

		data = addr + sizeof (image_header_t);
		len = ntohl (hdr->ih_size);

#ifdef CONFIG_HAS_DATAFLASH
		if (addr_dataflash (addr)) {
			read_dataflash (data, len, (char *) CFG_LOAD_ADDR);
			data = CFG_LOAD_ADDR;
		}
#endif

		if (verify) {
			ulong csum = 0;

			printf ("   Verifying Checksum ... ");
			csum = crc32 (0, (unsigned char *) data, len);
			if (csum != ntohl (hdr->ih_dcrc)) {
				printf ("Bad Data CRC\n");
				SHOW_BOOT_PROGRESS (-12);
				do_reset (cmdtp, flag, argc, argv);
			}
			printf ("OK\n");
		}

		SHOW_BOOT_PROGRESS (11);

		if ((hdr->ih_os != IH_OS_LINUX) ||
		    (hdr->ih_arch != IH_CPU_ARM) ||
		    (hdr->ih_type != IH_TYPE_RAMDISK)) {
			printf ("No Linux ARM Ramdisk Image\n");
			SHOW_BOOT_PROGRESS (-13);
			do_reset (cmdtp, flag, argc, argv);
		}

#if defined(CONFIG_B2) || defined(CONFIG_EVB4510) || defined(CONFIG_ARMADILLO)
		/*
		 *we need to copy the ramdisk to SRAM to let Linux boot
		 */
		memmove ((void *) ntohl(hdr->ih_load), (uchar *)data, len);
		data = ntohl(hdr->ih_load);
#endif /* CONFIG_B2 || CONFIG_EVB4510 */

		/*
		 * Now check if we have a multifile image
		 */
	} else if ((hdr->ih_type == IH_TYPE_MULTI) && (len_ptr[1])) {
		ulong tail = ntohl (len_ptr[0]) % 4;
		int i;

		SHOW_BOOT_PROGRESS (13);

		/* skip kernel length and terminator */
		data = (ulong) (&len_ptr[2]);
		/* skip any additional image length fields */
		for (i = 1; len_ptr[i]; ++i)
			data += 4;
		/* add kernel length, and align */
		data += ntohl (len_ptr[0]);
		if (tail) {
			data += 4 - tail;
		}

		len = ntohl (len_ptr[1]);

	} else {
		/*
		 * no initrd image
		 */
		SHOW_BOOT_PROGRESS (14);

		len = data = 0;
	}

#ifdef	DEBUG
	if (!data) {
		printf ("No initrd\n");
	}
#endif

	if (data) {
		initrd_start = data;
		initrd_end = initrd_start + len;
	} else {
		initrd_start = 0;
		initrd_end = 0;
	}

	SHOW_BOOT_PROGRESS (15);

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) theKernel);

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG) || \
    defined (CONFIG_LCD) || \
    defined (CONFIG_VFD)
	setup_start_tag (bd);
#ifdef CONFIG_SERIAL_TAG
	setup_serial_tag (&params);
#endif
#ifdef CONFIG_REVISION_TAG
	setup_revision_tag (&params);
#endif

#ifdef CONFIG_REVISION16_TAG
	setup_revision16_tag (&params);
#endif

#ifdef CONFIG_SERIAL16_TAG
	setup_serial16_tag (&params);
#endif

#ifdef CONFIG_MACADDR_TAG
	setup_macaddr_tag (&params);
#endif

#ifdef CONFIG_BOOTMODE_TAG
	setup_bootmode_tag (&params);
#endif

#ifdef CONFIG_GYROCAL_TAG
	setup_gyrocal_tag (&params);
#endif

#ifdef CONFIG_SETUP_MEMORY_TAGS
	setup_memory_tags (bd);
#endif
#ifdef CONFIG_CMDLINE_TAG
	setup_commandline_tag (bd, commandline);
#endif
#ifdef CONFIG_INITRD_TAG
	if (initrd_start && initrd_end)
		setup_initrd_tag (bd, initrd_start, initrd_end);
#endif
#if defined (CONFIG_VFD) || defined (CONFIG_LCD)
	setup_videolfb_tag ((gd_t *) gd);
#endif
	setup_end_tag (bd);
#endif

	/* we assume that the kernel is in place */
	printf ("\nStarting kernel ...\n\n");

#ifdef CONFIG_USB_DEVICE
	{
		extern void udc_disconnect (void);
		udc_disconnect ();
	}
#endif

	cleanup_before_linux ();

	theKernel (0, bd->bi_arch_number, bd->bi_boot_params);
}


void do_booti_linux (boot_img_hdr *hdr)
{
	ulong initrd_start, initrd_end;
	void (*theKernel)(int zero, int arch, uint params);
	bd_t *bd = gd->bd;

	theKernel = (void (*)(int, int, uint))(hdr->kernel_addr);

	initrd_start = hdr->ramdisk_addr;
	initrd_end = initrd_start + hdr->ramdisk_size;

#if defined (CONFIG_SETUP_MEMORY_TAGS)
	setup_start_tag (bd);
#ifdef CONFIG_SERIAL_TAG
	setup_serial_tag (&params);
#endif
#ifdef CONFIG_REVISION_TAG
	setup_revision_tag (&params);
#endif

 #ifdef CONFIG_REVISION16_TAG
	setup_revision16_tag (&params);
#endif
#ifdef CONFIG_SERIAL16_TAG
        setup_serial16_tag (&params);
#endif
#ifdef CONFIG_MACADDR_TAG
        setup_macaddr_tag (&params);
#endif

#ifdef CONFIG_BOOTMODE_TAG
        setup_bootmode_tag (&params);
#endif

#ifdef CONFIG_GYROCAL_TAG
        setup_gyrocal_tag (&params);
#endif

#ifdef CONFIG_SETUP_MEMORY_TAGS
	setup_memory_tags (bd);
#endif
#ifdef CONFIG_CMDLINE_TAG
	setup_commandline_tag (bd, hdr->cmdline);
#endif
#ifdef CONFIG_INITRD_TAG
	if (hdr->ramdisk_size)
		setup_initrd_tag (bd, initrd_start, initrd_end);
#endif
#if defined (CONFIG_VFD) || defined (CONFIG_LCD)
	setup_videolfb_tag ((gd_t *) gd);
#endif
	setup_end_tag (bd);
#endif

	set_bl_for_recovery ();

	/* we assume that the kernel is in place */
	printf ("\nStarting kernel ...\n\n");

#ifdef CONFIG_USB_DEVICE
	{
		extern void udc_disconnect (void);
		udc_disconnect ();
	}
#endif

	cleanup_before_linux ();

	theKernel (0, bd->bi_arch_number, bd->bi_boot_params);
}

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG) || \
    defined (CONFIG_LCD) || \
    defined (CONFIG_VFD)
static void setup_start_tag (bd_t *bd)
{
	params = (struct tag *) bd->bi_boot_params;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size (tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next (params);
}


#ifdef CONFIG_SETUP_MEMORY_TAGS
static void setup_memory_tags (bd_t *bd)
{
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size (tag_mem32);

		params->u.mem.start = bd->bi_dram[i].start;
		params->u.mem.size = bd->bi_dram[i].size;

		params = tag_next (params);
	}
}
#endif /* CONFIG_SETUP_MEMORY_TAGS */


static void setup_commandline_tag (bd_t *bd, char *commandline)
{
	char *p;

	if (!commandline)
		return;

	/* eat leading white space */
	for (p = commandline; *p == ' '; p++);

	/* skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;

	strcpy (params->u.cmdline.cmdline, p);

	params = tag_next (params);
}


#ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start, ulong initrd_end)
{
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size (tag_initrd);

	params->u.initrd.start = initrd_start;
	params->u.initrd.size = initrd_end - initrd_start;

	params = tag_next (params);
}
#endif /* CONFIG_INITRD_TAG */


#if defined (CONFIG_VFD) || defined (CONFIG_LCD)
extern ulong calc_fbsize (void);
static void setup_videolfb_tag (gd_t *gd)
{
	/* An ATAG_VIDEOLFB node tells the kernel where and how large
	 * the framebuffer for video was allocated (among other things).
	 * Note that a _physical_ address is passed !
	 *
	 * We only use it to pass the address and size, the other entries
	 * in the tag_videolfb are not of interest.
	 */
	params->hdr.tag = ATAG_VIDEOLFB;
	params->hdr.size = tag_size (tag_videolfb);

	params->u.videolfb.lfb_base = (u32) gd->fb_base;
	/* Fb size is calculated according to parameters for our panel
	 */
	params->u.videolfb.lfb_size = calc_fbsize();

	params = tag_next (params);
}
#endif /* CONFIG_VFD || CONFIG_LCD */

#ifdef CONFIG_SERIAL_TAG
void setup_serial_tag (struct tag **tmp)
{
	struct tag *params = *tmp;
	struct tag_serialnr serialnr;
	void get_board_serial(struct tag_serialnr *serialnr);

	get_board_serial(&serialnr);
	params->hdr.tag = ATAG_SERIAL;
	params->hdr.size = tag_size (tag_serialnr);
	params->u.serialnr.low = serialnr.low;
	params->u.serialnr.high= serialnr.high;
	params = tag_next (params);
	*tmp = params;
}
#endif


#ifdef CONFIG_SERIAL16_TAG
extern const u8 *idme_get_board_serial(void);

/* 16-byte serial number tag (alphanumeric string) */
void setup_serial16_tag(struct tag **in_params)
{
	const u8 *sn = 0;

	sn = idme_get_board_serial();
	if (!sn){
                printf("Erro, failed to get_board_serial\n");
		return; /* ignore if NULL was returned. */
        }

        printf("board serial: %s\n", sn);
	params->hdr.tag = ATAG_SERIAL16;
	params->hdr.size = tag_size (tag_id16);
	memcpy(params->u.id16.data, sn, sizeof params->u.id16.data);
	params = tag_next (params);
}
#endif  /* CONFIG_SERIAL16_TAG */

#ifdef CONFIG_REVISION_TAG
void setup_revision_tag(struct tag **in_params)
{
	u32 rev = 0;

#ifdef CONFIG_BOARD_REVISION
	rev = gd->bd->bi_board_revision;
#else
	u32 get_board_rev(void);
	rev = get_board_rev();
#endif

	params->hdr.tag = ATAG_REVISION;
	params->hdr.size = tag_size (tag_revision);
	params->u.revision.rev = rev;
	params = tag_next (params);
}
#endif  /* CONFIG_REVISION_TAG */

#ifdef CONFIG_REVISION16_TAG
extern const u8 *idme_get_board_id(void);

/* 16-byte revision tag (alphanumeric string) */
void setup_revision16_tag(struct tag **in_params)
{
	const u8 *rev = 0;

	rev = idme_get_board_id();
	if (!rev){
                printf("Erro, failed to get_board_id\n");
		return; /* ignore if NULL was returned. */
        }
        printf("board id: %s\n", rev);
	params->hdr.tag = ATAG_REVISION16;
	params->hdr.size = tag_size (tag_id16);
	memcpy (params->u.id16.data, rev, sizeof(params->u.id16.data));
	params = tag_next (params);
}
#endif  /* CONFIG_REVISION16_TAG */

#ifdef CONFIG_MACADDR_TAG
#ifdef CONFIG_ENABLE_IDME
#include <idme.h>
#endif

/* MAC address/secret tag (alphanumeric strings) */
void setup_macaddr_tag(struct tag **in_params)
{
#ifdef CONFIG_ENABLE_IDME  
    char mac_buf[IDME_MAX_MAC_LEN+1];
    char sec_buf[IDME_MAX_SEC_LEN+1];
    char bt_mac_buf[IDME_MAX_MAC_LEN+1];
    
    memset(mac_buf, 0, IDME_MAX_MAC_LEN+1);
    memset(sec_buf, 0, IDME_MAX_SEC_LEN+1);
    memset(bt_mac_buf, 0, IDME_MAX_MAC_LEN+1);

    if (idme_get_var("mac", mac_buf, sizeof(mac_buf))){
            printf("Error, failed to get the mac address from idme\n");
            return;
    }

    printf("mac: %s\n", mac_buf);

    if (idme_get_var("bt",bt_mac_buf, sizeof(bt_mac_buf))){
            printf("Error, failed to get the mac address from idme\n");
            return;
    }

    printf("bt mac: %s\n", bt_mac_buf);


    if (idme_get_var("sec", sec_buf, sizeof(sec_buf))){
            printf("Error, failed to get the sec address from idme\n");
            return;
    }
    printf("sec: %s\n", sec_buf);

    params->hdr.tag = ATAG_MACADDR;
    params->hdr.size = tag_size (tag_macaddr);

    memcpy (params->u.macaddr.secret, 
            sec_buf, 
            sizeof params->u.macaddr.secret);

    memcpy (params->u.macaddr.wifi_addr, 
            mac_buf, 
            sizeof params->u.macaddr.wifi_addr);

    memcpy (params->u.macaddr.bt_addr, 
            bt_mac_buf, 
            sizeof params->u.macaddr.bt_addr);

    params = tag_next (params);
#endif

}
#endif  /* CONFIG_MACADDR_TAG */


#ifdef CONFIG_BOOTMODE_TAG
#ifdef CONFIG_CMD_IDME
#include <idme.h>
#endif

/* bootmode tag (alphanumeric strings) */
void setup_bootmode_tag(struct tag **in_params)
{
#ifdef CONFIG_ENABLE_IDME  
    char bootmode_buf[IDME_MAX_BOOTMODE_LEN+1];
    char postmode_buf[IDME_MAX_BOOTMODE_LEN+1];
    
    unsigned long count = 0;

    memset(bootmode_buf, 0, IDME_MAX_BOOTMODE_LEN+1);
    memset(postmode_buf, 0, IDME_MAX_BOOTMODE_LEN+1);

    if (idme_get_var("bootmode", bootmode_buf, sizeof(bootmode_buf))){
            printf("Error, failed to get the bootmode\n");
            return;
    }

    printf("bootmode: %s\n",bootmode_buf);


    if (idme_get_var("postmode", postmode_buf, sizeof(postmode_buf))){
            printf("Error, failed to get postmode\n");
            return;
    }

    memcpy(&count, postmode_buf, sizeof(unsigned long));
    printf("count in postmode buf = %lu\n", count);

    //    printf("postmode: %s\n", postmode_buf);

    params->hdr.tag = ATAG_BOOTMODE;
    params->hdr.size = tag_size (tag_bootmode);
    memcpy (params->u.bootmode.boot, 
            bootmode_buf, 
            sizeof params->u.bootmode.boot);
    memcpy (params->u.bootmode.post, 
            postmode_buf, 
            sizeof params->u.bootmode.post);
	

    memcpy((char*)&count, params->u.bootmode.post, sizeof(unsigned long));

    printf("postmode in atag = %lu\n", count);

    params = tag_next (params);
#endif

}
#endif  /* CONFIG_BOOTMODE_TAG */

#ifdef CONFIG_GYROCAL_TAG
#ifdef CONFIG_CMD_IDME
#include <idme.h>
#endif

/* gyrocal tag */
void setup_gyrocal_tag(struct tag **in_params)
{
#ifdef CONFIG_ENABLE_IDME
    int j = 0;
    char gyrocal_data_buf[IDME_MAX_GYROCAL_LEN+1];
//    params = (struct tag *) bd->bi_boot_params;

    memset(gyrocal_data_buf, 0, IDME_MAX_GYROCAL_LEN+1);

    if (idme_get_var("gyrocal", gyrocal_data_buf, sizeof(gyrocal_data_buf))){
            printf("Error, failed to get the gyrocal\n");
            return;
    }

    printf("gyrocal: %s\n",gyrocal_data_buf);
#if 1
        printf("gyrocal_size=%d. \n",sizeof(gyrocal_data_buf));

        printf("        0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");

        for ( j=0 ; j < sizeof(gyrocal_data_buf) ; j++){

             if( j%16 == 0 ){
                     printf("\n  %03X:", j/16);
                     printf(" %02X", gyrocal_data_buf[j]);
             }else{
                     printf(" %02X", gyrocal_data_buf[j]);
             }
        }
        printf("\n\n");
#endif

    params->hdr.tag = ATAG_GYROCAL;
    params->hdr.size = tag_size (tag_gyrocal);
    memcpy (params->u.gyrocal.gyrocal_data,
            gyrocal_data_buf,
            sizeof params->u.gyrocal.gyrocal_data);
	params = tag_next (params);
#endif

}
#endif  /* CONFIG_GYROCAL_TAG */

static void setup_end_tag (bd_t *bd)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */
