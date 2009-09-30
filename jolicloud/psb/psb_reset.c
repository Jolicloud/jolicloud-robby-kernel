/**************************************************************************
 * Copyright (c) 2007, Intel Corporation.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 * develop this driver.
 *
 **************************************************************************/
/*
 * Authors:
 * Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 */

#include "drmP.h"
#include "psb_drv.h"
#include "psb_reg.h"
#include "psb_scene.h"
#include "psb_msvdx.h"

#define PSB_2D_TIMEOUT_MSEC 100

void psb_reset(struct drm_psb_private *dev_priv, int reset_2d)
{
	uint32_t val;

	val = _PSB_CS_RESET_BIF_RESET |
	    _PSB_CS_RESET_DPM_RESET |
	    _PSB_CS_RESET_TA_RESET |
	    _PSB_CS_RESET_USE_RESET |
	    _PSB_CS_RESET_ISP_RESET | _PSB_CS_RESET_TSP_RESET;

	if (reset_2d)
		val |= _PSB_CS_RESET_TWOD_RESET;

	PSB_WSGX32(val, PSB_CR_SOFT_RESET);
	(void)PSB_RSGX32(PSB_CR_SOFT_RESET);

	msleep(1);

	PSB_WSGX32(0, PSB_CR_SOFT_RESET);
	wmb();
	PSB_WSGX32(PSB_RSGX32(PSB_CR_BIF_CTRL) | _PSB_CB_CTRL_CLEAR_FAULT,
		   PSB_CR_BIF_CTRL);
	wmb();
	(void)PSB_RSGX32(PSB_CR_BIF_CTRL);

	msleep(1);
	PSB_WSGX32(PSB_RSGX32(PSB_CR_BIF_CTRL) & ~_PSB_CB_CTRL_CLEAR_FAULT,
		   PSB_CR_BIF_CTRL);
	(void)PSB_RSGX32(PSB_CR_BIF_CTRL);
}

void psb_print_pagefault(struct drm_psb_private *dev_priv)
{
	uint32_t val;
	uint32_t addr;

	val = PSB_RSGX32(PSB_CR_BIF_INT_STAT);
	addr = PSB_RSGX32(PSB_CR_BIF_FAULT);

	if (val) {
		if (val & _PSB_CBI_STAT_PF_N_RW)
			DRM_ERROR("Poulsbo MMU page fault:\n");
		else
			DRM_ERROR("Poulsbo MMU read / write "
				  "protection fault:\n");

		if (val & _PSB_CBI_STAT_FAULT_CACHE)
			DRM_ERROR("\tCache requestor.\n");
		if (val & _PSB_CBI_STAT_FAULT_TA)
			DRM_ERROR("\tTA requestor.\n");
		if (val & _PSB_CBI_STAT_FAULT_VDM)
			DRM_ERROR("\tVDM requestor.\n");
		if (val & _PSB_CBI_STAT_FAULT_2D)
			DRM_ERROR("\t2D requestor.\n");
		if (val & _PSB_CBI_STAT_FAULT_PBE)
			DRM_ERROR("\tPBE requestor.\n");
		if (val & _PSB_CBI_STAT_FAULT_TSP)
			DRM_ERROR("\tTSP requestor.\n");
		if (val & _PSB_CBI_STAT_FAULT_ISP)
			DRM_ERROR("\tISP requestor.\n");
		if (val & _PSB_CBI_STAT_FAULT_USSEPDS)
			DRM_ERROR("\tUSSEPDS requestor.\n");
		if (val & _PSB_CBI_STAT_FAULT_HOST)
			DRM_ERROR("\tHost requestor.\n");

		DRM_ERROR("\tMMU failing address is 0x%08x.\n", (unsigned)addr);
	}
}

void psb_schedule_watchdog(struct drm_psb_private *dev_priv)
{
	struct timer_list *wt = &dev_priv->watchdog_timer;
	unsigned long irq_flags;

	spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
	if (dev_priv->timer_available && !timer_pending(wt)) {
		wt->expires = jiffies + PSB_WATCHDOG_DELAY;
		add_timer(wt);
	}
	spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);
}

#if 0
static void psb_seq_lockup_idle(struct drm_psb_private *dev_priv,
				unsigned int engine, int *lockup, int *idle)
{
	uint32_t received_seq;

	received_seq = dev_priv->comm[engine << 4];
	spin_lock(&dev_priv->sequence_lock);
	*idle = (received_seq == dev_priv->sequence[engine]);
	spin_unlock(&dev_priv->sequence_lock);

	if (*idle) {
		dev_priv->idle[engine] = 1;
		*lockup = 0;
		return;
	}

	if (dev_priv->idle[engine]) {
		dev_priv->idle[engine] = 0;
		dev_priv->last_sequence[engine] = received_seq;
		*lockup = 0;
		return;
	}

	*lockup = (dev_priv->last_sequence[engine] == received_seq);
}

#endif
static void psb_watchdog_func(unsigned long data)
{
	struct drm_psb_private *dev_priv = (struct drm_psb_private *)data;
	int lockup;
	int msvdx_lockup;
	int msvdx_idle;
	int lockup_2d;
	int idle_2d;
	int idle;
	unsigned long irq_flags;

	psb_scheduler_lockup(dev_priv, &lockup, &idle);
	psb_msvdx_lockup(dev_priv, &msvdx_lockup, &msvdx_idle);
#if 0
	psb_seq_lockup_idle(dev_priv, PSB_ENGINE_2D, &lockup_2d, &idle_2d);
#else
	lockup_2d = FALSE;
	idle_2d = TRUE;
#endif
	if (lockup || msvdx_lockup || lockup_2d) {
		spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
		dev_priv->timer_available = 0;
		spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);
		if (lockup) {
			psb_print_pagefault(dev_priv);
			schedule_work(&dev_priv->watchdog_wq);
		}
		if (msvdx_lockup)
			schedule_work(&dev_priv->msvdx_watchdog_wq);
	}
	if (!idle || !msvdx_idle || !idle_2d)
		psb_schedule_watchdog(dev_priv);
}

void psb_msvdx_flush_cmd_queue(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_msvdx_cmd_queue *msvdx_cmd;
	struct list_head *list, *next;
	/*Flush the msvdx cmd queue and signal all fences in the queue */
	list_for_each_safe(list, next, &dev_priv->msvdx_queue) {
		msvdx_cmd = list_entry(list, struct psb_msvdx_cmd_queue, head);
		PSB_DEBUG_GENERAL("MSVDXQUE: flushing sequence:%d\n",
				  msvdx_cmd->sequence);
		dev_priv->msvdx_current_sequence = msvdx_cmd->sequence;
		psb_fence_error(dev, PSB_ENGINE_VIDEO,
				dev_priv->msvdx_current_sequence,
				DRM_FENCE_TYPE_EXE, DRM_CMD_HANG);
		list_del(list);
		kfree(msvdx_cmd->cmd);
		drm_free(msvdx_cmd, sizeof(struct psb_msvdx_cmd_queue),
			 DRM_MEM_DRIVER);
	}
}

static void psb_msvdx_reset_wq(struct work_struct *work)
{
	struct drm_psb_private *dev_priv =
	    container_of(work, struct drm_psb_private, msvdx_watchdog_wq);

	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;

	mutex_lock(&dev_priv->msvdx_mutex);
	dev_priv->msvdx_needs_reset = 1;
	dev_priv->msvdx_current_sequence++;
	PSB_DEBUG_GENERAL
	    ("MSVDXFENCE: incremented msvdx_current_sequence to :%d\n",
	     dev_priv->msvdx_current_sequence);

	psb_fence_error(scheduler->dev, PSB_ENGINE_VIDEO,
			dev_priv->msvdx_current_sequence, DRM_FENCE_TYPE_EXE,
			DRM_CMD_HANG);

	spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
	dev_priv->timer_available = 1;
	spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);

	spin_lock_irqsave(&dev_priv->msvdx_lock, irq_flags);
	psb_msvdx_flush_cmd_queue(scheduler->dev);
	spin_unlock_irqrestore(&dev_priv->msvdx_lock, irq_flags);

	psb_schedule_watchdog(dev_priv);
	mutex_unlock(&dev_priv->msvdx_mutex);
}

static int psb_xhw_mmu_reset(struct drm_psb_private *dev_priv)
{
	struct psb_xhw_buf buf;
	uint32_t bif_ctrl;

	INIT_LIST_HEAD(&buf.head);
	psb_mmu_set_pd_context(psb_mmu_get_default_pd(dev_priv->mmu), 0);
	bif_ctrl = PSB_RSGX32(PSB_CR_BIF_CTRL);
	PSB_WSGX32(bif_ctrl |
		   _PSB_CB_CTRL_CLEAR_FAULT |
		   _PSB_CB_CTRL_INVALDC, PSB_CR_BIF_CTRL);
	(void)PSB_RSGX32(PSB_CR_BIF_CTRL);
	msleep(1);
	PSB_WSGX32(bif_ctrl, PSB_CR_BIF_CTRL);
	(void)PSB_RSGX32(PSB_CR_BIF_CTRL);
	return psb_xhw_reset_dpm(dev_priv, &buf);
}

/*
 * Block command submission and reset hardware and schedulers.
 */

static void psb_reset_wq(struct work_struct *work)
{
	struct drm_psb_private *dev_priv =
	    container_of(work, struct drm_psb_private, watchdog_wq);
	int lockup_2d;
	int idle_2d;
	unsigned long irq_flags;
	int ret;
	int reset_count = 0;
	struct psb_xhw_buf buf;
	uint32_t xhw_lockup;

	/*
	 * Block command submission.
	 */

	mutex_lock(&dev_priv->reset_mutex);

	INIT_LIST_HEAD(&buf.head);
	if (psb_xhw_check_lockup(dev_priv, &buf, &xhw_lockup) == 0) {
		if (xhw_lockup == 0 && psb_extend_raster_timeout(dev_priv) == 0) {
			/*
			 * no lockup, just re-schedule
			 */
			spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
			dev_priv->timer_available = 1;
			spin_unlock_irqrestore(&dev_priv->watchdog_lock,
					       irq_flags);
			psb_schedule_watchdog(dev_priv);
			mutex_unlock(&dev_priv->reset_mutex);
			return;
		}
	}
#if 0
	msleep(PSB_2D_TIMEOUT_MSEC);

	psb_seq_lockup_idle(dev_priv, PSB_ENGINE_2D, &lockup_2d, &idle_2d);

	if (lockup_2d) {
		uint32_t seq_2d;
		spin_lock(&dev_priv->sequence_lock);
		seq_2d = dev_priv->sequence[PSB_ENGINE_2D];
		spin_unlock(&dev_priv->sequence_lock);
		psb_fence_error(dev_priv->scheduler.dev,
				PSB_ENGINE_2D,
				seq_2d, DRM_FENCE_TYPE_EXE, -EBUSY);
		DRM_INFO("Resetting 2D engine.\n");
	}

	psb_reset(dev_priv, lockup_2d);
#else
	(void)lockup_2d;
	(void)idle_2d;
	psb_reset(dev_priv, 0);
#endif
	(void)psb_xhw_mmu_reset(dev_priv);
	DRM_INFO("Resetting scheduler.\n");
	psb_scheduler_pause(dev_priv);
	psb_scheduler_reset(dev_priv, -EBUSY);
	psb_scheduler_ta_mem_check(dev_priv);

	while (dev_priv->ta_mem &&
	       !dev_priv->force_ta_mem_load && ++reset_count < 10) {

		/*
		 * TA memory is currently fenced so offsets
		 * are valid. Reload offsets into the dpm now.
		 */

		struct psb_xhw_buf buf;
		INIT_LIST_HEAD(&buf.head);

		msleep(100);
		DRM_INFO("Trying to reload TA memory.\n");
		ret = psb_xhw_ta_mem_load(dev_priv, &buf,
					  PSB_TA_MEM_FLAG_TA |
					  PSB_TA_MEM_FLAG_RASTER |
					  PSB_TA_MEM_FLAG_HOSTA |
					  PSB_TA_MEM_FLAG_HOSTD |
					  PSB_TA_MEM_FLAG_INIT,
					  dev_priv->ta_mem->ta_memory->offset,
					  dev_priv->ta_mem->hw_data->offset,
					  dev_priv->ta_mem->hw_cookie);
		if (!ret)
			break;

		psb_reset(dev_priv, 0);
		(void)psb_xhw_mmu_reset(dev_priv);
	}

	psb_scheduler_restart(dev_priv);
	spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
	dev_priv->timer_available = 1;
	spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);
	mutex_unlock(&dev_priv->reset_mutex);
}

void psb_watchdog_init(struct drm_psb_private *dev_priv)
{
	struct timer_list *wt = &dev_priv->watchdog_timer;
	unsigned long irq_flags;

	dev_priv->watchdog_lock = SPIN_LOCK_UNLOCKED;
	spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
	init_timer(wt);
	INIT_WORK(&dev_priv->watchdog_wq, &psb_reset_wq);
	INIT_WORK(&dev_priv->msvdx_watchdog_wq, &psb_msvdx_reset_wq);
	wt->data = (unsigned long)dev_priv;
	wt->function = &psb_watchdog_func;
	dev_priv->timer_available = 1;
	spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);
}

void psb_watchdog_takedown(struct drm_psb_private *dev_priv)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
	dev_priv->timer_available = 0;
	spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);
	(void)del_timer_sync(&dev_priv->watchdog_timer);
}
