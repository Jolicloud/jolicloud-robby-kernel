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
#include "psb_scene.h"

void psb_clear_scene_atomic(struct psb_scene *scene)
{
	int i;
	struct page *page;
	void *v;

	for (i = 0; i < scene->clear_num_pages; ++i) {
		page = ttm_tt_get_page(scene->hw_data->ttm,
				       scene->clear_p_start + i);
		if (in_irq())
			v = kmap_atomic(page, KM_IRQ0);
		else
			v = kmap_atomic(page, KM_USER0);

		memset(v, 0, PAGE_SIZE);

		if (in_irq())
			kunmap_atomic(v, KM_IRQ0);
		else
			kunmap_atomic(v, KM_USER0);
	}
}

int psb_clear_scene(struct psb_scene *scene)
{
	struct ttm_bo_kmap_obj bmo;
	bool is_iomem;
	void *addr;

	int ret = ttm_bo_kmap(scene->hw_data, scene->clear_p_start,
			      scene->clear_num_pages, &bmo);

	PSB_DEBUG_RENDER("Scene clear.\n");
	if (ret)
		return ret;

	addr = ttm_kmap_obj_virtual(&bmo, &is_iomem);
	BUG_ON(is_iomem);
	memset(addr, 0, scene->clear_num_pages << PAGE_SHIFT);
	ttm_bo_kunmap(&bmo);

	return 0;
}

static void psb_destroy_scene(struct kref *kref)
{
	struct psb_scene *scene =
	    container_of(kref, struct psb_scene, kref);

	PSB_DEBUG_RENDER("Scene destroy.\n");
	psb_scheduler_remove_scene_refs(scene);
	ttm_bo_unref(&scene->hw_data);
	kfree(scene);
}

void psb_scene_unref(struct psb_scene **p_scene)
{
	struct psb_scene *scene = *p_scene;

	PSB_DEBUG_RENDER("Scene unref.\n");
	*p_scene = NULL;
	kref_put(&scene->kref, &psb_destroy_scene);
}

struct psb_scene *psb_scene_ref(struct psb_scene *src)
{
	PSB_DEBUG_RENDER("Scene ref.\n");
	kref_get(&src->kref);
	return src;
}

static struct psb_scene *psb_alloc_scene(struct drm_device *dev,
					 uint32_t w, uint32_t h)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_bo_device *bdev = &dev_priv->bdev;
	int ret = -EINVAL;
	struct psb_scene *scene;
	uint32_t bo_size;
	struct psb_xhw_buf buf;

	PSB_DEBUG_RENDER("Alloc scene w %u h %u msaa %u\n", w & 0xffff, h,
			 w >> 16);

	scene = kcalloc(1, sizeof(*scene), GFP_KERNEL);

	if (!scene) {
		DRM_ERROR("Out of memory allocating scene object.\n");
		return NULL;
	}

	scene->dev = dev;
	scene->w = w;
	scene->h = h;
	scene->hw_scene = NULL;
	kref_init(&scene->kref);

	INIT_LIST_HEAD(&buf.head);
	ret = psb_xhw_scene_info(dev_priv, &buf, scene->w, scene->h,
				 scene->hw_cookie, &bo_size,
				 &scene->clear_p_start,
				 &scene->clear_num_pages);
	if (ret)
		goto out_err;

	ret = ttm_buffer_object_create(bdev, bo_size, ttm_bo_type_kernel,
				       DRM_PSB_FLAG_MEM_MMU |
				       TTM_PL_FLAG_CACHED,
				       0, 0, 1, NULL, &scene->hw_data);
	if (ret)
		goto out_err;

	return scene;
out_err:
	kfree(scene);
	return NULL;
}

int psb_validate_scene_pool(struct psb_context *context,
			    struct psb_scene_pool *pool,
			    uint32_t w,
			    uint32_t h,
			    int final_pass, struct psb_scene **scene_p)
{
	struct drm_device *dev = pool->dev;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct psb_scene *scene = pool->scenes[pool->cur_scene];
	int ret;
	unsigned long irq_flags;
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	uint32_t bin_pt_offset;
	uint32_t bin_param_offset;

	PSB_DEBUG_RENDER("Validate scene pool. Scene %u\n",
			 pool->cur_scene);

	if (unlikely(!dev_priv->ta_mem)) {
		dev_priv->ta_mem =
		    psb_alloc_ta_mem(dev, dev_priv->ta_mem_pages);
		if (!dev_priv->ta_mem)
			return -ENOMEM;

		bin_pt_offset = ~0;
		bin_param_offset = ~0;
	} else {
		bin_pt_offset = dev_priv->ta_mem->hw_data->offset;
		bin_param_offset = dev_priv->ta_mem->ta_memory->offset;
	}

	pool->w = w;
	pool->h = h;
	if (scene && (scene->w != pool->w || scene->h != pool->h)) {
		spin_lock_irqsave(&scheduler->lock, irq_flags);
		if (scene->flags & PSB_SCENE_FLAG_DIRTY) {
			spin_unlock_irqrestore(&scheduler->lock,
					       irq_flags);
			DRM_ERROR("Trying to resize a dirty scene.\n");
			return -EINVAL;
		}
		spin_unlock_irqrestore(&scheduler->lock, irq_flags);
		psb_scene_unref(&pool->scenes[pool->cur_scene]);
		scene = NULL;
	}

	if (!scene) {
		pool->scenes[pool->cur_scene] = scene =
		    psb_alloc_scene(pool->dev, pool->w, pool->h);

		if (!scene)
			return -ENOMEM;

		scene->flags = PSB_SCENE_FLAG_CLEARED;
	}

	ret = psb_validate_kernel_buffer(context, scene->hw_data,
					 PSB_ENGINE_TA,
					 PSB_BO_FLAG_SCENE |
					 PSB_GPU_ACCESS_READ |
					 PSB_GPU_ACCESS_WRITE, 0);
	if (unlikely(ret != 0))
		return ret;

	/*
	 * FIXME: We need atomic bit manipulation here for the
	 * scheduler. For now use the spinlock.
	 */

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	if (!(scene->flags & PSB_SCENE_FLAG_CLEARED)) {
		spin_unlock_irqrestore(&scheduler->lock, irq_flags);
		PSB_DEBUG_RENDER("Waiting to clear scene memory.\n");
		mutex_lock(&scene->hw_data->mutex);

		ret = ttm_bo_wait(scene->hw_data, 0, 1, 0);
		mutex_unlock(&scene->hw_data->mutex);
		if (ret)
			return ret;

		ret = psb_clear_scene(scene);

		if (ret)
			return ret;
		spin_lock_irqsave(&scheduler->lock, irq_flags);
		scene->flags |= PSB_SCENE_FLAG_CLEARED;
	}
	spin_unlock_irqrestore(&scheduler->lock, irq_flags);

	ret = psb_validate_kernel_buffer(context, dev_priv->ta_mem->hw_data,
					 PSB_ENGINE_TA,
					 PSB_BO_FLAG_SCENE |
					 PSB_GPU_ACCESS_READ |
					 PSB_GPU_ACCESS_WRITE, 0);
	if (unlikely(ret != 0))
		return ret;

	ret =
	    psb_validate_kernel_buffer(context,
				       dev_priv->ta_mem->ta_memory,
				       PSB_ENGINE_TA,
				       PSB_BO_FLAG_SCENE |
				       PSB_GPU_ACCESS_READ |
				       PSB_GPU_ACCESS_WRITE, 0);

	if (unlikely(ret != 0))
		return ret;

	if (unlikely(bin_param_offset !=
		     dev_priv->ta_mem->ta_memory->offset ||
		     bin_pt_offset !=
		     dev_priv->ta_mem->hw_data->offset ||
		     dev_priv->force_ta_mem_load)) {

		struct psb_xhw_buf buf;

		INIT_LIST_HEAD(&buf.head);
		ret = psb_xhw_ta_mem_load(dev_priv, &buf,
					  PSB_TA_MEM_FLAG_TA |
					  PSB_TA_MEM_FLAG_RASTER |
					  PSB_TA_MEM_FLAG_HOSTA |
					  PSB_TA_MEM_FLAG_HOSTD |
					  PSB_TA_MEM_FLAG_INIT,
					  dev_priv->ta_mem->ta_memory->
					  offset,
					  dev_priv->ta_mem->hw_data->
					  offset,
					  dev_priv->ta_mem->hw_cookie);
		if (ret)
			return ret;

		dev_priv->force_ta_mem_load = 0;
	}

	if (final_pass) {

		/*
		 * Clear the scene on next use. Advance the scene counter.
		 */

		spin_lock_irqsave(&scheduler->lock, irq_flags);
		scene->flags &= ~PSB_SCENE_FLAG_CLEARED;
		spin_unlock_irqrestore(&scheduler->lock, irq_flags);
		pool->cur_scene = (pool->cur_scene + 1) % pool->num_scenes;
	}

	*scene_p = psb_scene_ref(scene);
	return 0;
}

static void psb_scene_pool_destroy(struct kref *kref)
{
	struct psb_scene_pool *pool =
	    container_of(kref, struct psb_scene_pool, kref);
	int i;
	PSB_DEBUG_RENDER("Scene pool destroy.\n");

	for (i = 0; i < pool->num_scenes; ++i) {
		PSB_DEBUG_RENDER("scenes %d is 0x%08lx\n", i,
				 (unsigned long) pool->scenes[i]);
		if (pool->scenes[i])
			psb_scene_unref(&pool->scenes[i]);
	}

	kfree(pool);
}

void psb_scene_pool_unref(struct psb_scene_pool **p_pool)
{
	struct psb_scene_pool *pool = *p_pool;

	PSB_DEBUG_RENDER("Scene pool unref\n");
	*p_pool = NULL;
	kref_put(&pool->kref, &psb_scene_pool_destroy);
}

struct psb_scene_pool *psb_scene_pool_ref(struct psb_scene_pool *src)
{
	kref_get(&src->kref);
	return src;
}

/*
 * Callback for base object manager.
 */

static void psb_scene_pool_release(struct ttm_base_object **p_base)
{
	struct ttm_base_object *base = *p_base;
	struct psb_scene_pool *pool =
	    container_of(base, struct psb_scene_pool, base);
	*p_base = NULL;

	psb_scene_pool_unref(&pool);
}

struct psb_scene_pool *psb_scene_pool_lookup(struct drm_file *file_priv,
					     uint32_t handle,
					     int check_owner)
{
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;
	struct ttm_base_object *base;
	struct psb_scene_pool *pool;


	base = ttm_base_object_lookup(tfile, handle);
	if (!base || (base->object_type != PSB_USER_OBJECT_SCENE_POOL)) {
		DRM_ERROR("Could not find scene pool object 0x%08x\n",
			  handle);
		return NULL;
	}

	if (check_owner && tfile != base->tfile && !base->shareable) {
		ttm_base_object_unref(&base);
		return NULL;
	}

	pool = container_of(base, struct psb_scene_pool, base);
	kref_get(&pool->kref);
	ttm_base_object_unref(&base);
	return pool;
}

struct psb_scene_pool *psb_scene_pool_alloc(struct drm_file *file_priv,
					    int shareable,
					    uint32_t num_scenes,
					    uint32_t w, uint32_t h)
{
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;
	struct drm_device *dev = file_priv->minor->dev;
	struct psb_scene_pool *pool;
	int ret;

	PSB_DEBUG_RENDER("Scene pool alloc\n");
	pool = kcalloc(1, sizeof(*pool), GFP_KERNEL);
	if (!pool) {
		DRM_ERROR("Out of memory allocating scene pool object.\n");
		return NULL;
	}
	pool->w = w;
	pool->h = h;
	pool->dev = dev;
	pool->num_scenes = num_scenes;
	kref_init(&pool->kref);

	/*
	 * The base object holds a reference.
	 */

	kref_get(&pool->kref);
	ret = ttm_base_object_init(tfile, &pool->base, shareable,
				   PSB_USER_OBJECT_SCENE_POOL,
				   &psb_scene_pool_release, NULL);
	if (unlikely(ret != 0))
		goto out_err;

	return pool;
out_err:
	kfree(pool);
	return NULL;
}

/*
 * Code to support multiple ta memory buffers.
 */

static void psb_ta_mem_destroy(struct kref *kref)
{
	struct psb_ta_mem *ta_mem =
	    container_of(kref, struct psb_ta_mem, kref);

	ttm_bo_unref(&ta_mem->hw_data);
	ttm_bo_unref(&ta_mem->ta_memory);
	kfree(ta_mem);
}

void psb_ta_mem_unref(struct psb_ta_mem **p_ta_mem)
{
	struct psb_ta_mem *ta_mem = *p_ta_mem;
	*p_ta_mem = NULL;
	kref_put(&ta_mem->kref, psb_ta_mem_destroy);
}

struct psb_ta_mem *psb_ta_mem_ref(struct psb_ta_mem *src)
{
	kref_get(&src->kref);
	return src;
}

struct psb_ta_mem *psb_alloc_ta_mem(struct drm_device *dev, uint32_t pages)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_bo_device *bdev = &dev_priv->bdev;
	int ret = -EINVAL;
	struct psb_ta_mem *ta_mem;
	uint32_t bo_size;
	uint32_t ta_min_size;
	struct psb_xhw_buf buf;

	INIT_LIST_HEAD(&buf.head);

	ta_mem = kcalloc(1, sizeof(*ta_mem), GFP_KERNEL);

	if (!ta_mem) {
		DRM_ERROR("Out of memory allocating parameter memory.\n");
		return NULL;
	}

	kref_init(&ta_mem->kref);
	ret = psb_xhw_ta_mem_info(dev_priv, &buf, pages,
				  ta_mem->hw_cookie,
				  &bo_size,
				  &ta_min_size);
	if (ret == -ENOMEM) {
		DRM_ERROR("Parameter memory size is too small.\n");
		DRM_INFO("Attempted to use %u kiB of parameter memory.\n",
			 (unsigned int) (pages * (PAGE_SIZE / 1024)));
		DRM_INFO("The Xpsb driver thinks this is too small and\n");
		DRM_INFO("suggests %u kiB. Check the psb DRM\n",
			 (unsigned int)(ta_min_size / 1024));
		DRM_INFO("\"ta_mem_size\" parameter!\n");
	}
	if (ret)
		goto out_err0;

	ret = ttm_buffer_object_create(bdev, bo_size, ttm_bo_type_kernel,
				       DRM_PSB_FLAG_MEM_MMU,
				       0, 0, 0, NULL,
				       &ta_mem->hw_data);
	if (ret)
		goto out_err0;

	bo_size = pages * PAGE_SIZE;
	ret =
	    ttm_buffer_object_create(bdev, bo_size,
				     ttm_bo_type_kernel,
				     DRM_PSB_FLAG_MEM_RASTGEOM,
				     0,
				     1024 * 1024 >> PAGE_SHIFT, 0,
				     NULL,
				     &ta_mem->ta_memory);
	if (ret)
		goto out_err1;

	return ta_mem;
out_err1:
	ttm_bo_unref(&ta_mem->hw_data);
out_err0:
	kfree(ta_mem);
	return NULL;
}

int drm_psb_scene_unref_ioctl(struct drm_device *dev,
			      void *data, struct drm_file *file_priv)
{
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;
	struct drm_psb_scene *scene = (struct drm_psb_scene *) data;
	int ret = 0;
	struct drm_psb_private *dev_priv = psb_priv(dev);
	if (!scene->handle_valid)
		return 0;
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);

	ret =
	    ttm_ref_object_base_unref(tfile, scene->handle, TTM_REF_USAGE);
	if (unlikely(ret != 0))
		DRM_ERROR("Could not unreference a scene object.\n");
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	return ret;
}
