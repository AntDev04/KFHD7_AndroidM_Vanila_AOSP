/*
 * smb347.c
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
#include <smb347.h>
#include <twl6030.h>
#include <asm/arch/mux.h>

#if defined(CONFIG_SMB347)

#define SMB347_I2C_BUS	(2)			/* I2C Bus # of SMB347 */
#define SMB347_ADDR_ORIG (0x06)			/* I2C Addr of SMB347 (Orig) */
#define SMB347_ADDR	 (0x5F)			/* I2C Addr of SMB347 */

#define SMB347_LOG(fmt, args...) printf("SMB347: " fmt, ## args)

extern int select_bus(int bus, int speed);
extern int bq27541_capacity(u16 *capacity);

static int smb347_addr = 0;
int gnUSBAttached = 0;


static int smb347_i2c_probe()
{
    int status = -1;
    u8 buffer;

    /* Switch to the I2C bus the gas gauge is connected to */
    if (select_bus(SMB347_I2C_BUS, 100)) {
        SMB347_LOG("Failed to switch to bus %d\n", SMB347_I2C_BUS);
        goto done;
    }

    status = i2c_read(SMB347_ADDR, 0, 1, &buffer, 1);

    if (status == 0) {
        smb347_addr = SMB347_ADDR;
    } else {
        status = i2c_read(SMB347_ADDR_ORIG, 0, 1, &buffer, 1);

        if (status == 0) {
            smb347_addr = SMB347_ADDR_ORIG;
        }
    }

 done:
    /* Switch back to the original I2C bus */
    select_bus(0, 100);

    return smb347_addr;
}

static int smb347_i2c_read(u8 reg, void *buffer, size_t size)
{
    int status = -1;
    int first_read = 1;

    /* Switch to the I2C bus the gas gauge is connected to */
    if (select_bus(SMB347_I2C_BUS, 100)) {
        SMB347_LOG("Failed to switch to bus %d\n", SMB347_I2C_BUS);
        goto done;
    }

    status = i2c_read(smb347_addr, reg, 1, buffer, size);
    
    if (status && first_read)
        SMB347_LOG("I2C read failed: %d\n", status);

 done:
 	 first_read = 0;
 	 
    /* Switch back to the original I2C bus */
    select_bus(0, 100);

    return status;
}

static int smb347_i2c_write(u8 reg, void *buffer, size_t size)
{
	int status = -1;

	/* Switch to the I2C bus the gas gauge is connected to */
	if (select_bus(SMB347_I2C_BUS, 100)) {
		SMB347_LOG("Failed to switch to bus %d\n", SMB347_I2C_BUS);
		goto done;
	}

	if ((status = i2c_write(smb347_addr, reg, 1, buffer, size)))
        SMB347_LOG("I2C write failed: %d\n", status);

 done:
	/* Switch back to the original I2C bus */
	select_bus(0, 100);

	return status;
}

inline int smb347_aicl_current(int reg)
{
	const int aicl_currents[] = {
		300, 500, 700, 900,
		1200, 1500, 1800, 2000,
		2200, 2500, 2500, 2500,
		2500, 2500, 2500, 2500
	};

	if (SMB347_IS_AICL_DONE(reg)) {
		int value = SMB347_AICL_RESULT(reg);

		if (value >= 0 && value <= 15) {
			return aicl_currents[value];
		}

		return -1;
	} else {
		return -1;
	}
}

inline int smb347_apsd_result(int reg)
{
	if (SMB347_IS_APSD_DONE(reg)) {
		return SMB347_APSD_RESULT(reg);
	} else {
		return -1;
	}
}

inline int smb347_charging_current(int reg)
{
	int current = -1;

	if (reg & (1 << 5)) {
		switch (reg & 0x7) {
		case 0:
			current = 700;
			break;
		case 1:
			current = 900;
			break;
		case 2:
			current = 1200;
			break;
		case 3:
			current = 1500;
			break;
		case 4:
			current = 1800;
			break;
		case 5:
			current = 2000;
			break;
		case 6:
			current = 2200;
			break;
		case 7:
			current = 2500;
			break;
		}
	} else {
		switch ((reg >> 3) & 0x3) {
		case 0:
			current = 100;
			break;
		case 1:
			current = 150;
			break;
		case 2:
			current = 200;
			break;
		case 3:
			current = 250;
			break;
		}
	}

	return current;
}

static const char *smb347_apsd_result_string(u8 value)
{
	switch (smb347_apsd_result(value)) {
	case SMB347_APSD_RESULT_CDP:
		return "CDP";
		break;
	case SMB347_APSD_RESULT_DCP:
		return "DCP";
		break;
	case SMB347_APSD_RESULT_OTHER:
		return "Other Downstream Port";
		break;
	case SMB347_APSD_RESULT_SDP:
		return "SDP";
		break;
	case SMB347_APSD_RESULT_ACA:
		return "ADA charger";
		break;
	case SMB347_APSD_RESULT_TBD_1:
	case SMB347_APSD_RESULT_TBD_2:
		return "TBD";
		break;
	case -1:
		return "not run";
		break;
	case SMB347_APSD_RESULT_NONE:
	default:
		return "unknown";
		break;
	}
}

int smb347_config_enable(int flag)
{
	int status = -1;
	u8 value = 0;

	if (smb347_i2c_read(SMB347_COMMAND_REG_A, &value, 1)) {
		SMB347_LOG("Unable to read command register A\n");
		goto done;
	}

	if (flag) {
		/* Enable bit 7 */
		value |= (1 << 7);
	} else {
		/* Disable bit 7 */
		value &= ~(1 << 7);
	}

	if (smb347_i2c_write(SMB347_COMMAND_REG_A, &value, 1)) {
		SMB347_LOG("Unable to write command register A\n");
		goto done;
	}

	status = 0;
done:
	return status;
}

int smb347_redo_apsd(void)
{
	int status = -1, timeout = 0;
	u8 val = 0;

	/* Enable volatile writes to config registers */
	if (smb347_config_enable(1))
		goto done;

	udelay(1000);

	/* Disable APSD */
	if (smb347_i2c_read(0x04, &val, 1)) {
		SMB347_LOG("Unable to read config reg 4 to disable APSD\n");
		goto done;
	}

	udelay(1000);
	val &= ~(1 << 2);

	if (smb347_i2c_write(0x04, &val, 1)) {
		SMB347_LOG("Unable to write config reg 4 to disable APSD\n");
		goto done;
	}

	udelay(1000);

	/* Enable APSD */
	val |= (1 << 2);

	if (smb347_i2c_write(0x04, &val, 1)) {
		SMB347_LOG("Unable to write config reg 4 to enable APSD\n");
		goto done;
	}

	udelay(1000);

	/* Disable volatile writes to config registers */
	if (smb347_config_enable(0))
		goto done;

	udelay(1000);

	/* Loop until APSD is done, or timeout (10 ms) */
	while (timeout <= 100) {
		if (smb347_i2c_read(SMB347_STATUS_REG_D, &val, 1)) {
			SMB347_LOG("Unable to read status reg D\n");
			goto done;
		}

		if (smb347_apsd_result(val) != -1)
			break;

		timeout++;
		udelay(1000);
	}

	if (timeout > 100) {
		SMB347_LOG("APSD timed out\n");
		goto done;
	}
	status = val;
done:
	return status;
}

static inline void smb347_parse_status_reg_a(u8 value)
{
	SMB347_LOG("Status Register A = 0x%02x\n", value);

	SMB347_LOG("Thermal Regulation Status: %s\n",
			(value & (1 << 7)) ? "Active" : "Inactive");

	SMB347_LOG("THERM Soft Limit Regulation Status: %s\n",
			(value & (1 << 6)) ? "Active" : "Inactive");

	int voltage = 3500 + (value & 0x3f) * 20;

	/* Max out at 4500 mV */
	if (voltage > 4500) voltage = 4500;

	SMB347_LOG("Actual Float Voltage after compensation: %d mV\n", voltage);
}

static inline void smb347_parse_status_reg_b(u8 value)
{
	SMB347_LOG("Status Register B = 0x%02x\n", value);

	SMB347_LOG("USB Suspend Mode: %s\n",
			(value & (1 << 7)) ? "Active" : "Inactive");

	int current = smb347_charging_current(value);

	if (current != -1) {
		SMB347_LOG("Actual Charge Current after "
				"compensation: %d mA\n", current);
	} else {
		SMB347_LOG("Actual Charge Current after "
				"compensation: Unknown\n");
	}
}

static inline void smb347_parse_status_reg_c(u8 value)
{
	SMB347_LOG("Status Register C = 0x%02x\n", value);

	SMB347_LOG("Charging Enable/Disable: %s\n",
			(value & 0x1) ? "Enabled" : "Disabled");

	switch (SMB347_CHARGING_STATUS(value)) {
	case SMB347_CHARGING_STATUS_NOT_CHARGING:
		SMB347_LOG("Charging Status: Not charging\n");
		break;
	case SMB347_CHARGING_STATUS_PRE_CHARGING:
		SMB347_LOG("Charging Status: Pre-charging\n");
		break;
	case SMB347_CHARGING_STATUS_FAST_CHARGING:
		SMB347_LOG("Charging Status: Fast-charging\n");
		break;
	case SMB347_CHARGING_STATUS_TAPER_CHARGING:
		SMB347_LOG("Charging Status: Taper-charging\n");
		break;
	default:
		SMB347_LOG("Charging Status: Unknown\n");
		break;
	}

	SMB347_LOG("Charger %s hold-off status\n",
			(value & (1 << 3)) ? "in" : "not in");

	SMB347_LOG("Vbatt %c 2.1 V\n", (value & (1 << 4)) ? '<' : '>');

	if (value & (1 << 5)) {
		SMB347_LOG("At least one charging cycle has terminated\n");
	} else {
		SMB347_LOG("No full charge cycle has occurred\n");
	}

	if (value & (1 << 6))
		SMB347_LOG("Charger has encountered an error\n");

	SMB347_LOG("Charger error %s an IRQ signal\n",
			(value & (1 << 7)) ? "asserts" : "does not assert");
}

static inline void smb347_parse_status_reg_d(u8 value)
{
	SMB347_LOG("Status Register D = 0x%02x\n", value);

	if (smb347_apsd_result(value) != -1) {
		SMB347_LOG("APSD completed, result: %s\n",
					smb347_apsd_result_string(value));
	} else {
		SMB347_LOG("APSD not completed\n");
	}
}

static inline void smb347_parse_status_reg_e(u8 value)
{
	int current = -1;

	SMB347_LOG("Status Register E = 0x%02x\n", value);

	SMB347_LOG("USBIN Input: %s\n",
		(value & (1 << 7)) ? "In Use" : "Not In Use");

	switch (SMB347_USB1_5_HC_MODE(value)) {
	case SMB347_HC_MODE:
		SMB347_LOG("In HC mode\n");
		break;
	case SMB347_USB1_MODE:
		SMB347_LOG("In USB1 mode\n");
		break;
	case SMB347_USB5_MODE:
		SMB347_LOG("In USB5 mode\n");
		break;
	}

	current = smb347_aicl_current(value);

	if (current != -1) {
		SMB347_LOG("AICL Completed, Result = %d mA\n", current);
	} else {
		SMB347_LOG("AICL not completed\n");
	}
}

static int do_smb347_status_register(int argc, char *argv[])
{
	int status = -1;
	u8 value = 0;

	if (argc < 3) {
		SMB347_LOG("Invalid # of arguments\n");
		goto done;
	}

	if (!strcmp(argv[2], "a") || !strcmp(argv[2], "3b")) {
		/* Status Register A (0x3b) */
		if (smb347_i2c_read(SMB347_STATUS_REG_A, &value, 1)) {
			SMB347_LOG("Failed to read status register A\n");
			goto done;
		}

		smb347_parse_status_reg_a(value);

	} else if (!strcmp(argv[2], "b") || !strcmp(argv[2], "3c")) {
		/* Status Register B (0x3c) */
		if (smb347_i2c_read(SMB347_STATUS_REG_B, &value, 1)) {
			SMB347_LOG("Failed to read status register B\n");
			goto done;
		}

		smb347_parse_status_reg_b(value);
	} else if (!strcmp(argv[2], "c") || !strcmp(argv[2], "3d")) {
		/* Status Register C (0x3d) */
		if (smb347_i2c_read(SMB347_STATUS_REG_C, &value, 1)) {
			SMB347_LOG("Failed to read status register C\n");
			goto done;
		}

		smb347_parse_status_reg_c(value);
	} else if (!strcmp(argv[2], "d") || !strcmp(argv[2], "3e")) {
		/* Status Register D (0x3e) */
		if (smb347_i2c_read(SMB347_STATUS_REG_D, &value, 1)) {
			SMB347_LOG("Failed to read status register D\n");
			goto done;
		}

		smb347_parse_status_reg_d(value);
	} else if (!strcmp(argv[2], "e") || !strcmp(argv[2], "3f")) {
		/* Status Register E (0x3f) */
		if (smb347_i2c_read(SMB347_STATUS_REG_E, &value, 1)) {
			SMB347_LOG("Failed to read status register E\n");
			goto done;
		}

		smb347_parse_status_reg_e(value);
	} else {
		SMB347_LOG("Unknown status register\n");
		goto done;
	}

	status = 0;
done:
	return status;
}

static int do_smb347_config_register(int argc, char *argv[])
{
	int status = -1, reg = -1;
	u8 val = 0;

	if (argc < 3) {
		SMB347_LOG("Invalid # of arguments\n");
		goto done;
	}

	if (!strcmp(argv[2], "enable")) {
		if (smb347_config_enable(1)) {
			SMB347_LOG("Unable to enable volatile writes "
					"to config registers\n");
			goto done;
		} else {
			SMB347_LOG("Enabled volatile writes "
					"to config registers\n");
		}

		status = 0;
	} else if (!strcmp(argv[2], "disable")) {
		if (smb347_config_enable(0)) {
			SMB347_LOG("Unable to disable volatile writes "
					"to config registers\n");
			goto done;
		} else {
			SMB347_LOG("Disabled volatile writes "
					"to config registers\n");
		}

		status = 0;
	} else if (argc == 3) {
		reg = simple_strtol(argv[2], NULL, 16);

		if (reg < 0x00 || reg > 0x0e) {
			SMB347_LOG("Invalid config register: %s\n", argv[2]);
			goto done;
		}

		if (smb347_i2c_read(reg, &val, 1)) {
			SMB347_LOG("Unable to read config register %x\n", reg);
			goto done;
		}

		SMB347_LOG("Config register %x = 0x%02x\n", reg, val);

		status = 0;
	} else if (argc == 4) {
		reg = simple_strtol(argv[2], NULL, 16);
		val = (u8)simple_strtol(argv[3], NULL, 16);

		if (reg < 0x00 || reg > 0x0e) {
			SMB347_LOG("Invalid config register: %s\n", argv[2]);
			goto done;
		}

		if (smb347_i2c_write(reg, &val, 1)) {
			SMB347_LOG("Unable to write to config register %x\n", reg);
			goto done;
		}

		SMB347_LOG("Wrote 0x%02x to config register %x\n", val, reg);

		status = 0;
	} else {
		SMB347_LOG("Invalid command\n");
	}

done:
	return status;
}

static int do_smb347_command_register(int argc, char *argv[])
{
	int status = -1, reg = -1;
	u8 val = 0;

	if (argc < 3) {
		SMB347_LOG("Invalid # of arguments\n");
		goto done;
	}

	/* Determine the right command register */
	if (!strcmp(argv[2], "a") || !strcmp(argv[2], "30")) {
		reg = SMB347_COMMAND_REG_A;
	} else if (!strcmp(argv[2], "b") || !strcmp(argv[2], "31")) {
		reg = SMB347_COMMAND_REG_B;
	} else if (!strcmp(argv[2], "c") || !strcmp(argv[2], "33")) {
		reg = SMB347_COMMAND_REG_C;
	} else {
		SMB347_LOG("Invalid command register: %s\n", argv[2]);
		goto done;
	}

	if (argc == 3) {
		/* Read from command register */
		if (smb347_i2c_read(reg, &val, 1)) {
			SMB347_LOG("Unable to read command register %x\n", reg);
			goto done;
		}

		SMB347_LOG("Command register %x = 0x%02x\n", reg, val);

		status = 0;
	} else if (argc == 4) {
		/* Write to command register */
		val = (u8)simple_strtol(argv[3], NULL, 16);

		if (smb347_i2c_write(reg, &val, 1)) {
			SMB347_LOG("Unable to write to command register %x\n", reg);
			goto done;
		}

		SMB347_LOG("Wrote 0x%02x to command register %x\n", val, reg);

		status = 0;
	} else {
		SMB347_LOG("Invalid command\n");
	}

done:
	return status;
}

static int do_smb347_commands(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int status = 0;

	if (argc >= 3 && !strcmp("status", argv[1])) {
		status = do_smb347_status_register(argc, argv);
	} else if (argc >= 3 && !strcmp("config", argv[1])) {
		status = do_smb347_config_register(argc, argv);
	} else if (argc >= 3 && !strcmp("command", argv[1])) {
		status = do_smb347_command_register(argc, argv);
	} else if (argc == 2 && !strcmp("apsd", argv[1])) {
		SMB347_LOG("Re-doing APSD...\n");
		int apsd = smb347_redo_apsd();

		if (apsd != -1)
			SMB347_LOG("%s\n", smb347_apsd_result_string(apsd));
	} else {
		SMB347_LOG("Invalid command\n");
		status = -1;
	}

	return status;
}

int smb347_init(int *wall_charger)
{
	int status = -1, apsd = 0;
	u8 val = 0, mode = 0, vusb = 0, tmp = 0;
	u32 reg = 0;
	u8 usb_detect = 0;

    int i2c_addr;

	/* Disable USB2PHY charger detection */
	reg = __raw_readl(OMAP44XX_CTRL_BASE + CONTROL_USB2PHYCORE);
	__raw_writel(reg | 0x40000000,
			OMAP44XX_CTRL_BASE + CONTROL_USB2PHYCORE);

	/* Check if we have VBUS */
	if (twl6030_i2c_read_u8(TWL6030_CHIP_CHARGER, &vusb, CONTROLLER_STAT1)) {
		SMB347_LOG("Unable to communicate with PMIC\n");
		goto done;
	}

	if (!(vusb & (1 << 2))) {
		/* Nothing on USB, skip USB detection */
		SMB347_LOG("Nothing on USB\n");
		status = 0;
		goto done;
	}
	else {
		usb_detect = 1;
	}

	/* Power down USB PHY */
	reg = __raw_readl(OMAP44XX_CTRL_BASE + CONTROL_DEV_CONF);
	__raw_writel(reg | 0x01, OMAP44XX_CTRL_BASE + CONTROL_DEV_CONF);

	/* Disable VUSB to shut off the USB PHY transceivers */
	twl6030_disable_vusb();

	/* Enable VUSB */
	twl6030_enable_vusb();

	SMB347_LOG("Detecting USB source...\n");

    i2c_addr = smb347_i2c_probe();

    if (i2c_addr)
        SMB347_LOG("SMB347 at I2C Address %02x\n", i2c_addr);
    else
        SMB347_LOG("SMB347 not Detected\n");

	/*
	 * Bit 0 of register 6 is used to determine
	 * whether the unit has rebooted due to
	 * re-doing APSD
	 */
	if (smb347_i2c_read(0x6, &val, 1))
		goto done;

	if (val & 0x01) {
		/* First boot, switch on the flag */
		val &= ~0x01;

		/* Use register control for charging mode */
		val &= ~(1 << 5 | 1 << 6);
		val |= (1 << 5);

		if (smb347_i2c_read(0x7, &mode, 1))
			goto done;

		/*
		 * Disable thermistor monitor to avoid shutdown
		 * due to missing battery when we do APSD
		 */
		mode |= (1 << 4);

		if (smb347_config_enable(1))
			goto done;

		if (smb347_i2c_write(0x06, &val, 1))
			goto done;

		if (smb347_i2c_write(0x07, &mode, 1))
			goto done;

		/* Set taper current to 150 mA */
		if (smb347_i2c_read(0x00, &tmp, 1))
			goto done;

		tmp &= ~(1 << 2);
		tmp |= 0x03;

		if (smb347_i2c_write(0x00, &tmp, 1))
			goto done;

		if (smb347_config_enable(0))
			goto done;

		/* Enable USB5 mode */
		if (smb347_i2c_read(SMB347_COMMAND_REG_B, &mode, 1))
			goto done;

		mode &= ~(1 << 0);
		mode |= (1 << 1);

		if (smb347_i2c_write(SMB347_COMMAND_REG_B, &mode, 1))
			goto done;

		/* Re-do APSD */
		apsd = smb347_redo_apsd();
	} else {
		/* Second boot */

		/*
		 * Unit must have rebooted due to re-do APSD,
		 * get the result of the detection
		 */
		if (smb347_i2c_read(SMB347_STATUS_REG_D, &apsd, 1))
			goto done;
	}

	/* Clean up the flag */
	val |= 0x01;

	/* Use register control for USB1/5/HC mode */
	val &= ~(1 << 4);

	/* We can now safely re-enable thermistor monitor */
	if (smb347_i2c_read(0x07, &mode, 1))
		goto done;

	mode &= ~(1 << 4);

	if (smb347_config_enable(1))
		goto done;

	if (smb347_i2c_write(0x06, &val, 1))
		goto done;

	if (smb347_i2c_write(0x07, &mode, 1))
		goto done;

	/*
	 * Disable the suspend mode.
	 */
	if (smb347_i2c_read(0x30, &mode, 1))
		goto done;

	mode &= ~(1 << 2);

	if (smb347_i2c_write(0x30, &mode, 1))
		goto done;

	if (smb347_config_enable(0))
		goto done;

	if (apsd != -1 && smb347_apsd_result(apsd) != -1) {
		SMB347_LOG("%s detected\n", smb347_apsd_result_string(apsd));

		if (smb347_apsd_result(apsd) == SMB347_APSD_RESULT_DCP &&
				wall_charger)
			*wall_charger = 1;

		if (smb347_apsd_result(apsd) == SMB347_APSD_RESULT_OTHER) {
			/*
			 * 3rd party chargers we need to manually enable
			 * HC mode
			 */
			if (smb347_i2c_read(SMB347_COMMAND_REG_B, &mode, 1))
				goto done;

			/*
			 * FIXME: If on a dead battery and turn on HC mode,
			 * device will reset, so do NOT turn on HC mode
			 * when there's no battery
			 */
			u16 capacity = 0;

#if defined(CONFIG_MACH_BOWSER_SUBTYPE_TATE)
			if (bq27541_capacity(&capacity)) {
				mode |= 0x02;
			} else {
				mode |= 0x03;
			}
#else
			mode |= 0x03;
#endif

			if (smb347_i2c_write(SMB347_COMMAND_REG_B, &mode, 1))
				goto done;
		}
	}

	status = 0;
done:
	/* Enable USB PHY */
	reg = __raw_readl(OMAP44XX_CTRL_BASE + CONTROL_DEV_CONF);
	__raw_writel(reg & ~0x01, OMAP44XX_CTRL_BASE + CONTROL_DEV_CONF);

	if (usb_detect && (*wall_charger != 1)) {
		gnUSBAttached = 1;
	}

	return status;
}

U_BOOT_CMD(
	smb347, 4, 0, do_smb347_commands,
	"smb347  - Commands on smb347 charger\n",
	"status [a-e]\n    Reads and parses SMB347 status registers A-E\n"
        "smb347 command <reg>\n    Reads SMB347 command register <reg>\n"
	"smb347 command <reg> <value>\n    Writes <val> to SMB347 command register <reg>\n"
	"smb347 config enable\n    Enable writes to SMB347 config registers\n"
	"smb347 config disable\n    Disable writes to SMB347 config registers\n"
	"smb347 config <reg>\n    Reads SMB347 config register <reg>\n"
	"smb347 config <reg> <value>\n    Writes <val> to SMB347 config register <reg>\n"
	"smb347 apsd\n    Re-do Automatic Power Source Detection (APSD)\n"
);

#endif
