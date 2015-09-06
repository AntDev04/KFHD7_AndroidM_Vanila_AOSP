/*
 * smb347.h
 *
 * Copyright (C) Amazon Technologies Inc. All rights reserved.
 * Donald Chan (hoiho@lab126.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if defined(CONFIG_SMB347)

/* SMB347 registers */
#define SMB347_COMMAND_REG_A	(0x30)
#define SMB347_COMMAND_REG_B	(0x31)
#define SMB347_COMMAND_REG_C	(0x33)
#define SMB347_STATUS_REG_A	(0x3b)
#define SMB347_STATUS_REG_B	(0x3c)
#define SMB347_STATUS_REG_C	(0x3d)
#define SMB347_STATUS_REG_D	(0x3e)
#define SMB347_STATUS_REG_E	(0x3f)

#define SMB347_IS_APSD_DONE(value)	((value) & (1 << 3))

#define SMB347_APSD_RESULT(value)	((value) & 0x7)
#define SMB347_APSD_RESULT_NONE		(0)
#define SMB347_APSD_RESULT_CDP		(1)
#define SMB347_APSD_RESULT_DCP		(2)
#define SMB347_APSD_RESULT_OTHER	(3)
#define SMB347_APSD_RESULT_SDP		(4)
#define SMB347_APSD_RESULT_ACA		(5)
#define SMB347_APSD_RESULT_TBD_1	(6)
#define SMB347_APSD_RESULT_TBD_2	(7)

#define SMB347_CHARGING_STATUS(value)	(((value) >> 1) & 0x3)

#define SMB347_CHARGING_STATUS_NOT_CHARGING	(0)
#define SMB347_CHARGING_STATUS_PRE_CHARGING	(1)
#define SMB347_CHARGING_STATUS_FAST_CHARGING	(2)
#define SMB347_CHARGING_STATUS_TAPER_CHARGING	(3)
#define SMB347_CHARGING_STATUS_UNKNOWN		(4)

#define SMB347_USB1_5_HC_MODE(value)	(((value) >> 5) & 0x3)

#define SMB347_HC_MODE			(0)
#define SMB347_USB1_MODE		(1)
#define SMB347_USB5_MODE		(2)

#define SMB347_IS_AICL_DONE(value)	((value) & (1 << 4))

#define SMB347_AICL_RESULT(value)	((value) & 0xf)

#endif
