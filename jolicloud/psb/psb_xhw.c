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
 * Make calls into closed source X server code.
 */

#include "drmP.h"
#include "psb_drv.h"

void
psb_xhw_clean_buf(struct drm_psb_private *dev_priv, struct psb_xhw_buf *buf)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
	list_del_init(&buf->head);
	if (dev_priv->xhw_cur_buf == buf)
		dev_priv->xhw_cur_buf = NULL;
	atomic_set(&buf->done, 1);
	spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);
}

static inline int psb_xhw_add(struct drm_psb_private *dev_priv,
			      struct psb_xhw_buf *buf)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
	atomic_set(&buf->done, 0);
	if (unlikely(!dev_priv->xhw_submit_ok)) {
		spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);
		DRM_ERROR("No Xpsb 3D extension available.\n");
		return -EINVAL;
	}
	if (!list_empty(&buf->head)) {
		DRM_ERROR("Recursive list adding.\n");
		goto out;
	}
	list_add_tail(&buf->head, &dev_priv->xhw_in);
	wake_up_interruptible(&dev_priv->xhw_queue);
      out:
	spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);
	return 0;
}

int psb_xhw_hotplug(struct drm_psb_private *dev_priv, struct psb_xhw_buf *buf)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;
	int ret;

	buf->copy_back = 1;
	xa->op = PSB_XHW_HOTPLUG;
	xa->issue_irq = 0;
	xa->irq_op = 0;

	ret = psb_xhw_add(dev_priv, buf);
	return ret;
}

int psb_xhw_scene_info(struct drm_psb_private *dev_priv,
		       struct psb_xhw_buf *buf,
		       uint32_t w,
		       uint32_t h,
		       uint32_t * hw_cookie,
		       uint32_t * bo_size,
		       uint32_t * clear_p_start, uint32_t * clear_num_pages)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;
	int ret;

	buf->copy_back = 1;
	xa->op = PSB_XHW_SCENE_INFO;
	xa->irq_op = 0;
	xa->issue_irq = 0;
	xa->arg.si.w = w;
	xa->arg.si.h = h;

	ret = psb_xhw_add(dev_priv, buf);
	if (ret)
		return ret;

	(void)wait_event_timeout(dev_priv->xhw_caller_queue,
				 atomic_read(&buf->done), DRM_HZ);

	if (!atomic_read(&buf->done)) {
		psb_xhw_clean_buf(dev_priv, buf);
		return -EBUSY;
	}

	if (!xa->ret) {
		memcpy(hw_cookie, xa->cookie, sizeof(xa->cookie));
		*bo_size = xa->arg.si.size;
		*clear_p_start = xa->arg.si.clear_p_start;
		*clear_num_pages = xa->arg.si.clear_num_pages;
	}
	return xa->ret;
}

int psb_xhw_fire_raster(struct drm_psb_private *dev_priv,
			struct psb_xhw_buf *buf, uint32_t fire_flags)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;

	buf->copy_back = 0;
	xa->op = PSB_XHW_FIRE_RASTER;
	xa->issue_irq = 0;
	xa->arg.sb.fire_flags = 0;

	return psb_xhw_add(dev_priv, buf);
}

int psb_xhw_vistest(struct drm_psb_private *dev_priv, struct psb_xhw_buf *buf)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;

	buf->copy_back = 1;
	xa->op = PSB_XHW_VISTEST;
	/*
	 * Could perhaps decrease latency somewhat by
	 * issuing an irq in this case.
	 */
	xa->issue_irq = 0;
	xa->irq_op = PSB_UIRQ_VISTEST;
	return psb_xhw_add(dev_priv, buf);
}

int psb_xhw_scene_bind_fire(struct drm_psb_private *dev_priv,
			    struct psb_xhw_buf *buf,
			    uint32_t fire_flags,
			    uint32_t hw_context,
			    uint32_t * cookie,
			    uint32_t * oom_cmds,
			    uint32_t num_oom_cmds,
			    uint32_t offset, uint32_t engine, uint32_t flags)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;

	buf->copy_back = (fire_flags & PSB_FIRE_FLAG_XHW_OOM);
	xa->op = PSB_XHW_SCENE_BIND_FIRE;
	xa->issue_irq = (buf->copy_back) ? 1 : 0;
	if (unlikely(buf->copy_back))
		xa->irq_op = (engine == PSB_SCENE_ENGINE_TA) ?
		    PSB_UIRQ_FIRE_TA_REPLY : PSB_UIRQ_FIRE_RASTER_REPLY;
	else
		xa->irq_op = 0;
	xa->arg.sb.fire_flags = fire_flags;
	xa->arg.sb.hw_context = hw_context;
	xa->arg.sb.offset = offset;
	xa->arg.sb.engine = engine;
	xa->arg.sb.flags = flags;
	xa->arg.sb.num_oom_cmds = num_oom_cmds;
	memcpy(xa->cookie, cookie, sizeof(xa->cookie));
	if (num_oom_cmds)
		memcpy(xa->arg.sb.oom_cmds, oom_cmds,
		       sizeof(uint32_t) * num_oom_cmds);
	return psb_xhw_add(dev_priv, buf);
}

int psb_xhw_reset_dpm(struct drm_psb_private *dev_priv, struct psb_xhw_buf *buf)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;
	int ret;

	buf->copy_back = 1;
	xa->op = PSB_XHW_RESET_DPM;
	xa->issue_irq = 0;
	xa->irq_op = 0;

	ret = psb_xhw_add(dev_priv, buf);
	if (ret)
		return ret;

	(void)wait_event_timeout(dev_priv->xhw_caller_queue,
				 atomic_read(&buf->done), 3 * DRM_HZ);

	if (!atomic_read(&buf->done)) {
		psb_xhw_clean_buf(dev_priv, buf);
		return -EBUSY;
	}

	return xa->ret;
}

int psb_xhw_check_lockup(struct drm_psb_private *dev_priv,
			 struct psb_xhw_buf *buf, uint32_t * value)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;
	int ret;

	*value = 0;

	buf->copy_back = 1;
	xa->op = PSB_XHW_CHECK_LOCKUP;
	xa->issue_irq = 0;
	xa->irq_op = 0;

	ret = psb_xhw_add(dev_priv, buf);
	if (ret)
		return ret;

	(void)wait_event_timeout(dev_priv->xhw_caller_queue,
				 atomic_read(&buf->done), DRM_HZ * 3);

	if (!atomic_read(&buf->done)) {
		psb_xhw_clean_buf(dev_priv, buf);
		return -EBUSY;
	}

	if (!xa->ret)
		*value = xa->arg.cl.value;

	return xa->ret;
}

static int psb_xhw_terminate(struct drm_psb_private *dev_priv,
			     struct psb_xhw_buf *buf)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;
	unsigned long irq_flags;

	buf->copy_back = 0;
	xa->op = PSB_XHW_TERMINATE;
	xa->issue_irq = 0;

	spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
	dev_priv->xhw_submit_ok = 0;
	atomic_set(&buf->done, 0);
	if (!list_empty(&buf->head)) {
		DRM_ERROR("Recursive list adding.\n");
		goto out;
	}
	list_add_tail(&buf->head, &dev_priv->xhw_in);
      out:
	spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);
	wake_up_interruptible(&dev_priv->xhw_queue);

	(void)wait_event_timeout(dev_priv->xhw_caller_queue,
				 atomic_read(&buf->done), DRM_HZ / 10);

	if (!atomic_read(&buf->done)) {
		DRM_ERROR("Xpsb terminate timeout.\n");
		psb_xhw_clean_buf(dev_priv, buf);
		return -EBUSY;
	}

	return 0;
}

int psb_xhw_ta_mem_info(struct drm_psb_private *dev_priv,
			struct psb_xhw_buf *buf,
			uint32_t pages, uint32_t * hw_cookie, uint32_t * size)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;
	int ret;

	buf->copy_back = 1;
	xa->op = PSB_XHW_TA_MEM_INFO;
	xa->issue_irq = 0;
	xa->irq_op = 0;
	xa->arg.bi.pages = pages;

	ret = psb_xhw_add(dev_priv, buf);
	if (ret)
		return ret;

	(void)wait_event_timeout(dev_priv->xhw_caller_queue,
				 atomic_read(&buf->done), DRM_HZ);

	if (!atomic_read(&buf->done)) {
		psb_xhw_clean_buf(dev_priv, buf);
		return -EBUSY;
	}

	if (!xa->ret)
		memcpy(hw_cookie, xa->cookie, sizeof(xa->cookie));

	*size = xa->arg.bi.size;
	return xa->ret;
}

int psb_xhw_ta_mem_load(struct drm_psb_private *dev_priv,
			struct psb_xhw_buf *buf,
			uint32_t flags,
			uint32_t param_offset,
			uint32_t pt_offset, uint32_t * hw_cookie)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;
	int ret;

	buf->copy_back = 1;
	xa->op = PSB_XHW_TA_MEM_LOAD;
	xa->issue_irq = 0;
	xa->irq_op = 0;
	xa->arg.bl.flags = flags;
	xa->arg.bl.param_offset = param_offset;
	xa->arg.bl.pt_offset = pt_offset;
	memcpy(xa->cookie, hw_cookie, sizeof(xa->cookie));

	ret = psb_xhw_add(dev_priv, buf);
	if (ret)
		return ret;

	(void)wait_event_timeout(dev_priv->xhw_caller_queue,
				 atomic_read(&buf->done), 3 * DRM_HZ);

	if (!atomic_read(&buf->done)) {
		psb_xhw_clean_buf(dev_priv, buf);
		return -EBUSY;
	}

	if (!xa->ret)
		memcpy(hw_cookie, xa->cookie, sizeof(xa->cookie));

	return xa->ret;
}

int psb_xhw_ta_oom(struct drm_psb_private *dev_priv,
		   struct psb_xhw_buf *buf, uint32_t * cookie)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;

	/*
	 * This calls the extensive closed source
	 * OOM handler, which resolves the condition and
	 * sends a reply telling the scheduler what to do
	 * with the task.
	 */

	buf->copy_back = 1;
	xa->op = PSB_XHW_OOM;
	xa->issue_irq = 1;
	xa->irq_op = PSB_UIRQ_OOM_REPLY;
	memcpy(xa->cookie, cookie, sizeof(xa->cookie));

	return psb_xhw_add(dev_priv, buf);
}

void psb_xhw_ta_oom_reply(struct drm_psb_private *dev_priv,
			  struct psb_xhw_buf *buf,
			  uint32_t * cookie,
			  uint32_t * bca, uint32_t * rca, uint32_t * flags)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;

	/*
	 * Get info about how to schedule an OOM task.
	 */

	memcpy(cookie, xa->cookie, sizeof(xa->cookie));
	*bca = xa->arg.oom.bca;
	*rca = xa->arg.oom.rca;
	*flags = xa->arg.oom.flags;
}

void psb_xhw_fire_reply(struct drm_psb_private *dev_priv,
			struct psb_xhw_buf *buf, uint32_t * cookie)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;

	memcpy(cookie, xa->cookie, sizeof(xa->cookie));
}

int psb_xhw_resume(struct drm_psb_private *dev_priv, struct psb_xhw_buf *buf)
{
	struct drm_psb_xhw_arg *xa = &buf->arg;

	buf->copy_back = 0;
	xa->op = PSB_XHW_RESUME;
	xa->issue_irq = 0;
	xa->irq_op = 0;
	return psb_xhw_add(dev_priv, buf);
}

void psb_xhw_takedown(struct drm_psb_private *dev_priv)
{
}

int psb_xhw_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	unsigned long irq_flags;

	INIT_LIST_HEAD(&dev_priv->xhw_in);
	dev_priv->xhw_lock = SPIN_LOCK_UNLOCKED;
	atomic_set(&dev_priv->xhw_client, 0);
	init_waitqueue_head(&dev_priv->xhw_queue);
	init_waitqueue_head(&dev_priv->xhw_caller_queue);
	mutex_init(&dev_priv->xhw_mutex);
	spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
	dev_priv->xhw_on = 0;
	spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);

	return 0;
}

static int psb_xhw_init_init(struct drm_device *dev,
			     struct drm_file *file_priv,
			     struct drm_psb_xhw_init_arg *arg)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	int ret;
	int is_iomem;

	if (atomic_add_unless(&dev_priv->xhw_client, 1, 1)) {
		unsigned long irq_flags;

		mutex_lock(&dev->struct_mutex);
		dev_priv->xhw_bo =
		    drm_lookup_buffer_object(file_priv, arg->buffer_handle, 1);
		mutex_unlock(&dev->struct_mutex);
		if (!dev_priv->xhw_bo) {
			ret = -EINVAL;
			goto out_err;
		}
		ret = drm_bo_kmap(dev_priv->xhw_bo, 0,
				  dev_priv->xhw_bo->num_pages,
				  &dev_priv->xhw_kmap);
		if (ret) {
			DRM_ERROR("Failed mapping X server "
				  "communications buffer.\n");
			goto out_err0;
		}
		dev_priv->xhw = drm_bmo_virtual(&dev_priv->xhw_kmap, &is_iomem);
		if (is_iomem) {
			DRM_ERROR("X server communications buffer"
				  "is in device memory.\n");
			ret = -EINVAL;
			goto out_err1;
		}
		dev_priv->xhw_file = file_priv;

		spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
		dev_priv->xhw_on = 1;
		dev_priv->xhw_submit_ok = 1;
		spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);

		return 0;
	} else {
		DRM_ERROR("Xhw is already initialized.\n");
		return -EBUSY;
	}
      out_err1:
	dev_priv->xhw = NULL;
	drm_bo_kunmap(&dev_priv->xhw_kmap);
      out_err0:
	drm_bo_usage_deref_unlocked(&dev_priv->xhw_bo);
      out_err:
	atomic_dec(&dev_priv->xhw_client);
	return ret;
}

static void psb_xhw_queue_empty(struct drm_psb_private *dev_priv)
{
	struct psb_xhw_buf *cur_buf, *next;
	unsigned long irq_flags;

	spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
	dev_priv->xhw_submit_ok = 0;

	list_for_each_entry_safe(cur_buf, next, &dev_priv->xhw_in, head) {
		list_del_init(&cur_buf->head);
		if (cur_buf->copy_back) {
			cur_buf->arg.ret = -EINVAL;
		}
		atomic_set(&cur_buf->done, 1);
	}
	spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);
	wake_up(&dev_priv->xhw_caller_queue);
}

void psb_xhw_init_takedown(struct drm_psb_private *dev_priv,
			   struct drm_file *file_priv, int closing)
{

	if (dev_priv->xhw_file == file_priv &&
	    atomic_add_unless(&dev_priv->xhw_client, -1, 0)) {

		if (closing)
			psb_xhw_queue_empty(dev_priv);
		else {
			struct psb_xhw_buf buf;
			INIT_LIST_HEAD(&buf.head);

			psb_xhw_terminate(dev_priv, &buf);
			psb_xhw_queue_empty(dev_priv);
		}

		dev_priv->xhw = NULL;
		drm_bo_kunmap(&dev_priv->xhw_kmap);
		drm_bo_usage_deref_unlocked(&dev_priv->xhw_bo);
		dev_priv->xhw_file = NULL;
	}
}

int psb_xhw_init_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_psb_xhw_init_arg *arg = (struct drm_psb_xhw_init_arg *)data;
	struct drm_psb_private *dev_priv = 
		(struct drm_psb_private *)dev->dev_private;

	switch (arg->operation) {
	case PSB_XHW_INIT:
		return psb_xhw_init_init(dev, file_priv, arg);
	case PSB_XHW_TAKEDOWN:
		psb_xhw_init_takedown(dev_priv, file_priv, 0);
	}
	return 0;
}

static int psb_xhw_in_empty(struct drm_psb_private *dev_priv)
{
	int empty;
	unsigned long irq_flags;

	spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
	empty = list_empty(&dev_priv->xhw_in);
	spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);
	return empty;
}

int psb_xhw_handler(struct drm_psb_private *dev_priv)
{
	unsigned long irq_flags;
	struct drm_psb_xhw_arg *xa;
	struct psb_xhw_buf *buf;

	spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);

	if (!dev_priv->xhw_on) {
		spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);
		return -EINVAL;
	}

	buf = dev_priv->xhw_cur_buf;
	if (buf && buf->copy_back) {
		xa = &buf->arg;
		memcpy(xa, dev_priv->xhw, sizeof(*xa));
		dev_priv->comm[PSB_COMM_USER_IRQ] = xa->irq_op;
		atomic_set(&buf->done, 1);
		wake_up(&dev_priv->xhw_caller_queue);
	} else
		dev_priv->comm[PSB_COMM_USER_IRQ] = 0;

	dev_priv->xhw_cur_buf = 0;
	spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);
	return 0;
}

int psb_xhw_ioctl(struct drm_device *dev, void *data,
		  struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	unsigned long irq_flags;
	struct drm_psb_xhw_arg *xa;
	int ret;
	struct list_head *list;
	struct psb_xhw_buf *buf;

	if (!dev_priv)
		return -EINVAL;

	if (mutex_lock_interruptible(&dev_priv->xhw_mutex))
		return -EAGAIN;

	if (psb_forced_user_interrupt(dev_priv)) {
		mutex_unlock(&dev_priv->xhw_mutex);
		return -EINVAL;
	}

	spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
	while (list_empty(&dev_priv->xhw_in)) {
		spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);
		ret = wait_event_interruptible_timeout(dev_priv->xhw_queue,
						       !psb_xhw_in_empty
						       (dev_priv), DRM_HZ);
		if (ret == -ERESTARTSYS || ret == 0) {
			mutex_unlock(&dev_priv->xhw_mutex);
			return -EAGAIN;
		}
		spin_lock_irqsave(&dev_priv->xhw_lock, irq_flags);
	}

	list = dev_priv->xhw_in.next;
	list_del_init(list);

	buf = list_entry(list, struct psb_xhw_buf, head);
	xa = &buf->arg;
	memcpy(dev_priv->xhw, xa, sizeof(*xa));

	if (unlikely(buf->copy_back))
		dev_priv->xhw_cur_buf = buf;
	else {
		atomic_set(&buf->done, 1);
		dev_priv->xhw_cur_buf = NULL;
	}

	if (xa->op == PSB_XHW_TERMINATE) {
		dev_priv->xhw_on = 0;
		wake_up(&dev_priv->xhw_caller_queue);
	}
	spin_unlock_irqrestore(&dev_priv->xhw_lock, irq_flags);

	mutex_unlock(&dev_priv->xhw_mutex);

	return 0;
}
