/*
 * bq27541.c
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

#if defined(CONFIG_BQ27541)
#include <bq27541.h>

#define BQ27541_I2C_BUS	(2)			/* I2C Bus # of BQ27541 */
#define BQ27541_ADDR	(0x55)			/* I2C Addr of BQ27541 */

/* BQ27541 registers */
#define BQ27541_REG_CONTROL	(0x00)		/* Control register */
#define BQ27541_REG_TEMP	(0x06)		/* Temperature (in 0.1 K) */
#define BQ27541_REG_VOLTAGE	(0x08)		/* Voltage (in mV) */
#define BQ27541_REG_FLAGS	(0x0a)		/* Flags */
#define BQ27541_REG_FLAGS_DSG	(1 << 0)	/* Discharging */
#define BQ27541_REG_FLAGS_FC	(1 << 9)	/* Full charge */
#define BQ27541_REG_FLAGS_OTD	(1 << 14)	/* Over-temp under discharge */
#define BQ27541_REG_FLAGS_OTC	(1 << 15)	/* Over-temp under charge */
#define BQ27541_REG_CURRENT	(0x14)		/* Current (in mA) */
#define BQ27541_REG_SOC		(0x2c)		/* State of Charge (in %) */

extern int select_bus(int bus, int speed);

static int bq27541_i2c_read(u8 reg, void *buffer, size_t size)
{
	int status = -1;

	/* Switch to the I2C bus the gas gauge is connected to */
	if (select_bus(BQ27541_I2C_BUS, 100)) {
		printf("BQ27541: Failed to switch to bus %d\n", BQ27541_I2C_BUS);
		goto done;
	}

	if ((status = i2c_read(BQ27541_ADDR, reg, 1, buffer, size)))
		printf("BQ27541: I2C read failed: %d\n", status);

done:
	/* Switch back to the original I2C bus */
	select_bus(0, 100);

	return status;
}

static int bq27541_i2c_write(u8 reg, void *buffer, size_t size)
{
	int status = -1;

	/* Switch to the I2C bus the gas gauge is connected to */
	if (select_bus(BQ27541_I2C_BUS, 100)) {
		printf("BQ27541: Failed to switch to bus %d\n", BQ27541_I2C_BUS);
		goto done;
	}

	if ((status = i2c_write(BQ27541_ADDR, reg, 1, buffer, size)))
		printf("BQ27541: I2C write failed: %d\n", status);
	
done:
	/* Switch back to the original I2C bus */
	select_bus(0, 100);

	return status;
}

int bq27541_device_type(void)
{
	u8 cmd[2] = { 0x01, 0x00 };

	if (bq27541_i2c_write(BQ27541_REG_CONTROL, cmd, sizeof(cmd))) {
		printf("BQ27541: Error writing to control register\n");
		return -1;
	}

	if (bq27541_i2c_read(BQ27541_REG_CONTROL, cmd, sizeof(cmd))) {
		printf("BQ27541: Error reading from control register\n");
	}

	if (cmd[1] == 0x05 && cmd[0] == 0x41) {
		printf("BQ27541: Device type is 0x0541\n");
		return 0;
	} else {
		printf("BQ27541: Device type is invalid\n");
		return 1;
	}
}

int bq27541_capacity(u16 *capacity)
{
	if (!capacity)
		return -1;

	if (bq27541_i2c_read(BQ27541_REG_SOC, capacity, sizeof(*capacity)))
		return -1;

	*capacity = __le16_to_cpu(*capacity);

	return 0;
}

static int do_bq27541_capacity(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u16 capacity = 0;

	if (bq27541_capacity(&capacity)) {
		printf("BQ27541: Failed to read capacity\n");
		return -1;
	}

	printf("%d%%\n", capacity);

	return 0;
}

int bq27541_temperature(s16 *temp)
{
	u16 value = 0;

	if (!temp)
		return -1;

	if (bq27541_i2c_read(BQ27541_REG_TEMP, &value, sizeof(value)))
		return -1;

	/* Convert to C */
	*temp = __le16_to_cpu(value) - 2731;
	*temp = *temp / 10;

	return 0;
}

static int do_bq27541_temperature(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	s16 temp = 0;

	if (bq27541_temperature(&temp)) {
		printf("BQ27541: Failed to read temperature\n");
		return -1;
	}

	printf("%d C\n", temp);

	return 0;
}

int bq27541_voltage(u16 *voltage)
{
	if (!voltage)
		return -1;

	if (bq27541_i2c_read(BQ27541_REG_VOLTAGE, voltage, sizeof(*voltage)))
		return -1;

	*voltage = __le16_to_cpu(*voltage);

	return 0;
}

static int do_bq27541_voltage(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u16 voltage = 0;

	if (bq27541_voltage(&voltage)) {
		printf("BQ27541: Failed to read voltage\n");
		return -1;
	}

	printf("%d mV\n", voltage);

	return 0;
}

int bq27541_current(s16 *current)
{
	if (!current)
		return -1;

	if (bq27541_i2c_read(BQ27541_REG_CURRENT, current, sizeof(*current)))
		return -1;

	*current = __le16_to_cpu(*current);

	return 0;
}

static int do_bq27541_current(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	s16 current = 0;

	if (bq27541_current(&current)) {
		printf("BQ27541: Failed to read current\n");
		return -1;
	}

	printf("%d mA\n", current);

	return 0;
}

int bq27541_flags(u16 *flags)
{
	if (!flags)
		return -1;

	if (bq27541_i2c_read(BQ27541_REG_FLAGS, flags, sizeof(*flags)))
		return -1;

	*flags = __le16_to_cpu(*flags);

	return 0;
}

static int do_bq27541_flags(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u16 flags = 0;
	s16 current = 0;

	if (bq27541_flags(&flags)) {
		printf("BQ27541: Failed to read flags\n");
		return -1;
	}

	if (flags & BQ27541_REG_FLAGS_FC) {
		printf("BQ27541: Battery has fully charged\n");
	} else if (flags & BQ27541_REG_FLAGS_DSG) {
		printf("BQ27541: Battery is being discharged\n");
	} else {
		if (bq27541_current(&current)) {
			printf("BQ27541: Failed to read current\n");
			return -1;
		}

		if (current > 0) {
			printf("BQ27541: Battery is being charged\n");
		} else {
			printf("BQ27541: Battery is not being charged\n");
		}
	}

	if (flags & BQ27541_REG_FLAGS_OTC) {
		printf("BQ27541: Over-Temperature during charge detected\n");
	} else if (flags & BQ27541_REG_FLAGS_OTD) {
		printf("BQ27541: Over-Temperature during discharge detected\n");
	}

	return 0;
}

static int do_bq27541_type(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	bq27541_device_type();
	return 0;
}

U_BOOT_CMD(
        bq27541_capacity, 1, 0, do_bq27541_capacity,
	"bq27541_capacity - Return battery capacity in % (0 - 100)\n",
	NULL
);

U_BOOT_CMD(
	bq27541_temp, 1, 0, do_bq27541_temperature,
	"bq27541_temperature - Return battery temperature in C\n",
	NULL
);

U_BOOT_CMD(
        bq27541_voltage, 1, 0, do_bq27541_voltage,
	"bq27541_voltage - Return battery voltage in mV\n",
	NULL
);

U_BOOT_CMD(
        bq27541_current, 1, 0, do_bq27541_current,
	"bq27541_current - Return battery current in mA\n",
	NULL
);

U_BOOT_CMD(
	bq27541_status, 1, 0, do_bq27541_flags,
	"bq27541_status - Display BQ27541 status\n",
	NULL
);

U_BOOT_CMD(
	bq27541_type, 1, 0, do_bq27541_type,
	"bq27541_type - Display BQ27541 device type\n",
	NULL
);

#endif
