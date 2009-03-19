/**************************************************************************
 * Copyright (c) 2007, Intel Corporation.
 * All Rights Reserved.
 * Copyright (c) 2008, Tungsten Graphics, Inc. Cedar Park, TX. USA.
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
 */

#include <drm/drmP.h>
#include "psb_drv.h"
#include "psb_drm.h"
#include "psb_reg.h"
#include "psb_scene.h"
#include "psb_msvdx.h"
#include "lnc_topaz.h"
#include "ttm/ttm_bo_api.h"
#include "ttm/ttm_execbuf_util.h"
#include "ttm/ttm_userobj_api.h"
#include "ttm/ttm_placement_common.h"
#include "psb_sgx.h"

static inline int psb_same_page(unsigned long offset,
				unsigned long offset2)
{
	return (offset & PAGE_MASK) == (offset2 & PAGE_MASK);
}

static inline unsigned long psb_offset_end(unsigned long offset,
					      unsigned long end)
{
	offset = (offset + PAGE_SIZE) & PAGE_MASK;
	return (end < offset) ? end : offset;
}

static void psb_idle_engine(struct drm_device *dev, int engine);

struct psb_dstbuf_cache {
	unsigned int dst;
	struct ttm_buffer_object *dst_buf;
	unsigned long dst_offset;
	uint32_t *dst_page;
	unsigned int dst_page_offset;
	struct ttm_bo_kmap_obj dst_kmap;
	bool dst_is_iomem;
};

struct psb_validate_buffer {
	struct ttm_validate_buffer base;
	struct psb_validate_req req;
	int ret;
	struct psb_validate_arg __user *user_val_arg;
	uint32_t flags;
	uint32_t offset;
	int po_correct;
};



#define PSB_REG_GRAN_SHIFT 2
#define PSB_REG_GRANULARITY (1 << PSB_REG_GRAN_SHIFT)
#define PSB_MAX_REG 0x1000

static const uint32_t disallowed_ranges[][2] = {
	{0x0000, 0x0200},
	{0x0208, 0x0214},
	{0x021C, 0x0224},
	{0x0230, 0x0234},
	{0x0248, 0x024C},
	{0x0254, 0x0358},
	{0x0428, 0x0428},
	{0x0430, 0x043C},
	{0x0498, 0x04B4},
	{0x04CC, 0x04D8},
	{0x04E0, 0x07FC},
	{0x0804, 0x0A14},
	{0x0A4C, 0x0A58},
	{0x0A68, 0x0A80},
	{0x0AA0, 0x0B1C},
	{0x0B2C, 0x0CAC},
	{0x0CB4, PSB_MAX_REG - PSB_REG_GRANULARITY}
};

static uint32_t psb_disallowed_regs[PSB_MAX_REG /
				    (PSB_REG_GRANULARITY *
				     (sizeof(uint32_t) << 3))];

static inline int psb_disallowed(uint32_t reg)
{
	reg >>= PSB_REG_GRAN_SHIFT;
	return (psb_disallowed_regs[reg >> 5] & (1 << (reg & 31))) != 0;
}

void psb_init_disallowed(void)
{
	int i;
	uint32_t reg, tmp;
	static int initialized;

	if (initialized)
		return;

	initialized = 1;
	memset(psb_disallowed_regs, 0, sizeof(psb_disallowed_regs));

	for (i = 0;
	     i < (sizeof(disallowed_ranges) / (2 * sizeof(uint32_t)));
	     ++i) {
		for (reg = disallowed_ranges[i][0];
		     reg <= disallowed_ranges[i][1]; reg += 4) {
			tmp = reg >> 2;
			psb_disallowed_regs[tmp >> 5] |= (1 << (tmp & 31));
		}
	}
}

static int psb_memcpy_check(uint32_t *dst, const uint32_t *src,
			    uint32_t size)
{
	size >>= 3;
	while (size--) {
		if (unlikely((*src >= 0x1000) || psb_disallowed(*src))) {
			DRM_ERROR("Forbidden SGX register access: "
				  "0x%04x.\n", *src);
			return -EPERM;
		}
		*dst++ = *src++;
		*dst++ = *src++;
	}
	return 0;
}

int psb_2d_wait_available(struct drm_psb_private *dev_priv,
			  unsigned size)
{
	uint32_t avail = PSB_RSGX32(PSB_CR_2D_SOCIF);
	int ret = 0;

retry:
	if (avail < size) {
#if 0
		/* We'd ideally
		 * like to have an IRQ-driven event here.
		 */

		psb_2D_irq_on(dev_priv);
		DRM_WAIT_ON(ret, dev_priv->event_2d_queue, DRM_HZ,
			    ((avail =
			      PSB_RSGX32(PSB_CR_2D_SOCIF)) >= size));
		psb_2D_irq_off(dev_priv);
		if (ret == 0)
			return 0;
		if (ret == -EINTR) {
			ret = 0;
			goto retry;
		}
#else
		avail = PSB_RSGX32(PSB_CR_2D_SOCIF);
		goto retry;
#endif
	}
	return ret;
}

int psb_2d_submit(struct drm_psb_private *dev_priv, uint32_t *cmdbuf,
		  unsigned size)
{
	int ret = 0;
	int i;
	unsigned submit_size;

	while (size > 0) {
		submit_size = (size < 0x60) ? size : 0x60;
		size -= submit_size;
		ret = psb_2d_wait_available(dev_priv, submit_size);
		if (ret)
			return ret;

		submit_size <<= 2;
		mutex_lock(&dev_priv->reset_mutex);
		for (i = 0; i < submit_size; i += 4) {
			PSB_WSGX32(*cmdbuf++, PSB_SGX_2D_SLAVE_PORT + i);
		}
		(void)PSB_RSGX32(PSB_SGX_2D_SLAVE_PORT + i - 4);
		mutex_unlock(&dev_priv->reset_mutex);
	}
	return 0;
}

int psb_blit_sequence(struct drm_psb_private *dev_priv, uint32_t sequence)
{
	uint32_t buffer[8];
	uint32_t *bufp = buffer;
	int ret;

	*bufp++ = PSB_2D_FENCE_BH;

	*bufp++ = PSB_2D_DST_SURF_BH |
	    PSB_2D_DST_8888ARGB | (4 << PSB_2D_DST_STRIDE_SHIFT);
	*bufp++ = dev_priv->comm_mmu_offset - dev_priv->mmu_2d_offset;

	*bufp++ = PSB_2D_BLIT_BH |
	    PSB_2D_ROT_NONE |
	    PSB_2D_COPYORDER_TL2BR |
	    PSB_2D_DSTCK_DISABLE |
	    PSB_2D_SRCCK_DISABLE | PSB_2D_USE_FILL | PSB_2D_ROP3_PATCOPY;

	*bufp++ = sequence << PSB_2D_FILLCOLOUR_SHIFT;
	*bufp++ = (0 << PSB_2D_DST_XSTART_SHIFT) |
	    (0 << PSB_2D_DST_YSTART_SHIFT);
	*bufp++ =
	    (1 << PSB_2D_DST_XSIZE_SHIFT) | (1 << PSB_2D_DST_YSIZE_SHIFT);

	*bufp++ = PSB_2D_FLUSH_BH;

	psb_2d_lock(dev_priv);
	ret = psb_2d_submit(dev_priv, buffer, bufp - buffer);
	psb_2d_unlock(dev_priv);

	if (!ret)
		psb_schedule_watchdog(dev_priv);
	return ret;
}

int psb_emit_2d_copy_blit(struct drm_device *dev,
			  uint32_t src_offset,
			  uint32_t dst_offset, uint32_t pages,
			  int direction)
{
	uint32_t cur_pages;
	struct drm_psb_private *dev_priv = dev->dev_private;
	uint32_t buf[10];
	uint32_t *bufp;
	uint32_t xstart;
	uint32_t ystart;
	uint32_t blit_cmd;
	uint32_t pg_add;
	int ret = 0;

	if (!dev_priv)
		return 0;

	if (direction) {
		pg_add = (pages - 1) << PAGE_SHIFT;
		src_offset += pg_add;
		dst_offset += pg_add;
	}

	blit_cmd = PSB_2D_BLIT_BH |
	    PSB_2D_ROT_NONE |
	    PSB_2D_DSTCK_DISABLE |
	    PSB_2D_SRCCK_DISABLE |
	    PSB_2D_USE_PAT |
	    PSB_2D_ROP3_SRCCOPY |
	    (direction ? PSB_2D_COPYORDER_BR2TL : PSB_2D_COPYORDER_TL2BR);
	xstart = (direction) ? ((PAGE_SIZE - 1) >> 2) : 0;

	psb_2d_lock(dev_priv);
	while (pages > 0) {
		cur_pages = pages;
		if (cur_pages > 2048)
			cur_pages = 2048;
		pages -= cur_pages;
		ystart = (direction) ? cur_pages - 1 : 0;

		bufp = buf;
		*bufp++ = PSB_2D_FENCE_BH;

		*bufp++ = PSB_2D_DST_SURF_BH | PSB_2D_DST_8888ARGB |
		    (PAGE_SIZE << PSB_2D_DST_STRIDE_SHIFT);
		*bufp++ = dst_offset;
		*bufp++ = PSB_2D_SRC_SURF_BH | PSB_2D_SRC_8888ARGB |
		    (PAGE_SIZE << PSB_2D_SRC_STRIDE_SHIFT);
		*bufp++ = src_offset;
		*bufp++ =
		    PSB_2D_SRC_OFF_BH | (xstart <<
					 PSB_2D_SRCOFF_XSTART_SHIFT) |
		    (ystart << PSB_2D_SRCOFF_YSTART_SHIFT);
		*bufp++ = blit_cmd;
		*bufp++ = (xstart << PSB_2D_DST_XSTART_SHIFT) |
		    (ystart << PSB_2D_DST_YSTART_SHIFT);
		*bufp++ = ((PAGE_SIZE >> 2) << PSB_2D_DST_XSIZE_SHIFT) |
		    (cur_pages << PSB_2D_DST_YSIZE_SHIFT);

		ret = psb_2d_submit(dev_priv, buf, bufp - buf);
		if (ret)
			goto out;
		pg_add =
		    (cur_pages << PAGE_SHIFT) * ((direction) ? -1 : 1);
		src_offset += pg_add;
		dst_offset += pg_add;
	}
out:
	psb_2d_unlock(dev_priv);
	return ret;
}

void psb_init_2d(struct drm_psb_private *dev_priv)
{
	spin_lock_init(&dev_priv->sequence_lock);
	psb_reset(dev_priv, 1);
	dev_priv->mmu_2d_offset = dev_priv->pg->gatt_start;
	PSB_WSGX32(dev_priv->mmu_2d_offset, PSB_CR_BIF_TWOD_REQ_BASE);
	(void) PSB_RSGX32(PSB_CR_BIF_TWOD_REQ_BASE);
}

int psb_idle_2d(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned long _end = jiffies + DRM_HZ;
	int busy = 0;

	/*
	 * First idle the 2D engine.
	 */

	if (dev_priv->engine_lockup_2d)
		return -EBUSY;

	if ((PSB_RSGX32(PSB_CR_2D_SOCIF) == _PSB_C2_SOCIF_EMPTY) &&
	    ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) & _PSB_C2B_STATUS_BUSY) ==
	     0))
		goto out;

	do {
		busy =
		    (PSB_RSGX32(PSB_CR_2D_SOCIF) != _PSB_C2_SOCIF_EMPTY);
	} while (busy && !time_after_eq(jiffies, _end));

	if (busy)
		busy =
		    (PSB_RSGX32(PSB_CR_2D_SOCIF) != _PSB_C2_SOCIF_EMPTY);
	if (busy)
		goto out;

	do {
		busy =
		    ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) &
		      _PSB_C2B_STATUS_BUSY)
		     != 0);
	} while (busy && !time_after_eq(jiffies, _end));
	if (busy)
		busy =
		    ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) &
		      _PSB_C2B_STATUS_BUSY)
		     != 0);

out:
	if (busy)
		dev_priv->engine_lockup_2d = 1;

	return (busy) ? -EBUSY : 0;
}

int psb_idle_3d(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	int ret;

	ret = wait_event_timeout(scheduler->idle_queue,
				 psb_scheduler_finished(dev_priv),
				 DRM_HZ * 10);

	return (ret < 1) ? -EBUSY : 0;
}

static int psb_check_presumed(struct psb_validate_req *req,
			      struct ttm_buffer_object *bo,
			      struct psb_validate_arg __user *data,
			      int *presumed_ok)
{
	struct psb_validate_req __user *user_req = &(data->d.req);

	*presumed_ok = 0;

	if (bo->mem.mem_type == TTM_PL_SYSTEM) {
		*presumed_ok = 1;
		return 0;
	}

	if (unlikely(!(req->presumed_flags & PSB_USE_PRESUMED)))
		return 0;

	if (bo->offset == req->presumed_gpu_offset) {
		*presumed_ok = 1;
		return 0;
	}

	return __put_user(req->presumed_flags & ~PSB_USE_PRESUMED,
			  &user_req->presumed_flags);
}


static void psb_unreference_buffers(struct psb_context *context)
{
	struct ttm_validate_buffer *entry, *next;
	struct psb_validate_buffer *vbuf;
	struct list_head *list = &context->validate_list;

	list_for_each_entry_safe(entry, next, list, head) {
		vbuf =
		    container_of(entry, struct psb_validate_buffer, base);
		list_del(&entry->head);
		ttm_bo_unref(&entry->bo);
	}

	list = &context->kern_validate_list;

	list_for_each_entry_safe(entry, next, list, head) {
		vbuf =
		    container_of(entry, struct psb_validate_buffer, base);
		list_del(&entry->head);
		ttm_bo_unref(&entry->bo);
	}
}


static int psb_lookup_validate_buffer(struct drm_file *file_priv,
				      uint64_t data,
				      struct psb_validate_buffer *item)
{
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;

	item->user_val_arg =
	    (struct psb_validate_arg __user *) (unsigned long) data;

	if (unlikely(copy_from_user(&item->req, &item->user_val_arg->d.req,
				    sizeof(item->req)) != 0)) {
		DRM_ERROR("Lookup copy fault.\n");
		return -EFAULT;
	}

	item->base.bo =
	    ttm_buffer_object_lookup(tfile, item->req.buffer_handle);

	if (unlikely(item->base.bo == NULL)) {
		DRM_ERROR("Bo lookup fault.\n");
		return -EINVAL;
	}

	return 0;
}

static int psb_reference_buffers(struct drm_file *file_priv,
				 uint64_t data,
				 struct psb_context *context)
{
	struct psb_validate_buffer *item;
	int ret;

	while (likely(data != 0)) {
		if (unlikely(context->used_buffers >=
			     PSB_NUM_VALIDATE_BUFFERS)) {
			DRM_ERROR("Too many buffers "
				  "on validate list.\n");
			ret = -EINVAL;
			goto out_err0;
		}

		item = &context->buffers[context->used_buffers];

		ret = psb_lookup_validate_buffer(file_priv, data, item);
		if (unlikely(ret != 0))
			goto out_err0;

		item->base.reserved = 0;
		list_add_tail(&item->base.head, &context->validate_list);
		context->used_buffers++;
		data = item->req.next;
	}
	return 0;

out_err0:
	psb_unreference_buffers(context);
	return ret;
}

static int
psb_placement_fence_type(struct ttm_buffer_object *bo,
			 uint64_t set_val_flags,
			 uint64_t clr_val_flags,
			 uint32_t new_fence_class,
			 uint32_t *new_fence_type)
{
	int ret;
	uint32_t n_fence_type;
	uint32_t set_flags = set_val_flags & 0xFFFFFFFF;
	uint32_t clr_flags = clr_val_flags & 0xFFFFFFFF;
	struct ttm_fence_object *old_fence;
	uint32_t old_fence_type;

	if (unlikely
	    (!(set_val_flags &
	       (PSB_GPU_ACCESS_READ | PSB_GPU_ACCESS_WRITE)))) {
		DRM_ERROR
		    ("GPU access type (read / write) is not indicated.\n");
		return -EINVAL;
	}

	ret = ttm_bo_check_placement(bo, set_flags, clr_flags);
	if (unlikely(ret != 0))
		return ret;

	switch (new_fence_class) {
	case PSB_ENGINE_TA:
		n_fence_type = _PSB_FENCE_TYPE_EXE |
		    _PSB_FENCE_TYPE_TA_DONE | _PSB_FENCE_TYPE_RASTER_DONE;
		if (set_val_flags & PSB_BO_FLAG_TA)
			n_fence_type &= ~_PSB_FENCE_TYPE_RASTER_DONE;
		if (set_val_flags & PSB_BO_FLAG_COMMAND)
			n_fence_type &=
			    ~(_PSB_FENCE_TYPE_RASTER_DONE |
			      _PSB_FENCE_TYPE_TA_DONE);
		if (set_val_flags & PSB_BO_FLAG_SCENE)
			n_fence_type |= _PSB_FENCE_TYPE_SCENE_DONE;
		if (set_val_flags & PSB_BO_FLAG_FEEDBACK)
			n_fence_type |= _PSB_FENCE_TYPE_FEEDBACK;
		break;
	default:
		n_fence_type = _PSB_FENCE_TYPE_EXE;
	}

	*new_fence_type = n_fence_type;
	old_fence = (struct ttm_fence_object *) bo->sync_obj;
	old_fence_type = (uint32_t) (unsigned long) bo->sync_obj_arg;

	if (old_fence && ((new_fence_class != old_fence->fence_class) ||
			  ((n_fence_type ^ old_fence_type) &
			   old_fence_type))) {
		ret = ttm_bo_wait(bo, 0, 1, 0);
		if (unlikely(ret != 0))
			return ret;
	}

	bo->proposed_flags = (bo->proposed_flags | set_flags)
		& ~clr_flags & TTM_PL_MASK_MEMTYPE;

	return 0;
}

int psb_validate_kernel_buffer(struct psb_context *context,
			       struct ttm_buffer_object *bo,
			       uint32_t fence_class,
			       uint64_t set_flags, uint64_t clr_flags)
{
	struct psb_validate_buffer *item;
	uint32_t cur_fence_type;
	int ret;

	if (unlikely(context->used_buffers >= PSB_NUM_VALIDATE_BUFFERS)) {
		DRM_ERROR("Out of free validation buffer entries for "
			  "kernel buffer validation.\n");
		return -ENOMEM;
	}

	item = &context->buffers[context->used_buffers];
	item->user_val_arg = NULL;
	item->base.reserved = 0;

	ret = ttm_bo_reserve(bo, 1, 0, 1, context->val_seq);
	if (unlikely(ret != 0))
		goto out_unlock;

	mutex_lock(&bo->mutex);
	ret = psb_placement_fence_type(bo, set_flags, clr_flags, fence_class,
				       &cur_fence_type);
	if (unlikely(ret != 0)) {
		ttm_bo_unreserve(bo);
		goto out_unlock;
	}

	item->base.bo = ttm_bo_reference(bo);
	item->base.new_sync_obj_arg = (void *) (unsigned long) cur_fence_type;
	item->base.reserved = 1;

	list_add_tail(&item->base.head, &context->kern_validate_list);
	context->used_buffers++;

	ret = ttm_buffer_object_validate(bo, 1, 0);
	if (unlikely(ret != 0))
		goto out_unlock;

	item->offset = bo->offset;
	item->flags = bo->mem.flags;
	context->fence_types |= cur_fence_type;

out_unlock:
	mutex_unlock(&bo->mutex);
	return ret;
}


static int psb_validate_buffer_list(struct drm_file *file_priv,
				    uint32_t fence_class,
				    struct psb_context *context,
				    int *po_correct)
{
	struct psb_validate_buffer *item;
	struct ttm_buffer_object *bo;
	int ret;
	struct psb_validate_req *req;
	uint32_t fence_types = 0;
	uint32_t cur_fence_type;
	struct ttm_validate_buffer *entry;
	struct list_head *list = &context->validate_list;

	*po_correct = 1;

	list_for_each_entry(entry, list, head) {
		item =
		    container_of(entry, struct psb_validate_buffer, base);
		bo = entry->bo;
		item->ret = 0;
		req = &item->req;

		mutex_lock(&bo->mutex);
		ret = psb_placement_fence_type(bo,
					       req->set_flags,
					       req->clear_flags,
					       fence_class,
					       &cur_fence_type);
		if (unlikely(ret != 0))
			goto out_err;

		ret = ttm_buffer_object_validate(bo, 1, 0);

		if (unlikely(ret != 0))
			goto out_err;

		fence_types |= cur_fence_type;
		entry->new_sync_obj_arg = (void *)
			(unsigned long) cur_fence_type;

		item->offset = bo->offset;
		item->flags = bo->mem.flags;
		mutex_unlock(&bo->mutex);

		ret =
		    psb_check_presumed(&item->req, bo, item->user_val_arg,
				       &item->po_correct);
		if (unlikely(ret != 0))
			goto out_err;

		if (unlikely(!item->po_correct))
			*po_correct = 0;

		item++;
	}

	context->fence_types |= fence_types;

	return 0;
out_err:
	mutex_unlock(&bo->mutex);
	item->ret = ret;
	return ret;
}


int
psb_reg_submit(struct drm_psb_private *dev_priv, uint32_t *regs,
	       unsigned int cmds)
{
	int i;

	/*
	 * cmds is 32-bit words.
	 */

	cmds >>= 1;
	for (i = 0; i < cmds; ++i) {
		PSB_WSGX32(regs[1], regs[0]);
		regs += 2;
	}
	wmb();
	return 0;
}

/*
 * Security: Block user-space writing to MMU mapping registers.
 * This is important for security and brings Poulsbo DRM
 * up to par with the other DRM drivers. Using this,
 * user-space should not be able to map arbitrary memory
 * pages to graphics memory, but all user-space processes
 * basically have access to all buffer objects mapped to
 * graphics memory.
 */

int
psb_submit_copy_cmdbuf(struct drm_device *dev,
		       struct ttm_buffer_object *cmd_buffer,
		       unsigned long cmd_offset,
		       unsigned long cmd_size,
		       int engine, uint32_t *copy_buffer)
{
	unsigned long cmd_end = cmd_offset + (cmd_size << 2);
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned long cmd_page_offset =
	    cmd_offset - (cmd_offset & PAGE_MASK);
	unsigned long cmd_next;
	struct ttm_bo_kmap_obj cmd_kmap;
	uint32_t *cmd_page;
	unsigned cmds;
	bool is_iomem;
	int ret = 0;

	if (cmd_size == 0)
		return 0;

	if (engine == PSB_ENGINE_2D)
		psb_2d_lock(dev_priv);

	do {
		cmd_next = psb_offset_end(cmd_offset, cmd_end);
		ret = ttm_bo_kmap(cmd_buffer, cmd_offset >> PAGE_SHIFT,
				  1, &cmd_kmap);

		if (ret) {
			if (engine == PSB_ENGINE_2D)
				psb_2d_unlock(dev_priv);
			return ret;
		}
		cmd_page = ttm_kmap_obj_virtual(&cmd_kmap, &is_iomem);
		cmd_page_offset = (cmd_offset & ~PAGE_MASK) >> 2;
		cmds = (cmd_next - cmd_offset) >> 2;

		switch (engine) {
		case PSB_ENGINE_2D:
			ret =
			    psb_2d_submit(dev_priv,
					  cmd_page + cmd_page_offset,
					  cmds);
			break;
		case PSB_ENGINE_RASTERIZER:
		case PSB_ENGINE_TA:
		case PSB_ENGINE_HPRAST:
			PSB_DEBUG_GENERAL("Reg copy.\n");
			ret = psb_memcpy_check(copy_buffer,
					       cmd_page + cmd_page_offset,
					       cmds * sizeof(uint32_t));
			copy_buffer += cmds;
			break;
		default:
			ret = -EINVAL;
		}
		ttm_bo_kunmap(&cmd_kmap);
		if (ret)
			break;
	} while (cmd_offset = cmd_next, cmd_offset != cmd_end);

	if (engine == PSB_ENGINE_2D)
		psb_2d_unlock(dev_priv);

	return ret;
}

static void psb_clear_dstbuf_cache(struct psb_dstbuf_cache *dst_cache)
{
	if (dst_cache->dst_page) {
		ttm_bo_kunmap(&dst_cache->dst_kmap);
		dst_cache->dst_page = NULL;
	}
	dst_cache->dst_buf = NULL;
	dst_cache->dst = ~0;
}

static int psb_update_dstbuf_cache(struct psb_dstbuf_cache *dst_cache,
				   struct psb_validate_buffer *buffers,
				   unsigned int dst,
				   unsigned long dst_offset)
{
	int ret;

	PSB_DEBUG_GENERAL("Destination buffer is %d.\n", dst);

	if (unlikely(dst != dst_cache->dst || NULL == dst_cache->dst_buf)) {
		psb_clear_dstbuf_cache(dst_cache);
		dst_cache->dst = dst;
		dst_cache->dst_buf = buffers[dst].base.bo;
	}

	if (unlikely
	    (dst_offset > dst_cache->dst_buf->num_pages * PAGE_SIZE)) {
		DRM_ERROR("Relocation destination out of bounds.\n");
		return -EINVAL;
	}

	if (!psb_same_page(dst_cache->dst_offset, dst_offset) ||
	    NULL == dst_cache->dst_page) {
		if (NULL != dst_cache->dst_page) {
			ttm_bo_kunmap(&dst_cache->dst_kmap);
			dst_cache->dst_page = NULL;
		}

		ret =
		    ttm_bo_kmap(dst_cache->dst_buf,
				dst_offset >> PAGE_SHIFT, 1,
				&dst_cache->dst_kmap);
		if (ret) {
			DRM_ERROR("Could not map destination buffer for "
				  "relocation.\n");
			return ret;
		}

		dst_cache->dst_page =
		    ttm_kmap_obj_virtual(&dst_cache->dst_kmap,
					 &dst_cache->dst_is_iomem);
		dst_cache->dst_offset = dst_offset & PAGE_MASK;
		dst_cache->dst_page_offset = dst_cache->dst_offset >> 2;
	}
	return 0;
}

static int psb_apply_reloc(struct drm_psb_private *dev_priv,
			   uint32_t fence_class,
			   const struct drm_psb_reloc *reloc,
			   struct psb_validate_buffer *buffers,
			   int num_buffers,
			   struct psb_dstbuf_cache *dst_cache,
			   int no_wait, int interruptible)
{
	uint32_t val;
	uint32_t background;
	unsigned int index;
	int ret;
	unsigned int shift;
	unsigned int align_shift;
	struct ttm_buffer_object *reloc_bo;


	PSB_DEBUG_GENERAL("Reloc type %d\n"
			  "\t where 0x%04x\n"
			  "\t buffer 0x%04x\n"
			  "\t mask 0x%08x\n"
			  "\t shift 0x%08x\n"
			  "\t pre_add 0x%08x\n"
			  "\t background 0x%08x\n"
			  "\t dst_buffer 0x%08x\n"
			  "\t arg0 0x%08x\n"
			  "\t arg1 0x%08x\n",
			  reloc->reloc_op,
			  reloc->where,
			  reloc->buffer,
			  reloc->mask,
			  reloc->shift,
			  reloc->pre_add,
			  reloc->background,
			  reloc->dst_buffer, reloc->arg0, reloc->arg1);

	if (unlikely(reloc->buffer >= num_buffers)) {
		DRM_ERROR("Illegal relocation buffer %d.\n",
			  reloc->buffer);
		return -EINVAL;
	}

	if (buffers[reloc->buffer].po_correct)
		return 0;

	if (unlikely(reloc->dst_buffer >= num_buffers)) {
		DRM_ERROR
		    ("Illegal destination buffer for relocation %d.\n",
		     reloc->dst_buffer);
		return -EINVAL;
	}

	ret =
	    psb_update_dstbuf_cache(dst_cache, buffers, reloc->dst_buffer,
				    reloc->where << 2);
	if (ret)
		return ret;

	reloc_bo = buffers[reloc->buffer].base.bo;

	if (unlikely(reloc->pre_add > (reloc_bo->num_pages << PAGE_SHIFT))) {
		DRM_ERROR("Illegal relocation offset add.\n");
		return -EINVAL;
	}

	switch (reloc->reloc_op) {
	case PSB_RELOC_OP_OFFSET:
		val = reloc_bo->offset + reloc->pre_add;
		break;
	case PSB_RELOC_OP_2D_OFFSET:
		val = reloc_bo->offset + reloc->pre_add -
		    dev_priv->mmu_2d_offset;
		if (unlikely(val >= PSB_2D_SIZE)) {
			DRM_ERROR("2D relocation out of bounds\n");
			return -EINVAL;
		}
		break;
	case PSB_RELOC_OP_PDS_OFFSET:
		val =
		    reloc_bo->offset + reloc->pre_add - PSB_MEM_PDS_START;
		if (unlikely
		    (val >= (PSB_MEM_MMU_START - PSB_MEM_PDS_START))) {
			DRM_ERROR("PDS relocation out of bounds\n");
			return -EINVAL;
		}
		break;
	default:
		DRM_ERROR("Unimplemented relocation.\n");
		return -EINVAL;
	}

	shift =
	    (reloc->shift & PSB_RELOC_SHIFT_MASK) >> PSB_RELOC_SHIFT_SHIFT;
	align_shift =
	    (reloc->
	     shift & PSB_RELOC_ALSHIFT_MASK) >> PSB_RELOC_ALSHIFT_SHIFT;

	val = ((val >> align_shift) << shift);
	index = reloc->where - dst_cache->dst_page_offset;

	background = reloc->background;
	val = (background & ~reloc->mask) | (val & reloc->mask);
	dst_cache->dst_page[index] = val;

	PSB_DEBUG_GENERAL("Reloc buffer %d index 0x%08x, value 0x%08x\n",
			  reloc->dst_buffer, index,
			  dst_cache->dst_page[index]);

	return 0;
}

static int psb_ok_to_map_reloc(struct drm_psb_private *dev_priv,
			       unsigned int num_pages)
{
	int ret = 0;

	spin_lock(&dev_priv->reloc_lock);
	if (dev_priv->rel_mapped_pages + num_pages <= PSB_MAX_RELOC_PAGES) {
		dev_priv->rel_mapped_pages += num_pages;
		ret = 1;
	}
	spin_unlock(&dev_priv->reloc_lock);
	return ret;
}

static int psb_fixup_relocs(struct drm_file *file_priv,
			    uint32_t fence_class,
			    unsigned int num_relocs,
			    unsigned int reloc_offset,
			    uint32_t reloc_handle,
			    struct psb_context *context,
			    int no_wait, int interruptible)
{
	struct drm_device *dev = file_priv->minor->dev;
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_buffer_object *reloc_buffer = NULL;
	unsigned int reloc_num_pages;
	unsigned int reloc_first_page;
	unsigned int reloc_last_page;
	struct psb_dstbuf_cache dst_cache;
	struct drm_psb_reloc *reloc;
	struct ttm_bo_kmap_obj reloc_kmap;
	bool reloc_is_iomem;
	int count;
	int ret = 0;
	int registered = 0;
	uint32_t num_buffers = context->used_buffers;

	if (num_relocs == 0)
		return 0;

	memset(&dst_cache, 0, sizeof(dst_cache));
	memset(&reloc_kmap, 0, sizeof(reloc_kmap));

	reloc_buffer = ttm_buffer_object_lookup(tfile, reloc_handle);
	if (!reloc_buffer)
		goto out;

	if (unlikely(atomic_read(&reloc_buffer->reserved) != 1)) {
		DRM_ERROR("Relocation buffer was not on validate list.\n");
		ret = -EINVAL;
		goto out;
	}

	reloc_first_page = reloc_offset >> PAGE_SHIFT;
	reloc_last_page =
	    (reloc_offset +
	     num_relocs * sizeof(struct drm_psb_reloc)) >> PAGE_SHIFT;
	reloc_num_pages = reloc_last_page - reloc_first_page + 1;
	reloc_offset &= ~PAGE_MASK;

	if (reloc_num_pages > PSB_MAX_RELOC_PAGES) {
		DRM_ERROR("Relocation buffer is too large\n");
		ret = -EINVAL;
		goto out;
	}

	DRM_WAIT_ON(ret, dev_priv->rel_mapped_queue, 3 * DRM_HZ,
		    (registered =
		     psb_ok_to_map_reloc(dev_priv, reloc_num_pages)));

	if (ret == -EINTR) {
		ret = -ERESTART;
		goto out;
	}
	if (ret) {
		DRM_ERROR("Error waiting for space to map "
			  "relocation buffer.\n");
		goto out;
	}

	ret = ttm_bo_kmap(reloc_buffer, reloc_first_page,
			  reloc_num_pages, &reloc_kmap);

	if (ret) {
		DRM_ERROR("Could not map relocation buffer.\n"
			  "\tReloc buffer id 0x%08x.\n"
			  "\tReloc first page %d.\n"
			  "\tReloc num pages %d.\n",
			  reloc_handle, reloc_first_page, reloc_num_pages);
		goto out;
	}

	reloc = (struct drm_psb_reloc *)
	    ((unsigned long)
	     ttm_kmap_obj_virtual(&reloc_kmap,
				  &reloc_is_iomem) + reloc_offset);

	for (count = 0; count < num_relocs; ++count) {
		ret = psb_apply_reloc(dev_priv, fence_class,
				      reloc, context->buffers,
				      num_buffers, &dst_cache,
				      no_wait, interruptible);
		if (ret)
			goto out1;
		reloc++;
	}

out1:
	ttm_bo_kunmap(&reloc_kmap);
out:
	if (registered) {
		spin_lock(&dev_priv->reloc_lock);
		dev_priv->rel_mapped_pages -= reloc_num_pages;
		spin_unlock(&dev_priv->reloc_lock);
		DRM_WAKEUP(&dev_priv->rel_mapped_queue);
	}

	psb_clear_dstbuf_cache(&dst_cache);
	if (reloc_buffer)
		ttm_bo_unref(&reloc_buffer);
	return ret;
}

void psb_fence_or_sync(struct drm_file *file_priv,
		       uint32_t engine,
		       uint32_t fence_types,
		       uint32_t fence_flags,
		       struct list_head *list,
		       struct psb_ttm_fence_rep *fence_arg,
		       struct ttm_fence_object **fence_p)
{
	struct drm_device *dev = file_priv->minor->dev;
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_fence_device *fdev = &dev_priv->fdev;
	int ret;
	struct ttm_fence_object *fence;
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;
	uint32_t handle;

	ret = ttm_fence_user_create(fdev, tfile,
				    engine, fence_types,
				    TTM_FENCE_FLAG_EMIT, &fence, &handle);
	if (ret) {

		/*
		 * Fence creation failed.
		 * Fall back to synchronous operation and idle the engine.
		 */

		psb_idle_engine(dev, engine);
		if (!(fence_flags & DRM_PSB_FENCE_NO_USER)) {

			/*
			 * Communicate to user-space that
			 * fence creation has failed and that
			 * the engine is idle.
			 */

			fence_arg->handle = ~0;
			fence_arg->error = ret;
		}

		ttm_eu_backoff_reservation(list);
		if (fence_p)
			*fence_p = NULL;
		return;
	}

	ttm_eu_fence_buffer_objects(list, fence);
	if (!(fence_flags & DRM_PSB_FENCE_NO_USER)) {
		struct ttm_fence_info info = ttm_fence_get_info(fence);
		fence_arg->handle = handle;
		fence_arg->fence_class = ttm_fence_class(fence);
		fence_arg->fence_type = ttm_fence_types(fence);
		fence_arg->signaled_types = info.signaled_types;
		fence_arg->error = 0;
	} else {
		ret =
		    ttm_ref_object_base_unref(tfile, handle,
					      ttm_fence_type);
		BUG_ON(ret);
	}

	if (fence_p)
		*fence_p = fence;
	else if (fence)
		ttm_fence_object_unref(&fence);
}



static int psb_cmdbuf_2d(struct drm_file *priv,
			 struct list_head *validate_list,
			 uint32_t fence_type,
			 struct drm_psb_cmdbuf_arg *arg,
			 struct ttm_buffer_object *cmd_buffer,
			 struct psb_ttm_fence_rep *fence_arg)
{
	struct drm_device *dev = priv->minor->dev;
	int ret;

	ret = psb_submit_copy_cmdbuf(dev, cmd_buffer, arg->cmdbuf_offset,
				     arg->cmdbuf_size, PSB_ENGINE_2D,
				     NULL);
	if (ret)
		goto out_unlock;

	psb_fence_or_sync(priv, PSB_ENGINE_2D, fence_type,
			  arg->fence_flags, validate_list, fence_arg,
			  NULL);

	mutex_lock(&cmd_buffer->mutex);
	if (cmd_buffer->sync_obj != NULL)
		ttm_fence_sync_obj_unref(&cmd_buffer->sync_obj);
	mutex_unlock(&cmd_buffer->mutex);
out_unlock:
	return ret;
}

#if 0
static int psb_dump_page(struct ttm_buffer_object *bo,
			 unsigned int page_offset, unsigned int num)
{
	struct ttm_bo_kmap_obj kmobj;
	int is_iomem;
	uint32_t *p;
	int ret;
	unsigned int i;

	ret = ttm_bo_kmap(bo, page_offset, 1, &kmobj);
	if (ret)
		return ret;

	p = ttm_kmap_obj_virtual(&kmobj, &is_iomem);
	for (i = 0; i < num; ++i)
		PSB_DEBUG_GENERAL("0x%04x: 0x%08x\n", i, *p++);

	ttm_bo_kunmap(&kmobj);
	return 0;
}
#endif

static void psb_idle_engine(struct drm_device *dev, int engine)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	uint32_t dummy;
	unsigned long dummy2;

	switch (engine) {
	case PSB_ENGINE_2D:

		/*
		 * Make sure we flush 2D properly using a dummy
		 * fence sequence emit.
		 */

		(void) psb_fence_emit_sequence(&dev_priv->fdev,
					       PSB_ENGINE_2D, 0,
					       &dummy, &dummy2);
		psb_2d_lock(dev_priv);
		(void) psb_idle_2d(dev);
		psb_2d_unlock(dev_priv);
		break;
	case PSB_ENGINE_TA:
	case PSB_ENGINE_RASTERIZER:
	case PSB_ENGINE_HPRAST:
		(void) psb_idle_3d(dev);
		break;
	default:

		/*
		 * FIXME: Insert video engine idle command here.
		 */

		break;
	}
}

static int psb_handle_copyback(struct drm_device *dev,
			       struct psb_context *context,
			       int ret)
{
	int err = ret;
	struct ttm_validate_buffer *entry;
	struct psb_validate_arg arg;
	struct list_head *list = &context->validate_list;

	if (ret) {
		ttm_eu_backoff_reservation(list);
		ttm_eu_backoff_reservation(&context->kern_validate_list);
	}


	if (ret != -EAGAIN && ret != -EINTR && ret != -ERESTART) {
		list_for_each_entry(entry, list, head) {
			struct psb_validate_buffer *vbuf =
			    container_of(entry, struct psb_validate_buffer,
					 base);
			arg.handled = 1;
			arg.ret = vbuf->ret;
			if (!arg.ret) {
				struct ttm_buffer_object *bo = entry->bo;
				mutex_lock(&bo->mutex);
				arg.d.rep.gpu_offset = bo->offset;
				arg.d.rep.placement = bo->mem.flags;
				arg.d.rep.fence_type_mask =
					(uint32_t) (unsigned long)
					entry->new_sync_obj_arg;
				mutex_unlock(&bo->mutex);
			}

			if (__copy_to_user(vbuf->user_val_arg,
					   &arg, sizeof(arg)))
				err = -EFAULT;

			if (arg.ret)
				break;
		}
	}

	return err;
}


static int psb_cmdbuf_video(struct drm_file *priv,
			    struct list_head *validate_list,
			    uint32_t fence_type,
			    struct drm_psb_cmdbuf_arg *arg,
			    struct ttm_buffer_object *cmd_buffer,
			    struct psb_ttm_fence_rep *fence_arg)
{
	struct drm_device *dev = priv->minor->dev;
	struct ttm_fence_object *fence;
	int ret;

	/*
	 * Check this. Doesn't seem right. Have fencing done AFTER command
	 * submission and make sure drm_psb_idle idles the MSVDX completely.
	 */
	ret =
	    psb_submit_video_cmdbuf(dev, cmd_buffer, arg->cmdbuf_offset,
				    arg->cmdbuf_size, NULL);
	if (ret)
		return ret;


	/* DRM_ERROR("Intel: Fix video fencing!!\n"); */
	psb_fence_or_sync(priv, PSB_ENGINE_VIDEO, fence_type,
			  arg->fence_flags, validate_list, fence_arg,
			  &fence);


	ttm_fence_object_unref(&fence);
	mutex_lock(&cmd_buffer->mutex);
	if (cmd_buffer->sync_obj != NULL)
		ttm_fence_sync_obj_unref(&cmd_buffer->sync_obj);
	mutex_unlock(&cmd_buffer->mutex);
	return 0;
}

static int psb_feedback_buf(struct ttm_object_file *tfile,
			    struct psb_context *context,
			    uint32_t feedback_ops,
			    uint32_t handle,
			    uint32_t offset,
			    uint32_t feedback_breakpoints,
			    uint32_t feedback_size,
			    struct psb_feedback_info *feedback)
{
	struct ttm_buffer_object *bo;
	struct page *page;
	uint32_t page_no;
	uint32_t page_offset;
	int ret;

	if (feedback_ops & ~PSB_FEEDBACK_OP_VISTEST) {
		DRM_ERROR("Illegal feedback op.\n");
		return -EINVAL;
	}

	if (feedback_breakpoints != 0) {
		DRM_ERROR("Feedback breakpoints not implemented yet.\n");
		return -EINVAL;
	}

	if (feedback_size < PSB_HW_FEEDBACK_SIZE * sizeof(uint32_t)) {
		DRM_ERROR("Feedback buffer size too small.\n");
		return -EINVAL;
	}

	page_offset = offset & ~PAGE_MASK;
	if ((PAGE_SIZE - PSB_HW_FEEDBACK_SIZE * sizeof(uint32_t))
	    < page_offset) {
		DRM_ERROR("Illegal feedback buffer alignment.\n");
		return -EINVAL;
	}

	bo = ttm_buffer_object_lookup(tfile, handle);
	if (unlikely(bo == NULL)) {
		DRM_ERROR("Failed looking up feedback buffer.\n");
		return -EINVAL;
	}


	ret = psb_validate_kernel_buffer(context, bo,
					 PSB_ENGINE_TA,
					 TTM_PL_FLAG_SYSTEM |
					 TTM_PL_FLAG_CACHED |
					 PSB_GPU_ACCESS_WRITE |
					 PSB_BO_FLAG_FEEDBACK,
					 TTM_PL_MASK_MEM &
					 ~(TTM_PL_FLAG_SYSTEM |
					   TTM_PL_FLAG_CACHED));
	if (unlikely(ret != 0))
		goto out_unref;

	page_no = offset >> PAGE_SHIFT;
	if (unlikely(page_no >= bo->num_pages)) {
		ret = -EINVAL;
		DRM_ERROR("Illegal feedback buffer offset.\n");
		goto out_unref;
	}

	if (unlikely(bo->ttm == NULL)) {
		ret = -EINVAL;
		DRM_ERROR("Vistest buffer without TTM.\n");
		goto out_unref;
	}

	page = ttm_tt_get_page(bo->ttm, page_no);
	if (unlikely(page == NULL)) {
		ret = -ENOMEM;
		goto out_unref;
	}

	feedback->page = page;
	feedback->offset = page_offset;

	/*
	 * Note: bo referece transferred.
	 */

	feedback->bo = bo;
	return 0;

out_unref:
	ttm_bo_unref(&bo);
	return ret;
}

void psb_down_island_power(struct drm_device *dev, int islands)
{
	u32 pwr_cnt = 0;
	pwr_cnt = MSG_READ32(PSB_PUNIT_PORT, PSB_PWRGT_CNT);
	if (islands & PSB_GRAPHICS_ISLAND)
		pwr_cnt |= 0x3;
	if (islands & PSB_VIDEO_ENC_ISLAND)
		pwr_cnt |= 0x30;
	if (islands & PSB_VIDEO_DEC_ISLAND)
		pwr_cnt |= 0xc;
	MSG_WRITE32(PSB_PUNIT_PORT, PSB_PWRGT_CNT, pwr_cnt);
}
void psb_up_island_power(struct drm_device *dev, int islands)
{
	u32 pwr_cnt = 0;
	u32 count = 5;
	u32 pwr_sts = 0;
	u32 pwr_mask = 0;
	pwr_cnt = MSG_READ32(PSB_PUNIT_PORT, PSB_PWRGT_CNT);
	if (islands & PSB_GRAPHICS_ISLAND) {
		pwr_cnt &= ~PSB_PWRGT_GFX_MASK;
		pwr_mask |= PSB_PWRGT_GFX_MASK;
	}
	if (islands & PSB_VIDEO_ENC_ISLAND) {
		pwr_cnt &= ~PSB_PWRGT_VID_ENC_MASK;
		pwr_mask |= PSB_PWRGT_VID_ENC_MASK;
	}
	if (islands & PSB_VIDEO_DEC_ISLAND) {
		pwr_cnt &= ~PSB_PWRGT_VID_DEC_MASK;
		pwr_mask |= PSB_PWRGT_VID_DEC_MASK;
	}
	MSG_WRITE32(PSB_PUNIT_PORT, PSB_PWRGT_CNT, pwr_cnt);
	while (count--) {
		pwr_sts = MSG_READ32(PSB_PUNIT_PORT, PSB_PWRGT_STS);
		if ((pwr_sts & pwr_mask) == 0)
			break;
		else
			udelay(10);
	}
}

static int psb_power_down_sgx(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;

	PSB_DEBUG_PM("power down sgx \n");

#ifdef OSPM_STAT
	if (dev_priv->graphics_state == PSB_PWR_STATE_D0i0)
		dev_priv->gfx_d0i0_time += jiffies - dev_priv->gfx_last_mode_change;
	else
		PSB_DEBUG_PM("power down:illegal previous power state\n");
	dev_priv->gfx_last_mode_change = jiffies;
	dev_priv->gfx_d0i3_cnt++;
#endif

	dev_priv->saveCLOCKGATING = PSB_RSGX32(PSB_CR_CLKGATECTL);
	dev_priv->graphics_state = PSB_PWR_STATE_D0i3;
	psb_down_island_power(dev, PSB_GRAPHICS_ISLAND);
	return 0;
}
static int psb_power_up_sgx(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	if ((dev_priv->graphics_state & PSB_PWR_STATE_MASK) !=
	     PSB_PWR_STATE_D0i3)
		return -EINVAL;

	PSB_DEBUG_PM("power up sgx \n");
	if (unlikely(PSB_D_PM & drm_psb_debug))
		dump_stack();
	INIT_LIST_HEAD(&dev_priv->resume_buf.head);

	psb_up_island_power(dev, PSB_GRAPHICS_ISLAND);

	/*
	 * The SGX loses it's register contents.
	 * Restore BIF registers. The MMU page tables are
	 * "normal" pages, so their contents should be kept.
	 */

	PSB_WSGX32(dev_priv->saveCLOCKGATING, PSB_CR_CLKGATECTL);
	PSB_WSGX32(0x00000000, PSB_CR_BIF_BANK0);
	PSB_WSGX32(0x00000000, PSB_CR_BIF_BANK1);
	PSB_RSGX32(PSB_CR_BIF_BANK1);

	psb_mmu_set_pd_context(psb_mmu_get_default_pd(dev_priv->mmu), 0);
	psb_mmu_set_pd_context(dev_priv->pf_pd, 1);
	psb_mmu_enable_requestor(dev_priv->mmu, _PSB_MMU_ER_MASK);

	/*
	 * 2D Base registers..
	 */
	psb_init_2d(dev_priv);
	/*
	 * Persistant 3D base registers and USSE base registers..
	 */

	PSB_WSGX32(PSB_MEM_PDS_START, PSB_CR_PDS_EXEC_BASE);
	PSB_WSGX32(PSB_MEM_RASTGEOM_START, PSB_CR_BIF_3D_REQ_BASE);
	PSB_WSGX32(dev_priv->sgx2_irq_mask, PSB_CR_EVENT_HOST_ENABLE2);
	PSB_WSGX32(dev_priv->sgx_irq_mask, PSB_CR_EVENT_HOST_ENABLE);
	(void)PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE);
	/*
	 * Now, re-initialize the 3D engine.
	 */
	if (dev_priv->xhw_on)
		psb_xhw_resume(dev_priv, &dev_priv->resume_buf);

	psb_scheduler_ta_mem_check(dev_priv);
	if (dev_priv->ta_mem && !dev_priv->force_ta_mem_load) {
		psb_xhw_ta_mem_load(dev_priv, &dev_priv->resume_buf,
				    PSB_TA_MEM_FLAG_TA |
				    PSB_TA_MEM_FLAG_RASTER |
				    PSB_TA_MEM_FLAG_HOSTA |
				    PSB_TA_MEM_FLAG_HOSTD |
				    PSB_TA_MEM_FLAG_INIT,
				    dev_priv->ta_mem->ta_memory->offset,
				    dev_priv->ta_mem->hw_data->offset,
				    dev_priv->ta_mem->hw_cookie);
	}

#ifdef OSPM_STAT
	if (dev_priv->graphics_state == PSB_PWR_STATE_D0i3)
		dev_priv->gfx_d0i3_time += jiffies - dev_priv->gfx_last_mode_change;
	else
		PSB_DEBUG_PM("power up:illegal previous power state\n");
	dev_priv->gfx_last_mode_change = jiffies;
	dev_priv->gfx_d0i0_cnt++;
#endif

	dev_priv->graphics_state = PSB_PWR_STATE_D0i0;

	return 0;
}

int psb_try_power_down_sgx(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	int ret;
	if (!down_write_trylock(&dev_priv->sgx_sem))
		return -EBUSY;
	/*Try lock 2d, because FB driver ususally use 2D engine.*/
	if (!psb_2d_trylock(dev_priv)) {
		ret = -EBUSY;
		goto out_err0;
	}
	if ((dev_priv->graphics_state & PSB_PWR_STATE_MASK) !=
		PSB_PWR_STATE_D0i0) {
		ret = -EINVAL;
		goto out_err1;
	}
	if ((PSB_RSGX32(PSB_CR_2D_SOCIF) != _PSB_C2_SOCIF_EMPTY) ||
	    ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) & _PSB_C2B_STATUS_BUSY) != 0)) {
		ret = -EBUSY;
		goto out_err1;
	}
	if (!scheduler->idle ||
	       !list_empty(&scheduler->raster_queue) ||
	       !list_empty(&scheduler->ta_queue) ||
	       !list_empty(&scheduler->hp_raster_queue)) {
		ret = -EBUSY;
		goto out_err1;
	}
	/*flush_scheduled_work();*/
	ret = psb_power_down_sgx(dev);
out_err1:
	psb_2d_atomic_unlock(dev_priv);
out_err0:
	up_write(&dev_priv->sgx_sem);
	return ret;
}
/*check power state, if in sleep, wake up*/
void psb_check_power_state(struct drm_device *dev,  int devices)
{
	struct pci_dev *pdev = dev->pdev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	down(&dev_priv->pm_sem);
	switch (pdev->current_state) {
	case PCI_D3hot:
		dev->driver->pci_driver.resume(pdev);
		break;
	default:

		if (devices & PSB_DEVICE_SGX) {
			if ((dev_priv->graphics_state & PSB_PWR_STATE_MASK) ==
			     PSB_PWR_STATE_D0i3) {
				/*power up sgx*/
				psb_power_up_sgx(dev);
			}
		} else if (devices & PSB_DEVICE_MSVDX) {
			if ((dev_priv->msvdx_state & PSB_PWR_STATE_MASK) ==
				PSB_PWR_STATE_D0i3) {
				psb_power_up_msvdx(dev);
			} else {
				dev_priv->msvdx_last_action = jiffies;
			}
		}
		break;
	}
	up(&dev_priv->pm_sem);
}

void psb_init_ospm(struct drm_psb_private *dev_priv)
{
	static int init;
	if (!init) {
		dev_priv->graphics_state = PSB_PWR_STATE_D0i0;
		init_rwsem(&dev_priv->sgx_sem);
		sema_init(&dev_priv->pm_sem, 1);
#ifdef OSPM_STAT
		dev_priv->gfx_last_mode_change = jiffies;
		dev_priv->gfx_d0i0_time = 0;
		dev_priv->gfx_d0i3_time = 0;
		dev_priv->gfx_d3_time = 0;
#endif
		init = 1;
	}
}

int psb_cmdbuf_ioctl(struct drm_device *dev, void *data,
		     struct drm_file *file_priv)
{
	struct drm_psb_cmdbuf_arg *arg = data;
	int ret = 0;
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;
	struct ttm_buffer_object *cmd_buffer = NULL;
	struct ttm_buffer_object *ta_buffer = NULL;
	struct ttm_buffer_object *oom_buffer = NULL;
	struct psb_ttm_fence_rep fence_arg;
	struct drm_psb_scene user_scene;
	struct psb_scene_pool *pool = NULL;
	struct psb_scene *scene = NULL;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)file_priv->minor->dev->dev_private;
	int engine;
	struct psb_feedback_info feedback;
	int po_correct;
	struct psb_context *context;
	unsigned num_buffers;

	num_buffers = PSB_NUM_VALIDATE_BUFFERS;

	ret = ttm_read_lock(&dev_priv->ttm_lock, true);
	if (unlikely(ret != 0))
		return ret;

	if ((arg->engine == PSB_ENGINE_2D) || (arg->engine == PSB_ENGINE_TA)
		|| (arg->engine == PSB_ENGINE_RASTERIZER)) {
		down_read(&dev_priv->sgx_sem);
		psb_check_power_state(dev, PSB_DEVICE_SGX);
	}

	ret = mutex_lock_interruptible(&dev_priv->cmdbuf_mutex);
	if (unlikely(ret != 0))
		goto out_err0;


	context = &dev_priv->context;
	context->used_buffers = 0;
	context->fence_types = 0;
	BUG_ON(!list_empty(&context->validate_list));
	BUG_ON(!list_empty(&context->kern_validate_list));

	if (unlikely(context->buffers == NULL)) {
		context->buffers = vmalloc(PSB_NUM_VALIDATE_BUFFERS *
					   sizeof(*context->buffers));
		if (unlikely(context->buffers == NULL)) {
			ret = -ENOMEM;
			goto out_err1;
		}
	}

	ret = psb_reference_buffers(file_priv,
				    arg->buffer_list,
				    context);

	if (unlikely(ret != 0))
		goto out_err1;

	context->val_seq = atomic_add_return(1, &dev_priv->val_seq);

	ret = ttm_eu_reserve_buffers(&context->validate_list,
				     context->val_seq);
	if (unlikely(ret != 0)) {
		goto out_err2;
	}

	engine = (arg->engine == PSB_ENGINE_RASTERIZER) ?
	    PSB_ENGINE_TA : arg->engine;

	ret = psb_validate_buffer_list(file_priv, engine,
				       context, &po_correct);
	if (unlikely(ret != 0))
		goto out_err3;

	if (!po_correct) {
		ret = psb_fixup_relocs(file_priv, engine, arg->num_relocs,
				       arg->reloc_offset,
				       arg->reloc_handle, context, 0, 1);
		if (unlikely(ret != 0))
			goto out_err3;

	}

	cmd_buffer = ttm_buffer_object_lookup(tfile, arg->cmdbuf_handle);
	if (unlikely(cmd_buffer == NULL)) {
		ret = -EINVAL;
		goto out_err4;
	}

	switch (arg->engine) {
	case PSB_ENGINE_2D:
		ret = psb_cmdbuf_2d(file_priv, &context->validate_list,
				    context->fence_types, arg, cmd_buffer,
				    &fence_arg);
		if (unlikely(ret != 0))
			goto out_err4;
		break;
	case PSB_ENGINE_VIDEO:
		psb_check_power_state(dev, PSB_DEVICE_MSVDX);
		ret = psb_cmdbuf_video(file_priv, &context->validate_list,
				       context->fence_types, arg,
				       cmd_buffer, &fence_arg);

		if (unlikely(ret != 0))
			goto out_err4;
		break;
	case LNC_ENGINE_ENCODE:
		psb_check_power_state(dev, PSB_DEVICE_TOPAZ);
		ret = lnc_cmdbuf_video(file_priv, &context->validate_list,
				       context->fence_types, arg,
				       cmd_buffer, &fence_arg);
		if (unlikely(ret != 0))
			goto out_err4;
		break;
	case PSB_ENGINE_RASTERIZER:
		ret = psb_cmdbuf_raster(file_priv, context,
					arg, cmd_buffer, &fence_arg);
		if (unlikely(ret != 0))
			goto out_err4;
		break;
	case PSB_ENGINE_TA:
		if (arg->ta_handle == arg->cmdbuf_handle) {
			ta_buffer = ttm_bo_reference(cmd_buffer);
		} else {
			ta_buffer =
			    ttm_buffer_object_lookup(tfile,
						     arg->ta_handle);
			if (!ta_buffer) {
				ret = -EINVAL;
				goto out_err4;
			}
		}
		if (arg->oom_size != 0) {
			if (arg->oom_handle == arg->cmdbuf_handle) {
				oom_buffer = ttm_bo_reference(cmd_buffer);
			} else {
				oom_buffer =
				    ttm_buffer_object_lookup(tfile,
							     arg->
							     oom_handle);
				if (!oom_buffer) {
					ret = -EINVAL;
					goto out_err4;
				}
			}
		}

		ret = copy_from_user(&user_scene, (void __user *)
				     ((unsigned long) arg->scene_arg),
				     sizeof(user_scene));
		if (ret)
			goto out_err4;

		if (!user_scene.handle_valid) {
			pool = psb_scene_pool_alloc(file_priv, 0,
						    user_scene.num_buffers,
						    user_scene.w,
						    user_scene.h);
			if (!pool) {
				ret = -ENOMEM;
				goto out_err0;
			}

			user_scene.handle = psb_scene_pool_handle(pool);
			user_scene.handle_valid = 1;
			ret = copy_to_user((void __user *)
					   ((unsigned long) arg->
					    scene_arg), &user_scene,
					   sizeof(user_scene));

			if (ret)
				goto out_err4;
		} else {
			pool =
			    psb_scene_pool_lookup(file_priv,
						  user_scene.handle, 1);
			if (!pool) {
				ret = -EINVAL;
				goto out_err4;
			}
		}

		ret = psb_validate_scene_pool(context, pool,
					      user_scene.w,
					      user_scene.h,
					      arg->ta_flags &
					      PSB_TA_FLAG_LASTPASS, &scene);
		if (ret)
			goto out_err4;

		memset(&feedback, 0, sizeof(feedback));
		if (arg->feedback_ops) {
			ret = psb_feedback_buf(tfile,
					       context,
					       arg->feedback_ops,
					       arg->feedback_handle,
					       arg->feedback_offset,
					       arg->feedback_breakpoints,
					       arg->feedback_size,
					       &feedback);
			if (ret)
				goto out_err4;
		}
		ret = psb_cmdbuf_ta(file_priv, context,
				    arg, cmd_buffer, ta_buffer,
				    oom_buffer, scene, &feedback,
				    &fence_arg);
		if (ret)
			goto out_err4;
		break;
	default:
		DRM_ERROR
		    ("Unimplemented command submission mechanism (%x).\n",
		     arg->engine);
		ret = -EINVAL;
		goto out_err4;
	}

	if (!(arg->fence_flags & DRM_PSB_FENCE_NO_USER)) {
		ret = copy_to_user((void __user *)
				   ((unsigned long) arg->fence_arg),
				   &fence_arg, sizeof(fence_arg));
	}

out_err4:
	if (scene)
		psb_scene_unref(&scene);
	if (pool)
		psb_scene_pool_unref(&pool);
	if (cmd_buffer)
		ttm_bo_unref(&cmd_buffer);
	if (ta_buffer)
		ttm_bo_unref(&ta_buffer);
	if (oom_buffer)
		ttm_bo_unref(&oom_buffer);
out_err3:
	ret = psb_handle_copyback(dev, context, ret);
out_err2:
	psb_unreference_buffers(context);
out_err1:
	mutex_unlock(&dev_priv->cmdbuf_mutex);
out_err0:
	ttm_read_unlock(&dev_priv->ttm_lock);
	if ((arg->engine == PSB_ENGINE_2D) || (arg->engine == PSB_ENGINE_TA)
		|| (arg->engine == PSB_ENGINE_RASTERIZER))
		up_read(&dev_priv->sgx_sem);
	return ret;
}
