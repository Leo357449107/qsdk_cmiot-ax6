/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "sw.h"
#include "adpt.h"
#include "hsl.h"
#include "hsl_dev.h"
#include "hsl_phy.h"
#include "hsl_port_prop.h"
#include "adpt_appe.h"

static sw_error_t
adpt_appe_led_ctrl_pattern_set(a_uint32_t dev_id, led_pattern_group_t group,
	led_pattern_id_t led_pattern_id, led_ctrl_pattern_t * pattern)
{
	sw_error_t rv;
	a_uint32_t port_id;

	HSL_DEV_ID_CHECK(dev_id);
	if(group != LED_LAN_PORT_GROUP && group != LED_WAN_PORT_GROUP)
	{
		SSDK_ERROR("group %x is not supported\n", group);
		return SW_NOT_SUPPORTED;
	}

	port_id = led_pattern_id;
	if (A_TRUE != hsl_port_prop_check(dev_id, port_id, HSL_PP_PHY))
	{
		return SW_BAD_PARAM;
	}

	rv = hsl_port_phy_led_ctrl_pattern_set (dev_id, port_id, pattern);

	return rv;
}

static sw_error_t
adpt_appe_led_ctrl_pattern_get(a_uint32_t dev_id, led_pattern_group_t group,
	led_pattern_id_t led_pattern_id, led_ctrl_pattern_t * pattern)
{
	sw_error_t rv;
	a_uint32_t port_id;

	HSL_DEV_ID_CHECK(dev_id);
	if(group != LED_LAN_PORT_GROUP && group != LED_WAN_PORT_GROUP)
	{
		SSDK_ERROR("group %x is not supported\n", group);
		return SW_NOT_SUPPORTED;
	}

	port_id = led_pattern_id;
	if (A_TRUE != hsl_port_prop_check(dev_id, port_id, HSL_PP_PHY))
	{
		return SW_BAD_PARAM;
	}

	rv = hsl_port_phy_led_ctrl_pattern_get (dev_id, port_id, pattern);

	return rv;
}

static sw_error_t
adpt_appe_led_ctrl_source_set(a_uint32_t dev_id, a_uint32_t source_id,
	led_ctrl_pattern_t *pattern)
{
	sw_error_t rv;
	a_uint32_t port_id;

	HSL_DEV_ID_CHECK(dev_id);
	/*one port can support max three led source*/
	port_id = source_id/PORT_LED_SOURCE_MAX+1;
	source_id = source_id%PORT_LED_SOURCE_MAX;
	if (A_TRUE != hsl_port_prop_check(dev_id, port_id, HSL_PP_PHY))
	{
		return SW_BAD_PARAM;
	}

	rv = hsl_port_phy_led_ctrl_source_set (dev_id, port_id, source_id, pattern);

	return rv;
}

void adpt_appe_led_func_bitmap_init(a_uint32_t dev_id)
{
	adpt_api_t *p_adpt_api = NULL;

	p_adpt_api = adpt_api_ptr_get(dev_id);

	if(p_adpt_api == NULL)
		return;

	p_adpt_api->adpt_led_func_bitmap = \
		((1 << FUNC_LED_CTRL_PATTERN_SET)|
		(1 << FUNC_LED_CTRL_PATTERN_GET)|
		(1 << FUNC_LED_CTRL_SOURCE_SET));

	return;
}

static void adpt_appe_led_func_unregister(a_uint32_t dev_id, adpt_api_t *p_adpt_api)
{
	if(p_adpt_api == NULL)
		return;

	p_adpt_api->adpt_led_ctrl_pattern_set = NULL;
	p_adpt_api->adpt_led_ctrl_pattern_get = NULL;
	p_adpt_api->adpt_led_ctrl_source_set = NULL;

	return;
}

sw_error_t adpt_appe_led_init(a_uint32_t dev_id)
{
	adpt_api_t *p_adpt_api = NULL;

	p_adpt_api = adpt_api_ptr_get(dev_id);

	SW_RTN_ON_NULL (p_adpt_api);
	adpt_appe_led_func_unregister(dev_id, p_adpt_api);
	if (p_adpt_api->adpt_led_func_bitmap & BIT(FUNC_LED_CTRL_PATTERN_SET))
	{
		p_adpt_api->adpt_led_ctrl_pattern_set = adpt_appe_led_ctrl_pattern_set;
	}
	if (p_adpt_api->adpt_led_func_bitmap & BIT(FUNC_LED_CTRL_PATTERN_GET))
	{
		p_adpt_api->adpt_led_ctrl_pattern_get = adpt_appe_led_ctrl_pattern_get;
	}
	if (p_adpt_api->adpt_led_func_bitmap & BIT(FUNC_LED_CTRL_SOURCE_SET))
	{
		p_adpt_api->adpt_led_ctrl_source_set = adpt_appe_led_ctrl_source_set;
	}

	return SW_OK;
}
/**
 * @}
 */
