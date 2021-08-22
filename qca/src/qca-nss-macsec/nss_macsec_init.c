/*
 * Copyright (c) 2014, 2018-2019 The Linux Foundation. All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/phy.h>
#include <net/sock.h>
#include <asm/io.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0))
#include <mach/msm_iomap.h>
#else
#include <linux/of.h>
#include <linux/of_mdio.h>
#ifdef MACSEC_IPQ806X_SUPPORT
#include <soc/qcom/socinfo.h>
#endif
#endif

#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/notifier.h>

#include "nss_macsec_emac.h"
#include "nss_macsec_sdk_api.h"
#include "nss_macsec_fal_api.h"
#include "nss_macsec_utility.h"
#include "nss_macsec_os.h"
#include "nss_macsec_ops.h"

/* NSS MACSEC Base Addresses */
#define NSS_MACSEC_BASE			0x37800000
#define NSS_MACSEC_REG_LEN		0x00200000
#define MACSEC_DEVICE_NUM 3

static struct sock *sdk_nl_sk = NULL;

static bool phy_driver_flag[MACSEC_DEVICE_NUM] = {FALSE, FALSE, FALSE};
static bool secy_phy_flag;
/**
 * @brief MACSEC driver context
 */
struct nss_macsec_ctx {
	char __iomem *macsec_base[MACSEC_DEVICE_NUM];
};
static struct nss_macsec_ctx macsec_ctx;
static uint32_t macsec_notifier_register_status = 0;
static uint32_t nss_macsec_pre_init_flag = 0;


#ifdef MACSEC_IPQ806X_SUPPORT
static int macsec_ncb(struct notifier_block *nb, unsigned long value,
		      void *priv)
{
	int result = NOTIFY_OK;
	struct nss_gmac_speed_ctx *gmac_speed_ctx_p = priv;
	printk("macsec_ncb  mac_id:0x%x speed:0x%x\n",
		gmac_speed_ctx_p->mac_id, gmac_speed_ctx_p->speed);
	switch (value) {
	case NSS_GMAC_SPEED_SET:
		nss_macsec_speed(gmac_speed_ctx_p->mac_id - 1, gmac_speed_ctx_p->speed);
		break;
	default:
		result = NOTIFY_BAD;
	}
	return result;
}

static struct notifier_block macsec_notifier __attribute__ ((unused)) = {
	.notifier_call = macsec_ncb,
};
#endif

unsigned int nss_macsec_device_exist(unsigned int dev_id)
{
	if(macsec_ctx.macsec_base[dev_id] == NULL) {
		printk("macsec[%d] doesn't exist, any operation for it is not permitted!\n", dev_id);
		return ERROR_NOT_SUPPORT;
	}
	return ERROR_OK;
}

int nss_macsec_reg_read(unsigned int dev_id, unsigned int reg_addr,
			unsigned int *pvalue)
{
	if(macsec_ctx.macsec_base[dev_id] == NULL) {
		return ERROR_NOT_SUPPORT;
	}

	*pvalue =
	    readl((unsigned char *)((char *) macsec_ctx.macsec_base[dev_id] +
				    (reg_addr)));
	return 0;
}

int nss_macsec_reg_write(unsigned int dev_id, unsigned int reg_addr,
			 unsigned int pvalue)
{
	if(macsec_ctx.macsec_base[dev_id] == NULL) {
		return ERROR_NOT_SUPPORT;
	}
	writel(pvalue,
	       (unsigned char *)((char *) macsec_ctx.macsec_base[dev_id] +
				 (reg_addr)));
	return 0;
}

static unsigned int nss_macsec_msg_handle(void *msg_data, unsigned int *sync)
{
	struct sdk_msg_header *header = (struct sdk_msg_header *)msg_data;
	unsigned short cmd_type = header->cmd_type;
	unsigned int ret = 0;

	*sync = 1;
	switch (cmd_type) {
	case SDK_FAL_CMD:{
			ret = nss_macsec_fal_msg_handle(header);
		}
		break;

	default:
		break;
	}

	return ret;
}

static void nss_macsec_netlink_recv(struct sk_buff *__skb)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct sdk_msg_header *header = NULL;
	void *msg_data;
	u32 pid = 0, msg_type = 0, sync = 0, ret = 0, msg_len = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
	if ((skb = skb_clone(__skb, GFP_KERNEL)) != NULL) {
#else
	if ((skb = skb_get(__skb)) != NULL) {
#endif
		nlh = nlmsg_hdr(skb);
		pid = nlh->nlmsg_pid;
		msg_data = NLMSG_DATA(nlh);
		msg_type = nlh->nlmsg_type;
		msg_len = sizeof(struct nlmsghdr) + sizeof(struct sdk_msg_header);
		header = (struct sdk_msg_header *)msg_data;
		if (header->buf_len > (U32_MAX - msg_len))
			return;
		msg_len += header->buf_len;

		if(skb->len < msg_len) {
			printk("Unexpected msg received! skb_len [0x%x] less than 0x%x\n",
				skb->len, msg_len);
			return;
		} else if (msg_type == SDK_CALL_MSG) {
			ret = nss_macsec_msg_handle(msg_data, &sync);
			header->ret = ret;
		} else {
			printk("Unexpected msg:0x%x received!\n", msg_type);
			return;
		}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
		NETLINK_CB(skb).portid = 0; /* from kernel */
#else
		NETLINK_CB(skb).pid = 0;		/* from kernel */
#endif
		NETLINK_CB(skb).dst_group = 0;	/* unicast */
		netlink_unicast(sdk_nl_sk, skb, pid, MSG_DONTWAIT);
	}

	return;
}

static int nss_macsec_netlink_init(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
	struct netlink_kernel_cfg cfg = {
			.input			= nss_macsec_netlink_recv,
	};

	sdk_nl_sk = netlink_kernel_create(&init_net, NETLINK_SDK, &cfg);
#else
	sdk_nl_sk = netlink_kernel_create(&init_net,
						NETLINK_SDK,
						0,
						nss_macsec_netlink_recv,
						NULL, THIS_MODULE);
#endif

	if (sdk_nl_sk == NULL) {
		printk("Create netlink socket fail\n");
		return -ENODEV;
	}

	return 0;
}

static struct mii_bus *nss_macsec_miibus[MAX_SECY_ID] = {NULL, NULL, NULL};

int nss_macsec_phy_mdio_write(u32 secy_id, u32 reg_addr, u16 val)
{
	u32 phy_addr;

	if (!nss_macsec_miibus[secy_id])
		return -EIO;

	phy_addr = secy_id_to_phy_addr(secy_id);
	mdiobus_write(nss_macsec_miibus[secy_id], phy_addr, reg_addr, val);

	return 0;
}

int nss_macsec_phy_mdio_read(u32 secy_id, u32 reg_addr, u16 *val)
{
	u32 phy_addr;

	if (!nss_macsec_miibus[secy_id])
		return -EIO;

	phy_addr = secy_id_to_phy_addr(secy_id);
	*val = mdiobus_read(nss_macsec_miibus[secy_id], phy_addr, reg_addr);

	return 0;
}


static int nss_macsec_mdiobus_init(struct device_node *macsec_node, u32 secy_id)
{
	struct device_node *mdio_node = NULL;

	if (secy_id >= MAX_SECY_ID) {
		macsec_warning("Invalid secy id:%d!\n", secy_id);
		return -EINVAL;
	}

	mdio_node = of_parse_phandle(macsec_node, "mdiobus", 0);
	if (!mdio_node) {
		macsec_warning("Failed to find mdio node!\n");
		return -EINVAL;
	}
	nss_macsec_miibus[secy_id] = of_mdio_find_bus(mdio_node);
	if (!nss_macsec_miibus[secy_id]) {
		macsec_warning("Failed to find mdio bus!\n");
		return -EIO;
	}

	return 0;
}

static void nss_macsec_netlink_fini(void)
{
	if (sdk_nl_sk) {
		sock_release(sdk_nl_sk->sk_socket);
		sdk_nl_sk = NULL;
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
static int nss_macsec_dt_init(uint32_t dev_id)
{
	int ret = 0;
	unsigned int mem_start, mem_len;
	void __iomem *mmap_io_addr = NULL;
	struct device_node *nss_macsec_node = NULL;
	const __be32 *reg_cfg;
	unsigned int len = 0;
	char dev_name[32];
	unsigned int phy_address;

	memset(dev_name, 0, sizeof(dev_name));
	snprintf(dev_name, sizeof(dev_name), "nss-macsec%d", dev_id);

	nss_macsec_node = of_find_node_by_name(NULL, dev_name);
	if (!nss_macsec_node) {
		printk(KERN_ERR "cannot find nss-macsec%d node\n", dev_id);
		return -ENODEV;
	}
	printk(KERN_INFO "%s DT exist!\n", dev_name);

	nss_macsec_mdiobus_init(nss_macsec_node, dev_id);

/* add for Napa masec*/
	if (!of_property_read_u32(nss_macsec_node, "phy_addr", &phy_address)) {
		nss_macsec_secy_info_set(dev_id, phy_address);
		nss_macsec_secy_driver_init(dev_id);
		phy_driver_flag[dev_id] = TRUE;
		secy_phy_flag = TRUE;
		return ret;
	}
/* end for Napa masec*/

	reg_cfg = of_get_property(nss_macsec_node, "reg", &len);
	if (!reg_cfg) {
		printk(KERN_ERR "%s: error reading device node properties for reg\n", dev_name);
		return -EIO;
	}

	mem_start = be32_to_cpup(reg_cfg);
	mem_len = be32_to_cpup(reg_cfg + 1);

	if (!request_mem_region(mem_start, mem_len, dev_name)) {
		macsec_warning("%s Unable to request resource.", __func__);
		return -EIO;
	}

	mmap_io_addr = ioremap_nocache(mem_start, mem_len);
	if (!mmap_io_addr) {
		macsec_warning("%s ioremap fail.", __func__);
		return -EIO;
	}
	macsec_trace("macsec.%d phy_addr:0x%x len:0x%x mem_start:%p\n",
		     dev_id, mem_start, mem_len, mmap_io_addr);

	macsec_ctx.macsec_base[dev_id] = mmap_io_addr;

	if (nss_macsec_pre_init_flag == 0) {
#ifdef MACSEC_IPQ806X_SUPPORT
		nss_macsec_pre_init();
#endif
		nss_macsec_pre_init_flag = 1;
	}

	/* Invoke Macsec Initialization API */
	nss_macsec_secy_init(dev_id);

	if (macsec_notifier_register_status == 0) {
#ifdef MACSEC_IPQ806X_SUPPORT
		nss_gmac_link_state_change_notify_register(&macsec_notifier);
#endif
		macsec_notifier_register_status = 1;
	}

	macsec_trace("macsec.%d probe done\n", dev_id);
	return ret;

}

static int nss_macsec_clean(void)
{
	uint32_t dev_id = 0;
	int ret = 0, secy_id = 0;

	if (secy_phy_flag == TRUE) {
		for (secy_id = 0; secy_id < MACSEC_DEVICE_NUM; secy_id++) {
			if (phy_driver_flag[secy_id] == TRUE)
				nss_macsec_secy_driver_cleanup(secy_id);
		}
	} else {
		if (macsec_notifier_register_status) {
#ifdef MACSEC_IPQ806X_SUPPORT
			nss_gmac_link_state_change_notify_unregister(
				&macsec_notifier);
#endif
			macsec_notifier_register_status = 0;
		}

		for (dev_id = 0; dev_id < MACSEC_DEVICE_NUM; dev_id++) {
			iounmap(macsec_ctx.macsec_base[dev_id]);
			release_mem_region(
				NSS_MACSEC_BASE+(NSS_MACSEC_REG_LEN*dev_id),
					    NSS_MACSEC_REG_LEN);
		}
	}
	return ret;
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0))
static int nss_macsec_probe(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int mem_start, mem_len;
	void __iomem *mmap_io_addr = NULL;

	if (nss_macsec_pre_init_flag == 0) {
#ifdef MACSEC_IPQ806X_SUPPORT
		nss_macsec_pre_init();
#endif
		nss_macsec_pre_init_flag = 1;
	}

	mem_start = pdev->resource[0].start;
	mem_len = pdev->resource[0].end - mem_start + 1;
	if (!request_mem_region(mem_start, mem_len, pdev->name)) {
		macsec_warning("%s Unable to request resource.", __func__);
		return -EIO;
	}

	mmap_io_addr = ioremap_nocache(mem_start, mem_len);
	if (!mmap_io_addr) {
		macsec_warning("%s ioremap fail.", __func__);
		return -EIO;
	}
	macsec_trace("macsec.%d phy_addr:0x%x len:0x%x mem_start:%p\n",
		     pdev->id, mem_start, mem_len, mmap_io_addr);

	macsec_ctx.macsec_base[pdev->id] = mmap_io_addr;

	/* Invoke Macsec Initialization API */
	nss_macsec_secy_init(pdev->id);

	if (macsec_notifier_register_status == 0) {
#ifdef MACSEC_IPQ806X_SUPPORT
		nss_gmac_link_state_change_notify_register(&macsec_notifier);
#endif
		macsec_notifier_register_status = 1;
	}

	macsec_trace("macsec.%d probe done\n", pdev->id);
	return ret;
}

static int nss_macsec_remove(struct platform_device *pdev)
{
	unsigned int mem_start, mem_len;

	macsec_trace("%s for dev_id:%d\n", __func__, pdev->id);
	mem_start = pdev->resource[0].start;
	mem_len = pdev->resource[0].end - mem_start + 1;

	if (macsec_notifier_register_status) {
#ifdef MACSEC_IPQ806X_SUPPORT
		nss_gmac_link_state_change_notify_unregister(&macsec_notifier);
#endif
		macsec_notifier_register_status = 0;
	}
	iounmap(macsec_ctx.macsec_base[pdev->id]);
	release_mem_region(mem_start, mem_len);
	return 0;
}

/**
 * @brief Linux Platform driver for MACSEC
 */
static struct platform_driver nss_macsec_drv __attribute__ ((unused)) = {
	.probe = nss_macsec_probe,
	.remove = nss_macsec_remove,
	.driver = {
		   .name = "nss-macsec",
		   .owner = THIS_MODULE,
		   },
};
#endif

static int __init nss_macsec_init_module(void)
{
	uint32_t dev_id = 0;
	int ret = 0;

	for(dev_id=0; dev_id<MACSEC_DEVICE_NUM; dev_id++)
		macsec_ctx.macsec_base[dev_id] = NULL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
#ifdef MACSEC_IPQ806X_SUPPORT
	if (cpu_is_ipq8066() || cpu_is_ipq8068() || cpu_is_ipq8069()) {
#endif
		for (dev_id = 0; dev_id < MACSEC_DEVICE_NUM; dev_id++) {
			ret = nss_macsec_dt_init(dev_id);
			if (ret && (ret != -ENODEV))
				return -EIO;
		}
#ifdef MACSEC_IPQ806X_SUPPORT
	}
#endif
#else
	if (platform_driver_register(&nss_macsec_drv) != 0) {
		macsec_warning("platform drv reg failure\n");
		return -EIO;
	}
#endif
	macsec_trace("%s done!\n", __func__);

	return ret;
}

static int __init nss_macsec_init(void)
{
	nss_macsec_netlink_init();

	nss_macsec_mutex_init();

	nss_macsec_init_module();

	printk("nss_macsec init success\n");

	return 0;
}

static void __exit nss_macsec_fini(void)
{
	nss_macsec_netlink_fini();

	nss_macsec_mutex_destroy();
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0))
	platform_driver_unregister(&nss_macsec_drv);
#else
	nss_macsec_clean();
#endif
	if (nss_macsec_pre_init_flag) {
#ifdef MACSEC_IPQ806X_SUPPORT
		nss_macsec_pre_exit();
#endif
		nss_macsec_pre_init_flag = 0;
	}

	printk("nss_macsec exit success\n");
}

module_init(nss_macsec_init);
module_exit(nss_macsec_fini);
#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif
