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
#include "drm.h"
#include "psb_drm.h"
#include "psb_drv.h"
#include "psb_reg.h"
#include "i915_reg.h"
#include "psb_msvdx.h"
#include "drm_pciids.h"
#include "psb_scene.h"
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/fb.h>

int drm_psb_debug = 0;
EXPORT_SYMBOL(drm_psb_debug);
static int drm_psb_trap_pagefaults = 0;
static int drm_psb_clock_gating = 0;
static int drm_psb_ta_mem_size = 32 * 1024;
int drm_psb_disable_vsync = 0;
int drm_psb_detear = 0;
int drm_psb_no_fb = 0;
int drm_psb_force_pipeb = 0;
char* psb_init_mode;
int psb_init_xres;
int psb_init_yres;
/*
 *
 */
#define SII_1392_WA
#ifdef SII_1392_WA
extern int SII_1392;
#endif

MODULE_PARM_DESC(debug, "Enable debug output");
MODULE_PARM_DESC(clock_gating, "clock gating");
MODULE_PARM_DESC(no_fb, "Disable FBdev");
MODULE_PARM_DESC(trap_pagefaults, "Error and reset on MMU pagefaults");
MODULE_PARM_DESC(disable_vsync, "Disable vsync interrupts");
MODULE_PARM_DESC(detear, "eliminate video playback tearing");
MODULE_PARM_DESC(force_pipeb, "Forces PIPEB to become primary fb");
MODULE_PARM_DESC(ta_mem_size, "TA memory size in kiB");
MODULE_PARM_DESC(mode, "initial mode name");
MODULE_PARM_DESC(xres, "initial mode width");
MODULE_PARM_DESC(yres, "initial mode height");

module_param_named(debug, drm_psb_debug, int, 0600);
module_param_named(clock_gating, drm_psb_clock_gating, int, 0600);
module_param_named(no_fb, drm_psb_no_fb, int, 0600);
module_param_named(trap_pagefaults, drm_psb_trap_pagefaults, int, 0600);
module_param_named(disable_vsync, drm_psb_disable_vsync, int, 0600);
module_param_named(detear, drm_psb_detear, int, 0600);
module_param_named(force_pipeb, drm_psb_force_pipeb, int, 0600);
module_param_named(ta_mem_size, drm_psb_ta_mem_size, int, 0600);
module_param_named(mode, psb_init_mode, charp, 0600);
module_param_named(xres, psb_init_xres, int, 0600);
module_param_named(yres, psb_init_yres, int, 0600);

static struct pci_device_id pciidlist[] = {
	psb_PCI_IDS
};

#define DRM_PSB_CMDBUF_IOCTL    DRM_IOW(DRM_PSB_CMDBUF, \
					struct drm_psb_cmdbuf_arg)
#define DRM_PSB_XHW_INIT_IOCTL  DRM_IOR(DRM_PSB_XHW_INIT, \
					struct drm_psb_xhw_init_arg)
#define DRM_PSB_XHW_IOCTL       DRM_IO(DRM_PSB_XHW)

#define DRM_PSB_SCENE_UNREF_IOCTL DRM_IOWR(DRM_PSB_SCENE_UNREF, \
					   struct drm_psb_scene)
#define DRM_PSB_HW_INFO_IOCTL DRM_IOR(DRM_PSB_HW_INFO, \
                                           struct drm_psb_hw_info)

#define DRM_PSB_KMS_OFF_IOCTL	DRM_IO(DRM_PSB_KMS_OFF)
#define DRM_PSB_KMS_ON_IOCTL	DRM_IO(DRM_PSB_KMS_ON)

static struct drm_ioctl_desc psb_ioctls[] = {
	DRM_IOCTL_DEF(DRM_PSB_CMDBUF_IOCTL, psb_cmdbuf_ioctl, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_PSB_XHW_INIT_IOCTL, psb_xhw_init_ioctl,
		      DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_PSB_XHW_IOCTL, psb_xhw_ioctl, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_PSB_SCENE_UNREF_IOCTL, drm_psb_scene_unref_ioctl,
		      DRM_AUTH),
	DRM_IOCTL_DEF(DRM_PSB_KMS_OFF_IOCTL, psbfb_kms_off_ioctl,
		      DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_PSB_KMS_ON_IOCTL, psbfb_kms_on_ioctl, DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_PSB_HW_INFO_IOCTL, psb_hw_info_ioctl, DRM_AUTH),
};
static int psb_max_ioctl = DRM_ARRAY_SIZE(psb_ioctls);

static int probe(struct pci_dev *pdev, const struct pci_device_id *ent);

#ifdef USE_PAT_WC
#warning Init pat
static int __cpuinit psb_cpu_callback(struct notifier_block *nfb,
			    unsigned long action,
			    void *hcpu)
{
	if (action == CPU_ONLINE)
		drm_init_pat();

	return 0;
}

static struct notifier_block __cpuinitdata psb_nb = {
	.notifier_call = psb_cpu_callback,
	.priority = 1
};
#endif

static int dri_library_name(struct drm_device *dev, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "psb\n");
}

static void psb_set_uopt(struct drm_psb_uopt *uopt)
{
	uopt->clock_gating = drm_psb_clock_gating;
}

static void psb_lastclose(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;

	if (!dev->dev_private)
		return;

	mutex_lock(&dev->struct_mutex);
	if (dev_priv->ta_mem)
		psb_ta_mem_unref_devlocked(&dev_priv->ta_mem);
	mutex_unlock(&dev->struct_mutex);
	mutex_lock(&dev_priv->cmdbuf_mutex);
	if (dev_priv->buffers) {
		vfree(dev_priv->buffers);
		dev_priv->buffers = NULL;
	}
	mutex_unlock(&dev_priv->cmdbuf_mutex);
}

static void psb_do_takedown(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;

	mutex_lock(&dev->struct_mutex);
	if (dev->bm.initialized) {
		if (dev_priv->have_mem_rastgeom) {
			drm_bo_clean_mm(dev, DRM_PSB_MEM_RASTGEOM);
			dev_priv->have_mem_rastgeom = 0;
		}
		if (dev_priv->have_mem_mmu) {
			drm_bo_clean_mm(dev, DRM_PSB_MEM_MMU);
			dev_priv->have_mem_mmu = 0;
		}
		if (dev_priv->have_mem_aper) {
			drm_bo_clean_mm(dev, DRM_PSB_MEM_APER);
			dev_priv->have_mem_aper = 0;
		}
		if (dev_priv->have_tt) {
			drm_bo_clean_mm(dev, DRM_BO_MEM_TT);
			dev_priv->have_tt = 0;
		}
		if (dev_priv->have_vram) {
			drm_bo_clean_mm(dev, DRM_BO_MEM_VRAM);
			dev_priv->have_vram = 0;
		}
	}
	mutex_unlock(&dev->struct_mutex);

	if (dev_priv->has_msvdx)
		psb_msvdx_uninit(dev);

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
	(void)PSB_RSGX32(PSB_CR_CLKGATECTL);
}

static int psb_do_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct psb_gtt *pg = dev_priv->pg;

	uint32_t stolen_gtt;
	uint32_t tt_start;
	uint32_t tt_pages;

	int ret = -ENOMEM;

	DRM_ERROR("Debug is 0x%08x\n", drm_psb_debug);

	dev_priv->ta_mem_pages =
	    PSB_ALIGN_TO(drm_psb_ta_mem_size * 1024, PAGE_SIZE) >> PAGE_SHIFT;
	dev_priv->comm_page = alloc_page(GFP_KERNEL);
	if (!dev_priv->comm_page)
		goto out_err;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
	change_page_attr(dev_priv->comm_page, 1, PAGE_KERNEL_NOCACHE);
#else
	map_page_into_agp(dev_priv->comm_page);
#endif

	dev_priv->comm = kmap(dev_priv->comm_page);
	memset((void *)dev_priv->comm, 0, PAGE_SIZE);

	dev_priv->has_msvdx = 1;
	if (psb_msvdx_init(dev))
		dev_priv->has_msvdx = 0;

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
	stolen_gtt = (stolen_gtt < pg->gtt_pages) ? stolen_gtt : pg->gtt_pages;

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

	dev_priv->irqmask_lock = SPIN_LOCK_UNLOCKED;
	dev_priv->fence0_irq_on = 0;

	tt_pages = (pg->gatt_pages < PSB_TT_PRIV0_PLIMIT) ?
	    pg->gatt_pages : PSB_TT_PRIV0_PLIMIT;
	tt_start = dev_priv->gatt_free_offset - pg->gatt_start;
	tt_pages -= tt_start >> PAGE_SHIFT;

	mutex_lock(&dev->struct_mutex);

	if (!drm_bo_init_mm(dev, DRM_BO_MEM_VRAM, 0,
			    pg->stolen_size >> PAGE_SHIFT)) {
		dev_priv->have_vram = 1;
	}

	if (!drm_bo_init_mm(dev, DRM_BO_MEM_TT, tt_start >> PAGE_SHIFT,
			    tt_pages)) {
		dev_priv->have_tt = 1;
	}

	if (!drm_bo_init_mm(dev, DRM_PSB_MEM_MMU, 0x00000000,
			    (pg->gatt_start -
			     PSB_MEM_MMU_START) >> PAGE_SHIFT)) {
		dev_priv->have_mem_mmu = 1;
	}

	if (!drm_bo_init_mm(dev, DRM_PSB_MEM_RASTGEOM, 0x00000000,
			    (PSB_MEM_MMU_START -
			     PSB_MEM_RASTGEOM_START) >> PAGE_SHIFT)) {
		dev_priv->have_mem_rastgeom = 1;
	}
#if 0
	if (pg->gatt_pages > PSB_TT_PRIV0_PLIMIT) {
		if (!drm_bo_init_mm(dev, DRM_PSB_MEM_APER, PSB_TT_PRIV0_PLIMIT,
				    pg->gatt_pages - PSB_TT_PRIV0_PLIMIT)) {
			dev_priv->have_mem_aper = 1;
		}
	}
#endif

	mutex_unlock(&dev->struct_mutex);

	return 0;
      out_err:
	psb_do_takedown(dev);
	return ret;
}

static int psb_driver_unload(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;

#ifdef USE_PAT_WC
#warning Init pat
//	if (num_present_cpus() > 1)
	unregister_cpu_notifier(&psb_nb);
#endif

	intel_modeset_cleanup(dev);

	if (dev_priv) {
		psb_watchdog_takedown(dev_priv);
		psb_do_takedown(dev);
		psb_xhw_takedown(dev_priv);
		psb_scheduler_takedown(&dev_priv->scheduler);

		mutex_lock(&dev->struct_mutex);
		if (dev_priv->have_mem_pds) {
			drm_bo_clean_mm(dev, DRM_PSB_MEM_PDS);
			dev_priv->have_mem_pds = 0;
		}
		if (dev_priv->have_mem_kernel) {
			drm_bo_clean_mm(dev, DRM_PSB_MEM_KERNEL);
			dev_priv->have_mem_kernel = 0;
		}
		mutex_unlock(&dev->struct_mutex);

		(void)drm_bo_driver_finish(dev);

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
						    pg->
						    stolen_size >> PAGE_SHIFT);
			up_read(&pg->sem);
			psb_mmu_driver_takedown(dev_priv->mmu);
			dev_priv->mmu = NULL;
		}
		psb_gtt_takedown(dev_priv->pg, 1);
		if (dev_priv->scratch_page) {
			__free_page(dev_priv->scratch_page);
			dev_priv->scratch_page = NULL;
		}
		psb_takedown_use_base(dev_priv);
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

		drm_free(dev_priv, sizeof(*dev_priv), DRM_MEM_DRIVER);
		dev->dev_private = NULL;
	}
	return 0;
}

extern int drm_crtc_probe_output_modes(struct drm_device *dev, int, int);
extern int drm_pick_crtcs(struct drm_device *dev);
extern char drm_init_mode[32];
extern int drm_init_xres;
extern int drm_init_yres;

static int psb_initial_config(struct drm_device *dev, bool can_grow)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_output *output;
	struct drm_crtc *crtc;
	int ret = false;

	mutex_lock(&dev->mode_config.mutex);

	drm_crtc_probe_output_modes(dev, 2048, 2048);
	
	/* strncpy(drm_init_mode, psb_init_mode, strlen(psb_init_mode)); */
	drm_init_xres = psb_init_xres;
	drm_init_yres = psb_init_yres;
	printk(KERN_ERR "detear is %sabled\n", drm_psb_detear ? "en" : "dis" );

	drm_pick_crtcs(dev);

	if ((I915_READ(PIPEACONF) & PIPEACONF_ENABLE) && !drm_psb_force_pipeb)
		list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		if (!crtc->desired_mode)
			continue;

		dev->driver->fb_probe(dev, crtc);
	} else
		list_for_each_entry_reverse(crtc, &dev->mode_config.crtc_list,
					    head) {
		if (!crtc->desired_mode)
			continue;

		dev->driver->fb_probe(dev, crtc);
		}

	list_for_each_entry(output, &dev->mode_config.output_list, head) {

		if (!output->crtc || !output->crtc->desired_mode)
			continue;

		if (output->crtc->fb)
			drm_crtc_set_mode(output->crtc,
					  output->crtc->desired_mode, 0, 0);
	}

#ifdef SII_1392_WA
	if((SII_1392 != 1) || (drm_psb_no_fb==0))
		drm_disable_unused_functions(dev);
#else
	drm_disable_unused_functions(dev);
#endif

	mutex_unlock(&dev->mode_config.mutex);

	return ret;

}

static int psb_driver_load(struct drm_device *dev, unsigned long chipset)
{
	struct drm_psb_private *dev_priv;
	unsigned long resource_start;
	struct psb_gtt *pg;
	int ret = -ENOMEM;

	DRM_INFO("psb - %s\n", PSB_PACKAGE_VERSION);
	dev_priv = drm_calloc(1, sizeof(*dev_priv), DRM_MEM_DRIVER);
	if (dev_priv == NULL)
		return -ENOMEM;

	mutex_init(&dev_priv->temp_mem);
	mutex_init(&dev_priv->cmdbuf_mutex);
	mutex_init(&dev_priv->reset_mutex);
	psb_init_disallowed();

	atomic_set(&dev_priv->msvdx_mmu_invaldc, 0);

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

	dev->dev_private = (void *)dev_priv;
	dev_priv->chipset = chipset;
	psb_set_uopt(&dev_priv->uopt);

	psb_watchdog_init(dev_priv);
	psb_scheduler_init(dev, &dev_priv->scheduler);

	resource_start = pci_resource_start(dev->pdev, PSB_MMIO_RESOURCE);
	
	dev_priv->msvdx_reg =
	    ioremap(resource_start + PSB_MSVDX_OFFSET, PSB_MSVDX_SIZE);
	if (!dev_priv->msvdx_reg)
		goto out_err;

	dev_priv->vdc_reg =
	    ioremap(resource_start + PSB_VDC_OFFSET, PSB_VDC_SIZE);
	if (!dev_priv->vdc_reg)
		goto out_err;

	dev_priv->sgx_reg =
	    ioremap(resource_start + PSB_SGX_OFFSET, PSB_SGX_SIZE);
	if (!dev_priv->sgx_reg)
		goto out_err;

	psb_clockgating(dev_priv);
	if (psb_init_use_base(dev_priv, 3, 13))
		goto out_err;

	dev_priv->scratch_page = alloc_page(GFP_DMA32 | __GFP_ZERO);
	if (!dev_priv->scratch_page)
		goto out_err;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
	change_page_attr(dev_priv->scratch_page, 1, PAGE_KERNEL_NOCACHE);
#else
	map_page_into_agp(dev_priv->scratch_page);
#endif

	dev_priv->pg = psb_gtt_alloc(dev);
	if (!dev_priv->pg)
		goto out_err;

	ret = psb_gtt_init(dev_priv->pg, 0);
	if (ret)
		goto out_err;

	dev_priv->mmu = psb_mmu_driver_init(dev_priv->sgx_reg,
					    drm_psb_trap_pagefaults, 0,
					    &dev_priv->msvdx_mmu_invaldc);
	if (!dev_priv->mmu)
		goto out_err;

	pg = dev_priv->pg;

	/*
	 * Make sgx MMU aware of the stolen memory area we call VRAM.
	 */

	down_read(&pg->sem);
	ret =
	    psb_mmu_insert_pfn_sequence(psb_mmu_get_default_pd(dev_priv->mmu),
					pg->stolen_base >> PAGE_SHIFT,
					pg->gatt_start,
					pg->stolen_size >> PAGE_SHIFT, 0);
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

	ret = drm_bo_driver_init(dev);
	if (ret)
		goto out_err;

	ret = drm_bo_init_mm(dev, DRM_PSB_MEM_KERNEL, 0x00000000,
			     (PSB_MEM_PDS_START - PSB_MEM_KERNEL_START)
			     >> PAGE_SHIFT);
	if (ret)
		goto out_err;
	dev_priv->have_mem_kernel = 1;

	ret = drm_bo_init_mm(dev, DRM_PSB_MEM_PDS, 0x00000000,
			     (PSB_MEM_RASTGEOM_START - PSB_MEM_PDS_START)
			     >> PAGE_SHIFT);
	if (ret)
		goto out_err;
	dev_priv->have_mem_pds = 1;

	ret = psb_do_init(dev);
	if (ret)
		return ret;

	ret = psb_xhw_init(dev);
	if (ret)
		return ret;

	PSB_WSGX32(PSB_MEM_PDS_START, PSB_CR_PDS_EXEC_BASE);
	PSB_WSGX32(PSB_MEM_RASTGEOM_START, PSB_CR_BIF_3D_REQ_BASE);

	intel_modeset_init(dev);
	psb_initial_config(dev, false);

#ifdef USE_PAT_WC
#warning Init pat
//	if (num_present_cpus() > 1)
	register_cpu_notifier(&psb_nb);
#endif

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
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct drm_fence_manager *fm = &dev->fm;
	struct drm_fence_class_manager *fc = &fm->fence_class[PSB_ENGINE_VIDEO];
	struct drm_fence_object *fence;
	int ret = 0;
	int signaled = 0;
	int count = 0;
	unsigned long _end = jiffies + 3 * DRM_HZ;

	PSB_DEBUG_GENERAL("MSVDXACPI Entering psb_prepare_msvdx_suspend....\n");

	/*set the msvdx-reset flag here.. */
	dev_priv->msvdx_needs_reset = 1;

	/*Ensure that all pending IRQs are serviced, */
	list_for_each_entry(fence, &fc->ring, ring) {
		count++;
		do {
			DRM_WAIT_ON(ret, fc->fence_queue, 3 * DRM_HZ,
				    (signaled =
				     drm_fence_object_signaled(fence,
							       DRM_FENCE_TYPE_EXE)));
			if (signaled)
				break;
			if (time_after_eq(jiffies, _end))
				PSB_DEBUG_GENERAL
				    ("MSVDXACPI: fence 0x%x didn't get signaled for 3 secs; we will suspend anyways\n",
				     (unsigned int)fence);
		} while (ret == -EINTR);

	}
       
	/* Issue software reset */
        PSB_WMSVDX32 (msvdx_sw_reset_all, MSVDX_CONTROL);

        ret = psb_wait_for_register (dev_priv, MSVDX_CONTROL, 0,
                           MSVDX_CONTROL_CR_MSVDX_SOFT_RESET_MASK);

	PSB_DEBUG_GENERAL("MSVDXACPI: All MSVDX IRQs (%d) serviced...\n",
			  count);
	return 0;
}

static int psb_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
        struct drm_output *output;

	//if (drm_psb_no_fb == 0)
	//	psbfb_suspend(dev);
#ifdef WA_NO_FB_GARBAGE_DISPLAY
	//else {
	if (drm_psb_no_fb != 0) {
		if(num_registered_fb)
		{
			list_for_each_entry(output, &dev->mode_config.output_list, head) {
				if(output->crtc != NULL)
					intel_crtc_mode_save(output->crtc);
				//if(output->funcs->save)
				//	output->funcs->save(output);
			}
		}
	}
#endif

	dev_priv->saveCLOCKGATING = PSB_RSGX32(PSB_CR_CLKGATECTL);
	(void)psb_idle_3d(dev);
	(void)psb_idle_2d(dev);
	flush_scheduled_work();

	psb_takedown_use_base(dev_priv);

	if (dev_priv->has_msvdx)
		psb_prepare_msvdx_suspend(dev);

	pci_save_state(pdev);
	pci_disable_device(pdev);
	pci_set_power_state(pdev, PCI_D3hot);

	return 0;
}

static int psb_resume(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;
	struct psb_gtt *pg = dev_priv->pg;
        struct drm_output *output;
	int ret;

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

#ifdef USE_PAT_WC
#warning Init pat
	/* for single CPU's we do it here, then for more than one CPU we
	 * use the CPU notifier to reinit PAT on those CPU's. 
	 */
//	if (num_present_cpus() == 1)
	drm_init_pat();
#endif

	INIT_LIST_HEAD(&dev_priv->resume_buf.head);
	dev_priv->msvdx_needs_reset = 1;

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

	if (drm_psb_no_fb == 0) {
            list_for_each_entry(output, &dev->mode_config.output_list, head) {
                if(output->crtc != NULL)
                    drm_crtc_set_mode(output->crtc, &output->crtc->mode,
                              output->crtc->x, output->crtc->y);
            }
        }

	/*
	 * Persistant 3D base registers and USSE base registers..
	 */

	PSB_WSGX32(PSB_MEM_PDS_START, PSB_CR_PDS_EXEC_BASE);
	PSB_WSGX32(PSB_MEM_RASTGEOM_START, PSB_CR_BIF_3D_REQ_BASE);
	psb_init_use_base(dev_priv, 3, 13);

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

	//if (drm_psb_no_fb == 0)
	//	psbfb_resume(dev);
#ifdef WA_NO_FB_GARBAGE_DISPLAY
	//else {
	if (drm_psb_no_fb != 0) {
		if(num_registered_fb)
		{
			struct fb_info *fb_info=registered_fb[0];
			list_for_each_entry(output, &dev->mode_config.output_list, head) {
				if(output->crtc != NULL)
					intel_crtc_mode_restore(output->crtc);
			}
			if(fb_info)
			{
				fb_set_suspend(fb_info, 0);
				printk("set the fb_set_suspend resume end\n");
			}
		}
	} 
#endif

	return 0;
}

/* always available as we are SIGIO'd */
static unsigned int psb_poll(struct file *filp, struct poll_table_struct *wait)
{
	return (POLLIN | POLLRDNORM);
}

static int psb_release(struct inode *inode, struct file *filp)
{
	struct drm_file *file_priv = (struct drm_file *)filp->private_data;
	struct drm_device *dev = file_priv->head->dev;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)dev->dev_private;

	if (dev_priv && dev_priv->xhw_file) {
		psb_xhw_init_takedown(dev_priv, file_priv, 1);
	}
	return drm_release(inode, filp);
}

extern struct drm_fence_driver psb_fence_driver;

/*
 * Use this memory type priority if no eviction is needed.
 */
static uint32_t psb_mem_prios[] = { DRM_BO_MEM_VRAM,
	DRM_BO_MEM_TT,
	DRM_PSB_MEM_KERNEL,
	DRM_PSB_MEM_MMU,
	DRM_PSB_MEM_RASTGEOM,
	DRM_PSB_MEM_PDS,
	DRM_PSB_MEM_APER,
	DRM_BO_MEM_LOCAL
};

/*
 * Use this memory type priority if need to evict.
 */
static uint32_t psb_busy_prios[] = { DRM_BO_MEM_TT,
	DRM_BO_MEM_VRAM,
	DRM_PSB_MEM_KERNEL,
	DRM_PSB_MEM_MMU,
	DRM_PSB_MEM_RASTGEOM,
	DRM_PSB_MEM_PDS,
	DRM_PSB_MEM_APER,
	DRM_BO_MEM_LOCAL
};

static struct drm_bo_driver psb_bo_driver = {
	.mem_type_prio = psb_mem_prios,
	.mem_busy_prio = psb_busy_prios,
	.num_mem_type_prio = ARRAY_SIZE(psb_mem_prios),
	.num_mem_busy_prio = ARRAY_SIZE(psb_busy_prios),
	.create_ttm_backend_entry = drm_psb_tbe_init,
	.fence_type = psb_fence_types,
	.invalidate_caches = psb_invalidate_caches,
	.init_mem_type = psb_init_mem_type,
	.evict_mask = psb_evict_mask,
	.move = psb_move,
	.backend_size = psb_tbe_size,
	.command_stream_barrier = NULL,
};

static struct drm_driver driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_IRQ_SHARED |
	    DRIVER_IRQ_VBL | DRIVER_IRQ_VBL2,
	.load = psb_driver_load,
	.unload = psb_driver_unload,
	.dri_library_name = dri_library_name,
	.get_reg_ofs = drm_core_get_reg_ofs,
	.ioctls = psb_ioctls,
	.device_is_agp = psb_driver_device_is_agp,
	.vblank_wait = psb_vblank_wait2,
	.vblank_wait2 = psb_vblank_wait2,
	.irq_preinstall = psb_irq_preinstall,
	.irq_postinstall = psb_irq_postinstall,
	.irq_uninstall = psb_irq_uninstall,
	.irq_handler = psb_irq_handler,
	.fb_probe = psbfb_probe,
	.fb_remove = psbfb_remove,
	.firstopen = NULL,
	.lastclose = psb_lastclose,
	.fops = {
		 .owner = THIS_MODULE,
		 .open = drm_open,
		 .release = psb_release,
		 .ioctl = drm_ioctl,
		 .mmap = drm_mmap,
		 .poll = psb_poll,
		 .fasync = drm_fasync,
		 },
	.pci_driver = {
		       .name = DRIVER_NAME,
		       .id_table = pciidlist,
		       .probe = probe,
		       .remove = __devexit_p(drm_cleanup_pci),
		       .resume = psb_resume,
		       .suspend = psb_suspend,
		       },
	.fence_driver = &psb_fence_driver,
	.bo_driver = &psb_bo_driver,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = PSB_DRM_DRIVER_DATE,
	.major = PSB_DRM_DRIVER_MAJOR,
	.minor = PSB_DRM_DRIVER_MINOR,
	.patchlevel = PSB_DRM_DRIVER_PATCHLEVEL
};

static int probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	return drm_get_dev(pdev, ent, &driver);
}

static int __init psb_init(void)
{
	driver.num_ioctls = psb_max_ioctl;

	return drm_init(&driver, pciidlist);
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
