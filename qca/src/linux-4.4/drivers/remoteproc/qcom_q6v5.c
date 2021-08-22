// SPDX-License-Identifier: GPL-2.0
/*
 * Qualcomm Peripheral Image Loader for Q6V5
 *
 * Copyright (C) 2016-2018 Linaro Ltd.
 * Copyright (C) 2014 Sony Mobile Communications AB
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/soc/qcom/smem.h>
#include <linux/soc/qcom/smem_state.h>
#include <linux/remoteproc.h>
#include "qcom_q6v5.h"
#include <linux/delay.h>

#define STOP_ACK_TIMEOUT_MS 5000
/**
 * qcom_q6v5_prepare() - reinitialize the qcom_q6v5 context before start
 * @q6v5:	reference to qcom_q6v5 context to be reinitialized
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_prepare(struct qcom_q6v5 *q6v5)
{
	reinit_completion(&q6v5->start_done);
	reinit_completion(&q6v5->stop_done);

	q6v5->handover_issued = false;

	enable_irq(q6v5->handover_irq);

	return 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_prepare);

/**
 * qcom_q6v5_unprepare() - unprepare the qcom_q6v5 context after stop
 * @q6v5:	reference to qcom_q6v5 context to be unprepared
 *
 * Return: 0 on success, 1 if handover hasn't yet been called
 */
int qcom_q6v5_unprepare(struct qcom_q6v5 *q6v5)
{
	disable_irq(q6v5->handover_irq);

	return !q6v5->handover_issued;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_unprepare);

irqreturn_t q6v5_wdog_interrupt(int irq, void *data)
{
#ifdef CONFIG_CNSS2
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(data);
#else
	struct qcom_q6v5 *q6v5 = data;
#endif
	size_t len;
	char *msg;

	/* Sometimes the stop triggers a watchdog rather than a stop-ack */
	if (!q6v5->running) {
		complete(&q6v5->stop_done);
		return IRQ_HANDLED;
	}

	msg = qcom_smem_get(QCOM_SMEM_HOST_ANY, q6v5->crash_reason, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0])
		dev_err(q6v5->dev, "watchdog received: %s\n", msg);
	else
		dev_err(q6v5->dev, "watchdog without message\n");

#ifdef CONFIG_CNSS2
	subsys_set_crash_status(q6v5->subsys, CRASH_STATUS_WDOG_BITE);
	subsystem_restart_dev(q6v5->subsys);
#else
	rproc_report_crash(q6v5->rproc, RPROC_WATCHDOG);
#endif

	return IRQ_HANDLED;
}

irqreturn_t q6v5_fatal_interrupt(int irq, void *data)
{
#ifdef CONFIG_CNSS2
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(data);
#else
	struct qcom_q6v5 *q6v5 = data;
#endif
	size_t len;
	char *msg;

	msg = qcom_smem_get(QCOM_SMEM_HOST_ANY, q6v5->crash_reason, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0])
		dev_err(q6v5->dev, "fatal error received: %s\n", msg);
	else
		dev_err(q6v5->dev, "fatal error without message\n");

	q6v5->running = false;

#ifdef CONFIG_CNSS2
	subsys_set_crash_status(q6v5->subsys, CRASH_STATUS_ERR_FATAL);
	subsystem_restart_dev(q6v5->subsys);
#else
	rproc_report_crash(q6v5->rproc, RPROC_FATAL_ERROR);
#endif

	return IRQ_HANDLED;
}

irqreturn_t q6v5_ready_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	pr_info("Subsystem error monitoring/handling services are up\n");

	complete(&q6v5->start_done);

	return IRQ_HANDLED;
}

/**
 * qcom_q6v5_wait_for_start() - wait for remote processor start signal
 * @q6v5:	reference to qcom_q6v5 context
 * @timeout:	timeout to wait for the event, in jiffies
 *
 * qcom_q6v5_unprepare() should not be called when this function fails.
 *
 * Return: 0 on success, -ETIMEDOUT on timeout
 */
int qcom_q6v5_wait_for_start(struct qcom_q6v5 *q6v5, int timeout)
{
	int ret;

	ret = wait_for_completion_timeout(&q6v5->start_done, timeout);
	if (!ret)
		disable_irq(q6v5->handover_irq);

	return !ret ? -ETIMEDOUT : 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_wait_for_start);

static irqreturn_t q6v5_handover_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	if (q6v5->handover)
		q6v5->handover(q6v5);

	q6v5->handover_issued = true;

	return IRQ_HANDLED;
}

static irqreturn_t q6v5_spawn_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	complete(&q6v5->spawn_done);

	return IRQ_HANDLED;
}

irqreturn_t q6v5_stop_interrupt(int irq, void *data)
{
#ifdef CONFIG_CNSS2
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(data);
#else
	struct qcom_q6v5 *q6v5 = data;
#endif
	complete(&q6v5->stop_done);

	return IRQ_HANDLED;
}

/**
 * qcom_q6v5_request_stop() - request the remote processor to stop
 * @q6v5:	reference to qcom_q6v5 context
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_request_stop(struct qcom_q6v5 *q6v5)
{
	int ret;

	qcom_smem_state_update_bits(q6v5->stop_state,
				    BIT(q6v5->stop_bit), BIT(q6v5->stop_bit));

	ret = wait_for_completion_timeout(&q6v5->stop_done, 5 * HZ);

	qcom_smem_state_update_bits(q6v5->stop_state, BIT(q6v5->stop_bit), 0);

	return ret == 0 ? -ETIMEDOUT : 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_request_stop);

/**
 * qcom_q6v5_request_spawn() - request the remote processor to spawn
 * @q6v5:      reference to qcom_q6v5 context
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_request_spawn(struct qcom_q6v5 *q6v5)
{
	int ret;

	ret = qcom_smem_state_update_bits(q6v5->spawn_state,
				BIT(q6v5->spawn_bit), BIT(q6v5->spawn_bit));

	ret = wait_for_completion_timeout(&q6v5->spawn_done, 5 * HZ);

	qcom_smem_state_update_bits(q6v5->spawn_state, BIT(q6v5->spawn_bit), 0);

	return ret == 0 ? -ETIMEDOUT : 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_request_spawn);

static int q6v5_get_inbound_irq(struct qcom_q6v5 *q6v5,
			struct platform_device *pdev, const char *int_name,
			irqreturn_t (*handler)(int irq, void *data))
{
	int ret, irq;
	char *interrupt, *tmp = (char *)int_name;
	const char *str = (q6v5->rproc->parent) ? q6v5->rproc->name : "q6v5 ";

	irq = ret = platform_get_irq_byname(pdev, int_name);
	if (ret < 0) {
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev,
				"failed to retrieve %s IRQ: %d\n",
					int_name, ret);
		return ret;
	}

	if (!strcmp(int_name, "wdog"))
		q6v5->wdog_irq = irq;
	else if (!strcmp(int_name, "fatal"))
		q6v5->fatal_irq = irq;
	else if (!strcmp(int_name, "stop-ack")) {
		q6v5->stop_irq = irq;
		tmp = (q6v5->rproc->parent) ? "stop_ack" : "stop";
	} else if (!strcmp(int_name, "ready"))
		q6v5->ready_irq = irq;
	else if (!strcmp(int_name, "handover"))
		q6v5->handover_irq  = irq;
	else if (!strcmp(int_name, "spawn_ack"))
		q6v5->spawn_irq = irq;
	else {
		dev_err(&pdev->dev, "unknown interrupt\n");
		return -EINVAL;
	}

	interrupt = devm_kzalloc(&pdev->dev, BUF_SIZE, GFP_KERNEL);
	if (!interrupt)
		return -ENOMEM;

	strlcpy(interrupt, str, BUF_SIZE);
	if (q6v5->rproc->parent)
		strlcat(interrupt, "_", BUF_SIZE);
	strlcat(interrupt, tmp, BUF_SIZE);

	ret = devm_request_threaded_irq(&pdev->dev, irq,
			NULL, handler,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT,
			interrupt, q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire %s irq\n",
				interrupt);
		return ret;
	}
	return 0;
}

static int q6v5_get_outbound_irq(struct qcom_q6v5 *q6v5,
			struct platform_device *pdev, const char *int_name)
{
	struct qcom_smem_state *tmp_state;
	unsigned  bit;

	tmp_state = qcom_smem_state_get(&pdev->dev, int_name, &bit);
	if (IS_ERR(tmp_state)) {
		dev_err(&pdev->dev, "failed to acquire %s state\n", int_name);
		return PTR_ERR(tmp_state);
	}

	if (!strcmp(int_name, "stop")) {
		q6v5->stop_state = tmp_state;
		q6v5->stop_bit = bit;
	} else if (!strcmp(int_name, "spawn")) {
		q6v5->spawn_state = tmp_state;
		q6v5->spawn_bit = bit;
#ifdef CONFIG_CNSS2
	} else if (!strcmp(int_name, "shutdown")) {
		q6v5->shutdown_state = tmp_state;
		q6v5->shutdown_bit = bit;
#endif
	}

	return 0;
}

/**
 * qcom_q6v5_init() - initializer of the q6v5 common struct
 * @q6v5:	handle to be initialized
 * @pdev:	platform_device reference for acquiring resources
 * @rproc:	associated remoteproc instance
 * @crash_reason: SMEM id for crash reason string, or 0 if none
 * @handover:	function to be called when proxy resources should be released
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_init(struct qcom_q6v5 *q6v5, struct platform_device *pdev,
		   struct rproc *rproc, int crash_reason,
		   void (*handover)(struct qcom_q6v5 *q6v5))
{
	int ret;

	q6v5->rproc = rproc;
	q6v5->dev = &pdev->dev;
	q6v5->crash_reason = crash_reason;
	q6v5->handover = handover;

	init_completion(&q6v5->start_done);
	init_completion(&q6v5->stop_done);
	if (rproc->parent)
		init_completion(&q6v5->spawn_done);

#if !defined(CONFIG_CNSS2)
	if (!rproc->parent) {
		ret = q6v5_get_inbound_irq(q6v5, pdev, "wdog",
					q6v5_wdog_interrupt);
		if (ret)
			return ret;
	}

	ret = q6v5_get_inbound_irq(q6v5, pdev, "fatal",
					q6v5_fatal_interrupt);
	if (ret)
		return ret;
#endif
	ret = q6v5_get_inbound_irq(q6v5, pdev, "ready",
					q6v5_ready_interrupt);
	if (ret)
		return ret;

	if (!rproc->parent) {
		ret = q6v5_get_inbound_irq(q6v5, pdev, "handover",
						q6v5_handover_interrupt);
		if (ret)
			return ret;
		disable_irq(q6v5->handover_irq);
	}
#if !defined(CONFIG_CNSS2)
	ret = q6v5_get_inbound_irq(q6v5, pdev, "stop-ack",
					q6v5_stop_interrupt);
	if (ret)
		return ret;
#endif
	if (rproc->parent) {
		ret = q6v5_get_inbound_irq(q6v5, pdev, "spawn_ack",
					q6v5_spawn_interrupt);
		if (ret)
			return ret;
	}

	ret = q6v5_get_outbound_irq(q6v5, pdev, "stop");
	if (ret)
		return ret;

	if (rproc->parent) {
		ret = q6v5_get_outbound_irq(q6v5, pdev, "spawn");
		if (ret)
			return ret;
	}

#ifdef CONFIG_CNSS2
	ret = q6v5_get_outbound_irq(q6v5, pdev, "shutdown");
	if (ret)
		return ret;
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_init);

#ifdef CONFIG_CNSS2
void q6v5_panic_handler(const struct subsys_desc *subsys)
{
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(subsys);

	if (!subsys_get_crash_status(q6v5->subsys)) {
		qcom_smem_state_update_bits(q6v5->shutdown_state,
			BIT(q6v5->shutdown_bit), BIT(q6v5->shutdown_bit));
		mdelay(STOP_ACK_TIMEOUT_MS);
	}
}
#endif
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Qualcomm Peripheral Image Loader for Q6V5");
