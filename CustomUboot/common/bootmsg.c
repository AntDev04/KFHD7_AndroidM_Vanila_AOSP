/*
 * (C) Copyright 2011-2012
 * Texas Instruments France
 *
 * Sebastien Griffoul <s-griffoul@ti.com>

 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <command.h>
#include <fastboot.h>
#include <mmc.h>
#include <bootmsg.h>

#if (defined(CFG_BOOTMSG) && (CFG_BOOTMSG))

/*
 * the bootloader message block is a persitent memory area
 * used by Android to exchange data with the bootloader and
 * the recovery.
 * The bootloader message block is stored into the misc partition 
 * This message has several fields:
 *   - command       -> used by Android to select the image to be loaded by the bootloader
 *   - status        -> used by the recovery to send a status to Android
 *   - recovery area -> used by Android and recovery
 */
#define COMMAND_SIZE         32
#define STATUS_SIZE          32
#define RECOVERY_SIZE      1024

#define BOOTMSG_BLOCK_SIZE  512

struct bootloader_message {
    char command[COMMAND_SIZE];
    char status[STATUS_SIZE];
    char recovery[RECOVERY_SIZE];
};

#if (defined(CFG_BOOTMSG_IN_EMMC) && (CFG_BOOTMSG_IN_EMMC))

#ifndef CFG_BOOTMSG_PARTITION
#error "CFG_BOOTMSG_PARTITION has to be defined with the name of the partition used to hold the bootmsg"
#endif

static unsigned char  bootmsg_1st_block[BOOTMSG_BLOCK_SIZE];
static unsigned char  bootmsg_partition[] = CFG_BOOTMSG_PARTITION;

/* This function read the 1st bootmsg block and return the command which stores at
 * the begining of this block
 */
char* bootmsg_get_command()
{
    unsigned int   mmc_cont = 1;
    struct fastboot_ptentry *pte;

    pte = fastboot_flash_find_ptn(bootmsg_partition);
    if (!pte) 
    {
        printf("bootmsg_get_command: cannot find '%s' partition\n", bootmsg_partition);
	return NULL;
    }

    mmc_read(mmc_cont, pte->start, bootmsg_1st_block, BOOTMSG_BLOCK_SIZE);

    /* Ensure that command is null terminated */
    bootmsg_1st_block[COMMAND_SIZE-1] = '\0';

    return bootmsg_1st_block;
}

/* This function read the 1st bootmsg block and return the command which stores at
 * the begining of this block
 */
void bootmsg_write_command(const char* command)
{
    unsigned int   mmc_cont = 1;
    struct fastboot_ptentry *pte;

    pte = fastboot_flash_find_ptn(bootmsg_partition);
    if (!pte) 
    {
        printf("bootmsg_get_command: cannot find '%s' partition\n", bootmsg_partition);
    }

    /* read the 1st bootmsg block */ 
    mmc_read(mmc_cont, pte->start, bootmsg_1st_block, BOOTMSG_BLOCK_SIZE);

    /* Copy the new string */
    strncpy(bootmsg_1st_block, command, COMMAND_SIZE);

    /* Write it back */
    mmc_write(mmc_cont, bootmsg_1st_block, pte->start, BOOTMSG_BLOCK_SIZE);

    /* Ensure that command is null terminated */
    bootmsg_1st_block[COMMAND_SIZE-1] = '\0';
}
#endif /* CFG_BOOTMSG_IN_EMMC */
#endif /* CFG_BOOTMSG */
