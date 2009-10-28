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

struct psb_use_reg {
	struct drm_reg reg;
	struct drm_psb_private *dev_priv;
	uint32_t reg_seq;
	uint32_t base;
	uint32_t data_master;
};

struct psb_use_reg_data {
	uint32_t base;
	uint32_t size;
	uint32_t data_master;
};

static int psb_use_reg_reusable(const struct drm_reg *reg, const void *data)
{
	struct psb_use_reg *use_reg =
	    container_of(reg, struct psb_use_reg, reg);
	struct psb_use_reg_data *use_data = (struct psb_use_reg_data *)data;

	return ((use_reg->base <= use_data->base) &&
		(use_reg->base + PSB_USE_OFFSET_SIZE >
		 use_data->base + use_data->size) &&
		use_reg->data_master == use_data->data_master);
}

static int psb_use_reg_set(struct psb_use_reg *use_reg,
			   const struct psb_use_reg_data *use_data)
{
	struct drm_psb_private *dev_priv = use_reg->dev_priv;

	if (use_reg->reg.fence == NULL)
		use_reg->data_master = use_data->data_master;

	if (use_reg->reg.fence == NULL &&
	    !psb_use_reg_reusable(&use_reg->reg, (const void *)use_data)) {

		use_reg->base = use_data->base & ~PSB_USE_OFFSET_MASK;
		use_reg->data_master = use_data->data_master;

		if (!psb_use_reg_reusable(&use_reg->reg,
					  (const void *)use_data)) {
			DRM_ERROR("USE base mechanism didn't support "
				  "buffer size or alignment\n");
			return -EINVAL;
		}

		PSB_WSGX32(PSB_ALPL(use_reg->base, _PSB_CUC_BASE_ADDR) |
			   (use_reg->data_master << _PSB_CUC_BASE_DM_SHIFT),
			   PSB_CR_USE_CODE_BASE(use_reg->reg_seq));
	}
	return 0;

}

int psb_grab_use_base(struct drm_psb_private *dev_priv,
		      unsigned long base,
		      unsigned long size,
		      unsigned int data_master,
		      uint32_t fence_class,
		      uint32_t fence_type,
		      int no_wait,
		      int interruptible, int *r_reg, uint32_t * r_offset)
{
	struct psb_use_reg_data use_data = {
		.base = base,
		.size = size,
		.data_master = data_master
	};
	int ret;

	struct drm_reg *reg;
	struct psb_use_reg *use_reg;

	ret = drm_regs_alloc(&dev_priv->use_manager,
			     (const void *)&use_data,
			     fence_class,
			     fence_type, interruptible, no_wait, &reg);
	if (ret)
		return ret;

	use_reg = container_of(reg, struct psb_use_reg, reg);
	ret = psb_use_reg_set(use_reg, &use_data);

	if (ret)
		return ret;

	*r_reg = use_reg->reg_seq;
	*r_offset = base - use_reg->base;

	return 0;
};

static void psb_use_reg_destroy(struct drm_reg *reg)
{
	struct psb_use_reg *use_reg =
	    container_of(reg, struct psb_use_reg, reg);
	struct drm_psb_private *dev_priv = use_reg->dev_priv;

	PSB_WSGX32(PSB_ALPL(0, _PSB_CUC_BASE_ADDR),
		   PSB_CR_USE_CODE_BASE(use_reg->reg_seq));

	drm_free(use_reg, sizeof(*use_reg), DRM_MEM_DRIVER);
}

int psb_init_use_base(struct drm_psb_private *dev_priv,
		      unsigned int reg_start, unsigned int reg_num)
{
	struct psb_use_reg *use_reg;
	int i;
	int ret = 0;

	mutex_lock(&dev_priv->cmdbuf_mutex);

	drm_regs_init(&dev_priv->use_manager,
		      &psb_use_reg_reusable, &psb_use_reg_destroy);

	for (i = reg_start; i < reg_start + reg_num; ++i) {
		use_reg = drm_calloc(1, sizeof(*use_reg), DRM_MEM_DRIVER);
		if (!use_reg) {
			ret = -ENOMEM;
			goto out;
		}

		use_reg->dev_priv = dev_priv;
		use_reg->reg_seq = i;
		use_reg->base = 0;
		use_reg->data_master = _PSB_CUC_DM_PIXEL;

		PSB_WSGX32(PSB_ALPL(use_reg->base, _PSB_CUC_BASE_ADDR) |
			   (use_reg->data_master << _PSB_CUC_BASE_DM_SHIFT),
			   PSB_CR_USE_CODE_BASE(use_reg->reg_seq));

		drm_regs_add(&dev_priv->use_manager, &use_reg->reg);
	}
      out:
	mutex_unlock(&dev_priv->cmdbuf_mutex);

	return ret;

}

void psb_takedown_use_base(struct drm_psb_private *dev_priv)
{
	mutex_lock(&dev_priv->cmdbuf_mutex);
	drm_regs_free(&dev_priv->use_manager);
	mutex_unlock(&dev_priv->cmdbuf_mutex);
}
