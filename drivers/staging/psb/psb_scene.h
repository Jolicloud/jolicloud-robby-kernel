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

#ifndef _PSB_SCENE_H_
#define _PSB_SCENE_H_

#include "ttm/ttm_object.h"

#define PSB_USER_OBJECT_SCENE_POOL    ttm_driver_type0
#define PSB_USER_OBJECT_TA_MEM        ttm_driver_type1
#define PSB_MAX_NUM_SCENES            8

struct psb_hw_scene;
struct psb_hw_ta_mem;

struct psb_scene_pool {
	struct ttm_base_object base;
	struct drm_device *dev;
	struct kref kref;
	uint32_t w;
	uint32_t h;
	uint32_t cur_scene;
	struct psb_scene *scenes[PSB_MAX_NUM_SCENES];
	uint32_t num_scenes;
};

struct psb_scene {
	struct drm_device *dev;
	struct kref kref;
	uint32_t hw_cookie[PSB_SCENE_HW_COOKIE_SIZE];
	uint32_t bo_size;
	uint32_t w;
	uint32_t h;
	struct psb_ta_mem *ta_mem;
	struct psb_hw_scene *hw_scene;
	struct ttm_buffer_object *hw_data;
	uint32_t flags;
	uint32_t clear_p_start;
	uint32_t clear_num_pages;
};

#if 0
struct psb_scene_entry {
	struct list_head head;
	struct psb_scene *scene;
};

struct psb_user_scene {
	struct ttm_base_object base;
	struct drm_device *dev;
};

#endif

struct psb_ta_mem {
	struct ttm_base_object base;
	struct drm_device *dev;
	struct kref kref;
	uint32_t hw_cookie[PSB_TA_MEM_HW_COOKIE_SIZE];
	uint32_t bo_size;
	struct ttm_buffer_object *ta_memory;
	struct ttm_buffer_object *hw_data;
	int is_deallocating;
	int deallocating_scheduled;
};

extern struct psb_scene_pool *psb_scene_pool_alloc(struct drm_file *priv,
						   int shareable,
						   uint32_t num_scenes,
						   uint32_t w, uint32_t h);
extern void psb_scene_pool_unref(struct psb_scene_pool **pool);
extern struct psb_scene_pool *psb_scene_pool_lookup(struct drm_file
						    *priv,
						    uint32_t handle,
						    int check_owner);
extern int psb_validate_scene_pool(struct psb_context *context,
				   struct psb_scene_pool *pool,
				   uint32_t w,
				   uint32_t h, int final_pass,
				   struct psb_scene **scene_p);
extern void psb_scene_unref(struct psb_scene **scene);
extern struct psb_scene *psb_scene_ref(struct psb_scene *src);
extern int drm_psb_scene_unref_ioctl(struct drm_device *dev,
				     void *data,
				     struct drm_file *file_priv);

static inline uint32_t psb_scene_pool_handle(struct psb_scene_pool *pool)
{
	return pool->base.hash.key;
}

extern struct psb_ta_mem *psb_alloc_ta_mem(struct drm_device *dev,
					   uint32_t pages);
extern struct psb_ta_mem *psb_ta_mem_ref(struct psb_ta_mem *src);
extern void psb_ta_mem_unref(struct psb_ta_mem **ta_mem);

#endif
