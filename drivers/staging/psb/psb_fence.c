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
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 */

#include <drm/drmP.h>
#include "psb_drv.h"

static void psb_print_ta_fence_status(struct ttm_fence_device *fdev)
{
	struct drm_psb_private *dev_priv =
		container_of(fdev, struct drm_psb_private, fdev);
	struct psb_scheduler_seq *seq = dev_priv->scheduler.seq;
	int i;

	for (i=0; i < _PSB_ENGINE_TA_FENCE_TYPES; ++i) {
		DRM_INFO("Type 0x%02x, sequence %lu, reported %d\n",
			 (1 << i),
			 (unsigned long) seq->sequence,
			 seq->reported);
		seq++;
	}
}

static void psb_poll_ta(struct ttm_fence_device *fdev,
			uint32_t waiting_types)
{
	struct drm_psb_private *dev_priv =
	    container_of(fdev, struct drm_psb_private, fdev);
	uint32_t cur_flag = 1;
	uint32_t flags = 0;
	uint32_t sequence = 0;
	uint32_t remaining = 0xFFFFFFFF;
	uint32_t diff;

	struct psb_scheduler *scheduler;
	struct psb_scheduler_seq *seq;
	struct ttm_fence_class_manager *fc =
	    &fdev->fence_class[PSB_ENGINE_TA];

	scheduler = &dev_priv->scheduler;
	seq = scheduler->seq;

	while (likely(waiting_types & remaining)) {
		if (!(waiting_types & cur_flag))
			goto skip;
		if (seq->reported)
			goto skip;
		if (flags == 0)
			sequence = seq->sequence;
		else if (sequence != seq->sequence) {
			ttm_fence_handler(fdev, PSB_ENGINE_TA,
					  sequence, flags, 0);
			sequence = seq->sequence;
			flags = 0;
		}
		flags |= cur_flag;

		/*
		 * Sequence may not have ended up on the ring yet.
		 * In that case, report it but don't mark it as
		 * reported. A subsequent poll will report it again.
		 */

		diff = (fc->latest_queued_sequence - sequence) &
		    fc->sequence_mask;
		if (diff < fc->wrap_diff)
			seq->reported = 1;

skip:
		cur_flag <<= 1;
		remaining <<= 1;
		seq++;
	}

	if (flags)
		ttm_fence_handler(fdev, PSB_ENGINE_TA, sequence, flags, 0);

}

static void psb_poll_other(struct ttm_fence_device *fdev,
			   uint32_t fence_class, uint32_t waiting_types)
{
	struct drm_psb_private *dev_priv =
	    container_of(fdev, struct drm_psb_private, fdev);
	struct ttm_fence_class_manager *fc =
	    &fdev->fence_class[fence_class];
	uint32_t sequence;

	if (unlikely(!dev_priv))
		return;

	if (waiting_types) {
                switch (fence_class) {
                case PSB_ENGINE_VIDEO:
                        sequence = dev_priv->msvdx_current_sequence;
                        break;
                case LNC_ENGINE_ENCODE:
                        sequence = dev_priv->topaz_current_sequence;
                        break;
                default:
                        sequence = dev_priv->comm[fence_class << 4];
                        break;
                }

		ttm_fence_handler(fdev, fence_class, sequence,
				  _PSB_FENCE_TYPE_EXE, 0);

		switch (fence_class) {
		case PSB_ENGINE_2D:
			if (dev_priv->fence0_irq_on && !fc->waiting_types) {
				psb_2D_irq_off(dev_priv);
				dev_priv->fence0_irq_on = 0;
			} else if (!dev_priv->fence0_irq_on
				   && fc->waiting_types) {
				psb_2D_irq_on(dev_priv);
				dev_priv->fence0_irq_on = 1;
			}
			break;
#if 0
			/*
			 * FIXME: MSVDX irq switching
			 */

		case PSB_ENGINE_VIDEO:
			if (dev_priv->fence2_irq_on && !fc->waiting_types) {
				psb_msvdx_irq_off(dev_priv);
				dev_priv->fence2_irq_on = 0;
			} else if (!dev_priv->fence2_irq_on
				   && fc->pending_exe_flush) {
				psb_msvdx_irq_on(dev_priv);
				dev_priv->fence2_irq_on = 1;
			}
			break;
#endif
		default:
			return;
		}
	}
}

static void psb_fence_poll(struct ttm_fence_device *fdev,
			   uint32_t fence_class, uint32_t waiting_types)
{
	if (unlikely((PSB_D_PM & drm_psb_debug) && (fence_class == 0)))
		PSB_DEBUG_PM("psb_fence_poll: %d\n", fence_class);
	switch (fence_class) {
	case PSB_ENGINE_TA:
		psb_poll_ta(fdev, waiting_types);
		break;
	default:
		psb_poll_other(fdev, fence_class, waiting_types);
		break;
	}
}

void psb_fence_error(struct drm_device *dev,
		     uint32_t fence_class,
		     uint32_t sequence, uint32_t type, int error)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_fence_device *fdev = &dev_priv->fdev;
	unsigned long irq_flags;
	struct ttm_fence_class_manager *fc =
	    &fdev->fence_class[fence_class];

	BUG_ON(fence_class >= PSB_NUM_ENGINES);
	write_lock_irqsave(&fc->lock, irq_flags);
	ttm_fence_handler(fdev, fence_class, sequence, type, error);
	write_unlock_irqrestore(&fc->lock, irq_flags);
}

int psb_fence_emit_sequence(struct ttm_fence_device *fdev,
			    uint32_t fence_class,
			    uint32_t flags, uint32_t *sequence,
			    unsigned long *timeout_jiffies)
{
	struct drm_psb_private *dev_priv =
	    container_of(fdev, struct drm_psb_private, fdev);
	uint32_t seq = 0;
	int ret;

	if (!dev_priv)
		return -EINVAL;

	if (fence_class >= PSB_NUM_ENGINES)
		return -EINVAL;

	switch (fence_class) {
	case PSB_ENGINE_2D:
		spin_lock(&dev_priv->sequence_lock);
		seq = ++dev_priv->sequence[fence_class];
		spin_unlock(&dev_priv->sequence_lock);
		ret = psb_blit_sequence(dev_priv, seq);
		if (ret)
			return ret;
		break;
	case PSB_ENGINE_VIDEO:
		spin_lock(&dev_priv->sequence_lock);
		seq = dev_priv->sequence[fence_class]++;
		spin_unlock(&dev_priv->sequence_lock);
		break;
	case LNC_ENGINE_ENCODE:
		spin_lock(&dev_priv->sequence_lock);
		seq = dev_priv->sequence[fence_class]++;
		spin_unlock(&dev_priv->sequence_lock);
		break;
	default:
		spin_lock(&dev_priv->sequence_lock);
		seq = dev_priv->sequence[fence_class];
		spin_unlock(&dev_priv->sequence_lock);
	}

	*sequence = seq;

	if (fence_class == PSB_ENGINE_TA)
		*timeout_jiffies = jiffies + DRM_HZ / 2;
	else
		*timeout_jiffies = jiffies + DRM_HZ * 3;

	return 0;
}

uint32_t psb_fence_advance_sequence(struct drm_device *dev,
				    uint32_t fence_class)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	uint32_t sequence;

	spin_lock(&dev_priv->sequence_lock);
	sequence = ++dev_priv->sequence[fence_class];
	spin_unlock(&dev_priv->sequence_lock);

	return sequence;
}

static void psb_fence_lockup(struct ttm_fence_object *fence,
			     uint32_t fence_types)
{
	struct ttm_fence_class_manager *fc = ttm_fence_fc(fence);

	if (fence->fence_class == PSB_ENGINE_TA) {

		/*
		 * The 3D engine has its own lockup detection.
		 * Just extend the fence expiry time.
		 */

		DRM_INFO("Extending 3D fence timeout.\n");
		write_lock(&fc->lock);

		DRM_INFO("Sequence %lu, types 0x%08x signaled 0x%08x\n",
			 (unsigned long) fence->sequence, fence_types,
			 fence->info.signaled_types);

		if (time_after_eq(jiffies, fence->timeout_jiffies))
			fence->timeout_jiffies = jiffies + DRM_HZ / 2;

		psb_print_ta_fence_status(fence->fdev);
		write_unlock(&fc->lock);
	} else {
		DRM_ERROR
		    ("GPU timeout (probable lockup) detected on engine %u "
		     "fence type 0x%08x\n",
		     (unsigned int) fence->fence_class,
		     (unsigned int) fence_types);
		write_lock(&fc->lock);
		ttm_fence_handler(fence->fdev, fence->fence_class,
				  fence->sequence, fence_types, -EBUSY);
		write_unlock(&fc->lock);
	}
}

void psb_fence_handler(struct drm_device *dev, uint32_t fence_class)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_fence_device *fdev = &dev_priv->fdev;
	struct ttm_fence_class_manager *fc =
	    &fdev->fence_class[fence_class];
	unsigned long irq_flags;

#ifdef FIX_TG_16
	if (fence_class == PSB_ENGINE_2D) {

		if ((atomic_read(&dev_priv->ta_wait_2d_irq) == 1) &&
		    (PSB_RSGX32(PSB_CR_2D_SOCIF) == _PSB_C2_SOCIF_EMPTY) &&
		    ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) &
		      _PSB_C2B_STATUS_BUSY) == 0))
			psb_resume_ta_2d_idle(dev_priv);
	}
#endif
	write_lock_irqsave(&fc->lock, irq_flags);
	psb_fence_poll(fdev, fence_class, fc->waiting_types);
	write_unlock_irqrestore(&fc->lock, irq_flags);
}


static struct ttm_fence_driver psb_ttm_fence_driver = {
	.has_irq = NULL,
	.emit = psb_fence_emit_sequence,
	.flush = NULL,
	.poll = psb_fence_poll,
	.needed_flush = NULL,
	.wait = NULL,
	.signaled = NULL,
	.lockup = psb_fence_lockup,
};

int psb_ttm_fence_device_init(struct ttm_fence_device *fdev)
{
	struct drm_psb_private *dev_priv =
		container_of(fdev, struct drm_psb_private, fdev);
	struct ttm_fence_class_init fci = {.wrap_diff = (1 << 30),
		.flush_diff = (1 << 29),
		.sequence_mask = 0xFFFFFFFF
	};

	return ttm_fence_device_init(PSB_NUM_ENGINES,
				     dev_priv->mem_global_ref.object,
				     fdev, &fci, 1,
				     &psb_ttm_fence_driver);
}
