/* include/linux/usb/otg.h
 *
 * Copyright (c) 2015 Texas Instruments Incorporated - http://www.ti.com
 *
 * USB OTG (On The Go) defines
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __LINUX_USB_OTG_H
#define __LINUX_USB_OTG_H

enum usb_dr_mode {
	USB_DR_MODE_UNKNOWN,
	USB_DR_MODE_HOST,
	USB_DR_MODE_PERIPHERAL,
	USB_DR_MODE_OTG,
};

#endif /* __LINUX_USB_OTG_H */
