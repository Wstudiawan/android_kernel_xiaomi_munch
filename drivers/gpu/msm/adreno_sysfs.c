// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014-2019, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/sysfs.h>

#include "adreno.h"

struct adreno_sysfs_attribute {
	struct device_attribute attr;
	unsigned int (*show)(struct adreno_device *adreno_dev);
	int (*store)(struct adreno_device *adreno_dev, unsigned int val);
};

#define _ADRENO_SYSFS_ATTR(_name, __show, __store) \
struct adreno_sysfs_attribute adreno_attr_##_name = { \
	.attr = __ATTR(_name, 0644, __show, __store), \
	.show = _ ## _name ## _show, \
	.store = _ ## _name ## _store, \
}

#define _ADRENO_SYSFS_FPERM_ATTR(_name, _fperm, __show, __store) \
struct adreno_sysfs_attribute adreno_attr_##_name = { \
	.attr = __ATTR(_name, _fperm, __show, __store), \
	.show = _ ## _name ## _show, \
	.store = _ ## _name ## _store, \
}

#define _ADRENO_SYSFS_ATTR_RO(_name, __show) \
struct adreno_sysfs_attribute adreno_attr_##_name = { \
	.attr = __ATTR(_name, 0444, __show, NULL), \
	.show = _ ## _name ## _show, \
	.store = NULL, \
}

#define ADRENO_SYSFS_ATTR(_a) \
	container_of((_a), struct adreno_sysfs_attribute, attr)

static int _ft_policy_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	adreno_dev->ft_policy = val & KGSL_FT_POLICY_MASK;
	return 0;
}

static unsigned int _ft_policy_show(struct adreno_device *adreno_dev)
{
	return adreno_dev->ft_policy;
}

static int _preempt_level_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	struct adreno_preemption *preempt = &adreno_dev->preempt;

	if (val <= 2)
		preempt->preempt_level = val;
	return 0;
}

static unsigned int _preempt_level_show(struct adreno_device *adreno_dev)
{
	struct adreno_preemption *preempt = &adreno_dev->preempt;

	return preempt->preempt_level;
}

static int _usesgmem_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	struct adreno_preemption *preempt = &adreno_dev->preempt;

	preempt->usesgmem = val ? 1 : 0;
	return 0;
}

static unsigned int _usesgmem_show(struct adreno_device *adreno_dev)
{
	struct adreno_preemption *preempt = &adreno_dev->preempt;

	return preempt->usesgmem;
}

static int _skipsaverestore_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	struct adreno_preemption *preempt = &adreno_dev->preempt;

	preempt->skipsaverestore = val ? 1 : 0;
	return 0;
}

static unsigned int _skipsaverestore_show(struct adreno_device *adreno_dev)
{
	struct adreno_preemption *preempt = &adreno_dev->preempt;

	return preempt->skipsaverestore;
}

static int _ft_pagefault_policy_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	int ret = 0;

	mutex_lock(&device->mutex);
	val &= KGSL_FT_PAGEFAULT_MASK;

	if (test_bit(ADRENO_DEVICE_STARTED, &adreno_dev->priv))
		ret = kgsl_mmu_set_pagefault_policy(&device->mmu,
			(unsigned long) val);

	if (ret == 0)
		adreno_dev->ft_pf_policy = val;

	mutex_unlock(&device->mutex);

	return 0;
}

static unsigned int _ft_pagefault_policy_show(struct adreno_device *adreno_dev)
{
	return adreno_dev->ft_pf_policy;
}

static int _gpu_llc_slice_enable_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	adreno_dev->gpu_llc_slice_enable = val ? true : false;
	return 0;
}

static unsigned int _gpu_llc_slice_enable_show(struct adreno_device *adreno_dev)
{
	return adreno_dev->gpu_llc_slice_enable;
}

static int _gpuhtw_llc_slice_enable_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	adreno_dev->gpuhtw_llc_slice_enable = val ? true : false;
	return 0;
}

static unsigned int
_gpuhtw_llc_slice_enable_show(struct adreno_device *adreno_dev)
{
	return adreno_dev->gpuhtw_llc_slice_enable;
}

static int _ft_long_ib_detect_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	adreno_dev->long_ib_detect = val;
	return 0;
}

static unsigned int _ft_long_ib_detect_show(struct adreno_device *adreno_dev)
{
	return adreno_dev->long_ib_detect;
}

static int _ft_hang_intr_status_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	int ret = 0;

	if (val == test_bit(ADRENO_DEVICE_HANG_INTR, &adreno_dev->priv))
		return 0;

	mutex_lock(&device->mutex);
	change_bit(ADRENO_DEVICE_HANG_INTR, &adreno_dev->priv);

	if (test_bit(ADRENO_DEVICE_STARTED, &adreno_dev->priv)) {
		kgsl_pwrctrl_change_state(device, KGSL_STATE_ACTIVE);
	} else if (device->state == KGSL_STATE_INIT) {
		ret = -EACCES;
		change_bit(ADRENO_DEVICE_HANG_INTR, &adreno_dev->priv);
	}

	mutex_unlock(&device->mutex);
	return ret;
}

static unsigned int _ft_hang_intr_status_show(struct adreno_device *adreno_dev)
{
	return test_bit(ADRENO_DEVICE_HANG_INTR, &adreno_dev->priv);
}

static int _pwrctrl_store(struct adreno_device *adreno_dev,
		unsigned int val, unsigned int flag)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);

	if (val == test_bit(flag, &adreno_dev->pwrctrl_flag))
		return 0;

	mutex_lock(&device->mutex);

	/* Power down the GPU before changing the state */
	kgsl_pwrctrl_change_state(device, KGSL_STATE_SUSPEND);
	change_bit(flag, &adreno_dev->pwrctrl_flag);
	kgsl_pwrctrl_change_state(device, KGSL_STATE_SLUMBER);

	mutex_unlock(&device->mutex);

	return 0;
}

static int _preemption_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	struct kgsl_context *context;
	struct adreno_context *drawctxt;
	int id;

	mutex_lock(&device->mutex);

	if (!(ADRENO_FEATURE(adreno_dev, ADRENO_PREEMPTION)) ||
		(test_bit(ADRENO_DEVICE_PREEMPTION,
		&adreno_dev->priv) == val)) {
		mutex_unlock(&device->mutex);
		return 0;
	}

	kgsl_pwrctrl_change_state(device, KGSL_STATE_SUSPEND);
	change_bit(ADRENO_DEVICE_PREEMPTION, &adreno_dev->priv);
	adreno_dev->cur_rb = &(adreno_dev->ringbuffers[0]);

	/* Update the ringbuffer for each draw context */
	write_lock(&device->context_lock);
	idr_for_each_entry(&device->context_idr, context, id) {
		drawctxt = ADRENO_CONTEXT(context);
		drawctxt->rb = adreno_ctx_get_rb(adreno_dev, drawctxt);
	}
	write_unlock(&device->context_lock);

	kgsl_pwrctrl_change_state(device, KGSL_STATE_SLUMBER);

	mutex_unlock(&device->mutex);

	return 0;
}

static unsigned int _preemption_show(struct adreno_device *adreno_dev)
{
	return adreno_is_preemption_enabled(adreno_dev);
}

static int _hwcg_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	return _pwrctrl_store(adreno_dev, val, ADRENO_HWCG_CTRL);
}

static unsigned int _hwcg_show(struct adreno_device *adreno_dev)
{
	return test_bit(ADRENO_HWCG_CTRL, &adreno_dev->pwrctrl_flag);
}

static int _throttling_store(struct adreno_device *adreno_dev,
	unsigned int val)
{
	return 0;
}

static unsigned int _throttling_show(struct adreno_device *adreno_dev)
{
	return test_bit(ADRENO_THROTTLING_CTRL, &adreno_dev->pwrctrl_flag);
}

static int _sptp_pc_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	return _pwrctrl_store(adreno_dev, val, ADRENO_SPTP_PC_CTRL);
}

static unsigned int _sptp_pc_show(struct adreno_device *adreno_dev)
{
	return test_bit(ADRENO_SPTP_PC_CTRL, &adreno_dev->pwrctrl_flag);
}

static int _lm_store(struct adreno_device *adreno_dev, unsigned int val)
{
	return _pwrctrl_store(adreno_dev, val, ADRENO_LM_CTRL);
}

static unsigned int _lm_show(struct adreno_device *adreno_dev)
{
	return test_bit(ADRENO_LM_CTRL, &adreno_dev->pwrctrl_flag);
}

static int _ifpc_store(struct adreno_device *adreno_dev, unsigned int val)
{
	return gmu_core_dev_ifpc_store(KGSL_DEVICE(adreno_dev), val);
}

static unsigned int _ifpc_show(struct adreno_device *adreno_dev)
{
	return gmu_core_dev_ifpc_show(KGSL_DEVICE(adreno_dev));
}

static unsigned int _ifpc_count_show(struct adreno_device *adreno_dev)
{
	return adreno_dev->ifpc_count;
}

static unsigned int _preempt_count_show(struct adreno_device *adreno_dev)
{
	struct adreno_preemption *preempt = &adreno_dev->preempt;

	return preempt->count;
}

static unsigned int _acd_show(struct adreno_device *adreno_dev)
{
	return test_bit(ADRENO_ACD_CTRL, &adreno_dev->pwrctrl_flag);
}

static int _acd_store(struct adreno_device *adreno_dev, unsigned int val)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);

	if (test_bit(ADRENO_ACD_CTRL, &adreno_dev->pwrctrl_flag) == val)
		return 0;

	return gmu_core_acd_set(device, val);
}

static unsigned int _perfcounter_show(struct adreno_device *adreno_dev)
{
	return adreno_dev->perfcounter;
}

static int _perfcounter_store(struct adreno_device *adreno_dev,
		unsigned int val)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);

	if (adreno_dev->perfcounter == val)
		return 0;

	mutex_lock(&device->mutex);

	/* Power down the GPU before changing the state */
	kgsl_pwrctrl_change_state(device, KGSL_STATE_SUSPEND);
	adreno_dev->perfcounter = val;
	kgsl_pwrctrl_change_state(device, KGSL_STATE_SLUMBER);

	mutex_unlock(&device->mutex);

	return 0;
}

static ssize_t _sysfs_store_u32(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(dev_get_drvdata(dev));
	struct adreno_sysfs_attribute *_attr = ADRENO_SYSFS_ATTR(attr);
	unsigned int val = 0;
	int ret;

	ret = kgsl_sysfs_store(buf, &val);

	if (!ret && _attr->store)
		ret = _attr->store(adreno_dev, val);

	return (ssize_t) ret < 0 ? ret : count;
}

static ssize_t _sysfs_show_u32(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(dev_get_drvdata(dev));
	struct adreno_sysfs_attribute *_attr = ADRENO_SYSFS_ATTR(attr);
	unsigned int val = 0;

	if (_attr->show)
		val = _attr->show(adreno_dev);

	return scnprintf(buf, PAGE_SIZE, "0x%X\n", val);
}

static ssize_t _sysfs_store_bool(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(dev_get_drvdata(dev));
	struct adreno_sysfs_attribute *_attr = ADRENO_SYSFS_ATTR(attr);
	unsigned int val = 0;
	int ret;

	ret = kgsl_sysfs_store(buf, &val);

	if (!ret && _attr->store)
		ret = _attr->store(adreno_dev, val ? 1 : 0);

	return (ssize_t) ret < 0 ? ret : count;
}

static ssize_t _sysfs_show_bool(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(dev_get_drvdata(dev));
	struct adreno_sysfs_attribute *_attr = ADRENO_SYSFS_ATTR(attr);
	unsigned int val = 0;

	if (_attr->show)
		val = _attr->show(adreno_dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

#define ADRENO_SYSFS_BOOL(_name) \
	_ADRENO_SYSFS_ATTR(_name, _sysfs_show_bool, _sysfs_store_bool)

#define ADRENO_SYSFS_RO_BOOL(_name) \
	_ADRENO_SYSFS_FPERM_ATTR(_name, 0444, _sysfs_show_bool, _sysfs_store_bool)

#define ADRENO_SYSFS_U32(_name) \
	_ADRENO_SYSFS_ATTR(_name, _sysfs_show_u32, _sysfs_store_u32)

#define ADRENO_SYSFS_RO_U32(_name) \
	_ADRENO_SYSFS_ATTR_RO(_name, _sysfs_show_u32)

static ADRENO_SYSFS_U32(ft_policy);
static ADRENO_SYSFS_U32(ft_pagefault_policy);
static ADRENO_SYSFS_U32(preempt_level);
static ADRENO_SYSFS_RO_U32(preempt_count);
static ADRENO_SYSFS_BOOL(usesgmem);
static ADRENO_SYSFS_BOOL(skipsaverestore);
static ADRENO_SYSFS_BOOL(ft_long_ib_detect);
static ADRENO_SYSFS_BOOL(ft_hang_intr_status);
static ADRENO_SYSFS_BOOL(gpu_llc_slice_enable);
static ADRENO_SYSFS_BOOL(gpuhtw_llc_slice_enable);

static DEVICE_INT_ATTR(wake_nice, 0644, adreno_wake_nice);
static DEVICE_INT_ATTR(wake_timeout, 0644, adreno_wake_timeout);

static ADRENO_SYSFS_BOOL(sptp_pc);
static ADRENO_SYSFS_BOOL(lm);
static ADRENO_SYSFS_BOOL(preemption);
static ADRENO_SYSFS_BOOL(hwcg);
static ADRENO_SYSFS_RO_BOOL(throttling);
static ADRENO_SYSFS_BOOL(ifpc);
static ADRENO_SYSFS_RO_U32(ifpc_count);
static ADRENO_SYSFS_BOOL(acd);
static ADRENO_SYSFS_BOOL(perfcounter);


static const struct attribute *_attr_list[] = {
	&adreno_attr_ft_policy.attr.attr,
	&adreno_attr_ft_pagefault_policy.attr.attr,
	&adreno_attr_ft_long_ib_detect.attr.attr,
	&adreno_attr_ft_hang_intr_status.attr.attr,
	&dev_attr_wake_nice.attr.attr,
	&dev_attr_wake_timeout.attr.attr,
	&adreno_attr_sptp_pc.attr.attr,
	&adreno_attr_lm.attr.attr,
	&adreno_attr_preemption.attr.attr,
	&adreno_attr_hwcg.attr.attr,
	&adreno_attr_throttling.attr.attr,
	&adreno_attr_gpu_llc_slice_enable.attr.attr,
	&adreno_attr_gpuhtw_llc_slice_enable.attr.attr,
	&adreno_attr_preempt_level.attr.attr,
	&adreno_attr_usesgmem.attr.attr,
	&adreno_attr_skipsaverestore.attr.attr,
	&adreno_attr_ifpc.attr.attr,
	&adreno_attr_ifpc_count.attr.attr,
	&adreno_attr_preempt_count.attr.attr,
	&adreno_attr_acd.attr.attr,
	&adreno_attr_perfcounter.attr.attr,
	NULL,
};

/**
 * adreno_sysfs_close() - Take down the adreno sysfs files
 * @adreno_dev: Pointer to the adreno device
 *
 * Take down the sysfs files on when the device goes away
 */
void adreno_sysfs_close(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);

	sysfs_remove_files(&device->dev->kobj, _attr_list);
}

/**
 * adreno_sysfs_init() - Initialize adreno sysfs files
 * @adreno_dev: Pointer to the adreno device
 *
 * Initialize many of the adreno specific sysfs files especially for fault
 * tolerance and power control
 */
int adreno_sysfs_init(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = KGSL_DEVICE(adreno_dev);
	int ret;

	ret = sysfs_create_files(&device->dev->kobj, _attr_list);

	/* Notify userspace */
	if (!ret)
		kobject_uevent(&device->dev->kobj, KOBJ_ADD);

	return ret;
}

