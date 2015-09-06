/*
 * (C) Copyright 2011
 * Quanta Computer Inc.
 *
 */
//**************************************************************************************
// header include
//**************************************************************************************

#include <common.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <command.h>
#include <asm/arch/mux.h>

#include "tate_lcd.h"

#define mdelay(n) ({ unsigned long msec = (n); while (msec--) udelay(1000); })

extern char is_recovery_mode;

void dsi_write_reg(unsigned char reg, unsigned char data)
{
	uint t;
	int timeout = MAX_DELAY_COUNTER;

	t  = 0x15;		/* Data type: DCS Short WRITE, 1 parameter */
	t |= (reg) << 8;	/* Data byte 1 */
	t |= (data) << 16;	/* Data byte 2 */

	RegWrite32(DSI_BASE, DSI_VC0_SHORT_PACKET_HEADER, t);

	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC0_CTRL) & (1 << 16)) == 1)  && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! send DSI VC0 command time out\n");
}

//**************************************************************************************
// Function: LCD_Normal
// Description: Normal display
//**************************************************************************************
void LCD_Normal(void)
{
	uint t;
	int timeout = MAX_DELAY_COUNTER;

	//printf("[!!] mipi software reset..............\n");
	dsi_write_reg(0x01, 0x00);

	mdelay(20);  //need more than 20ms

	//printf("[!!] swing double mode..............\n");
	// setting to open_100ohms termination resistance @ TCON side
	dsi_write_reg(0xAE, 0x0B);

	//printf("[!!] test mode1..............\n");
	dsi_write_reg(0xEE, 0xEA);

	//printf("[!!] test mode2..............\n");
	dsi_write_reg(0xEF, 0x5F);

	//printf("[!!] bias..............\n");
	dsi_write_reg(0xF2, 0x68);

	// Color enhancement
	dsi_write_reg(0xB3, 0x03);

	dsi_write_reg(0xC8, 0x04);

	dsi_write_reg(0xEE, 0x00);

	dsi_write_reg(0xEF, 0x00);

	//After EVT1.0a, Brightness is controled by CABC
	//printf("[!!] enable CABC..............\n");
	dsi_write_reg(0xB0, 0x7E);
}


//**************************************************************************************
// Function: LCD_Bist
// Description: enter bist mode
//**************************************************************************************
void LCD_Bist(void)
{
	uint t;

	int timeout = MAX_DELAY_COUNTER;

	printf("[!!] mipi software reset..............\n");
	t  = 0x15;		/* Data type: DCS Short WRITE, 1 parameter */
	t |= (0x01) << 8;	/* Data byte 1 */
	t |= (0x00) << 16;	/* Data byte 2 */

	RegWrite32(DSI_BASE, DSI_VC0_SHORT_PACKET_HEADER, t);

	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC0_CTRL) & (1 << 16)) == 1)  && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! send DSI VC0 command time out\n");

	udelay(20000);  //need more than 20ms

	printf("[!!] enter bist mode..............\n");
	t  = 0x15;		/* Data type: DCS Short WRITE, 1 parameter */
	t |= (0xB1) << 8;	/* Data byte 1 */
	t |= (0xEF) << 16;	/* Data byte 2 */

	RegWrite32(DSI_BASE, DSI_VC0_SHORT_PACKET_HEADER, t);

	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC0_CTRL) & (1 << 16)) == 1)  && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! send DSI VC0 command time out\n");
}

//**************************************************************************************
// Function: dsi_videomode_PostInit
// Description:
//**************************************************************************************
void dsi_videomode_PostInit(void)
{
	ulong dwVal = 0;
	int timeout = MAX_DELAY_COUNTER;

	//disbale DSI Interface
	dwVal = RegRead32(DSI_BASE, DSI_CTRL);
	dwVal &= (~(1<<0));
	RegWrite32(DSI_BASE, DSI_CTRL, dwVal);

	//wait for interface enable
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_CTRL) & (1<<0)) != 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! DSI Interface disable timeout!!!\n");

#if 0
	//disable virtual channel 1
	dwVal = RegRead32(DSI_BASE, DSI_VC1_CTRL);
	dwVal &= (~(1<<0));
	RegWrite32(DSI_BASE, DSI_VC1_CTRL, dwVal);

	//wait for virtual channel 1 disabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC1_CTRL) & (1<<0)) != 0) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! virtual channel 1 disabled timeout!!!\n");
#endif

	//disable virtual channel 0
	dwVal = RegRead32(DSI_BASE, DSI_VC0_CTRL);
	dwVal &= (~(1<<0));
	RegWrite32(DSI_BASE, DSI_VC0_CTRL, dwVal);

	//wait for virtual channel 1 disabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC0_CTRL) & (1<<0)) != 0) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! virtual channel 0 disabled timeout!!!\n");

	//set mode to video mode
	dwVal = RegRead32(DSI_BASE, DSI_VC0_CTRL);
	dwVal |= (1<<4);
	dwVal |= (1<<9);
	RegWrite32(DSI_BASE, DSI_VC0_CTRL, dwVal);


	/* Form the DSI video mode packet header */
	//Henry: Fix head code
	dwVal = 0x0009603e;
	RegWrite32(DSI_BASE, DSI_VC_LONG_PACKET_HEADER(0), dwVal);

	//Enable virtual channel 0
	dwVal = RegRead32(DSI_BASE, DSI_VC0_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_VC0_CTRL, dwVal);

	//wait for virtual channel 0 enabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC0_CTRL) & (1<<0)) == 0) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for virtual channel 0 enabled timeout!!!\n");

	//Enable DSI Interface
	//the data acquisition on the video port starts on the next VSYNC
	dwVal = RegRead32(DSI_BASE, DSI_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_CTRL, dwVal);

	//wait for interface enable
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_CTRL) & (1<<0)) == 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! DSI Interface enable timeout!!!\n");

	//unset Stop state (LP-11)
	dwVal = RegRead32(DSI_BASE, DSI_TIMING1);
	dwVal &= (~(1<<15));
	RegWrite32(DSI_BASE, DSI_TIMING1, dwVal);
}

//**************************************************************************************
// Function: dsi_videomode_enable
// Description: Enable DSI video mode
//**************************************************************************************
void dsi_video_preinit(void)
{
	ulong dwVal = 0;
	int timeout = MAX_DELAY_COUNTER;

	//set timming
	dwVal = RegRead32(DSI_BASE, DSI_CTRL);
	dwVal |= (1<<15);	/* DSI_CTRL_VP_VSYNC_START */
	dwVal |= (1<<17);	/* DSI_CTRL_VP_HSYNC_START */
	dwVal |= (1<<20);	/* DSI_CTRL_BLANKING_MODE */
	dwVal |= (1<<21);	/* DSI_CTRL_HFP_BLANKING_MODE */
	dwVal |= (1<<22);	/* DSI_CTRL_HBP_BLANKING_MODE */
	dwVal |= (1<<23);	/* DSI_CTRL_HSA_BLANKING_MODE */
	RegWrite32(DSI_BASE, DSI_CTRL, dwVal);

	//Set DSI_VM_TIMING1
	dwVal = (DSI_HSA << 24) | (DSI_HFP << 12) | DSI_HBP;
	RegWrite32(DSI_BASE, DSI_VM_TIMING1, dwVal);

	//Set DSI_VM_TIMING2
	//window_sync  = 4, 0x04000000
	dwVal = 0x04000000 | (DSI_VSA << 16) | (DSI_VFP << 8) |	DSI_VBP;
	RegWrite32(DSI_BASE, DSI_VM_TIMING2, dwVal);

	//Set DSI_VM_TIMING3 [31:16]=TL [15:0]=VACT
	dwVal = (DSI_TL << 16) | DSI_VACT;
	RegWrite32(DSI_BASE, DSI_VM_TIMING3, dwVal);

	//set DSI_VM_TIMING4
	dwVal = 0; /*0x00487296;*/
	RegWrite32(DSI_BASE, DSI_VM_TIMING4, dwVal);

	//set DSI_VM_TIMING5
	dwVal = 0x0082df3b;
	RegWrite32(DSI_BASE, DSI_VM_TIMING5, dwVal);

	//set DSI_VM_TIMING6
	dwVal = 0x00000005;
	RegWrite32(DSI_BASE, DSI_VM_TIMING6, dwVal);

	//set DSI_VM_TIMING7
	dwVal = 0x0017000E;
	//dwVal = DSI_VM_TIMING7_setting;

	RegWrite32(DSI_BASE, DSI_VM_TIMING7, dwVal);

	//set DSI_STOPCLK_TIMING
	dwVal = 0x00000007;
	RegWrite32(DSI_BASE, DSI_STOPCLK_TIMING, dwVal);

	//set TA_TO_COUNTER accordignly to kozio value
	dwVal = 0;
	dwVal |= 8191;		/* STOP_STATE_COUNTER_IO */
	dwVal |= (1<<13);	/* STOP_STATE_X4_IO */
	dwVal |= (1<<14);	/* STOP_STATE_X16_IO */
	dwVal &= ~(1<<15);	/* FORCE_TX_STOP_MODE_IO */
	dwVal |= (8191<<16);	/* TA_TO_COUNTER */
	dwVal |= (1<<29);	/* TA_TO_X8 */
	dwVal |= (1<<30);	/* TA_TO_X16 */
	/* dwVal |= (1<<31); */	/* TA_TO */

	RegWrite32(DSI_BASE, DSI_TIMING1, dwVal);

	//set TA_TO_COUNTER accordignly to kozio value
	dwVal = 0;
	dwVal |= 8191;		/* LP_RX_TO_COUNTER */
	dwVal |= (1<<13);	/* LP_RX_TO_X4 */
	dwVal |= (1<<14);	/* LP_RX_TO_X16 */
	dwVal &= ~(1<<15);	/* LP_RX_TO */
	dwVal |= (8191<<16);	/* HS_TX_TO_COUNTER */
	dwVal |= (1<<29);	/* HS_TX_TO_X16 */
	dwVal |= (1<<30);	/* HS_TX_TO_X64 */
	dwVal |= (1<<31);	/* HS_TX_TO */

	RegWrite32(DSI_BASE, DSI_TIMING2, dwVal);

	//Configure virtual channel 0
	dwVal = RegRead32(DSI_BASE, DSI_VC0_CTRL);
	dwVal &= (~0xFFEE3FDF);
	dwVal |= 0x20800580; // set mode speed to LP
	RegWrite32(DSI_BASE, DSI_VC0_CTRL, dwVal);

	//Configure virtual channel 1
#if 0
	dwVal = RegRead32(DSI_BASE, DSI_VC1_CTRL);
	dwVal &= (~0xFFEE3F4F);
	dwVal |= 0x20800D80;
	RegWrite32(DSI_BASE, DSI_VC1_CTRL, dwVal);
#endif

	//disable DSI Interface
	dwVal = RegRead32(DSI_BASE, DSI_CTRL);
	dwVal &= (~(1<<0));
	RegWrite32(DSI_BASE, DSI_CTRL, dwVal);

	//wait for interface disable
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_CTRL) & (1<<0)) != 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! DSI Interface disable timeout!!!\n");

	//Enable virtual channel 0
	dwVal = RegRead32(DSI_BASE, DSI_VC0_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_VC0_CTRL, dwVal);

	//wait for virtual channel 0 enabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC0_CTRL) & (1<<0)) == 0) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for virtual channel 0 enabled timeout!!!\n");

#if 0
	//Enable virtual channel 1
	dwVal = RegRead32(DSI_BASE, DSI_VC1_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_VC1_CTRL, dwVal);

	//wait for virtual channel 1 enabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC1_CTRL) & (1<<0)) == 0) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for virtual channel 1 enabled timeout!!!\n");
#endif

	//Enable DSI Interface
	//the data acquisition on the video port starts on the next VSYNC
	dwVal = RegRead32(DSI_BASE, DSI_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_CTRL, dwVal);

	//wait for interface enable
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_CTRL) & (1<<0)) == 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! DSI Interface enable timeout!!!\n");

	//Send null packet to start DDR clock
	dwVal = 0;
	RegWrite32(DSI_BASE, DSI_VC0_SHORT_PACKET_HEADER, dwVal);
}

//**************************************************************************************
// Function: dsi_reset
// Description: reset DSI
//**************************************************************************************
void dsi_reset(void)
{
	ulong dwVal = 0;
	int timeout = MAX_DELAY_COUNTER;

	//Software reset
	dwVal = RegRead32(DSI_BASE, DSI_SYSCONFIG);
	dwVal |= 0x1;
	RegWrite32(DSI_BASE, DSI_SYSCONFIG, dwVal);

	//Wait until RESET_DONE
	printf("DSI module reset in progress... ");
	do{
		dwVal = RegRead32(DSI_BASE, DSI_SYSSTATUS);
		timeout--;
	}while( ((dwVal & 0x1) == 0) && (timeout > 0));

	if(timeout <=0)
		printf("Error!!! time out\n");
	else
		printf("Done\n");

	//disable IRQ
	dwVal = 0;
	RegWrite32(DSI_BASE, DSI_IRQENABLE, dwVal);
	RegWrite32(DSI_BASE, DSI_VC0_IRQENABLE, dwVal);
	RegWrite32(DSI_BASE, DSI_VC1_IRQENABLE, dwVal);
	RegWrite32(DSI_BASE, DSI_VC2_IRQENABLE, dwVal);
	RegWrite32(DSI_BASE, DSI_VC3_IRQENABLE, dwVal);

	//Reset IRQ status
	dwVal = RegRead32(DSI_BASE, DSI_IRQSTATUS);
	RegWrite32(DSI_BASE, DSI_IRQSTATUS, dwVal);

	dwVal = RegRead32(DSI_BASE, DSI_VC0_IRQSTATUS);
	RegWrite32(DSI_BASE, DSI_VC0_IRQSTATUS, dwVal);

	dwVal = RegRead32(DSI_BASE, DSI_VC1_IRQSTATUS);
	RegWrite32(DSI_BASE, DSI_VC1_IRQSTATUS, dwVal);

	dwVal = RegRead32(DSI_BASE, DSI_VC2_IRQSTATUS);
	RegWrite32(DSI_BASE, DSI_VC2_IRQSTATUS, dwVal);

	dwVal = RegRead32(DSI_BASE, DSI_VC3_IRQSTATUS);
	RegWrite32(DSI_BASE, DSI_VC3_IRQSTATUS, dwVal);

	dwVal = RegRead32(DSI_BASE, DSI_COMPLEXIO_IRQ_STATUS);
	RegWrite32(DSI_BASE, DSI_COMPLEXIO_IRQ_STATUS, dwVal);

	//DSI power management: Autoidle
	dwVal = RegRead32(DSI_BASE, DSI_SYSCONFIG);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_SYSCONFIG, dwVal);

	//DSI power management: ENWAKEUP
	dwVal = RegRead32(DSI_BASE, DSI_SYSCONFIG);
	dwVal |= (1<<2);
	RegWrite32(DSI_BASE, DSI_SYSCONFIG, dwVal);

	//DSI power management: SIDLEMODE smart-idle
	dwVal = RegRead32(DSI_BASE, DSI_SYSCONFIG);
	dwVal &= (~(3<<3));
	dwVal |= (2 << 3);
	RegWrite32(DSI_BASE, DSI_SYSCONFIG, dwVal);
}

//**************************************************************************************
// Function: dsi_set_dpll
// Description: Set DPLL for DSI
//**************************************************************************************
void dsi_set_dpll(void)
{
	ulong dwVal = 0;
	int timeout = MAX_DELAY_COUNTER;

	//CIO_CLK_ICG, Enable SCPClk,
	//SCPClk clock provided to DSI_PHY and PLL-CTRL module
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal |= (1<<14);
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

	//wait for DSI1_PLLCTRL_RESET_DONE
	timeout = MAX_DELAY_COUNTER;
	while((((RegRead32(DSI_BASE, DSI_PLL_STATUS) & (1<<0)) == 0 )) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! DSI PLL reset fail!!!\n");

	//power on both PLL and HSDIVISER
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal |= (2<<30);
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

	//Wait until PLL_PWR_STATUS = 0x2
	timeout = MAX_DELAY_COUNTER;
	while(((((RegRead32(DSI_BASE, DSI_CLK_CTRL) & 0x30000000) >> 28) != 0x2)) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! turn on PLL power fail!!!\n");

	// disable PLL_AUTOMODE
	dwVal = RegRead32(DSI_BASE, DSI_PLL_CONTROL);
	dwVal &= ~(1<<0);
	RegWrite32(DSI_BASE, DSI_PLL_CONTROL, dwVal);

	//Set PLL Divider by DSI_PLL_CONFIGURATION1
	dwVal = ((REGM_DSI - 1) << 26) | ((REGM_DISPC - 1) << 21) |
		(REGM << 9) | ((REGN - 1) << 1) | 1;
	RegWrite32(DSI_BASE, DSI_PLL_CONFIGURATION1, dwVal);

	//Set DSI_PLL_CONFIGURATION2
	dwVal = RegRead32(DSI_BASE, DSI_PLL_CONFIGURATION2);
	dwVal &= (~(1<<11));
	dwVal &= (~(1<<12));
	dwVal |= (1<<13);
	dwVal &= (~(1<<14));
	dwVal |= (1<<20);
	dwVal |= (3<<21);
	RegWrite32(DSI_BASE, DSI_PLL_CONFIGURATION2, dwVal);

	//Request locking
	dwVal = RegRead32(DSI_BASE, DSI_PLL_GO);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_PLL_GO, dwVal);

	//Wait for lock
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_PLL_GO) & 0x1) != 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! Wait for lock time out\n");

	// Waiting for PLL lock
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_PLL_STATUS) & 0x2 ) == 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! Waiting for PLL lock time out\n");

	//Locked, change the configuration
	dwVal = 0x656008;
	RegWrite32(DSI_BASE, DSI_PLL_CONFIGURATION2, dwVal);

	//wait for M4 clock active
	timeout = MAX_DELAY_COUNTER;
	while((  (RegRead32(DSI_BASE, DSI_PLL_STATUS) & (1<<7) ) == 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for M4 clock active time out\n");

	// wait for M5 Protocol Engine clock active
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_PLL_STATUS) & (1<<8)) == 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for M5 Protocol Engine clock active time out\n");

	//Select clock source
	//FCK_CLK_SWITCH = PLL1_CLK1 selected (from DSI1 PLL)
	//DSS_CTRL(9:8) = 01
	RegSet32(DSS_BASE, DSS_CTRL, 1 << 8);
	RegClear32(DSS_BASE, DSS_CTRL, 1 << 9);

	//Select clock source
	//DSI1_CLK_SWITCH = 1
	//DSS_CTRL(1) = 1
	RegSet32(DSS_BASE, DSS_CTRL, 1 << 1);

	//Select clock source
	//LCD1_CLK_SWITCH = PLL1_CLK1 selected (from DSI1 PLL)
	//DSS_CTRL(0) = 1
	RegSet32(DSS_BASE, DSS_CTRL, 1 << 0);
}


//**************************************************************************************
// Function: dsi_set_phy
// Description: init DSI phy
//**************************************************************************************
void dsi_set_phy(void)
{
	ulong dwVal = 0;
	int timeout = MAX_DELAY_COUNTER;

	//CIO_CLK_ICG, Enable SCPClk,
	//SCPClk clock provided to DSI_PHY and PLL-CTRL module
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal |= (1<<14);
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

	//HS_AUTO_STOP_ENABLE
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal |= (1<<18);
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

	/* A dummy read using the SCP interface to any DSIPHY register is
	 * required after DSIPHY reset to complete the reset of the DSI complex
	 * I/O. */
	dwVal = RegRead32(DSI_BASE, DSI_DSIPHY_CFG5);

	//wait for SCP clock domain of DSI phy Reset done
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_DSIPHY_CFG5) & (1<<30)) == 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! DSI phy Reset time out\n");

	//Set dsi complexio_config
	dwVal = 0x54321;
	RegWrite32(DSI_BASE, DSI_COMPLEXIO_CFG1, dwVal);

	//complex I/O power on
	dwVal = RegRead32(DSI_BASE, DSI_COMPLEXIO_CFG1);
	dwVal |= (1 << 27);
	RegWrite32(DSI_BASE, DSI_COMPLEXIO_CFG1, dwVal);

	//Set the GOBIT bit
	dwVal = RegRead32(DSI_BASE, DSI_COMPLEXIO_CFG1);
	dwVal |= (1<<30);
	RegWrite32(DSI_BASE, DSI_COMPLEXIO_CFG1, dwVal);

	//wait for PWR_STATUS = on
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_COMPLEXIO_CFG1) & (0x3<<25)) != (1 << 25) ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for PWR_STATUS timeout when set dsi_complexio!!!\n");

	//Wait for RESET_DONE
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_COMPLEXIO_CFG1) & (1<<29)) == 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! Wait for RESET_DONE timeout!!!\n");

	//set complexio timings
	dwVal = (THS_PREPARE << 24) | (THS_PREPARE_THE_ZERO << 16) |
		(THS_TRAIL << 8) | THS_EXIT;
	RegWrite32(DSI_BASE, DSI_DSIPHY_CFG0, dwVal);

	dwVal = RegRead32(DSI_BASE, DSI_DSIPHY_CFG1);
	dwVal &= (~0xFFFFFF);

	dwVal |= (TTAGET << 24) | (TLPX_HALF << 16) | (TCLK_TRAIL << 8) | TCLK_ZERO;
	RegWrite32(DSI_BASE, DSI_DSIPHY_CFG1, dwVal);

	dwVal = RegRead32(DSI_BASE, DSI_DSIPHY_CFG2);
	dwVal &= (~0xFF);

	dwVal |= TCLK_PREPARE;

	RegWrite32(DSI_BASE, DSI_DSIPHY_CFG2, dwVal);

}

//**************************************************************************************
// Function: dsi_set_protocol_engine
// Description: init DSI protocol engine
//**************************************************************************************
void dsi_set_protocol_engine(void)
{
	ulong dwVal = 0;
	int timeout = MAX_DELAY_COUNTER;

	//Enable LP_CLK
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal |= (1<<20);
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

	//set DDR clock
	dwVal = (DDR_CLK_PRE << 8) | DDR_CLK_POST;

	RegWrite32(DSI_BASE, DSI_CLK_TIMING, dwVal);

	//set the number of TXBYTECLKHS clock cycles
	dwVal = 0xe000f;
	RegWrite32(DSI_BASE, DSI_VM_TIMING7, dwVal);

	//set lp_clk_div
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal &= (~0x1FFF);
	dwVal |= LP_CLK_DIV;
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

	//RX SYNCHRO ENABLE
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal |= (1<<21);
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

	//set TX FIFO size
	dwVal = 0x13121110;
	RegWrite32(DSI_BASE, DSI_TX_FIFO_VC_SIZE, dwVal);

	//set RX FIFO size
	dwVal = 0x13121110;
	RegWrite32(DSI_BASE, DSI_RX_FIFO_VC_SIZE, dwVal);

	//set stop_state_counter
	dwVal = RegRead32(DSI_BASE, DSI_TIMING1);
	dwVal &= (~0x0000FFFF);
	dwVal |= 0x1000;
	RegWrite32(DSI_BASE, DSI_TIMING1, dwVal);

	//set_ta_timeout
	dwVal = RegRead32(DSI_BASE, DSI_TIMING1);
	dwVal &= (~0xFFFF0000);
	dwVal |= 0x7fff0000;
	RegWrite32(DSI_BASE, DSI_TIMING1, dwVal);

	//set_lp_rx_timeout
	dwVal = RegRead32(DSI_BASE, DSI_TIMING2);
	dwVal &= (~0x0000FFFF);
	dwVal |= 0x7fff;
	RegWrite32(DSI_BASE, DSI_TIMING2, dwVal);

	//set_hs_tx_timeout
	dwVal = RegRead32(DSI_BASE, DSI_TIMING2);
	dwVal &= (~0xFFFF0000);
	dwVal |= 0x7fff0000;
	RegWrite32(DSI_BASE, DSI_TIMING2, dwVal);

	//set DSI
	dwVal = RegRead32(DSI_BASE, DSI_CTRL);
	dwVal &= (~(1<<1));	/* CS_RX_EN */
	dwVal &= (~(1<<2));	/* ECC_RX_EN */
	dwVal |= (1<<3);	/* TX_FIFO_ARBITRATION */
	dwVal |= (1<<4);	/* VP_CLK_RATIO, always 1, see errata */
	dwVal &= (~(3<<6));	/* VP_DATA_BUS_WIDTH */
	dwVal |= (2<<6);	/* VP_DATA_BUS_WIDTH, WIDTH_24*/
	dwVal &= (~(1<<8));	/* VP_CLK_POL */
	dwVal |= (1<<9);	/* VP_DE_POL */
	dwVal |= (1<<11);	/* VP_VSYNC_POL */
	dwVal &= (~(3<<12));	/* LINE_BUFFER, 2 lines */
	dwVal |= (2<<12);	/* LINE_BUFFER, 2 lines */
	dwVal |= (1<<14);	/* TRIGGER_RESET_MODE */
	dwVal |= (1<<19);	/* EOT_ENABLE */
	RegWrite32(DSI_BASE, DSI_CTRL, dwVal);

	//Initial VC0
	dwVal = 0x20061d80;
	RegWrite32(DSI_BASE, DSI_VC0_CTRL, dwVal);


	//Initial VC1
	dwVal = 0x20808f80;
	RegWrite32(DSI_BASE, DSI_VC1_CTRL, dwVal);
#if 0
	//Initial VC2
	dwVal = 0x20061d80;
	RegWrite32(DSI_BASE, DSI_VC2_CTRL, dwVal);

	//Initial VC3
	dwVal = 0x20061d80;
	RegWrite32(DSI_BASE, DSI_VC3_CTRL, dwVal);
#endif

	//Disable virtual channel 0
	dwVal = RegRead32(DSI_BASE, DSI_VC0_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_VC0_CTRL, dwVal);

	//wait for virtual channel 0 disabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC0_CTRL) & (1<<0)) == 0) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for virtual channel 0 enabled timeout!!!\n");


	//Enable virtual channel 1
	dwVal = RegRead32(DSI_BASE, DSI_VC1_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_VC1_CTRL, dwVal);

	//wait for virtual channel 1 enabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC1_CTRL) & (1<<0)) == 0) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for virtual channel 1 enabled timeout!!!\n");
#if 0
	//Enable virtual channel 2
	dwVal = RegRead32(DSI_BASE, DSI_VC2_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_VC2_CTRL, dwVal);

	//wait for virtual channel 2 enabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC2_CTRL) & (1<<0)) == 0) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for virtual channel 2 enabled timeout!!!\n");

	//Enable virtual channel 3
	dwVal = RegRead32(DSI_BASE, DSI_VC3_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_VC3_CTRL, dwVal);

	//wait for virtual channel 3 enabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC3_CTRL) & (1<<0)) == 0) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! wait for virtual channel 1 enabled timeout!!!\n");
#endif

	//Enable DSI Interface
	//the data acquisition on the video port starts on the next VSYNC
	dwVal = RegRead32(DSI_BASE, DSI_CTRL);
	dwVal |= (1<<0);
	RegWrite32(DSI_BASE, DSI_CTRL, dwVal);

	//wait for interface enable
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_CTRL) & (1<<0)) == 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! DSI Interface enable timeout!!!\n");

	//Set DSI ForceTxStopMode
	dwVal = RegRead32(DSI_BASE, DSI_TIMING1);
	dwVal |= (1<<15);
	RegWrite32(DSI_BASE, DSI_TIMING1, dwVal);

	//wait for ForceTxStopMode
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_TIMING1) & (1<<15)) != 0 ) && (timeout > 0)) {
		timeout--;
	}
	if(timeout <=0) printf("Error!!! DSI ForceTxStopMode timeout!!!\n");

}

//**************************************************************************************
// Function: set_DISPC
// Description: Set DISPC registers
//**************************************************************************************
void set_DISPC(ulong dwFramebufferAddr)
{
	ulong dwVal = 0;

	//set STNTFT, LCD type of the primary LCD is TFT
	//DISPC_CONTROL1(3) = 1
	RegSet32(DISPC_BASE, DISPC_CONTROL1, 1 << 3);

	//DISPC_CONTROL1(11) = 0  , STALL Mode for the primary LCD output
	//DISPC_CONTROL1(15) = 1  , General Purpose Output Signal set
	//DISPC_CONTROL1(16) = 1  , General Purpose Output Signal set
	RegClear32(DISPC_BASE, DISPC_CONTROL1, 1 << 11);
	RegSet32(DISPC_BASE, DISPC_CONTROL1, 1 << 15);
	RegSet32(DISPC_BASE, DISPC_CONTROL1, 1 << 16);

	//DISPC_CONFIG1(16) = 0,
	//config BUFFERHANDCHECK
	RegClear32(DISPC_BASE, DISPC_CONFIG1, 1 << 16);

	//DISPC_CONTROL1(9, 8) = 11  , 24-bit output aligned on the LSB of the pixel data interface
	RegSet32(DISPC_BASE, DISPC_CONTROL1, 1 << 8);
	RegSet32(DISPC_BASE, DISPC_CONTROL1, 1 << 9);

	//set lcd timings, channel = 0
	dwVal = ((LCD_HBP - 1) << 20) | ((LCD_HFP - 1) << 8) | (LCD_HSW - 1);
	RegWrite32(DISPC_BASE, DISPC_TIMING_H, dwVal);

	//set lcd timings
	dwVal = (LCD_VBP << 20) | (LCD_VFP << 8) | (LCD_VSW - 1);
	RegWrite32(DISPC_BASE, DISPC_TIMING_V, dwVal);

	//configures the panel size (horizontal and vertical)
	//[26:16]=hight [10:0]=width
	dwVal = ((LCD_HIGHT - 1) << 16) | (LCD_WIDTH - 1);
	RegWrite32(DISPC_BASE, DISPC_SIZE_LCD, dwVal);

	//config clock div
	dwVal = (LCK_DIV << 16) | PCK_DIV;
	RegWrite32(DISPC_BASE, DISPC_DIVISOR, dwVal);

	//Set power management: MIDLEMODE=smart standby
	dwVal = RegRead32(DISPC_BASE, DISPC_SYSCONFIG);
	dwVal &= (~(0x3<<12));
	dwVal |= (2 << 12);

	//SIDLEMODE=smart idle
	dwVal &= (~(0x3<<3));
	dwVal |= (2 << 3);

	//ENWAKEUP
	dwVal |= (1<<2);

	//AUTOIDLE */
	dwVal |= (1<<0);

	RegWrite32(DISPC_BASE, DISPC_SYSCONFIG, dwVal);

	//set Loading Mode for the Palette/Gamma Table
	dwVal = RegRead32(DISPC_BASE, DISPC_CONFIG1);
	dwVal &= (~(0x3<<1));
	dwVal |= (2 << 1);
	RegWrite32(DISPC_BASE, DISPC_CONFIG1, dwVal);

	// Enabling the DISPC_DIVISOR
	dwVal = RegRead32(DISPC_BASE, DISPC_DIVISOR);
	dwVal &= (~0xFF0000);
	dwVal |= 0x10000;
	RegWrite32(DISPC_BASE, DISPC_DIVISOR, dwVal);

	// Disable INTR
	dwVal = 0;
	RegWrite32(DISPC_BASE, DISPC_IRQENABLE, dwVal);

	// Clear INTR
	dwVal = 0x3FFFFFFF;
	RegWrite32(DISPC_BASE, DISPC_IRQSTATUS, dwVal);

	//Graphics Channel Out configuration:TV output selected
	dwVal = RegRead32(DISPC_BASE, DISPC_GFX_ATTRIBUTES);
	dwVal |= (1<<8);
	RegWrite32(DISPC_BASE, DISPC_GFX_ATTRIBUTES, dwVal);

	//Graphics Channel Out configuration:LCD output
	//bit fields 31 and 30 defines the output associated (primary,
	//secondary or write-back).
	dwVal = RegRead32(DISPC_BASE, DISPC_GFX_ATTRIBUTES);
	dwVal &= (~(1<<8));
	RegWrite32(DISPC_BASE, DISPC_GFX_ATTRIBUTES, dwVal);

#if 0
	//VID2, Video Channel Out configuration:LCD output
	dwVal = RegRead32(DISPC_BASE, DISPC_VID2_ATTRIBUTES);
	dwVal &= (~(1<<16));
	RegWrite32(DISPC_BASE, DISPC_VID2_ATTRIBUTES, dwVal);

	//VID3, Video Channel Out configuration:LCD output
	dwVal = RegRead32(DISPC_BASE, DISPC_VID3_ATTRIBUTES);
	dwVal &= (~(1<<16));
	RegWrite32(DISPC_BASE, DISPC_VID3_ATTRIBUTES, dwVal);

	//default solid background color for the primary LCD
	dwVal = 0;
	RegWrite32(DISPC_BASE, DISPC_DEFAULT_COLOR0, dwVal);
#endif

	//set_channel_out(channel=0)
	dwVal = RegRead32(DISPC_BASE, DISPC_GFX_ATTRIBUTES);
	dwVal &= (~(1<<8));
	dwVal &= (~(3<<30));
	RegWrite32(DISPC_BASE, DISPC_GFX_ATTRIBUTES, dwVal);

	//set_color_mode [4:1] = 0x6: RGB16-565
	dwVal = RegRead32(DISPC_BASE, DISPC_GFX_ATTRIBUTES);
	dwVal &= (~(0xF<<1));
	dwVal |= (0x6<<1);
	RegWrite32(DISPC_BASE, DISPC_GFX_ATTRIBUTES, dwVal);

	//Set BA0
	dwVal = dwFramebufferAddr;
	RegWrite32(DISPC_BASE, DISPC_GFX_BA0, dwVal);

	//SetBA1
	dwVal = dwFramebufferAddr;
	RegWrite32(DISPC_BASE, DISPC_GFX_BA1, dwVal);

	//DISPC_GFX_POSITION
	dwVal = 0x0;
	RegWrite32(DISPC_BASE, DISPC_GFX_POSITION, dwVal);

	//DISPC_GFX_SIZE
	dwVal = ((LCD_HIGHT - 1) << 16) | (LCD_WIDTH - 1);
	RegWrite32(DISPC_BASE, DISPC_GFX_SIZE, dwVal);

	// dispc_enable_plane( enable=1 )
	dwVal = RegRead32(DISPC_BASE, DISPC_GFX_ATTRIBUTES);
	dwVal |= (1<<0);
	RegWrite32(DISPC_BASE, DISPC_GFX_ATTRIBUTES, dwVal);

}

//**************************************************************************************
// Function: init_DSS
// Description: Init DSS and enable all clocks
//**************************************************************************************
void init_DSS(void)
{
	ulong dwVal = 0;

	//Enable clocks and force enable by MODULEMODE_ENABLE
	dwVal = RegRead32(CM2_DSS, CM_DSS_DSS_CLKCTRL);
	dwVal |= CM_DSS_DSS_CLKCTRL_OPTFCLKEN_SYS_CLK;
	dwVal |= CM_DSS_DSS_CLKCTRL_OPTFCLKEN_DSSCLK;
	dwVal |= CM_DSS_DSS_CLKCTRL_OPTFCLKEN_TV_FCLK;
	dwVal |= CM_DSS_DSS_CLKCTRL_OPTFCLKEN_48MHZ_CLK;
	dwVal &= (~CM_DSS_DSS_CLKCTRL_MODULEMODE_MASK);
	dwVal |= CM_DSS_DSS_CLKCTRL_MODULEMODE_ENABLE;
	RegWrite32(CM2_DSS, CM_DSS_DSS_CLKCTRL, dwVal);

	//Wait until reset completes
	//while((RegRead32(DSS_BASE, DSS_SYSSTATUS) & 0x1) == 0) {
	//   printf("DSS_SYSSTATUS = %x\n", RegRead32(DSS_BASE, DSS_SYSSTATUS));
	//}

	udelay(50000);

	//autoidle
	dwVal = RegRead32(DSS_BASE, DSS_SYSCONFIG);
	dwVal |= 0x1;
	RegWrite32(DSS_BASE, DSS_SYSCONFIG, dwVal);

	//Select DPLL
	dwVal = RegRead32(DSS_BASE, DSS_CTRL);
	dwVal &= (~0x1);
	RegWrite32(DSS_BASE, DSS_CTRL, dwVal);
}


//**************************************************************************************
// Function: init_pinMux
// Description: Init the pinmux if needed
//**************************************************************************************
void init_pinMux(void)
{
	ulong dwVal = 0;

	//Enable DSI PHY
	dwVal = 0xFFFFC000;
	RegWrite32(SYSCTRL_PADCONF_CORE, CONTROL_DSIPHY, dwVal);
}

void set_bl_for_recovery()
{
	unsigned char addr = 0;
	unsigned char data = 0;

	if(is_recovery_mode)
	{
		printf("Changing bl mode in set_bl_for_recovery \n");
		/* Set I2C bus 1 @ 100KHz speed */
		if(select_bus(1, 0x64))
			puts("Error in setting I2C bus 1\n");

		/* Set BL_CTL and BRT_MODE[1:0] in LP8552 */
		addr = LP8552_DEV_CTRL_REG;
		data = 0;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		/* Set BRT[7:0] in LP8552 */
		addr = LP8552_BRT_CTRL_REG;
		data = 0;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		/* Switch back to the original I2C bus */
		if(select_bus(0, 0x64))
			puts("Error in setting I2C bus 0\n");

		is_recovery_mode = 0;
	}
	else
		printf("Doing nothing in set_bl_for_recovery \n");
}

//**************************************************************************************
// Function: Power_on_LCD
// Description: Power_on_LCD
// Depend: GPIO driver
//**************************************************************************************
void Power_on_LCD(void)
{
	//LCD Panel Enable
	//printf("LCD Panel Enable. \n");
	configure_pad_mode(GPIO_LCD_ENABLE);
	//Enable pullup to keep display powered during kernel gpio2 module reset.
	RegSet32(OMAP44XX_CTRL_BASE, CONTROL_PADCONF_GPMC_AD10, PTU << 16);
	configure_gpio_output(GPIO_LCD_ENABLE);
	set_gpio_output(GPIO_LCD_ENABLE, 1);

	//After EVT1.0a, Brightness is controled by CABC enable CABC
	configure_pad_mode(GPIO_BACKLIGHT_CABC_EN);
	configure_gpio_output(GPIO_BACKLIGHT_CABC_EN);
	set_gpio_output(GPIO_BACKLIGHT_CABC_EN, 1);
}

//**************************************************************************************
// Function: Power_off_LCD
// Description: Power_off_LCD
// Depend: GPIO driver
//**************************************************************************************
void Power_off_LCD(void)
{
	return 0;
	/* FIXME: This just turns off the backlight, not the LCD panel */
	configure_pad_mode(GPIO_BACKLIGHT_CABC_EN);
	configure_gpio_output(GPIO_BACKLIGHT_CABC_EN);
	set_gpio_output(GPIO_BACKLIGHT_CABC_EN, 0);

	//LCD Panel Disable
	//printf("LCD Panel Disable. \n");
	configure_pad_mode(GPIO_LCD_ENABLE);
	configure_gpio_output(GPIO_LCD_ENABLE);
	set_gpio_output(GPIO_LCD_ENABLE, 0);
} 

//**************************************************************************************
// Function: Set_Brightness
// Description: Set_Brightness
// Depend: GPIO driver
//**************************************************************************************

void Set_Brightness(int is_Max)
{
	unsigned char addr = 0;
	unsigned char data = 0;
	if(is_Max == 1) {
		/* FIXME: GPIO is not used to enable backlight driver in Tate EVT 1.x */
		//LCD Backlight Enable
		//printf("LCD Backlight Enable. \n");
		configure_pad_mode(GPIO_BACKLIGHT_EN_O2M);
		configure_gpio_output(GPIO_BACKLIGHT_EN_O2M);
		set_gpio_output(GPIO_BACKLIGHT_EN_O2M, 1);

		//printf("Set PWM to Max. \n");
		configure_pad_mode(GPIO_BACKLIGHT_PWM);
		configure_gpio_output(GPIO_BACKLIGHT_PWM);
		set_gpio_output(GPIO_BACKLIGHT_PWM, 1);

		/* Set I2C bus 1 @ 100KHz speed */
		if(select_bus(1, 0x64))
			puts("Error in setting I2C bus 1\n");

		/* Set BL_CTL and BRT_MODE[1:0] in LP8552 */
		addr = LP8552_DEV_CTRL_REG;
		/* Set default mode */
		data = 0;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		/* Set BRT[7:0] in LP8552 */
		addr = LP8552_BRT_CTRL_REG;
		/* Set default mode */
		data = 0;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		/* Set EEPROM ADDR0 for CURRENT[7:0] */
		addr = LP8552_EEPROM_ADDR0;
		data = LP8552_BRT_LEVEL;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		addr = LP8552_EEPROM_ADDR1;
		data = LP8552_NO_SLOPE_TIME;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		/* Switch back to the original I2C bus */
		if(select_bus(0, 0x64))
			puts("Error in setting I2C bus 0\n");
	}
	else if(is_Max == 2) {
		// just turn on gpios for showing up backlight during charging
		configure_pad_mode(GPIO_BACKLIGHT_EN_O2M);
		configure_gpio_output(GPIO_BACKLIGHT_EN_O2M);
		set_gpio_output(GPIO_BACKLIGHT_EN_O2M, 1);

		configure_pad_mode(GPIO_BACKLIGHT_PWM);
		configure_gpio_output(GPIO_BACKLIGHT_PWM);
		set_gpio_output(GPIO_BACKLIGHT_PWM, 1);
	}
	else if (is_Max == 0){
		//LCD Backlight disable
		//printf("LCD Backlight Enable. \n");
		configure_pad_mode(GPIO_BACKLIGHT_EN_O2M);
		configure_gpio_output(GPIO_BACKLIGHT_EN_O2M);
		set_gpio_output(GPIO_BACKLIGHT_EN_O2M, 0);

		//printf("Set PWM to L. \n");
		configure_pad_mode(GPIO_BACKLIGHT_PWM);
		configure_gpio_output(GPIO_BACKLIGHT_PWM);
		set_gpio_output(GPIO_BACKLIGHT_PWM, 0);
	}
	else {
		configure_pad_mode(GPIO_BACKLIGHT_EN_O2M);
		configure_gpio_output(GPIO_BACKLIGHT_EN_O2M);
		set_gpio_output(GPIO_BACKLIGHT_EN_O2M, 1);

		//printf("Set PWM to Max. \n");
		configure_pad_mode(GPIO_BACKLIGHT_PWM);
		configure_gpio_output(GPIO_BACKLIGHT_PWM);
		set_gpio_output(GPIO_BACKLIGHT_PWM, 1);

		/* Set I2C bus 1 @ 100KHz speed */
		if(select_bus(1, 0x64))
			puts("Error in setting I2C bus 1\n");

		/* Set BL_CTL and BRT_MODE[1:0] in LP8552 */
		addr = LP8552_DEV_CTRL_REG;
		/* Set default mode */
		data = 0;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		/* Set BRT[7:0] in LP8552 */
		addr = LP8552_BRT_CTRL_REG;
		/* Set default mode */
		data = 0;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		/* Set EEPROM ADDR0 for CURRENT[7:0] */
		addr = LP8552_EEPROM_ADDR0;
		data = is_Max;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		addr = LP8552_EEPROM_ADDR1;
		data = LP8552_NO_SLOPE_TIME;
		if(i2c_write(LP8552_I2C_ADDR, addr, 1, &data, 1))
			printf("Error writing to lp8552 register %x\n",addr);

		/* Switch back to the original I2C bus */
		if(select_bus(0, 0x64))
			puts("Error in setting I2C bus 0\n");

	}
}

//**************************************************************************************
// Function:  Enable_LCD
// Description: Enable_LCD
//**************************************************************************************
void Enable_LCD(void)
{
	ulong dwVal = 0;

	//goLCD
	dwVal = RegRead32(DISPC_BASE, DISPC_CONTROL1);
	dwVal |= 1<<5;
	RegWrite32(DISPC_BASE, DISPC_CONTROL1, dwVal);

	//Enable LCD
	dwVal = RegRead32(DISPC_BASE, DISPC_CONTROL1);
	dwVal |= 1<<0;
	RegWrite32(DISPC_BASE, DISPC_CONTROL1, dwVal);
}

//**************************************************************************************
// Function: load_rle
// Description: load rle data onto RAM
//**************************************************************************************
void load_rle(u_int16_t *fb, u_int16_t *logo_start, u_int16_t *logo_end)
{
	u_int16_t *target_addr = fb;
	u_int16_t *start = logo_start;
	u_int16_t *end = logo_end;
	u_int16_t count = start[0];

	/* Convert the RLE data into RGB565 */
	for (; start != end; start += 2) {
		count = start[0];

		while (count--)
			*target_addr++ = start[1];
	}
}

/* initlogo.rle data */
extern char const _binary_initlogo_rle_start[];
extern char const _binary_initlogo_rle_end[];
/* lowbattery.gz data */
extern char const _binary_lowbattery_gz_start[];
extern char const _binary_lowbattery_gz_end[];
/* charging.gz data */
extern char const _binary_charging_gz_start[];
extern char const _binary_charging_gz_end[];
/* devicetoohot.gz data */
extern char const _binary_devicetoohot_rle_start[];
extern char const _binary_devicetoohot_rle_end[];
/* fastboot.gz data */
extern char const _binary_fastboot_gz_start[];
extern char const _binary_fastboot_gz_end[];
/* provfailed.rle data */
extern char const _binary_provfailed_rle_start[];
extern char const _binary_provfailed_rle_end[];

//**************************************************************************************
// Function: Show_Logo
// Description: show init logo to LCD
//**************************************************************************************
void Show_Logo(void)
{
	load_rle((u_int16_t *)FRAME_BUFFER_ADDR,
			(u_int16_t *)_binary_initlogo_rle_start,
			(u_int16_t *)_binary_initlogo_rle_end);
}

void show_provfailed(void)
{
	load_rle((u_int16_t *)FRAME_BUFFER_ADDR,
			(u_int16_t *)_binary_provfailed_rle_start,
			(u_int16_t *)_binary_provfailed_rle_end);
}

//**************************************************************************************
// Function: show_lowbattery
// Description: show lowbattery logo (compressed as lowbattery.gz) to LCD
//**************************************************************************************
void show_lowbattery(void)
{
	unsigned char *dst_addr = (unsigned char *)GUNZIP_BUFFER_ADDR;
	unsigned long dst_len = ~0UL, src_len = LCD_PIXELS;

	if (gunzip(dst_addr, src_len,
			(void *)_binary_lowbattery_gz_start, &dst_len) != 0) {
		printf("Can not uncompress *.gz file!\n");
		return 1;
	}

	load_rle((u_int16_t *)FRAME_BUFFER_ADDR,
			(u_int16_t *)(dst_addr),
			(u_int16_t *)(dst_addr + dst_len));
}

//**************************************************************************************
// Function: show_charging
// Description: show charging logo (compressed as charging.gz) to LCD
//**************************************************************************************
void show_charging(void)
{
	unsigned char *dst_addr = (unsigned char *)GUNZIP_BUFFER_ADDR;
	unsigned long dst_len = ~0UL, src_len = LCD_PIXELS;

	if (gunzip(dst_addr, src_len,
			(void *)_binary_charging_gz_start, &dst_len) != 0) {
		printf("Can not uncompress *.gz file!\n");
		return 1;
	}

	load_rle((u_int16_t *)FRAME_BUFFER_ADDR,
			(u_int16_t *)(dst_addr),
			(u_int16_t *)(dst_addr + dst_len));
}

//**************************************************************************************
// Function: show_devicetoohot
// Description: show device too hot logo (compressed as devicetoohot.gz) to LCD
//**************************************************************************************
void show_devicetoohot(void)
{
	load_rle((u_int16_t *)FRAME_BUFFER_ADDR,
			(u_int16_t *)_binary_devicetoohot_rle_start,
			(u_int16_t *)_binary_devicetoohot_rle_end);

}


//**************************************************************************************
// Function: show_fastboot
// Description: show fastboot logo (compressed as fastboot.gz) to LCD
//**************************************************************************************
void show_fastboot(void)
{
	unsigned char *dst_addr = (unsigned char *)GUNZIP_BUFFER_ADDR;
	unsigned long dst_len = ~0UL, src_len = LCD_PIXELS;

	if (gunzip(dst_addr, src_len,
			(void *)_binary_fastboot_gz_start, &dst_len) != 0) {
		printf("Can not uncompress *.gz file!\n");
		return 1;
	}

	load_rle((u_int16_t *)FRAME_BUFFER_ADDR,
			(u_int16_t *)(dst_addr),
			(u_int16_t *)(dst_addr + dst_len));
}

//**************************************************************************************
// Function:  Init_LCD
// Description: configures DSS, DSI and LCD, caller is misc_init_r of omap4.c
//**************************************************************************************
void Init_LCD(void)
{
	static int lcd_init = 0;

	/* Avoid double init */
	if (lcd_init == 1)
		return;

	//power on LCD
	Power_on_LCD();

	mdelay(100); // need this delay before we send any DSI command

	//turn off backlight
	Set_Brightness(0);

	//init pinMux
	init_pinMux();

	//init DSS
	init_DSS();

	//Init DISPC
	set_DISPC(FRAME_BUFFER_ADDR);

	//rset DSI
	dsi_reset();

	//Set DPLL for DSI
	dsi_set_dpll();

#if 0
// These two are disabled for the second boot as otherwise display
// does not come alive.
	//set DSI Phy
	dsi_set_phy();

	//set DSI protocol engine
	dsi_set_protocol_engine();
#endif

	//set to Videomode
	dsi_video_preinit();

	//enter Bist Mode
	//LCD_Bist();

	//Normal display
	LCD_Normal();

	//Post init for Video Mode
	dsi_videomode_PostInit();

	//Enable LCD1 output
	Enable_LCD();

	//Clear LCD, show black
	show_color((u_int16_t)(0x0000));

	lcd_init = 1;
}

//**************************************************************************************
// Function: show_color
// Description: show color on LCD
//**************************************************************************************
void show_color(u_int16_t color)
{
	u_int16_t *target_addr = (u_int16_t *)FRAME_BUFFER_ADDR;
	unsigned int pixels = LCD_PIXELS / LCD_BPP;

	do {
		*target_addr++ = color;
	} while (--pixels);
}

//**************************************************************************************
// Function: show_main
// Description: main function of show color on LCD
//**************************************************************************************
int show_main(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc != 2) {
		goto show_cmd_usage;
	} else {
		if (strncmp(argv[1], "init", 4) ==0) { /* initialize DSI and LCD */
			Init_LCD();
		} else if (strncmp(argv[1], "blue", 4) == 0) {
			show_color((u_int16_t)(0x001F));
		} else if (strncmp(argv[1], "green", 5) == 0) {
			show_color((u_int16_t)(0x07E0));
		} else if (strncmp(argv[1], "red", 3) == 0) {
			show_color((u_int16_t)(0xF800));
		} else if (strncmp(argv[1], "white", 5) == 0) {
			show_color((u_int16_t)(0xFFFF));
		} else if (strncmp(argv[1], "black", 5) == 0) {
			show_color((u_int16_t)(0x0000));
		} else if (strncmp(argv[1], "logo", 4) == 0) { /* show init logo */
			Show_Logo();
		} else if (strncmp(argv[1], "lb", 2) == 0) {
			/* show low battery logo */
			show_lowbattery();
		} else if (strncmp(argv[1], "fastboot", 2) == 0) {
			/* show low fastboot logo */
			show_fastboot();
		} else if (strncmp(argv[1], "provfailed", 10) == 0) {
			show_provfailed();
		} else {
			goto show_cmd_usage;
		}

		return 0;
	}

show_cmd_usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

void display_disable()
{
	ulong dwVal;
	int timeout = MAX_DELAY_COUNTER;

	dwVal = RegRead32(DSI_BASE, DSI_VC0_CTRL);
	dwVal &= (~(1<<0));
	RegWrite32(DSI_BASE, DSI_VC0_CTRL, dwVal);

	//wait for virtual channel 0 disabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC0_CTRL) & (1<<0)) != 0) && (timeout > 0)) {
		timeout--;
		udelay(1);
	}
	if(timeout <=0) printf("Error in virtual channel 0 disabled timeout!!!\n");

	dwVal = RegRead32(DSI_BASE, DSI_VC1_CTRL);
	dwVal &= (~(1<<0));
	RegWrite32(DSI_BASE, DSI_VC1_CTRL, dwVal);

	//wait for virtual channel 1 disabled
	timeout = MAX_DELAY_COUNTER;
	while(((RegRead32(DSI_BASE, DSI_VC1_CTRL) & (1<<0)) != 0) && (timeout > 0)) {
		timeout--;
		udelay(1);
	}
	if(timeout <=0) printf("Error in virtual channel 1 disabled timeout!!!\n");

	dwVal = RegRead32(DSI_BASE, DSI_CTRL);
	dwVal &= (~(1<<0));
	RegWrite32(DSI_BASE, DSI_CTRL, dwVal);

	//wait for interface disable
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_CTRL) & (1<<0)) != 0 ) && (timeout > 0)) {
		timeout--;
		udelay(1);
	}
	if(timeout <=0) printf("Error in DSI Interface disable timeout!!!\n");

	//switch dispc clock
	dwVal = RegRead32(DSS_BASE, DSS_CTRL);
	dwVal &= (~(1<<0));
	RegWrite32(DSS_BASE, DSS_CTRL, dwVal);

	//switch dsi clock
	dwVal = RegRead32(DSS_BASE, DSS_CTRL);
	dwVal &= (~(1<<1));
	RegWrite32(DSS_BASE, DSS_CTRL, dwVal);

	//complex I/O power off
	dwVal = RegRead32(DSI_BASE, DSI_COMPLEXIO_CFG1);
	dwVal &= (~(1 << 27));
	RegWrite32(DSI_BASE, DSI_COMPLEXIO_CFG1, dwVal);

	//Set the GOBIT bit
	dwVal = RegRead32(DSI_BASE, DSI_COMPLEXIO_CFG1);
	dwVal |= (1<<30);
	RegWrite32(DSI_BASE, DSI_COMPLEXIO_CFG1, dwVal);

	//wait for PWR_STATUS = off
	timeout = MAX_DELAY_COUNTER;
	while(( (RegRead32(DSI_BASE, DSI_COMPLEXIO_CFG1) & (0x3<<25)) != (0 << 25) ) && (timeout > 0)) {
		timeout--;
		udelay(1);
	}
	if(timeout <=0) printf("Error in wait for PWR_STATUS timeout when set dsi_complexio!!!\n");

	//disable scp clock
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal &= (~(1<<14));
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

	//power off both PLL and HSDIVISER
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal &= (~(3<<30));
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

	//Wait until PLL_PWR_STATUS = 0x0
	timeout = MAX_DELAY_COUNTER;
	while(((((RegRead32(DSI_BASE, DSI_CLK_CTRL) & 0x30000000) >> 28) != 0x0)) && (timeout > 0)) {
		timeout--;
		udelay(1);
	}
	if(timeout <=0) printf("Error in turn off PLL power fail!!! %x \n", RegRead32(DSI_BASE, DSI_CLK_CTRL));

	//disable scp clock
	dwVal = RegRead32(DSI_BASE, DSI_CLK_CTRL);
	dwVal &= (~(1<<14));
	RegWrite32(DSI_BASE, DSI_CLK_CTRL, dwVal);

}

void display_reinit()
{
	//init DSS
	init_DSS();

	//Init DISPC
	set_DISPC(FRAME_BUFFER_ADDR);

	//rset DSI
	dsi_reset();

	//Set DPLL for DSI
	dsi_set_dpll();

	//set DSI Phy
	dsi_set_phy();

	//set DSI protocol engine
	dsi_set_protocol_engine();

	//set to Videomode
	dsi_video_preinit();

	//enter Bist Mode
	//LCD_Bist();

	//Normal display
	LCD_Normal();

	//Post init for Video Mode
	dsi_videomode_PostInit();

	//Enable LCD1 output
	Enable_LCD();
}

//**************************************************************************************
// Function:  U_BOOT_CMD
// Description: Uboot command define
//**************************************************************************************
U_BOOT_CMD(
	show, 2, 1, show_main,
	"show   - show color or logo on the DSI display\n",
	"show [blue, green, red, black, white, logo, lb, fastboot] \n"
);

static int do_brightness_lp8552(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]){
	printf("brightness = %s\n", argv[1]);
	Set_Brightness(simple_strtoul(argv[1], NULL, 8));
}

U_BOOT_CMD(
	bri, 2, 1, do_brightness_lp8552,
	"Usage: bri 0x0 - 0xff\n",
	NULL
);


static int show_ch(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]){
	printf("show_charging\n");
	show_charging();
}

U_BOOT_CMD(
	show_ch, 2, 1, show_ch,
	"Usage: show_ch\n",
	NULL
);

static int show_lo(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]){
	printf("show_lowbattery\n");
	show_lowbattery();
}

U_BOOT_CMD(
	show_lo, 2, 1, show_lo,
	"Usage: show_lo\n",
	NULL
);

static int show_ho(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]){
	printf("show_devicetoohot\n");
	show_devicetoohot();
}

U_BOOT_CMD(
	show_ho, 2, 1, show_ho,
	"Usage: show_ho\n",
	NULL
);
