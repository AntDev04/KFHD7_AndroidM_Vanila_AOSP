/*
 * (C) Copyright 2011
 * Quanta Computer Inc.
 *
 */

//**************************************************************************************
// Global Define
//**************************************************************************************

#define MAX_DELAY_COUNTER       1000

//Frame buffer
#define FRAME_BUFFER_ADDR       0x82000000
#define GUNZIP_BUFFER_ADDR	0x81000000

//
//KC2 timming setting
//
#define	LCD_WIDTH		800
#define	LCD_HIGHT		1280

#define	LCD_HFP		        102
#define	LCD_HSW		        9
#define	LCD_HBP		        1
#define	LCD_VFP		        10
#define	LCD_VSW		        1
#define	LCD_VBP		        10

#define DSI_HSA                 0
#define DSI_HFP                 23
#define DSI_HBP                 58
#define DSI_VSA                 1
#define DSI_VFP                 10
#define DSI_VBP                 10

#define	LCK_DIV			1
#define	PCK_DIV			2

#define	REGM			178
#define	REGN			16
#define	REGM_DISPC		6
#define	REGM_DSI		5
#define	LP_CLK_DIV		9

#define	DSI_VACT		LCD_HIGHT
#define	DSI_TL			684

#define LCD_BPP			2 /* Set RGB565 for uboot */
#define LCD_PIXELS		(LCD_WIDTH * LCD_HIGHT * LCD_BPP)

#define GPIO_BACKLIGHT_CABC_EN		37	/* enabling backlight CABC (Content Adaptive Backlight Control) */
#define GPIO_BACKLIGHT_EN_O2M		25	/* enabling O2Micro 9979 */
#define GPIO_LCD_ENABLE			35	/* enabling LCD panel */
#define GPIO_BACKLIGHT_PWM		190	/* GPTimer 10 */

//**************************************************************************************
// Functions Define
//**************************************************************************************

#define RegRead32(base, offset)		     __raw_readl(base+offset)
#define RegWrite32(base, offset, value)	     __raw_writel(value, base+offset)
#define RegSet32(base, offset, bitmask)      RegWrite32(base, offset, RegRead32(base, offset)|(bitmask))
#define RegClear32(base, offset, bitmask)    RegWrite32(base, offset, RegRead32(base, offset)&~(bitmask))

#define DDR_CLK			(2 * REGM * (38400000 / REGN) / 4)
#define N2DDR(ns) 		((ns * (DDR_CLK / 1000 / 1000) + 999) / 1000)
#define THS_PREPARE		(N2DDR(70) +2)
#define THS_PREPARE_THE_ZERO	69 /* (N2DDR(175) + 2) */
#define THS_TRAIL		(N2DDR(60) + 5)
#define THS_EXIT		(N2DDR(145))
#define TLPX_HALF		(N2DDR(25))
#define TCLK_TRAIL		(N2DDR(60) + 2)
#define TCLK_PREPARE		(N2DDR(65))
#define TCLK_ZERO		74 /* (N2DDR(260)) */
#define TCLK_POST		(N2DDR(60) + 26)
#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))
#define DDR_CLK_PRE 		31 /*(DIV_ROUND_UP(20 + 2*TLPX_HALF + TCLK_ZERO + TCLK_PREPARE, 4))*/
#define DDR_CLK_POST		14 /*(DIV_ROUND_UP(TCLK_POST + THS_TRAIL, 4) + 2)*/
#define TTAGET			4

//**************************************************************************************
// OMAP4 Address Define
//**************************************************************************************

//SYSCTRL_PADCONF_CORE Register
#define	SYSCTRL_PADCONF_CORE	                  0x4A100000
#define	CONTROL_DSIPHY			          0x618

//PRCM, DSS_CM2 Registers
#define	CM2_DSS					  0x4A009100
#define	CM_DSS_DSS_CLKCTRL			  0x20

//CLK Mask
#define	CM_DSS_DSS_CLKCTRL_MODULEMODE_MASK	  (0x3<<0)
#define	CM_DSS_DSS_CLKCTRL_MODULEMODE_ENABLE	  (2<<0)
#define	CM_DSS_DSS_CLKCTRL_OPTFCLKEN_TV_FCLK	  (1<<11)
#define	CM_DSS_DSS_CLKCTRL_OPTFCLKEN_SYS_CLK	  (1<<10)
#define	CM_DSS_DSS_CLKCTRL_OPTFCLKEN_48MHZ_CLK	  (1<<9)
#define	CM_DSS_DSS_CLKCTRL_OPTFCLKEN_DSSCLK	  (1<<8)

//----------------------------------------------------------
// DSS Reg
//----------------------------------------------------------

#define DSS_BASE			          0x48040000 //L4 Base Address
#define	DSS_SYSCONFIG				  0x10
#define	DSS_CTRL				  0x40
#define	DSS_STATUS				  0x5C
#define DSS_SYSSTATUS			          0x0014

//----------------------------------------------------------
// DSI Reg
//----------------------------------------------------------
#define DSI_BASE		        0x48044000 //L4 Base Address

// DSI Protocol Engine
#define DSI_REVISION			0x0000
#define DSI_SYSCONFIG			0x0010
#define DSI_SYSSTATUS			0x0014
#define DSI_IRQSTATUS			0x0018
#define DSI_IRQENABLE			0x001C
#define DSI_CTRL			0x0040
#define DSI_COMPLEXIO_CFG1		0x0048
#define DSI_COMPLEXIO_IRQ_STATUS	0x004C
#define DSI_COMPLEXIO_IRQ_ENABLE	0x0050
#define DSI_CLK_CTRL			0x0054
#define DSI_TIMING1			0x0058
#define DSI_TIMING2			0x005C
#define DSI_VM_TIMING1			0x0060
#define DSI_VM_TIMING2			0x0064
#define DSI_VM_TIMING3			0x0068
#define DSI_CLK_TIMING			0x006C
#define DSI_TX_FIFO_VC_SIZE		0x0070
#define DSI_RX_FIFO_VC_SIZE		0x0074
#define DSI_COMPLEXIO_CFG2		0x0078
#define DSI_RX_FIFO_VC_FULLNESS		0x007C
#define DSI_VM_TIMING4			0x0080
#define DSI_TX_FIFO_VC_EMPTINESS	0x0084
#define DSI_VM_TIMING5			0x0088
#define DSI_VM_TIMING6			0x008C
#define DSI_VM_TIMING7			0x0090
#define DSI_STOPCLK_TIMING		0x0094
#define DSI_VC_CTRL(n)			0x0100 + (n * 0x20)
#define DSI_VC_TE(n)			0x0104 + (n * 0x20)
#define DSI_VC_LONG_PACKET_HEADER(n)	0x0108 + (n * 0x20)
#define DSI_VC_LONG_PACKET_PAYLOAD(n)	0x010C + (n * 0x20)
#define DSI_VC_SHORT_PACKET_HEADER(n)	0x0110 + (n * 0x20)
#define DSI_VC_IRQSTATUS(n)		0x0118 + (n * 0x20)
#define DSI_VC_IRQENABLE(n)		0x011C + (n * 0x20)

// DSIPHY_SCP
#define DSI_DSIPHY_CFG0			0x200 + 0x0000
#define DSI_DSIPHY_CFG1			0x200 + 0x0004
#define DSI_DSIPHY_CFG2			0x200 + 0x0008
#define DSI_DSIPHY_CFG5			0x200 + 0x0014

// DSI Rev 3.0 Registers
#define DSI_DSIPHY_CFG8			0x200 + 0x0020)
#define DSI_DSIPHY_CFG12		0x200 + 0x0030)
#define DSI_DSIPHY_CFG14		0x200 + 0x0038)
#define DSI_TE_HSYNC_WIDTH(n)		0xA0 + (0xC * n)
#define DSI_TE_VSYNC_WIDTH(n)		0xA4 + (0xC * n)
#define DSI_TE_HSYNC_NUMBER(n)		0xA8 + (0xC * n)

// DSI_PLL_CTRL_SCP
#define DSI_PLL_CONTROL			0x300 + 0x0000
#define DSI_PLL_STATUS			0x300 + 0x0004
#define DSI_PLL_GO			0x300 + 0x0008
#define DSI_PLL_CONFIGURATION1		0x300 + 0x000C
#define DSI_PLL_CONFIGURATION2		0x300 + 0x0010

//DSI VC friendly define
#define DSI_VC0_CTRL                    DSI_VC_CTRL(0)
#define DSI_VC1_CTRL                    DSI_VC_CTRL(1)
#define DSI_VC2_CTRL                    DSI_VC_CTRL(2)
#define DSI_VC3_CTRL                    DSI_VC_CTRL(3)

#define DSI_VC0_IRQENABLE               DSI_VC_IRQENABLE(0)
#define DSI_VC1_IRQENABLE               DSI_VC_IRQENABLE(1)
#define DSI_VC2_IRQENABLE               DSI_VC_IRQENABLE(2)
#define DSI_VC3_IRQENABLE               DSI_VC_IRQENABLE(3)

#define DSI_VC0_IRQSTATUS               DSI_VC_IRQSTATUS(0)
#define DSI_VC1_IRQSTATUS               DSI_VC_IRQSTATUS(1)
#define DSI_VC2_IRQSTATUS               DSI_VC_IRQSTATUS(2)
#define DSI_VC3_IRQSTATUS               DSI_VC_IRQSTATUS(3)

#define DSI_VC0_SHORT_PACKET_HEADER     DSI_VC_SHORT_PACKET_HEADER(0)
#define DSI_VC1_SHORT_PACKET_HEADER     DSI_VC_SHORT_PACKET_HEADER(1)

//----------------------------------------------------------
// DISPC Reg
//----------------------------------------------------------

#define DISPC_BASE		       0x48041000 //L4 Base Address

#define DISPC_GFX_BA0			0x80
#define DISPC_GFX_BA1			0x84
#define DISPC_GFX_SIZE			0x8C
#define DISPC_GFX_ATTRIBUTES		0xA0

#define DISPC_IRQENABLE			0x1C
#define DISPC_DEFAULT_COLOR0		0x4C
#define DISPC_DEFAULT_COLOR1		0x50

#define DISPC_TIMING_H			0x64
#define DISPC_TIMING_V			0x0068


#define	DISPC_TIMING_H2			0x400
#define	DISPC_TIMING_V2			0x404


#define DISPC_DIVISOR			0x70
#define DISPC_SIZE_LCD			0x7C
#define DISPC_CONTROL1			0x40
#define DISPC_SYSCONFIG			0x10

#define	DISPC_CONFIG1			0x44

#define	DISPC_IRQSTATUS			0x18

#define	DISPC_WB_ATTRIBUTES		0x570
#define	DISPC_WB_BUF_SIZE_STATUS	0x588

#define	DISPC_GFX_BUF_SIZE_STATUS	0xA8
#define	DISPC_VID1_BUF_SIZE_STATUS	0xD4
#define	DISPC_VID2_BUF_SIZE_STATUS	0x164
#define	DISPC_VID2_ATTRIBUTES		0x15C
#define	DISPC_VID3_BUF_SIZE_STATUS	0x388
#define	DISPC_VID3_ATTRIBUTES		0x370

#define	DISPC_TRANS_COLOR0		0x54
#define	DISPC_GFX_ROW_INC		0xAC
#define	DISPC_GFX_PIXEL_INC		0xB0
#define	DISPC_GFX_POSITION		0x88
#define	DISPC_GLOBAL_ALPHA		0x74
#define	DISPC_GFX_BUF_THRESHOLD		0xA4

#define LP8552_I2C_ADDR		0x2C

#define LP8552_BRT_CTRL_REG		0x00
#define LP8552_DEV_CTRL_REG		0x01

#define LP8552_EEPROM_ADDR0		0xA0
#define LP8552_EEPROM_ADDR1		0xA1

#define LP8552_BRT_LEVEL		0x64
#define LP8552_BRT_LEVEL2		0x0A
#define LP8552_NO_SLOPE_TIME		0xF0
#define LP8552_BRT_MODE_REG_BASED	0x04
#define LP8552_BL_CTL_ENABLE		0x01
