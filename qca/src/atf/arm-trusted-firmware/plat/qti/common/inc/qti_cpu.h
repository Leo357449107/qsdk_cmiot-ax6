/*
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __QTI_CPU_H__
#define __QTI_CPU_H__

/* KRYO-3xx Gold MIDR */
#define QTI_KRYO3_GOLD_MIDR	0x517F802D
#define QTI_KRYO3_GOLD_IMPL_PN	0x51008020

/* KRYO-3xx Silver MIDR */
#define QTI_KRYO3_SILVER_MIDR	0x517F803D

#ifndef __ASSEMBLER__
/* API to execute CPU specific cache maintenance
 * operation before power collapse.
 * Argument xo is used to determine whether
 * L2 flush is required or not.
 */
void qti_cpu_cm_at_pc(uint8_t);
#endif /* __ASSEMBLER__ */

#endif /* __QTI_CPU_H__ */
