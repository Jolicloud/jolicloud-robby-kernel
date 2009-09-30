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

#ifndef _PSB_SCHEDULE_H_
#define _PSB_SCHEDULE_H_

#include "drmP.h"

enum psb_task_type {
	psb_ta_midscene_task,
	psb_ta_task,
	psb_raster_task,
	psb_freescene_task
};

#define PSB_MAX_TA_CMDS 60
#define PSB_MAX_RASTER_CMDS 60
#define PSB_MAX_OOM_CMDS 6

struct psb_xhw_buf {
	struct list_head head;
	int copy_back;
	atomic_t done;
	struct drm_psb_xhw_arg arg;

};

struct psb_feedback_info {
	struct drm_buffer_object *bo;
	struct page *page;
	uint32_t offset;
};

struct psb_task {
	struct list_head head;
	struct psb_scene *scene;
	struct psb_feedback_info feedback;
	enum psb_task_type task_type;
	uint32_t engine;
	uint32_t sequence;
	uint32_t ta_cmds[PSB_MAX_TA_CMDS];
	uint32_t raster_cmds[PSB_MAX_RASTER_CMDS];
	uint32_t oom_cmds[PSB_MAX_OOM_CMDS];
	uint32_t ta_cmd_size;
	uint32_t raster_cmd_size;
	uint32_t oom_cmd_size;
	uint32_t feedback_offset;
	uint32_t ta_complete_action;
	uint32_t raster_complete_action;
	uint32_t hw_cookie;
	uint32_t flags;
	uint32_t reply_flags;
	uint32_t aborting;
	struct psb_xhw_buf buf;

#ifdef PSB_DETEAR
	uint32_t bVideoFlag;
	uint32_t x, y, w, h;
	uint32_t pFBBOHandle;
	void *pFBVirtAddr;
#endif
};

struct psb_hw_scene {
	struct list_head head;
	uint32_t context_number;

	/*
	 * This pointer does not refcount the last_scene_buffer,
	 * so we must make sure it is set to NULL before destroying
	 * the corresponding task.
	 */

	struct psb_scene *last_scene;
};

struct psb_scene;
struct drm_psb_private;

struct psb_scheduler_seq {
	uint32_t sequence;
	int reported;
};

struct psb_scheduler {
	struct drm_device *dev;
	struct psb_scheduler_seq seq[_PSB_ENGINE_TA_FENCE_TYPES];
	struct psb_hw_scene hs[PSB_NUM_HW_SCENES];
	struct mutex task_wq_mutex;
	spinlock_t lock;
	struct list_head hw_scenes;
	struct list_head ta_queue;
	struct list_head raster_queue;
	struct list_head hp_raster_queue;
	struct list_head task_done_queue;
	struct psb_task *current_task[PSB_SCENE_NUM_ENGINES];
	struct psb_task *feedback_task;
	int ta_state;
	struct psb_hw_scene *pending_hw_scene;
	uint32_t pending_hw_scene_seq;
	struct delayed_work wq;
	struct psb_scene_pool *pool;
	uint32_t idle_count;
	int idle;
	wait_queue_head_t idle_queue;
	unsigned long ta_end_jiffies;
	unsigned long raster_end_jiffies;
	unsigned long total_raster_jiffies;
};

#define PSB_RF_FIRE_TA       (1 << 0)
#define PSB_RF_OOM           (1 << 1)
#define PSB_RF_OOM_REPLY     (1 << 2)
#define PSB_RF_TERMINATE     (1 << 3)
#define PSB_RF_TA_DONE       (1 << 4)
#define PSB_RF_FIRE_RASTER   (1 << 5)
#define PSB_RF_RASTER_DONE   (1 << 6)
#define PSB_RF_DEALLOC       (1 << 7)

extern struct psb_scene_pool *psb_alloc_scene_pool(struct drm_file *priv,
						   int shareable, uint32_t w,
						   uint32_t h);
extern uint32_t psb_scene_handle(struct psb_scene *scene);
extern int psb_scheduler_init(struct drm_device *dev,
			      struct psb_scheduler *scheduler);
extern void psb_scheduler_takedown(struct psb_scheduler *scheduler);
extern int psb_cmdbuf_ta(struct drm_file *priv,
			 struct drm_psb_cmdbuf_arg *arg,
			 struct drm_buffer_object *cmd_buffer,
			 struct drm_buffer_object *ta_buffer,
			 struct drm_buffer_object *oom_buffer,
			 struct psb_scene *scene,
			 struct psb_feedback_info *feedback,
			 struct drm_fence_arg *fence_arg);
extern int psb_cmdbuf_raster(struct drm_file *priv,
			     struct drm_psb_cmdbuf_arg *arg,
			     struct drm_buffer_object *cmd_buffer,
			     struct drm_fence_arg *fence_arg);
extern void psb_scheduler_handler(struct drm_psb_private *dev_priv,
				  uint32_t status);
extern void psb_scheduler_pause(struct drm_psb_private *dev_priv);
extern void psb_scheduler_restart(struct drm_psb_private *dev_priv);
extern int psb_scheduler_idle(struct drm_psb_private *dev_priv);
extern int psb_scheduler_finished(struct drm_psb_private *dev_priv);

extern void psb_scheduler_lockup(struct drm_psb_private *dev_priv,
				 int *lockup, int *idle);
extern void psb_scheduler_reset(struct drm_psb_private *dev_priv,
				int error_condition);
extern int psb_forced_user_interrupt(struct drm_psb_private *dev_priv);
extern void psb_scheduler_remove_scene_refs(struct psb_scene *scene);
extern void psb_scheduler_ta_mem_check(struct drm_psb_private *dev_priv);
extern int psb_extend_raster_timeout(struct drm_psb_private *dev_priv);

#endif
