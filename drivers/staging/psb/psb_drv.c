/**************************************************************************
 * Copyright (c) 2007, Intel Corporation.
 * All Rights Reserved.
 * Copyright (c) 2008, Tungsten Graphics, Inc. Cedar Park, TX., USA.
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

#include <drm/drmP.h>
#include <drm/drm.h>
#include "psb_drm.h"
#include "psb_drv.h"
#include "psb_reg.h"
#include "intel_reg.h"
#include "psb_msvdx.h"
#include "lnc_topaz.h"
#include <drm/drm_pciids.h>
#include "psb_scene.h"

#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>

int drm_psb_debug;
EXPORT_SYMBOL(drm_psb_debug);
static int drm_psb_trap_pagefaults;
static int drm_psb_clock_gating;
static int drm_psb_ta_mem_size = 32 * 1024;

int drm_psb_disable_vsync;
int drm_psb_no_fb;
int drm_psb_force_pipeb;
int drm_idle_check_interval = 5;
int drm_psb_ospm;

MODULE_PARM_DESC(debug, "Enable debug output");
MODULE_PARM_DESC(clock_gating, "clock gating");
MODULE_PARM_DESC(no_fb, "Disable FBdev");
MODULE_PARM_DESC(trap_pagefaults, "Error and reset on MMU pagefaults");
MODULE_PARM_DESC(disable_vsync, "Disable vsync interrupts");
MODULE_PARM_DESC(force_pipeb, "Forces PIPEB to become primary fb");
MODULE_PARM_DESC(ta_mem_size, "TA memory size in kiB");
MODULE_PARM_DESC(ospm, "switch for ospm support");
module_param_named(debug, drm_psb_debug, int, 0600);
module_param_named(clock_gating, drm_psb_clock_gating, int, 0600);
module_param_named(no_fb, drm_psb_no_fb, int, 0600);
module_param_named(trap_pagefaults, drm_psb_trap_pagefaults, int, 0600);
module_param_named(disable_vsync, drm_psb_disable_vsync, int, 0600);
module_param_named(force_pipeb, drm_psb_force_pipeb, int, 0600);
module_param_named(ta_mem_size, drm_psb_ta_mem_size, int, 0600);
module_param_named(ospm, drm_psb_ospm, int, 0600);

#ifndef CONFIG_X86_PAT
#warning "Don't build this driver without PAT support!!!"
#endif

#define psb_PCI_IDS \
    {0x8086, 0x8108, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_PSB_8108}, \
    {0x8086, 0x8109, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_PSB_8109}, \
    {0x8086, 0x4100, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_MRST_4100}, \
    {0x8086, 0x4101, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_MRST_4100}, \
    {0x8086, 0x4102, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_MRST_4100}, \
    {0x8086, 0x4103, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_MRST_4100}, \
    {0x8086, 0x4104, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_MRST_4100}, \
    {0x8086, 0x4105, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_MRST_4100}, \
    {0x8086, 0x4106, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_MRST_4100}, \
    {0x8086, 0x4107, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CHIP_MRST_4100}, \
    {0, 0, 0}

static struct pci_device_id pciidlist[] = {
	psb_PCI_IDS
};

/*
 * Standard IOCTLs.
 */

#define DRM_IOCTL_PSB_KMS_OFF	  DRM_IO(DRM_PSB_KMS_OFF + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_KMS_ON	  DRM_IO(DRM_PSB_KMS_ON + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_VT_LEAVE    DRM_IO(DRM_PSB_VT_LEAVE + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_VT_ENTER    DRM_IO(DRM_PSB_VT_ENTER + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_XHW_INIT    DRM_IOW(DRM_PSB_XHW_INIT + DRM_COMMAND_BASE, \
					struct drm_psb_xhw_init_arg)
#define DRM_IOCTL_PSB_XHW         DRM_IO(DRM_PSB_XHW + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_EXTENSION   DRM_IOWR(DRM_PSB_EXTENSION + DRM_COMMAND_BASE, \
                                         union drm_psb_extension_arg)
/*
 * TTM execbuf extension.
 */

#define DRM_PSB_CMDBUF            (DRM_PSB_EXTENSION + 1)
#define DRM_PSB_SCENE_UNREF       (DRM_PSB_CMDBUF + 1)
#define DRM_IOCTL_PSB_CMDBUF      DRM_IOW(DRM_PSB_CMDBUF + DRM_COMMAND_BASE,		\
					  struct drm_psb_cmdbuf_arg)
#define DRM_IOCTL_PSB_SCENE_UNREF DRM_IOW(DRM_PSB_SCENE_UNREF + DRM_COMMAND_BASE, \
					   struct drm_psb_scene)
#define DRM_IOCTL_PSB_KMS_OFF	  DRM_IO(DRM_PSB_KMS_OFF + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_KMS_ON	  DRM_IO(DRM_PSB_KMS_ON + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_EXTENSION   DRM_IOWR(DRM_PSB_EXTENSION + DRM_COMMAND_BASE, \
					   union drm_psb_extension_arg)
/*
 * TTM placement user extension.
 */

#define DRM_PSB_PLACEMENT_OFFSET   (DRM_PSB_SCENE_UNREF + 1)

#define DRM_PSB_TTM_PL_CREATE      (TTM_PL_CREATE + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_REFERENCE   (TTM_PL_REFERENCE + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_UNREF       (TTM_PL_UNREF + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_SYNCCPU     (TTM_PL_SYNCCPU + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_WAITIDLE    (TTM_PL_WAITIDLE + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_SETSTATUS   (TTM_PL_SETSTATUS + DRM_PSB_PLACEMENT_OFFSET)

/*
 * TTM fence extension.
 */

#define DRM_PSB_FENCE_OFFSET       (DRM_PSB_TTM_PL_SETSTATUS + 1)
#define DRM_PSB_TTM_FENCE_SIGNALED (TTM_FENCE_SIGNALED + DRM_PSB_FENCE_OFFSET)
#define DRM_PSB_TTM_FENCE_FINISH   (TTM_FENCE_FINISH + DRM_PSB_FENCE_OFFSET)
#define DRM_PSB_TTM_FENCE_UNREF    (TTM_FENCE_UNREF + DRM_PSB_FENCE_OFFSET)

#define DRM_IOCTL_PSB_TTM_PL_CREATE    \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_CREATE,\
		 union ttm_pl_create_arg)
#define DRM_IOCTL_PSB_TTM_PL_REFERENCE \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_REFERENCE,\
		 union ttm_pl_reference_arg)
#define DRM_IOCTL_PSB_TTM_PL_UNREF    \
	DRM_IOW(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_UNREF,\
		struct ttm_pl_reference_req)
#define DRM_IOCTL_PSB_TTM_PL_SYNCCPU    \
	DRM_IOW(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_SYNCCPU,\
		struct ttm_pl_synccpu_arg)
#define DRM_IOCTL_PSB_TTM_PL_WAITIDLE    \
	DRM_IOW(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_WAITIDLE,\
		struct ttm_pl_waitidle_arg)
#define DRM_IOCTL_PSB_TTM_PL_SETSTATUS \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_SETSTATUS,\
		 union ttm_pl_setstatus_arg)
#define DRM_IOCTL_PSB_TTM_FENCE_SIGNALED \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_FENCE_SIGNALED,	\
		  union ttm_fence_signaled_arg)
#define DRM_IOCTL_PSB_TTM_FENCE_FINISH \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_FENCE_FINISH,	\
		 union ttm_fence_finish_arg)
#define DRM_IOCTL_PSB_TTM_FENCE_UNREF \
	DRM_IOW(DRM_COMMAND_BASE + DRM_PSB_TTM_FENCE_UNREF,	\
		 struct ttm_fence_unref_arg)

static int psb_vt_leave_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);
static int psb_vt_enter_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);

#define PSB_IOCTL_DEF(ioctl, func, flags) \
	[DRM_IOCTL_NR(ioctl) - DRM_COMMAND_BASE] = {ioctl, func, flags}

static struct drm_ioctl_desc psb_ioctls[] = {
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_KMS_OFF, psbfb_kms_off_ioctl,
		      DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_KMS_ON, psbfb_kms_on_ioctl, DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_VT_LEAVE, psb_vt_leave_ioctl,
		      DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_VT_ENTER, psb_vt_enter_ioctl, DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_XHW_INIT, psb_xhw_init_ioctl,
		      DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_XHW, psb_xhw_ioctl, DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_EXTENSION, psb_extension_ioctl, DRM_AUTH),

	PSB_IOCTL_DEF(DRM_IOCTL_PSB_CMDBUF, psb_cmdbuf_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_SCENE_UNREF, drm_psb_scene_unref_ioctl,
		      DRM_AUTH),

	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_CREATE, psb_pl_create_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_REFERENCE, psb_pl_reference_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_UNREF, psb_pl_unref_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_SYNCCPU, psb_pl_synccpu_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_WAITIDLE, psb_pl_waitidle_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_SETSTATUS, psb_pl_setstatus_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_FENCE_SIGNALED,
		      psb_fence_signaled_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_FENCE_FINISH, psb_fence_finish_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_FENCE_UNREF, psb_fence_unref_ioctl,
		      DRM_AUTH)
};

static int psb_max_ioctl = DRM_ARRAY_SIZE(psb_ioctls);

static void get_ci_info(struct drm_psb_private *dev_priv)
{
	struct pci_dev *pdev;

	pdev = pci_get_subsys(0x8086, 0x080b, 0, 0, NULL);
	if (pdev == NULL) {
		/* IF no pci_device we set size & addr to 0, no ci
		 * share buffer can be created */
		dev_priv->ci_region_start = 0;
		dev_priv->ci_region_size = 0;
		printk(KERN_ERR "can't find CI device, no ci share buffer\n");
		return;
	}

	dev_priv->ci_region_start = pci_resource_start(pdev, 1);
	dev_priv->ci_region_size = pci_resource_len(pdev, 1);

	printk(KERN_INFO "ci_region_start %x ci_region_size %d\n",
	       dev_priv->ci_region_start, dev_priv->ci_region_size);

	pci_dev_put(pdev);

	return;
}

static void psb_set_uopt(struct drm_psb_uopt *uopt)
{
	uopt->clock_gating = drm_psb_clock_gating;
}

static void psb_lastclose(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	if (!dev->dev_private)
		return;

	if (dev_priv->ta_mem)
		psb_ta_mem_unref(&dev_priv->ta_mem);
	mutex_lock(&dev_priv->cmdbuf_mutex);
	if (dev_priv->context.buffers) {
		vfree(dev_priv->context.buffers);
		dev_priv->context.buffers = NULL;
	}
	mutex_unlock(&dev_priv->cmdbuf_mutex);
}

static void psb_do_takedown(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_bo_device *bdev = &dev_priv->bdev;


	if (dev_priv->have_mem_rastgeom) {
		ttm_bo_clean_mm(bdev, DRM_PSB_MEM_RASTGEOM);
		dev_priv->have_mem_rastgeom = 0;
	}
	if (dev_priv->have_mem_mmu) {
		ttm_bo_clean_mm(bdev, DRM_PSB_MEM_MMU);
		dev_priv->have_mem_mmu = 0;
	}
	if (dev_priv->have_mem_aper) {
		ttm_bo_clean_mm(bdev, DRM_PSB_MEM_APER);
		dev_priv->have_mem_aper = 0;
	}
	if (dev_priv->have_tt) {
		ttm_bo_clean_mm(bdev, TTM_PL_TT);
		dev_priv->have_tt = 0;
	}
	if (dev_priv->have_vram) {
		ttm_bo_clean_mm(bdev, TTM_PL_VRAM);
		dev_priv->have_vram = 0;
	}
	if (dev_priv->have_camera) {
		ttm_bo_clean_mm(bdev, TTM_PL_CI);
		dev_priv->have_camera = 0;
	}

	if (dev_priv->has_msvdx)
		psb_msvdx_uninit(dev);

	if (IS_MRST(dev)) {
		if (dev_priv->has_topaz)
			lnc_topaz_uninit(dev);
	}

	if (dev_priv->comm) {
		kunmap(dev_priv->comm_page);
		dev_priv->comm = NULL;
	}
	if (dev_priv->comm_page) {
		__free_page(dev_priv->comm_page);
		dev_priv->comm_page = NULL;
	}
}

void psb_clockgating(struct drm_psb_private *dev_priv)
{
	uint32_t clock_gating;

	if (dev_priv->uopt.clock_gating == 1) {
		PSB_DEBUG_INIT("Disabling clock gating.\n");

		clock_gating = (_PSB_C_CLKGATECTL_CLKG_DISABLED <<
				_PSB_C_CLKGATECTL_2D_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_DISABLED <<
		     _PSB_C_CLKGATECTL_ISP_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_DISABLED <<
		     _PSB_C_CLKGATECTL_TSP_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_DISABLED <<
		     _PSB_C_CLKGATECTL_TA_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_DISABLED <<
		     _PSB_C_CLKGATECTL_DPM_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_DISABLED <<
		     _PSB_C_CLKGATECTL_USE_CLKG_SHIFT);

	} else if (dev_priv->uopt.clock_gating == 2) {
		PSB_DEBUG_INIT("Enabling clock gating.\n");

		clock_gating = (_PSB_C_CLKGATECTL_CLKG_AUTO <<
				_PSB_C_CLKGATECTL_2D_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_AUTO <<
		     _PSB_C_CLKGATECTL_ISP_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_AUTO <<
		     _PSB_C_CLKGATECTL_TSP_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_AUTO <<
		     _PSB_C_CLKGATECTL_TA_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_AUTO <<
		     _PSB_C_CLKGATECTL_DPM_CLKG_SHIFT) |
		    (_PSB_C_CLKGATECTL_CLKG_AUTO <<
		     _PSB_C_CLKGATECTL_USE_CLKG_SHIFT);
	} else
		clock_gating = PSB_RSGX32(PSB_CR_CLKGATECTL);

#ifdef FIX_TG_2D_CLOCKGATE
	clock_gating &= ~_PSB_C_CLKGATECTL_2D_CLKG_MASK;
	clock_gating |= (_PSB_C_CLKGATECTL_CLKG_DISABLED <<
			 _PSB_C_CLKGATECTL_2D_CLKG_SHIFT);
#endif
	PSB_WSGX32(clock_gating, PSB_CR_CLKGATECTL);
	(void) PSB_RSGX32(PSB_CR_CLKGATECTL);
}

#define FB_REG06 0xD0810600
#define FB_MIPI_DISABLE  BIT11
#define FB_REG09 0xD0810900
#define FB_SKU_MASK  (BIT12|BIT13|BIT14)
#define FB_SKU_SHIFT 12
#define FB_SKU_100 0
#define FB_SKU_100L 1
#define FB_SKU_83 2
#if 1 /* FIXME remove it after PO */
#define FB_GFX_CLK_DIVIDE_MASK  (BIT20|BIT21|BIT22)
#define FB_GFX_CLK_DIVIDE_SHIFT 20
#define FB_VED_CLK_DIVIDE_MASK  (BIT23|BIT24)
#define FB_VED_CLK_DIVIDE_SHIFT 23
#define FB_VEC_CLK_DIVIDE_MASK  (BIT25|BIT26)
#define FB_VEC_CLK_DIVIDE_SHIFT 25
#endif  /* FIXME remove it after PO */


void mrst_get_fuse_settings(struct drm_psb_private *dev_priv)
{
	struct pci_dev *pci_root = pci_get_bus_and_slot(0, 0);
	uint32_t fuse_value = 0;
	uint32_t fuse_value_tmp = 0;

	pci_write_config_dword(pci_root, 0xD0, FB_REG06);
	pci_read_config_dword(pci_root, 0xD4, &fuse_value);

	dev_priv->iLVDS_enable = fuse_value & FB_MIPI_DISABLE;

	DRM_INFO("internal display is %s\n",
		 dev_priv->iLVDS_enable ? "LVDS display" : "MIPI display");

	pci_write_config_dword(pci_root, 0xD0, FB_REG09);
	pci_read_config_dword(pci_root, 0xD4, &fuse_value);

	DRM_INFO("SKU values is 0x%x. \n", fuse_value);
	fuse_value_tmp = (fuse_value & FB_SKU_MASK) >> FB_SKU_SHIFT;

	switch (fuse_value_tmp) {
	case FB_SKU_100:
		DRM_INFO("SKU values is SKU_100. LNC core clock is 200MHz. \n");
		dev_priv->sku_100 = true;
		break;
	case FB_SKU_100L:
		DRM_INFO("SKU values is SKU_100L. LNC core clock is 100MHz. \n");
		dev_priv->sku_100L = true;
		break;
	case FB_SKU_83:
		DRM_INFO("SKU values is SKU_83. LNC core clock is 166MHz. \n");
		dev_priv->sku_83 = true;
		break;
	default:
		DRM_ERROR("Invalid SKU values, SKU value = 0x%08x\n",
			  fuse_value_tmp);
	}

#if 1 /* FIXME remove it after PO */
	fuse_value_tmp = (fuse_value & FB_GFX_CLK_DIVIDE_MASK) >> FB_GFX_CLK_DIVIDE_SHIFT;

	switch (fuse_value_tmp) {
	case 0:
		DRM_INFO("Gfx clk : core clk = 1:1. \n");
		break;
	case 1:
		DRM_INFO("Gfx clk : core clk = 4:3. \n");
		break;
	case 2:
		DRM_INFO("Gfx clk : core clk = 8:5. \n");
		break;
	case 3:
		DRM_INFO("Gfx clk : core clk = 2:1. \n");
		break;
	case 5:
		DRM_INFO("Gfx clk : core clk = 8:3. \n");
		break;
	case 6:
		DRM_INFO("Gfx clk : core clk = 16:5. \n");
		break;
	default:
		DRM_ERROR("Invalid GFX CLK DIVIDE values, value = 0x%08x\n",
			  fuse_value_tmp);
	}

	fuse_value_tmp = (fuse_value & FB_VED_CLK_DIVIDE_MASK) >> FB_VED_CLK_DIVIDE_SHIFT;

	switch (fuse_value_tmp) {
	case 0:
		DRM_INFO("Ved clk : core clk = 1:1. \n");
		break;
	case 1:
		DRM_INFO("Ved clk : core clk = 4:3. \n");
		break;
	case 2:
		DRM_INFO("Ved clk : core clk = 8:5. \n");
		break;
	case 3:
		DRM_INFO("Ved clk : core clk = 2:1. \n");
		break;
	default:
		DRM_ERROR("Invalid VED CLK DIVIDE values, value = 0x%08x\n",
			  fuse_value_tmp);
	}

	fuse_value_tmp = (fuse_value & FB_VEC_CLK_DIVIDE_MASK) >> FB_VEC_CLK_DIVIDE_SHIFT;

	switch (fuse_value_tmp) {
	case 0:
		DRM_INFO("Vec clk : core clk = 1:1. \n");
		break;
	case 1:
		DRM_INFO("Vec clk : core clk = 4:3. \n");
		break;
	case 2:
		DRM_INFO("Vec clk : core clk = 8:5. \n");
		break;
	case 3:
		DRM_INFO("Vec clk : core clk = 2:1. \n");
		break;
	default:
		DRM_ERROR("Invalid VEC CLK DIVIDE values, value = 0x%08x\n",
			  fuse_value_tmp);
	}
#endif /* FIXME remove it after PO */

	return;
}

static int psb_do_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_bo_device *bdev = &dev_priv->bdev;
	struct psb_gtt *pg = dev_priv->pg;

	uint32_t stolen_gtt;
	uint32_t tt_start;
	uint32_t tt_pages;

	int ret = -ENOMEM;

	dev_priv->ta_mem_pages =
	    PSB_ALIGN_TO(drm_psb_ta_mem_size * 1024,
			 PAGE_SIZE) >> PAGE_SHIFT;
	dev_priv->comm_page = alloc_page(GFP_KERNEL);
	if (!dev_priv->comm_page)
		goto out_err;

	dev_priv->comm = kmap(dev_priv->comm_page);
	memset((void *) dev_priv->comm, 0, PAGE_SIZE);

	set_pages_uc(dev_priv->comm_page, 1);

	/*
	 * Initialize sequence numbers for the different command
	 * submission mechanisms.
	 */

	dev_priv->sequence[PSB_ENGINE_2D] = 0;
	dev_priv->sequence[PSB_ENGINE_RASTERIZER] = 0;
	dev_priv->sequence[PSB_ENGINE_TA] = 0;
	dev_priv->sequence[PSB_ENGINE_HPRAST] = 0;

	if (pg->gatt_start & 0x0FFFFFFF) {
		DRM_ERROR("Gatt must be 256M aligned. This is a bug.\n");
		ret = -EINVAL;
		goto out_err;
	}

	stolen_gtt = (pg->stolen_size >> PAGE_SHIFT) * 4;
	stolen_gtt = (stolen_gtt + PAGE_SIZE - 1) >> PAGE_SHIFT;
	stolen_gtt =
	    (stolen_gtt < pg->gtt_pages) ? stolen_gtt : pg->gtt_pages;

	dev_priv->gatt_free_offset = pg->gatt_start +
	    (stolen_gtt << PAGE_SHIFT) * 1024;

	/*
	 * Insert a cache-coherent communications page in mmu space
	 * just after the stolen area. Will be used for fencing etc.
	 */

	dev_priv->comm_mmu_offset = dev_priv->gatt_free_offset;
	dev_priv->gatt_free_offset += PAGE_SIZE;

	ret = psb_mmu_insert_pages(psb_mmu_get_default_pd(dev_priv->mmu),
				   &dev_priv->comm_page,
				   dev_priv->comm_mmu_offset, 1, 0, 0, 0);

	if (ret)
		goto out_err;

	if (1 || drm_debug) {
		uint32_t core_id = PSB_RSGX32(PSB_CR_CORE_ID);
		uint32_t core_rev = PSB_RSGX32(PSB_CR_CORE_REVISION);
		DRM_INFO("SGX core id = 0x%08x\n", core_id);
		DRM_INFO("SGX core rev major = 0x%02x, minor = 0x%02x\n",
			 (core_rev & _PSB_CC_REVISION_MAJOR_MASK) >>
			 _PSB_CC_REVISION_MAJOR_SHIFT,
			 (core_rev & _PSB_CC_REVISION_MINOR_MASK) >>
			 _PSB_CC_REVISION_MINOR_SHIFT);
		DRM_INFO
		    ("SGX core rev maintenance = 0x%02x, designer = 0x%02x\n",
		     (core_rev & _PSB_CC_REVISION_MAINTENANCE_MASK) >>
		     _PSB_CC_REVISION_MAINTENANCE_SHIFT,
		     (core_rev & _PSB_CC_REVISION_DESIGNER_MASK) >>
		     _PSB_CC_REVISION_DESIGNER_SHIFT);
	}

	spin_lock_init(&dev_priv->irqmask_lock);
	dev_priv->fence0_irq_on = 0;

	tt_pages = (pg->gatt_pages < PSB_TT_PRIV0_PLIMIT) ?
	    pg->gatt_pages : PSB_TT_PRIV0_PLIMIT;
	tt_start = dev_priv->gatt_free_offset - pg->gatt_start;
	tt_pages -= tt_start >> PAGE_SHIFT;

	if (!ttm_bo_init_mm(bdev, TTM_PL_VRAM, 0,
			    pg->vram_stolen_size >> PAGE_SHIFT)) {
		dev_priv->have_vram = 1;
	}

	if (!ttm_bo_init_mm(bdev, TTM_PL_CI, 0,
				dev_priv->ci_region_size >> PAGE_SHIFT)) {
		dev_priv->have_camera = 1;
	}

	if (!ttm_bo_init_mm(bdev, TTM_PL_TT, tt_start >> PAGE_SHIFT,
			    tt_pages)) {
		dev_priv->have_tt = 1;
	}

	if (!ttm_bo_init_mm(bdev, DRM_PSB_MEM_MMU, 0x00000000,
			    (pg->gatt_start - PSB_MEM_MMU_START -
			     pg->ci_stolen_size) >> PAGE_SHIFT)) {
		dev_priv->have_mem_mmu = 1;
	}

	if (!ttm_bo_init_mm(bdev, DRM_PSB_MEM_RASTGEOM, 0x00000000,
			    (PSB_MEM_MMU_START -
			     PSB_MEM_RASTGEOM_START) >> PAGE_SHIFT)) {
		dev_priv->have_mem_rastgeom = 1;
	}
#if 0
	if (pg->gatt_pages > PSB_TT_PRIV0_PLIMIT) {
		if (!ttm_bo_init_mm
		    (bdev, DRM_PSB_MEM_APER, PSB_TT_PRIV0_PLIMIT,
		     pg->gatt_pages - PSB_TT_PRIV0_PLIMIT, 1)) {
			dev_priv->have_mem_aper = 1;
		}
	}
#endif

	PSB_DEBUG_INIT("Init MSVDX\n");
	dev_priv->has_msvdx = 1;
	if (psb_msvdx_init(dev))
		dev_priv->has_msvdx = 0;

	if (IS_MRST(dev)) {
		PSB_DEBUG_INIT("Init Topaz\n");
		dev_priv->has_topaz = 1;
		if (lnc_topaz_init(dev))
			dev_priv->has_topaz = 0;
	}
	return 0;
out_err:
	psb_do_takedown(dev);
	return ret;
}

static int psb_driver_unload(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	if (drm_psb_no_fb == 0)
		psb_modeset_cleanup(dev);

	if (dev_priv) {
		struct ttm_bo_device *bdev = &dev_priv->bdev;

		psb_watchdog_takedown(dev_priv);
		psb_do_takedown(dev);
		psb_xhw_takedown(dev_priv);
		psb_scheduler_takedown(&dev_priv->scheduler);

		if (dev_priv->have_mem_pds) {
			ttm_bo_clean_mm(bdev, DRM_PSB_MEM_PDS);
			dev_priv->have_mem_pds = 0;
		}
		if (dev_priv->have_mem_kernel) {
			ttm_bo_clean_mm(bdev, DRM_PSB_MEM_KERNEL);
			dev_priv->have_mem_kernel = 0;
		}

		if (dev_priv->pf_pd) {
			psb_mmu_free_pagedir(dev_priv->pf_pd);
			dev_priv->pf_pd = NULL;
		}
		if (dev_priv->mmu) {
			struct psb_gtt *pg = dev_priv->pg;

			down_read(&pg->sem);
			psb_mmu_remove_pfn_sequence(psb_mmu_get_default_pd
						    (dev_priv->mmu),
						    pg->gatt_start,
						    pg->vram_stolen_size >>
						    PAGE_SHIFT);
			psb_mmu_remove_pfn_sequence(psb_mmu_get_default_pd
						    (dev_priv->mmu),
						    pg->gatt_start - pg->ci_stolen_size,
						    pg->ci_stolen_size >>
						    PAGE_SHIFT);
			up_read(&pg->sem);
			psb_mmu_driver_takedown(dev_priv->mmu);
			dev_priv->mmu = NULL;
		}
		psb_gtt_takedown(dev_priv->pg, 1);
		if (dev_priv->scratch_page) {
			__free_page(dev_priv->scratch_page);
			dev_priv->scratch_page = NULL;
		}
		if (dev_priv->has_bo_device) {
			ttm_bo_device_release(&dev_priv->bdev);
			dev_priv->has_bo_device = 0;
		}
		if (dev_priv->has_fence_device) {
			ttm_fence_device_release(&dev_priv->fdev);
			dev_priv->has_fence_device = 0;
		}
		if (dev_priv->vdc_reg) {
			iounmap(dev_priv->vdc_reg);
			dev_priv->vdc_reg = NULL;
		}
		if (dev_priv->sgx_reg) {
			iounmap(dev_priv->sgx_reg);
			dev_priv->sgx_reg = NULL;
		}
		if (dev_priv->msvdx_reg) {
			iounmap(dev_priv->msvdx_reg);
			dev_priv->msvdx_reg = NULL;
		}

		if (IS_MRST(dev)) {
			if (dev_priv->topaz_reg) {
				iounmap(dev_priv->topaz_reg);
				dev_priv->topaz_reg = NULL;
			}
		}

		if (dev_priv->tdev)
			ttm_object_device_release(&dev_priv->tdev);

		if (dev_priv->has_global)
			psb_ttm_global_release(dev_priv);

		kfree(dev_priv);
		dev->dev_private = NULL;
	}
	return 0;
}


static int psb_driver_load(struct drm_device *dev, unsigned long chipset)
{
	struct drm_psb_private *dev_priv;
	struct ttm_bo_device *bdev;
	unsigned long resource_start;
	struct psb_gtt *pg;
	int ret = -ENOMEM;

	if (IS_MRST(dev))
		DRM_INFO("Run drivers on Moorestown platform!\n");
	else
		DRM_INFO("Run drivers on Poulsbo platform!\n");

	dev_priv = kcalloc(1, sizeof(*dev_priv), GFP_KERNEL);
	if (dev_priv == NULL)
		return -ENOMEM;

	dev_priv->dev = dev;
	bdev = &dev_priv->bdev;

	ret = psb_ttm_global_init(dev_priv);
	if (unlikely(ret != 0))
		goto out_err;
	dev_priv->has_global = 1;

	dev_priv->tdev = ttm_object_device_init
		(dev_priv->mem_global_ref.object, PSB_OBJECT_HASH_ORDER);
	if (unlikely(dev_priv->tdev == NULL))
		goto out_err;

	mutex_init(&dev_priv->temp_mem);
	mutex_init(&dev_priv->cmdbuf_mutex);
	mutex_init(&dev_priv->reset_mutex);
	INIT_LIST_HEAD(&dev_priv->context.validate_list);
	INIT_LIST_HEAD(&dev_priv->context.kern_validate_list);
	psb_init_disallowed();

#ifdef FIX_TG_16
	atomic_set(&dev_priv->lock_2d, 0);
	atomic_set(&dev_priv->ta_wait_2d, 0);
	atomic_set(&dev_priv->ta_wait_2d_irq, 0);
	atomic_set(&dev_priv->waiters_2d, 0);;
	DRM_INIT_WAITQUEUE(&dev_priv->queue_2d);
#else
	mutex_init(&dev_priv->mutex_2d);
#endif

	spin_lock_init(&dev_priv->reloc_lock);

	DRM_INIT_WAITQUEUE(&dev_priv->rel_mapped_queue);
	DRM_INIT_WAITQUEUE(&dev_priv->event_2d_queue);

	dev->dev_private = (void *) dev_priv;
	dev_priv->chipset = chipset;
	psb_set_uopt(&dev_priv->uopt);

	PSB_DEBUG_GENERAL("Init watchdog and scheduler\n");
	psb_watchdog_init(dev_priv);
	psb_scheduler_init(dev, &dev_priv->scheduler);


	PSB_DEBUG_INIT("Mapping MMIO\n");
	resource_start = pci_resource_start(dev->pdev, PSB_MMIO_RESOURCE);

	if (IS_MRST(dev))
		dev_priv->msvdx_reg =
		    ioremap(resource_start + MRST_MSVDX_OFFSET,
			    PSB_MSVDX_SIZE);
	else
		dev_priv->msvdx_reg =
		    ioremap(resource_start + PSB_MSVDX_OFFSET,
			    PSB_MSVDX_SIZE);

	if (!dev_priv->msvdx_reg)
		goto out_err;

	if (IS_MRST(dev)) {
		dev_priv->topaz_reg =
		    ioremap(resource_start + LNC_TOPAZ_OFFSET,
			    LNC_TOPAZ_SIZE);
		if (!dev_priv->topaz_reg)
			goto out_err;
	}

	dev_priv->vdc_reg =
	    ioremap(resource_start + PSB_VDC_OFFSET, PSB_VDC_SIZE);
	if (!dev_priv->vdc_reg)
		goto out_err;

	if (IS_MRST(dev))
		dev_priv->sgx_reg =
		    ioremap(resource_start + MRST_SGX_OFFSET,
			    PSB_SGX_SIZE);
	else
		dev_priv->sgx_reg =
		    ioremap(resource_start + PSB_SGX_OFFSET, PSB_SGX_SIZE);

	if (!dev_priv->sgx_reg)
		goto out_err;

	if (IS_MRST(dev))
		mrst_get_fuse_settings(dev_priv);

	PSB_DEBUG_INIT("Init TTM fence and BO driver\n");

	get_ci_info(dev_priv);

	psb_clockgating(dev_priv);

	ret = psb_ttm_fence_device_init(&dev_priv->fdev);
	if (unlikely(ret != 0))
		goto out_err;

	dev_priv->has_fence_device = 1;
	ret = ttm_bo_device_init(bdev,
				 dev_priv->mem_global_ref.object,
				 &psb_ttm_bo_driver,
				 DRM_PSB_FILE_PAGE_OFFSET);
	if (unlikely(ret != 0))
		goto out_err;
	dev_priv->has_bo_device = 1;
	ttm_lock_init(&dev_priv->ttm_lock);

	ret = -ENOMEM;

	dev_priv->scratch_page = alloc_page(GFP_DMA32 | __GFP_ZERO);
	if (!dev_priv->scratch_page)
		goto out_err;

	set_pages_uc(dev_priv->scratch_page, 1);

	dev_priv->pg = psb_gtt_alloc(dev);
	if (!dev_priv->pg)
		goto out_err;

	ret = psb_gtt_init(dev_priv->pg, 0);
	if (ret)
		goto out_err;

	dev_priv->mmu = psb_mmu_driver_init(dev_priv->sgx_reg,
					drm_psb_trap_pagefaults, 0,
					dev_priv);
	if (!dev_priv->mmu)
		goto out_err;

	pg = dev_priv->pg;

	/*
	 * Make sgx MMU aware of the stolen memory area we call VRAM.
	 */

	down_read(&pg->sem);
	ret =
	    psb_mmu_insert_pfn_sequence(psb_mmu_get_default_pd
					(dev_priv->mmu),
					pg->stolen_base >> PAGE_SHIFT,
					pg->gatt_start,
					pg->vram_stolen_size >> PAGE_SHIFT, 0);
	up_read(&pg->sem);
	if (ret)
		goto out_err;

	/*
	 * Make sgx MMU aware of the stolen memory area we call VRAM.
	 */

	down_read(&pg->sem);
	ret =
	    psb_mmu_insert_pfn_sequence(psb_mmu_get_default_pd
					(dev_priv->mmu),
					dev_priv->ci_region_start >> PAGE_SHIFT,
					pg->gatt_start - pg->ci_stolen_size,
					pg->ci_stolen_size >> PAGE_SHIFT, 0);
	up_read(&pg->sem);
	if (ret)
		goto out_err;

	dev_priv->pf_pd = psb_mmu_alloc_pd(dev_priv->mmu, 1, 0);
	if (!dev_priv->pf_pd)
		goto out_err;

	/*
	 * Make all presumably unused requestors page-fault by making them
	 * use context 1 which does not have any valid mappings.
	 */

	PSB_WSGX32(0x00000000, PSB_CR_BIF_BANK0);
	PSB_WSGX32(0x00000000, PSB_CR_BIF_BANK1);
	PSB_RSGX32(PSB_CR_BIF_BANK1);

	psb_mmu_set_pd_context(psb_mmu_get_default_pd(dev_priv->mmu), 0);
	psb_mmu_set_pd_context(dev_priv->pf_pd, 1);
	psb_mmu_enable_requestor(dev_priv->mmu, _PSB_MMU_ER_MASK);

	psb_init_2d(dev_priv);

	ret = ttm_bo_init_mm(bdev, DRM_PSB_MEM_KERNEL, 0x00000000,
			     (PSB_MEM_PDS_START - PSB_MEM_KERNEL_START)
			     >> PAGE_SHIFT);
	if (ret)
		goto out_err;
	dev_priv->have_mem_kernel = 1;

	ret = ttm_bo_init_mm(bdev, DRM_PSB_MEM_PDS, 0x00000000,
			     (PSB_MEM_RASTGEOM_START - PSB_MEM_PDS_START)
			     >> PAGE_SHIFT);
	if (ret)
		goto out_err;
	dev_priv->have_mem_pds = 1;

	PSB_DEBUG_INIT("Begin to init SGX/MSVDX/Topaz\n");

	ret = psb_do_init(dev);
	if (ret)
		return ret;

	ret = psb_xhw_init(dev);
	if (ret)
		return ret;

	PSB_WSGX32(PSB_MEM_PDS_START, PSB_CR_PDS_EXEC_BASE);
	PSB_WSGX32(PSB_MEM_RASTGEOM_START, PSB_CR_BIF_3D_REQ_BASE);

	psb_init_ospm(dev_priv);

	if (drm_psb_no_fb == 0) {
		psb_modeset_init(dev);
		drm_helper_initial_config(dev);
	}
	/*set SGX in low power mode*/
	if (drm_psb_ospm && IS_MRST(dev))
		if (psb_try_power_down_sgx(dev))
			PSB_DEBUG_PM("initialize SGX to low power failed\n");
	return 0;
out_err:
	psb_driver_unload(dev);
	return ret;
}

int psb_driver_device_is_agp(struct drm_device *dev)
{
	return 0;
}

static int psb_prepare_msvdx_suspend(struct drm_device *dev)
{
#ifdef PSB_FIXME
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_fence_device *fdev = &dev_priv->fdev;
	struct ttm_fence_class_manager *fc =
	    &fdev->fence_class[PSB_ENGINE_VIDEO];
	struct ttm_fence_object *fence;
	int ret = 0;
	int signaled = 0;
	int count = 0;
	unsigned long _end = jiffies + 3 * DRM_HZ;

	PSB_DEBUG_GENERAL
	    ("MSVDXACPI Entering psb_prepare_msvdx_suspend....\n");

	/*set the msvdx-reset flag here.. */
	dev_priv->msvdx_needs_reset = 1;

	/*Ensure that all pending IRQs are serviced, */

	/*
	 * Save the last MSVDX fence in dev_priv instead!!!
	 * Need to be fc->write_locked while accessing a fence from the ring.
	 */

	list_for_each_entry(fence, &fc->ring, ring) {
		count++;
		do {
			DRM_WAIT_ON(ret, fc->fence_queue, 3 * DRM_HZ,
				    (signaled =
				     ttm_fence_object_signaled(fence,
							 DRM_FENCE_TYPE_EXE)));
			if (signaled)
				break;
			if (time_after_eq(jiffies, _end))
				PSB_DEBUG_GENERAL
				    ("MSVDXACPI: fence 0x%x didn't get"
				     " signaled for 3 secs; "
				     "we will suspend anyways\n",
				     (unsigned int) fence);
		} while (ret == -EINTR);

	}
	PSB_DEBUG_GENERAL("MSVDXACPI: All MSVDX IRQs (%d) serviced...\n",
			  count);
#endif
	return 0;
}

static int psb_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	if (!down_write_trylock(&dev_priv->sgx_sem))
		return -EBUSY;
	if (dev_priv->graphics_state != PSB_PWR_STATE_D0i0);
		PSB_DEBUG_PM("Not suspending from D0i0\n");
	if (dev_priv->graphics_state == PSB_PWR_STATE_D3)
		goto exit;
	if (drm_psb_no_fb == 0)
		psbfb_suspend(dev);

	dev_priv->saveCLOCKGATING = PSB_RSGX32(PSB_CR_CLKGATECTL);
	(void) psb_idle_3d(dev);
	(void) psb_idle_2d(dev);
	flush_scheduled_work();

	if (dev_priv->has_msvdx)
		psb_prepare_msvdx_suspend(dev);

	if (dev_priv->has_topaz)
		lnc_prepare_topaz_suspend(dev);

#ifdef OSPM_STAT
	if (dev_priv->graphics_state == PSB_PWR_STATE_D0i0)
		dev_priv->gfx_d0i0_time += jiffies - dev_priv->gfx_last_mode_change;
	else if (dev_priv->graphics_state == PSB_PWR_STATE_D0i3)
		dev_priv->gfx_d0i3_time += jiffies - dev_priv->gfx_last_mode_change;
	else
		PSB_DEBUG_PM("suspend: illegal previous power state\n");
	dev_priv->gfx_last_mode_change = jiffies;
	dev_priv->gfx_d3_cnt++;
#endif

	dev_priv->graphics_state = PSB_PWR_STATE_D3;
	dev_priv->msvdx_state = PSB_PWR_STATE_D3;
	dev_priv->topaz_power_state = LNC_TOPAZ_POWEROFF;
	pci_save_state(pdev);
	pci_disable_device(pdev);
	pci_set_power_state(pdev, PCI_D3hot);
	psb_down_island_power(dev, PSB_GRAPHICS_ISLAND | PSB_VIDEO_ENC_ISLAND
				| PSB_VIDEO_DEC_ISLAND);
exit:
	up_write(&dev_priv->sgx_sem);
	return 0;
}

static int psb_resume(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct psb_gtt *pg = dev_priv->pg;
	int ret;
	if (dev_priv->graphics_state != PSB_PWR_STATE_D3)
		return 0;

	psb_up_island_power(dev, PSB_GRAPHICS_ISLAND | PSB_VIDEO_ENC_ISLAND
				| PSB_VIDEO_DEC_ISLAND);
	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	DRM_ERROR("FIXME: topaz's resume is not ready..\n");
#ifdef OSPM_STAT
	if (dev_priv->graphics_state == PSB_PWR_STATE_D3)
		dev_priv->gfx_d3_time += jiffies - dev_priv->gfx_last_mode_change;
	else
		PSB_DEBUG_PM("resume :illegal previous power state\n");
	dev_priv->gfx_last_mode_change = jiffies;
	dev_priv->gfx_d0i0_cnt++;
#endif
	dev_priv->graphics_state = PSB_PWR_STATE_D0i0;
	dev_priv->msvdx_state = PSB_PWR_STATE_D0i0;
	dev_priv->topaz_power_state = LNC_TOPAZ_POWERON;
	INIT_LIST_HEAD(&dev_priv->resume_buf.head);
	dev_priv->msvdx_needs_reset = 1;

	lnc_prepare_topaz_resume(dev);

	PSB_WVDC32(pg->pge_ctl | _PSB_PGETBL_ENABLED, PSB_PGETBL_CTL);
	pci_write_config_word(pdev, PSB_GMCH_CTRL,
			      pg->gmch_ctrl | _PSB_GMCH_ENABLED);

	/*
	 * The GTT page tables are probably not saved.
	 * However, TT and VRAM is empty at this point.
	 */

	psb_gtt_init(dev_priv->pg, 1);

	/*
	 * The SGX loses it's register contents.
	 * Restore BIF registers. The MMU page tables are
	 * "normal" pages, so their contents should be kept.
	 */

	PSB_WSGX32(dev_priv->saveCLOCKGATING, PSB_CR_CLKGATECTL);
	PSB_WSGX32(0x00000000, PSB_CR_BIF_BANK0);
	PSB_WSGX32(0x00000000, PSB_CR_BIF_BANK1);
	PSB_RSGX32(PSB_CR_BIF_BANK1);

	psb_mmu_set_pd_context(psb_mmu_get_default_pd(dev_priv->mmu), 0);
	psb_mmu_set_pd_context(dev_priv->pf_pd, 1);
	psb_mmu_enable_requestor(dev_priv->mmu, _PSB_MMU_ER_MASK);

	/*
	 * 2D Base registers..
	 */
	psb_init_2d(dev_priv);

	/*
	 * Persistant 3D base registers and USSE base registers..
	 */

	PSB_WSGX32(PSB_MEM_PDS_START, PSB_CR_PDS_EXEC_BASE);
	PSB_WSGX32(PSB_MEM_RASTGEOM_START, PSB_CR_BIF_3D_REQ_BASE);
	PSB_WSGX32(dev_priv->sgx2_irq_mask, PSB_CR_EVENT_HOST_ENABLE2);
	PSB_WSGX32(dev_priv->sgx_irq_mask, PSB_CR_EVENT_HOST_ENABLE);
	(void)PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE);

	/*
	 * Now, re-initialize the 3D engine.
	 */

	psb_xhw_resume(dev_priv, &dev_priv->resume_buf);

	psb_scheduler_ta_mem_check(dev_priv);
	if (dev_priv->ta_mem && !dev_priv->force_ta_mem_load) {
		psb_xhw_ta_mem_load(dev_priv, &dev_priv->resume_buf,
				    PSB_TA_MEM_FLAG_TA |
				    PSB_TA_MEM_FLAG_RASTER |
				    PSB_TA_MEM_FLAG_HOSTA |
				    PSB_TA_MEM_FLAG_HOSTD |
				    PSB_TA_MEM_FLAG_INIT,
				    dev_priv->ta_mem->ta_memory->offset,
				    dev_priv->ta_mem->hw_data->offset,
				    dev_priv->ta_mem->hw_cookie);
	}

	if (drm_psb_no_fb == 0)
		psbfb_resume(dev);

	return 0;
}

int psb_extension_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	union drm_psb_extension_arg *arg = data;
	struct drm_psb_extension_rep *rep = &arg->rep;

	if (strcmp(arg->extension, "psb_ttm_placement_alphadrop") == 0) {
		rep->exists = 1;
		rep->driver_ioctl_offset = DRM_PSB_PLACEMENT_OFFSET;
		rep->sarea_offset = 0;
		rep->major = 1;
		rep->minor = 0;
		rep->pl = 0;
		return 0;
	}
	if (strcmp(arg->extension, "psb_ttm_fence_alphadrop") == 0) {
		rep->exists = 1;
		rep->driver_ioctl_offset = DRM_PSB_FENCE_OFFSET;
		rep->sarea_offset = 0;
		rep->major = 1;
		rep->minor = 0;
		rep->pl = 0;
		return 0;
	}
	if (strcmp(arg->extension, "psb_ttm_execbuf_alphadrop") == 0) {
		rep->exists = 1;
		rep->driver_ioctl_offset = DRM_PSB_CMDBUF;
		rep->sarea_offset = 0;
		rep->major = 1;
		rep->minor = 0;
		rep->pl = 0;
		return 0;
	}

	rep->exists = 0;
	return 0;
}

static int psb_vt_leave_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_bo_device *bdev = &dev_priv->bdev;
	struct ttm_mem_type_manager *man;
	int clean;
	int ret;

	ret = ttm_write_lock(&dev_priv->ttm_lock, 1,
			     psb_fpriv(file_priv)->tfile);
	if (unlikely(ret != 0))
		return ret;

	/*
	 * Clean VRAM and TT for fbdev.
	 */

	ret = ttm_bo_evict_mm(&dev_priv->bdev, TTM_PL_VRAM);
	if (unlikely(ret != 0))
		goto out_unlock;

	man = &bdev->man[TTM_PL_VRAM];
	spin_lock(&bdev->lru_lock);
	clean = drm_mm_clean(&man->manager);
	spin_unlock(&bdev->lru_lock);
	if (unlikely(!clean))
		DRM_INFO("Notice: VRAM was not clean after VT switch, if you are running fbdev please ignore.\n");

	ret = ttm_bo_evict_mm(&dev_priv->bdev, TTM_PL_TT);
	if (unlikely(ret != 0))
		goto out_unlock;

	man = &bdev->man[TTM_PL_TT];
	spin_lock(&bdev->lru_lock);
	clean = drm_mm_clean(&man->manager);
	spin_unlock(&bdev->lru_lock);
	if (unlikely(!clean))
		DRM_INFO("Warning: GATT was not clean after VT switch.\n");

	ttm_bo_swapout_all(&dev_priv->bdev);

	return 0;
out_unlock:
	(void) ttm_write_unlock(&dev_priv->ttm_lock,
				psb_fpriv(file_priv)->tfile);
	return ret;
}

static int psb_vt_enter_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	return ttm_write_unlock(&dev_priv->ttm_lock,
				psb_fpriv(file_priv)->tfile);
}

/* always available as we are SIGIO'd */
static unsigned int psb_poll(struct file *filp,
			     struct poll_table_struct *wait)
{
	return POLLIN | POLLRDNORM;
}

int psb_driver_open(struct drm_device *dev, struct drm_file *priv)
{
	/*psb_check_power_state(dev, PSB_DEVICE_SGX);*/
	return 0;
}

static long psb_unlocked_ioctl(struct file *filp, unsigned int cmd,
			       unsigned long arg)
{
	struct drm_file *file_priv = filp->private_data;
	struct drm_device *dev = file_priv->minor->dev;
	unsigned int nr = DRM_IOCTL_NR(cmd);
	long ret;

	/*
	 * The driver private ioctls and TTM ioctls should be
	 * thread-safe.
	 */

	if ((nr >= DRM_COMMAND_BASE) && (nr < DRM_COMMAND_END)
	    && (nr < DRM_COMMAND_BASE + dev->driver->num_ioctls)) {
		struct drm_ioctl_desc *ioctl = &psb_ioctls[nr - DRM_COMMAND_BASE];

		if (unlikely(ioctl->cmd != cmd)) {
			DRM_ERROR("Invalid drm command %d\n",
				  nr - DRM_COMMAND_BASE);
			return -EINVAL;
		}

		return drm_unlocked_ioctl(filp, cmd, arg);
	}
	/*
	 * Not all old drm ioctls are thread-safe.
	 */

	lock_kernel();
	ret = drm_unlocked_ioctl(filp, cmd, arg);
	unlock_kernel();
	return ret;
}

static int psb_ospm_read(char *buf, char **start, off_t offset, int request,
			 int *eof, void *data)
{
	struct drm_minor *minor = (struct drm_minor *) data;
	struct drm_device *dev = minor->dev;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	int len = 0;
	unsigned long d0i0 = 0;
	unsigned long d0i3 = 0;
	unsigned long d3 = 0;
	*start = &buf[offset];
	*eof = 0;
	DRM_PROC_PRINT("D0i3:%s   ", drm_psb_ospm ? "enabled" : "disabled");
	switch (dev_priv->graphics_state) {
	case PSB_PWR_STATE_D0i0:
		DRM_PROC_PRINT("GFX:%s\n", "D0i0");
		break;
	case PSB_PWR_STATE_D0i3:
		DRM_PROC_PRINT("GFX:%s\n", "D0i3");
		break;
	case PSB_PWR_STATE_D3:
		DRM_PROC_PRINT("GFX:%s\n", "D3");
		break;
	default:
		DRM_PROC_PRINT("GFX:%s\n", "unkown");
	}
#ifdef OSPM_STAT
	d0i0 = dev_priv->gfx_d0i0_time * 1000 / HZ;
	d0i3 = dev_priv->gfx_d0i3_time * 1000 / HZ;
	d3 = dev_priv->gfx_d3_time * 1000 / HZ;
	switch (dev_priv->graphics_state) {
	case PSB_PWR_STATE_D0i0:
		d0i0 += (jiffies - dev_priv->gfx_last_mode_change) * 1000 / HZ;
		break;
	case PSB_PWR_STATE_D0i3:
		d0i3 += (jiffies - dev_priv->gfx_last_mode_change) * 1000 / HZ;
		break;
	case PSB_PWR_STATE_D3:
		d3 += (jiffies - dev_priv->gfx_last_mode_change) * 1000 / HZ;
		break;
	}
	DRM_PROC_PRINT("GFX(cnt/ms):\n");
	DRM_PROC_PRINT("D0i0:%lu/%lu, D0i3:%lu/%lu, D3:%lu/%lu \n",
		dev_priv->gfx_d0i0_cnt, d0i0, dev_priv->gfx_d0i3_cnt, d0i3,
		dev_priv->gfx_d3_cnt, d3);
#endif
	if (len > request + offset)
		return request;
	*eof = 1;
	return len - offset;
}

static int psb_proc_init(struct drm_minor *minor)
{
	struct proc_dir_entry *ent;
	if (!minor->proc_root)
		return 0;
	ent = create_proc_read_entry(OSPM_PROC_ENTRY, 0, minor->proc_root,
			psb_ospm_read, minor);
	if (ent)
		return 0;
	else
		return -1;
}

static void psb_proc_cleanup(struct drm_minor *minor)
{
	if (!minor->proc_root)
		return;
	remove_proc_entry(OSPM_PROC_ENTRY, minor->proc_root);
	return;
}

static struct drm_driver driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_IRQ_SHARED,
	.load = psb_driver_load,
	.unload = psb_driver_unload,
	.get_reg_ofs = drm_core_get_reg_ofs,
	.ioctls = psb_ioctls,
	.device_is_agp = psb_driver_device_is_agp,
	.irq_preinstall = psb_irq_preinstall,
	.irq_postinstall = psb_irq_postinstall,
	.irq_uninstall = psb_irq_uninstall,
	.irq_handler = psb_irq_handler,
	.firstopen = NULL,
	.lastclose = psb_lastclose,
	.open = psb_driver_open,
	.proc_init = psb_proc_init,
	.proc_cleanup = psb_proc_cleanup,
	.fops = {
		 .owner = THIS_MODULE,
		 .open = psb_open,
		 .release = psb_release,
		 .unlocked_ioctl = psb_unlocked_ioctl,
		 .mmap = psb_mmap,
		 .poll = psb_poll,
		 .fasync = drm_fasync,
		 },
	.pci_driver = {
		       .name = DRIVER_NAME,
		       .id_table = pciidlist,
		       .resume = psb_resume,
		       .suspend = psb_suspend,
		       },
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = PSB_DRM_DRIVER_DATE,
	.major = PSB_DRM_DRIVER_MAJOR,
	.minor = PSB_DRM_DRIVER_MINOR,
	.patchlevel = PSB_DRM_DRIVER_PATCHLEVEL
};

static int __init psb_init(void)
{
	driver.num_ioctls = psb_max_ioctl;

	return drm_init(&driver);
}

static void __exit psb_exit(void)
{
	drm_exit(&driver);
}

module_init(psb_init);
module_exit(psb_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
