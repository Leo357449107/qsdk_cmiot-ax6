/*
 * (C) Copyright 2015 Hans de Goede <hdegoede@redhat.com>
 *
 * X-Powers AX Power Management IC support header
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _AXP_PMIC_H_

#ifdef CONFIG_AXP152_POWER
#include <axp152.h>
#endif
#ifdef CONFIG_AXP209_POWER
#include <axp209.h>
#endif
#ifdef CONFIG_AXP221_POWER
#include <axp221.h>
#endif
#ifdef CONFIG_AXP818_POWER
#include <axp818.h>
#endif

int axp_set_dcdc1(unsigned int mvolt);
int axp_set_dcdc2(unsigned int mvolt);
int axp_set_dcdc3(unsigned int mvolt);
int axp_set_dcdc4(unsigned int mvolt);
int axp_set_dcdc5(unsigned int mvolt);
int axp_set_aldo1(unsigned int mvolt);
int axp_set_aldo2(unsigned int mvolt);
int axp_set_aldo3(unsigned int mvolt);
int axp_set_aldo4(unsigned int mvolt);
int axp_set_dldo1(unsigned int mvolt);
int axp_set_dldo2(unsigned int mvolt);
int axp_set_dldo3(unsigned int mvolt);
int axp_set_dldo4(unsigned int mvolt);
int axp_set_eldo(int eldo_num, unsigned int mvolt);
int axp_init(void);
int axp_get_sid(unsigned int *sid);

#endif
