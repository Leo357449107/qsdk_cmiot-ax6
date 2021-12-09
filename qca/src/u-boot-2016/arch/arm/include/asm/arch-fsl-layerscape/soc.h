/*
 * Copyright 2015 Freescale Semiconductor
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ASM_ARMV8_FSL_LAYERSCAPE_SOC_H_
#define _ASM_ARMV8_FSL_LAYERSCAPE_SOC_H_

#ifdef CONFIG_SYS_FSL_CCSR_GUR_LE
#define gur_in32(a)       in_le32(a)
#define gur_out32(a, v)   out_le32(a, v)
#elif defined(CONFIG_SYS_FSL_CCSR_GUR_BE)
#define gur_in32(a)       in_be32(a)
#define gur_out32(a, v)   out_be32(a, v)
#endif

#ifdef CONFIG_SYS_FSL_CCSR_SCFG_LE
#define scfg_in32(a)       in_le32(a)
#define scfg_out32(a, v)   out_le32(a, v)
#elif defined(CONFIG_SYS_FSL_CCSR_SCFG_BE)
#define scfg_in32(a)       in_be32(a)
#define scfg_out32(a, v)   out_be32(a, v)
#endif

#ifdef CONFIG_SYS_FSL_PEX_LUT_LE
#define pex_lut_in32(a)       in_le32(a)
#define pex_lut_out32(a, v)   out_le32(a, v)
#elif defined(CONFIG_SYS_FSL_PEX_LUT_BE)
#define pex_lut_in32(a)       in_be32(a)
#define pex_lut_out32(a, v)   out_be32(a, v)
#endif

struct cpu_type {
	char name[15];
	u32 soc_ver;
	u32 num_cores;
};

#define CPU_TYPE_ENTRY(n, v, nc) \
	{ .name = #n, .soc_ver = SVR_##v, .num_cores = (nc)}

#define SVR_WO_E		0xFFFFFE
#define SVR_LS1043		0x879204
#define SVR_LS2045		0x870120
#define SVR_LS2080		0x870110
#define SVR_LS2085		0x870100

#define SVR_MAJ(svr)		(((svr) >> 4) & 0xf)
#define SVR_MIN(svr)		(((svr) >> 0) & 0xf)
#define SVR_SOC_VER(svr)	(((svr) >> 8) & SVR_WO_E)
#define IS_E_PROCESSOR(svr)	(!((svr >> 8) & 0x1))

/* ahci port register default value */
#define AHCI_PORT_PHY_1_CFG    0xa003fffe
#define AHCI_PORT_PHY_2_CFG    0x28184d1f
#define AHCI_PORT_PHY_3_CFG    0x0e081509
#define AHCI_PORT_TRANS_CFG    0x08000029

/* AHCI (sata) register map */
struct ccsr_ahci {
	u32 res1[0xa4/4];	/* 0x0 - 0xa4 */
	u32 pcfg;	/* port config */
	u32 ppcfg;	/* port phy1 config */
	u32 pp2c;	/* port phy2 config */
	u32 pp3c;	/* port phy3 config */
	u32 pp4c;	/* port phy4 config */
	u32 pp5c;	/* port phy5 config */
	u32 axicc;	/* AXI cache control */
	u32 paxic;	/* port AXI config */
	u32 axipc;	/* AXI PROT control */
	u32 ptc;	/* port Trans Config */
	u32 pts;	/* port Trans Status */
	u32 plc;	/* port link config */
	u32 plc1;	/* port link config1 */
	u32 plc2;	/* port link config2 */
	u32 pls;	/* port link status */
	u32 pls1;	/* port link status1 */
	u32 pcmdc;	/* port CMD config */
	u32 ppcs;	/* port phy control status */
	u32 pberr;	/* port 0/1 BIST error */
	u32 cmds;	/* port 0/1 CMD status error */
};

#ifdef CONFIG_FSL_LSCH3
void fsl_lsch3_early_init_f(void);
#elif defined(CONFIG_FSL_LSCH2)
void fsl_lsch2_early_init_f(void);
#endif

void cpu_name(char *name);
#ifdef CONFIG_SYS_FSL_ERRATUM_A009635
void erratum_a009635(void);
#endif
#endif /* _ASM_ARMV8_FSL_LAYERSCAPE_SOC_H_ */
