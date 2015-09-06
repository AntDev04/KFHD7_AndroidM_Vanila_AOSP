/*
 * battery_loop.c
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
#include <twl6030.h>
#include "tate_lcd.h"

#define mdelay(n) { unsigned long msec = (n); while (msec--) udelay(1000); }

/*
 * This code should be included on platforms that has a gas gauge,
 * but only BQ27541 is what we support now
 */
#if defined(CONFIG_BQ27541)

int bq27541_capacity(u16 *capacity);
int bq27541_voltage(u16 *voltage);
int bq27541_current(s16 *current);
int bq27541_temperature(s16 *temperature);

#define TEMPERATURE_COLD_LIMIT			-19	/* -19 C */
#define TEMPERATURE_HOT_LIMIT			59	/* 59 C */
#define LOW_BATTERY_CAPACITY_LIMIT		2	/* 2% */
#define LOW_BATTERY_VOLTAGE_LIMIT		3430	/* 3.43 V */
#define LOW_BATTERY_SPLASH_VOLTAGE_LIMIT	3100	/* 3.1 V */
#define LOW_CURRENT_LIMIT 1400 /* 1400 mA (Wall Charger) */

#define BATTERY_CHRG_ON 0x01
#define BATTERY_CHRG_OFF 0x00

#define TMP103_I2C_BUS	(2)			/* I2C Bus # of TMP103 */
#define TMP103_ADDR	(0x70)			/* I2C Addr of TMP103 */

#define DEVICE_TEMP_BOOT_LIMIT			64 /* in degres C */

static int charge_screen_displayed = 0;

static void battery_charge_loop(void);

int wall_charger_used = 0;

static int tmp103_temperature_read(void *buffer, size_t size)
{
	int status = -1;

	/* Switch to the I2C bus the gas gauge is connected to */
	if (select_bus(TMP103_I2C_BUS, 100)) {
		printf("TMP103: Failed to switch to bus %d\n", TMP103_I2C_BUS);
		goto done;
	}

	if ((status = i2c_read(TMP103_ADDR, 0x0, 1, buffer, size)))
		printf("TMP103: I2C read failed: %d\n", status);

done:
	/* Switch back to the original I2C bus */
	select_bus(0, 100);

	return status;
}

int check_battery_condition(int wall_charger)
{
	u16 voltage = 0, capacity = 0;
	s16 temperature = 0, current = 0;
	u8 tmp103_temp = 0;
	wall_charger_used = wall_charger;

	if (bq27541_voltage(&voltage)) {
		printf("Unable to determine battery voltage\n");
		goto err;
	}

	if (bq27541_capacity(&capacity)) {
		printf("Unable to determine battery capacity\n");
		goto err;
	}

	if (bq27541_current(&current)) {
		printf("Unable to determine battery current\n");
		goto err;
	}

	if (bq27541_temperature(&temperature)) {
		printf("Unable to determine battery temperature\n");
		goto err;
	}

	/* Display battery information */
	printf("Battery: voltage  = %4d mV, current     = %4d mA,\n",
				voltage, current);
	printf("         capacity = %4d %%,  temperature = %3d C\n",
				capacity, temperature);

	if (tmp103_temperature_read(&tmp103_temp, sizeof(tmp103_temp))) {
		printf("Unable to read temperature from TMP103\n");
	}
	printf("TMP103: The Temperature is %d\n", tmp103_temp);

	if (tmp103_temp >= DEVICE_TEMP_BOOT_LIMIT) {
		int timeout;
		printf("Device is too hot (%d C). It is not safe to boot\n", tmp103_temp);

/* The message of over-heating should be always show up. */
//		if (voltage > LOW_BATTERY_SPLASH_VOLTAGE_LIMIT ) {
		if (1) {
		  Init_LCD(); /* this is safe, function checks if LCD is already initialised */
		  show_devicetoohot();
		  //Turn on the Backlight to darker one
		  Set_Brightness(LP8552_BRT_LEVEL2);

		  /* Wait for 4 seconds before shutting down the device */
		  for (timeout = 0; timeout < 4000; timeout++)
		    udelay(1000);
		} else {
		  printf("Unable to display Device Too Hot Screen\n");
		}
		
		printf("Shutting down device...\n");
		twl6030_shutdown();
	}

	/* Until battery has reached a safe limit to boot, stay in loop */
	if ((voltage < LOW_BATTERY_VOLTAGE_LIMIT) || (capacity < LOW_BATTERY_CAPACITY_LIMIT)) {
#if defined(CONFIG_MACH_BOWSER_SUBTYPE_TATE)
		int ms = 0;

		/* We do want to show SOL when wall charger is plugged or just a press on power button without USB. */
		if ((voltage > LOW_BATTERY_SPLASH_VOLTAGE_LIMIT) || wall_charger_used || (current <= 0) ) {
		  Init_LCD();

		  if (current > 0)
		    show_charging();
		  else
		    show_lowbattery();

		  //Turn on the Backlight to darker one
		  Set_Brightness(LP8552_BRT_LEVEL2);

		  charge_screen_displayed = 1;

		  /* Delay for 5 seconds */
		  for (ms = 0; ms < 5000; ms++)
		    udelay(1000);
		  
		  Set_Brightness(0);
		  mdelay(100);
		  Power_off_LCD();
		  mdelay(100);
		  display_disable();
		  mdelay(100);
		} else {
		  printf("Unable to dispay low battery notification\n");
		}

		/* Need to re-enable backlight after battery loop */
		prcm_reinit_battery_chrg(BATTERY_CHRG_ON);
		scale_reinit_vcores(BATTERY_CHRG_ON);
#endif

		battery_charge_loop();

#if defined(CONFIG_MACH_BOWSER_SUBTYPE_TATE)
		scale_reinit_vcores(BATTERY_CHRG_OFF);
		prcm_reinit_battery_chrg(BATTERY_CHRG_OFF);

		Init_LCD();
		Power_on_LCD();
		mdelay(200);
		display_reinit();
		mdelay(100);
		Show_Logo();
#endif
	}

	return 0;

err:
	return -1;
}

static void battery_charge_loop(void)
{
	u16 voltage = 0, capacity = 0;
	s16 current = 0, temperature = 0;
	int ms = 0, sec = 0, discharge_count = 0;

	printf("Entering charge loop...\n");

	while (1) {
		if (bq27541_voltage(&voltage)) {
			printf("Unable to determine battery voltage\n");
			goto skip;
		}

		if (bq27541_capacity(&capacity)) {
			printf("Unable to determine battery capacity\n");
			goto skip;
		}

		if (bq27541_current(&current)) {
			printf("Unable to determine battery current\n");
			goto skip;
		}

		if (bq27541_temperature(&temperature)) {
			printf("Unable to determine battery temperature\n");
			goto skip;
		}

		/* Temperature check */
		if ((temperature > TEMPERATURE_HOT_LIMIT) ||
				(temperature < TEMPERATURE_COLD_LIMIT)) {
			printf("Battery temperature threshold reached: %d C",
					temperature);
			goto shutdown;
		}

		/* USB connection check */
		if (twl6030_get_vbus_status() == 0) {
			printf("No USB connection detected\n");
			goto shutdown;
		}

		/* Charging current check */
		if (current <= 0 && ++discharge_count > 10) {
			printf("Battery is being discharged for too long\n");
			goto shutdown;
		} else if (current > 0) {
			/* Reset discharge count */
			discharge_count = 0;
		}

		/* Software charging time check */
		if (sec > (30 * 60 * 5 * 2)) {
			printf("Battery has been charged for too long: %d s\n",
					sec);
			goto shutdown;
		}

		/* Check if battery is good enough to boot into system */
		if ((voltage >= LOW_BATTERY_VOLTAGE_LIMIT) &&
				(capacity >= LOW_BATTERY_CAPACITY_LIMIT)) {
			printf("Battery is enough to boot into system, "
				"breaking out of loop\n");
			break;
		}

#if 0
		/* Check for Wall Charger and exit if present. */
		if (current > LOW_CURRENT_LIMIT) {
			printf("Charging current is high enough to boot into system.");
			break;
		}
#endif

		/* Display status every 5 seconds */
		printf("t = %4d, voltage = %4d mV, current = %4d mA, "
			"capacity = %3d %%, temperature = %3d C\n",
			sec, voltage, current, capacity, temperature);

		u8 button_state = 0x01, pressed = 0;

skip:
		/* Delay for ~5 seconds, note: hand-tuned loop, be careful! */
		for (ms = 0; ms < 20; ms++) {
			/* Read whether a button was pressed/released */
			u8 data = 0;
			twl6030_i2c_read_u8(TWL6030_CHIP_PM, &data,
				PHOENIX_STS_HW_CONDITION);
			data &= 0x01;

			/* If it's different, then it's been pressed */
			if (button_state ^ data) {
				pressed = 1;
				break;
			}

			button_state = data;

			udelay(50000);
		}

		if ((pressed || ! charge_screen_displayed) && (voltage > LOW_BATTERY_SPLASH_VOLTAGE_LIMIT) ) {
#if defined(CONFIG_MACH_BOWSER_SUBTYPE_TATE)
			int i = 0;
			if (pressed) printf("pwr button pressed\n");
			/* Display the low battery splash for 5 seconds */
			Init_LCD();
			Power_on_LCD();
			mdelay(200);
			display_reinit();
			mdelay(100);
			if (current > 0)
			  show_charging();
			else
			  show_lowbattery();

			charge_screen_displayed = 1;
			//Turn on the Backlight to darker one
			Set_Brightness(LP8552_BRT_LEVEL2);

			/* Compensate for amount of time spent in prev loop */
			for (i = 0; i < 5000 - ms * 50; i++)
				udelay(1000);

			/* Turn off backlight and LCD */
			Set_Brightness(0);
			mdelay(100);
			Power_off_LCD();
			mdelay(100);
			display_disable();
#endif
		} 

		sec += 5;

		continue;
shutdown:
		printf("Shutting down device...\n");
#if defined(CONFIG_TWL6030)
		twl6030_shutdown();
#else
		while (1);
#endif
	}

	printf("Leaving charge loop...\n");
}

#endif
