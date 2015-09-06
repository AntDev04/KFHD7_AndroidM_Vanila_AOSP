#include <common.h>
#include <command.h>

#include <asm/arch/mux.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;


// debug code
#undef DEBUG_GPIO
#ifdef DEBUG_GPIO 
#define DEBUGF(fmt,args...) printf(fmt ,##args)
#else
#define DEBUGF(fmt,args...)
#endif

// gpio module register base addresses
static u32 gpio_base_addresses[] = 
{
	0x4A310000,
	0x48055000,
	0x48057000,
	0x48059000,
	0x4805B000,
	0x4805D000,
};

#define OMAP4_GPIO_OE			0x0134
#define OMAP4_GPIO_CLEARDATAOUT		0x0190
#define OMAP4_GPIO_SETDATAOUT		0x0194
#define OMAP4_GPIO_DATAIN               0x0138


// giant table to map gpio numbers to their pin's pad configuration addresses
#define		OMAP44XX_WKUP_CTRL_BASE		0x4A31E000
#define		CP(x)	(OMAP44XX_CTRL_BASE + CONTROL_PADCONF_##x)
#define		WK(x)	(OMAP44XX_WKUP_CTRL_BASE + CONTROL_WKUP_##x)
#define		INVALID_PADCONF_VALUE 0x00000000

static u32 gpio_padconf_map[] = {
	WK(PAD0_SIM_IO),		// gpio_wk0
	WK(PAD1_SIM_CLK),
	WK(PAD0_SIM_RESET),
	WK(PAD1_SIM_CD),
	WK(PAD0_SIM_PWRCTRL),		// gpio_wk4
	WK(PAD0_FREF_SLICER_IN),
	WK(PAD0_FREF_CLK0_OUT),
	WK(PAD1_FREF_CLK4_REQ),		// gpio_wk7
	WK(PAD0_FREF_CLK4_OUT),
	WK(PAD0_SYS_BOOT6),
	WK(PAD1_SYS_BOOT7),		// gpio_wk10
	CP(DPM_EMU0),			// gpio_11
	CP(DPM_EMU1),
	CP(DPM_EMU2),
	CP(DPM_EMU3),
	CP(DPM_EMU4),
	CP(DPM_EMU5),
	CP(DPM_EMU6),
	CP(DPM_EMU7),
	CP(DPM_EMU8),
	CP(DPM_EMU9),
	CP(DPM_EMU10),
	CP(DPM_EMU11),
	CP(DPM_EMU12),
	CP(DPM_EMU13),
	CP(DPM_EMU14),
	CP(DPM_EMU15),
	CP(DPM_EMU16),
	CP(DPM_EMU17),			// gpio_28
	WK(PAD1_SYS_PWRON_RESET),	// gpio_wk29
	WK(PAD1_FREF_CLK3_REQ),
	WK(PAD0_FREF_CLK3_OUT),
	CP(GPMC_AD8),
	CP(GPMC_AD9),
	CP(GPMC_AD10),
	CP(GPMC_AD11),
	CP(GPMC_AD12),
	CP(GPMC_AD13),
	CP(GPMC_AD14),
	CP(GPMC_AD15),
	CP(GPMC_A16),			// gpio_40
	CP(GPMC_A17),
	CP(GPMC_A18),
	CP(GPMC_A19),
	CP(GPMC_A20),
	CP(GPMC_A21),
	CP(GPMC_A22),
	CP(GPMC_A23),
	CP(GPMC_A24),
	CP(GPMC_A25),
	CP(GPMC_NCS0),		// gpio_50
	CP(GPMC_NCS1),
	CP(GPMC_NCS2),
	CP(GPMC_NCS3),
	CP(GPMC_NWP),
	CP(GPMC_CLK),
	CP(GPMC_NADV_ALE),
	INVALID_PADCONF_VALUE,			//gpio_57
	INVALID_PADCONF_VALUE,
	CP(GPMC_NBE0_CLE),
	CP(GPMC_NBE1),		// gpio_60
	CP(GPMC_WAIT0),
	CP(GPMC_WAIT1),
	CP(HDMI_HPD),		// gpio_63
	CP(HDMI_CEC),
	CP(HDMI_DDC_SCL),
	CP(HDMI_DDC_SDA),
	INVALID_PADCONF_VALUE,		// GPI only
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,		// gpi_70
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,		// gpi_80
	CP(CAM_SHUTTER),
	CP(CAM_STROBE),
	CP(CAM_GLOBALRESET),
	CP(USBB1_ULPITLL_CLK),
	CP(USBB1_ULPITLL_STP),
	CP(USBB1_ULPITLL_DIR),
	CP(USBB1_ULPITLL_NXT),
	CP(USBB1_ULPITLL_DAT0),
	CP(USBB1_ULPITLL_DAT1),
	CP(USBB1_ULPITLL_DAT2),		// gpio_90
	CP(USBB1_ULPITLL_DAT3),
	CP(USBB1_ULPITLL_DAT4),
	CP(USBB1_ULPITLL_DAT5),
	CP(USBB1_ULPITLL_DAT6),
	CP(USBB1_ULPITLL_DAT7),
	CP(USBB1_HSIC_DATA),
	CP(USBB1_HSIC_STROBE),		// gpio_97
	CP(USBC1_ICUSB_DP),
	CP(USBC1_ICUSB_DM),
	CP(SDMMC1_CLK),		// gpio_100
	CP(SDMMC1_CMD),
	CP(SDMMC1_DAT0),
	CP(SDMMC1_DAT1),
	CP(SDMMC1_DAT2),
	CP(SDMMC1_DAT3),
	CP(SDMMC1_DAT4),
	CP(SDMMC1_DAT5),
	CP(SDMMC1_DAT6),
	CP(SDMMC1_DAT7),
	CP(ABE_MCBSP2_CLKX),		// gpio_110
	CP(ABE_MCBSP2_DR),
	CP(ABE_MCBSP2_DX),
	CP(ABE_MCBSP2_FSX),
	CP(ABE_MCBSP1_CLKX),
	CP(ABE_MCBSP1_DR),
	CP(ABE_MCBSP1_DX),
	CP(ABE_MCBSP1_FSX),
	CP(ABE_CLKS),
	CP(ABE_DMIC_CLK1),
	CP(ABE_DMIC_DIN1),		// gpio_120
	CP(ABE_DMIC_DIN2),
	CP(ABE_DMIC_DIN3),
	CP(UART2_CTS),
	CP(UART2_RTS),
	CP(UART2_RX),
	CP(UART2_TX),
	CP(HDQ_SIO),
	CP(I2C2_SCL),
	CP(I2C2_SDA),
	CP(I2C3_SCL),		// gpio_130
	CP(I2C3_SDA),
	CP(I2C4_SCL),
	CP(I2C4_SDA),
	CP(MCSPI1_CLK),
	CP(MCSPI1_SOMI),
	CP(MCSPI1_SIMO),
	CP(MCSPI1_CS0),
	CP(MCSPI1_CS1),
	CP(MCSPI1_CS2),
	CP(MCSPI1_CS3),		// gpio_140
	CP(UART3_CTS_RCTX),
	CP(UART3_RTS_SD),
	CP(UART3_RX_IRRX),
	CP(UART3_TX_IRTX),
	CP(SDMMC5_CLK),
	CP(SDMMC5_CMD),
	CP(SDMMC5_DAT0),
	CP(SDMMC5_DAT1),
	CP(SDMMC5_DAT2),
	CP(SDMMC5_DAT3),	// gpio_150
	CP(MCSPI4_CLK),
	CP(MCSPI4_SIMO),
	CP(MCSPI4_SOMI),
	CP(MCSPI4_CS0),
	CP(UART4_RX),
	CP(UART4_TX),
	CP(USBB2_ULPITLL_CLK),
	CP(USBB2_ULPITLL_STP),
	CP(USBB2_ULPITLL_DIR),
	CP(USBB2_ULPITLL_NXT),		// gpio_160
	CP(USBB2_ULPITLL_DAT0),
	CP(USBB2_ULPITLL_DAT1),
	CP(USBB2_ULPITLL_DAT2),
	CP(USBB2_ULPITLL_DAT3),
	CP(USBB2_ULPITLL_DAT4),
	CP(USBB2_ULPITLL_DAT5),
	CP(USBB2_ULPITLL_DAT6),
	CP(USBB2_ULPITLL_DAT7),
	CP(USBB2_HSIC_DATA),
	CP(USBB2_HSIC_STROBE),		// gpio_170
	CP(UNIPRO_TX0),
	CP(UNIPRO_TY0),
	CP(UNIPRO_TX1),
	CP(UNIPRO_TY1),
	CP(UNIPRO_RX0),
	CP(UNIPRO_RY0),
	CP(UNIPRO_RX1),
	CP(UNIPRO_RY1),
	INVALID_PADCONF_VALUE,
	INVALID_PADCONF_VALUE,		// gpio_180
	CP(FREF_CLK1_OUT),
	CP(FREF_CLK2_OUT),
	CP(SYS_NIRQ2),
	CP(SYS_BOOT0),
	CP(SYS_BOOT1),
	CP(SYS_BOOT2),
	CP(SYS_BOOT3),
	CP(SYS_BOOT4),
	CP(SYS_BOOT5),
	CP(DPM_EMU18),		// gpio_190
	CP(DPM_EMU19),
};

#define GPIO_PIN_COUNT ((sizeof(gpio_padconf_map)/sizeof(gpio_padconf_map[0])))


// read/write helper functions (with logging)
u32 READL(u32 address)
{
	u32 val = 0;

	val = __raw_readl(address);
	DEBUGF("READ[0x%x] = 0x%x\n", address, val);

	return val;
}

void WRITEL(u32 val, u32 address)
{

	DEBUGF("WRITE[0x%x] = 0x%x\n", address, val);
	__raw_writel(val, address);
}

u32 READW(u32 address)
{
	u32 val = 0;

	val = __raw_readw(address);
	DEBUGF("[0x%x] = 0x%x\n", address, val);

	return val;
}

void WRITEW(u16 val, u32 address)
{
	DEBUGF("WRITE[0x%x] = 0x%x\n", address, val);
	__raw_writew(val, address);
}

u32 is_valid_gpio_number(u32 gpio_num)
{
	if (gpio_num >= GPIO_PIN_COUNT)
	{
		DEBUGF("invalid gpio number: %d!\n", gpio_num);
		return 0;
	}

	if (gpio_padconf_map[gpio_num] == INVALID_PADCONF_VALUE)
	{
		DEBUGF("invalid generic use GPIO pin: %x!\n", gpio_num);
		return 0;
	}

	return 1;
}

// gpio helper functions
void configure_pad_mode(u32 gpio_num)
{
	u32 padconf_address = 0;
	u32 padconf_val = 0;

	// error validation
	if (!is_valid_gpio_number(gpio_num))
	{
		return;
	}

	// lookup padconf address and current padconf
	padconf_address = gpio_padconf_map[gpio_num];
	padconf_val = READW(padconf_address);

	// only update to gpio non-input mode if necessary
	if (padconf_val != 0x0003)
	{
		WRITEW(0x0003, padconf_address);
	}
}

void configure_gpio_output(u32 gpio_num)
{
	u32 gpio_base_id = 0; 
	u32 gpio_offset = 0;
	u32 gpio_oe_address = 0;
	u32 oe_val = 0;

	// error validation
	if (!is_valid_gpio_number(gpio_num))
	{
		return;
	}

	// get basic gpio info
	gpio_base_id = gpio_num / 32;
	gpio_offset = gpio_num % 32;
	gpio_oe_address = gpio_base_addresses[gpio_base_id] + OMAP4_GPIO_OE;

	// read current value , mask in our single bit, then write
	oe_val = READL(gpio_oe_address);
	oe_val &= ~(1 << gpio_offset);
	WRITEL(oe_val, gpio_oe_address);
}

static int once = 1;
u32 get_gpio_value(u32 gpio_num)
{
	u32 gpio_base_id = gpio_num / 32;
        u32 gpio_offset = gpio_num % 32;
	u32 read_address = gpio_base_addresses[gpio_base_id];
        u32 value = 0;
        
	// error validation
	if (!is_valid_gpio_number(gpio_num))
	{
                printf("Error, invalid gpio number\n");
		return 0;
	}

        read_address += OMAP4_GPIO_DATAIN;
        
        value = READL(read_address);
        
        value  = value >> gpio_offset;
        value &= 1;
#if 0
        printf("GPIO_DATAIN  0x%x\n", READL(0x4a310138));
        printf("GPIO_IRQSTATUS  0x%x\n", READL(0x4a310020));
        printf("GPIO_IRQSTATUS  0x%x\n", READL(0x4a31002c));
        printf("GPIO_CONTROL  0x%x\n", READL(0x4a310010));
        
        if (once){
                volatile u8 reg = READL(0x4a310010);   //enable wakeup pin
                reg |= (1 << 2);
                WRITEL(reg, 0x4a310010);
                reg = READL(0x4a310044);   //enable wakeup pin
                reg |= (1 << 2);
                WRITEL(reg, 0x4a310044);
                once = 0;
        }
#endif
	return value;
}

void set_gpio_output(u32 gpio_num, u8 set_value)
{
	u32 gpio_base_id = gpio_num / 32;
	u32 gpio_offset = gpio_num % 32;
	u32 write_address = gpio_base_addresses[gpio_base_id];

	// error validation
	if (!is_valid_gpio_number(gpio_num))
	{
		return;
	}

	// get basic gpio info
	gpio_base_id = gpio_num / 32;
	gpio_offset = gpio_num % 32;

	// get appropriate write address to set/clear data out
	write_address = gpio_base_addresses[gpio_base_id];
	if (set_value == 0x00)
	{
		write_address += OMAP4_GPIO_CLEARDATAOUT;
	}
	else
	{
		write_address += OMAP4_GPIO_SETDATAOUT;
	}

	// send command
	WRITEL((u32)(1 << gpio_offset), write_address);
}

int do_gpio(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int status = 0;

	if (strcmp(argv[1], "set") == 0)
	{
		u32 gpio_number = 0;
		u32 gpio_value = 0;

		if (argc != 4)
		{
			printf("error invalid usage\n");
			return -1;
		}

		gpio_number = simple_strtoul(argv[2], NULL, 10);
		gpio_value = simple_strtoul(argv[3], NULL, 10);

		configure_pad_mode(gpio_number);
		configure_gpio_output(gpio_number);
		set_gpio_output(gpio_number, (gpio_value == 0) ? 0 : 1);
	}else if(strcmp(argv[1], "get") == 0){
		u32 gpio_number = 0;
		if (argc != 3)
		{
			printf("error invalid usage\n");
			return -1;
		}

                gpio_number = simple_strtoul(argv[2], NULL, 10);
                printf("Read GPIO Pin %d = 0x%x\n", gpio_number, get_gpio_value(gpio_number));
        }

	return status;
}

U_BOOT_CMD(
	gpio,	4,	0,	do_gpio,
	"gpio set [num] [val] (base 10)\n",
	"Commands:\n"
	"\tset [gpio_num] [value] - configures gpio_[num] pin as a gpio output and sets its output to [value] (base 10)\n"
);

