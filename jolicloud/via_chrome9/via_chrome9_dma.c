/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * (including the next paragraph) shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL VIA, S3 GRAPHICS, AND/OR
 * ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "drmP.h"
#include "drm.h"
#include "via_chrome9_drm.h"
#include "via_chrome9_drv.h"
#include "via_chrome9_3d_reg.h"
#include "via_chrome9_dma.h"
#include "via_chrome9_mm.h"

#define NULLCOMMANDNUMBER 256
#define AGP_REQ_FIFO_DEPTH  34 /*hardware AGP request FIFO depth 34x128(bits)*/
unsigned int NULL_COMMAND_INV[4] =
	{ 0xCC000000, 0xCD000000, 0xCE000000, 0xCF000000 };

#if defined(CONFIG_X86)
static void
via_chrome9_clflush_page(struct page *page)
{
       uint8_t *page_virtual;
       unsigned int i;

	if (unlikely(page == NULL))
		return;

	page_virtual = kmap_atomic(page, KM_USER0);
	for (i = 0; i < PAGE_SIZE; i += boot_cpu_data.x86_clflush_size)
		clflush(page_virtual + i);
	kunmap_atomic(page_virtual, KM_USER0);
}
#endif

void
via_chrome9_clflush_pages(struct page *pages[], unsigned long num_pages)
{
#if defined(CONFIG_X86)
	if (cpu_has_clflush) {
		unsigned long i;
		mb();
		for (i = 0; i < num_pages; ++i)
			via_chrome9_clflush_page(*pages++);
		mb();
		return;
	}

	wbinvd();
#endif
}

void
via_chrome9ke_assert(int a)
{
}

unsigned int
protect_size_value(unsigned int size)
{
	unsigned int i;
	for (i = 0; i < 8; i++)
		if ((size > (1 << (i + 12)))
		    && (size <= (1 << (i + 13))))
			return i + 1;
	return 0;
}

void via_chrome9_dma_init_inv(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;

	if (dev_priv->chip_sub_index == CHIP_H6S2) {
		unsigned int *pgarttable;
		unsigned int i, entries, gartoffset;
		unsigned char sr6a, sr6b, sr6c, sr6f, sr7b;
		unsigned int *addrlinear;
		unsigned int size, alignedoffset;

		entries = dev_priv->pagetable_map.pagetable_size /
			sizeof(unsigned int);
		pgarttable = dev_priv->pagetable_map.pagetable_handle;

		gartoffset = dev_priv->pagetable_map.pagetable_offset;

		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
		sr6c = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
		sr6c &= (~0x80);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);

		sr6a = (unsigned char)((gartoffset & 0xff000) >> 12);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6a);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6a);

		sr6b = (unsigned char)((gartoffset & 0xff00000) >> 20);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6b);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6b);

		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
		sr6c = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
		sr6c |= ((unsigned char)((gartoffset >> 28) & 0x01));
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);

		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x7b);
		sr7b = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
		sr7b &= (~0x0f);
		sr7b |= protect_size_value(dev_priv->
			pagetable_map.pagetable_size);
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr7b);

		for (i = 0; i < entries; i++)
			writel(0x80000000, pgarttable+i);

		/*flush*/
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6f);
		do {
			sr6f = getmmioregisteru8(dev_priv->mmio->handle,
				0x83c5);
		} while (sr6f & 0x80);

		sr6f |= 0x80;
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6f);

		setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
		sr6c = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
		sr6c |= 0x80;
		setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);

		if (dev_priv->drm_agp_type != DRM_AGP_DISABLED) {
			size = lpcmdmamanager->dmasize * sizeof(unsigned int) +
			dev_priv->agp_size;
			alignedoffset = 0;
			entries = (size + PAGE_SIZE - 1) / PAGE_SIZE;
			addrlinear =
				(unsigned int *)dev_priv->pcie_vmalloc_nocache;

			setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
			sr6c =
			getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
			sr6c &= (~0x80);
			setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);

			setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6f);
			do {
				sr6f = getmmioregisteru8(dev_priv->mmio->handle,
					0x83c5);
			} while (sr6f & 0x80);

			for (i = 0; i < dev->sg->pages; i++)
				writel(page_to_pfn(dev->sg->pagelist[i]) &
				0x3fffffff, pgarttable + i + alignedoffset);

			sr6f |= 0x80;
			setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6f);

			setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
			sr6c =
			getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
			sr6c |= 0x80;
			setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);
		}

	}

	if (dev_priv->drm_agp_type == DRM_AGP_DOUBLE_BUFFER)
		set_agp_double_cmd_inv(dev);
	else if (dev_priv->drm_agp_type == DRM_AGP_RING_BUFFER)
		set_agp_ring_cmd_inv(dev);

	return ;
}

static unsigned int
init_pcie_gart(struct drm_via_chrome9_private *dev_priv)
{
	unsigned int *pgarttable;
	unsigned int i, entries, gartoffset;
	unsigned char sr6a, sr6b, sr6c, sr6f, sr7b;

	if (!dev_priv->pagetable_map.pagetable_size)
		return 0;

	entries = dev_priv->pagetable_map.pagetable_size / sizeof(unsigned int);

	pgarttable =
		ioremap_nocache(dev_priv->fb_base_address +
				dev_priv->pagetable_map.pagetable_offset,
				dev_priv->pagetable_map.pagetable_size);
	if (pgarttable)
		dev_priv->pagetable_map.pagetable_handle = pgarttable;
	else
		return 0;

	/*set gart table base */
	gartoffset = dev_priv->pagetable_map.pagetable_offset;

	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
	sr6c = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	sr6c &= (~0x80);
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);

	sr6a = (unsigned char) ((gartoffset & 0xff000) >> 12);
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6a);
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6a);

	sr6b = (unsigned char) ((gartoffset & 0xff00000) >> 20);
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6b);
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6b);

	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
	sr6c = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	sr6c |= ((unsigned char) ((gartoffset >> 28) & 0x01));
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);

	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x7b);
	sr7b = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	sr7b &= (~0x0f);
	sr7b |= protect_size_value(dev_priv->pagetable_map.pagetable_size);
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr7b);

	for (i = 0; i < entries; i++)
		writel(0x80000000, pgarttable + i);
	/*flush */
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6f);
	do {
		sr6f = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	}
	while (sr6f & 0x80)
		;

	sr6f |= 0x80;
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6f);

	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
	sr6c = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	sr6c |= 0x80;
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);

	return 1;
}

void via_chrome9_drm_sg_cleanup(struct drm_sg_mem *entry)
{
	struct page *page;
	int i;

	for (i = 0; i < entry->pages; i++) {
		page = entry->pagelist[i];
		if (page)
			ClearPageReserved(page);
	}

	vfree(entry->virtual);

	drm_free_large(entry->busaddr);
	drm_free_large(entry->pagelist);
	drm_free_large(entry);
}

static int
via_chrome9_drm_sg_alloc(struct drm_device *dev,
	struct drm_scatter_gather *request)
{
	struct drm_sg_mem *entry;
	unsigned long pages, i, j;

	DRM_DEBUG("\n");

	if (!drm_core_check_feature(dev, DRIVER_SG))
		return -EINVAL;

	if (dev->sg)
		return -EINVAL;

	entry = drm_calloc_large(sizeof(*entry), DRM_MEM_SGLISTS);
	if (!entry)
		return -ENOMEM;

	memset(entry, 0, sizeof(*entry));
	pages = (request->size + PAGE_SIZE - 1) / PAGE_SIZE;
	DRM_DEBUG("size=%ld pages=%ld\n", request->size, pages);

	entry->pages = pages;
	entry->pagelist = drm_calloc_large(pages * sizeof(*entry->pagelist),
				    DRM_MEM_PAGES);
	if (!entry->pagelist) {
		drm_free_large(entry);
		return -ENOMEM;
	}

	memset(entry->pagelist, 0, pages * sizeof(*entry->pagelist));

	entry->busaddr = drm_calloc_large(pages * sizeof(*entry->busaddr),
				   DRM_MEM_PAGES);
	if (!entry->busaddr) {
		drm_free_large(entry->pagelist);
		drm_free_large(entry);
		return -ENOMEM;
	}
	memset((void *)entry->busaddr, 0, pages * sizeof(*entry->busaddr));

	entry->virtual = __vmalloc(pages << PAGE_SHIFT,
		GFP_KERNEL | __GFP_HIGHMEM, PAGE_KERNEL_NOCACHE);
	if (!entry->virtual) {
		drm_free_large(entry->busaddr);
		drm_free_large(entry->pagelist);
		drm_free_large(entry);
		return -ENOMEM;
	}

	/* This also forces the mapping of COW pages, so our page list
	 * will be valid.  Please don't remove it...
	 */
	memset(entry->virtual, 0, pages << PAGE_SHIFT);

	entry->handle = (unsigned long)entry->virtual;

	DRM_DEBUG("handle  = %08lx\n", entry->handle);
	DRM_DEBUG("virtual = %p\n", entry->virtual);

	for (i = (unsigned long)entry->virtual, j = 0; j < pages;
	     i += PAGE_SIZE, j++) {
		entry->pagelist[j] = vmalloc_to_page((void *)i);
		if (!entry->pagelist[j])
			goto failed;
		SetPageReserved(entry->pagelist[j]);
	}

	request->handle = entry->handle;
	dev->sg = entry;

	return 0;
failed:
	via_chrome9_drm_sg_cleanup(entry);
	return -ENOMEM;
}


static unsigned int *
alloc_bind_pcie_memory(struct drm_device *dev,
	unsigned int size, unsigned int offset)
{
	unsigned int *pgarttable;
	unsigned int entries, alignedoffset, i;
	unsigned char sr6c, sr6f;
	struct drm_scatter_gather request;
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;

	if (!size)
		return NULL;

	entries = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	alignedoffset = (offset + PAGE_SIZE - 1) / PAGE_SIZE;

	if ((entries + alignedoffset) >
	    (dev_priv->pagetable_map.pagetable_size / sizeof(unsigned int)))
		return NULL;

	/* Ask scatter-gather sub-system to allocate pcie memory
	 * then user mode can mmap this pcie memory through it
	 */
	request.size = size;
	request.handle = 0;
	if (via_chrome9_drm_sg_alloc(dev, &request)) {
		DRM_INFO("Scatter-gather allocate memory failed \n");
		return NULL;
	}

	pgarttable = dev_priv->pagetable_map.pagetable_handle;

	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
	sr6c = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	sr6c &= (~0x80);
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);

	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6f);
	do {
		sr6f = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	}
	while (sr6f & 0x80)
		;

	for (i = 0; i < dev->sg->pages; i++)
		writel(page_to_pfn(dev->sg->pagelist[i]) & 0x3fffffff,
			pgarttable + i + alignedoffset);

	sr6f |= 0x80;
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6f);

	setmmioregisteru8(dev_priv->mmio->handle, 0x83c4, 0x6c);
	sr6c = getmmioregisteru8(dev_priv->mmio->handle, 0x83c5);
	sr6c |= 0x80;
	setmmioregisteru8(dev_priv->mmio->handle, 0x83c5, sr6c);

	return (unsigned int *)request.handle;

}

void
set_agp_double_cmd_inv(struct drm_device *dev)
{
	/* we now don't use double buffer */
	return;
}

void
set_agp_ring_cmd_inv(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		(struct drm_via_chrome9_dma_manager *) dev_priv->dma_manager;
	unsigned int agpbuflinearbase = 0, agpbufphysicalbase = 0;
	unsigned long *pFree;
	unsigned int dwstart, dwend, dwpause, agpcurraddr, agpcurstat, curragp;
	unsigned int dwreg60, dwreg61, dwreg62, dwreg63,
		dwreg64, dwreg65, dwjump;

	lpcmdmamanager->pFree = lpcmdmamanager->pBeg;

	agpbuflinearbase = (unsigned int) lpcmdmamanager->addr_linear;
	agpbufphysicalbase =
		(dev_priv->chip_agp ==
		 CHIP_PCIE) ? 0 : (unsigned int) dev->agp->base +
		lpcmdmamanager->pphysical;
	/*add shadow offset */

	curragp =
		getmmioregister(dev_priv->mmio->handle, INV_RB_AGPCMD_CURRADDR);
	agpcurstat =
		getmmioregister(dev_priv->mmio->handle, INV_RB_AGPCMD_STATUS);

	if (agpcurstat & INV_AGPCMD_InPause) {
		agpcurraddr =
			getmmioregister(dev_priv->mmio->handle,
					INV_RB_AGPCMD_CURRADDR);
		pFree = (unsigned long *) (agpbuflinearbase + agpcurraddr -
					   agpbufphysicalbase);
		addcmdheader2_invi(pFree, INV_REG_CR_TRANS, INV_ParaType_Dummy);
		if (dev_priv->chip_sub_index == CHIP_H6S2)
			do {
				addcmddata_invi(pFree, 0xCCCCCCC0);
				addcmddata_invi(pFree, 0xDDD00000);
			}
			while ((u32)((unsigned int) pFree) & 0x7f)
				;
			/*for 8*128bit aligned */
		else
			do {
				addcmddata_invi(pFree, 0xCCCCCCC0);
				addcmddata_invi(pFree, 0xDDD00000);
			}
			while ((u32) ((unsigned int) pFree) & 0x1f)
				;
			/*for 256bit aligned */
		dwpause =
			(u32) (((unsigned int) pFree) - agpbuflinearbase +
			       agpbufphysicalbase - 16);

		dwreg64 = INV_SubA_HAGPBpL | INV_HWBasL(dwpause);
		dwreg65 =
			INV_SubA_HAGPBpID | INV_HWBasH(dwpause) |
			INV_HAGPBpID_STOP;

		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS,
				INV_ParaType_PreCR);
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
				dwreg64);
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
				dwreg65);

		while (getmmioregister(dev_priv->mmio->handle,
			INV_RB_ENG_STATUS) & INV_ENG_BUSY_ALL)
			;
	}
	dwstart =
		(u32) ((unsigned int) lpcmdmamanager->pBeg - agpbuflinearbase +
		       agpbufphysicalbase);
	dwend = (u32) ((unsigned int) lpcmdmamanager->pEnd - agpbuflinearbase +
		       agpbufphysicalbase);

	lpcmdmamanager->pFree = lpcmdmamanager->pBeg;
	if (dev_priv->chip_sub_index == CHIP_H6S2) {
		addcmdheader2_invi(lpcmdmamanager->pFree, INV_REG_CR_TRANS,
				   INV_ParaType_Dummy);
		do {
			addcmddata_invi(lpcmdmamanager->pFree, 0xCCCCCCC0);
			addcmddata_invi(lpcmdmamanager->pFree, 0xDDD00000);
		}
		while ((u32)((unsigned long *) lpcmdmamanager->pFree) & 0x7f)
			;
	}
	dwjump = 0xFFFFFFF0;
	dwpause =
		(u32)(((unsigned int) lpcmdmamanager->pFree) -
		16 - agpbuflinearbase + agpbufphysicalbase);

	DRM_DEBUG("dwstart = %08x, dwend = %08x, dwpause = %08x\n", dwstart,
		  dwend, dwpause);

	dwreg60 = INV_SubA_HAGPBstL | INV_HWBasL(dwstart);
	dwreg61 = INV_SubA_HAGPBstH | INV_HWBasH(dwstart);
	dwreg62 = INV_SubA_HAGPBendL | INV_HWBasL(dwend);
	dwreg63 = INV_SubA_HAGPBendH | INV_HWBasH(dwend);
	dwreg64 = INV_SubA_HAGPBpL | INV_HWBasL(dwpause);
	dwreg65 = INV_SubA_HAGPBpID | INV_HWBasH(dwpause) | INV_HAGPBpID_PAUSE;

	if (dev_priv->chip_sub_index == CHIP_H6S2)
		dwreg60 |= 0x01;

	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS,
			INV_ParaType_PreCR);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg60);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg61);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg62);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg63);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg64);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg65);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
			INV_SubA_HAGPBjumpL | INV_HWBasL(dwjump));
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
			INV_SubA_HAGPBjumpH | INV_HWBasH(dwjump));

	/* Trigger AGP cycle */
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
			INV_SubA_HFthRCM | INV_HFthRCM_10 | INV_HAGPBTrig);

	/*for debug */
	curragp =
		getmmioregister(dev_priv->mmio->handle, INV_RB_AGPCMD_CURRADDR);

	lpcmdmamanager->pinusebysw = lpcmdmamanager->pFree;
}

/* Do hw intialization and determine whether to use dma or mmio to
talk with hw */
int
via_chrome9_hw_init(struct drm_device *dev,
	struct drm_via_chrome9_init *init)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	unsigned retval = 0;
	unsigned int *pgarttable, *addrlinear = NULL;
	struct drm_clb_event_tag_info *event_tag_info;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager = NULL;

	if (init->chip_agp == CHIP_PCIE) {
		dev_priv->pagetable_map.pagetable_offset =
			init->garttable_offset;
		dev_priv->pagetable_map.pagetable_size = init->garttable_size;
		/* Prepare size of PCIE texture buffer */
		dev_priv->agp_size = init->agp_tex_size;
		/* special fake physical base address for PCIE texture buffer */
		dev_priv->agp_offset = init->DMA_size;
	} else {
		dev_priv->pagetable_map.pagetable_offset = 0;
		dev_priv->pagetable_map.pagetable_size = 0;
	}

	dev_priv->dma_manager =
		kmalloc(sizeof(struct drm_via_chrome9_dma_manager), GFP_KERNEL);
	if (!dev_priv->dma_manager) {
		DRM_ERROR("could not allocate system for dma_manager!\n");
		return -ENOMEM;
	}

	lpcmdmamanager =
		(struct drm_via_chrome9_dma_manager *) dev_priv->dma_manager;
	((struct drm_via_chrome9_dma_manager *)
		dev_priv->dma_manager)->dmasize = init->DMA_size;
	((struct drm_via_chrome9_dma_manager *)
		dev_priv->dma_manager)->pphysical = init->DMA_phys_address;

	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS, 0x00110000);
	if (dev_priv->chip_sub_index == CHIP_H6S2) {
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
				0x06000000);
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
				0x07100000);
	} else {
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
				0x02000000);
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
				0x03100000);
	}

	/* Specify fence command read back ID */
	/* Default the read back ID is CR */
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS,
			INV_ParaType_PreCR);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
			INV_SubA_HSetRBGID | INV_HSetRBGID_CR);

	DRM_DEBUG("begin to init\n");

	if (dev_priv->chip_sub_index == CHIP_H6S2) {
		dev_priv->pcie_vmalloc_nocache = 0;
		if (dev_priv->pagetable_map.pagetable_size)
			retval = init_pcie_gart(dev_priv);

		if (retval && dev_priv->drm_agp_type != DRM_AGP_DISABLED) {
			addrlinear =
				alloc_bind_pcie_memory(dev,
						       lpcmdmamanager->dmasize +
						       dev_priv->agp_size, 0);
			if (addrlinear) {
				dev_priv->pcie_vmalloc_nocache = (unsigned long)
					addrlinear;
			} else {
				dev_priv->bci_buffer =
					vmalloc(MAX_BCI_BUFFER_SIZE);
				dev_priv->drm_agp_type = DRM_AGP_DISABLED;
			}
		} else {
			dev_priv->bci_buffer = vmalloc(MAX_BCI_BUFFER_SIZE);
			dev_priv->drm_agp_type = DRM_AGP_DISABLED;
		}
	} else {
		if (dev_priv->drm_agp_type != DRM_AGP_DISABLED) {
			pgarttable = NULL;
			addrlinear = (unsigned int *)
				ioremap(dev->agp->base +
					lpcmdmamanager->pphysical,
					lpcmdmamanager->dmasize);
			dev_priv->bci_buffer = NULL;
		} else {
			dev_priv->bci_buffer = vmalloc(MAX_BCI_BUFFER_SIZE);
			/* BCI path always use this block of memory8 */
		}
	}

	/*till here we have known whether support dma or not */
	event_tag_info = vmalloc(sizeof(struct drm_clb_event_tag_info));
	memset(event_tag_info, 0, sizeof(struct drm_clb_event_tag_info));
	if (!event_tag_info)
		return DRM_ERROR(" event_tag_info allocate error!");

	/* aligned to 16k alignment */
	event_tag_info->linear_address =
		(int
		 *) (((unsigned int) dev_priv->shadow_map.shadow_handle +
		      0x3fff) & 0xffffc000);
	event_tag_info->event_tag_linear_address =
		event_tag_info->linear_address + 3;
	dev_priv->event_tag_info = (void *) event_tag_info;
	dev_priv->max_apertures = NUMBER_OF_APERTURES_CLB;

	/* Initialize DMA data structure */
	lpcmdmamanager->dmasize /= sizeof(unsigned int);
	lpcmdmamanager->pBeg = addrlinear;
	lpcmdmamanager->pFree = lpcmdmamanager->pBeg;
	lpcmdmamanager->pinusebysw = lpcmdmamanager->pBeg;
	lpcmdmamanager->pinusebyhw = lpcmdmamanager->pBeg;
	lpcmdmamanager->lastissuedeventtag = (unsigned int) (unsigned long *)
		lpcmdmamanager->pBeg;
	lpcmdmamanager->ppinusebyhw =
		(unsigned int **) ((char *) (dev_priv->mmio->handle) +
				   INV_RB_AGPCMD_CURRADDR);
	lpcmdmamanager->bdmaagp = dev_priv->chip_agp;
	lpcmdmamanager->addr_linear = (unsigned int *) addrlinear;

	if (dev_priv->drm_agp_type == DRM_AGP_DOUBLE_BUFFER) {
		lpcmdmamanager->maxkickoffsize = lpcmdmamanager->dmasize >> 1;
		lpcmdmamanager->pEnd =
			lpcmdmamanager->addr_linear +
			(lpcmdmamanager->dmasize >> 1) - 1;
		set_agp_double_cmd_inv(dev);
		if (dev_priv->chip_sub_index == CHIP_H6S2) {
			DRM_INFO("DMA buffer initialized finished. ");
			DRM_INFO("Use PCIE Double Buffer type!\n");
			DRM_INFO("Total PCIE DMA buffer size = %8d bytes. \n",
				 lpcmdmamanager->dmasize << 2);
		} else {
			DRM_INFO("DMA buffer initialized finished. ");
			DRM_INFO("Use AGP Double Buffer type!\n");
			DRM_INFO("Total AGP DMA buffer size = %8d bytes. \n",
				 lpcmdmamanager->dmasize << 2);
		}
	} else if (dev_priv->drm_agp_type == DRM_AGP_RING_BUFFER) {
		lpcmdmamanager->maxkickoffsize = lpcmdmamanager->dmasize;
		lpcmdmamanager->pEnd =
			lpcmdmamanager->addr_linear + lpcmdmamanager->dmasize;
		set_agp_ring_cmd_inv(dev);
		if (dev_priv->chip_sub_index == CHIP_H6S2) {
			DRM_INFO("DMA buffer initialized finished. \n");
			DRM_INFO("Use PCIE Ring Buffer type!");
			DRM_INFO("Total PCIE DMA buffer size = %8d bytes. \n",
				 lpcmdmamanager->dmasize << 2);
		} else {
			DRM_INFO("DMA buffer initialized finished. ");
			DRM_INFO("Use AGP Ring Buffer type!\n");
			DRM_INFO("Total AGP DMA buffer size = %8d bytes. \n",
				 lpcmdmamanager->dmasize << 2);
		}
	} else if (dev_priv->drm_agp_type == DRM_AGP_DISABLED) {
		lpcmdmamanager->maxkickoffsize = 0x0;
		if (dev_priv->chip_sub_index == CHIP_H6S2)
			DRM_INFO("PCIE init failed! Use PCI\n");
		else
			DRM_INFO("AGP init failed! Use PCI\n");
	}
	return 0;
}

static void
kickoff_bci_inv(struct drm_device *dev,
		struct drm_via_chrome9_flush *dma_info)
{
	u32 hdtype, dwqwcount, i, dwcount, add1, addr2, swpointer,
		swpointerend;
	unsigned long *pcmddata;
	int result;

	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	/*pcmddata = __s3gke_vmalloc(dma_info->cmd_size<<2); */
	pcmddata = dev_priv->bci_buffer;

	if (!pcmddata)
		return;
	result = copy_from_user((int *) pcmddata, dma_info->usermode_dma_buf,
				dma_info->cmd_size << 2);
	if (result) {
		DRM_ERROR("In function kickoff_bci_inv,\
		copy_from_user is fault. \n");
		return ;
	}
#if VIA_CHROME9_VERIFY_ENABLE
	result = via_chrome9_verify_command_stream(
		(const uint32_t *)pcmddata,	dma_info->cmd_size << 2,
		dev, dev_priv->chip_sub_index == CHIP_H6S2 ? 0 : 1);
	if (result) {
		DRM_ERROR("The command has the security issue \n");
		return ;
	}
#endif
	swpointer = 0;
	swpointerend = (u32) dma_info->cmd_size;
	while (swpointer < swpointerend) {
		hdtype = pcmddata[swpointer] & INV_AGPHeader_MASK;
		switch (hdtype) {
		case INV_AGPHeader0:
		case INV_AGPHeader5:
			dwqwcount = pcmddata[swpointer + 1];
			swpointer += 4;

			for (i = 0; i < dwqwcount; i++) {
				setmmioregister(dev_priv->mmio->handle,
						pcmddata[swpointer],
						pcmddata[swpointer + 1]);
				swpointer += 2;
			}
			break;

		case INV_AGPHeader1:
			dwcount = pcmddata[swpointer + 1];
			add1 = 0x0;
			swpointer += 4;	/* skip 128-bit. */

			for (; dwcount > 0; dwcount--, swpointer++,
				add1 += 4) {
				setmmioregister(dev_priv->hostBlt->handle,
						add1, pcmddata[swpointer]);
			}
			break;

		case INV_AGPHeader4:
			dwcount = pcmddata[swpointer + 1];
			add1 = pcmddata[swpointer] & 0x0000FFFF;
			swpointer += 4;	/* skip 128-bit. */

			for (; dwcount > 0; dwcount--, swpointer++)
				setmmioregister(dev_priv->mmio->handle, add1,
						pcmddata[swpointer]);
			break;

		case INV_AGPHeader2:
			add1 = pcmddata[swpointer + 1] & 0xFFFF;
			addr2 = pcmddata[swpointer] & 0xFFFF;

			/* Write first data (either ParaType or whatever) to
			add1 */
			setmmioregister(dev_priv->mmio->handle, add1,
					pcmddata[swpointer + 2]);
			swpointer += 4;

			/* The following data are all written to addr2,
			   until another header is met */
			while (!is_agp_header(pcmddata[swpointer])
			       && (swpointer < swpointerend)) {
				setmmioregister(dev_priv->mmio->handle, addr2,
						pcmddata[swpointer]);
				swpointer++;
			}
			break;

		case INV_AGPHeader3:
			add1 = pcmddata[swpointer] & 0xFFFF;
			addr2 = add1 + 4;
			dwcount = pcmddata[swpointer + 1];

			/* Write first data (either ParaType or whatever) to
			add1 */
			setmmioregister(dev_priv->mmio->handle, add1,
					pcmddata[swpointer + 2]);
			swpointer += 4;

			for (i = 0; i < dwcount; i++) {
				setmmioregister(dev_priv->mmio->handle, addr2,
						pcmddata[swpointer]);
				swpointer++;
			}
			break;

		case INV_AGPHeader6:
			break;

		case INV_AGPHeader7:
			break;

		default:
			swpointer += 4;	/* Advance to next header */
		}

		swpointer = (swpointer + 3) & ~3;
	}
}

void
kickoff_dma_db_inv(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;

	u32 BufferSize = (u32) (lpcmdmamanager->pFree - lpcmdmamanager->pBeg);

	unsigned int agpbuflinearbase =
		(unsigned int) lpcmdmamanager->addr_linear;
	unsigned int agpbufphysicalbase =
		(unsigned int) dev->agp->base + lpcmdmamanager->pphysical;
	/*add shadow offset */

	unsigned int dwstart, dwend, dwpause;
	unsigned int dwreg60, dwreg61, dwreg62, dwreg63, dwreg64, dwreg65;
	unsigned int CR_Status;

	if (BufferSize == 0)
		return;

	/* 256-bit alignment of AGP pause address */
	if ((u32) ((unsigned long *) lpcmdmamanager->pFree) & 0x1f) {
		addcmdheader2_invi(lpcmdmamanager->pFree, INV_REG_CR_TRANS,
				   INV_ParaType_Dummy);
		do {
			addcmddata_invi(lpcmdmamanager->pFree, 0xCCCCCCC0);
			addcmddata_invi(lpcmdmamanager->pFree, 0xDDD00000);
		}
		while (((unsigned int) lpcmdmamanager->pFree) & 0x1f)
			;
	}

	dwstart =
		(u32) (unsigned long *)lpcmdmamanager->pBeg -
		agpbuflinearbase + agpbufphysicalbase;
	dwend = (u32) (unsigned long *)lpcmdmamanager->pEnd -
		agpbuflinearbase + agpbufphysicalbase;
	dwpause =
		(u32)(unsigned long *)lpcmdmamanager->pFree -
		agpbuflinearbase + agpbufphysicalbase - 4;

	dwreg60 = INV_SubA_HAGPBstL | INV_HWBasL(dwstart);
	dwreg61 = INV_SubA_HAGPBstH | INV_HWBasH(dwstart);
	dwreg62 = INV_SubA_HAGPBendL | INV_HWBasL(dwend);
	dwreg63 = INV_SubA_HAGPBendH | INV_HWBasH(dwend);
	dwreg64 = INV_SubA_HAGPBpL | INV_HWBasL(dwpause);
	dwreg65 = INV_SubA_HAGPBpID | INV_HWBasH(dwpause) | INV_HAGPBpID_STOP;

	/* wait CR idle */
	CR_Status = getmmioregister(dev_priv->mmio->handle, INV_RB_ENG_STATUS);
	while (CR_Status & INV_ENG_BUSY_CR)
		CR_Status =
			getmmioregister(dev_priv->mmio->handle,
					INV_RB_ENG_STATUS);

	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS,
			INV_ParaType_PreCR);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg60);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg61);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg62);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg63);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg64);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg65);

	/* Trigger AGP cycle */
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN,
			INV_SubA_HFthRCM | INV_HFthRCM_10 | INV_HAGPBTrig);

	if (lpcmdmamanager->pBeg == lpcmdmamanager->addr_linear) {
		/* The second AGP command buffer */
		lpcmdmamanager->pBeg =
			lpcmdmamanager->addr_linear +
			(lpcmdmamanager->dmasize >> 2);
		lpcmdmamanager->pEnd =
			lpcmdmamanager->addr_linear + lpcmdmamanager->dmasize;
		lpcmdmamanager->pFree = lpcmdmamanager->pBeg;
	} else {
		/* The first AGP command buffer */
		lpcmdmamanager->pBeg = lpcmdmamanager->addr_linear;
		lpcmdmamanager->pEnd =
			lpcmdmamanager->addr_linear +
			(lpcmdmamanager->dmasize / 2) - 1;
		lpcmdmamanager->pFree = lpcmdmamanager->pBeg;
	}
	CR_Status = getmmioregister(dev_priv->mmio->handle, INV_RB_ENG_STATUS);
}


void
kickoff_dma_ring_inv(struct drm_device *dev)
{
	unsigned int dwpause, dwreg64, dwreg65;

	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;

	unsigned int agpbuflinearbase =
		(unsigned int) lpcmdmamanager->addr_linear;
	unsigned int agpbufphysicalbase =
		(dev_priv->chip_agp ==
		 CHIP_PCIE) ? 0 : (unsigned int) dev->agp->base +
		lpcmdmamanager->pphysical;
	/*add shadow offset */

	/* 256-bit alignment of AGP pause address */
	if (dev_priv->chip_sub_index == CHIP_H6S2) {
		if ((u32)
		    ((unsigned long *) lpcmdmamanager->pFree) & 0x7f) {
			addcmdheader2_invi(lpcmdmamanager->pFree,
					   INV_REG_CR_TRANS,
					   INV_ParaType_Dummy);
			do {
				addcmddata_invi(lpcmdmamanager->pFree,
						0xCCCCCCC0);
				addcmddata_invi(lpcmdmamanager->pFree,
						0xDDD00000);
			}
			while ((u32)((unsigned long *) lpcmdmamanager->pFree) &
				0x7f)
				;
		}
	} else {
		if ((u32)
		    ((unsigned long *) lpcmdmamanager->pFree) & 0x1f) {
			addcmdheader2_invi(lpcmdmamanager->pFree,
					   INV_REG_CR_TRANS,
					   INV_ParaType_Dummy);
			do {
				addcmddata_invi(lpcmdmamanager->pFree,
						0xCCCCCCC0);
				addcmddata_invi(lpcmdmamanager->pFree,
						0xDDD00000);
			}
			while ((u32)((unsigned long *) lpcmdmamanager->pFree) &
				0x1f)
				;
		}
	}


	dwpause = (u32) ((unsigned long *) lpcmdmamanager->pFree)
		- agpbuflinearbase + agpbufphysicalbase - 16;

	dwreg64 = INV_SubA_HAGPBpL | INV_HWBasL(dwpause);
	dwreg65 = INV_SubA_HAGPBpID | INV_HWBasH(dwpause) | INV_HAGPBpID_PAUSE;

	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS,
			INV_ParaType_PreCR);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg64);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg65);

	lpcmdmamanager->pinusebysw = lpcmdmamanager->pFree;
}

static int
waitchipidle_inv(struct drm_via_chrome9_private *dev_priv)
{
	unsigned int count = 50000;
	unsigned int eng_status;
	unsigned int engine_busy;

	do {
		eng_status =
			getmmioregister(dev_priv->mmio->handle,
					INV_RB_ENG_STATUS);
		engine_busy = eng_status & INV_ENG_BUSY_ALL;
		count--;
	}
	while (engine_busy && count)
		;
	if (count && engine_busy == 0)
		return 0;
	return -1;
}

void
get_space_db_inv(struct drm_device *dev,
	struct cmd_get_space *lpcmgetspacedata)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;

	unsigned int dwRequestSize = lpcmgetspacedata->dwRequestSize;
	if (dwRequestSize > lpcmdmamanager->maxkickoffsize) {
		DRM_INFO("too big DMA buffer request!!!\n");
		via_chrome9ke_assert(0);
		*lpcmgetspacedata->pcmddata = (unsigned int) NULL;
		return;
	}

	if ((lpcmdmamanager->pFree + dwRequestSize) >
	    (lpcmdmamanager->pEnd - INV_CMDBUF_THRESHOLD * 2))
		kickoff_dma_db_inv(dev);

	*lpcmgetspacedata->pcmddata = (unsigned int) lpcmdmamanager->pFree;
}

void
rewind_ring_agp_inv(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;

	unsigned int agpbuflinearbase =
		(unsigned int) lpcmdmamanager->addr_linear;
	unsigned int agpbufphysicalbase =
		(dev_priv->chip_agp ==
		 CHIP_PCIE) ? 0 : (unsigned int) dev->agp->base +
		lpcmdmamanager->pphysical;
	/*add shadow offset */

	unsigned int dwpause, dwjump;
	unsigned int dwReg66, dwReg67;
	unsigned int dwreg64, dwreg65;

	addcmdheader2_invi(lpcmdmamanager->pFree, INV_REG_CR_TRANS,
			   INV_ParaType_Dummy);
	addcmddata_invi(lpcmdmamanager->pFree, 0xCCCCCCC7);
	if (dev_priv->chip_sub_index == CHIP_H6S2)
		while ((unsigned int) lpcmdmamanager->pFree & 0x7F)
			addcmddata_invi(lpcmdmamanager->pFree, 0xCCCCCCC7);
	else
		while ((unsigned int) lpcmdmamanager->pFree & 0x1F)
			addcmddata_invi(lpcmdmamanager->pFree, 0xCCCCCCC7);
	dwjump = ((u32) ((unsigned long *) lpcmdmamanager->pFree))
		- agpbuflinearbase + agpbufphysicalbase - 16;

	lpcmdmamanager->pFree = lpcmdmamanager->pBeg;

	dwpause = ((u32) ((unsigned long *) lpcmdmamanager->pFree))
		- agpbuflinearbase + agpbufphysicalbase - 16;

	dwreg64 = INV_SubA_HAGPBpL | INV_HWBasL(dwpause);
	dwreg65 = INV_SubA_HAGPBpID | INV_HWBasH(dwpause) | INV_HAGPBpID_PAUSE;

	dwReg66 = INV_SubA_HAGPBjumpL | INV_HWBasL(dwjump);
	dwReg67 = INV_SubA_HAGPBjumpH | INV_HWBasH(dwjump);

	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS,
			INV_ParaType_PreCR);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwReg66);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwReg67);

	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg64);
	setmmioregister(dev_priv->mmio->handle, INV_REG_CR_BEGIN, dwreg65);
	lpcmdmamanager->pinusebysw = lpcmdmamanager->pFree;
}


void
get_space_ring_inv(struct drm_device *dev,
		   struct cmd_get_space *lpcmgetspacedata)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;
	unsigned int dwUnFlushed;
	unsigned int dwRequestSize = lpcmgetspacedata->dwRequestSize;

	unsigned int agpbuflinearbase =
		(unsigned int) lpcmdmamanager->addr_linear;
	unsigned int agpbufphysicalbase =
		(dev_priv->chip_agp ==
		 CHIP_PCIE) ? 0 : (unsigned int) dev->agp->base +
		lpcmdmamanager->pphysical;
	/*add shadow offset */
	u32 BufStart, BufEnd, CurSW, CurHW, NextSW, BoundaryCheck;

	dwUnFlushed =
		(unsigned int) (lpcmdmamanager->pFree - lpcmdmamanager->pBeg);
	/*default bEnableModuleSwitch is on for metro,is off for rest */
	/*cmHW_Module_Switch is context-wide variable which is enough for 2d/3d
	   switch in a context. */
	/*But we must keep the dma buffer being wrapped head and tail by 3d cmds
	   when it is kicked off to kernel mode. */
	/*Get DMA Space (If requested, or no BCI space and BCI not forced. */

	if (dwRequestSize > lpcmdmamanager->maxkickoffsize) {
		DRM_INFO("too big DMA buffer request!!!\n");
		via_chrome9ke_assert(0);
		*lpcmgetspacedata->pcmddata = 0;
		return;
	}

	if (dwUnFlushed + dwRequestSize > lpcmdmamanager->maxkickoffsize)
		kickoff_dma_ring_inv(dev);

	BufStart =
		(u32)((unsigned int) lpcmdmamanager->pBeg) - agpbuflinearbase +
		agpbufphysicalbase;
	BufEnd = (u32)((unsigned int) lpcmdmamanager->pEnd) - agpbuflinearbase +
		agpbufphysicalbase;
	dwRequestSize = lpcmgetspacedata->dwRequestSize << 2;
	NextSW = (u32) ((unsigned int) lpcmdmamanager->pFree) + dwRequestSize +
		INV_CMDBUF_THRESHOLD * 8 - agpbuflinearbase +
		agpbufphysicalbase;

	CurSW = (u32)((unsigned int) lpcmdmamanager->pFree) - agpbuflinearbase +
		agpbufphysicalbase;
	CurHW = getmmioregister(dev_priv->mmio->handle, INV_RB_AGPCMD_CURRADDR);

	if (NextSW >= BufEnd) {
		kickoff_dma_ring_inv(dev);
		CurSW = (u32) ((unsigned int) lpcmdmamanager->pFree) -
			agpbuflinearbase + agpbufphysicalbase;
		/* make sure the last rewind is completed */
		CurHW = getmmioregister(dev_priv->mmio->handle,
					INV_RB_AGPCMD_CURRADDR);
		while (CurHW > CurSW)
			CurHW = getmmioregister(dev_priv->mmio->handle,
						INV_RB_AGPCMD_CURRADDR);
		/* Sometime the value read from HW is unreliable,
		so need double confirm. */
		CurHW = getmmioregister(dev_priv->mmio->handle,
					INV_RB_AGPCMD_CURRADDR);
		while (CurHW > CurSW)
			CurHW = getmmioregister(dev_priv->mmio->handle,
						INV_RB_AGPCMD_CURRADDR);
		BoundaryCheck =
			BufStart + dwRequestSize + INV_QW_PAUSE_ALIGN * 16;
		if (BoundaryCheck >= BufEnd)
			/* If an empty command buffer can't hold
			the request data. */
			via_chrome9ke_assert(0);
		else {
			/* We need to guarntee the new commands have no chance
			to override the unexected commands or wait until there
			is no unexecuted commands in agp buffer */
			if (CurSW <= BoundaryCheck) {
				CurHW = getmmioregister(dev_priv->mmio->handle,
							INV_RB_AGPCMD_CURRADDR);
				while (CurHW < CurSW)
					CurHW = getmmioregister(
					dev_priv->mmio->handle,
					INV_RB_AGPCMD_CURRADDR);
				/*Sometime the value read from HW is unreliable,
				   so need double confirm. */
				CurHW = getmmioregister(dev_priv->mmio->handle,
							INV_RB_AGPCMD_CURRADDR);
				while (CurHW < CurSW) {
					CurHW = getmmioregister(
						dev_priv->mmio->handle,
						INV_RB_AGPCMD_CURRADDR);
				}
				rewind_ring_agp_inv(dev);
				CurSW = (u32) ((unsigned long *)
					       lpcmdmamanager->pFree) -
					agpbuflinearbase + agpbufphysicalbase;
				CurHW = getmmioregister(dev_priv->mmio->handle,
							INV_RB_AGPCMD_CURRADDR);
				/* Waiting until hw pointer jump to start
				and hw pointer will */
				/* equal to sw pointer */
				while (CurHW != CurSW) {
					CurHW = getmmioregister(
						dev_priv->mmio->handle,
						INV_RB_AGPCMD_CURRADDR);
				}
			} else {
				CurHW = getmmioregister(dev_priv->mmio->handle,
							INV_RB_AGPCMD_CURRADDR);

				while (CurHW <= BoundaryCheck) {
					CurHW = getmmioregister(
						dev_priv->mmio->handle,
						INV_RB_AGPCMD_CURRADDR);
				}
				CurHW = getmmioregister(dev_priv->mmio->handle,
							INV_RB_AGPCMD_CURRADDR);
				/* Sometime the value read from HW is
				unreliable, so need double confirm. */
				while (CurHW <= BoundaryCheck) {
					CurHW = getmmioregister(
						dev_priv->mmio->handle,
						INV_RB_AGPCMD_CURRADDR);
				}
				rewind_ring_agp_inv(dev);
			}
		}
	} else {
		/* no need to rewind Ensure unexecuted agp commands will
		not be override by new
		agp commands */
		CurSW = (u32) ((unsigned int) lpcmdmamanager->pFree) -
			agpbuflinearbase + agpbufphysicalbase;
		CurHW = getmmioregister(dev_priv->mmio->handle,
					INV_RB_AGPCMD_CURRADDR);

		while ((CurHW > CurSW) && (CurHW <= NextSW))
			CurHW = getmmioregister(dev_priv->mmio->handle,
						INV_RB_AGPCMD_CURRADDR);

		/* Sometime the value read from HW is unreliable,
		so need double confirm. */
		CurHW = getmmioregister(dev_priv->mmio->handle,
					INV_RB_AGPCMD_CURRADDR);
		while ((CurHW > CurSW) && (CurHW <= NextSW))
			CurHW = getmmioregister(dev_priv->mmio->handle,
						INV_RB_AGPCMD_CURRADDR);
	}
	/*return the space handle */
	*lpcmgetspacedata->pcmddata = (unsigned int) lpcmdmamanager->pFree;
}

void
release_space_inv(struct drm_device *dev,
		  struct cmd_release_space *lpcmReleaseSpaceData)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_dma_manager *lpcmdmamanager =
		dev_priv->dma_manager;
	unsigned int dwReleaseSize = lpcmReleaseSpaceData->dwReleaseSize;
	int i = 0;

	lpcmdmamanager->pFree += dwReleaseSize;

	/* aligned address */
	while (((unsigned int) lpcmdmamanager->pFree) & 0xF) {
		/* not in 4 unsigned ints (16 Bytes) align address,
		insert NULL Commands */
		*lpcmdmamanager->pFree++ = NULL_COMMAND_INV[i & 0x3];
		i++;
	}

	if ((dev_priv->chip_sub_index == CHIP_H5 ||
		dev_priv->chip_sub_index == CHIP_H6S2) &&
		(dev_priv->drm_agp_type == DRM_AGP_RING_BUFFER)) {
		addcmdheader2_invi(lpcmdmamanager->pFree, INV_REG_CR_TRANS,
				   INV_ParaType_Dummy);
		for (i = 0; i < NULLCOMMANDNUMBER; i++)
			addcmddata_invi(lpcmdmamanager->pFree, 0xCC000000);
	}
}

int
via_chrome9_ioctl_flush(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	struct drm_via_chrome9_flush *dma_info = data;
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	int ret = 0;
	int result = 0;
	struct cmd_get_space getspace;
	struct cmd_release_space releasespace;
	unsigned long *pcmddata = NULL;

	/* Test that the hardware lock is held by the caller, */
	/* returning otherwise	*/
	LOCK_TEST_WITH_RETURN(dev, file_priv);
	switch (dma_info->dma_cmd_type) {
		/* Copy DMA buffer to BCI command buffer */
	case flush_bci:
	case flush_bci_and_wait:
		if (dma_info->cmd_size <= 0)
			return 0;
		if (dma_info->cmd_size > MAX_BCI_BUFFER_SIZE) {
			DRM_INFO("too big BCI space request!!!\n");
			return 0;
		}

		kickoff_bci_inv(dev, dma_info);
		waitchipidle_inv(dev_priv);
		break;
		/* Use DRM DMA buffer manager to kick off DMA directly */
	case dma_kickoff:
		break;

		/* Copy user mode DMA buffer to kernel DMA buffer,
		then kick off DMA */
	case flush_dma_buffer:
	case flush_dma_and_wait:
		if (dma_info->cmd_size <= 0)
			return 0;

		getspace.dwRequestSize = dma_info->cmd_size;
		if ((dev_priv->chip_sub_index == CHIP_H5 ||
			dev_priv->chip_sub_index == CHIP_H6S2) &&
			(dev_priv->drm_agp_type == DRM_AGP_RING_BUFFER))
			getspace.dwRequestSize += (NULLCOMMANDNUMBER + 4);
		/* Patch for VT3293 agp ring buffer stability */
		getspace.pcmddata = (unsigned int *) &pcmddata;

		if (dev_priv->drm_agp_type == DRM_AGP_DOUBLE_BUFFER)
			get_space_db_inv(dev, &getspace);
		else if (dev_priv->drm_agp_type == DRM_AGP_RING_BUFFER)
			get_space_ring_inv(dev, &getspace);
		if (pcmddata) {
			/*copy data from userspace to kernel-dma-agp buffer */
			result = copy_from_user((int *)
						pcmddata,
						dma_info->usermode_dma_buf,
						dma_info->cmd_size << 2);
			if (result) {
				DRM_ERROR("In function via_chrome9_ioctl_flush,"
				"copy_from_user is fault. \n");
				return -EINVAL;
			}

#if VIA_CHROME9_VERIFY_ENABLE
		result = via_chrome9_verify_command_stream(
			(const uint32_t *)pcmddata, dma_info->cmd_size << 2,
			dev, dev_priv->chip_sub_index == CHIP_H6S2 ? 0 : 1);
		if (result) {
			DRM_ERROR("The user command has security issue.\n");
			return -EINVAL;
		}
#endif

			releasespace.dwReleaseSize = dma_info->cmd_size;
			release_space_inv(dev, &releasespace);
			if (dev_priv->drm_agp_type == DRM_AGP_DOUBLE_BUFFER)
				kickoff_dma_db_inv(dev);
			else if (dev_priv->drm_agp_type == DRM_AGP_RING_BUFFER)
				kickoff_dma_ring_inv(dev);

			if (dma_info->dma_cmd_type == flush_dma_and_wait)
				waitchipidle_inv(dev_priv);
		} else {
			DRM_INFO("No enough DMA space");
			ret = -ENOMEM;
		}
		break;

	default:
		DRM_INFO("Invalid DMA buffer type");
		ret = -EINVAL;
		break;
	}
	return ret;
}

int insert_ringbuffer_execute_h5(struct drm_device *dev,
	struct kmd_branch_buf_rec *p_kmd, struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct cmd_get_space getspace;
	struct cmd_release_space releasespace;
	unsigned int *pCmdData = NULL;
	unsigned int *pCmdBuf = NULL;
	unsigned int FenceCfg0, FenceCfg1, FenceCfg2;
	unsigned int buf_phy_addr, cmd_size, fenceID, sysfence_phyaddr;
	unsigned int i, dwReg69, dwReg6A, dwReg6B;
	unsigned int ret = 0;

	/* this is the agp physical address :*/
	/* agp offset + agp base physical address */
	buf_phy_addr = p_kmd->offset + (unsigned int)dev_priv->agp_offset;
	cmd_size = p_kmd->cmd_size;
	fenceID = p_kmd->fence_id;
	sysfence_phyaddr = p_kmd->sysfence_phyaddr;

#if BRANCH_AGP_DEBUG
	DRM_INFO(" IN function insert_ringbuffer_execute_h5:  kmd buffer Id %x"
		"fence id 0x%08x\n set fence id to system memory "
		"viraddr[0x%08x]/phyaddr[0x%08x]\n ",
		p_kmd->id, p_kmd->fence_id,
		(unsigned int)p_kmd->sysfence_viraddr, p_kmd->sysfence_phyaddr);
#endif

	FenceCfg0 = (INV_SubA_HFCWBasL | (sysfence_phyaddr & 0x00FFFFFF));
	FenceCfg1 = (INV_SubA_HFCIDL | (fenceID & 0x00FFFFFF));
	/* fence trigger: interrupt gen + record fence ID to register */
	FenceCfg2 = (INV_SubA_HFCTrig | INV_HFCTrg | INV_HFCMode_IntGen |
		INV_HFCMode_Idle | ((fenceID & 0xFF000000) >> 16) |
		((sysfence_phyaddr & 0xFF000000) >> 24));
	/* fence trigger: write to sysa */
	if (dev_priv->capabilities & CAPS_FENCE_IN_SYSTEM)
		FenceCfg2 |= INV_HFCMode_SysWrite;

	if (dev_priv->capabilities & CAPS_FENCE_INT_WAIT)
		FenceCfg2 |= INV_HFCMode_IntWait;

	getspace.dwRequestSize = AGP_REQ_FIFO_DEPTH*4+28;
	getspace.pcmddata = (unsigned int *) &pCmdData;
	get_space_ring_inv(dev, &getspace);

	if (pCmdData) {
		pCmdBuf = pCmdData;
		dwReg69 = 0x69000000 | ((buf_phy_addr & 0x00fffff0) | 0x01);
		dwReg6A = 0x6A000000 | ((buf_phy_addr & 0xff000000) >> 24);
		dwReg6B = 0x6B000000 | (cmd_size >> 2);
		addcmdheader2_invi(pCmdBuf, INV_REG_CR_TRANS,
			INV_ParaType_PreCR);
		addcmddata_invi(pCmdBuf, 0xCC000000);
		addcmddata_invi(pCmdBuf, dwReg69);
		addcmddata_invi(pCmdBuf, dwReg6A);
		addcmddata_invi(pCmdBuf, dwReg6B);

		addcmdheader82_invi(pCmdBuf, INV_REG_CR_TRANS,
			INV_ParaType_Dummy);


		addcmdheader2_invi(pCmdBuf, INV_REG_CR_TRANS,
			INV_ParaType_CR);
		addcmddata_invi(pCmdBuf, 0xCC00000F);
		addcmddata_invi(pCmdBuf, FenceCfg0);
		addcmddata_invi(pCmdBuf, FenceCfg1);
		addcmddata_invi(pCmdBuf, FenceCfg2);

		for (i = 0 ; i < AGP_REQ_FIFO_DEPTH/2; i++) {
			addcmdheader2_invi(pCmdBuf, INV_REG_CR_TRANS,
			INV_ParaType_Dummy);
			addcmddata_invi(pCmdBuf, 0xCC0CCCCC);
		}
		releasespace.dwReleaseSize = (unsigned int)pCmdBuf -
			(unsigned int)pCmdData;
		releasespace.dwReleaseSize = releasespace.dwReleaseSize>>2;

		release_space_inv(dev, &releasespace);
		kickoff_dma_ring_inv(dev);
	} else {
		DRM_INFO("No enough DMA space");
		ret = -ENOMEM;
	}

	return ret;
}

int insert_ringbuffer_execute_h6(struct drm_device *dev,
	struct kmd_branch_buf_rec *p_kmd, struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct cmd_get_space getspace;
	struct cmd_release_space releasespace;
	unsigned int *pCmdData = NULL;
	unsigned int *pCmdBuf = NULL;
	unsigned int FenceCfg0, FenceCfg1, FenceCfg2, FenceCfg3;
	unsigned int buf_phy_addr, cmd_size, fenceID, sysfence_phyaddr;
	unsigned int i, dwReg68, dwReg69, dwReg6A, dwReg6B;
	unsigned int ret = 0;

	/* This is the agp physical address :     */
	/* agp offset + agp base physical address */
	buf_phy_addr = p_kmd->offset + (unsigned int)dev_priv->agp_offset;
	cmd_size = p_kmd->cmd_size;
	fenceID = p_kmd->fence_id;
	sysfence_phyaddr = p_kmd->sysfence_phyaddr;

	FenceCfg0 = (INV_SubA_HFCWBasH | (sysfence_phyaddr & 0xFF000000)>>24);
	FenceCfg1 = (INV_SubA_HFCWBasL2 | (sysfence_phyaddr & 0x00FFFFF0));
	FenceCfg2 = (INV_SubA_HFCIDL2 | (fenceID & 0x00FFFFFF));
	FenceCfg3 = (INV_SubA_HFCTrig2 | INV_HFCTrg | INV_HFCMode_IntGen |
		INV_HFCMode_Idle | ((fenceID & 0xFF000000) >> 16));

	/* fence trigger: write to sys */
	if (dev_priv->capabilities & CAPS_FENCE_IN_SYSTEM)
		FenceCfg3 |= INV_HFCMode_SysWrite;
	if (dev_priv->capabilities & CAPS_FENCE_INT_WAIT)
		FenceCfg3 |= INV_HFCMode_IntWait;

	getspace.dwRequestSize = AGP_REQ_FIFO_DEPTH*4+28;
	if ((dev_priv->chip_sub_index == CHIP_H6S2) &&
		(dev_priv->drm_agp_type == DRM_AGP_RING_BUFFER))
		getspace.dwRequestSize += (NULLCOMMANDNUMBER + 4);
	getspace.pcmddata = (unsigned int *) &pCmdData;
	get_space_ring_inv(dev, &getspace);

	if (pCmdData) {
		pCmdBuf = pCmdData;
		dwReg68 = 0x68000000;
		dwReg69 = 0x69000000 | ((buf_phy_addr & 0x00fffffe));
		dwReg6A = 0x6A000000 | ((buf_phy_addr & 0xff000000) >> 24) |
			0x00400000; 	/* use SF for branch buffer*/
		dwReg6B = 0x6B000000 | (cmd_size >> 2);
		addcmdheader2_invi(pCmdBuf, INV_REG_CR_TRANS,
			INV_ParaType_CR);
		addcmddata_invi(pCmdBuf, dwReg68);
		addcmddata_invi(pCmdBuf, dwReg69);
		addcmddata_invi(pCmdBuf, dwReg6A);
		addcmddata_invi(pCmdBuf, dwReg6B);

		addcmdheader82_invi(pCmdBuf, INV_REG_CR_TRANS,
			INV_ParaType_Dummy);

		addcmdheader2_invi(pCmdBuf, INV_REG_CR_TRANS,
			INV_ParaType_CR);
		addcmddata_invi(pCmdBuf, FenceCfg0);
		addcmddata_invi(pCmdBuf, FenceCfg1);
		addcmddata_invi(pCmdBuf, FenceCfg2);
		addcmddata_invi(pCmdBuf, FenceCfg3);

		for (i = 0 ; i < AGP_REQ_FIFO_DEPTH/2; i++) {
			addcmdheader2_invi(pCmdBuf, INV_REG_CR_TRANS,
			INV_ParaType_Dummy);
			addcmddata_invi(pCmdBuf, 0xCC0CCCCC);
		}
		releasespace.dwReleaseSize = (unsigned int)pCmdBuf -
			(unsigned int)pCmdData;
		releasespace.dwReleaseSize = releasespace.dwReleaseSize>>2;

		release_space_inv(dev, &releasespace);
		kickoff_dma_ring_inv(dev);
	} else {
		DRM_INFO("No enough DMA space");
		ret = -ENOMEM;
	}

	return ret;
}

int via_chrome9_branch_buf_flush(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	static struct kmd_branch_buf_rec *p_kmd;
	struct umd_branch_buf_rec *p_umd =
		(struct umd_branch_buf_rec *)data;
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	unsigned int sg_entry_idx, sg_page_num;
	bool valid = true;
#if BRANCH_AGP_DEBUG
	int i;  /* for dump branch agp buffer */
#endif

	/* Test that the hardware lock is held by the caller, */
	/* returning otherwise                                */
	LOCK_TEST_WITH_RETURN(dev, file_priv);

#if BRANCH_AGP_DEBUG
	DRM_INFO("IN via_chrome9_branch_buf_flush :\n");
	DRM_INFO("flush umd branch buffer : id = %x cmd_size = %x \n",
		p_umd->id, p_umd->cmd_size);
#endif
	if (p_umd->id >= BRANCH_BUFFER_NUMBER)
		return -EINVAL;

	/* find this dma buffer in the allocated array, if can't find,
	 * return error
	 */
	p_kmd = get_from_branch_buf_array(dev_priv,
		p_umd->id, ALLOCATED_BUF_ARRAY);
	if (!p_kmd)
		return -EINVAL;

	if ((p_umd->cmd_size == 0) || (p_umd->cmd_size > BRANCH_BUFFER_SIZE/4))
		valid = false;
	/* add for VA ddmpeg video decode AGP Header 5 command verify */
	if ((p_umd->cmd_size == 4) && (p_kmd->kernel_virtual)) {
		if (((((unsigned long *)p_kmd->kernel_virtual)[0] &
			INV_AGPHeader_MASK) == INV_AGPHeader5) &&
		((unsigned long *)p_kmd->kernel_virtual)[1] == 0x00000000) {
			valid = false;
		}
	}

#if VIA_CHROME9_VERIFY_ENABLE
	if (via_chrome9_verify_command_stream(
		(const uint32_t *)p_kmd->kernel_virtual,p_umd->cmd_size << 2,
		dev, dev_priv->chip_sub_index == CHIP_H6S2 ? 0 : 1)) {
		DRM_ERROR("The command has the security issue \n");
		valid = false;
	}
#endif

	if (valid == false) {
		/* Reset this dma buffer into prepared dma buffer */
		add_into_branch_buf_array(dev_priv, p_kmd, PREPARED_BUF_ARRAY);
		wake_up_interruptible(&dev_priv->prepared_array_wait_head);

#if BRANCH_AGP_DEBUG
		DRM_INFO("EMPTY command buffer, or INVALID command count in "
			"buffer flushed \n");
		DRM_INFO(" free branch buffer num : %x\n",
			dev_priv->freebuf_num_in_prepared_array);
		dev_priv->branch_empty_flushcount++;
#endif
		return 0;
	}


	/* assign a unique fence id into this dma buffer */
	mutex_lock(&dev_priv->branch_buffer_mutex);
	/* the only need sync info between p_kmd and p_umd :
	 * the valid command size in branch agp buffer.
	 */
	p_kmd->cmd_size = p_umd->cmd_size;
	sys_fenceid_generate_new(dev_priv);
	p_kmd->fence_id = dev_priv->sys_fenceid;
	mutex_unlock(&dev_priv->branch_buffer_mutex);


#if BRANCH_AGP_DEBUG
	DRM_INFO("flush kmd branch buffer : id = %x cmd_size  = %x fenceid "
		"= %x\n", p_kmd->id, p_kmd->cmd_size, p_kmd->fence_id);
	DRM_INFO("flush command dump : cmd_size = %x\n", p_kmd->cmd_size);
	for (i = 0; i < (p_kmd->cmd_size / 4); i++)
		DRM_INFO("%08x  %08x  %08x  %08x\n",
			((unsigned int *)p_kmd->kernel_virtual + 4 * i)[0],
			((unsigned int *)p_kmd->kernel_virtual + 4 * i)[1],
			((unsigned int *)p_kmd->kernel_virtual + 4 * i)[2],
			((unsigned int *)p_kmd->kernel_virtual + 4 * i)[3]
			);
	for (i = (4 * i); i < p_kmd->cmd_size; i++)
		DRM_INFO("%08x  ",
			((unsigned int *)p_kmd->kernel_virtual + i)[0]);
	DRM_INFO("\n\n");
#endif

	/* Add this dma buffer into flushed dma buffer */
	add_into_branch_buf_array(dev_priv, p_kmd, FLUSHED_BUF_ARRAY);

	if ((dev_priv->chip_sub_index == CHIP_H5) ||
		(dev_priv->chip_sub_index == CHIP_H5S1))
		insert_ringbuffer_execute_h5(dev, p_kmd, file_priv);

	if ((dev_priv->chip_sub_index == CHIP_H6S1) ||
		(dev_priv->chip_sub_index == CHIP_H6S2)) {
		sg_entry_idx = (p_kmd->offset +
				dev_priv->agp_offset)>>PAGE_SHIFT;
		sg_page_num = (p_kmd->cmd_size * sizeof(unsigned int)
			+ PAGE_SIZE - 1)>>PAGE_SHIFT;
		/* Add pcie memory sg pages range validation verify */
		if ((sg_entry_idx + sg_page_num) < dev->sg->pages) {
			via_chrome9_clflush_pages(
				&dev->sg->pagelist[sg_entry_idx],
				sg_page_num);
		}

		insert_ringbuffer_execute_h6(dev, p_kmd, file_priv);
	}

#if BRANCH_AGP_DEBUG
	dev_priv->branch_flushcount++;
	DRM_INFO("flush kmd branch buffer succeed!\n");
#endif
	return 0;
}

int
via_chrome9_ioctl_free(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	return 0;
}

int
via_chrome9_ioctl_wait_chip_idle(struct drm_device *dev, void *data,
			 struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;

	waitchipidle_inv(dev_priv);
	/* maybe_bug here, do we always return 0 */
	return 0;
}

int
via_chrome9_ioctl_flush_cache(struct drm_device *dev, void *data,
		      struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct umd_cflush_pages_rec *cache_page =
		(struct umd_cflush_pages_rec *)data;
	unsigned int sg_entry_idx, sg_page_num;

	if ((dev_priv->chip_sub_index == CHIP_H6S1) ||
		(dev_priv->chip_sub_index == CHIP_H6S2)) {
		sg_entry_idx = (cache_page->offset +
			dev_priv->agp_offset)>>PAGE_SHIFT;
		sg_page_num = (cache_page->size + PAGE_SIZE - 1)>>PAGE_SHIFT;
		/* Add pcie memory sg pages range validation verify */
		if ((sg_entry_idx + sg_page_num) < dev->sg->pages) {
			via_chrome9_clflush_pages(
				&dev->sg->pagelist[sg_entry_idx],
				sg_page_num);
		}
	}
	return 0;
}
