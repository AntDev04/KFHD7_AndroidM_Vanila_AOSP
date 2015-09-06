/*==============================================================================
*
*            TEXAS INSTRUMENTS INCORPORATED PROPRIETARY INFORMATION
*
*   Property of Texas Instruments
*   For Unrestricted Internal Use Only
*   Unauthorized reproduction and/or distribution is strictly prohibited.
*   This product is protected under copyright law and trade secret law as an
*   unpublished work.
*   Created 2010, (C) Copyright 2010 Texas Instruments.  All rights reserved.
*
*   Component  :  
*
*   Filename   : 
*
*   Description: 
*
*-------------------------------------------------------------------------------
*=============================================================================*/

#ifndef _OMAP4_SIGNATURE_H_
#define _OMAP4_SIGNATURE_H_

#define CERT_RSA_PK_Verify_PK               (1<<0)
#define CERT_RSA_PK_Verify_ISW              (1<<1)
#define CERT_RSA_PK_Verify_PPA              (1<<2)
#define CERT_RSA_PK_Verify_PAU              (1<<3)
#define CERT_RSA_PK_Verify_PAS              (1<<4)
#define CERT_RSA_PK_Verify_RD1              (1<<5)
#define CERT_RSA_PK_Verify_RD2              (1<<6)
#define CERT_RSA_PK_Verify_KI               (1<<7)

#define ISW_CERTIFICATE_LENGTH              0x238	/* SW certificate len excluding RSA signature */
#define ISW_CERTIFICATE_LENGTH_FULL         0x350	/* SW certificate len in full */

typedef struct                                        
{
	u32 start_offset;
	u32 length;
	u8 hash[20];
}CERT_Hashes;

typedef struct 
{   
	u32 *out;		/* Ouput pointer */  
	u32 *in;		/* Input pointer */
	u32 in_byte_size;	/* Size of the input in bytes */
	u32 dma;		/* 1(DMA), 0(No DMA) */
}Cl_Hash_SHA1_Struct;

#define API_HAL_KM_VERIFYCERTIFICATESIGNATURE_INDEX	0x0000000C
#define API_HAL_PPA_SERV_HASH_SHA1_INDEX		0x0000002B

#define API_HAL_RET_VALUE_OK	0x0

#endif /* _OMAP4_SIGNATURE_H_ */
