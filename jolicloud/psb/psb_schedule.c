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
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics.com>
 */

#include "drmP.h"
#include "psb_drm.h"
#include "psb_drv.h"
#include "psb_reg.h"
#include "psb_scene.h"

#define PSB_ALLOWED_RASTER_RUNTIME (DRM_HZ * 20)
#define PSB_RASTER_TIMEOUT (DRM_HZ / 2)
#define PSB_TA_TIMEOUT (DRM_HZ / 5)

#undef PSB_SOFTWARE_WORKAHEAD

#ifdef PSB_STABLE_SETTING

/*
 * Software blocks completely while the engines are working so there can be no
 * overlap.
 */

#define PSB_WAIT_FOR_RASTER_COMPLETION
#define PSB_WAIT_FOR_TA_COMPLETION

#elif defined(PSB_PARANOID_SETTING)
/*
 * Software blocks "almost" while the engines are working so there can be no
 * overlap.
 */

#define PSB_WAIT_FOR_RASTER_COMPLETION
#define PSB_WAIT_FOR_TA_COMPLETION
#define PSB_BE_PARANOID

#elif defined(PSB_SOME_OVERLAP_BUT_LOCKUP)
/*
 * Software leaps ahead while the rasterizer is running and prepares
 * a new ta job that can be scheduled before the rasterizer has
 * finished.
 */

#define PSB_WAIT_FOR_TA_COMPLETION

#elif defined(PSB_SOFTWARE_WORKAHEAD)
/*
 * Don't sync, but allow software to work ahead. and queue a number of jobs.
 * But block overlapping in the scheduler.
 */

#define PSB_BLOCK_OVERLAP
#define ONLY_ONE_JOB_IN_RASTER_QUEUE

#endif

/*
 * Avoid pixelbe pagefaults on C0.
 */
#if 0
#define PSB_BLOCK_OVERLAP
#endif

static void psb_dispatch_ta(struct drm_psb_private *dev_priv,
			    struct psb_scheduler *scheduler,
			    uint32_t reply_flag);
static void psb_dispatch_raster(struct drm_psb_private *dev_priv,
				struct psb_scheduler *scheduler,
				uint32_t reply_flag);

#ifdef FIX_TG_16

static void psb_2d_atomic_unlock(struct drm_psb_private *dev_priv);
static int psb_2d_trylock(struct drm_psb_private *dev_priv);
static int psb_check_2d_idle(struct drm_psb_private *dev_priv);

#endif

void psb_scheduler_lockup(struct drm_psb_private *dev_priv,
			  int *lockup, int *idle)
{
	unsigned long irq_flags;
	struct psb_scheduler *scheduler = &dev_priv->scheduler;

	*lockup = 0;
	*idle = 1;

	spin_lock_irqsave(&scheduler->lock, irq_flags);

	if (scheduler->current_task[PSB_SCENE_ENGINE_TA] != NULL &&
	    time_after_eq(jiffies, scheduler->ta_end_jiffies)) {
		*lockup = 1;
	}
	if (!*lockup
	    && (scheduler->current_task[PSB_SCENE_ENGINE_RASTER] != NULL)
	    && time_after_eq(jiffies, scheduler->raster_end_jiffies)) {
		*lockup = 1;
	}
	if (!*lockup)
		*idle = scheduler->idle;

	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
}

static inline void psb_set_idle(struct psb_scheduler *scheduler)
{
	scheduler->idle =
	    (scheduler->current_task[PSB_SCENE_ENGINE_RASTER] == NULL) &&
	    (scheduler->current_task[PSB_SCENE_ENGINE_TA] == NULL);
	if (scheduler->idle)
		wake_up(&scheduler->idle_queue);
}

/*
 * Call with the scheduler spinlock held.
 * Assigns a scene context to either the ta or the rasterizer,
 * flushing out other scenes to memory if necessary.
 */

static int psb_set_scene_fire(struct psb_scheduler *scheduler,
			      struct psb_scene *scene,
			      int engine, struct psb_task *task)
{
	uint32_t flags = 0;
	struct psb_hw_scene *hw_scene;
	struct drm_device *dev = scene->dev;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;

	hw_scene = scene->hw_scene;
	if (hw_scene && hw_scene->last_scene == scene) {

		/*
		 * Reuse the last hw scene context and delete it from the
		 * free list.
		 */

		PSB_DEBUG_RENDER("Reusing hw scene %d.\n",
				 hw_scene->context_number);
		if (scene->flags & PSB_SCENE_FLAG_DIRTY) {

			/*
			 * No hw context initialization to be done.
			 */

			flags |= PSB_SCENE_FLAG_SETUP_ONLY;
		}

		list_del_init(&hw_scene->head);

	} else {
		struct list_head *list;
		hw_scene = NULL;

		/*
		 * Grab a new hw scene context.
		 */

		list_for_each(list, &scheduler->hw_scenes) {
			hw_scene = list_entry(list, struct psb_hw_scene, head);
			break;
		}
		BUG_ON(!hw_scene);
		PSB_DEBUG_RENDER("New hw scene %d.\n",
				 hw_scene->context_number);

		list_del_init(list);
	}
	scene->hw_scene = hw_scene;
	hw_scene->last_scene = scene;

	flags |= PSB_SCENE_FLAG_SETUP;

	/*
	 * Switch context and setup the engine.
	 */

	return psb_xhw_scene_bind_fire(dev_priv,
				       &task->buf,
				       task->flags,
				       hw_scene->context_number,
				       scene->hw_cookie,
				       task->oom_cmds,
				       task->oom_cmd_size,
				       scene->hw_data->offset,
				       engine, flags | scene->flags);
}

static inline void psb_report_fence(struct psb_scheduler *scheduler,
				    uint32_t class,
				    uint32_t sequence,
				    uint32_t type, int call_handler)
{
	struct psb_scheduler_seq *seq = &scheduler->seq[type];

	seq->sequence = sequence;
	seq->reported = 0;
	if (call_handler)
		psb_fence_handler(scheduler->dev, class);
}

static void psb_schedule_raster(struct drm_psb_private *dev_priv,
				struct psb_scheduler *scheduler);

static void psb_schedule_ta(struct drm_psb_private *dev_priv,
			    struct psb_scheduler *scheduler)
{
	struct psb_task *task = NULL;
	struct list_head *list, *next;
	int pushed_raster_task = 0;

	PSB_DEBUG_RENDER("schedule ta\n");

	if (scheduler->idle_count != 0)
		return;

	if (scheduler->current_task[PSB_SCENE_ENGINE_TA] != NULL)
		return;

	if (scheduler->ta_state)
		return;

	/*
	 * Skip the ta stage for rasterization-only
	 * tasks. They arrive here to make sure we're rasterizing
	 * tasks in the correct order.
	 */

	list_for_each_safe(list, next, &scheduler->ta_queue) {
		task = list_entry(list, struct psb_task, head);
		if (task->task_type != psb_raster_task)
			break;

		list_del_init(list);
		list_add_tail(list, &scheduler->raster_queue);
		psb_report_fence(scheduler, task->engine, task->sequence,
				 _PSB_FENCE_TA_DONE_SHIFT, 1);
		task = NULL;
		pushed_raster_task = 1;
	}

	if (pushed_raster_task)
		psb_schedule_raster(dev_priv, scheduler);

	if (!task)
		return;

	/*
	 * Still waiting for a vistest?
	 */

	if (scheduler->feedback_task == task)
		return;

#ifdef ONLY_ONE_JOB_IN_RASTER_QUEUE

	/*
	 * Block ta from trying to use both hardware contexts
	 * without the rasterizer starting to render from one of them.
	 */

	if (!list_empty(&scheduler->raster_queue)) {
		return;
	}
#endif

#ifdef PSB_BLOCK_OVERLAP
	/*
	 * Make sure rasterizer isn't doing anything.
	 */
	if (scheduler->current_task[PSB_SCENE_ENGINE_RASTER] != NULL)
		return;
#endif
	if (list_empty(&scheduler->hw_scenes))
		return;

#ifdef FIX_TG_16
	if (psb_check_2d_idle(dev_priv))
		return;
#endif

	list_del_init(&task->head);
	if (task->flags & PSB_FIRE_FLAG_XHW_OOM)
		scheduler->ta_state = 1;

	scheduler->current_task[PSB_SCENE_ENGINE_TA] = task;
	scheduler->idle = 0;
	scheduler->ta_end_jiffies = jiffies + PSB_TA_TIMEOUT;

	task->reply_flags = (task->flags & PSB_FIRE_FLAG_XHW_OOM) ?
	    0x00000000 : PSB_RF_FIRE_TA;

	(void)psb_reg_submit(dev_priv, task->ta_cmds, task->ta_cmd_size);
	psb_set_scene_fire(scheduler, task->scene, PSB_SCENE_ENGINE_TA, task);
	psb_schedule_watchdog(dev_priv);
}

static int psb_fire_raster(struct psb_scheduler *scheduler,
			   struct psb_task *task)
{
	struct drm_device *dev = scheduler->dev;
	struct drm_psb_private *dev_priv = (struct drm_psb_private *)
	    dev->dev_private;

	PSB_DEBUG_RENDER("Fire raster %d\n", task->sequence);

	return psb_xhw_fire_raster(dev_priv, &task->buf, task->flags);
}

/*
 * Take the first rasterization task from the hp raster queue or from the
 * raster queue and fire the rasterizer.
 */

static void psb_schedule_raster(struct drm_psb_private *dev_priv,
				struct psb_scheduler *scheduler)
{
	struct psb_task *task;
	struct list_head *list;

	if (scheduler->idle_count != 0)
		return;

	if (scheduler->current_task[PSB_SCENE_ENGINE_RASTER] != NULL) {
		PSB_DEBUG_RENDER("Raster busy.\n");
		return;
	}
/* #ifdef PSB_BLOCK_OVERLAP */
#if 1
	if (scheduler->current_task[PSB_SCENE_ENGINE_TA] != NULL) {
		PSB_DEBUG_RENDER("TA busy.\n");
		return;
	}
#endif

	if (!list_empty(&scheduler->hp_raster_queue))
		list = scheduler->hp_raster_queue.next;
	else if (!list_empty(&scheduler->raster_queue))
		list = scheduler->raster_queue.next;
	else {
		PSB_DEBUG_RENDER("Nothing in list\n");
		return;
	}

	task = list_entry(list, struct psb_task, head);

	/*
	 * Sometimes changing ZLS format requires an ISP reset.
	 * Doesn't seem to consume too much time.
	 */

	if (task->scene)
		PSB_WSGX32(_PSB_CS_RESET_ISP_RESET, PSB_CR_SOFT_RESET);

	scheduler->current_task[PSB_SCENE_ENGINE_RASTER] = task;

	list_del_init(list);
	scheduler->idle = 0;
	scheduler->raster_end_jiffies = jiffies + PSB_RASTER_TIMEOUT;
	scheduler->total_raster_jiffies = 0;

	if (task->scene)
		PSB_WSGX32(0, PSB_CR_SOFT_RESET);

	(void)psb_reg_submit(dev_priv, task->raster_cmds,
			     task->raster_cmd_size);

	if (task->scene) {
		task->reply_flags = (task->flags & PSB_FIRE_FLAG_XHW_OOM) ?
		    0x00000000 : PSB_RF_FIRE_RASTER;
		psb_set_scene_fire(scheduler,
				   task->scene, PSB_SCENE_ENGINE_RASTER, task);
	} else {
		task->reply_flags = PSB_RF_DEALLOC | PSB_RF_FIRE_RASTER;
		psb_fire_raster(scheduler, task);
	}
	psb_schedule_watchdog(dev_priv);
}

int psb_extend_raster_timeout(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;
	int ret;

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	scheduler->total_raster_jiffies +=
	    jiffies - scheduler->raster_end_jiffies + PSB_RASTER_TIMEOUT;
	scheduler->raster_end_jiffies = jiffies + PSB_RASTER_TIMEOUT;
	ret = (scheduler->total_raster_jiffies > PSB_ALLOWED_RASTER_RUNTIME) ?
	    -EBUSY : 0;
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
	return ret;
}

/*
 * TA done handler.
 */

static void psb_ta_done(struct drm_psb_private *dev_priv,
			struct psb_scheduler *scheduler)
{
	struct psb_task *task = scheduler->current_task[PSB_SCENE_ENGINE_TA];
	struct psb_scene *scene = task->scene;

	PSB_DEBUG_RENDER("TA done %u\n", task->sequence);

	switch (task->ta_complete_action) {
	case PSB_RASTER_BLOCK:
		scheduler->ta_state = 1;
		scene->flags |=
		    (PSB_SCENE_FLAG_DIRTY | PSB_SCENE_FLAG_COMPLETE);
		list_add_tail(&task->head, &scheduler->raster_queue);
		break;
	case PSB_RASTER:
		scene->flags |=
		    (PSB_SCENE_FLAG_DIRTY | PSB_SCENE_FLAG_COMPLETE);
		list_add_tail(&task->head, &scheduler->raster_queue);
		break;
	case PSB_RETURN:
		scheduler->ta_state = 0;
		scene->flags |= PSB_SCENE_FLAG_DIRTY;
		list_add_tail(&scene->hw_scene->head, &scheduler->hw_scenes);

		break;
	}

	scheduler->current_task[PSB_SCENE_ENGINE_TA] = NULL;

#ifdef FIX_TG_16
	psb_2d_atomic_unlock(dev_priv);
#endif

	if (task->ta_complete_action != PSB_RASTER_BLOCK)
		psb_report_fence(scheduler, task->engine, task->sequence,
				 _PSB_FENCE_TA_DONE_SHIFT, 1);

	psb_schedule_raster(dev_priv, scheduler);
	psb_schedule_ta(dev_priv, scheduler);
	psb_set_idle(scheduler);

	if (task->ta_complete_action != PSB_RETURN)
		return;

	list_add_tail(&task->head, &scheduler->task_done_queue);
	schedule_delayed_work(&scheduler->wq, 1);
}

/*
 * Rasterizer done handler.
 */

static void psb_raster_done(struct drm_psb_private *dev_priv,
			    struct psb_scheduler *scheduler)
{
	struct psb_task *task =
	    scheduler->current_task[PSB_SCENE_ENGINE_RASTER];
	struct psb_scene *scene = task->scene;
	uint32_t complete_action = task->raster_complete_action;

	PSB_DEBUG_RENDER("Raster done %u\n", task->sequence);

	scheduler->current_task[PSB_SCENE_ENGINE_RASTER] = NULL;

	if (complete_action != PSB_RASTER)
		psb_schedule_raster(dev_priv, scheduler);

	if (scene) {
		if (task->feedback.page) {
			if (unlikely(scheduler->feedback_task)) {
				/*
				 * This should never happen, since the previous
				 * feedback query will return before the next
				 * raster task is fired.
				 */
				DRM_ERROR("Feedback task busy.\n");
			}
			scheduler->feedback_task = task;
			psb_xhw_vistest(dev_priv, &task->buf);
		}
		switch (complete_action) {
		case PSB_RETURN:
			scene->flags &=
			    ~(PSB_SCENE_FLAG_DIRTY | PSB_SCENE_FLAG_COMPLETE);
			list_add_tail(&scene->hw_scene->head,
				      &scheduler->hw_scenes);
			psb_report_fence(scheduler, task->engine,
					 task->sequence,
					 _PSB_FENCE_SCENE_DONE_SHIFT, 1);
			if (task->flags & PSB_FIRE_FLAG_XHW_OOM) {
				scheduler->ta_state = 0;
			}
			break;
		case PSB_RASTER:
			list_add(&task->head, &scheduler->raster_queue);
			task->raster_complete_action = PSB_RETURN;
			psb_schedule_raster(dev_priv, scheduler);
			break;
		case PSB_TA:
			list_add(&task->head, &scheduler->ta_queue);
			scheduler->ta_state = 0;
			task->raster_complete_action = PSB_RETURN;
			task->ta_complete_action = PSB_RASTER;
			break;

		}
	}
	psb_schedule_ta(dev_priv, scheduler);
	psb_set_idle(scheduler);

	if (complete_action == PSB_RETURN) {
		if (task->scene == NULL) {
			psb_report_fence(scheduler, task->engine,
					 task->sequence,
					 _PSB_FENCE_RASTER_DONE_SHIFT, 1);
		}
		if (!task->feedback.page) {
			list_add_tail(&task->head, &scheduler->task_done_queue);
			schedule_delayed_work(&scheduler->wq, 1);
		}
	}

}

void psb_scheduler_pause(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	scheduler->idle_count++;
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
}

void psb_scheduler_restart(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	if (--scheduler->idle_count == 0) {
		psb_schedule_ta(dev_priv, scheduler);
		psb_schedule_raster(dev_priv, scheduler);
	}
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
}

int psb_scheduler_idle(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;
	int ret;
	spin_lock_irqsave(&scheduler->lock, irq_flags);
	ret = scheduler->idle_count != 0 && scheduler->idle;
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
	return ret;
}

int psb_scheduler_finished(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;
	int ret;
	spin_lock_irqsave(&scheduler->lock, irq_flags);
	ret = (scheduler->idle &&
	       list_empty(&scheduler->raster_queue) &&
	       list_empty(&scheduler->ta_queue) &&
	       list_empty(&scheduler->hp_raster_queue));
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
	return ret;
}

static void psb_ta_oom(struct drm_psb_private *dev_priv,
		       struct psb_scheduler *scheduler)
{

	struct psb_task *task = scheduler->current_task[PSB_SCENE_ENGINE_TA];
	if (!task)
		return;

	if (task->aborting)
		return;
	task->aborting = 1;

	DRM_INFO("Info: TA out of parameter memory.\n");

	(void)psb_xhw_ta_oom(dev_priv, &task->buf, task->scene->hw_cookie);
}

static void psb_ta_oom_reply(struct drm_psb_private *dev_priv,
			     struct psb_scheduler *scheduler)
{

	struct psb_task *task = scheduler->current_task[PSB_SCENE_ENGINE_TA];
	uint32_t flags;
	if (!task)
		return;

	psb_xhw_ta_oom_reply(dev_priv, &task->buf,
			     task->scene->hw_cookie,
			     &task->ta_complete_action,
			     &task->raster_complete_action, &flags);
	task->flags |= flags;
	task->aborting = 0;
	psb_dispatch_ta(dev_priv, scheduler, PSB_RF_OOM_REPLY);
}

static void psb_ta_hw_scene_freed(struct drm_psb_private *dev_priv,
				  struct psb_scheduler *scheduler)
{
	DRM_ERROR("TA hw scene freed.\n");
}

static void psb_vistest_reply(struct drm_psb_private *dev_priv,
			      struct psb_scheduler *scheduler)
{
	struct psb_task *task = scheduler->feedback_task;
	uint8_t *feedback_map;
	uint32_t add;
	uint32_t cur;
	struct drm_psb_vistest *vistest;
	int i;

	scheduler->feedback_task = NULL;
	if (!task) {
		DRM_ERROR("No Poulsbo feedback task.\n");
		return;
	}
	if (!task->feedback.page) {
		DRM_ERROR("No Poulsbo feedback page.\n");
		goto out;
	}

	if (in_irq())
		feedback_map = kmap_atomic(task->feedback.page, KM_IRQ0);
	else
		feedback_map = kmap_atomic(task->feedback.page, KM_USER0);

	/*
	 * Loop over all requested vistest components here.
	 * Only one (vistest) currently.
	 */

	vistest = (struct drm_psb_vistest *)
	    (feedback_map + task->feedback.offset);

	for (i = 0; i < PSB_HW_FEEDBACK_SIZE; ++i) {
		add = task->buf.arg.arg.feedback[i];
		cur = vistest->vt[i];

		/*
		 * Vistest saturates.
		 */

		vistest->vt[i] = (cur + add < cur) ? ~0 : cur + add;
	}
	if (in_irq())
		kunmap_atomic(feedback_map, KM_IRQ0);
	else
		kunmap_atomic(feedback_map, KM_USER0);
      out:
	psb_report_fence(scheduler, task->engine, task->sequence,
			 _PSB_FENCE_FEEDBACK_SHIFT, 1);

	if (list_empty(&task->head)) {
		list_add_tail(&task->head, &scheduler->task_done_queue);
		schedule_delayed_work(&scheduler->wq, 1);
	} else
		psb_schedule_ta(dev_priv, scheduler);
}

static void psb_ta_fire_reply(struct drm_psb_private *dev_priv,
			      struct psb_scheduler *scheduler)
{
	struct psb_task *task = scheduler->current_task[PSB_SCENE_ENGINE_TA];

	psb_xhw_fire_reply(dev_priv, &task->buf, task->scene->hw_cookie);

	psb_dispatch_ta(dev_priv, scheduler, PSB_RF_FIRE_TA);
}

static void psb_raster_fire_reply(struct drm_psb_private *dev_priv,
				  struct psb_scheduler *scheduler)
{
	struct psb_task *task =
	    scheduler->current_task[PSB_SCENE_ENGINE_RASTER];
	uint32_t reply_flags;

	if (!task) {
		DRM_ERROR("Null task.\n");
		return;
	}

	task->raster_complete_action = task->buf.arg.arg.sb.rca;
	psb_xhw_fire_reply(dev_priv, &task->buf, task->scene->hw_cookie);

	reply_flags = PSB_RF_FIRE_RASTER;
	if (task->raster_complete_action == PSB_RASTER)
		reply_flags |= PSB_RF_DEALLOC;

	psb_dispatch_raster(dev_priv, scheduler, reply_flags);
}

static int psb_user_interrupt(struct drm_psb_private *dev_priv,
			      struct psb_scheduler *scheduler)
{
	uint32_t type;
	int ret;
	unsigned long irq_flags;

	/*
	 * Xhw cannot write directly to the comm page, so
	 * do it here. Firmware would have written directly.
	 */

	ret = psb_xhw_handler(dev_priv);
	if (unlikely(ret))
		return ret;

	spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
	type = dev_priv->comm[PSB_COMM_USER_IRQ];
	dev_priv->comm[PSB_COMM_USER_IRQ] = 0;
	if (dev_priv->comm[PSB_COMM_USER_IRQ_LOST]) {
		dev_priv->comm[PSB_COMM_USER_IRQ_LOST] = 0;
		DRM_ERROR("Lost Poulsbo hardware event.\n");
	}
	spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);

	if (type == 0)
		return 0;

	switch (type) {
	case PSB_UIRQ_VISTEST:
		psb_vistest_reply(dev_priv, scheduler);
		break;
	case PSB_UIRQ_OOM_REPLY:
		psb_ta_oom_reply(dev_priv, scheduler);
		break;
	case PSB_UIRQ_FIRE_TA_REPLY:
		psb_ta_fire_reply(dev_priv, scheduler);
		break;
	case PSB_UIRQ_FIRE_RASTER_REPLY:
		psb_raster_fire_reply(dev_priv, scheduler);
		break;
	default:
		DRM_ERROR("Unknown Poulsbo hardware event. %d\n", type);
	}
	return 0;
}

int psb_forced_user_interrupt(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;
	int ret;

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	ret = psb_user_interrupt(dev_priv, scheduler);
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
	return ret;
}

static void psb_dispatch_ta(struct drm_psb_private *dev_priv,
			    struct psb_scheduler *scheduler,
			    uint32_t reply_flag)
{
	struct psb_task *task = scheduler->current_task[PSB_SCENE_ENGINE_TA];
	uint32_t flags;
	uint32_t mask;

	task->reply_flags |= reply_flag;
	flags = task->reply_flags;
	mask = PSB_RF_FIRE_TA;

	if (!(flags & mask))
		return;

	mask = PSB_RF_TA_DONE;
	if ((flags & mask) == mask) {
		task->reply_flags &= ~mask;
		psb_ta_done(dev_priv, scheduler);
	}

	mask = PSB_RF_OOM;
	if ((flags & mask) == mask) {
		task->reply_flags &= ~mask;
		psb_ta_oom(dev_priv, scheduler);
	}

	mask = (PSB_RF_OOM_REPLY | PSB_RF_TERMINATE);
	if ((flags & mask) == mask) {
		task->reply_flags &= ~mask;
		psb_ta_done(dev_priv, scheduler);
	}
}

static void psb_dispatch_raster(struct drm_psb_private *dev_priv,
				struct psb_scheduler *scheduler,
				uint32_t reply_flag)
{
	struct psb_task *task =
	    scheduler->current_task[PSB_SCENE_ENGINE_RASTER];
	uint32_t flags;
	uint32_t mask;

	task->reply_flags |= reply_flag;
	flags = task->reply_flags;
	mask = PSB_RF_FIRE_RASTER;

	if (!(flags & mask))
		return;

	/*
	 * For rasterizer-only tasks, don't report fence done here,
	 * as this is time consuming and the rasterizer wants a new
	 * task immediately. For other tasks, the hardware is probably
	 * still busy deallocating TA memory, so we can report
	 * fence done in parallel.
	 */

	if (task->raster_complete_action == PSB_RETURN &&
	    (reply_flag & PSB_RF_RASTER_DONE) && task->scene != NULL) {
		psb_report_fence(scheduler, task->engine, task->sequence,
				 _PSB_FENCE_RASTER_DONE_SHIFT, 1);
	}

	mask = PSB_RF_RASTER_DONE | PSB_RF_DEALLOC;
	if ((flags & mask) == mask) {
		task->reply_flags &= ~mask;
		psb_raster_done(dev_priv, scheduler);
	}
}

void psb_scheduler_handler(struct drm_psb_private *dev_priv, uint32_t status)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;

	spin_lock(&scheduler->lock);

	if (status & _PSB_CE_PIXELBE_END_RENDER) {
		psb_dispatch_raster(dev_priv, scheduler, PSB_RF_RASTER_DONE);
	}
	if (status & _PSB_CE_DPM_3D_MEM_FREE) {
		psb_dispatch_raster(dev_priv, scheduler, PSB_RF_DEALLOC);
	}
	if (status & _PSB_CE_TA_FINISHED) {
		psb_dispatch_ta(dev_priv, scheduler, PSB_RF_TA_DONE);
	}
	if (status & _PSB_CE_TA_TERMINATE) {
		psb_dispatch_ta(dev_priv, scheduler, PSB_RF_TERMINATE);
	}
	if (status & (_PSB_CE_DPM_REACHED_MEM_THRESH |
		      _PSB_CE_DPM_OUT_OF_MEMORY_GBL |
		      _PSB_CE_DPM_OUT_OF_MEMORY_MT)) {
		psb_dispatch_ta(dev_priv, scheduler, PSB_RF_OOM);
	}
	if (status & _PSB_CE_DPM_TA_MEM_FREE) {
		psb_ta_hw_scene_freed(dev_priv, scheduler);
	}
	if (status & _PSB_CE_SW_EVENT) {
		psb_user_interrupt(dev_priv, scheduler);
	}
	spin_unlock(&scheduler->lock);
}

static void psb_free_task_wq(struct work_struct *work)
{
	struct psb_scheduler *scheduler =
	    container_of(work, struct psb_scheduler, wq.work);

	struct drm_device *dev = scheduler->dev;
	struct list_head *list, *next;
	unsigned long irq_flags;
	struct psb_task *task;

	if (!mutex_trylock(&scheduler->task_wq_mutex))
		return;

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	list_for_each_safe(list, next, &scheduler->task_done_queue) {
		task = list_entry(list, struct psb_task, head);
		list_del_init(list);
		spin_unlock_irqrestore(&scheduler->lock, irq_flags);

		PSB_DEBUG_RENDER("Checking Task %d: Scene 0x%08lx, "
				 "Feedback bo 0x%08lx, done %d\n",
				 task->sequence, (unsigned long)task->scene,
				 (unsigned long)task->feedback.bo,
				 atomic_read(&task->buf.done));

		if (task->scene) {
			mutex_lock(&dev->struct_mutex);
			PSB_DEBUG_RENDER("Unref scene %d\n", task->sequence);
			psb_scene_unref_devlocked(&task->scene);
			if (task->feedback.bo) {
				PSB_DEBUG_RENDER("Unref feedback bo %d\n",
						 task->sequence);
				drm_bo_usage_deref_locked(&task->feedback.bo);
			}
			mutex_unlock(&dev->struct_mutex);
		}

		if (atomic_read(&task->buf.done)) {
			PSB_DEBUG_RENDER("Deleting task %d\n", task->sequence);
			drm_free(task, sizeof(*task), DRM_MEM_DRIVER);
			task = NULL;
		}
		spin_lock_irqsave(&scheduler->lock, irq_flags);
		if (task != NULL)
			list_add(list, &scheduler->task_done_queue);
	}
	if (!list_empty(&scheduler->task_done_queue)) {
		PSB_DEBUG_RENDER("Rescheduling wq\n");
		schedule_delayed_work(&scheduler->wq, 1);
	}
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);

	mutex_unlock(&scheduler->task_wq_mutex);
}

/*
 * Check if any of the tasks in the queues is using a scene.
 * In that case we know the TA memory buffer objects are
 * fenced and will not be evicted until that fence is signaled.
 */

void psb_scheduler_ta_mem_check(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;
	struct psb_task *task;
	struct psb_task *next_task;

	dev_priv->force_ta_mem_load = 1;
	spin_lock_irqsave(&scheduler->lock, irq_flags);
	list_for_each_entry_safe(task, next_task, &scheduler->ta_queue, head) {
		if (task->scene) {
			dev_priv->force_ta_mem_load = 0;
			break;
		}
	}
	list_for_each_entry_safe(task, next_task, &scheduler->raster_queue,
				 head) {
		if (task->scene) {
			dev_priv->force_ta_mem_load = 0;
			break;
		}
	}
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
}

void psb_scheduler_reset(struct drm_psb_private *dev_priv, int error_condition)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long wait_jiffies;
	unsigned long cur_jiffies;
	struct psb_task *task;
	struct psb_task *next_task;
	unsigned long irq_flags;

	psb_scheduler_pause(dev_priv);
	if (!psb_scheduler_idle(dev_priv)) {
		spin_lock_irqsave(&scheduler->lock, irq_flags);

		cur_jiffies = jiffies;
		wait_jiffies = cur_jiffies;
		if (scheduler->current_task[PSB_SCENE_ENGINE_TA] &&
		    time_after_eq(scheduler->ta_end_jiffies, wait_jiffies))
			wait_jiffies = scheduler->ta_end_jiffies;
		if (scheduler->current_task[PSB_SCENE_ENGINE_RASTER] &&
		    time_after_eq(scheduler->raster_end_jiffies, wait_jiffies))
			wait_jiffies = scheduler->raster_end_jiffies;

		wait_jiffies -= cur_jiffies;
		spin_unlock_irqrestore(&scheduler->lock, irq_flags);

		(void)wait_event_timeout(scheduler->idle_queue,
					 psb_scheduler_idle(dev_priv),
					 wait_jiffies);
	}

	if (!psb_scheduler_idle(dev_priv)) {
		spin_lock_irqsave(&scheduler->lock, irq_flags);
		task = scheduler->current_task[PSB_SCENE_ENGINE_RASTER];
		if (task) {
			DRM_ERROR("Detected Poulsbo rasterizer lockup.\n");
			if (task->engine == PSB_ENGINE_HPRAST) {
				psb_fence_error(scheduler->dev,
						PSB_ENGINE_HPRAST,
						task->sequence,
						_PSB_FENCE_TYPE_RASTER_DONE,
						error_condition);

				list_del(&task->head);
				psb_xhw_clean_buf(dev_priv, &task->buf);
				list_add_tail(&task->head,
					      &scheduler->task_done_queue);
			} else {
				list_add(&task->head, &scheduler->raster_queue);
			}
		}
		scheduler->current_task[PSB_SCENE_ENGINE_RASTER] = NULL;
		task = scheduler->current_task[PSB_SCENE_ENGINE_TA];
		if (task) {
			DRM_ERROR("Detected Poulsbo ta lockup.\n");
			list_add_tail(&task->head, &scheduler->raster_queue);
#ifdef FIX_TG_16
			psb_2d_atomic_unlock(dev_priv);
#endif
		}
		scheduler->current_task[PSB_SCENE_ENGINE_TA] = NULL;
		scheduler->ta_state = 0;

#ifdef FIX_TG_16
		atomic_set(&dev_priv->ta_wait_2d, 0);
		atomic_set(&dev_priv->ta_wait_2d_irq, 0);
		wake_up(&dev_priv->queue_2d);
#endif
		spin_unlock_irqrestore(&scheduler->lock, irq_flags);
	}

	/*
	 * Empty raster queue.
	 */

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	list_for_each_entry_safe(task, next_task, &scheduler->raster_queue,
				 head) {
		struct psb_scene *scene = task->scene;

		psb_fence_error(scheduler->dev,
				task->engine,
				task->sequence,
				_PSB_FENCE_TYPE_TA_DONE |
				_PSB_FENCE_TYPE_RASTER_DONE |
				_PSB_FENCE_TYPE_SCENE_DONE |
				_PSB_FENCE_TYPE_FEEDBACK, error_condition);
		if (scene) {
			scene->flags = 0;
			if (scene->hw_scene) {
				list_add_tail(&scene->hw_scene->head,
					      &scheduler->hw_scenes);
				scene->hw_scene = NULL;
			}
		}

		psb_xhw_clean_buf(dev_priv, &task->buf);
		list_del(&task->head);
		list_add_tail(&task->head, &scheduler->task_done_queue);
	}

	schedule_delayed_work(&scheduler->wq, 1);
	scheduler->idle = 1;
	wake_up(&scheduler->idle_queue);

	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
	psb_scheduler_restart(dev_priv);

}

int psb_scheduler_init(struct drm_device *dev, struct psb_scheduler *scheduler)
{
	struct psb_hw_scene *hw_scene;
	int i;

	memset(scheduler, 0, sizeof(*scheduler));
	scheduler->dev = dev;
	mutex_init(&scheduler->task_wq_mutex);
	scheduler->lock = SPIN_LOCK_UNLOCKED;
	scheduler->idle = 1;

	INIT_LIST_HEAD(&scheduler->ta_queue);
	INIT_LIST_HEAD(&scheduler->raster_queue);
	INIT_LIST_HEAD(&scheduler->hp_raster_queue);
	INIT_LIST_HEAD(&scheduler->hw_scenes);
	INIT_LIST_HEAD(&scheduler->task_done_queue);
	INIT_DELAYED_WORK(&scheduler->wq, &psb_free_task_wq);
	init_waitqueue_head(&scheduler->idle_queue);

	for (i = 0; i < PSB_NUM_HW_SCENES; ++i) {
		hw_scene = &scheduler->hs[i];
		hw_scene->context_number = i;
		list_add_tail(&hw_scene->head, &scheduler->hw_scenes);
	}

	for (i = 0; i < _PSB_ENGINE_TA_FENCE_TYPES; ++i) {
		scheduler->seq[i].reported = 0;
	}

	return 0;
}

/*
 * Scene references maintained by the scheduler are not refcounted.
 * Remove all references to a particular scene here.
 */

void psb_scheduler_remove_scene_refs(struct psb_scene *scene)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)scene->dev->dev_private;
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	struct psb_hw_scene *hw_scene;
	unsigned long irq_flags;
	unsigned int i;

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	for (i = 0; i < PSB_NUM_HW_SCENES; ++i) {
		hw_scene = &scheduler->hs[i];
		if (hw_scene->last_scene == scene) {
			BUG_ON(list_empty(&hw_scene->head));
			hw_scene->last_scene = NULL;
		}
	}
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
}

void psb_scheduler_takedown(struct psb_scheduler *scheduler)
{
	flush_scheduled_work();
}

static int psb_setup_task_devlocked(struct drm_device *dev,
				    struct drm_psb_cmdbuf_arg *arg,
				    struct drm_buffer_object *raster_cmd_buffer,
				    struct drm_buffer_object *ta_cmd_buffer,
				    struct drm_buffer_object *oom_cmd_buffer,
				    struct psb_scene *scene,
				    enum psb_task_type task_type,
				    uint32_t engine,
				    uint32_t flags, struct psb_task **task_p)
{
	struct psb_task *task;
	int ret;

	if (ta_cmd_buffer && arg->ta_size > PSB_MAX_TA_CMDS) {
		DRM_ERROR("Too many ta cmds %d.\n", arg->ta_size);
		return -EINVAL;
	}
	if (raster_cmd_buffer && arg->cmdbuf_size > PSB_MAX_RASTER_CMDS) {
		DRM_ERROR("Too many raster cmds %d.\n", arg->cmdbuf_size);
		return -EINVAL;
	}
	if (oom_cmd_buffer && arg->oom_size > PSB_MAX_OOM_CMDS) {
		DRM_ERROR("Too many raster cmds %d.\n", arg->oom_size);
		return -EINVAL;
	}

	task = drm_calloc(1, sizeof(*task), DRM_MEM_DRIVER);
	if (!task)
		return -ENOMEM;

	atomic_set(&task->buf.done, 1);
	task->engine = engine;
	INIT_LIST_HEAD(&task->head);
	INIT_LIST_HEAD(&task->buf.head);
	if (ta_cmd_buffer && arg->ta_size != 0) {
		task->ta_cmd_size = arg->ta_size;
		ret = psb_submit_copy_cmdbuf(dev, ta_cmd_buffer,
					     arg->ta_offset,
					     arg->ta_size,
					     PSB_ENGINE_TA, task->ta_cmds);
		if (ret)
			goto out_err;
	}
	if (raster_cmd_buffer) {
		task->raster_cmd_size = arg->cmdbuf_size;
		ret = psb_submit_copy_cmdbuf(dev, raster_cmd_buffer,
					     arg->cmdbuf_offset,
					     arg->cmdbuf_size,
					     PSB_ENGINE_TA, task->raster_cmds);
		if (ret)
			goto out_err;
	}
	if (oom_cmd_buffer && arg->oom_size != 0) {
		task->oom_cmd_size = arg->oom_size;
		ret = psb_submit_copy_cmdbuf(dev, oom_cmd_buffer,
					     arg->oom_offset,
					     arg->oom_size,
					     PSB_ENGINE_TA, task->oom_cmds);
		if (ret)
			goto out_err;
	}
	task->task_type = task_type;
	task->flags = flags;
	if (scene)
		task->scene = psb_scene_ref(scene);

#ifdef PSB_DETEAR
	if(PSB_VIDEO_BLIT == arg->sVideoInfo.flag) {
		task->bVideoFlag = PSB_VIDEO_BLIT;
		task->x = arg->sVideoInfo.x;
		task->y = arg->sVideoInfo.y;
		task->w = arg->sVideoInfo.w;
		task->h = arg->sVideoInfo.h;
		task->pFBBOHandle = arg->sVideoInfo.pFBBOHandle;
		task->pFBVirtAddr = arg->sVideoInfo.pFBVirtAddr; 
	}
#endif

	*task_p = task;
	return 0;
      out_err:
	drm_free(task, sizeof(*task), DRM_MEM_DRIVER);
	*task_p = NULL;
	return ret;
}

int psb_cmdbuf_ta(struct drm_file *priv,
		  struct drm_psb_cmdbuf_arg *arg,
		  struct drm_buffer_object *cmd_buffer,
		  struct drm_buffer_object *ta_buffer,
		  struct drm_buffer_object *oom_buffer,
		  struct psb_scene *scene,
		  struct psb_feedback_info *feedback,
		  struct drm_fence_arg *fence_arg)
{
	struct drm_device *dev = priv->head->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_fence_object *fence = NULL;
	struct psb_task *task = NULL;
	int ret;
	struct psb_scheduler *scheduler = &dev_priv->scheduler;

	PSB_DEBUG_RENDER("Cmdbuf ta\n");

	ret = mutex_lock_interruptible(&dev_priv->reset_mutex);
	if (ret)
		return -EAGAIN;

	mutex_lock(&dev->struct_mutex);
	ret = psb_setup_task_devlocked(dev, arg, cmd_buffer, ta_buffer,
				       oom_buffer, scene,
				       psb_ta_task, PSB_ENGINE_TA,
				       PSB_FIRE_FLAG_RASTER_DEALLOC, &task);
	mutex_unlock(&dev->struct_mutex);

	if (ret)
		goto out_err;

	task->feedback = *feedback;

	/*
	 * Hand the task over to the scheduler.
	 */

	spin_lock_irq(&scheduler->lock);
	task->sequence = psb_fence_advance_sequence(dev, PSB_ENGINE_TA);

	task->ta_complete_action = PSB_RASTER;
	task->raster_complete_action = PSB_RETURN;

	list_add_tail(&task->head, &scheduler->ta_queue);
	PSB_DEBUG_RENDER("queued ta %u\n", task->sequence);

	psb_schedule_ta(dev_priv, scheduler);
	spin_unlock_irq(&scheduler->lock);

	psb_fence_or_sync(priv, PSB_ENGINE_TA, arg, fence_arg, &fence);
	drm_regs_fence(&dev_priv->use_manager, fence);
	if (fence) {
		spin_lock_irq(&scheduler->lock);
		psb_report_fence(scheduler, PSB_ENGINE_TA, task->sequence, 0, 1);
		spin_unlock_irq(&scheduler->lock);
		fence_arg->signaled |= DRM_FENCE_TYPE_EXE;
	}

      out_err:
	if (ret && ret != -EAGAIN)
		DRM_ERROR("TA task queue job failed.\n");

	if (fence) {
#ifdef PSB_WAIT_FOR_TA_COMPLETION
		drm_fence_object_wait(fence, 1, 1, DRM_FENCE_TYPE_EXE |
				      _PSB_FENCE_TYPE_TA_DONE);
#ifdef PSB_BE_PARANOID
		drm_fence_object_wait(fence, 1, 1, DRM_FENCE_TYPE_EXE |
				      _PSB_FENCE_TYPE_SCENE_DONE);
#endif
#endif
		drm_fence_usage_deref_unlocked(&fence);
	}
	mutex_unlock(&dev_priv->reset_mutex);

	return ret;
}

int psb_cmdbuf_raster(struct drm_file *priv,
		      struct drm_psb_cmdbuf_arg *arg,
		      struct drm_buffer_object *cmd_buffer,
		      struct drm_fence_arg *fence_arg)
{
	struct drm_device *dev = priv->head->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_fence_object *fence = NULL;
	struct psb_task *task = NULL;
	int ret;
	struct psb_scheduler *scheduler = &dev_priv->scheduler;

	uint32_t sequence_temp;

	PSB_DEBUG_RENDER("Cmdbuf Raster\n");

	ret = mutex_lock_interruptible(&dev_priv->reset_mutex);
	if (ret)
		return -EAGAIN;

	mutex_lock(&dev->struct_mutex);
	ret = psb_setup_task_devlocked(dev, arg, cmd_buffer, NULL, NULL,
				       NULL, psb_raster_task,
				       PSB_ENGINE_TA, 0, &task);
	mutex_unlock(&dev->struct_mutex);

	if (ret)
		goto out_err;

	/*
	 * Hand the task over to the scheduler.
	 */

	spin_lock_irq(&scheduler->lock);
	task->sequence = psb_fence_advance_sequence(dev, PSB_ENGINE_TA);
	sequence_temp = task->sequence; // backup for the change
	task->ta_complete_action = PSB_RASTER;
	task->raster_complete_action = PSB_RETURN;

	list_add_tail(&task->head, &scheduler->ta_queue);
	PSB_DEBUG_RENDER("queued raster %u\n", task->sequence);
	psb_schedule_ta(dev_priv, scheduler);
	spin_unlock_irq(&scheduler->lock);

	psb_fence_or_sync(priv, PSB_ENGINE_TA, arg, fence_arg, &fence);
	drm_regs_fence(&dev_priv->use_manager, fence);
	if (fence) {
		spin_lock_irq(&scheduler->lock);
		psb_report_fence(scheduler, PSB_ENGINE_TA, sequence_temp, 0, 1);
		spin_unlock_irq(&scheduler->lock);
		fence_arg->signaled |= DRM_FENCE_TYPE_EXE;
	}
      out_err:
	if (ret && ret != -EAGAIN)
		DRM_ERROR("Raster task queue job failed.\n");

	if (fence) {
#ifdef PSB_WAIT_FOR_RASTER_COMPLETION
		drm_fence_object_wait(fence, 1, 1, fence->type);
#endif
		drm_fence_usage_deref_unlocked(&fence);
	}

	mutex_unlock(&dev_priv->reset_mutex);

	return ret;
}

#ifdef FIX_TG_16

static int psb_check_2d_idle(struct drm_psb_private *dev_priv)
{
	if (psb_2d_trylock(dev_priv)) {
		if ((PSB_RSGX32(PSB_CR_2D_SOCIF) == _PSB_C2_SOCIF_EMPTY) &&
		    !((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) &
		       _PSB_C2B_STATUS_BUSY))) {
			return 0;
		}
		if (atomic_cmpxchg(&dev_priv->ta_wait_2d_irq, 0, 1) == 0)
			psb_2D_irq_on(dev_priv);

		PSB_WSGX32(PSB_2D_FENCE_BH, PSB_SGX_2D_SLAVE_PORT);
		PSB_WSGX32(PSB_2D_FLUSH_BH, PSB_SGX_2D_SLAVE_PORT);
		(void)PSB_RSGX32(PSB_SGX_2D_SLAVE_PORT);

		psb_2d_atomic_unlock(dev_priv);
	}

	atomic_set(&dev_priv->ta_wait_2d, 1);
	return -EBUSY;
}

static void psb_atomic_resume_ta_2d_idle(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;

	if (atomic_cmpxchg(&dev_priv->ta_wait_2d, 1, 0) == 1) {
		psb_schedule_ta(dev_priv, scheduler);
		if (atomic_read(&dev_priv->waiters_2d) != 0)
			wake_up(&dev_priv->queue_2d);
	}
}

void psb_resume_ta_2d_idle(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	if (atomic_cmpxchg(&dev_priv->ta_wait_2d_irq, 1, 0) == 1) {
		atomic_set(&dev_priv->ta_wait_2d, 0);
		psb_2D_irq_off(dev_priv);
		psb_schedule_ta(dev_priv, scheduler);
		if (atomic_read(&dev_priv->waiters_2d) != 0)
			wake_up(&dev_priv->queue_2d);
	}
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
}

/*
 * 2D locking functions. Can't use a mutex since the trylock() and
 * unlock() methods need to be accessible from interrupt context.
 */

static int psb_2d_trylock(struct drm_psb_private *dev_priv)
{
	return (atomic_cmpxchg(&dev_priv->lock_2d, 0, 1) == 0);
}

static void psb_2d_atomic_unlock(struct drm_psb_private *dev_priv)
{
	atomic_set(&dev_priv->lock_2d, 0);
	if (atomic_read(&dev_priv->waiters_2d) != 0)
		wake_up(&dev_priv->queue_2d);
}

void psb_2d_unlock(struct drm_psb_private *dev_priv)
{
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	psb_2d_atomic_unlock(dev_priv);
	if (atomic_read(&dev_priv->ta_wait_2d) != 0)
		psb_atomic_resume_ta_2d_idle(dev_priv);
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);
}

void psb_2d_lock(struct drm_psb_private *dev_priv)
{
	atomic_inc(&dev_priv->waiters_2d);
	wait_event(dev_priv->queue_2d, atomic_read(&dev_priv->ta_wait_2d) == 0);
	wait_event(dev_priv->queue_2d, psb_2d_trylock(dev_priv));
	atomic_dec(&dev_priv->waiters_2d);
}

#endif
