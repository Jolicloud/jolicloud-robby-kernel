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

#include "drmP.h"
#include "drm.h"
#include "drm_crtc.h"
#include "psb_drv.h"
#include "drm_compat.h"

#define SII_1392_WA
#ifdef SII_1392_WA
extern int SII_1392;
#endif

struct psbfb_vm_info {
	struct drm_buffer_object *bo;
	struct address_space *f_mapping;
	struct mutex vm_mutex;
	atomic_t refcount;
};

struct psbfb_par {
	struct drm_device *dev;
	struct drm_crtc *crtc;
	struct drm_output *output;
	struct psbfb_vm_info *vi;
	int dpms_state;
};

static void psbfb_vm_info_deref(struct psbfb_vm_info **vi)
{
	struct psbfb_vm_info *tmp = *vi;
	*vi = NULL;
	if (atomic_dec_and_test(&tmp->refcount)) {
		drm_bo_usage_deref_unlocked(&tmp->bo);
		drm_free(tmp, sizeof(*tmp), DRM_MEM_MAPS);
	}
}

static struct psbfb_vm_info *psbfb_vm_info_ref(struct psbfb_vm_info *vi)
{
	atomic_inc(&vi->refcount);
	return vi;
}

static struct psbfb_vm_info *psbfb_vm_info_create(void)
{
	struct psbfb_vm_info *vi;

	vi = drm_calloc(1, sizeof(*vi), DRM_MEM_MAPS);
	if (!vi)
		return NULL;

	mutex_init(&vi->vm_mutex);
	atomic_set(&vi->refcount, 1);
	return vi;
}

#define CMAP_TOHW(_val, _width) ((((_val) << (_width)) + 0x7FFF - (_val)) >> 16)

static int psbfb_setcolreg(unsigned regno, unsigned red, unsigned green,
			   unsigned blue, unsigned transp, struct fb_info *info)
{
	struct psbfb_par *par = info->par;
	struct drm_crtc *crtc = par->crtc;
	uint32_t v;

	if (!crtc->fb)
		return -ENOMEM;

	if (regno > 15)
		return 1;

	if (crtc->funcs->gamma_set)
		crtc->funcs->gamma_set(crtc, red, green, blue, regno);

	red = CMAP_TOHW(red, info->var.red.length);
	blue = CMAP_TOHW(blue, info->var.blue.length);
	green = CMAP_TOHW(green, info->var.green.length);
	transp = CMAP_TOHW(transp, info->var.transp.length);

	v = (red << info->var.red.offset) |
	    (green << info->var.green.offset) |
	    (blue << info->var.blue.offset) |
	    (transp << info->var.transp.offset);

	switch (crtc->fb->bits_per_pixel) {
	case 16:
		((uint32_t *) info->pseudo_palette)[regno] = v;
		break;
	case 24:
	case 32:
		((uint32_t *) info->pseudo_palette)[regno] = v;
		break;
	}

	return 0;
}

static int psbfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct psbfb_par *par = info->par;
	struct drm_device *dev = par->dev;
	struct drm_framebuffer *fb = par->crtc->fb;
	struct drm_display_mode *drm_mode;
	struct drm_output *output;
	int depth;
	int pitch;
	int bpp = var->bits_per_pixel;

	if (!fb)
		return -ENOMEM;

	if (!var->pixclock)
		return -EINVAL;

	/* don't support virtuals for now */
	if (var->xres_virtual > var->xres)
		return -EINVAL;

	if (var->yres_virtual > var->yres)
		return -EINVAL;

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
		return -EINVAL;
	}

	pitch = ((var->xres * ((bpp + 1) / 8)) + 0x3f) & ~0x3f;

	/* Check that we can resize */
	if ((pitch * var->yres) > (fb->bo->num_pages << PAGE_SHIFT)) {
#if 1
		/* Need to resize the fb object.
		 * But the generic fbdev code doesn't really understand
		 * that we can do this. So disable for now.
		 */
		DRM_INFO("Can't support requested size, too big!\n");
		return -EINVAL;
#else
		int ret;
		struct drm_buffer_object *fbo = NULL;
		struct drm_bo_kmap_obj tmp_kmap;

		/* a temporary BO to check if we could resize in setpar.
		 * Therefore no need to set NO_EVICT.
		 */
		ret = drm_buffer_object_create(dev,
					       pitch * var->yres,
					       drm_bo_type_kernel,
					       DRM_BO_FLAG_READ |
					       DRM_BO_FLAG_WRITE |
					       DRM_BO_FLAG_MEM_TT |
					       DRM_BO_FLAG_MEM_VRAM,
					       DRM_BO_HINT_DONT_FENCE,
					       0, 0, &fbo);
		if (ret || !fbo)
			return -ENOMEM;

		ret = drm_bo_kmap(fbo, 0, fbo->num_pages, &tmp_kmap);
		if (ret) {
			drm_bo_usage_deref_unlocked(&fbo);
			return -EINVAL;
		}

		drm_bo_kunmap(&tmp_kmap);
		/* destroy our current fbo! */
		drm_bo_usage_deref_unlocked(&fbo);
#endif
	}

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

#if 0
	/* Here we walk the output mode list and look for modes. If we haven't
	 * got it, then bail. Not very nice, so this is disabled.
	 * In the set_par code, we create our mode based on the incoming
	 * parameters. Nicer, but may not be desired by some.
	 */
	list_for_each_entry(output, &dev->mode_config.output_list, head) {
		if (output->crtc == par->crtc)
			break;
	}

	list_for_each_entry(drm_mode, &output->modes, head) {
		if (drm_mode->hdisplay == var->xres &&
		    drm_mode->vdisplay == var->yres && drm_mode->clock != 0)
			break;
	}

	if (!drm_mode)
		return -EINVAL;
#else
	(void)dev;		/* silence warnings */
	(void)output;
	(void)drm_mode;
#endif

	return 0;
}

static int psbfb_move_fb_bo(struct fb_info *info, struct drm_buffer_object *bo,
			    uint64_t mem_type_flags)
{
	struct psbfb_par *par;
	loff_t holelen;
	int ret;

	/*
	 * Kill all user-space mappings of this device. They will be
	 * faulted back using nopfn when accessed.
	 */

	par = info->par;
	holelen = ((loff_t) bo->mem.num_pages) << PAGE_SHIFT;
	mutex_lock(&par->vi->vm_mutex);
	if (par->vi->f_mapping) {
		unmap_mapping_range(par->vi->f_mapping, 0, holelen, 1);
	}

	ret = drm_bo_do_validate(bo,
				 mem_type_flags,
				 DRM_BO_MASK_MEM |
				 DRM_BO_FLAG_NO_EVICT,
				 DRM_BO_HINT_DONT_FENCE, 0, 1, NULL);

	mutex_unlock(&par->vi->vm_mutex);
	return ret;
}

/* this will let fbcon do the mode init */
static int psbfb_set_par(struct fb_info *info)
{
	struct psbfb_par *par = info->par;
	struct drm_framebuffer *fb = par->crtc->fb;
	struct drm_device *dev = par->dev;
	struct drm_display_mode *drm_mode;
	struct fb_var_screeninfo *var = &info->var;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_output *output;
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
		return -EINVAL;
	}

	pitch = ((var->xres * ((bpp + 1) / 8)) + 0x3f) & ~0x3f;

	if ((pitch * var->yres) > (fb->bo->num_pages << PAGE_SHIFT)) {
#if 1
		/* Need to resize the fb object.
		 * But the generic fbdev code doesn't really understand
		 * that we can do this. So disable for now.
		 */
		DRM_INFO("Can't support requested size, too big!\n");
		return -EINVAL;
#else
		int ret;
		struct drm_buffer_object *fbo = NULL, *tfbo;
		struct drm_bo_kmap_obj tmp_kmap, tkmap;

		ret = drm_buffer_object_create(dev,
					       pitch * var->yres,
					       drm_bo_type_kernel,
					       DRM_BO_FLAG_READ |
					       DRM_BO_FLAG_WRITE |
					       DRM_BO_FLAG_MEM_TT |
					       DRM_BO_FLAG_MEM_VRAM |
					       DRM_BO_FLAG_NO_EVICT,
					       DRM_BO_HINT_DONT_FENCE,
					       0, 0, &fbo);
		if (ret || !fbo) {
			DRM_ERROR
			    ("failed to allocate new resized framebuffer\n");
			return -ENOMEM;
		}

		ret = drm_bo_kmap(fbo, 0, fbo->num_pages, &tmp_kmap);
		if (ret) {
			DRM_ERROR("failed to kmap framebuffer.\n");
			drm_bo_usage_deref_unlocked(&fbo);
			return -EINVAL;
		}

		DRM_DEBUG("allocated %dx%d fb: 0x%08lx, bo %p\n", fb->width,
			  fb->height, fb->offset, fbo);

		/* set new screen base */
		info->screen_base = tmp_kmap.virtual;

		tkmap = fb->kmap;
		fb->kmap = tmp_kmap;
		drm_bo_kunmap(&tkmap);

		tfbo = fb->bo;
		fb->bo = fbo;
		drm_bo_usage_deref_unlocked(&tfbo);
#endif
	}

	fb->offset = fb->bo->offset - dev_priv->pg->gatt_start;
	fb->width = var->xres;
	fb->height = var->yres;
	fb->bits_per_pixel = bpp;
	fb->pitch = pitch;
	fb->depth = depth;

	info->fix.line_length = fb->pitch;
	info->fix.visual =
	    (fb->depth == 8) ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_DIRECTCOLOR;

	/* some fbdev's apps don't want these to change */
	info->fix.smem_start = dev->mode_config.fb_base + fb->offset;

	/* we have to align the output base address because the fb->bo
	   may be moved in the previous drm_bo_do_validate().
	   Otherwise the output screens may go black when exit the X
	   window and re-enter the console */
	info->screen_base = fb->kmap.virtual;

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
#if 0
	list_for_each_entry(output, &dev->mode_config.output_list, head) {
		if (output->crtc == par->crtc)
			break;
	}

	list_for_each_entry(drm_mode, &output->modes, head) {
		if (drm_mode->hdisplay == var->xres &&
		    drm_mode->vdisplay == var->yres && drm_mode->clock != 0)
			break;
	}
#else
	(void)output;		/* silence warning */

	drm_mode = drm_mode_create(dev);
	drm_mode->hdisplay = var->xres;
	drm_mode->hsync_start = drm_mode->hdisplay + var->right_margin;
	drm_mode->hsync_end = drm_mode->hsync_start + var->hsync_len;
	drm_mode->htotal = drm_mode->hsync_end + var->left_margin;
	drm_mode->vdisplay = var->yres;
	drm_mode->vsync_start = drm_mode->vdisplay + var->lower_margin;
	drm_mode->vsync_end = drm_mode->vsync_start + var->vsync_len;
	drm_mode->vtotal = drm_mode->vsync_end + var->upper_margin;
	drm_mode->clock = PICOS2KHZ(var->pixclock);
	drm_mode->vrefresh = drm_mode_vrefresh(drm_mode);
	drm_mode_set_name(drm_mode);
	drm_mode_set_crtcinfo(drm_mode, CRTC_INTERLACE_HALVE_V);
#endif

	if (!drm_crtc_set_mode(par->crtc, drm_mode, 0, 0))
		return -EINVAL;

	/* Have to destroy our created mode if we're not searching the mode
	 * list for it.
	 */
#if 1
	drm_mode_destroy(dev, drm_mode);
#endif

	return 0;
}

extern int psb_2d_submit(struct drm_psb_private *, uint32_t *, uint32_t);;

static int psb_accel_2d_fillrect(struct drm_psb_private *dev_priv,
				 uint32_t dst_offset, uint32_t dst_stride,
				 uint32_t dst_format, uint16_t dst_x,
				 uint16_t dst_y, uint16_t size_x,
				 uint16_t size_y, uint32_t fill)
{
	uint32_t buffer[10];
	uint32_t *buf;
	int ret;

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

	psb_2d_lock(dev_priv);
	ret = psb_2d_submit(dev_priv, buffer, buf - buffer);
	psb_2d_unlock(dev_priv);

	return ret;
}

static void psbfb_fillrect_accel(struct fb_info *info,
				 const struct fb_fillrect *r)
{
	struct psbfb_par *par = info->par;
	struct drm_framebuffer *fb = par->crtc->fb;
	struct drm_psb_private *dev_priv = par->dev->dev_private;
	uint32_t offset;
	uint32_t stride;
	uint32_t format;

	if (!fb)
		return;

	offset = fb->offset;
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

static void psbfb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	if (info->state != FBINFO_STATE_RUNNING)
		return;
	if (info->flags & FBINFO_HWACCEL_DISABLED) {
		cfb_fillrect(info, rect);
		return;
	}
	if (in_interrupt() || in_atomic()) {
		/*
		 * Catch case when we're shutting down.
		 */
		cfb_fillrect(info, rect);
		return;
	}
	psbfb_fillrect_accel(info, rect);
}

uint32_t psb_accel_2d_copy_direction(int xdir, int ydir)
{
	if (xdir < 0)
		return ((ydir <
			 0) ? PSB_2D_COPYORDER_BR2TL : PSB_2D_COPYORDER_TR2BL);
	else
		return ((ydir <
			 0) ? PSB_2D_COPYORDER_BL2TR : PSB_2D_COPYORDER_TL2BR);
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
			     uint16_t src_x, uint16_t src_y, uint16_t dst_x,
			     uint16_t dst_y, uint16_t size_x, uint16_t size_y)
{
	uint32_t blit_cmd;
	uint32_t buffer[10];
	uint32_t *buf;
	uint32_t direction;
	int ret;

	buf = buffer;

	direction = psb_accel_2d_copy_direction(src_x - dst_x, src_y - dst_y);

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
	    PSB_2D_SRC_OFF_BH | (src_x << PSB_2D_SRCOFF_XSTART_SHIFT) | (src_y
									 <<
									 PSB_2D_SRCOFF_YSTART_SHIFT);
	*buf++ = blit_cmd;
	*buf++ =
	    (dst_x << PSB_2D_DST_XSTART_SHIFT) | (dst_y <<
						  PSB_2D_DST_YSTART_SHIFT);
	*buf++ =
	    (size_x << PSB_2D_DST_XSIZE_SHIFT) | (size_y <<
						  PSB_2D_DST_YSIZE_SHIFT);
	*buf++ = PSB_2D_FLUSH_BH;

	psb_2d_lock(dev_priv);
	ret = psb_2d_submit(dev_priv, buffer, buf - buffer);
	psb_2d_unlock(dev_priv);
	return ret;
}

static void psbfb_copyarea_accel(struct fb_info *info,
				 const struct fb_copyarea *a)
{
	struct psbfb_par *par = info->par;
	struct drm_framebuffer *fb = par->crtc->fb;
	struct drm_psb_private *dev_priv = par->dev->dev_private;
	uint32_t offset;
	uint32_t stride;
	uint32_t src_format;
	uint32_t dst_format;

	if (!fb)
		return;

	offset = fb->offset;
	stride = fb->pitch;

	if (a->width == 8 || a->height == 8) {
		psb_2d_lock(dev_priv);
		psb_idle_2d(par->dev);
		psb_2d_unlock(dev_priv);
		cfb_copyarea(info, a);
		return;
	}

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
	if (info->state != FBINFO_STATE_RUNNING)
		return;
	if (info->flags & FBINFO_HWACCEL_DISABLED) {
		cfb_copyarea(info, region);
		return;
	}
	if (in_interrupt() || in_atomic()) {
		/*
		 * Catch case when we're shutting down.
		 */
		cfb_copyarea(info, region);
		return;
	}

	psbfb_copyarea_accel(info, region);
}

void psbfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	if (info->state != FBINFO_STATE_RUNNING)
		return;
	if (info->flags & FBINFO_HWACCEL_DISABLED) {
		cfb_imageblit(info, image);
		return;
	}
	if (in_interrupt() || in_atomic()) {
		cfb_imageblit(info, image);
		return;
	}

	cfb_imageblit(info, image);
}

static int psbfb_blank(int blank_mode, struct fb_info *info)
{
	int dpms_mode;
	struct psbfb_par *par = info->par;
	struct drm_output *output;

	par->dpms_state = blank_mode;

	switch(blank_mode) {
	case FB_BLANK_UNBLANK:
		dpms_mode = DPMSModeOn;
		break;
	case FB_BLANK_NORMAL:
		if (!par->crtc)
			return 0;
		(*par->crtc->funcs->dpms)(par->crtc, DPMSModeStandby);
		return 0;
	case FB_BLANK_HSYNC_SUSPEND:
	default:
		dpms_mode = DPMSModeStandby;
		break;
	case FB_BLANK_VSYNC_SUSPEND:
		dpms_mode = DPMSModeSuspend;
		break;
	case FB_BLANK_POWERDOWN:
		dpms_mode = DPMSModeOff;
		break;
	}

	if (!par->crtc)
		return 0;

	list_for_each_entry(output, &par->dev->mode_config.output_list, head) {
		if (output->crtc == par->crtc)
			(*output->funcs->dpms)(output, dpms_mode);
	}

	(*par->crtc->funcs->dpms)(par->crtc, dpms_mode);
	return 0;
}


static int psbfb_kms_off(struct drm_device *dev, int suspend)
{
	struct drm_framebuffer *fb = 0;
	struct drm_buffer_object *bo = 0;
	struct drm_psb_private *dev_priv = dev->dev_private;
	int ret = 0;

	DRM_DEBUG("psbfb_kms_off_ioctl\n");

	mutex_lock(&dev->mode_config.mutex);
	list_for_each_entry(fb, &dev->mode_config.fb_list, head) {
		struct fb_info *info = fb->fbdev;
		struct psbfb_par *par = info->par;
		int save_dpms_state;

		if (suspend)
			fb_set_suspend(info, 1);
		else
			info->state &= ~FBINFO_STATE_RUNNING;

		info->screen_base = NULL;

		bo = fb->bo;

		if (!bo)
			continue;

		drm_bo_kunmap(&fb->kmap);

		/*
		 * We don't take the 2D lock here as we assume that the
		 * 2D engine will eventually idle anyway.
		 */

		if (!suspend) {
			uint32_t dummy2 = 0;
			(void) psb_fence_emit_sequence(dev, PSB_ENGINE_2D, 0,
							&dummy2, &dummy2);
			psb_2d_lock(dev_priv);
			(void)psb_idle_2d(dev);
			psb_2d_unlock(dev_priv);
		} else
			psb_idle_2d(dev);

		save_dpms_state = par->dpms_state;
		psbfb_blank(FB_BLANK_NORMAL, info);
		par->dpms_state = save_dpms_state;

		ret = psbfb_move_fb_bo(info, bo, DRM_BO_FLAG_MEM_LOCAL);

		if (ret)
			goto out_err;
	}
      out_err:
	mutex_unlock(&dev->mode_config.mutex);

	return ret;
}

int psbfb_kms_off_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	int ret;

	acquire_console_sem();
	ret = psbfb_kms_off(dev, 0);
	release_console_sem();

	return ret;
}

static int psbfb_kms_on(struct drm_device *dev, int resume)
{
	struct drm_framebuffer *fb = 0;
	struct drm_buffer_object *bo = 0;
	struct drm_psb_private *dev_priv = dev->dev_private;
	int ret = 0;
	int dummy;

	DRM_DEBUG("psbfb_kms_on_ioctl\n");

	if (!resume) {
		uint32_t dummy2 = 0;
		(void) psb_fence_emit_sequence(dev, PSB_ENGINE_2D, 0,
						       &dummy2, &dummy2);
		psb_2d_lock(dev_priv);
		(void)psb_idle_2d(dev);
		psb_2d_unlock(dev_priv);
	} else
		psb_idle_2d(dev);

	mutex_lock(&dev->mode_config.mutex);
	list_for_each_entry(fb, &dev->mode_config.fb_list, head) {
		struct fb_info *info = fb->fbdev;
		struct psbfb_par *par = info->par;

		bo = fb->bo;
		if (!bo)
			continue;

		ret = psbfb_move_fb_bo(info, bo,
				       DRM_BO_FLAG_MEM_TT |
				       DRM_BO_FLAG_MEM_VRAM |
				       DRM_BO_FLAG_NO_EVICT);
		if (ret)
			goto out_err;

		ret = drm_bo_kmap(bo, 0, bo->num_pages, &fb->kmap);
		if (ret)
			goto out_err;

		info->screen_base = drm_bmo_virtual(&fb->kmap, &dummy);
		fb->offset = bo->offset - dev_priv->pg->gatt_start;

		if (ret)
			goto out_err;

		if (resume)
			fb_set_suspend(info, 0);
		else
			info->state |= FBINFO_STATE_RUNNING;

		/*
		 * Re-run modesetting here, since the VDS scanout offset may
		 * have changed.
		 */

		if (par->crtc->enabled) {
			psbfb_set_par(info);
			psbfb_blank(par->dpms_state, info);
		}
	}
      out_err:
	mutex_unlock(&dev->mode_config.mutex);

	return ret;
}

int psbfb_kms_on_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	int ret;

	acquire_console_sem();
	ret = psbfb_kms_on(dev, 0);
	release_console_sem();
#ifdef SII_1392_WA
	if((SII_1392 != 1) || (drm_psb_no_fb==0))
		drm_disable_unused_functions(dev);
#else
	drm_disable_unused_functions(dev);
#endif
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
#ifdef SII_1392_WA
	if((SII_1392 != 1) || (drm_psb_no_fb==0))
		drm_disable_unused_functions(dev);
#else
	drm_disable_unused_functions(dev);
#endif
}

/*
 * FIXME: Before kernel inclusion, migrate nopfn to fault.
 * Also, these should be the default vm ops for buffer object type fbs.
 */

extern unsigned long drm_bo_vm_nopfn(struct vm_area_struct *vma,
				     unsigned long address);

/*
 * This wrapper is a bit ugly and is here because we need access to a mutex
 * that we can lock both around nopfn and around unmap_mapping_range + move.
 * Normally, this would've been done using the bo mutex, but unfortunately
 * we cannot lock it around drm_bo_do_validate(), since that would imply
 * recursive locking.
 */

static unsigned long psbfb_nopfn(struct vm_area_struct *vma,
				 unsigned long address)
{
	struct psbfb_vm_info *vi = (struct psbfb_vm_info *)vma->vm_private_data;
	struct vm_area_struct tmp_vma;
	unsigned long ret;

	mutex_lock(&vi->vm_mutex);
	tmp_vma = *vma;
	tmp_vma.vm_private_data = vi->bo;
	ret = drm_bo_vm_nopfn(&tmp_vma, address);
	mutex_unlock(&vi->vm_mutex);
	return ret;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
static int psbfb_fault(struct vm_area_struct *vma,
				 struct vm_fault *vmf)
{
	struct psbfb_vm_info *vi = (struct psbfb_vm_info *)vma->vm_private_data;
	struct vm_area_struct tmp_vma;
	unsigned long ret;

        unsigned long address = (unsigned long)vmf->virtual_address;

	mutex_lock(&vi->vm_mutex);
	tmp_vma = *vma;
	tmp_vma.vm_private_data = vi->bo;
	ret = drm_bo_vm_nopfn(&tmp_vma, address);
	mutex_unlock(&vi->vm_mutex);
	return ret;
}
#endif
static void psbfb_vm_open(struct vm_area_struct *vma)
{
	struct psbfb_vm_info *vi = (struct psbfb_vm_info *)vma->vm_private_data;

	atomic_inc(&vi->refcount);
}

static void psbfb_vm_close(struct vm_area_struct *vma)
{
	psbfb_vm_info_deref((struct psbfb_vm_info **)&vma->vm_private_data);
}

static struct vm_operations_struct psbfb_vm_ops = {
  #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
        .fault = psbfb_fault,
  #else
        .nopfn = psbfb_nopfn,
  #endif
	.open = psbfb_vm_open,
	.close = psbfb_vm_close,
};

static int psbfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct psbfb_par *par = info->par;
	struct drm_framebuffer *fb = par->crtc->fb;
	struct drm_buffer_object *bo = fb->bo;
	unsigned long size = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
	unsigned long offset = vma->vm_pgoff;

	if (vma->vm_pgoff != 0)
		return -EINVAL;
	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	if (offset + size > bo->num_pages)
		return -EINVAL;

	mutex_lock(&par->vi->vm_mutex);
	if (!par->vi->f_mapping)
		par->vi->f_mapping = vma->vm_file->f_mapping;
	mutex_unlock(&par->vi->vm_mutex);

	vma->vm_private_data = psbfb_vm_info_ref(par->vi);

	vma->vm_ops = &psbfb_vm_ops;
	vma->vm_flags |= VM_PFNMAP;

	return 0;
}

int psbfb_sync(struct fb_info *info)
{
	struct psbfb_par *par = info->par;
	struct drm_psb_private *dev_priv = par->dev->dev_private;

	psb_2d_lock(dev_priv);
	psb_idle_2d(par->dev);
	psb_2d_unlock(dev_priv);

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

int psbfb_probe(struct drm_device *dev, struct drm_crtc *crtc)
{
	struct fb_info *info;
	struct psbfb_par *par;
	struct device *device = &dev->pdev->dev;
	struct drm_framebuffer *fb;
	struct drm_display_mode *mode = crtc->desired_mode;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct drm_buffer_object *fbo = NULL;
	int ret;
	int is_iomem;

	if (drm_psb_no_fb) {
		/* need to do this as the DRM will disable the output */
		crtc->enabled = 1;
		return 0;
	}

	info = framebuffer_alloc(sizeof(struct psbfb_par), device);
	if (!info) {
		return -ENOMEM;
	}

	fb = drm_framebuffer_create(dev);
	if (!fb) {
		framebuffer_release(info);
		DRM_ERROR("failed to allocate fb.\n");
		return -ENOMEM;
	}
	crtc->fb = fb;

	fb->width = mode->hdisplay;
	fb->height = mode->vdisplay;

	fb->bits_per_pixel = 32;
	fb->depth = 24;
	fb->pitch =
	    ((fb->width * ((fb->bits_per_pixel + 1) / 8)) + 0x3f) & ~0x3f;

	ret = drm_buffer_object_create(dev,
				       fb->pitch * fb->height,
				       drm_bo_type_kernel,
				       DRM_BO_FLAG_READ |
				       DRM_BO_FLAG_WRITE |
				       DRM_BO_FLAG_MEM_TT |
				       DRM_BO_FLAG_MEM_VRAM |
				       DRM_BO_FLAG_NO_EVICT,
				       DRM_BO_HINT_DONT_FENCE, 0, 0, &fbo);
	if (ret || !fbo) {
		DRM_ERROR("failed to allocate framebuffer\n");
		goto out_err0;
	}

	fb->offset = fbo->offset - dev_priv->pg->gatt_start;
	fb->bo = fbo;
	DRM_DEBUG("allocated %dx%d fb: 0x%08lx, bo %p\n", fb->width,
		  fb->height, fb->offset, fbo);

	fb->fbdev = info;

	par = info->par;

	par->dev = dev;
	par->crtc = crtc;
	par->vi = psbfb_vm_info_create();
	if (!par->vi)
		goto out_err1;

	mutex_lock(&dev->struct_mutex);
	par->vi->bo = fbo;
	atomic_inc(&fbo->usage);
	mutex_unlock(&dev->struct_mutex);

	par->vi->f_mapping = NULL;
	info->fbops = &psbfb_ops;

	strcpy(info->fix.id, "psbfb");
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = FB_VISUAL_DIRECTCOLOR;
	info->fix.type_aux = 0;
	info->fix.xpanstep = 1;
	info->fix.ypanstep = 1;
	info->fix.ywrapstep = 0;
	info->fix.accel = FB_ACCEL_NONE;	/* ??? */
	info->fix.type_aux = 0;
	info->fix.mmio_start = 0;
	info->fix.mmio_len = 0;
	info->fix.line_length = fb->pitch;
	info->fix.smem_start = dev->mode_config.fb_base + fb->offset;
	info->fix.smem_len = info->fix.line_length * fb->height;

	info->flags = FBINFO_DEFAULT |
	    FBINFO_PARTIAL_PAN_OK /*| FBINFO_MISC_ALWAYS_SETPAR */ ;

	ret = drm_bo_kmap(fb->bo, 0, fb->bo->num_pages, &fb->kmap);
	if (ret) {
		DRM_ERROR("error mapping fb: %d\n", ret);
		goto out_err2;
	}

	info->screen_base = drm_bmo_virtual(&fb->kmap, &is_iomem);
	memset(info->screen_base, 0x00, fb->pitch*fb->height);
	info->screen_size = info->fix.smem_len;	/* FIXME */
	info->pseudo_palette = fb->pseudo_palette;
	info->var.xres_virtual = fb->width;
	info->var.yres_virtual = fb->height;
	info->var.bits_per_pixel = fb->bits_per_pixel;
	info->var.xoffset = 0;
	info->var.yoffset = 0;
	info->var.activate = FB_ACTIVATE_NOW;
	info->var.height = -1;
	info->var.width = -1;
	info->var.vmode = FB_VMODE_NONINTERLACED;

	info->var.xres = mode->hdisplay;
	info->var.right_margin = mode->hsync_start - mode->hdisplay;
	info->var.hsync_len = mode->hsync_end - mode->hsync_start;
	info->var.left_margin = mode->htotal - mode->hsync_end;
	info->var.yres = mode->vdisplay;
	info->var.lower_margin = mode->vsync_start - mode->vdisplay;
	info->var.vsync_len = mode->vsync_end - mode->vsync_start;
	info->var.upper_margin = mode->vtotal - mode->vsync_end;
	info->var.pixclock = 10000000 / mode->htotal * 1000 /
	    mode->vtotal * 100;
	/* avoid overflow */
	info->var.pixclock = info->var.pixclock * 1000 / mode->vrefresh;

	info->pixmap.size = 64 * 1024;
	info->pixmap.buf_align = 8;
	info->pixmap.access_align = 32;
	info->pixmap.flags = FB_PIXMAP_SYSTEM;
	info->pixmap.scan_align = 1;

	DRM_DEBUG("fb depth is %d\n", fb->depth);
	DRM_DEBUG("   pitch is %d\n", fb->pitch);
	switch (fb->depth) {
	case 8:
		info->var.red.offset = 0;
		info->var.green.offset = 0;
		info->var.blue.offset = 0;
		info->var.red.length = 8;	/* 8bit DAC */
		info->var.green.length = 8;
		info->var.blue.length = 8;
		info->var.transp.offset = 0;
		info->var.transp.length = 0;
		break;
	case 15:
		info->var.red.offset = 10;
		info->var.green.offset = 5;
		info->var.blue.offset = 0;
		info->var.red.length = info->var.green.length =
		    info->var.blue.length = 5;
		info->var.transp.offset = 15;
		info->var.transp.length = 1;
		break;
	case 16:
		info->var.red.offset = 11;
		info->var.green.offset = 5;
		info->var.blue.offset = 0;
		info->var.red.length = 5;
		info->var.green.length = 6;
		info->var.blue.length = 5;
		info->var.transp.offset = 0;
		break;
	case 24:
		info->var.red.offset = 16;
		info->var.green.offset = 8;
		info->var.blue.offset = 0;
		info->var.red.length = info->var.green.length =
		    info->var.blue.length = 8;
		info->var.transp.offset = 0;
		info->var.transp.length = 0;
		break;
	case 32:
		info->var.red.offset = 16;
		info->var.green.offset = 8;
		info->var.blue.offset = 0;
		info->var.red.length = info->var.green.length =
		    info->var.blue.length = 8;
		info->var.transp.offset = 24;
		info->var.transp.length = 8;
		break;
	default:
		break;
	}

	if (register_framebuffer(info) < 0)
		goto out_err3;

	if (psbfb_check_var(&info->var, info) < 0)
		goto out_err4;

	psbfb_set_par(info);

	DRM_INFO("fb%d: %s frame buffer device\n", info->node, info->fix.id);

	return 0;
      out_err4:
	unregister_framebuffer(info);
      out_err3:
	drm_bo_kunmap(&fb->kmap);
      out_err2:
	psbfb_vm_info_deref(&par->vi);
      out_err1:
	drm_bo_usage_deref_unlocked(&fb->bo);
      out_err0:
	drm_framebuffer_destroy(fb);
	framebuffer_release(info);
	crtc->fb = NULL;
	return -EINVAL;
}

EXPORT_SYMBOL(psbfb_probe);

int psbfb_remove(struct drm_device *dev, struct drm_crtc *crtc)
{
	struct drm_framebuffer *fb;
	struct fb_info *info;
	struct psbfb_par *par;

	if (drm_psb_no_fb)
		return 0;

	fb = crtc->fb;
	info = fb->fbdev;

	if (info) {
		unregister_framebuffer(info);
		drm_bo_kunmap(&fb->kmap);
		par = info->par;
		if (par)
			psbfb_vm_info_deref(&par->vi);
		drm_bo_usage_deref_unlocked(&fb->bo);
		drm_framebuffer_destroy(fb);
		framebuffer_release(info);
	}
	return 0;
}

EXPORT_SYMBOL(psbfb_remove);
