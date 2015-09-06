/*
 * bq27200.c
 *
 * Copyright (C) Amazon Technologies Inc. All rights reserved.
 * Donald Chan (hoiho@lab126.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <config.h>
#include <common.h>
#include <command.h>
#include <i2c.h>
#include <asm/io.h>

#if defined(CONFIG_BQ27200)

#define BQ27200_I2C_BUS	(2)			/* I2C Bus # of BQ27200 */
#define BQ27200_ADDR	(0x55)			/* I2C Addr of BQ27200 */

/* BQ27200 registers */
#define BQ27200_REG_TEMP	(0x06)		/* Temperature (in 0.25 K) */
#define BQ27200_REG_VOLTAGE	(0x08)		/* Voltage (in mV) */
#define BQ27200_REG_FLAGS	(0x0a)		/* Flags */
#define BQ27200_REG_FLAGS_CHRG	(1 << 7)	/* Charging */
#define BQ27200_REG_CURRENT	(0x14)		/* Current (in mA) */
#define BQ27200_REG_SOC		(0x2c)		/* State of Charge (in %) */

extern int select_bus(int bus, int speed);

static int bq27200_i2c_read(u8 reg, void *buffer, size_t size)
{
	int status = -1;

	/* Switch to the I2C bus the gas gauge is connected to */
	if (select_bus(BQ27200_I2C_BUS, 100)) {
		printf("BQ27200: Failed to switch to bus %d\n", BQ27200_I2C_BUS);
		goto done;
	}

	if ((status = i2c_read(BQ27200_ADDR, reg, 1, buffer, size)))
		printf("BQ27200: I2C read failed: %d\n", status);

done:
	/* Switch back to the original I2C bus */
	select_bus(0, 100);

	return status;
}

int bq27200_capacity(u8 *capacity)
{
	if (!capacity)
		return -1;

	if (bq27200_i2c_read(BQ27200_REG_SOC, capacity, sizeof(*capacity)))
		return -1;

	return 0;
}

static int do_bq27200_capacity(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u8 capacity = 0;

	if (bq27200_capacity(&capacity)) {
		printf("BQ27200: Failed to read capacity\n");
		return -1;
	}

	printf("%d%%\n", capacity);

	return 0;
}

int bq27200_temperature(s16 *temp)
{
	u16 value = 0;

	if (!temp)
		return -1;

	if (bq27200_i2c_read(BQ27200_REG_TEMP, &value, sizeof(value)))
		return -1;

	/* Convert to C */
	*temp = __le16_to_cpu(value) / 4 - 273;

	return 0;
}

static int do_bq27200_temperature(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	s16 temp = 0;

	if (bq27200_temperature(&temp)) {
		printf("BQ27200: Failed to read temperature\n");
		return -1;
	}

	printf("%d C\n", temp);

	return 0;
}

int bq27200_voltage(u16 *voltage)
{
	if (!voltage)
		return -1;

	if (bq27200_i2c_read(BQ27200_REG_VOLTAGE, voltage, sizeof(*voltage)))
		return -1;

	*voltage = __le16_to_cpu(*voltage);

	return 0;
}

static int do_bq27200_voltage(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u16 voltage = 0;

	if (bq27200_voltage(&voltage)) {
		printf("BQ27200: Failed to read voltage\n");
		return -1;
	}

	printf("%d mV\n", voltage);

	return 0;
}

int bq27200_current(s16 *current)
{
	u8 flags = 0;

	if (!current)
		return -1;

	if (bq27200_i2c_read(BQ27200_REG_CURRENT, current, sizeof(*current)))
		return -1;

	if (bq27200_i2c_read(BQ27200_REG_FLAGS, &flags, sizeof(flags)))
		return -1;

	/* Units of 3.57 uV divided by a sense resistor of 0.02 ohm */
	*current = __le16_to_cpu(*current) * 357 / 2000;

	if (!(flags & BQ27200_REG_FLAGS_CHRG))
		*current = *current * -1;

	return 0;
}

static int do_bq27200_current(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	s16 current = 0;

	if (bq27200_current(&current)) {
		printf("BQ27200: Failed to read current\n");
		return -1;
	}

	printf("%d mA\n", current);

	return 0;
}

int bq27200_flags(u8 *flags)
{
	if (!flags)
		return -1;

	if (bq27200_i2c_read(BQ27200_REG_FLAGS, flags, sizeof(*flags)))
		return -1;

	return 0;
}

static int do_bq27200_flags(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	s16 current = 0;

	if (bq27200_current(&current)) {
		printf("BQ27200: Failed to read current\n");
		return -1;
	}

	if (current > 0) {
		printf("BQ27200: Battery is being charged\n");
	} else {
		printf("BQ27200: Battery is not being charged\n");
	}

	return 0;
}

U_BOOT_CMD(
        bq27200_capacity, 1, 0, do_bq27200_capacity,
	"bq27200_capacity - Return battery capacity in % (0 - 100)\n",
	NULL
);

U_BOOT_CMD(
	bq27200_temp, 1, 0, do_bq27200_temperature,
	"bq27200_temperature - Return battery temperature in C\n",
	NULL
);

U_BOOT_CMD(
        bq27200_voltage, 1, 0, do_bq27200_voltage,
	"bq27200_voltage - Return battery voltage in mV\n",
	NULL
);

U_BOOT_CMD(
        bq27200_current, 1, 0, do_bq27200_current,
	"bq27200_current - Return battery current in mA\n",
	NULL
);

U_BOOT_CMD(
	bq27200_status, 1, 0, do_bq27200_flags,
	"bq27200_status - Display BQ27200 status\n",
	NULL
);

#endif
