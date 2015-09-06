/*
 * (C) Copyright 2011-2012
 * Texas Instruments France
 *
 * Sebastien Griffoul <s-griffoul@ti.com>
 *
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

#ifndef _BOOTMSG_H
#define _BOOTMSG_H

/* return the command store into the boot message */
char* bootmsg_get_command(void);
void  bootmsg_write_command(const char* command);

/* Available command */
#define BOOTMSG_COMMAND_RECOVERY   "boot-recovery"
#define BOOTMSG_COMMAND_NORMAL     "boot"


#endif
