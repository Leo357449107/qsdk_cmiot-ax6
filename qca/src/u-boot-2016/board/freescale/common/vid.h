/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __VID_H_
#define __VID_H_

#define IR36021_LOOP1_MANUAL_ID_OFFSET	0x6A
#define IR36021_LOOP1_VOUT_OFFSET	0x9A
#define IR36021_MFR_ID_OFFSET		0x92
#define IR36021_MFR_ID			0x43

/* step the IR regulator in 5mV increments */
#define IR_VDD_STEP_DOWN		5
#define IR_VDD_STEP_UP			5
int adjust_vdd(ulong vdd_override);

#endif  /* __VID_H_ */
