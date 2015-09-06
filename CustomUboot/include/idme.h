/*
 * idme.h 
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

/*!
 * @file idme.h
 * @brief This file contains functions for interacting with variables
 *in the userstore partition
 *
 */

#ifndef __IDME_H__
#define __IDME_H__

#ifdef CONFIG_ENABLE_IDME 

int idme_check_update(void);
int idme_get_var(const char *name, char *buf, int buflen);
int idme_update_var(const char *name, const char *value);
int idme_select_boot_image(char **ptn);
int idme_get_board_type_rev_string(char *);

#define IDME_MAX_MAC_LEN 16
#define IDME_MAX_SEC_LEN 32
#define IDME_MAX_BOOTMODE_LEN 16
#define IDME_MAX_BOARD_SERIAL_LEN 16
#define IDME_MAX_BOARD_ID_LEN 16
#define IDME_MAX_BT_LEN 16
#define	IDME_MAX_GYROCAL_LEN 36
#if defined(CONFIG_MACH_BOWSER_SUBTYPE_TATE)
#define IDME_MAX_BTMAC_LEN 16
#define IDME_MAX_QMFG_LEN 512
#endif

#endif /* CONFIG_IDME_ENABLE */
	
#endif /* __IDME_H__ */
