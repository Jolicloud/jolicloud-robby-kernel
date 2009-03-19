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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/console.h>

#include <drm/drmP.h>
#include <drm/drm.h>
#include <drm/drm_crtc.h>

#include "psb_drv.h"
#include "intel_reg.h"
#include "intel_drv.h"
#include "ttm/ttm_userobj_api.h"
#include "psb_fb.h"
#include "psb_sgx.h"

static int fill_fb_bitfield(struct fb_var_screeninfo *var, int depth)
{
	switch (depth) {
	case 8:
		var->red.offset = 0;
		var->green.offset = 0;
		var->blue.offset = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.length = 0;
		var->transp.offset = 0;
		break;
	case 15:
		var->red.offset = 10;
		var->green.offset = 5;
		var->blue.offset = 0;
		var->red.length = 5;
		var->green.length = 5;
		var->blue.length = 5;
		var->transp.length = 1;
		var->transp.offset = 15;
		break;
	case 16:
		var->red.offset = 11;
		var->green.offset = 5;
		var->blue.offset = 0;
		var->red.length = 5;
		var->green.length = 6;
		var->blue.length = 5;
		var->transp.length = 0;
		var->transp.offset = 0;
		break;
	case 24:
		var->red.offset = 16;
		var->green.offset = 8;
		var->blue.offset = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.length = 0;
		var->transp.offset = 0;
		break;
	case 32:
		var->red.offset = 16;
		var->green.offset = 8;
		var->blue.offset = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.length = 8;
		var->transp.offset = 24;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void psb_user_framebuffer_destroy(struct drm_framebuffer *fb);
static int psb_user_framebuffer_create_handle(struct drm_framebuffer *fb,
					      struct drm_file *file_priv,
					      unsigned int *handle);

static const struct drm_framebuffer_funcs psb_fb_funcs = {
	.destroy = psb_user_framebuffer_destroy,
	.create_handle = psb_user_framebuffer_create_handle,
};

struct psbfb_par {
	struct drm_device *dev;
	struct psb_framebuffer *psbfb;

	int dpms_state;

	int crtc_count;
	/* crtc currently bound to this */
	uint32_t crtc_ids[2];
};

#define CMAP_TOHW(_val, _width) ((((_val) << (_width)) + 0x7FFF - (_val)) >> 16)

static int psbfb_setcolreg(unsigned regno, unsigned red, unsigned green,
			   unsigned blue, unsigned transp,
			   struct fb_info *info)
{
	struct psbfb_par *par = info->par;
	struct drm_framebuffer *fb = &par->psbfb->base;
	uint32_t v;

	if (!fb)
		return -ENOMEM;

	if (regno > 255)
		return 1;

#if 0				/* JB: not drop, check that this works */
	if (fb->bits_per_pixel == 8) {
		list_for_each_entry(crtc, &dev->mode_config.crtc_list,
				    head) {
			for (i = 0; i < par->crtc_count; i++)
				if (crtc->base.id == par->crtc_ids[i])
					break;

			if (i == par->crtc_count)
				continue;

			if (crtc->funcs->gamma_set)
				crtc->funcs->gamma_set(crtc, red, green,
						       blue, regno);
		}
		return 0;
	}
#endif

	red = CMAP_TOHW(red, info->var.red.length);
	blue = CMAP_TOHW(blue, info->var.blue.length);
	green = CMAP_TOHW(green, info->var.green.length);
	transp = CMAP_TOHW(transp, info->var.transp.length);

	v = (red << info->var.red.offset) |
	    (green << info->var.green.offset) |
	    (blue << info->var.blue.offset) |
	    (transp << info->var.transp.offset);

	if (regno < 16) {
		switch (fb->bits_per_pixel) {
		case 16:
			((uint32_t *) info->pseudo_palette)[regno] = v;
			break;
		case 24:
		case 32:
			((uint32_t *) info->pseudo_palette)[regno] = v;
			break;
		}
	}

	return 0;
}

static struct drm_display_mode *psbfb_find_first_mode(struct
						      fb_var_screeninfo
						      *var,
						      struct fb_info *info,
						      struct drm_crtc
						      *crtc)
{
	struct psbfb_par *par = info->par;
	struct drm_device *dev = par->dev;
	struct drm_display_mode *drm_mode;
	struct drm_display_mode *last_mode = NULL;
	struct drm_connector *connector;
	int found;

	found = 0;
	list_for_each_entry(connector, &dev->mode_config.connector_list,
			    head) {
		if (connector->encoder && connector->encoder->crtc == crtc) {
			found = 1;
			break;
		}
	}

	/* found no connector, bail */
	if (!found)
		return NULL;

	found = 0;
	list_for_each_entry(drm_mode, &connector->modes, head) {
		if (drm_mode->hdisplay == var->xres &&
		    drm_mode->vdisplay == var->yres
		    && drm_mode->clock != 0) {
			found = 1;
			last_mode = drm_mode;
		}
	}

	/* No mode matching mode found */
	if (!found)
		return NULL;

	return last_mode;
}

static int psbfb_check_var(struct fb_var_screeninfo *var,
			   struct fb_info *info)
{
	struct psbfb_par *par = info->par;
	struct psb_framebuffer *psbfb = par->psbfb;
	struct drm_device *dev = par->dev;
	int ret;
	int depth;
	int pitch;
	int bpp = var->bits_per_pixel;

	if (!psbfb)
		return -ENOMEM;

	if (!var->pixclock)
		return -EINVAL;

	/* don't support virtuals for now */
	if (var->xres_virtual > var->xres)
		return -EINVAL;

	if (var->yres_virtual > var->yres)
		return -EINVAL;

	switch (bpp) {
#if 0				/* JB: for now only support true color */
	case 8:
		depth = 8;
		break;
#endif
	case 16:
		depth = (var->green.length == 6) ? 16 : 15;
		break;
	case 24:		/* assume this is 32bpp / depth 24 */
		bpp = 32;
		/* fallthrough */
	case 32:
		depth = (var->transp.length > 0) ? 32 : 24;
		break;
	default:
		return -EINVAL;
	}

	pitch = ((var->xres * ((bpp + 1) / 8)) + 0x3f) & ~0x3f;

	/* Check that we can resize */
	if ((pitch * var->yres) > (psbfb->bo->num_pages << PAGE_SHIFT)) {
#if 1
		/* Need to resize the fb object.
		 * But the generic fbdev code doesn't really understand
		 * that we can do this. So disable for now.
		 */
		DRM_INFO("Can't support requested size, too big!\n");
		return -EINVAL;
#else
		struct drm_psb_private *dev_priv = psb_priv(dev);
		struct ttm_bo_device *bdev = &dev_priv->bdev;
		struct ttm_buffer_object *fbo = NULL;
		struct ttm_bo_kmap_obj tmp_kmap;

		/* a temporary BO to check if we could resize in setpar.
		 * Therefore no need to set NO_EVICT.
		 */
		ret = ttm_buffer_object_create(bdev,
					       pitch * var->yres,
					       ttm_bo_type_kernel,
					       TTM_PL_FLAG_TT |
					       TTM_PL_FLAG_VRAM |
					       TTM_PL_FLAG_NO_EVICT,
					       0, 0, &fbo);
		if (ret || !fbo)
			return -ENOMEM;

		ret = ttm_bo_kmap(fbo, 0, fbo->num_pages, &tmp_kmap);
		if (ret) {
			ttm_bo_usage_deref_unlocked(&fbo);
			return -EINVAL;
		}

		ttm_bo_kunmap(&tmp_kmap);
		/* destroy our current fbo! */
		ttm_bo_usage_deref_unlocked(&fbo);
#endif
	}

	ret = fill_fb_bitfield(var, depth);
	if (ret)
		return ret;

#if 1
	/* Here we walk the output mode list and look for modes. If we haven't
	 * got it, then bail. Not very nice, so this is disabled.
	 * In the set_par code, we create our mode based on the incoming
	 * parameters. Nicer, but may not be desired by some.
	 */
	{
		struct drm_crtc *crtc;
		int i;

		list_for_each_entry(crtc, &dev->mode_config.crtc_list,
				    head) {
			struct intel_crtc *intel_crtc =
			    to_intel_crtc(crtc);

			for (i = 0; i < par->crtc_count; i++)
				if (crtc->base.id == par->crtc_ids[i])
					break;

			if (i == par->crtc_count)
				continue;

			if (intel_crtc->mode_set.num_connectors == 0)
				continue;

			if (!psbfb_find_first_mode(&info->var, info, crtc))
				return -EINVAL;
		}
	}
#else
	(void) i;
	(void) dev;		/* silence warnings */
	(void) crtc;
	(void) drm_mode;
	(void) connector;
#endif

	return 0;
}

/* this will let fbcon do the mode init */
static int psbfb_set_par(struct fb_info *info)
{
	struct psbfb_par *par = info->par;
	struct psb_framebuffer *psbfb = par->psbfb;
	struct drm_framebuffer *fb = &psbfb->base;
	struct drm_device *dev = par->dev;
	struct fb_var_screeninfo *var = &info->var;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_display_mode *drm_mode;
	int pitch;
	int depth;
	int bpp = var->bits_per_pixel;

	if (!fb)
		return -ENOMEM;

	switch (bpp) {
	case 8:
		depth = 8;
		break;
	case 16:
		depth = (var->green.length == 6) ? 16 : 15;
		break;
	case 24:		/* assume this is 32bpp / depth 24 */
		bpp = 32;
		/* fallthrough */
	case 32:
		depth = (var->transp.length > 0) ? 32 : 24;
		break;
	default:
		DRM_ERROR("Illegal BPP\n");
		return -EINVAL;
	}

	pitch = ((var->xres * ((bpp + 1) / 8)) + 0x3f) & ~0x3f;

	if ((pitch * var->yres) > (psbfb->bo->num_pages << PAGE_SHIFT)) {
#if 1
		/* Need to resize the fb object.
		 * But the generic fbdev code doesn't really understand
		 * that we can do this. So disable for now.
		 */
		DRM_INFO("Can't support requested size, too big!\n");
		return -EINVAL;
#else
		int ret;
		struct ttm_buffer_object *fbo = NULL, *tfbo;
		struct ttm_bo_kmap_obj tmp_kmap, tkmap;

		ret = ttm_buffer_object_create(bdev,
					       pitch * var->yres,
					       ttm_bo_type_kernel,
					       TTM_PL_FLAG_MEM_TT |
					       TTM_PL_FLAG_MEM_VRAM |
					       TTM_PL_FLAG_NO_EVICT,
					       0, 0, &fbo);
		if (ret || !fbo) {
			DRM_ERROR
			    ("failed to allocate new resized framebuffer\n");
			return -ENOMEM;
		}

		ret = ttm_bo_kmap(fbo, 0, fbo->num_pages, &tmp_kmap);
		if (ret) {
			DRM_ERROR("failed to kmap framebuffer.\n");
			ttm_bo_usage_deref_unlocked(&fbo);
			return -EINVAL;
		}

		DRM_DEBUG("allocated %dx%d fb: 0x%08lx, bo %p\n",
			  fb->width, fb->height, fb->offset, fbo);

		/* set new screen base */
		info->screen_base = tmp_kmap.virtual;

		tkmap = fb->kmap;
		fb->kmap = tmp_kmap;
		ttm_bo_kunmap(&tkmap);

		tfbo = fb->bo;
		fb->bo = fbo;
		ttm_bo_usage_deref_unlocked(&tfbo);
#endif
	}

	psbfb->offset = psbfb->bo->offset - dev_priv->pg->gatt_start;
	fb->width = var->xres;
	fb->height = var->yres;
	fb->bits_per_pixel = bpp;
	fb->pitch = pitch;
	fb->depth = depth;

	info->fix.line_length = psbfb->base.pitch;
	info->fix.visual =
	    (psbfb->base.depth ==
	     8) ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_DIRECTCOLOR;

	/* some fbdev's apps don't want these to change */
	info->fix.smem_start = dev->mode_config.fb_base + psbfb->offset;

#if 0
	/* relates to resize - disable */
	info->fix.smem_len = info->fix.line_length * var->yres;
	info->screen_size = info->fix.smem_len;	/* ??? */
#endif

	/* Should we walk the output's modelist or just create our own ???
	 * For now, we create and destroy a mode based on the incoming
	 * parameters. But there's commented out code below which scans
	 * the output list too.
	 */
#if 1
	/* This code is now in the for loop futher down. */
#endif

	{
		struct drm_crtc *crtc;
		int ret;
		int i;

		list_for_each_entry(crtc, &dev->mode_config.crtc_list,
				    head) {
			struct intel_crtc *intel_crtc =
			    to_intel_crtc(crtc);

			for (i = 0; i < par->crtc_count; i++)
				if (crtc->base.id == par->crtc_ids[i])
					break;

			if (i == par->crtc_count)
				continue;

			if (intel_crtc->mode_set.num_connectors == 0)
				continue;

#if 1
			drm_mode =
			    psbfb_find_first_mode(&info->var, info, crtc);
			if (!drm_mode)
				DRM_ERROR("No matching mode found\n");
			intel_crtc->mode_set.mode = drm_mode;
#endif

#if 0				/* FIXME: TH */
			if (crtc->fb == intel_crtc->mode_set.fb) {
#endif
				DRM_DEBUG
				    ("setting mode on crtc %p with id %u\n",
				     crtc, crtc->base.id);
				ret =
				    crtc->funcs->
				    set_config(&intel_crtc->mode_set);
				if (ret) {
					DRM_ERROR("Failed setting mode\n");
					return ret;
				}
#if 0
			}
#endif
		}
		DRM_DEBUG("Set par returned OK.\n");
		return 0;
	}

	return 0;
}

static int psbfb_2d_submit(struct drm_psb_private *dev_priv, uint32_t *cmdbuf,
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
		for (i = 0; i < submit_size; i += 4) {
			PSB_WSGX32(*cmdbuf++, PSB_SGX_2D_SLAVE_PORT + i);
		}
		(void)PSB_RSGX32(PSB_SGX_2D_SLAVE_PORT + i - 4);
	}
	return 0;
}

static int psb_accel_2d_fillrect(struct drm_psb_private *dev_priv,
				 uint32_t dst_offset, uint32_t dst_stride,
				 uint32_t dst_format, uint16_t dst_x,
				 uint16_t dst_y, uint16_t size_x,
				 uint16_t size_y, uint32_t fill)
{
	uint32_t buffer[10];
	uint32_t *buf;

	buf = buffer;

	*buf++ = PSB_2D_FENCE_BH;

	*buf++ =
	    PSB_2D_DST_SURF_BH | dst_format | (dst_stride <<
					       PSB_2D_DST_STRIDE_SHIFT);
	*buf++ = dst_offset;

	*buf++ =
	    PSB_2D_BLIT_BH |
	    PSB_2D_ROT_NONE |
	    PSB_2D_COPYORDER_TL2BR |
	    PSB_2D_DSTCK_DISABLE |
	    PSB_2D_SRCCK_DISABLE | PSB_2D_USE_FILL | PSB_2D_ROP3_PATCOPY;

	*buf++ = fill << PSB_2D_FILLCOLOUR_SHIFT;
	*buf++ =
	    (dst_x << PSB_2D_DST_XSTART_SHIFT) | (dst_y <<
						  PSB_2D_DST_YSTART_SHIFT);
	*buf++ =
	    (size_x << PSB_2D_DST_XSIZE_SHIFT) | (size_y <<
						  PSB_2D_DST_YSIZE_SHIFT);
	*buf++ = PSB_2D_FLUSH_BH;

	return psbfb_2d_submit(dev_priv, buffer, buf - buffer);
}

static void psbfb_fillrect_accel(struct fb_info *info,
				 const struct fb_fillrect *r)
{
	struct psbfb_par *par = info->par;
	struct psb_framebuffer *psbfb = par->psbfb;
	struct drm_framebuffer *fb = &psbfb->base;
	struct drm_psb_private *dev_priv = par->dev->dev_private;
	uint32_t offset;
	uint32_t stride;
	uint32_t format;

	if (!fb)
		return;

	offset = psbfb->offset;
	stride = fb->pitch;

	switch (fb->depth) {
	case 8:
		format = PSB_2D_DST_332RGB;
		break;
	case 15:
		format = PSB_2D_DST_555RGB;
		break;
	case 16:
		format = PSB_2D_DST_565RGB;
		break;
	case 24:
	case 32:
		/* this is wrong but since we don't do blending its okay */
		format = PSB_2D_DST_8888ARGB;
		break;
	default:
		/* software fallback */
		cfb_fillrect(info, r);
		return;
	}

	psb_accel_2d_fillrect(dev_priv,
			      offset, stride, format,
			      r->dx, r->dy, r->width, r->height, r->color);
}

static void psbfb_fillrect(struct fb_info *info,
			   const struct fb_fillrect *rect)
{
	struct psbfb_par *par = info->par;
	struct drm_device *dev = par->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;

	if (unlikely(info->state != FBINFO_STATE_RUNNING))
		return;

	if (info->flags & FBINFO_HWACCEL_DISABLED)
		return cfb_fillrect(info, rect);

	if (psb_2d_trylock(dev_priv)) {
		psb_check_power_state(dev, PSB_DEVICE_SGX);
		psbfb_fillrect_accel(info, rect);
		psb_2d_unlock(dev_priv);
		if (drm_psb_ospm && IS_MRST(dev))
			schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	} else
		cfb_fillrect(info, rect);
}

uint32_t psb_accel_2d_copy_direction(int xdir, int ydir)
{
	if (xdir < 0)
		return (ydir <
			 0) ? PSB_2D_COPYORDER_BR2TL :
			PSB_2D_COPYORDER_TR2BL;
	else
		return (ydir <
			 0) ? PSB_2D_COPYORDER_BL2TR :
			PSB_2D_COPYORDER_TL2BR;
}

/*
 * @srcOffset in bytes
 * @srcStride in bytes
 * @srcFormat psb 2D format defines
 * @dstOffset in bytes
 * @dstStride in bytes
 * @dstFormat psb 2D format defines
 * @srcX offset in pixels
 * @srcY offset in pixels
 * @dstX offset in pixels
 * @dstY offset in pixels
 * @sizeX of the copied area
 * @sizeY of the copied area
 */
static int psb_accel_2d_copy(struct drm_psb_private *dev_priv,
			     uint32_t src_offset, uint32_t src_stride,
			     uint32_t src_format, uint32_t dst_offset,
			     uint32_t dst_stride, uint32_t dst_format,
			     uint16_t src_x, uint16_t src_y,
			     uint16_t dst_x, uint16_t dst_y,
			     uint16_t size_x, uint16_t size_y)
{
	uint32_t blit_cmd;
	uint32_t buffer[10];
	uint32_t *buf;
	uint32_t direction;

	buf = buffer;

	direction =
	    psb_accel_2d_copy_direction(src_x - dst_x, src_y - dst_y);

	if (direction == PSB_2D_COPYORDER_BR2TL ||
	    direction == PSB_2D_COPYORDER_TR2BL) {
		src_x += size_x - 1;
		dst_x += size_x - 1;
	}
	if (direction == PSB_2D_COPYORDER_BR2TL ||
	    direction == PSB_2D_COPYORDER_BL2TR) {
		src_y += size_y - 1;
		dst_y += size_y - 1;
	}

	blit_cmd =
	    PSB_2D_BLIT_BH |
	    PSB_2D_ROT_NONE |
	    PSB_2D_DSTCK_DISABLE |
	    PSB_2D_SRCCK_DISABLE |
	    PSB_2D_USE_PAT | PSB_2D_ROP3_SRCCOPY | direction;

	*buf++ = PSB_2D_FENCE_BH;
	*buf++ =
	    PSB_2D_DST_SURF_BH | dst_format | (dst_stride <<
					       PSB_2D_DST_STRIDE_SHIFT);
	*buf++ = dst_offset;
	*buf++ =
	    PSB_2D_SRC_SURF_BH | src_format | (src_stride <<
					       PSB_2D_SRC_STRIDE_SHIFT);
	*buf++ = src_offset;
	*buf++ =
	    PSB_2D_SRC_OFF_BH | (src_x << PSB_2D_SRCOFF_XSTART_SHIFT) |
	    (src_y << PSB_2D_SRCOFF_YSTART_SHIFT);
	*buf++ = blit_cmd;
	*buf++ =
	    (dst_x << PSB_2D_DST_XSTART_SHIFT) | (dst_y <<
						  PSB_2D_DST_YSTART_SHIFT);
	*buf++ =
	    (size_x << PSB_2D_DST_XSIZE_SHIFT) | (size_y <<
						  PSB_2D_DST_YSIZE_SHIFT);
	*buf++ = PSB_2D_FLUSH_BH;

	return psbfb_2d_submit(dev_priv, buffer, buf - buffer);
}

static void psbfb_copyarea_accel(struct fb_info *info,
				 const struct fb_copyarea *a)
{
	struct psbfb_par *par = info->par;
	struct psb_framebuffer *psbfb = par->psbfb;
	struct drm_framebuffer *fb = &psbfb->base;
	struct drm_psb_private *dev_priv = par->dev->dev_private;
	uint32_t offset;
	uint32_t stride;
	uint32_t src_format;
	uint32_t dst_format;

	if (!fb)
		return;

	offset = psbfb->offset;
	stride = fb->pitch;

	switch (fb->depth) {
	case 8:
		src_format = PSB_2D_SRC_332RGB;
		dst_format = PSB_2D_DST_332RGB;
		break;
	case 15:
		src_format = PSB_2D_SRC_555RGB;
		dst_format = PSB_2D_DST_555RGB;
		break;
	case 16:
		src_format = PSB_2D_SRC_565RGB;
		dst_format = PSB_2D_DST_565RGB;
		break;
	case 24:
	case 32:
		/* this is wrong but since we don't do blending its okay */
		src_format = PSB_2D_SRC_8888ARGB;
		dst_format = PSB_2D_DST_8888ARGB;
		break;
	default:
		/* software fallback */
		cfb_copyarea(info, a);
		return;
	}

	psb_accel_2d_copy(dev_priv,
			  offset, stride, src_format,
			  offset, stride, dst_format,
			  a->sx, a->sy, a->dx, a->dy, a->width, a->height);
}

static void psbfb_copyarea(struct fb_info *info,
			   const struct fb_copyarea *region)
{
	struct psbfb_par *par = info->par;
	struct drm_device *dev = par->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;

	if (unlikely(info->state != FBINFO_STATE_RUNNING))
		return;

	if (info->flags & FBINFO_HWACCEL_DISABLED)
		return cfb_copyarea(info, region);

	if (psb_2d_trylock(dev_priv)) {
		psb_check_power_state(dev, PSB_DEVICE_SGX);
		psbfb_copyarea_accel(info, region);
		psb_2d_unlock(dev_priv);
		if (drm_psb_ospm && IS_MRST(dev))
			schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	} else
		cfb_copyarea(info, region);
}

void psbfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	if (unlikely(info->state != FBINFO_STATE_RUNNING))
		return;

	cfb_imageblit(info, image);
}

static void psbfb_onoff(struct fb_info *info, int dpms_mode)
{
	struct psbfb_par *par = info->par;
	struct drm_device *dev = par->dev;
	struct drm_crtc *crtc;
	struct drm_encoder *encoder;
	int i;

	/*
	 * For each CRTC in this fb, find all associated encoders
	 * and turn them off, then turn off the CRTC.
	 */
	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		struct drm_crtc_helper_funcs *crtc_funcs =
		    crtc->helper_private;

		for (i = 0; i < par->crtc_count; i++)
			if (crtc->base.id == par->crtc_ids[i])
				break;

		if (i == par->crtc_count)
			continue;

		if (dpms_mode == DRM_MODE_DPMS_ON)
			crtc_funcs->dpms(crtc, dpms_mode);

		/* Found a CRTC on this fb, now find encoders */
		list_for_each_entry(encoder,
				    &dev->mode_config.encoder_list, head) {
			if (encoder->crtc == crtc) {
				struct drm_encoder_helper_funcs
				*encoder_funcs;
				encoder_funcs = encoder->helper_private;
				encoder_funcs->dpms(encoder, dpms_mode);
			}
		}

		if (dpms_mode == DRM_MODE_DPMS_OFF)
			crtc_funcs->dpms(crtc, dpms_mode);
	}
}

static int psbfb_blank(int blank_mode, struct fb_info *info)
{
	struct psbfb_par *par = info->par;

	par->dpms_state = blank_mode;
	PSB_DEBUG_PM("psbfb_blank \n");
	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		psbfb_onoff(info, DRM_MODE_DPMS_ON);
		break;
	case FB_BLANK_NORMAL:
		psbfb_onoff(info, DRM_MODE_DPMS_STANDBY);
		break;
	case FB_BLANK_HSYNC_SUSPEND:
		psbfb_onoff(info, DRM_MODE_DPMS_STANDBY);
		break;
	case FB_BLANK_VSYNC_SUSPEND:
		psbfb_onoff(info, DRM_MODE_DPMS_SUSPEND);
		break;
	case FB_BLANK_POWERDOWN:
		psbfb_onoff(info, DRM_MODE_DPMS_OFF);
		break;
	}

	return 0;
}


static int psbfb_kms_off(struct drm_device *dev, int suspend)
{
	struct drm_framebuffer *fb = 0;
	DRM_DEBUG("psbfb_kms_off_ioctl\n");

	mutex_lock(&dev->mode_config.mutex);
	list_for_each_entry(fb, &dev->mode_config.fb_list, head) {
		struct fb_info *info = fb->fbdev;

		if (suspend)
			fb_set_suspend(info, 1);
	}
	mutex_unlock(&dev->mode_config.mutex);

	return 0;
}

int psbfb_kms_off_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	int ret;

	if (drm_psb_no_fb)
		return 0;
	acquire_console_sem();
	ret = psbfb_kms_off(dev, 0);
	release_console_sem();

	return ret;
}

static int psbfb_kms_on(struct drm_device *dev, int resume)
{
	struct drm_framebuffer *fb = 0;

	DRM_DEBUG("psbfb_kms_on_ioctl\n");

	mutex_lock(&dev->mode_config.mutex);
	list_for_each_entry(fb, &dev->mode_config.fb_list, head) {
		struct fb_info *info = fb->fbdev;

		if (resume)
			fb_set_suspend(info, 0);

	}
	mutex_unlock(&dev->mode_config.mutex);

	return 0;
}

int psbfb_kms_on_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	int ret;

	if (drm_psb_no_fb)
		return 0;
	acquire_console_sem();
	ret = psbfb_kms_on(dev, 0);
	release_console_sem();
	drm_helper_disable_unused_functions(dev);
	return ret;
}

void psbfb_suspend(struct drm_device *dev)
{
	acquire_console_sem();
	psbfb_kms_off(dev, 1);
	release_console_sem();
}

void psbfb_resume(struct drm_device *dev)
{
	acquire_console_sem();
	psbfb_kms_on(dev, 1);
	release_console_sem();
	drm_helper_disable_unused_functions(dev);
}

static int psbfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct psbfb_par *par = info->par;
	struct psb_framebuffer *psbfb = par->psbfb;
	struct ttm_buffer_object *bo = psbfb->bo;
	unsigned long size = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
	unsigned long offset = vma->vm_pgoff;

	if (vma->vm_pgoff != 0)
		return -EINVAL;
	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	if (offset + size > bo->num_pages)
		return -EINVAL;

	mutex_lock(&bo->mutex);
	if (!psbfb->addr_space)
		psbfb->addr_space = vma->vm_file->f_mapping;
	mutex_unlock(&bo->mutex);

	return ttm_fbdev_mmap(vma, bo);
}

int psbfb_sync(struct fb_info *info)
{
	struct psbfb_par *par = info->par;
	struct drm_psb_private *dev_priv = par->dev->dev_private;

	if (psb_2d_trylock(dev_priv)) {
		if (dev_priv->graphics_state == PSB_PWR_STATE_D0i0)
			psb_idle_2d(par->dev);
		psb_2d_unlock(dev_priv);
	} else
		udelay(5);

	return 0;
}

static struct fb_ops psbfb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = psbfb_check_var,
	.fb_set_par = psbfb_set_par,
	.fb_setcolreg = psbfb_setcolreg,
	.fb_fillrect = psbfb_fillrect,
	.fb_copyarea = psbfb_copyarea,
	.fb_imageblit = psbfb_imageblit,
	.fb_mmap = psbfb_mmap,
	.fb_sync = psbfb_sync,
	.fb_blank = psbfb_blank,
};

static struct drm_mode_set panic_mode;

int psbfb_panic(struct notifier_block *n, unsigned long ununsed,
		void *panic_str)
{
	DRM_ERROR("panic occurred, switching back to text console\n");
	drm_crtc_helper_set_config(&panic_mode);

	return 0;
}
EXPORT_SYMBOL(psbfb_panic);

static struct notifier_block paniced = {
	.notifier_call = psbfb_panic,
};


static struct drm_framebuffer *psb_framebuffer_create
			(struct drm_device *dev, struct drm_mode_fb_cmd *r,
			 void *mm_private)
{
	struct psb_framebuffer *fb;
	int ret;

	fb = kzalloc(sizeof(*fb), GFP_KERNEL);
	if (!fb)
		return NULL;

	ret = drm_framebuffer_init(dev, &fb->base, &psb_fb_funcs);

	if (ret)
		goto err;

	drm_helper_mode_fill_fb_struct(&fb->base, r);

	fb->bo = mm_private;

	return &fb->base;

err:
	kfree(fb);
	return NULL;
}

static struct drm_framebuffer *psb_user_framebuffer_create
			(struct drm_device *dev, struct drm_file *filp,
			 struct drm_mode_fb_cmd *r)
{
	struct ttm_buffer_object *bo = NULL;
	uint64_t size;

	bo = ttm_buffer_object_lookup(psb_fpriv(filp)->tfile, r->handle);
	if (!bo)
		return NULL;

	/* JB: TODO not drop, make smarter */
	size = ((uint64_t) bo->num_pages) << PAGE_SHIFT;
	if (size < r->width * r->height * 4)
		return NULL;

	/* JB: TODO not drop, refcount buffer */
	return psb_framebuffer_create(dev, r, bo);
}

int psbfb_create(struct drm_device *dev, uint32_t fb_width,
		 uint32_t fb_height, uint32_t surface_width,
		 uint32_t surface_height, struct psb_framebuffer **psbfb_p)
{
	struct fb_info *info;
	struct psbfb_par *par;
	struct drm_framebuffer *fb;
	struct psb_framebuffer *psbfb;
	struct ttm_bo_kmap_obj tmp_kmap;
	struct drm_mode_fb_cmd mode_cmd;
	struct device *device = &dev->pdev->dev;
	struct ttm_bo_device *bdev = &psb_priv(dev)->bdev;
	int size, aligned_size, ret;
	struct ttm_buffer_object *fbo = NULL;
	bool is_iomem;

	mode_cmd.width = surface_width;	/* crtc->desired_mode->hdisplay; */
	mode_cmd.height = surface_height; /* crtc->desired_mode->vdisplay; */

	mode_cmd.bpp = 32;
	mode_cmd.pitch = mode_cmd.width * ((mode_cmd.bpp + 1) / 8);
	mode_cmd.depth = 24;

	size = mode_cmd.pitch * mode_cmd.height;
	aligned_size = ALIGN(size, PAGE_SIZE);
	ret = ttm_buffer_object_create(bdev,
				       aligned_size,
				       ttm_bo_type_kernel,
				       TTM_PL_FLAG_TT |
				       TTM_PL_FLAG_VRAM |
				       TTM_PL_FLAG_NO_EVICT,
				       0, 0, 0, NULL, &fbo);

	if (unlikely(ret != 0)) {
		DRM_ERROR("failed to allocate framebuffer.\n");
		return -ENOMEM;
	}

	mutex_lock(&dev->struct_mutex);
	fb = psb_framebuffer_create(dev, &mode_cmd, fbo);
	if (!fb) {
		DRM_ERROR("failed to allocate fb.\n");
		ret = -ENOMEM;
		goto out_err0;
	}
	psbfb = to_psb_fb(fb);
	psbfb->bo = fbo;

	list_add(&fb->filp_head, &dev->mode_config.fb_kernel_list);
	info = framebuffer_alloc(sizeof(struct psbfb_par), device);
	if (!info) {
		ret = -ENOMEM;
		goto out_err1;
	}

	par = info->par;
	par->psbfb = psbfb;

	strcpy(info->fix.id, "psbfb");
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.type_aux = 0;
	info->fix.xpanstep = 1;	/* doing it in hw */
	info->fix.ypanstep = 1;	/* doing it in hw */
	info->fix.ywrapstep = 0;
	info->fix.accel = FB_ACCEL_I830;
	info->fix.type_aux = 0;

	info->flags = FBINFO_DEFAULT;

	info->fbops = &psbfb_ops;

	info->fix.line_length = fb->pitch;
	info->fix.smem_start =
	    dev->mode_config.fb_base + psbfb->bo->offset;
	info->fix.smem_len = size;

	info->flags = FBINFO_DEFAULT;

	ret = ttm_bo_kmap(psbfb->bo, 0, psbfb->bo->num_pages, &tmp_kmap);
	if (ret) {
		DRM_ERROR("error mapping fb: %d\n", ret);
		goto out_err2;
	}


	info->screen_base = ttm_kmap_obj_virtual(&tmp_kmap, &is_iomem);
	info->screen_size = size;

	if (is_iomem)
		memset_io(info->screen_base, 0, size);
	else
		memset(info->screen_base, 0, size);

	info->pseudo_palette = fb->pseudo_palette;
	info->var.xres_virtual = fb->width;
	info->var.yres_virtual = fb->height;
	info->var.bits_per_pixel = fb->bits_per_pixel;
	info->var.xoffset = 0;
	info->var.yoffset = 0;
	info->var.activate = FB_ACTIVATE_NOW;
	info->var.height = -1;
	info->var.width = -1;

	info->var.xres = fb_width;
	info->var.yres = fb_height;

	info->fix.mmio_start = pci_resource_start(dev->pdev, 0);
	info->fix.mmio_len = pci_resource_len(dev->pdev, 0);

	info->pixmap.size = 64 * 1024;
	info->pixmap.buf_align = 8;
	info->pixmap.access_align = 32;
	info->pixmap.flags = FB_PIXMAP_SYSTEM;
	info->pixmap.scan_align = 1;

	DRM_DEBUG("fb depth is %d\n", fb->depth);
	DRM_DEBUG("   pitch is %d\n", fb->pitch);
	fill_fb_bitfield(&info->var, fb->depth);

	fb->fbdev = info;

	par->dev = dev;

	/* To allow resizing without swapping buffers */
	printk(KERN_INFO"allocated %dx%d fb: 0x%08lx, bo %p\n",
	       psbfb->base.width,
	       psbfb->base.height, psbfb->bo->offset, psbfb->bo);

	if (psbfb_p)
		*psbfb_p = psbfb;

	mutex_unlock(&dev->struct_mutex);

	return 0;
out_err2:
	unregister_framebuffer(info);
out_err1:
	fb->funcs->destroy(fb);
out_err0:
	mutex_unlock(&dev->struct_mutex);
	ttm_bo_unref(&fbo);
	return ret;
}

static int psbfb_multi_fb_probe_crtc(struct drm_device *dev,
				     struct drm_crtc *crtc)
{
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	struct drm_framebuffer *fb = crtc->fb;
	struct psb_framebuffer *psbfb = to_psb_fb(crtc->fb);
	struct drm_connector *connector;
	struct fb_info *info;
	struct psbfb_par *par;
	struct drm_mode_set *modeset;
	unsigned int width, height;
	int new_fb = 0;
	int ret, i, conn_count;

	if (!drm_helper_crtc_in_use(crtc))
		return 0;

	if (!crtc->desired_mode)
		return 0;

	width = crtc->desired_mode->hdisplay;
	height = crtc->desired_mode->vdisplay;

	/* is there an fb bound to this crtc already */
	if (!intel_crtc->mode_set.fb) {
		ret =
		    psbfb_create(dev, width, height, width, height,
				 &psbfb);
		if (ret)
			return -EINVAL;
		new_fb = 1;
	} else {
		fb = intel_crtc->mode_set.fb;
		if ((fb->width < width) || (fb->height < height))
			return -EINVAL;
	}

	info = fb->fbdev;
	par = info->par;

	modeset = &intel_crtc->mode_set;
	modeset->fb = fb;
	conn_count = 0;
	list_for_each_entry(connector, &dev->mode_config.connector_list,
			    head) {
		if (connector->encoder)
			if (connector->encoder->crtc == modeset->crtc) {
				modeset->connectors[conn_count] =
				    connector;
				conn_count++;
				if (conn_count > INTELFB_CONN_LIMIT)
					BUG();
			}
	}

	for (i = conn_count; i < INTELFB_CONN_LIMIT; i++)
		modeset->connectors[i] = NULL;

	par->crtc_ids[0] = crtc->base.id;

	modeset->num_connectors = conn_count;
	if (modeset->mode != modeset->crtc->desired_mode)
		modeset->mode = modeset->crtc->desired_mode;

	par->crtc_count = 1;

	if (new_fb) {
		info->var.pixclock = -1;
		if (register_framebuffer(info) < 0)
			return -EINVAL;
	} else
		psbfb_set_par(info);

	printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node,
	       info->fix.id);

	/* Switch back to kernel console on panic */
	panic_mode = *modeset;
	atomic_notifier_chain_register(&panic_notifier_list, &paniced);
	printk(KERN_INFO "registered panic notifier\n");

	return 0;
}

static int psbfb_multi_fb_probe(struct drm_device *dev)
{

	struct drm_crtc *crtc;
	int ret = 0;

	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		ret = psbfb_multi_fb_probe_crtc(dev, crtc);
		if (ret)
			return ret;
	}
	return ret;
}

static int psbfb_single_fb_probe(struct drm_device *dev)
{
	struct drm_crtc *crtc;
	struct drm_connector *connector;
	unsigned int fb_width = (unsigned) -1, fb_height = (unsigned) -1;
	unsigned int surface_width = 0, surface_height = 0;
	int new_fb = 0;
	int crtc_count = 0;
	int ret, i, conn_count = 0;
	struct fb_info *info;
	struct psbfb_par *par;
	struct drm_mode_set *modeset = NULL;
	struct drm_framebuffer *fb = NULL;
	struct psb_framebuffer *psbfb = NULL;

	/* first up get a count of crtcs now in use and
	 * new min/maxes width/heights */
	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		if (drm_helper_crtc_in_use(crtc)) {
			if (crtc->desired_mode) {
				fb = crtc->fb;
				if (crtc->desired_mode->hdisplay <
				    fb_width)
					fb_width =
					    crtc->desired_mode->hdisplay;

				if (crtc->desired_mode->vdisplay <
				    fb_height)
					fb_height =
					    crtc->desired_mode->vdisplay;

				if (crtc->desired_mode->hdisplay >
				    surface_width)
					surface_width =
					    crtc->desired_mode->hdisplay;

				if (crtc->desired_mode->vdisplay >
				    surface_height)
					surface_height =
					    crtc->desired_mode->vdisplay;

			}
			crtc_count++;
		}
	}

	if (crtc_count == 0 || fb_width == -1 || fb_height == -1) {
		/* hmm everyone went away - assume VGA cable just fell out
		   and will come back later. */
		return 0;
	}

	/* do we have an fb already? */
	if (list_empty(&dev->mode_config.fb_kernel_list)) {
		/* create an fb if we don't have one */
		ret =
		    psbfb_create(dev, fb_width, fb_height, surface_width,
				 surface_height, &psbfb);
		if (ret)
			return -EINVAL;
		new_fb = 1;
		fb = &psbfb->base;
	} else {
		fb = list_first_entry(&dev->mode_config.fb_kernel_list,
				      struct drm_framebuffer, filp_head);

		/* if someone hotplugs something bigger than we have already
		 * allocated, we are pwned. As really we can't resize an
		 * fbdev that is in the wild currently due to fbdev not really
		 * being designed for the lower layers moving stuff around
		 * under it. - so in the grand style of things - punt. */
		if ((fb->width < surface_width)
		    || (fb->height < surface_height)) {
			DRM_ERROR
			("Framebuffer not large enough to scale"
			 " console onto.\n");
			return -EINVAL;
		}
	}

	info = fb->fbdev;
	par = info->par;

	crtc_count = 0;
	/* okay we need to setup new connector sets in the crtcs */
	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
		modeset = &intel_crtc->mode_set;
		modeset->fb = fb;
		conn_count = 0;
		list_for_each_entry(connector,
				    &dev->mode_config.connector_list,
				    head) {
			if (connector->encoder)
				if (connector->encoder->crtc ==
				    modeset->crtc) {
					modeset->connectors[conn_count] =
					    connector;
					conn_count++;
					if (conn_count >
					    INTELFB_CONN_LIMIT)
						BUG();
				}
		}

		for (i = conn_count; i < INTELFB_CONN_LIMIT; i++)
			modeset->connectors[i] = NULL;

		par->crtc_ids[crtc_count++] = crtc->base.id;

		modeset->num_connectors = conn_count;
		if (modeset->mode != modeset->crtc->desired_mode)
			modeset->mode = modeset->crtc->desired_mode;
	}
	par->crtc_count = crtc_count;

	if (new_fb) {
		info->var.pixclock = -1;
		if (register_framebuffer(info) < 0)
			return -EINVAL;
	} else
		psbfb_set_par(info);

	printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node,
	       info->fix.id);

	/* Switch back to kernel console on panic */
	panic_mode = *modeset;
	atomic_notifier_chain_register(&panic_notifier_list, &paniced);
	printk(KERN_INFO "registered panic notifier\n");

	return 0;
}

int psbfb_probe(struct drm_device *dev)
{
	int ret = 0;

	DRM_DEBUG("\n");

	/* something has changed in the lower levels of hell - deal with it
	   here */

	/* two modes : a) 1 fb to rule all crtcs.
	   b) one fb per crtc.
	   two actions 1) new connected device
	   2) device removed.
	   case a/1 : if the fb surface isn't big enough -
	   resize the surface fb.
	   if the fb size isn't big enough - resize fb into surface.
	   if everything big enough configure the new crtc/etc.
	   case a/2 : undo the configuration
	   possibly resize down the fb to fit the new configuration.
	   case b/1 : see if it is on a new crtc - setup a new fb and add it.
	   case b/2 : teardown the new fb.
	 */

	/* mode a first */
	/* search for an fb */
	if (0 /*i915_fbpercrtc == 1 */)
		ret = psbfb_multi_fb_probe(dev);
	else
		ret = psbfb_single_fb_probe(dev);

	return ret;
}
EXPORT_SYMBOL(psbfb_probe);

int psbfb_remove(struct drm_device *dev, struct drm_framebuffer *fb)
{
	struct fb_info *info;
	struct psb_framebuffer *psbfb = to_psb_fb(fb);

	if (drm_psb_no_fb)
		return 0;

	info = fb->fbdev;

	if (info) {
		unregister_framebuffer(info);
		ttm_bo_kunmap(&psbfb->kmap);
		ttm_bo_unref(&psbfb->bo);
		framebuffer_release(info);
	}

	atomic_notifier_chain_unregister(&panic_notifier_list, &paniced);
	memset(&panic_mode, 0, sizeof(struct drm_mode_set));
	return 0;
}
EXPORT_SYMBOL(psbfb_remove);

static int psb_user_framebuffer_create_handle(struct drm_framebuffer *fb,
					      struct drm_file *file_priv,
					      unsigned int *handle)
{
	/* JB: TODO currently we can't go from a bo to a handle with ttm */
	(void) file_priv;
	*handle = 0;
	return 0;
}

static void psb_user_framebuffer_destroy(struct drm_framebuffer *fb)
{
	struct drm_device *dev = fb->dev;
	if (fb->fbdev)
		psbfb_remove(dev, fb);

	/* JB: TODO not drop, refcount buffer */
	drm_framebuffer_cleanup(fb);

	kfree(fb);
}

static const struct drm_mode_config_funcs psb_mode_funcs = {
	.fb_create = psb_user_framebuffer_create,
	.fb_changed = psbfb_probe,
};

static void psb_setup_outputs(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct drm_connector *connector;

	if (IS_MRST(dev)) {
		if (dev_priv->iLVDS_enable)
			/* Set up integrated LVDS for MRST */
			mrst_lvds_init(dev, &dev_priv->mode_dev);
		else {
			/* Set up integrated MIPI for MRST */
			mrst_dsi_init(dev, &dev_priv->mode_dev);
		}
	} else {
		intel_lvds_init(dev, &dev_priv->mode_dev);
		/* intel_sdvo_init(dev, SDVOB); */
	}

	list_for_each_entry(connector, &dev->mode_config.connector_list,
			    head) {
		struct intel_output *intel_output =
		    to_intel_output(connector);
		struct drm_encoder *encoder = &intel_output->enc;
		int crtc_mask = 0, clone_mask = 0;

		/* valid crtcs */
		switch (intel_output->type) {
		case INTEL_OUTPUT_SDVO:
			crtc_mask = ((1 << 0) | (1 << 1));
			clone_mask = (1 << INTEL_OUTPUT_SDVO);
			break;
		case INTEL_OUTPUT_LVDS:
			if (IS_MRST(dev))
				crtc_mask = (1 << 0);
			else
				crtc_mask = (1 << 1);

			clone_mask = (1 << INTEL_OUTPUT_LVDS);
			break;
		case INTEL_OUTPUT_MIPI:
			crtc_mask = (1 << 0);
			clone_mask = (1 << INTEL_OUTPUT_MIPI);
			break;
		}
		encoder->possible_crtcs = crtc_mask;
		encoder->possible_clones =
		    intel_connector_clones(dev, clone_mask);
	}
}

static void *psb_bo_from_handle(struct drm_device *dev,
				struct drm_file *file_priv,
				unsigned int handle)
{
	return ttm_buffer_object_lookup(psb_fpriv(file_priv)->tfile,
					handle);
}

static size_t psb_bo_size(struct drm_device *dev, void *bof)
{
	struct ttm_buffer_object *bo = bof;
	return bo->num_pages << PAGE_SHIFT;
}

static size_t psb_bo_offset(struct drm_device *dev, void *bof)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_buffer_object *bo = bof;

	size_t offset = bo->offset - dev_priv->pg->gatt_start;
	DRM_DEBUG("Offset %u\n", offset);
	return offset;
}

static int psb_bo_pin_for_scanout(struct drm_device *dev, void *bo)
{
#if 0				/* JB: Not used for the drop */
	struct ttm_buffer_object *bo = bof;
		We should do things like check if
		the buffer is in a scanout : able
		    place.And make sure that its pinned.
#endif
		 return 0;
		}

	static int psb_bo_unpin_for_scanout(struct drm_device *dev,
						    void *bo) {
#if 0				/* JB: Not used for the drop */
		struct ttm_buffer_object *bo = bof;
#endif
		return 0;
	}

	void psb_modeset_init(struct drm_device *dev)
	{
		struct drm_psb_private *dev_priv =
		    (struct drm_psb_private *) dev->dev_private;
		struct intel_mode_device *mode_dev = &dev_priv->mode_dev;
		int i;
		int num_pipe;

		/* Init mm functions */
		mode_dev->bo_from_handle = psb_bo_from_handle;
		mode_dev->bo_size = psb_bo_size;
		mode_dev->bo_offset = psb_bo_offset;
		mode_dev->bo_pin_for_scanout = psb_bo_pin_for_scanout;
		mode_dev->bo_unpin_for_scanout = psb_bo_unpin_for_scanout;

		drm_mode_config_init(dev);

		dev->mode_config.min_width = 0;
		dev->mode_config.min_height = 0;

		dev->mode_config.funcs = (void *) &psb_mode_funcs;

		dev->mode_config.max_width = 2048;
		dev->mode_config.max_height = 2048;

		/* set memory base */
		dev->mode_config.fb_base =
		    pci_resource_start(dev->pdev, 0);

		if (IS_MRST(dev))
			num_pipe = 1;
		else
			num_pipe = 2;


		for (i = 0; i < num_pipe; i++)
			intel_crtc_init(dev, i, mode_dev);

		psb_setup_outputs(dev);

		/* setup fbs */
		/* drm_initial_config(dev, false); */
	}

	void psb_modeset_cleanup(struct drm_device *dev)
	{
		drm_mode_config_cleanup(dev);
	}
