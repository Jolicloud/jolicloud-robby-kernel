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
#include "psb_scene.h"

void psb_clear_scene_atomic(struct psb_scene *scene)
{
	int i;
	struct page *page;
	void *v;

	for (i = 0; i < scene->clear_num_pages; ++i) {
		page = drm_ttm_get_page(scene->hw_data->ttm,
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
	struct drm_bo_kmap_obj bmo;
	int is_iomem;
	void *addr;

	int ret = drm_bo_kmap(scene->hw_data, scene->clear_p_start,
			      scene->clear_num_pages, &bmo);

	PSB_DEBUG_RENDER("Scene clear\n");
	if (ret)
		return ret;

	addr = drm_bmo_virtual(&bmo, &is_iomem);
	BUG_ON(is_iomem);
	memset(addr, 0, scene->clear_num_pages << PAGE_SHIFT);
	drm_bo_kunmap(&bmo);

	return 0;
}

static void psb_destroy_scene_devlocked(struct psb_scene *scene)
{
	if (!scene)
		return;

	PSB_DEBUG_RENDER("Scene destroy\n");
	drm_bo_usage_deref_locked(&scene->hw_data);
	drm_free(scene, sizeof(*scene), DRM_MEM_DRIVER);
}

void psb_scene_unref_devlocked(struct psb_scene **scene)
{
	struct psb_scene *tmp_scene = *scene;

	PSB_DEBUG_RENDER("Scene unref\n");
	*scene = NULL;
	if (atomic_dec_and_test(&tmp_scene->ref_count)) {
		psb_scheduler_remove_scene_refs(tmp_scene);
		psb_destroy_scene_devlocked(tmp_scene);
	}
}

struct psb_scene *psb_scene_ref(struct psb_scene *src)
{
	PSB_DEBUG_RENDER("Scene ref\n");
	atomic_inc(&src->ref_count);
	return src;
}

static struct psb_scene *psb_alloc_scene(struct drm_device *dev,
					 uint32_t w, uint32_t h)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	int ret = -EINVAL;
	struct psb_scene *scene;
	uint32_t bo_size;
	struct psb_xhw_buf buf;

	PSB_DEBUG_RENDER("Alloc scene w %u h %u\n", w, h);

	scene = drm_calloc(1, sizeof(*scene), DRM_MEM_DRIVER);

	if (!scene) {
		DRM_ERROR("Out of memory allocating scene object.\n");
		return NULL;
	}

	scene->dev = dev;
	scene->w = w;
	scene->h = h;
	scene->hw_scene = NULL;
	atomic_set(&scene->ref_count, 1);

	INIT_LIST_HEAD(&buf.head);
	ret = psb_xhw_scene_info(dev_priv, &buf, scene->w, scene->h,
				 scene->hw_cookie, &bo_size,
				 &scene->clear_p_start,
				 &scene->clear_num_pages);
	if (ret)
		goto out_err;

	ret = drm_buffer_object_create(dev, bo_size, drm_bo_type_kernel,
				       DRM_PSB_FLAG_MEM_MMU |
				       DRM_BO_FLAG_READ |
				       DRM_BO_FLAG_CACHED |
				       PSB_BO_FLAG_SCENE |
				       DRM_BO_FLAG_WRITE,
				       DRM_BO_HINT_DONT_FENCE,
				       0, 0, &scene->hw_data);
	if (ret)
		goto out_err;

	return scene;
      out_err:
	drm_free(scene, sizeof(*scene), DRM_MEM_DRIVER);
	return NULL;
}

int psb_validate_scene_pool(struct psb_scene_pool *pool, uint64_t flags,
			    uint64_t mask,
			    uint32_t hint,
			    uint32_t w,
			    uint32_t h,
			    int final_pass, struct psb_scene **scene_p)
{
	struct drm_device *dev = pool->dev;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct psb_scene *scene = pool->scenes[pool->cur_scene];
	int ret;
	unsigned long irq_flags;
	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	uint32_t bin_pt_offset;
	uint32_t bin_param_offset;

	PSB_DEBUG_RENDER("Validate scene pool. Scene %u\n", pool->cur_scene);

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
			spin_unlock_irqrestore(&scheduler->lock, irq_flags);
			DRM_ERROR("Trying to resize a dirty scene.\n");
			return -EINVAL;
		}
		spin_unlock_irqrestore(&scheduler->lock, irq_flags);
		mutex_lock(&dev->struct_mutex);
		psb_scene_unref_devlocked(&pool->scenes[pool->cur_scene]);
		mutex_unlock(&dev->struct_mutex);
		scene = NULL;
	}

	if (!scene) {
		pool->scenes[pool->cur_scene] = scene =
		    psb_alloc_scene(pool->dev, pool->w, pool->h);

		if (!scene)
			return -ENOMEM;

		scene->flags = PSB_SCENE_FLAG_CLEARED;
	}

	/*
	 * FIXME: We need atomic bit manipulation here for the
	 * scheduler. For now use the spinlock.
	 */

	spin_lock_irqsave(&scheduler->lock, irq_flags);
	if (!(scene->flags & PSB_SCENE_FLAG_CLEARED)) {
		spin_unlock_irqrestore(&scheduler->lock, irq_flags);
		PSB_DEBUG_RENDER("Waiting to clear scene memory.\n");
		mutex_lock(&scene->hw_data->mutex);
		ret = drm_bo_wait(scene->hw_data, 0, 0, 0);
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

	ret = drm_bo_do_validate(scene->hw_data, flags, mask, hint,
				 PSB_ENGINE_TA, 0, NULL);
	if (ret)
		return ret;
	ret = drm_bo_do_validate(dev_priv->ta_mem->hw_data, 0, 0, 0,
				 PSB_ENGINE_TA, 0, NULL);
	if (ret)
		return ret;
	ret = drm_bo_do_validate(dev_priv->ta_mem->ta_memory, 0, 0, 0,
				 PSB_ENGINE_TA, 0, NULL);
	if (ret)
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
					  dev_priv->ta_mem->ta_memory->offset,
					  dev_priv->ta_mem->hw_data->offset,
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

static void psb_scene_pool_destroy_devlocked(struct psb_scene_pool *pool)
{
	int i;

	if (!pool)
		return;

	PSB_DEBUG_RENDER("Scene pool destroy.\n");
	for (i = 0; i < pool->num_scenes; ++i) {
		PSB_DEBUG_RENDER("scenes %d is 0x%08lx\n", i,
				 (unsigned long)pool->scenes[i]);
		if (pool->scenes[i])
			psb_scene_unref_devlocked(&pool->scenes[i]);
	}
	drm_free(pool, sizeof(*pool), DRM_MEM_DRIVER);
}

void psb_scene_pool_unref_devlocked(struct psb_scene_pool **pool)
{
	struct psb_scene_pool *tmp_pool = *pool;
	struct drm_device *dev = tmp_pool->dev;

	PSB_DEBUG_RENDER("Scene pool unref\n");
	(void)dev;
	DRM_ASSERT_LOCKED(&dev->struct_mutex);
	*pool = NULL;
	if (--tmp_pool->ref_count == 0)
		psb_scene_pool_destroy_devlocked(tmp_pool);
}

struct psb_scene_pool *psb_scene_pool_ref_devlocked(struct psb_scene_pool *src)
{
	++src->ref_count;
	return src;
}

/*
 * Callback for user object manager.
 */

static void psb_scene_pool_destroy(struct drm_file *priv,
				   struct drm_user_object *base)
{
	struct psb_scene_pool *pool =
	    drm_user_object_entry(base, struct psb_scene_pool, user);

	psb_scene_pool_unref_devlocked(&pool);
}

struct psb_scene_pool *psb_scene_pool_lookup_devlocked(struct drm_file *priv,
						       uint32_t handle,
						       int check_owner)
{
	struct drm_user_object *uo;
	struct psb_scene_pool *pool;

	uo = drm_lookup_user_object(priv, handle);
	if (!uo || (uo->type != PSB_USER_OBJECT_SCENE_POOL)) {
		DRM_ERROR("Could not find scene pool object 0x%08x\n", handle);
		return NULL;
	}

	if (check_owner && priv != uo->owner) {
		if (!drm_lookup_ref_object(priv, uo, _DRM_REF_USE))
			return NULL;
	}

	pool = drm_user_object_entry(uo, struct psb_scene_pool, user);
	return psb_scene_pool_ref_devlocked(pool);
}

struct psb_scene_pool *psb_scene_pool_alloc(struct drm_file *priv,
					    int shareable,
					    uint32_t num_scenes,
					    uint32_t w, uint32_t h)
{
	struct drm_device *dev = priv->head->dev;
	struct psb_scene_pool *pool;
	int ret;

	PSB_DEBUG_RENDER("Scene pool alloc\n");
	pool = drm_calloc(1, sizeof(*pool), DRM_MEM_DRIVER);
	if (!pool) {
		DRM_ERROR("Out of memory allocating scene pool object.\n");
		return NULL;
	}
	pool->w = w;
	pool->h = h;
	pool->dev = dev;
	pool->num_scenes = num_scenes;

	mutex_lock(&dev->struct_mutex);
	ret = drm_add_user_object(priv, &pool->user, shareable);
	if (ret)
		goto out_err;

	pool->user.type = PSB_USER_OBJECT_SCENE_POOL;
	pool->user.remove = &psb_scene_pool_destroy;
	pool->ref_count = 2;
	mutex_unlock(&dev->struct_mutex);
	return pool;
      out_err:
	drm_free(pool, sizeof(*pool), DRM_MEM_DRIVER);
	return NULL;
}

/*
 * Code to support multiple ta memory buffers.
 */

static void psb_destroy_ta_mem_devlocked(struct psb_ta_mem *ta_mem)
{
	if (!ta_mem)
		return;

	drm_bo_usage_deref_locked(&ta_mem->hw_data);
	drm_bo_usage_deref_locked(&ta_mem->ta_memory);
	drm_free(ta_mem, sizeof(*ta_mem), DRM_MEM_DRIVER);
}

void psb_ta_mem_unref_devlocked(struct psb_ta_mem **ta_mem)
{
	struct psb_ta_mem *tmp_ta_mem = *ta_mem;
	struct drm_device *dev = tmp_ta_mem->dev;

	(void)dev;
	DRM_ASSERT_LOCKED(&dev->struct_mutex);
	*ta_mem = NULL;
	if (--tmp_ta_mem->ref_count == 0)
		psb_destroy_ta_mem_devlocked(tmp_ta_mem);
}

void psb_ta_mem_ref_devlocked(struct psb_ta_mem **dst, struct psb_ta_mem *src)
{
	struct drm_device *dev = src->dev;

	(void)dev;
	DRM_ASSERT_LOCKED(&dev->struct_mutex);
	*dst = src;
	++src->ref_count;
}

struct psb_ta_mem *psb_alloc_ta_mem(struct drm_device *dev, uint32_t pages)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	int ret = -EINVAL;
	struct psb_ta_mem *ta_mem;
	uint32_t bo_size;
	struct psb_xhw_buf buf;

	INIT_LIST_HEAD(&buf.head);

	ta_mem = drm_calloc(1, sizeof(*ta_mem), DRM_MEM_DRIVER);

	if (!ta_mem) {
		DRM_ERROR("Out of memory allocating parameter memory.\n");
		return NULL;
	}

	ret = psb_xhw_ta_mem_info(dev_priv, &buf, pages,
				  ta_mem->hw_cookie, &bo_size);
	if (ret == -ENOMEM) {
		DRM_ERROR("Parameter memory size is too small.\n");
		DRM_INFO("Attempted to use %u kiB of parameter memory.\n",
			 (unsigned int)(pages * (PAGE_SIZE / 1024)));
		DRM_INFO("The Xpsb driver thinks this is too small and\n");
		DRM_INFO("suggests %u kiB. Check the psb DRM\n",
			 (unsigned int)(bo_size / 1024));
		DRM_INFO("\"ta_mem_size\" parameter!\n");
	}
	if (ret)
		goto out_err0;

	bo_size = pages * PAGE_SIZE;
	ta_mem->dev = dev;
	ret = drm_buffer_object_create(dev, bo_size, drm_bo_type_kernel,
				       DRM_PSB_FLAG_MEM_MMU | DRM_BO_FLAG_READ |
				       DRM_BO_FLAG_WRITE |
				       PSB_BO_FLAG_SCENE,
				       DRM_BO_HINT_DONT_FENCE, 0, 0,
				       &ta_mem->hw_data);
	if (ret)
		goto out_err0;

	ret =
	    drm_buffer_object_create(dev, pages << PAGE_SHIFT,
				     drm_bo_type_kernel,
				     DRM_PSB_FLAG_MEM_RASTGEOM |
				     DRM_BO_FLAG_READ |
				     DRM_BO_FLAG_WRITE |
				     PSB_BO_FLAG_SCENE,
				     DRM_BO_HINT_DONT_FENCE, 0,
				     1024 * 1024 >> PAGE_SHIFT,
				     &ta_mem->ta_memory);
	if (ret)
		goto out_err1;

	ta_mem->ref_count = 1;
	return ta_mem;
      out_err1:
	drm_bo_usage_deref_unlocked(&ta_mem->hw_data);
      out_err0:
	drm_free(ta_mem, sizeof(*ta_mem), DRM_MEM_DRIVER);
	return NULL;
}

int drm_psb_scene_unref_ioctl(struct drm_device *dev,
			      void *data, struct drm_file *file_priv)
{
	struct drm_psb_scene *scene = (struct drm_psb_scene *)data;
	struct drm_user_object *uo;
	struct drm_ref_object *ro;
	int ret = 0;

	mutex_lock(&dev->struct_mutex);
	if (!scene->handle_valid)
		goto out_unlock;

	uo = drm_lookup_user_object(file_priv, scene->handle);
	if (!uo) {
		ret = -EINVAL;
		goto out_unlock;
	}
	if (uo->type != PSB_USER_OBJECT_SCENE_POOL) {
		DRM_ERROR("Not a scene pool object.\n");
		ret = -EINVAL;
		goto out_unlock;
	}
	if (uo->owner != file_priv) {
		DRM_ERROR("Not owner of scene pool object.\n");
		ret = -EPERM;
		goto out_unlock;
	}

	scene->handle_valid = 0;
	ro = drm_lookup_ref_object(file_priv, uo, _DRM_REF_USE);
	BUG_ON(!ro);
	drm_remove_ref_object(file_priv, ro);

      out_unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}
