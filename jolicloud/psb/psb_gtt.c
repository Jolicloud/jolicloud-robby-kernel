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

static inline uint32_t psb_gtt_mask_pte(uint32_t pfn, int type)
{
	uint32_t mask = PSB_PTE_VALID;

	if (type & PSB_MMU_CACHED_MEMORY)
		mask |= PSB_PTE_CACHED;
	if (type & PSB_MMU_RO_MEMORY)
		mask |= PSB_PTE_RO;
	if (type & PSB_MMU_WO_MEMORY)
		mask |= PSB_PTE_WO;

	return (pfn << PAGE_SHIFT) | mask;
}

struct psb_gtt *psb_gtt_alloc(struct drm_device *dev)
{
	struct psb_gtt *tmp = drm_calloc(1, sizeof(*tmp), DRM_MEM_DRIVER);

	if (!tmp)
		return NULL;

	init_rwsem(&tmp->sem);
	tmp->dev = dev;

	return tmp;
}

void psb_gtt_takedown(struct psb_gtt *pg, int free)
{
	struct drm_psb_private *dev_priv = pg->dev->dev_private;

	if (!pg)
		return;

	if (pg->gtt_map) {
		iounmap(pg->gtt_map);
		pg->gtt_map = NULL;
	}
	if (pg->initialized) {
		pci_write_config_word(pg->dev->pdev, PSB_GMCH_CTRL,
				      pg->gmch_ctrl);
		PSB_WVDC32(pg->pge_ctl, PSB_PGETBL_CTL);
		(void)PSB_RVDC32(PSB_PGETBL_CTL);
	}
	if (free)
		drm_free(pg, sizeof(*pg), DRM_MEM_DRIVER);
}

int psb_gtt_init(struct psb_gtt *pg, int resume)
{
	struct drm_device *dev = pg->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	unsigned gtt_pages;
	unsigned long stolen_size;
	unsigned i, num_pages;
	unsigned pfn_base;

	int ret = 0;
	uint32_t pte;

	pci_read_config_word(dev->pdev, PSB_GMCH_CTRL, &pg->gmch_ctrl);
	pci_write_config_word(dev->pdev, PSB_GMCH_CTRL,
			      pg->gmch_ctrl | _PSB_GMCH_ENABLED);

	pg->pge_ctl = PSB_RVDC32(PSB_PGETBL_CTL);
	PSB_WVDC32(pg->pge_ctl | _PSB_PGETBL_ENABLED, PSB_PGETBL_CTL);
	(void)PSB_RVDC32(PSB_PGETBL_CTL);

	pg->initialized = 1;

	pg->gtt_phys_start = pg->pge_ctl & PAGE_MASK;
	pg->gatt_start = pci_resource_start(dev->pdev, PSB_GATT_RESOURCE);
	pg->gtt_start = pci_resource_start(dev->pdev, PSB_GTT_RESOURCE);
	gtt_pages = pci_resource_len(dev->pdev, PSB_GTT_RESOURCE) >> PAGE_SHIFT;
	pg->gatt_pages = pci_resource_len(dev->pdev, PSB_GATT_RESOURCE)
	    >> PAGE_SHIFT;
	pci_read_config_dword(dev->pdev, PSB_BSM, &pg->stolen_base);
	stolen_size = pg->gtt_phys_start - pg->stolen_base - PAGE_SIZE;

	PSB_DEBUG_INIT("GTT phys start: 0x%08x.\n", pg->gtt_phys_start);
	PSB_DEBUG_INIT("GTT start: 0x%08x.\n", pg->gtt_start);
	PSB_DEBUG_INIT("GATT start: 0x%08x.\n", pg->gatt_start);
	PSB_DEBUG_INIT("GTT pages: %u\n", gtt_pages);
	PSB_DEBUG_INIT("Stolen size: %lu kiB\n", stolen_size / 1024);

	if (resume && (gtt_pages != pg->gtt_pages) &&
	    (stolen_size != pg->stolen_size)) {
		DRM_ERROR("GTT resume error.\n");
		ret = -EINVAL;
		goto out_err;
	}

	pg->gtt_pages = gtt_pages;
	pg->stolen_size = stolen_size;
	if(!resume)
		pg->gtt_map =
			ioremap_nocache(pg->gtt_phys_start, gtt_pages << PAGE_SHIFT);
	if (!pg->gtt_map) {
		DRM_ERROR("Failure to map gtt.\n");
		ret = -ENOMEM;
		goto out_err;
	}

	/*
	 * insert stolen pages.
	 */

	pfn_base = pg->stolen_base >> PAGE_SHIFT;
	num_pages = stolen_size >> PAGE_SHIFT;
	PSB_DEBUG_INIT("Set up %d stolen pages starting at 0x%08x\n",
		       num_pages, pfn_base);
	for (i = 0; i < num_pages; ++i) {
		pte = psb_gtt_mask_pte(pfn_base + i, 0);
		iowrite32(pte, pg->gtt_map + i);
	}

	/*
	 * Init rest of gtt.
	 */

	pfn_base = page_to_pfn(dev_priv->scratch_page);
	pte = psb_gtt_mask_pte(pfn_base, 0);
	PSB_DEBUG_INIT("Initializing the rest of a total "
		       "of %d gtt pages.\n", pg->gatt_pages);

	for (; i < pg->gatt_pages; ++i)
		iowrite32(pte, pg->gtt_map + i);
	(void)ioread32(pg->gtt_map + i - 1);

	return 0;

      out_err:
	psb_gtt_takedown(pg, 0);
	return ret;
}

int psb_gtt_insert_pages(struct psb_gtt *pg, struct page **pages,
			 unsigned offset_pages, unsigned num_pages,
			 unsigned desired_tile_stride, unsigned hw_tile_stride,
			 int type)
{
	unsigned rows = 1;
	unsigned add;
	unsigned row_add;
	unsigned i;
	unsigned j;
	uint32_t *cur_page = NULL;
	uint32_t pte;

	if (hw_tile_stride)
		rows = num_pages / desired_tile_stride;
	else
		desired_tile_stride = num_pages;

	add = desired_tile_stride;
	row_add = hw_tile_stride;

	down_read(&pg->sem);
	for (i = 0; i < rows; ++i) {
		cur_page = pg->gtt_map + offset_pages;
		for (j = 0; j < desired_tile_stride; ++j) {
			pte = psb_gtt_mask_pte(page_to_pfn(*pages++), type);
			iowrite32(pte, cur_page++);
		}
		offset_pages += add;
	}
	(void)ioread32(cur_page - 1);
	up_read(&pg->sem);

	return 0;
}

int psb_gtt_remove_pages(struct psb_gtt *pg, unsigned offset_pages,
			 unsigned num_pages, unsigned desired_tile_stride,
			 unsigned hw_tile_stride)
{
	struct drm_psb_private *dev_priv = pg->dev->dev_private;
	unsigned rows = 1;
	unsigned add;
	unsigned row_add;
	unsigned i;
	unsigned j;
	uint32_t *cur_page = NULL;
	unsigned pfn_base = page_to_pfn(dev_priv->scratch_page);
	uint32_t pte = psb_gtt_mask_pte(pfn_base, 0);

	if (hw_tile_stride)
		rows = num_pages / desired_tile_stride;
	else
		desired_tile_stride = num_pages;

	add = desired_tile_stride;
	row_add = hw_tile_stride;

	down_read(&pg->sem);
	for (i = 0; i < rows; ++i) {
		cur_page = pg->gtt_map + offset_pages;
		for (j = 0; j < desired_tile_stride; ++j) {
			iowrite32(pte, cur_page++);
		}
		offset_pages += add;
	}
	(void)ioread32(cur_page - 1);
	up_read(&pg->sem);

	return 0;
}
