/* Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _CNSS_BUS_H
#define _CNSS_BUS_H

#include "main.h"

enum cnss_dev_bus_type cnss_get_bus_type(unsigned long device_id);
void *cnss_bus_dev_to_bus_priv(struct device *dev);
struct cnss_plat_data *cnss_bus_dev_to_plat_priv(struct device *dev);
int cnss_bus_init(struct cnss_plat_data *plat_priv);
int cnss_bus_init_by_type(int type);
void cnss_bus_deinit(struct cnss_plat_data *plat_priv);
int cnss_bus_load_m3(struct cnss_plat_data *plat_priv);
int cnss_bus_alloc_fw_mem(struct cnss_plat_data *plat_priv);
void cnss_bus_free_fw_mem(struct cnss_plat_data *plat_priv);
int cnss_bus_alloc_qdss_mem(struct cnss_plat_data *plat_priv);
void cnss_bus_free_qdss_mem(struct cnss_plat_data *plat_priv);
u32 cnss_bus_get_wake_irq(struct cnss_plat_data *plat_priv);
int cnss_bus_force_fw_assert_hdlr(struct cnss_plat_data *plat_priv);
void cnss_bus_fw_boot_timeout_hdlr(struct timer_list *timer);
void cnss_bus_collect_dump_info(struct cnss_plat_data *plat_priv,
				bool in_panic);
int cnss_bus_call_driver_probe(struct cnss_plat_data *plat_priv);
int cnss_bus_call_driver_remove(struct cnss_plat_data *plat_priv);
int cnss_bus_dev_powerup(struct cnss_plat_data *plat_priv);
int cnss_bus_dev_shutdown(struct cnss_plat_data *plat_priv);
int cnss_bus_dev_crash_shutdown(struct cnss_plat_data *plat_priv);
int cnss_bus_dev_ramdump(struct cnss_plat_data *plat_priv);
int cnss_bus_register_driver_hdlr(struct cnss_plat_data *plat_priv, void *data);
int cnss_bus_unregister_driver_hdlr(struct cnss_plat_data *plat_priv);
int cnss_bus_call_driver_modem_status(struct cnss_plat_data *plat_priv,
				      int modem_current_status);
int cnss_bus_update_status(struct cnss_plat_data *plat_priv,
			   enum cnss_driver_status status);

#endif /* _CNSS_BUS_H */
