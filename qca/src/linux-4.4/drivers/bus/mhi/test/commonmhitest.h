/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include <linux/err.h>
#include <soc/qcom/subsystem_restart.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-direction.h>
#include <linux/mhi.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/irq.h>

#define MHI_MAX_DEVICE 2

/*PCI/PCIe specific macro here  */
#define PCI_BAR_NUM                     0
#define PCI_DMA_MASK_64_BIT		64
#define MHI_TIMEOUT_DEFAULT		10000
#define PCIE_SOC_GLOBAL_RESET_VALUE	0x5
#define PCIE_SOC_GLOBAL_RESET_ADDRESS	0x3008
#define MAX_SOC_GLOBAL_RESET_WAIT_CNT	50 /* x 20msec */

/* Add DEBUG related here  */

enum MHITEST_DEBUG_KLVL{
	MHITEST_LOG_LVL_VERBOSE,
	MHITEST_LOG_LVL_INFO,
	MHITEST_LOG_LVL_ERR,
};
extern int debug_lvl;
#define PCI_MHI_TEST_DEBUG 1
#ifdef PCI_MHI_TEST_DEBUG
#define pr_mhitest(msg, ...)  pr_err("[mhitest]: " msg,  ##__VA_ARGS__)
#define pr_mhitest2(msg, ...) \
	pr_err("[mhitest]: %s[%d] " msg, __func__, __LINE__,   ##__VA_ARGS__)

#define MHITEST_EMERG(msg, ...) do {\
		pr_err("[mhitest][A]: [%s] " msg, __func__,   ##__VA_ARGS__);\
} while (0)

#define MHITEST_ERR(msg, ...) do {\
	if  (debug_lvl <= MHITEST_LOG_LVL_ERR) \
		pr_err("[mhitest][E]: [%s] " msg, __func__,   ##__VA_ARGS__);\
} while (0)

#define MHITEST_VERB(msg, ...) do {\
	if  (debug_lvl <= MHITEST_LOG_LVL_VERBOSE) \
		pr_err("[mhitest][D]: [%s][%d] " msg, __func__, __LINE__,   ##__VA_ARGS__);\
} while (0)

#define MHITEST_LOG(msg, ...) do {\
	if  (debug_lvl <= MHITEST_LOG_LVL_INFO) \
		pr_err("[mhitest][I]: [%s] " msg, __func__,   ##__VA_ARGS__);\
} while (0)
#else
#define pr_mhitest(msg) \
	do { } while (0)
#define pr_mhitest2(msg) \
	do { } while (0)

#define MHITEST_EMERG(msg, ...) do {\
	} while (0)
#define MHITEST_ERR(msg, ...) \
	do { } while (0)
#define MHITEST_VERB(msg, ...) \
	do { } while (0)
#define MHITEST_LOG(msg, ...) \
	do { } while (0)
#endif

#define VERIFY_ME(val, announce)\
	do {		\
		if (val) {	\
			pr_mhitest2("%s Error val :%d\n", announce, val);\
		}		\
		else {		\
			pr_mhitest2("%s Pass!\n", announce);	\
		}	\
	} while (0)




#define QCA6174_VENDOR_ID               0x168C
#define QCA6174_DEVICE_ID               0x003E

#define QCA6290_VENDOR_ID               0x17CB
#define QCA6290_DEVICE_ID               0x1104

#define QCN90xx_VENDOR_ID               0x17CB
#define QCN90xx_DEVICE_ID               0x1104

#define QCA6390_VENDOR_ID               0x17CB
#define QCA6390_DEVICE_ID               0x1101

#define QCA6490_VENDOR_ID               0x17CB
#define QCA6490_DEVICE_ID               0x1103


#define QCA8074_DEVICE_ID               0xFFFF
#define QCA8074V2_DEVICE_ID             0xFFFE
#define QCA6018_DEVICE_ID               0xFFFD
#define QCA5018_DEVICE_ID               0xFFFC

#define PCI_LINK_UP                     1
#define PCI_LINK_DOWN                   0
/*
 *Structure specific to mhitest module
 */

/*taken this from cnss2 driver , lets see if we could cut down anything ?TODO*/
struct mhitest_msi_user {
	char *name;
	int num_vectors;
	u32 base_vector;
};

enum fw_dump_type {
	FW_IMAGE,
	FW_RDDM,
	FW_REMOTE_HEAP,
};
/* recovery reasons using only default and rddm one for now */
enum mhitest_recovery_reason {
	MHI_DEFAULT,
	MHI_LINK_DOWN,
	MHI_RDDM,
	MHI_TIMEOUT,
};
struct mhitest_msi_config {
	int total_vectors;
	int total_users;
	struct mhitest_msi_user *users;
};
struct mhitest_dump_seg {
	unsigned long address;
	void *v_address;
	unsigned long size;
	u32 type;
};
struct mhitest_dump_data {
	u32 version;
	u32 magic;
	char name[32];
	phys_addr_t paddr;
	int nentries;
	u32 seg_version;
};
struct mhitest_ramdump_info {
	struct ramdump_device *ramdump_dev;
	unsigned long ramdump_size;
	void *dump_data_vaddr;
	u8 dump_data_valid;
	struct mhitest_dump_data dump_data;
};

struct mhitest_platform {
	struct platform_device *plat_dev;
	struct pci_dev *pci_dev;
	unsigned long  device_id;
	const struct pci_device_id  *pci_dev_id;
	u16 def_link_speed;
	u16 def_link_width;
	u8 pci_link_state;
	struct pci_saved_state *pci_dev_default_state;
	void __iomem *bar;
	char fw_name[30];
/*mhi  msi */
	struct mhitest_msi_config *msi_config;
	u32 msi_ep_base_data;
	struct mhi_controller *mhi_ctrl;

/* subsystem related */
	struct subsys_device *mhitest_ss_device;
	struct subsys_desc mhitest_ss_desc;
	void *subsys_handle;
/* ramdump */
	struct mhitest_ramdump_info mhitest_rdinfo;
/* event work queue*/
	struct list_head event_list;
	spinlock_t event_lock; /* spinlock for driver work event handling */
	struct work_struct event_work;
	struct workqueue_struct *event_wq;
/* probed device no. 0 to (MAX-1)*/
	int d_instance;
/* klog level for mhitest driver */
	enum MHITEST_DEBUG_KLVL  mhitest_klog_lvl;

};

enum mhi_state {
	MHI_INIT,
	MHI_DEINIT,
	MHI_POWER_ON,
	MHI_POWER_OFF,
	MHI_FORCE_POWER_OFF,
};
enum mhitest_event_type {
	MHITEST_RECOVERY_EVENT,
	MHITEST_MAX_EVENT,
};
struct mhitest_recovery_data {
	enum mhitest_recovery_reason reason;
};
struct mhitest_driver_event {
	struct list_head list;
	enum mhitest_event_type type;
	bool sync;
	struct completion complete;
	int ret;
	void *data;
};


/* pci/pcie bhi mhi related prototypes
*/
int mhitest_pci_register(void);
int mhitest_subsystem_register(struct mhitest_platform *);
void mhitest_pci_unregister(void);
void mhitest_subsystem_unregister(struct mhitest_platform *);
int mhitest_pci_enable_bus(struct mhitest_platform *);
struct mhitest_platform *get_mhitest_mplat(int);
int mhitest_pci_en_msi(struct mhitest_platform *);
int mhitest_pci_register_mhi(struct mhitest_platform *);
int mhitest_pci_get_mhi_msi(struct mhitest_platform *);
int mhitest_pci_get_link_status(struct mhitest_platform *);
int mhitest_prepare_pci_mhi_msi(struct mhitest_platform *);
int mhitest_prepare_start_mhi(struct mhitest_platform *);
int mhitest_dump_info(struct mhitest_platform *mplat, bool in_panic);
int mhitest_post_event(struct mhitest_platform *,
	struct mhitest_recovery_data *, enum mhitest_event_type, u32 flags);
struct platform_device *get_plat_device(void);
int mhitest_store_mplat(struct mhitest_platform *);
void mhitest_free_mplat(struct mhitest_platform *);
int mhitest_event_work_init(struct mhitest_platform *);
void mhitest_event_work_deinit(struct mhitest_platform *);
int mhitest_pci_start_mhi(struct mhitest_platform *);
void mhitest_global_soc_reset(struct mhitest_platform *);
int mhitest_ss_powerup(const struct subsys_desc *);
int mhitest_pci_set_mhi_state(struct mhitest_platform *, enum mhi_state);
void mhitest_pci_disable_bus(struct mhitest_platform *);
int mhitest_unregister_ramdump(struct mhitest_platform *);
int mhitest_pci_remove_all(struct mhitest_platform *);
