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
#include "psb_drv.h"
#include "psb_schedule.h"

struct drm_psb_ttm_backend {
	struct drm_ttm_backend base;
	struct page **pages;
	unsigned int desired_tile_stride;
	unsigned int hw_tile_stride;
	int mem_type;
	unsigned long offset;
	unsigned long num_pages;
};

int psb_fence_types(struct drm_buffer_object *bo, uint32_t * class,
		    uint32_t * type)
{
	switch (*class) {
	case PSB_ENGINE_TA:
		*type = DRM_FENCE_TYPE_EXE |
		    _PSB_FENCE_TYPE_TA_DONE | _PSB_FENCE_TYPE_RASTER_DONE;
		if (bo->mem.mask & PSB_BO_FLAG_TA)
			*type &= ~_PSB_FENCE_TYPE_RASTER_DONE;
		if (bo->mem.mask & PSB_BO_FLAG_SCENE)
			*type |= _PSB_FENCE_TYPE_SCENE_DONE;
		if (bo->mem.mask & PSB_BO_FLAG_FEEDBACK)
			*type |= _PSB_FENCE_TYPE_FEEDBACK;
		break;
	default:
		*type = DRM_FENCE_TYPE_EXE;
	}
	return 0;
}

/*
 * Poulsbo GPU virtual space looks like this
 * (We currently use only one MMU context).
 *
 * gatt_start = Start of GATT aperture in bus space.
 * stolen_end = End of GATT populated by stolen memory in bus space.
 * gatt_end   = End of GATT
 * twod_end   = MIN(gatt_start + 256_MEM, gatt_end)
 *
 * 0x00000000 -> 0x10000000 Temporary mapping space for tiling- and copy operations.
 *                          This space is not managed and is protected by the
 *                          temp_mem mutex.
 *
 * 0x10000000 -> 0x20000000 DRM_PSB_MEM_KERNEL For kernel buffers.
 *
 * 0x20000000 -> gatt_start DRM_PSB_MEM_MMU    For generic MMU-only use.
 *
 * gatt_start -> stolen_end DRM_BO_MEM_VRAM    Pre-populated GATT pages.
 *
 * stolen_end -> twod_end   DRM_BO_MEM_TT      GATT memory usable by 2D engine.
 *
 * twod_end -> gatt_end     DRM_BO_MEM_APER    GATT memory not usable by 2D engine.
 *
 * gatt_end ->   0xffffffff Currently unused.
 */

int psb_init_mem_type(struct drm_device *dev, uint32_t type,
		      struct drm_mem_type_manager *man)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct psb_gtt *pg = dev_priv->pg;

	switch (type) {
	case DRM_BO_MEM_LOCAL:
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_MEMTYPE_CACHED;
		man->drm_bus_maptype = 0;
		break;
	case DRM_PSB_MEM_KERNEL:
		man->io_offset = 0x00000000;
		man->io_size = 0x00000000;
		man->io_addr = NULL;
		man->drm_bus_maptype = _DRM_TTM;
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_MEMTYPE_CMA;
		man->gpu_offset = PSB_MEM_KERNEL_START;
		break;
	case DRM_PSB_MEM_MMU:
		man->io_offset = 0x00000000;
		man->io_size = 0x00000000;
		man->io_addr = NULL;
		man->drm_bus_maptype = _DRM_TTM;
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_MEMTYPE_CMA;
		man->gpu_offset = PSB_MEM_MMU_START;
		break;
	case DRM_PSB_MEM_PDS:
		man->io_offset = 0x00000000;
		man->io_size = 0x00000000;
		man->io_addr = NULL;
		man->drm_bus_maptype = _DRM_TTM;
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_MEMTYPE_CMA;
		man->gpu_offset = PSB_MEM_PDS_START;
		break;
	case DRM_PSB_MEM_RASTGEOM:
		man->io_offset = 0x00000000;
		man->io_size = 0x00000000;
		man->io_addr = NULL;
		man->drm_bus_maptype = _DRM_TTM;
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_MEMTYPE_CMA;
		man->gpu_offset = PSB_MEM_RASTGEOM_START;
		break;
	case DRM_BO_MEM_VRAM:
		man->io_addr = NULL;
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_MEMTYPE_FIXED | _DRM_FLAG_NEEDS_IOREMAP;
#ifdef PSB_WORKING_HOST_MMU_ACCESS
		man->drm_bus_maptype = _DRM_AGP;
		man->io_offset = pg->gatt_start;
		man->io_size = pg->gatt_pages << PAGE_SHIFT;
#else
		man->drm_bus_maptype = _DRM_TTM;	/* Forces uncached */
		man->io_offset = pg->stolen_base;
		man->io_size = pg->stolen_size;
#endif
		man->gpu_offset = pg->gatt_start;
		break;
	case DRM_BO_MEM_TT:	/* Mappable GATT memory */
		man->io_offset = pg->gatt_start;
		man->io_size = pg->gatt_pages << PAGE_SHIFT;
		man->io_addr = NULL;
#ifdef PSB_WORKING_HOST_MMU_ACCESS
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_NEEDS_IOREMAP;
		man->drm_bus_maptype = _DRM_AGP;
#else
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_MEMTYPE_CMA;
		man->drm_bus_maptype = _DRM_TTM;
#endif
		man->gpu_offset = pg->gatt_start;
		break;
	case DRM_PSB_MEM_APER:	/*MMU memory. Mappable. Not usable for 2D. */
		man->io_offset = pg->gatt_start;
		man->io_size = pg->gatt_pages << PAGE_SHIFT;
		man->io_addr = NULL;
#ifdef PSB_WORKING_HOST_MMU_ACCESS
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_NEEDS_IOREMAP;
		man->drm_bus_maptype = _DRM_AGP;
#else
		man->flags = _DRM_FLAG_MEMTYPE_MAPPABLE |
		    _DRM_FLAG_MEMTYPE_CMA;
		man->drm_bus_maptype = _DRM_TTM;
#endif
		man->gpu_offset = pg->gatt_start;
		break;
	default:
		DRM_ERROR("Unsupported memory type %u\n", (unsigned)type);
		return -EINVAL;
	}
	return 0;
}

uint32_t psb_evict_mask(struct drm_buffer_object * bo)
{
	switch (bo->mem.mem_type) {
	case DRM_BO_MEM_VRAM:
		return DRM_BO_FLAG_MEM_TT;
	default:
		return DRM_BO_FLAG_MEM_LOCAL;
	}
}

int psb_invalidate_caches(struct drm_device *dev, uint64_t flags)
{
	return 0;
}

static int psb_move_blit(struct drm_buffer_object *bo,
			 int evict, int no_wait, struct drm_bo_mem_reg *new_mem)
{
	struct drm_bo_mem_reg *old_mem = &bo->mem;
	int dir = 0;

	if ((old_mem->mem_type == new_mem->mem_type) &&
	    (new_mem->mm_node->start <
	     old_mem->mm_node->start + old_mem->mm_node->size)) {
		dir = 1;
	}

	psb_emit_2d_copy_blit(bo->dev,
			      old_mem->mm_node->start << PAGE_SHIFT,
			      new_mem->mm_node->start << PAGE_SHIFT,
			      new_mem->num_pages, dir);

	return drm_bo_move_accel_cleanup(bo, evict, no_wait, 0,
					 DRM_FENCE_TYPE_EXE, 0, new_mem);
}

/*
 * Flip destination ttm into GATT,
 * then blit and subsequently move out again.
 */

static int psb_move_flip(struct drm_buffer_object *bo,
			 int evict, int no_wait, struct drm_bo_mem_reg *new_mem)
{
	struct drm_device *dev = bo->dev;
	struct drm_bo_mem_reg tmp_mem;
	int ret;

	tmp_mem = *new_mem;
	tmp_mem.mm_node = NULL;
	tmp_mem.mask = DRM_BO_FLAG_MEM_TT;

	ret = drm_bo_mem_space(bo, &tmp_mem, no_wait);
	if (ret)
		return ret;
	ret = drm_bind_ttm(bo->ttm, &tmp_mem);
	if (ret)
		goto out_cleanup;
	ret = psb_move_blit(bo, 1, no_wait, &tmp_mem);
	if (ret)
		goto out_cleanup;

	ret = drm_bo_move_ttm(bo, evict, no_wait, new_mem);
      out_cleanup:
	if (tmp_mem.mm_node) {
		mutex_lock(&dev->struct_mutex);
		if (tmp_mem.mm_node != bo->pinned_node)
			drm_mm_put_block(tmp_mem.mm_node);
		tmp_mem.mm_node = NULL;
		mutex_unlock(&dev->struct_mutex);
	}
	return ret;
}

int psb_move(struct drm_buffer_object *bo,
	     int evict, int no_wait, struct drm_bo_mem_reg *new_mem)
{
	struct drm_bo_mem_reg *old_mem = &bo->mem;

	if (old_mem->mem_type == DRM_BO_MEM_LOCAL) {
		return drm_bo_move_memcpy(bo, evict, no_wait, new_mem);
	} else if (new_mem->mem_type == DRM_BO_MEM_LOCAL) {
		if (psb_move_flip(bo, evict, no_wait, new_mem))
			return drm_bo_move_memcpy(bo, evict, no_wait, new_mem);
	} else {
		if (psb_move_blit(bo, evict, no_wait, new_mem))
			return drm_bo_move_memcpy(bo, evict, no_wait, new_mem);
	}
	return 0;
}

static int drm_psb_tbe_nca(struct drm_ttm_backend *backend)
{
	return ((backend->flags & DRM_BE_FLAG_BOUND_CACHED) ? 0 : 1);
}

static int drm_psb_tbe_populate(struct drm_ttm_backend *backend,
				unsigned long num_pages, struct page **pages)
{
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);

	psb_be->pages = pages;
	return 0;
}

static int drm_psb_tbe_unbind(struct drm_ttm_backend *backend)
{
	struct drm_device *dev = backend->dev;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);
	struct psb_mmu_pd *pd = psb_mmu_get_default_pd(dev_priv->mmu);
	struct drm_mem_type_manager *man = &dev->bm.man[psb_be->mem_type];

	PSB_DEBUG_RENDER("MMU unbind.\n");

	if (psb_be->mem_type == DRM_BO_MEM_TT) {
		uint32_t gatt_p_offset = (psb_be->offset - man->gpu_offset) >>
		    PAGE_SHIFT;

		(void)psb_gtt_remove_pages(dev_priv->pg, gatt_p_offset,
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

static int drm_psb_tbe_bind(struct drm_ttm_backend *backend,
			    struct drm_bo_mem_reg *bo_mem)
{
	struct drm_device *dev = backend->dev;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);
	struct psb_mmu_pd *pd = psb_mmu_get_default_pd(dev_priv->mmu);
	struct drm_mem_type_manager *man = &dev->bm.man[bo_mem->mem_type];
	int type;
	int ret = 0;

	psb_be->mem_type = bo_mem->mem_type;
	psb_be->num_pages = bo_mem->num_pages;
	psb_be->desired_tile_stride = bo_mem->desired_tile_stride;
	psb_be->hw_tile_stride = bo_mem->hw_tile_stride;
	psb_be->desired_tile_stride = 0;
	psb_be->hw_tile_stride = 0;
	psb_be->offset = (bo_mem->mm_node->start << PAGE_SHIFT) +
	    man->gpu_offset;

	type = (bo_mem->flags & DRM_BO_FLAG_CACHED) ? PSB_MMU_CACHED_MEMORY : 0;

	PSB_DEBUG_RENDER("MMU bind.\n");
	if (psb_be->mem_type == DRM_BO_MEM_TT) {
		uint32_t gatt_p_offset = (psb_be->offset - man->gpu_offset) >>
		    PAGE_SHIFT;

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

	DRM_FLAG_MASKED(backend->flags, (bo_mem->flags & DRM_BO_FLAG_CACHED) ?
			DRM_BE_FLAG_BOUND_CACHED : 0, DRM_BE_FLAG_BOUND_CACHED);

	return 0;
      out_err:
	drm_psb_tbe_unbind(backend);
	return ret;

}

static void drm_psb_tbe_clear(struct drm_ttm_backend *backend)
{
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);

	psb_be->pages = NULL;
	return;
}

static void drm_psb_tbe_destroy(struct drm_ttm_backend *backend)
{
	struct drm_psb_ttm_backend *psb_be =
	    container_of(backend, struct drm_psb_ttm_backend, base);

	if (backend)
		drm_free(psb_be, sizeof(*psb_be), DRM_MEM_TTM);
}

static struct drm_ttm_backend_func psb_ttm_backend = {
	.needs_ub_cache_adjust = drm_psb_tbe_nca,
	.populate = drm_psb_tbe_populate,
	.clear = drm_psb_tbe_clear,
	.bind = drm_psb_tbe_bind,
	.unbind = drm_psb_tbe_unbind,
	.destroy = drm_psb_tbe_destroy,
};

struct drm_ttm_backend *drm_psb_tbe_init(struct drm_device *dev)
{
	struct drm_psb_ttm_backend *psb_be;

	psb_be = drm_calloc(1, sizeof(*psb_be), DRM_MEM_TTM);
	if (!psb_be)
		return NULL;
	psb_be->pages = NULL;
	psb_be->base.func = &psb_ttm_backend;
	psb_be->base.dev = dev;

	return &psb_be->base;
}

int psb_tbe_size(struct drm_device *dev, unsigned long num_pages)
{
	/*
	 * Return the size of the structures themselves and the
	 * estimated size of the pagedir and pagetable entries.
	 */

	return drm_size_align(sizeof(struct drm_psb_ttm_backend)) +
		8*num_pages;
}
