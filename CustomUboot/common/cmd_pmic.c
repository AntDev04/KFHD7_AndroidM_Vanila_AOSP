#include <common.h>
#include <command.h>

#include <i2c.h>

#ifdef CONFIG_CMD_PMIC

DECLARE_GLOBAL_DATA_PTR;

// exported by omap4 i2c driver
int select_bus(int, int);

// debug code
#undef DEBUG_PMIC
#ifdef DEBUG_PMIC 
#define DEBUGF(fmt,args...) printf(fmt ,##args)
#else
#define DEBUGF(fmt,args...)
#endif

// data structures
typedef struct _spms_reg_map
{
	u8 grp;
	u8 trans;
	u8 state;
	u8 _reserved;
	u8 voltage;
} smps_reg_map;

typedef struct _ldo_reg_map
{
	u8 grp;
	u8 trans;
	u8 state;
	u8 voltage;
} ldo_reg_map;

typedef struct _voltage_rail
{
	const char *name;
	u8 base_address;
	u8 bit_offset;
} voltage_rail;


// configuration information
static voltage_rail ldo_rail_list[] = {
	{
		.name = "vana",
		.base_address = 0x80,
		.bit_offset = 0,
	},
	{
		.name = "vaux1",
		.base_address = 0x84,
		.bit_offset = 0,
	},
	{
		.name = "vaux2",
		.base_address = 0x88,
		.bit_offset = 0,
	},
	{
		.name = "vaux3",
		.base_address = 0x8C,
		.bit_offset = 0,
	},
	{
		.name = "vcxio",
		.base_address = 0x90,
		.bit_offset = 0,
	},
	{
		.name = "vdac",
		.base_address = 0x94,
		.bit_offset = 0,
	},
	{
		.name = "vmmc",
		.base_address = 0x98,
		.bit_offset = 0,
	},
	{
		.name = "vpp",
		.base_address = 0x9C,
		.bit_offset = 0,
	},
	{
		.name = "vusb",
		.base_address = 0xA0,
		.bit_offset = 0,
	},
	{
		.name = "vusim",
		.base_address = 0xA4,
		.bit_offset = 0,
	},
};

static voltage_rail smps_rail_list[] = {
	{
		.name = "v1v29",
		.base_address = 0x40,
		.bit_offset = 0,
	},
	{
		.name = "v1v8",
		.base_address = 0x46,
		.bit_offset = 1,
	},
	{
		.name = "v2v1",
		.base_address = 0x4C,
		.bit_offset = 2,
	},
	{
		.name = "vcore1",
		.base_address = 0x52,
		.bit_offset = 3,
	},
	{
		.name = "vcore2",
		.base_address = 0x58,
		.bit_offset = 4,
	},
	{
		.name = "vcore3",
		.base_address = 0x5E,
		.bit_offset = 5,
	},
	{
		.name = "vmem",
		.base_address = 0x64,
		.bit_offset = 6,
	},
};

#define NUM_SMPS_RAILS (sizeof(smps_rail_list)/sizeof(smps_rail_list[0]))
#define NUM_LDO_RAILS  (sizeof(ldo_rail_list)/sizeof(ldo_rail_list[0]))

// pmic wrapper function for i2c_read
int pmic_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	int status = 0;

	if ((buffer == NULL) || (len <= 0))
	{
		printf("invalid i2c read attempted! (0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n", chip, addr, alen, buffer, len);
		return -1;
	}

	DEBUGF("reading %d bytes from address 0x%x on chip 0x%x\n", len, addr, chip);
	status = i2c_read(chip, addr, alen, buffer, len);
	
	if (status == 0)
	{
		int i = 0;
		DEBUGF("READ: ");
		for (i = 0; i < len; i++)
		{
			DEBUGF("0x%x ", buffer[i]);
		}
		DEBUGF("\n");
	}
	return status;
}

// pmic wrapper function for i2c_write
int pmic_write(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	int status = 0;
	int i = 0;

	if ((buffer == NULL) || (len <= 0))
	{
		printf("invalid i2c write attempted! (0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n", chip, addr, alen, buffer, len);
		return -1;
	}

	DEBUGF("Writing %d bytes to address 0x%x on chip 0x%x:", len, addr, chip);
	for (i = 0; i < len; i++)
	{
		DEBUGF("0x%x ", buffer[i]);
	}
	DEBUGF("\n");

	status = i2c_write(chip, addr, alen, buffer, len);

	return status;
}

// sets voltage for a given rail
int configure_voltage(char *rail_name, u8 voltage_code)
{
	int status = 0;
	int i = 0;

	for (i = 0; i < NUM_SMPS_RAILS; i++)
	{
		if (strcmp(rail_name, smps_rail_list[i].name) == 0)
		{
			smps_reg_map current_smps = {0};

			// read voltage rail
			status = pmic_read(0x48, smps_rail_list[i].base_address, 1, (uchar*)&current_smps, 5);
			if (status != 0)
			{
				printf("i2c read failed!\n");
				return status;
			}
			else
			{
				current_smps.voltage = ((current_smps.voltage & 0xC0) | (voltage_code & 0x3F));

				status = pmic_write(0x48, smps_rail_list[i].base_address+4, 1, &current_smps.voltage, 1);
				if (status != 0)
				{
					printf("i2c write failed!\n");
					return status;
				}
			}
		}
	}

	for (i = 0; i < NUM_LDO_RAILS; i++)
	{
		if (strcmp(rail_name, ldo_rail_list[i].name) == 0)
		{
			ldo_reg_map current_ldo = {0};

			// check for invalid voltage code
			if (voltage_code >= 0x19)
			{
				printf("invalid voltage code 0x%x for LDO rail!\n", voltage_code);
				return -1;
			}

			// read voltage rail
			status = pmic_read(0x48, ldo_rail_list[i].base_address, 1, (uchar*)&current_ldo, 4);
			if (status != 0)
			{
				printf("i2c read failed!\n");
				return status;
			}
			else
			{
				current_ldo.voltage = ((current_ldo.voltage & 0xE0) | (voltage_code & 0x1F));

				status = pmic_write(0x48, ldo_rail_list[i].base_address+3, 1, &current_ldo.voltage, 1);
				if (status != 0)
				{
					printf("i2c write failed!\n");
					return status;
				}
			}
		}
	}
	
	return status;
}

// returns voltage (in micro-volts) of LDO rail
int get_ldo_voltage(u8 voltage_code)
{
	int micro_volts = 0;

	if (voltage_code == 0)
	{
		micro_volts = 0;
	}
	else if (voltage_code < 0x19)
	{
		micro_volts = 1000000 + (100000 * (voltage_code - 1));
	}
	else
	{
		if (voltage_code == 0x1F)
		{
			micro_volts = 2750000;
		}
		else
		{
			// 0x19 -> 0x1E
			// reserved voltage range
			micro_volts = -1;
		}
	}
	
	return micro_volts;
}

// returns voltage (in micro-volts) of SMPS power rail
int get_smps_voltage(u8 voltage_code, u8 is_offset, u8 is_extended)
{
	int micro_volts = 0;

	if (voltage_code >= 0x3A)
	{
		if (is_extended)
		{
			// special discrete values
			switch (voltage_code)
			{
				case 0x3A: micro_volts = (is_offset) ? 2084000 : 4167000; break;
				case 0x3B: micro_volts = (is_offset) ? 2315000 : 2315000; break;
				case 0x3C: micro_volts = (is_offset) ? 2778000 : 2778000; break;
				case 0x3D: micro_volts = (is_offset) ? 2932000 : 2932000; break;
				case 0x3E: micro_volts = (is_offset) ? 3241000 : 3241000; break;
				default:
					   micro_volts = -1;
			}
		}
		else
		{
			// special discrete values
			switch (voltage_code)
			{
				case 0x3A: micro_volts = 1367400; break;
				case 0x3B: micro_volts = 1519300; break;
				case 0x3C: micro_volts = 1823100; break;
				case 0x3D: micro_volts = 1924400; break;
				case 0x3E: micro_volts = 2127000; break;
				default:
					   micro_volts = -1;
			}
		}
	}
	else
	{	
		// calculate voltage programatically
		if (voltage_code == 0)
		{
			micro_volts = 0;
		}
		else
		{
			micro_volts = 607700;
			micro_volts += 12660 * (voltage_code - 1);
			if (is_offset)
			{
				micro_volts += 101300;					
			}
			if (is_extended)
			{
				micro_volts = (micro_volts * 64) / 21;
			}
		}
	}

	return micro_volts;
}

// verifies PMIC is present and outputs verification as string
int verify_pmic_present()
{	
	int status = 0;
	u8 buf[4] = {0};	
	u8 expected_data[4] = {0x51, 0x04, 0x30, 0xC0};

	status = pmic_read(0x49, 0x00, 0x01, buf, 4);
	if (status != 0)
	{
		printf("TWL6030 verification failed: I2C read failed!\n");
		return status;
	}

	if (memcmp(buf, expected_data, 4) != 0)
	{
		printf("TWL6030 verification test failed! Chip reported data: 0x%x\n", (*(u32*)buf));
		return -1;
	}

	printf("TWL6030 verification succeeded!\n");
	return 0;
}

// prints the rail specified by name_filter, or ALL if NULL
int print_rails(char *name_filter)
{
	int status = 0;
	int i = 0;
	u8 smps_mult = 0;
	u8 smps_offset = 0;
	u8 print_all = (name_filter == NULL);

	status = pmic_read(0x48, 0xE3, 1, &smps_mult, sizeof(smps_mult));
	if (status != 0)
	{
		printf("unable to read SMPS_MULT register\n");
		return status;
	}

	status = pmic_read(0x48, 0xE0, 1, &smps_offset, sizeof(smps_offset));
	if (status != 0)
	{
		printf("unable to read SMPS_OFFSET register\n");
		return status;
	}

	// dump all SMPS voltage levels
	for (i = 0; i < NUM_SMPS_RAILS; i++)
	{
		smps_reg_map current_smps = {0};

		if (print_all || (strcmp(smps_rail_list[i].name, name_filter) == 0))
		{
			status = pmic_read(0x48, smps_rail_list[i].base_address, 1, (uchar*)&current_smps, 5);
			if (status != 0)
			{
				printf("i2c read failed for register %s!\n", smps_rail_list[i].name);
			}
			else
			{
				u8 voltage_code = current_smps.voltage & 0x3F;
				u8 is_offset = ((smps_offset & (1 << smps_rail_list[i].bit_offset)) &&
						(smps_offset & 0x80));
				u8 is_extended = ((smps_mult & (1 << smps_rail_list[i].bit_offset)) &&
						(smps_mult & 0x80));
				u32 micro_volts = get_smps_voltage(voltage_code, is_offset, is_extended);

				u32 volts = micro_volts/1000000;
				u32 milli_volts = (micro_volts - (volts * 1000000))/1000;
				printf("%s: %d.%dV\n", smps_rail_list[i].name, volts, milli_volts);
			}
		}
	}

	// dump all LDO voltage levels
	for (i = 0; i < NUM_LDO_RAILS; i++)
	{
		ldo_reg_map current_ldo = {0};

		if (print_all || (strcmp(ldo_rail_list[i].name, name_filter) == 0))
		{
			status = pmic_read(0x48, ldo_rail_list[i].base_address, 1, (u8*)&current_ldo, 4);
			if (status != 0)
			{
				printf("i2c read failed for register %s!\n", ldo_rail_list[i].name);
			}
			else
			{
				u8 voltage_code = current_ldo.voltage & 0x1F;

				u32 micro_volts = get_ldo_voltage(voltage_code);

				u32 volts = micro_volts/1000000;
				u32 milli_volts = (micro_volts - (volts * 1000000))/1000;
				printf("%s: %d.%dV\n", ldo_rail_list[i].name, volts, milli_volts);
			}
		}
	}

	return 0;
}

int do_pmic(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int status = 0;

	// early exit
	if (argc <= 1)
	{
		printf("no command specified. see help!\n");
		return 0;
	}

	// switch bus/speed
	status = select_bus(PMIC_I2C_BUS, PMIC_I2C_SPEED);
	if (status != 0)
	{
		printf("unable to select PMIC i2c bus!\n");
		return status;
	}	

	// commands
	if (strcmp(argv[1], "verify") == 0)
	{
		status = verify_pmic_present();
	}
	else if (strcmp(argv[1], "dump-voltages") == 0)
	{
		status = print_rails(NULL);
	}
	else if (strcmp(argv[1], "get-voltage") == 0)
	{
		if (argc == 3)
		{
			status = print_rails(argv[2]);
		}
		else
		{
			printf("invalid usage. please see help!\n");
			return 1;
		}
	}
	else if (strcmp(argv[1], "set-voltage") == 0)
	{
		if (argc == 4)
		{
			u8 voltage_code = simple_strtoul(argv[3], NULL, 16);
			DEBUGF("configuring rail %s to voltage code 0x%x\n", argv[2], voltage_code);
			status = configure_voltage(argv[2], voltage_code);
		}
		else
		{
			printf("invalid usage. please see help!\n");
			return 1;
		}
	}
	else if (strcmp(argv[1], "read") == 0)
	{
		if (argc == 4)
		{
			u8 chip = simple_strtoul(argv[2], NULL, 16);
			u8 addr = simple_strtoul(argv[3], NULL, 16);
			u8 buf = 0;

			status = pmic_read(chip, addr, 1, &buf, 1);

			printf("PMIC[0x%x][0x%x] = 0x%x\n", chip, addr, buf);
		}
		else
		{
			printf("invalid usage. please see help!\n");
			return 1;
		}
	}
	else if (strcmp(argv[1], "write") == 0)
	{
		if (argc == 5)
		{
			u8 chip = simple_strtoul(argv[2], NULL, 16);
			u8 addr = simple_strtoul(argv[3], NULL, 16);
			u8 buf = simple_strtoul(argv[4], NULL, 16);

			status = pmic_write(chip, addr, 1, &buf, 1);

			printf("PMIC[0x%x][0x%x] = 0x%x\n", chip, addr, buf);
		}
		else
		{
			printf("invalid usage. please see help!\n");
			return 1;
		}
	

	}
	else
	{
		printf("unsupported command. please see help!\n");
		status = -1;
	}

	return status;
}

U_BOOT_CMD(
	pmic,	5,	0,	do_pmic,
	"pmic	- tests pmic functionality\n",
	"Commands:\n"
	"\tverify - validates twl6030 is at expected address on i2c bus\n"
	"\tdump-voltages - dumps all smps/ldo voltages\n"
	"\tget-voltage <rail name> - gets configured voltage for specified rail\n"
	"\tset-voltage <rail name> 0x<voltage_code> - sets configured voltage for specified rail\n"
);

#endif	/* CFG_CMD_PMIC */
