/*
 * (C) Copyright 2009
 * Texas Instruments, <www.ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <config.h>
#ifdef CONFIG_TWL6030

#include <twl6030.h>

#define REGEN1_CFG_TRANS	0xAE

/* Functions to read and write from TWL6030 */
static inline int twl6030_i2c_write_u8(u8 chip_no, u8 val, u8 reg)
{
	return i2c_write(chip_no, reg, 1, &val, 1);
}

inline int twl6030_i2c_read_u8(u8 chip_no, u8 *val, u8 reg)
{
	return i2c_read(chip_no, reg, 1, val, 1);
}

void twl6030_enable_vusb(void)
{
	u8 data = 0;

	/* Enable VUSB */
	twl6030_i2c_read_u8(TWL6030_CHIP_PM, &data, MISC2);
	data |= 0x10;
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, data, MISC2);

	/* Select APP Group and set state to ON */
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x18, VUSB_CFG_VOLTAGE);
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x0, VUSB_CFG_GRP);
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x0, VUSB_CFG_TRANS);
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x01, VUSB_CFG_STATE);
}

void twl6030_disable_vusb(void)
{
	u8 data = 0;

	/* Select APP Group and set state to OFF */
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x18, VUSB_CFG_VOLTAGE);
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x0, VUSB_CFG_GRP);
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x0, VUSB_CFG_TRANS);
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x0, VUSB_CFG_STATE);

	/* Disable VUSB and CHRG_PMID */
	twl6030_i2c_read_u8(TWL6030_CHIP_PM, &data, MISC2);
	data &= ~0x18;
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, data, MISC2);
}

void twl6030_shutdown()
{
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, PHONIX_APP_DEVOFF, TWL6030_PHONIX_DEV_ON);
}

int twl6030_get_vbus_status()
{
	u8 data = 0x00;

	twl6030_i2c_read_u8(TWL6030_CHIP_CHARGER, &data, CONTROLLER_STAT1);

	return ((data & 0x4) >> 2);
}

void twl6030_start_usb_charging(void)
{
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CHARGERUSB_VICHRG_1500,
							CHARGERUSB_VICHRG);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CHARGERUSB_CIN_LIMIT_NONE,
							CHARGERUSB_CINLIMIT);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, MBAT_TEMP,
							CONTROLLER_INT_MASK);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, MASK_MCHARGERUSB_THMREG,
							CHARGERUSB_INT_MASK);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CHARGERUSB_VOREG_4P0,
							CHARGERUSB_VOREG);
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CHARGERUSB_CTRL2_VITERM_100,
							CHARGERUSB_CTRL2);
	/* Enable USB charging */
	twl6030_i2c_write_u8(TWL6030_CHIP_CHARGER, CONTROLLER_CTRL1_EN_CHARGER,
							CONTROLLER_CTRL1);

	return;
}

void twl6030_init_battery_charging(void)
{
	twl6030_start_usb_charging();
	return;
}

void twl6032_init(void)
{
	printf("twl6032_init \n");

	/* Enable the PREQ1 */
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0xC0, PHOENIX_MSK_TRANSITION);

}

void twl6030_usb_device_settings()
{
	u8 data = 0;

	/* Select APP Group and set state to ON */
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, 0x21, VUSB_CFG_STATE);

	twl6030_i2c_read_u8(TWL6030_CHIP_PM, &data, MISC2);
	data |= 0x10;

	/* Select the input supply for VBUS regulator */
	twl6030_i2c_write_u8(TWL6030_CHIP_PM, data, MISC2);
}
#endif
