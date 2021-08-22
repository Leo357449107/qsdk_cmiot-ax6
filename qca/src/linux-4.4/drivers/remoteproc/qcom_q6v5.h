/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __QCOM_Q6V5_H__
#define __QCOM_Q6V5_H__

#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/irqreturn.h>
#include <soc/qcom/subsystem_restart.h>
#include "qcom_common.h"

#define EM_QDSP6 164
#define BUF_SIZE 35

struct rproc;
struct qcom_smem_state;

struct qcom_q6v5 {
	struct device *dev;
	struct rproc *rproc;

	struct qcom_smem_state *stop_state;
	unsigned stop_bit;
	struct qcom_smem_state *spawn_state;
	unsigned spawn_bit;
#ifdef CONFIG_CNSS2
	struct qcom_smem_state *shutdown_state;
	unsigned shutdown_bit;
#endif
	int wdog_irq;
	int fatal_irq;
	int ready_irq;
	int handover_irq;
	int stop_irq;
	int spawn_irq;

#ifdef CONFIG_CNSS2
	struct subsys_device *subsys;
	struct subsys_desc subsys_desc;
#endif
	bool handover_issued;

	struct completion start_done;
	struct completion stop_done;
	struct completion spawn_done;
	struct work_struct crash_handler;

	int crash_reason;

	bool running;

	void (*handover)(struct qcom_q6v5 *q6v5);
};

#define subsys_to_pdata(d) container_of(d, struct qcom_q6v5, subsys_desc)

int qcom_q6v5_init(struct qcom_q6v5 *q6v5, struct platform_device *pdev,
		   struct rproc *rproc, int crash_reason,
		   void (*handover)(struct qcom_q6v5 *q6v5));

int qcom_q6v5_prepare(struct qcom_q6v5 *q6v5);
int qcom_q6v5_unprepare(struct qcom_q6v5 *q6v5);
int qcom_q6v5_request_stop(struct qcom_q6v5 *q6v5);
int qcom_q6v5_request_spawn(struct qcom_q6v5 *q6v5);
int qcom_q6v5_wait_for_start(struct qcom_q6v5 *q6v5, int timeout);

irqreturn_t q6v5_wdog_interrupt(int irq, void *data);
irqreturn_t q6v5_fatal_interrupt(int irq, void *data);
irqreturn_t q6v5_ready_interrupt(int irq, void *data);
irqreturn_t q6v5_stop_interrupt(int irq, void *data);
#ifdef CONFIG_CNSS2
void q6v5_panic_handler(const struct subsys_desc *subsys);
#endif

#endif
