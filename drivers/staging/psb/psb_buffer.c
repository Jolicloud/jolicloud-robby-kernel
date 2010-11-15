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
#include <drm/drmP.h>
#include "psb_drv.h"
#include "psb_schedule.h"
#include "ttm/ttm_placement_common.h"
#include "ttm/ttm_execbuf_util.h"
#include "ttm/ttm_fence_api.h"

struct drm_psb_ttm_backend {
	struct ttm_backend base;
	struct page **pages;
	unsigned int desired_tile_stride;
	unsigned int hw_tile_stride;
	int mem_type;
	unsigned long offset;
	unsigned long num_pages;
};

/*
 * Poulsbo GPU virtual space looks like this
 * (We currently use only one MMU context).
 *
 * gatt_start = Start of GATT aperture in bus space.
 * stolen_end = End of GATT populated by stolen memory in bus space.
 * gatt_end   = End of GATT
 * twod_end   = MIN(gatt_start + 256_MEM, gatt_end)
 *
 * 0x00000000 -> 0x10000000 Temporary mapping space for tiling-
 * and copy operations.
 *                          This space is not managed and is protected by the
 *                          temp_mem mutex.
 *
 * 0x10000000 -> 0x20000000 DRM_PSB_MEM_KERNEL For kernel buffers.
 *
 * 0x20000000 -> gatt_start DRM_PSB_MEM_MMU    For generic MMU-only use.
 *
 * gatt_start -> stolen_end TTM_PL_VRAM    Pre-populated GATT pages.
 *
 * stolen_end -> twod_end   TTM_PL_TT      GATT memory usable by 2D engine.
 *
 * twod_end -> gatt_end     DRM_BO_MEM_APER    GATT memory not
 * usable by 2D engine.
 *
 * gatt_end ->   0xffffffff Currently unused.
 */

static int psb_init_mem_type(struct ttm_bo_device *bdev, uint32_t type,
			     struct ttm_mem_type_manager *man)
{

	struct drm_psb_private *dev_priv =
	    container_of(bdev, struct drm_psb_private, bdev);
	struct psb_gtt *pg = dev_priv->pg;

	switch (type) {
	case TTM_PL_SYSTEM:
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE;
		man->available_caching = TTM_PL_FLAG_CACHED |
		    TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_CACHED;
		break;
	case DRM_PSB_MEM_KERNEL:
		man->io_offset = 0x00000000;
		man->io_size = 0x00000000;
		man->io_addr = NULL;
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
		    TTM_MEMTYPE_FLAG_CMA;
		man->gpu_offset = PSB_MEM_KERNEL_START;
		man->available_caching = TTM_PL_FLAG_CACHED |
		    TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		break;
	case DRM_PSB_MEM_MMU:
		man->io_offset = 0x00000000;
		man->io_size = 0x00000000;
		man->io_addr = NULL;
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
		    TTM_MEMTYPE_FLAG_CMA;
		man->gpu_offset = PSB_MEM_MMU_START;
		man->available_caching = TTM_PL_FLAG_CACHED |
		    TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		break;
	case DRM_PSB_MEM_PDS:
		man->io_offset = 0x00000000;
		man->io_size = 0x00000000;
		man->io_addr = NULL;
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
		    TTM_MEMTYPE_FLAG_CMA;
		man->gpu_offset = PSB_MEM_PDS_START;
		man->available_caching = TTM_PL_FLAG_CACHED |
		    TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		break;
	case DRM_PSB_MEM_RASTGEOM:
		man->io_offset = 0x00000000;
		man->io_size = 0x00000000;
		man->io_addr = NULL;
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
		    TTM_MEMTYPE_FLAG_CMA;
		man->gpu_offset = PSB_MEM_RASTGEOM_START;
		man->available_caching = TTM_PL_FLAG_CACHED |
		    TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		break;
	case TTM_PL_VRAM:
		man->io_addr = NULL;
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
		    TTM_MEMTYPE_FLAG_FIXED |
		    TTM_MEMTYPE_FLAG_NEEDS_IOREMAP;
#ifdef PSB_WORKING_HOST_MMU_ACCESS
		man->io_offset = pg->gatt_start;
		man->io_size = pg->gatt_pages << PAGE_SHIFT;
#else
		man->io_offset = pg->stolen_base;
		man->io_size = pg->vram_stolen_size;
#endif
		man->gpu_offset = pg->gatt_start;
		man->available_caching = TTM_PL_FLAG_UNCACHED |
		    TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		break;
	case TTM_PL_CI:
		man->io_addr = NULL;
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
			TTM_MEMTYPE_FLAG_FIXED |
			TTM_MEMTYPE_FLAG_NEEDS_IOREMAP;
		man->io_offset = dev_priv->ci_region_start;
		man->io_size = pg->ci_stolen_size;
		man->gpu_offset = pg->gatt_start - pg->ci_stolen_size;
		man->available_caching = TTM_PL_FLAG_UNCACHED;
		man->default_caching = TTM_PL_FLAG_UNCACHED;
		break;
	case TTM_PL_TT:	/* Mappable GATT memory */
		man->io_offset = pg->gatt_start;
		man->io_size = pg->gatt_pages << PAGE_SHIFT;
		man->io_addr = NULL;
#ifdef PSB_WORKING_HOST_MMU_ACCESS
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
		    TTM_MEMTYPE_FLAG_NEEDS_IOREMAP;
#else
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
		    TTM_MEMTYPE_FLAG_CMA;
#endif
		man->available_caching = TTM_PL_FLAG_CACHED |
		    TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		man->gpu_offset = pg->gatt_start;
		break;
	case DRM_PSB_MEM_APER:	/*MMU memory. Mappable. Not usable for 2D. */
		man->io_offset = pg->gatt_start;
		man->io_size = pg->gatt_pages << PAGE_SHIFT;
		man->io_addr = NULL;
#ifdef PSB_WORKING_HOST_MMU_ACCESS
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
		    TTM_MEMTYPE_FLAG_NEEDS_IOREMAP;
#else
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE |
		    TTM_MEMTYPE_FLAG_CMA;
#endif
		man->gpu_offset = pg->gatt_start;
		man->available_caching = TTM_PL_FLAG_CACHED |
		    TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
		break;
	default:
		DRM_ERROR("Unsupported memory type %u\n", (unsigned) type);
		return -EINVAL;
	}
	return 0;
}

static uint32_t psb_evict_mask(struct ttm_buffer_object *bo)
{
	uint32_t cur_placement = bo->mem.flags & ~TTM_PL_MASK_MEM;


	switch (bo->mem.mem_type) {
	case TTM_PL_VRAM:
		if (bo->mem.proposed_flags & TTM_PL_FLAG_TT)
			return cur_placement | TTM_PL_FLAG_TT;
		else
			return cur_placement | TTM_PL_FLAG_SYSTEM;
	default:
		return cur_placement | TTM_PL_FLAG_SYSTEM;
	}
}

static int psb_invalidate_caches(struct ttm_bo_device *bdev,
				 uint32_t placement)
{
	return 0;
}

static int psb_move_blit(struct ttm_buffer_object *bo,
			 bool evict, bool no_wait,
			 struct ttm_mem_reg *new_mem)
{
	struct drm_psb_private *dev_priv =
	    container_of(bo->bdev, struct drm_psb_private, bdev);
	struct drm_device *dev = dev_priv->dev;
	struct ttm_mem_reg *old_mem = &bo->mem;
	struct ttm_fence_object *fence;
	int dir = 0;
	int ret;

	if ((old_mem->mem_type == new_mem->mem_type) &&
	    (new_mem->mm_node->start <
	     old_mem->mm_node->start + old_mem->mm_node->size)) {
		dir = 1;
	}

	psb_emit_2d_copy_blit(dev,
			      old_mem->mm_node->start << PAGE_SHIFT,
			      new_mem->mm_node->start << PAGE_SHIFT,
			      new_mem->num_pages, dir);

	ret = ttm_fence_object_create(&dev_priv->fdev, 0,
				      _PSB_FENCE_TYPE_EXE,
				      TTM_FENCE_FLAG_EMIT,
				      &fence);
	if (unlikely(ret != 0)) {
		psb_idle_2d(dev);
		if (fence)
			ttm_fence_object_unref(&fence);
	}

	ret = ttm_bo_move_accel_cleanup(bo, (void *) fence,
					(void *) (unsigned long)
					_PSB_FENCE_TYPE_EXE,
					evict, no_wait, new_mem);
	if (fence)
		ttm_fence_object_unref(&fence);
	return ret;
}

/*
 * Flip destination ttm into GATT,
 * then blit and subsequently move out again.
 */

static int psb_move_flip(struct ttm_buffer_object *bo,
			 bool evict, bool interruptible, bool no_wait,
			 struct ttm_mem_reg *new_mem)
{
	struct ttm_bo_device *bdev = bo->bdev;
	struct ttm_mem_reg tmp_mem;
	int ret;

	tmp_mem = *new_mem;
	tmp_mem.mm_node = NULL;
	tmp_mem.proposed_flags = TTM_PL_FLAG_TT;

	ret = ttm_bo_mem_space(bo, &tmp_mem, interruptible, no_wait);
	if (ret)
		return ret;
	ret = ttm_tt_bind(bo->ttm, &tmp_mem);
	if (ret)
		goto out_cleanup;
	ret = psb_move_blit(bo, true, no_wait, &tmp_mem);
	if (ret)
		goto out_cleanup;

	ret = ttm_bo_move_ttm(bo, evict, no_wait, new_mem);
out_cleanup:
	if (tmp_mem.mm_node) {
		spin_lock(&bdev->lru_lock);
		drm_mm_put_block(tmp_mem.mm_node);
		tmp_mem.mm_node = NULL;
		spin_unlock(&bdev->lru_lock);
	}
	return ret;
}

static int psb_move(struct ttm_buffer_object *bo,
		    bool evict, bool interruptible,
		    bool no_wait, struct ttm_mem_reg *new_mem)
{
	struct ttm_mem_reg *old_mem = &bo->mem;

	if (old_mem->mem_type == TTM_PL_SYSTEM) {
		return ttm_bo_move_memcpy(bo, evict, no_wait, new_mem);
	} else if (new_mem->mem_type == TTM_PL_SYSTEM) {
		int ret = psb_move_flip(bo, evict, interruptible,
					no_wait, new_mem);
		if (unlikely(ret != 0)) {
			if (ret == -ERESTART)
				return ret;
			else
				return ttm_bo_move_memcpy(bo, evict, no_wait,
							  new_mem);
		}
	} else {
		if (psb_move_blit(bo, evict, no_wait, new_mem))
			return ttm_bo_move_memcpy(bo, evict, no_wait,
						  new_mem);
	}
	return 0;
}

static int drm_psb_tbe_populate(struct ttm_backend *backend,
				unsigned long num_pages,
				struct page **pages,
				struct page *dummy_read_page)
{
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);

	psb_be->pages = pages;
	return 0;
}

static int drm_psb_tbe_unbind(struct ttm_backend *backend)
{
	struct ttm_bo_device *bdev = backend->bdev;
	struct drm_psb_private *dev_priv =
	    container_of(bdev, struct drm_psb_private, bdev);
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);
	struct psb_mmu_pd *pd = psb_mmu_get_default_pd(dev_priv->mmu);
	struct ttm_mem_type_manager *man = &bdev->man[psb_be->mem_type];

	PSB_DEBUG_RENDER("MMU unbind.\n");

	if (psb_be->mem_type == TTM_PL_TT) {
		uint32_t gatt_p_offset =
		    (psb_be->offset - man->gpu_offset) >> PAGE_SHIFT;

		(void) psb_gtt_remove_pages(dev_priv->pg, gatt_p_offset,
					    psb_be->num_pages,
					    psb_be->desired_tile_stride,
					    psb_be->hw_tile_stride);
	}

	psb_mmu_remove_pages(pd, psb_be->offset,
			     psb_be->num_pages,
			     psb_be->desired_tile_stride,
			     psb_be->hw_tile_stride);

	return 0;
}

static int drm_psb_tbe_bind(struct ttm_backend *backend,
			    struct ttm_mem_reg *bo_mem)
{
	struct ttm_bo_device *bdev = backend->bdev;
	struct drm_psb_private *dev_priv =
	    container_of(bdev, struct drm_psb_private, bdev);
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);
	struct psb_mmu_pd *pd = psb_mmu_get_default_pd(dev_priv->mmu);
	struct ttm_mem_type_manager *man = &bdev->man[bo_mem->mem_type];
	int type;
	int ret = 0;

	psb_be->mem_type = bo_mem->mem_type;
	psb_be->num_pages = bo_mem->num_pages;
	psb_be->desired_tile_stride = 0;
	psb_be->hw_tile_stride = 0;
	psb_be->offset = (bo_mem->mm_node->start << PAGE_SHIFT) +
	    man->gpu_offset;

	type =
	    (bo_mem->
	     flags & TTM_PL_FLAG_CACHED) ? PSB_MMU_CACHED_MEMORY : 0;

	PSB_DEBUG_RENDER("MMU bind.\n");
	if (psb_be->mem_type == TTM_PL_TT) {
		uint32_t gatt_p_offset =
		    (psb_be->offset - man->gpu_offset) >> PAGE_SHIFT;

		ret = psb_gtt_insert_pages(dev_priv->pg, psb_be->pages,
					   gatt_p_offset,
					   psb_be->num_pages,
					   psb_be->desired_tile_stride,
					   psb_be->hw_tile_stride, type);
	}

	ret = psb_mmu_insert_pages(pd, psb_be->pages,
				   psb_be->offset, psb_be->num_pages,
				   psb_be->desired_tile_stride,
				   psb_be->hw_tile_stride, type);
	if (ret)
		goto out_err;

	return 0;
out_err:
	drm_psb_tbe_unbind(backend);
	return ret;

}

static void drm_psb_tbe_clear(struct ttm_backend *backend)
{
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);

	psb_be->pages = NULL;
	return;
}

static void drm_psb_tbe_destroy(struct ttm_backend *backend)
{
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);

	if (backend)
		kfree(psb_be);
}

static struct ttm_backend_func psb_ttm_backend = {
	.populate = drm_psb_tbe_populate,
	.clear = drm_psb_tbe_clear,
	.bind = drm_psb_tbe_bind,
	.unbind = drm_psb_tbe_unbind,
	.destroy = drm_psb_tbe_destroy,
};

static struct ttm_backend *drm_psb_tbe_init(struct ttm_bo_device *bdev)
{
	struct drm_psb_ttm_backend *psb_be;

	psb_be = kcalloc(1, sizeof(*psb_be), GFP_KERNEL);
	if (!psb_be)
		return NULL;
	psb_be->pages = NULL;
	psb_be->base.func = &psb_ttm_backend;
	psb_be->base.bdev = bdev;
	return &psb_be->base;
}

/*
 * Use this memory type priority if no eviction is needed.
 */
static uint32_t psb_mem_prios[] = {
	TTM_PL_CI,
	TTM_PL_VRAM,
	TTM_PL_TT,
	DRM_PSB_MEM_KERNEL,
	DRM_PSB_MEM_MMU,
	DRM_PSB_MEM_RASTGEOM,
	DRM_PSB_MEM_PDS,
	DRM_PSB_MEM_APER,
	TTM_PL_SYSTEM
};

/*
 * Use this memory type priority if need to evict.
 */
static uint32_t psb_busy_prios[] = {
	TTM_PL_TT,
	TTM_PL_VRAM,
	TTM_PL_CI,
	DRM_PSB_MEM_KERNEL,
	DRM_PSB_MEM_MMU,
	DRM_PSB_MEM_RASTGEOM,
	DRM_PSB_MEM_PDS,
	DRM_PSB_MEM_APER,
	TTM_PL_SYSTEM
};


struct ttm_bo_driver psb_ttm_bo_driver = {
	.mem_type_prio = psb_mem_prios,
	.mem_busy_prio = psb_busy_prios,
	.num_mem_type_prio = ARRAY_SIZE(psb_mem_prios),
	.num_mem_busy_prio = ARRAY_SIZE(psb_busy_prios),
	.create_ttm_backend_entry = &drm_psb_tbe_init,
	.invalidate_caches = &psb_invalidate_caches,
	.init_mem_type = &psb_init_mem_type,
	.evict_flags = &psb_evict_mask,
	.move = &psb_move,
	.verify_access = &psb_verify_access,
	.sync_obj_signaled = &ttm_fence_sync_obj_signaled,
	.sync_obj_wait = &ttm_fence_sync_obj_wait,
	.sync_obj_flush = &ttm_fence_sync_obj_flush,
	.sync_obj_unref = &ttm_fence_sync_obj_unref,
	.sync_obj_ref = &ttm_fence_sync_obj_ref
};
