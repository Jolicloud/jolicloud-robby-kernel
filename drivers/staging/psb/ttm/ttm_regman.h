/**************************************************************************
 *
 * Copyright (c) 2006-2008 Tungsten Graphics, Inc., Cedar Park, TX., USA
 * All Rights Reserved.
 * Copyright (c) 2009 VMware, Inc., Palo Alto, CA., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 */

#ifndef _TTM_REGMAN_H_
#define _TTM_REGMAN_H_

#include <linux/list.h>

struct ttm_fence_object;

struct ttm_reg {
	struct list_head head;
	struct ttm_fence_object *fence;
	uint32_t fence_type;
	uint32_t new_fence_type;
};

struct ttm_reg_manager {
	struct list_head free;
	struct list_head lru;
	struct list_head unfenced;

	int (*reg_reusable)(const struct ttm_reg *reg, const void *data);
	void (*reg_destroy)(struct ttm_reg *reg);
};

extern int ttm_regs_alloc(struct ttm_reg_manager *manager,
			  const void *data,
			  uint32_t fence_class,
			  uint32_t fence_type,
			  int interruptible,
			  int no_wait,
			  struct ttm_reg **reg);

extern void ttm_regs_fence(struct ttm_reg_manager *regs,
			   struct ttm_fence_object *fence);

extern void ttm_regs_free(struct ttm_reg_manager *manager);
extern void ttm_regs_add(struct ttm_reg_manager *manager, struct ttm_reg *reg);
extern void ttm_regs_init(struct ttm_reg_manager *manager,
			  int (*reg_reusable)(const struct ttm_reg *,
					      const void *),
			  void (*reg_destroy)(struct ttm_reg *));

#endif
