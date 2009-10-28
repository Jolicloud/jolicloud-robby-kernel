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
 */

#include "drmP.h"
#include "psb_drv.h"
#include "psb_drm.h"
#include "psb_reg.h"
#include "psb_scene.h"
#include "psb_detear.h"

#include "psb_msvdx.h"

int psb_submit_video_cmdbuf(struct drm_device *dev,
			    struct drm_buffer_object *cmd_buffer,
			    unsigned long cmd_offset, unsigned long cmd_size,
			    struct drm_fence_object *fence);

struct psb_dstbuf_cache {
	unsigned int dst;
	uint32_t *use_page;
	unsigned int use_index;
	uint32_t use_background;
	struct drm_buffer_object *dst_buf;
	unsigned long dst_offset;
	uint32_t *dst_page;
	unsigned int dst_page_offset;
	struct drm_bo_kmap_obj dst_kmap;
	int dst_is_iomem;
};

struct psb_buflist_item {
	struct drm_buffer_object *bo;
	void __user *data;
	int ret;
	int presumed_offset_correct;
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
	{0x0804, 0x0A58},
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
	return ((psb_disallowed_regs[reg >> 5] & (1 << (reg & 31))) != 0);
}

void psb_init_disallowed(void)
{
	int i;
	uint32_t reg, tmp;
	static int initialized = 0;

	if (initialized)
		return;

	initialized = 1;
	memset(psb_disallowed_regs, 0, sizeof(psb_disallowed_regs));

	for (i = 0; i < (sizeof(disallowed_ranges) / (2 * sizeof(uint32_t)));
	     ++i) {
		for (reg = disallowed_ranges[i][0];
		     reg <= disallowed_ranges[i][1]; reg += 4) {
			tmp = reg >> 2;
			psb_disallowed_regs[tmp >> 5] |= (1 << (tmp & 31));
		}
	}
}

static int psb_memcpy_check(uint32_t * dst, const uint32_t * src, uint32_t size)
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

static int psb_2d_wait_available(struct drm_psb_private *dev_priv,
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
			    ((avail = PSB_RSGX32(PSB_CR_2D_SOCIF)) >= size));
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

int psb_2d_submit(struct drm_psb_private *dev_priv, uint32_t * cmdbuf,
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

#ifdef PSB_DETEAR
		/* delayed 2D blit tasks are not executed right now,
		   let's save a copy of the task */
		if(dev_priv->blit_2d) {
			/* FIXME: should use better approach other
			   than the dev_priv->blit_2d to distinguish
			   delayed 2D blit tasks */
			dev_priv->blit_2d = 0; 
			memcpy(psb_blit_info.cmdbuf, cmdbuf, 10*4);
		} else
#endif	/* PSB_DETEAR */
		{
			for (i = 0; i < submit_size; i += 4) {
				PSB_WSGX32(*cmdbuf++, PSB_SGX_2D_SLAVE_PORT + i);
			}
			(void)PSB_RSGX32(PSB_SGX_2D_SLAVE_PORT + i - 4);
		}
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
	*bufp++ = (1 << PSB_2D_DST_XSIZE_SHIFT) | (1 << PSB_2D_DST_YSIZE_SHIFT);

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
			  uint32_t dst_offset, uint32_t pages, int direction)
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
		    PSB_2D_SRC_OFF_BH | (xstart << PSB_2D_SRCOFF_XSTART_SHIFT) |
		    (ystart << PSB_2D_SRCOFF_YSTART_SHIFT);
		*bufp++ = blit_cmd;
		*bufp++ = (xstart << PSB_2D_DST_XSTART_SHIFT) |
		    (ystart << PSB_2D_DST_YSTART_SHIFT);
		*bufp++ = ((PAGE_SIZE >> 2) << PSB_2D_DST_XSIZE_SHIFT) |
		    (cur_pages << PSB_2D_DST_YSIZE_SHIFT);

		ret = psb_2d_submit(dev_priv, buf, bufp - buf);
		if (ret)
			goto out;
		pg_add = (cur_pages << PAGE_SHIFT) * ((direction) ? -1 : 1);
		src_offset += pg_add;
		dst_offset += pg_add;
	}
      out:
	psb_2d_unlock(dev_priv);
	return ret;
}

void psb_init_2d(struct drm_psb_private *dev_priv)
{
	dev_priv->sequence_lock = SPIN_LOCK_UNLOCKED;
	psb_reset(dev_priv, 1);
	dev_priv->mmu_2d_offset = dev_priv->pg->gatt_start;
	PSB_WSGX32(dev_priv->mmu_2d_offset, PSB_CR_BIF_TWOD_REQ_BASE);
	(void)PSB_RSGX32(PSB_CR_BIF_TWOD_REQ_BASE);
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
	    ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) & _PSB_C2B_STATUS_BUSY) == 0))
		goto out;

	do {
		busy = (PSB_RSGX32(PSB_CR_2D_SOCIF) != _PSB_C2_SOCIF_EMPTY);
	} while (busy && !time_after_eq(jiffies, _end));

	if (busy)
		busy = (PSB_RSGX32(PSB_CR_2D_SOCIF) != _PSB_C2_SOCIF_EMPTY);
	if (busy)
		goto out;

	do {
		busy =
		    ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) & _PSB_C2B_STATUS_BUSY)
		     != 0);
	} while (busy && !time_after_eq(jiffies, _end));
	if (busy)
		busy =
		    ((PSB_RSGX32(PSB_CR_2D_BLIT_STATUS) & _PSB_C2B_STATUS_BUSY)
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
				 psb_scheduler_finished(dev_priv), DRM_HZ * 10);

	return (ret < 1) ? -EBUSY : 0;
}

static void psb_dereference_buffers_locked(struct psb_buflist_item *buffers,
					   unsigned num_buffers)
{
	while (num_buffers--)
		drm_bo_usage_deref_locked(&((buffers++)->bo));

}

static int psb_check_presumed(struct drm_bo_op_arg *arg,
			      struct drm_buffer_object *bo,
			      uint32_t __user * data, int *presumed_ok)
{
	struct drm_bo_op_req *req = &arg->d.req;
	uint32_t hint_offset;
	uint32_t hint = req->bo_req.hint;

	*presumed_ok = 0;

	if (!(hint & DRM_BO_HINT_PRESUMED_OFFSET))
		return 0;
	if (bo->mem.mem_type == DRM_BO_MEM_LOCAL) {
		*presumed_ok = 1;
		return 0;
	}
	if (bo->offset == req->bo_req.presumed_offset) {
		*presumed_ok = 1;
		return 0;
	}

	/*
	 * We need to turn off the HINT_PRESUMED_OFFSET for this buffer in
	 * the user-space IOCTL argument list, since the buffer has moved,
	 * we're about to apply relocations and we might subsequently
	 * hit an -EAGAIN. In that case the argument list will be reused by
	 * user-space, but the presumed offset is no longer valid.
	 *
	 * Needless to say, this is a bit ugly.
	 */

	hint_offset = (uint32_t *) & req->bo_req.hint - (uint32_t *) arg;
	hint &= ~DRM_BO_HINT_PRESUMED_OFFSET;
	return __put_user(hint, data + hint_offset);
}

static int psb_validate_buffer_list(struct drm_file *file_priv,
				    unsigned fence_class,
				    unsigned long data,
				    struct psb_buflist_item *buffers,
				    unsigned *num_buffers)
{
	struct drm_bo_op_arg arg;
	struct drm_bo_op_req *req = &arg.d.req;
	int ret = 0;
	unsigned buf_count = 0;
	struct psb_buflist_item *item = buffers;

	do {
		if (buf_count >= *num_buffers) {
			DRM_ERROR("Buffer count exceeded %d\n.", *num_buffers);
			ret = -EINVAL;
			goto out_err;
		}
		item = buffers + buf_count;
		item->bo = NULL;

		if (copy_from_user(&arg, (void __user *)data, sizeof(arg))) {
			ret = -EFAULT;
			DRM_ERROR("Error copying validate list.\n"
				  "\tbuffer %d, user addr 0x%08lx %d\n",
				  buf_count, (unsigned long)data, sizeof(arg));
			goto out_err;
		}

		ret = 0;
		if (req->op != drm_bo_validate) {
			DRM_ERROR
			    ("Buffer object operation wasn't \"validate\".\n");
			ret = -EINVAL;
			goto out_err;
		}

		item->ret = 0;
		item->data = (void *)__user data;
		ret = drm_bo_handle_validate(file_priv,
					     req->bo_req.handle,
					     fence_class,
					     req->bo_req.flags,
					     req->bo_req.mask,
					     req->bo_req.hint,
					     0, NULL, &item->bo);
		if (ret)
			goto out_err;

		PSB_DEBUG_GENERAL("Validated buffer at 0x%08lx\n",
				  buffers[buf_count].bo->offset);

		buf_count++;


		ret = psb_check_presumed(&arg, item->bo,
					 (uint32_t __user *)
					 (unsigned long) data,
					 &item->presumed_offset_correct);

		if (ret)
			goto out_err;

		data = arg.next;
	} while (data);

	*num_buffers = buf_count;

	return 0;
      out_err:

	*num_buffers = buf_count;
	item->ret = (ret != -EAGAIN) ? ret : 0;
	return ret;
}

int
psb_reg_submit(struct drm_psb_private *dev_priv, uint32_t * regs,
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
		       struct drm_buffer_object *cmd_buffer,
		       unsigned long cmd_offset,
		       unsigned long cmd_size,
		       int engine, uint32_t * copy_buffer)
{
	unsigned long cmd_end = cmd_offset + (cmd_size << 2);
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned long cmd_page_offset = cmd_offset - (cmd_offset & PAGE_MASK);
	unsigned long cmd_next;
	struct drm_bo_kmap_obj cmd_kmap;
	uint32_t *cmd_page;
	unsigned cmds;
	int is_iomem;
	int ret = 0;

	if (cmd_size == 0)
		return 0;

	if (engine == PSB_ENGINE_2D)
		psb_2d_lock(dev_priv);

	do {
		cmd_next = drm_bo_offset_end(cmd_offset, cmd_end);
		ret = drm_bo_kmap(cmd_buffer, cmd_offset >> PAGE_SHIFT,
				  1, &cmd_kmap);

		if (ret)
			return ret;
		cmd_page = drm_bmo_virtual(&cmd_kmap, &is_iomem);
		cmd_page_offset = (cmd_offset & ~PAGE_MASK) >> 2;
		cmds = (cmd_next - cmd_offset) >> 2;

		switch (engine) {
		case PSB_ENGINE_2D:
			ret =
			    psb_2d_submit(dev_priv, cmd_page + cmd_page_offset,
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
		drm_bo_kunmap(&cmd_kmap);
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
		drm_bo_kunmap(&dst_cache->dst_kmap);
		dst_cache->dst_page = NULL;
	}
	dst_cache->dst_buf = NULL;
	dst_cache->dst = ~0;
	dst_cache->use_page = NULL;
}

static int psb_update_dstbuf_cache(struct psb_dstbuf_cache *dst_cache,
				   struct psb_buflist_item *buffers,
				   unsigned int dst, unsigned long dst_offset)
{
	int ret;

	PSB_DEBUG_RELOC("Destination buffer is %d.\n", dst);

	if (unlikely(dst != dst_cache->dst || NULL == dst_cache->dst_buf)) {
		psb_clear_dstbuf_cache(dst_cache);
		dst_cache->dst = dst;
		dst_cache->dst_buf = buffers[dst].bo;
	}

	if (unlikely(dst_offset > dst_cache->dst_buf->num_pages * PAGE_SIZE)) {
		DRM_ERROR("Relocation destination out of bounds.\n");
		return -EINVAL;
	}

	if (!drm_bo_same_page(dst_cache->dst_offset, dst_offset) ||
	    NULL == dst_cache->dst_page) {
		if (NULL != dst_cache->dst_page) {
			drm_bo_kunmap(&dst_cache->dst_kmap);
			dst_cache->dst_page = NULL;
		}

		ret = drm_bo_kmap(dst_cache->dst_buf, dst_offset >> PAGE_SHIFT,
				  1, &dst_cache->dst_kmap);
		if (ret) {
			DRM_ERROR("Could not map destination buffer for "
				  "relocation.\n");
			return ret;
		}

		dst_cache->dst_page = drm_bmo_virtual(&dst_cache->dst_kmap,
						      &dst_cache->dst_is_iomem);
		dst_cache->dst_offset = dst_offset & PAGE_MASK;
		dst_cache->dst_page_offset = dst_cache->dst_offset >> 2;
	}
	return 0;
}

static int psb_apply_reloc(struct drm_psb_private *dev_priv,
			   uint32_t fence_class,
			   const struct drm_psb_reloc *reloc,
			   struct psb_buflist_item *buffers,
			   int num_buffers,
			   struct psb_dstbuf_cache *dst_cache,
			   int no_wait, int interruptible)
{
	int reg;
	uint32_t val;
	uint32_t background;
	unsigned int index;
	int ret;
	unsigned int shift;
	unsigned int align_shift;
	uint32_t fence_type;
	struct drm_buffer_object *reloc_bo;

	PSB_DEBUG_RELOC("Reloc type %d\n"
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
		DRM_ERROR("Illegal relocation buffer %d.\n", reloc->buffer);
		return -EINVAL;
	}

	if (buffers[reloc->buffer].presumed_offset_correct)
		return 0;

	if (unlikely(reloc->dst_buffer >= num_buffers)) {
		DRM_ERROR("Illegal destination buffer for relocation %d.\n",
			  reloc->dst_buffer);
		return -EINVAL;
	}

	ret = psb_update_dstbuf_cache(dst_cache, buffers, reloc->dst_buffer,
				      reloc->where << 2);
	if (ret)
		return ret;

	reloc_bo = buffers[reloc->buffer].bo;

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
		val = reloc_bo->offset + reloc->pre_add - PSB_MEM_PDS_START;
		if (unlikely(val >= (PSB_MEM_MMU_START - PSB_MEM_PDS_START))) {
			DRM_ERROR("PDS relocation out of bounds\n");
			return -EINVAL;
		}
		break;
	case PSB_RELOC_OP_USE_OFFSET:
	case PSB_RELOC_OP_USE_REG:

		/*
		 * Security:
		 * Only allow VERTEX or PIXEL data masters, as
		 * shaders run under other data masters may in theory
		 * alter MMU mappings.
		 */

		if (unlikely(reloc->arg1 != _PSB_CUC_DM_PIXEL &&
			     reloc->arg1 != _PSB_CUC_DM_VERTEX)) {
			DRM_ERROR("Invalid data master in relocation. %d\n",
				  reloc->arg1);
			return -EPERM;
		}

		fence_type = reloc_bo->fence_type;
		ret = psb_grab_use_base(dev_priv,
					reloc_bo->offset +
					reloc->pre_add, reloc->arg0,
					reloc->arg1, fence_class,
					fence_type, no_wait,
					interruptible, &reg, &val);
		if (ret)
			return ret;

		val = (reloc->reloc_op == PSB_RELOC_OP_USE_REG) ? reg : val;
		break;
	default:
		DRM_ERROR("Unimplemented relocation.\n");
		return -EINVAL;
	}

	shift = (reloc->shift & PSB_RELOC_SHIFT_MASK) >> PSB_RELOC_SHIFT_SHIFT;
	align_shift = (reloc->shift & PSB_RELOC_ALSHIFT_MASK) >>
	    PSB_RELOC_ALSHIFT_SHIFT;

	val = ((val >> align_shift) << shift);
	index = reloc->where - dst_cache->dst_page_offset;

	background = reloc->background;

	if (reloc->reloc_op == PSB_RELOC_OP_USE_OFFSET) {
		if (dst_cache->use_page == dst_cache->dst_page &&
		    dst_cache->use_index == index)
			background = dst_cache->use_background;
		else
			background = dst_cache->dst_page[index];
	}
#if 0
	if (dst_cache->dst_page[index] != PSB_RELOC_MAGIC &&
	    reloc->reloc_op != PSB_RELOC_OP_USE_OFFSET)
		DRM_ERROR("Inconsistent relocation 0x%08lx.\n",
			  (unsigned long)dst_cache->dst_page[index]);
#endif

	val = (background & ~reloc->mask) | (val & reloc->mask);
	dst_cache->dst_page[index] = val;

	if (reloc->reloc_op == PSB_RELOC_OP_USE_OFFSET ||
	    reloc->reloc_op == PSB_RELOC_OP_USE_REG) {
		dst_cache->use_page = dst_cache->dst_page;
		dst_cache->use_index = index;
		dst_cache->use_background = val;
	}

	PSB_DEBUG_RELOC("Reloc buffer %d index 0x%08x, value 0x%08x\n",
			  reloc->dst_buffer, index, dst_cache->dst_page[index]);

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
			    struct psb_buflist_item *buffers,
			    unsigned int num_buffers,
			    int no_wait, int interruptible)
{
	struct drm_device *dev = file_priv->head->dev;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct drm_buffer_object *reloc_buffer = NULL;
	unsigned int reloc_num_pages;
	unsigned int reloc_first_page;
	unsigned int reloc_last_page;
	struct psb_dstbuf_cache dst_cache;
	struct drm_psb_reloc *reloc;
	struct drm_bo_kmap_obj reloc_kmap;
	int reloc_is_iomem;
	int count;
	int ret = 0;
	int registered = 0;
	int short_circuit = 1;
	int i;

	if (num_relocs == 0)
		return 0;

	for (i=0; i<num_buffers; ++i) {
		if (!buffers[i].presumed_offset_correct) {
			short_circuit = 0;
			break;
		}
	}

	if (short_circuit)
		return 0;

	memset(&dst_cache, 0, sizeof(dst_cache));
	memset(&reloc_kmap, 0, sizeof(reloc_kmap));

	mutex_lock(&dev->struct_mutex);
	reloc_buffer = drm_lookup_buffer_object(file_priv, reloc_handle, 1);
	mutex_unlock(&dev->struct_mutex);
	if (!reloc_buffer)
		goto out;

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
		ret = -EAGAIN;
		goto out;
	}
	if (ret) {
		DRM_ERROR("Error waiting for space to map "
			  "relocation buffer.\n");
		goto out;
	}

	ret = drm_bo_kmap(reloc_buffer, reloc_first_page,
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
	    ((unsigned long)drm_bmo_virtual(&reloc_kmap, &reloc_is_iomem) +
	     reloc_offset);

	for (count = 0; count < num_relocs; ++count) {
		ret = psb_apply_reloc(dev_priv, fence_class,
				      reloc, buffers,
				      num_buffers, &dst_cache,
				      no_wait, interruptible);
		if (ret)
			goto out1;
		reloc++;
	}

      out1:
	drm_bo_kunmap(&reloc_kmap);
      out:
	if (registered) {
		spin_lock(&dev_priv->reloc_lock);
		dev_priv->rel_mapped_pages -= reloc_num_pages;
		spin_unlock(&dev_priv->reloc_lock);
		DRM_WAKEUP(&dev_priv->rel_mapped_queue);
	}

	psb_clear_dstbuf_cache(&dst_cache);
	if (reloc_buffer)
		drm_bo_usage_deref_unlocked(&reloc_buffer);
	return ret;
}

static int psb_cmdbuf_2d(struct drm_file *priv,
		struct drm_psb_cmdbuf_arg *arg,
		struct drm_buffer_object *cmd_buffer,
		struct drm_fence_arg *fence_arg)
{
	struct drm_device *dev = priv->head->dev;
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *)dev->dev_private;
	int ret;

	ret = mutex_lock_interruptible(&dev_priv->reset_mutex);
	if (ret)
		return -EAGAIN;

#ifdef PSB_DETEAR
	if(arg->sVideoInfo.flag == (PSB_DELAYED_2D_BLIT)) {
		dev_priv->blit_2d = 1;
	}
#endif	/* PSB_DETEAR */

	ret = psb_submit_copy_cmdbuf(dev, cmd_buffer, arg->cmdbuf_offset,
			arg->cmdbuf_size, PSB_ENGINE_2D, NULL);
	if (ret)
		goto out_unlock;

#ifdef PSB_DETEAR
	if(arg->sVideoInfo.flag == (PSB_DELAYED_2D_BLIT)) {
		arg->sVideoInfo.flag = 0;
		clear_bit(0, &psb_blit_info.vdc_bit);
		psb_blit_info.cmd_ready = 1;
		/* block until the delayed 2D blit task finishes
		   execution */
		while(test_bit(0, &psb_blit_info.vdc_bit)==0)
			schedule();
	}
#endif	/* PSB_DETEAR */

	psb_fence_or_sync(priv, PSB_ENGINE_2D, arg, fence_arg, NULL);

	mutex_lock(&cmd_buffer->mutex);
	if (cmd_buffer->fence != NULL)
		drm_fence_usage_deref_unlocked(&cmd_buffer->fence);
	mutex_unlock(&cmd_buffer->mutex);
out_unlock:
	mutex_unlock(&dev_priv->reset_mutex);
	return ret;
}

#if 0
static int psb_dump_page(struct drm_buffer_object *bo,
			 unsigned int page_offset, unsigned int num)
{
	struct drm_bo_kmap_obj kmobj;
	int is_iomem;
	uint32_t *p;
	int ret;
	unsigned int i;

	ret = drm_bo_kmap(bo, page_offset, 1, &kmobj);
	if (ret)
		return ret;

	p = drm_bmo_virtual(&kmobj, &is_iomem);
	for (i = 0; i < num; ++i)
		PSB_DEBUG_GENERAL("0x%04x: 0x%08x\n", i, *p++);

	drm_bo_kunmap(&kmobj);
	return 0;
}
#endif

static void psb_idle_engine(struct drm_device *dev, int engine)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	uint32_t dummy;

	switch (engine) {
	case PSB_ENGINE_2D:

		/*
		 * Make sure we flush 2D properly using a dummy
		 * fence sequence emit.
		 */

		(void)psb_fence_emit_sequence(dev, PSB_ENGINE_2D, 0,
					      &dummy, &dummy);
		psb_2d_lock(dev_priv);
		(void)psb_idle_2d(dev);
		psb_2d_unlock(dev_priv);
		break;
	case PSB_ENGINE_TA:
	case PSB_ENGINE_RASTERIZER:
	case PSB_ENGINE_HPRAST:
		(void)psb_idle_3d(dev);
		break;
	default:

		/*
		 * FIXME: Insert video engine idle command here.
		 */

		break;
	}
}

void psb_fence_or_sync(struct drm_file *priv,
		       int engine,
		       struct drm_psb_cmdbuf_arg *arg,
		       struct drm_fence_arg *fence_arg,
		       struct drm_fence_object **fence_p)
{
	struct drm_device *dev = priv->head->dev;
	int ret;
	struct drm_fence_object *fence;

	ret = drm_fence_buffer_objects(dev, NULL, arg->fence_flags,
				       NULL, &fence);

	if (ret) {

		/*
		 * Fence creation failed.
		 * Fall back to synchronous operation and idle the engine.
		 */

		psb_idle_engine(dev, engine);
		if (!(arg->fence_flags & DRM_FENCE_FLAG_NO_USER)) {

			/*
			 * Communicate to user-space that
			 * fence creation has failed and that
			 * the engine is idle.
			 */

			fence_arg->handle = ~0;
			fence_arg->error = ret;
		}

		drm_putback_buffer_objects(dev);
		if (fence_p)
			*fence_p = NULL;
		return;
	}

	if (!(arg->fence_flags & DRM_FENCE_FLAG_NO_USER)) {

		ret = drm_fence_add_user_object(priv, fence,
						arg->fence_flags &
						DRM_FENCE_FLAG_SHAREABLE);
		if (!ret)
			drm_fence_fill_arg(fence, fence_arg);
		else {
			/*
			 * Fence user object creation failed.
			 * We must idle the engine here as well, as user-
			 * space expects a fence object to wait on. Since we
			 * have a fence object we wait for it to signal
			 * to indicate engine "sufficiently" idle.
			 */

			(void)drm_fence_object_wait(fence, 0, 1, fence->type);
			drm_fence_usage_deref_unlocked(&fence);
			fence_arg->handle = ~0;
			fence_arg->error = ret;
		}
	}

	if (fence_p)
		*fence_p = fence;
	else if (fence)
		drm_fence_usage_deref_unlocked(&fence);
}

int psb_handle_copyback(struct drm_device *dev,
			struct psb_buflist_item *buffers,
			unsigned int num_buffers, int ret, void *data)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct drm_bo_op_arg arg;
	struct psb_buflist_item *item = buffers;
	struct drm_buffer_object *bo;
	int err = ret;
	int i;

	/*
	 * Clear the unfenced use base register lists and buffer lists.
	 */

	if (ret) {
		drm_regs_fence(&dev_priv->use_manager, NULL);
		drm_putback_buffer_objects(dev);
	}

	if (ret != -EAGAIN) {
		for (i = 0; i < num_buffers; ++i) {
			arg.handled = 1;
			arg.d.rep.ret = item->ret;
			bo = item->bo;
			mutex_lock(&bo->mutex);
			drm_bo_fill_rep_arg(bo, &arg.d.rep.bo_info);
			mutex_unlock(&bo->mutex);
			if (copy_to_user(item->data, &arg, sizeof(arg)))
				err = -EFAULT;
			++item;
		}
	}

	return err;
}

static int psb_cmdbuf_video(struct drm_file *priv,
			    struct drm_psb_cmdbuf_arg *arg,
			    unsigned int num_buffers,
			    struct drm_buffer_object *cmd_buffer,
			    struct drm_fence_arg *fence_arg)
{
	struct drm_device *dev = priv->head->dev;
	struct drm_fence_object *fence;
	int ret;

	/*
	 * Check this. Doesn't seem right. Have fencing done AFTER command
	 * submission and make sure drm_psb_idle idles the MSVDX completely.
	 */

	psb_fence_or_sync(priv, PSB_ENGINE_VIDEO, arg, fence_arg, &fence);
	ret = psb_submit_video_cmdbuf(dev, cmd_buffer, arg->cmdbuf_offset,
				      arg->cmdbuf_size, fence);

	if (ret)
		return ret;

	drm_fence_usage_deref_unlocked(&fence);
	mutex_lock(&cmd_buffer->mutex);
	if (cmd_buffer->fence != NULL)
		drm_fence_usage_deref_unlocked(&cmd_buffer->fence);
	mutex_unlock(&cmd_buffer->mutex);
	return 0;
}

int psb_feedback_buf(struct drm_file *file_priv,
		     uint32_t feedback_ops,
		     uint32_t handle,
		     uint32_t offset,
		     uint32_t feedback_breakpoints,
		     uint32_t feedback_size, struct psb_feedback_info *feedback)
{
	struct drm_buffer_object *bo;
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

	ret = drm_bo_handle_validate(file_priv,
				     handle,
				     PSB_ENGINE_TA,
				     DRM_BO_FLAG_MEM_LOCAL |
				     DRM_BO_FLAG_CACHED |
				     DRM_BO_FLAG_WRITE |
				     PSB_BO_FLAG_FEEDBACK,
				     DRM_BO_MASK_MEM |
				     DRM_BO_FLAG_CACHED |
				     DRM_BO_FLAG_WRITE |
				     PSB_BO_FLAG_FEEDBACK, 0, 0, NULL, &bo);
	if (ret)
		return ret;

	page_no = offset >> PAGE_SHIFT;
	if (page_no >= bo->num_pages) {
		ret = -EINVAL;
		DRM_ERROR("Illegal feedback buffer offset.\n");
		goto out_unref;
	}

	if (bo->ttm == NULL) {
		ret = -EINVAL;
		DRM_ERROR("Vistest buffer without TTM.\n");
		goto out_unref;
	}

	page = drm_ttm_get_page(bo->ttm, page_no);
	if (!page) {
		ret = -ENOMEM;
		goto out_unref;
	}

	feedback->page = page;
	feedback->bo = bo;
	feedback->offset = page_offset;
	return 0;

      out_unref:
	drm_bo_usage_deref_unlocked(&bo);
	return ret;
}

int psb_cmdbuf_ioctl(struct drm_device *dev, void *data,
		     struct drm_file *file_priv)
{
	drm_psb_cmdbuf_arg_t *arg = data;
	int ret = 0;
	unsigned num_buffers;
	struct drm_buffer_object *cmd_buffer = NULL;
	struct drm_buffer_object *ta_buffer = NULL;
	struct drm_buffer_object *oom_buffer = NULL;
	struct drm_fence_arg fence_arg;
	struct drm_psb_scene user_scene;
	struct psb_scene_pool *pool = NULL;
	struct psb_scene *scene = NULL;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)file_priv->head->dev->dev_private;
	int engine;
	struct psb_feedback_info feedback;

	if (!dev_priv)
		return -EINVAL;

	ret = drm_bo_read_lock(&dev->bm.bm_lock, 1);
	if (ret)
		return ret;

	num_buffers = PSB_NUM_VALIDATE_BUFFERS;

	ret = mutex_lock_interruptible(&dev_priv->cmdbuf_mutex);
	if (ret) {
		drm_bo_read_unlock(&dev->bm.bm_lock);
		return -EAGAIN;
	}
	if (unlikely(dev_priv->buffers == NULL)) {
		dev_priv->buffers = vmalloc(PSB_NUM_VALIDATE_BUFFERS *
					    sizeof(*dev_priv->buffers));
		if (dev_priv->buffers == NULL) {
			drm_bo_read_unlock(&dev->bm.bm_lock);
			return -ENOMEM;
		}
	}


	engine = (arg->engine == PSB_ENGINE_RASTERIZER) ?
	    PSB_ENGINE_TA : arg->engine;

	ret =
	    psb_validate_buffer_list(file_priv, engine,
				     (unsigned long)arg->buffer_list,
				     dev_priv->buffers, &num_buffers);
	if (ret)
		goto out_err0;

	ret = psb_fixup_relocs(file_priv, engine, arg->num_relocs,
			       arg->reloc_offset, arg->reloc_handle,
			       dev_priv->buffers, num_buffers, 0, 1);
	if (ret)
		goto out_err0;

	mutex_lock(&dev->struct_mutex);
	cmd_buffer = drm_lookup_buffer_object(file_priv, arg->cmdbuf_handle, 1);
	mutex_unlock(&dev->struct_mutex);
	if (!cmd_buffer) {
		ret = -EINVAL;
		goto out_err0;
	}

	switch (arg->engine) {
	case PSB_ENGINE_2D:
		ret = psb_cmdbuf_2d(file_priv, arg, cmd_buffer, &fence_arg);
		if (ret)
			goto out_err0;
		break;
	case PSB_ENGINE_VIDEO:
		ret =
		    psb_cmdbuf_video(file_priv, arg, num_buffers, cmd_buffer,
				     &fence_arg);
		if (ret)
			goto out_err0;
		break;
	case PSB_ENGINE_RASTERIZER:
		ret = psb_cmdbuf_raster(file_priv, arg, cmd_buffer, &fence_arg);
		if (ret)
			goto out_err0;
		break;
	case PSB_ENGINE_TA:
		if (arg->ta_handle == arg->cmdbuf_handle) {
			mutex_lock(&dev->struct_mutex);
			atomic_inc(&cmd_buffer->usage);
			ta_buffer = cmd_buffer;
			mutex_unlock(&dev->struct_mutex);
		} else {
			mutex_lock(&dev->struct_mutex);
			ta_buffer =
			    drm_lookup_buffer_object(file_priv,
						     arg->ta_handle, 1);
			mutex_unlock(&dev->struct_mutex);
			if (!ta_buffer) {
				ret = -EINVAL;
				goto out_err0;
			}
		}
		if (arg->oom_size != 0) {
			if (arg->oom_handle == arg->cmdbuf_handle) {
				mutex_lock(&dev->struct_mutex);
				atomic_inc(&cmd_buffer->usage);
				oom_buffer = cmd_buffer;
				mutex_unlock(&dev->struct_mutex);
			} else {
				mutex_lock(&dev->struct_mutex);
				oom_buffer =
				    drm_lookup_buffer_object(file_priv,
							     arg->oom_handle,
							     1);
				mutex_unlock(&dev->struct_mutex);
				if (!oom_buffer) {
					ret = -EINVAL;
					goto out_err0;
				}
			}
		}

		ret = copy_from_user(&user_scene, (void __user *)
				     ((unsigned long)arg->scene_arg),
				     sizeof(user_scene));
		if (ret)
			goto out_err0;

		if (!user_scene.handle_valid) {
			pool = psb_scene_pool_alloc(file_priv, 0,
						    user_scene.num_buffers,
						    user_scene.w, user_scene.h);
			if (!pool) {
				ret = -ENOMEM;
				goto out_err0;
			}

			user_scene.handle = psb_scene_pool_handle(pool);
			user_scene.handle_valid = 1;
			ret = copy_to_user((void __user *)
					   ((unsigned long)arg->scene_arg),
					   &user_scene, sizeof(user_scene));

			if (ret)
				goto out_err0;
		} else {
			mutex_lock(&dev->struct_mutex);
			pool = psb_scene_pool_lookup_devlocked(file_priv,
							       user_scene.
							       handle, 1);
			mutex_unlock(&dev->struct_mutex);
			if (!pool) {
				ret = -EINVAL;
				goto out_err0;
			}
		}

		mutex_lock(&dev_priv->reset_mutex);
		ret = psb_validate_scene_pool(pool, 0, 0, 0,
					      user_scene.w,
					      user_scene.h,
					      arg->ta_flags &
					      PSB_TA_FLAG_LASTPASS, &scene);
		mutex_unlock(&dev_priv->reset_mutex);

		if (ret)
			goto out_err0;

		memset(&feedback, 0, sizeof(feedback));
		if (arg->feedback_ops) {
			ret = psb_feedback_buf(file_priv,
					       arg->feedback_ops,
					       arg->feedback_handle,
					       arg->feedback_offset,
					       arg->feedback_breakpoints,
					       arg->feedback_size, &feedback);
			if (ret)
				goto out_err0;
		}
		ret = psb_cmdbuf_ta(file_priv, arg, cmd_buffer, ta_buffer,
				    oom_buffer, scene, &feedback, &fence_arg);
		if (ret)
			goto out_err0;
		break;
	default:
		DRM_ERROR("Unimplemented command submission mechanism (%x).\n",
			  arg->engine);
		ret = -EINVAL;
		goto out_err0;
	}

	if (!(arg->fence_flags & DRM_FENCE_FLAG_NO_USER)) {
		ret = copy_to_user((void __user *)
				   ((unsigned long)arg->fence_arg),
				   &fence_arg, sizeof(fence_arg));
	}

      out_err0:
	ret =
	    psb_handle_copyback(dev, dev_priv->buffers, num_buffers, ret, data);
	mutex_lock(&dev->struct_mutex);
	if (scene)
		psb_scene_unref_devlocked(&scene);
	if (pool)
		psb_scene_pool_unref_devlocked(&pool);
	if (cmd_buffer)
		drm_bo_usage_deref_locked(&cmd_buffer);
	if (ta_buffer)
		drm_bo_usage_deref_locked(&ta_buffer);
	if (oom_buffer)
		drm_bo_usage_deref_locked(&oom_buffer);

	psb_dereference_buffers_locked(dev_priv->buffers, num_buffers);
	mutex_unlock(&dev->struct_mutex);
	mutex_unlock(&dev_priv->cmdbuf_mutex);

	drm_bo_read_unlock(&dev->bm.bm_lock);
	return ret;
}
