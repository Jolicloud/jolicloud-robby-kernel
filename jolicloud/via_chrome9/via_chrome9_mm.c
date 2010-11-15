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
#include "via_chrome9_drm.h"
#include "via_chrome9_drv.h"
#include "drm_sman.h"
#include "via_chrome9_mm.h"
#include "via_chrome9_3d_reg.h"

#define VIA_CHROME9_MM_GRANULARITY 4
#define VIA_CHROME9_MM_GRANULARITY_MASK ((1 << VIA_CHROME9_MM_GRANULARITY) - 1)

static struct kmd_branch_buf_rec branch_buf_pool[BRANCH_BUFFER_NUMBER];
static struct list_head branch_buf_array[BRANCH_BUF_ARRAY_NUMBER];

/* NOTE: array_index: 0->prepared, 1->allocated, 2->flushed.
 */
void
add_into_branch_buf_array(struct drm_via_chrome9_private *dev_priv,
	struct kmd_branch_buf_rec *p_kmd, u32 array_index)
{
	if (array_index < 0 || array_index > 2) {
		DRM_ERROR(" **ERROR** function add_into_branch_buf_array has "
			"been passed into a invalid array_index \n");
		return ;
	}
	mutex_lock(&dev_priv->branch_buffer_mutex);
	/* Modify its status before insert into relevant buffer array */
	p_kmd->status = (enum buf_status)array_index;
	list_add_tail(&p_kmd->neighbor, &branch_buf_array[array_index]);
	if (array_index == PREPARED_BUF_ARRAY)
		dev_priv->freebuf_num_in_prepared_array++;
	mutex_unlock(&dev_priv->branch_buffer_mutex);
}

/* This function is for fetching one branch buffer from each branch
 * buffer array which is indicated by array_index. For calling
 * this function, one should pass the branch buffer id into "condition"
 * when getting from "allocated" and "flushed" buffer,
 * ignore the parameter of "condition" when getting from prepared buffer.
 *
 * NOTE: array_index: 0->prepared, 1->allocated, 2->flushed.
 */
struct kmd_branch_buf_rec *
get_from_branch_buf_array(struct drm_via_chrome9_private *dev_priv,
	u32 condition, u32 array_index)
{
	struct list_head *p;
	struct kmd_branch_buf_rec *p_kmd;
	unsigned int fence_id;

	if (array_index < 0 || array_index > 2) {
		DRM_ERROR(" **ERROR** function get_from_branch_buf_array has "
			"been passed into a invalid array_index \n");
		return 0;
	}

	mutex_lock(&dev_priv->branch_buffer_mutex);
	list_for_each(p, &branch_buf_array[array_index]) {
		p_kmd = list_entry(p, struct kmd_branch_buf_rec, neighbor);

		switch (array_index) {
		case PREPARED_BUF_ARRAY:
			/* check the prepared buffer array only,
			 * no condition must be matched.
			 */
			list_del(p);
			dev_priv->freebuf_num_in_prepared_array--;
			mutex_unlock(&dev_priv->branch_buffer_mutex);
			return p_kmd;
			break;

		case ALLOCATED_BUF_ARRAY:
			/* check the allocated buffer array only,
			 * one condition must be matched:
			 * the buffer id is the right id that we are checking.
			 */
			if (p_kmd->id == condition) {
				list_del(p);
				mutex_unlock(&dev_priv->branch_buffer_mutex);
				return p_kmd;
			}
			break;

		case FLUSHED_BUF_ARRAY:
			/* check the flushed buffer array only,
			 * Two condition must be matched:
			 * 1. the buffer id is the right
			 * id that we are checking.
			 * 2. the buffer's fenceid is
			 * finished by hw cr interrupt.
			 */
			if (p_kmd->id != condition)
				break;
			if (dev_priv->capabilities & CAPS_FENCE_IN_SYSTEM) {
				fence_id = *p_kmd->sysfence_viraddr;
#if BRANCH_AGP_DEBUG
			DRM_INFO("IN function get_from_branch_buf_array:  "
				"kmd buffer Id %x fence id 0x%08x\n "
				"get fence id from system memory "
				"viraddr[0x%08x]/ phyaddr[0x%08x] = 0x%08x\n ",
				p_kmd->id, p_kmd->fence_id,
				(unsigned int)p_kmd->sysfence_viraddr,
				p_kmd->sysfence_phyaddr, fence_id);
#endif
			} else {
				fence_id =
					getmmioregister(dev_priv->mmio->handle,
					dev_priv->fenceid_readback_register);
#if BRANCH_AGP_DEBUG
				DRM_INFO(
					"IN function get_from_branch_buf_array:"
					"kmd buffer Id %x fence id 0x%08x\n "
					"get fence id from register[0x%08x] "
					"= 0x%08x\n",
					p_kmd->id, p_kmd->fence_id,
					dev_priv->fenceid_readback_register,
					fence_id);
#endif
			}

			if (p_kmd->fence_id <= fence_id) {
				list_del(p);
				mutex_unlock(&dev_priv->branch_buffer_mutex);
				return p_kmd;
			}
			break;

		}
	}
	mutex_unlock(&dev_priv->branch_buffer_mutex);
	return NULL;
}

/* This function is searching the flushed branch buf array for match
 * finished condition.if one branch buf is fenced_id is equal to that
 * of system memory fence id read back,then delete it from flushed
 * array, and correct it status to prepare, add into prepared array.
 * This function will be called in GFX IRQ related work queue for
 * handling fence id issue.
 * NOTE: array_index: 0->prepared, 1->allocated, 2->flushed.
 */
static void branch_buf_bottom_half_interrupt(struct work_struct *work)
{
	struct kmd_branch_buf_rec *p_kmd;
	struct drm_via_chrome9_private *dev_priv =
		container_of(work, struct drm_via_chrome9_private,
			branch_buf_work);
	u32 finish_count = 0;
	int i;

#if BRANCH_AGP_DEBUG
	DRM_INFO("IN branch_buf_bottom_half_interrupt:\n");
	DRM_INFO("Using work queue: %s \n", (dev_priv->branch_buf_wq) ?
		"Branch buffer private" : "Linux kernel share");
	dev_priv->branch_wqcount++;
#endif

	for (i = 0; i < BRANCH_BUFFER_NUMBER; i++) {
		p_kmd = get_from_branch_buf_array(dev_priv,
				i, FLUSHED_BUF_ARRAY);
		if (p_kmd) {
			finish_count++;
			add_into_branch_buf_array(dev_priv, p_kmd,
				PREPARED_BUF_ARRAY);
#if BRANCH_AGP_DEBUG
		DRM_INFO("finish kmd branch buffer succeed: id: %x "
			"cmd size: %x fence id: %x \n",
			p_kmd->id, p_kmd->cmd_size, p_kmd->fence_id);
		DRM_INFO("free branch buffer num : %x\n",
			dev_priv->freebuf_num_in_prepared_array);
#endif
		}
	}

	if (finish_count)
		wake_up_interruptible(&dev_priv->prepared_array_wait_head);

}

int via_chrome9_map_init(struct drm_device *dev,
	struct drm_via_chrome9_init *init)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *)dev->dev_private;

	dev_priv->sarea = drm_getsarea(dev);
	if (!dev_priv->sarea) {
		DRM_ERROR("could not find sarea!\n");
		goto error;
	}
	dev_priv->sarea_priv =
		(struct drm_via_chrome9_sarea *)((unsigned char *)dev_priv->
		sarea->handle + init->sarea_priv_offset);

	dev_priv->fb = drm_core_findmap(dev, init->fb_handle);
	if (!dev_priv->fb) {
		DRM_ERROR("could not find framebuffer!\n");
		goto error;
	}
	/* Frame buffer physical base address */
	dev_priv->fb_base_address = init->fb_base_address;

	if (init->shadow_size) {
		/* find apg shadow region mappings */
	dev_priv->shadow_map.shadow = drm_core_findmap(dev, init->
	shadow_handle);
		if (!dev_priv->shadow_map.shadow) {
			DRM_ERROR("could not shadow map!\n");
			goto error;
			}
		dev_priv->shadow_map.shadow_size = init->shadow_size;
		dev_priv->shadow_map.shadow_handle = (unsigned int *)dev_priv->
			shadow_map.shadow->handle;
		init->shadow_handle = dev_priv->shadow_map.shadow->offset;
		}
	if (init->agp_tex_size && init->chip_agp != CHIP_PCIE) {
		/* find apg texture buffer mappings */
		if (!(drm_core_findmap(dev, init->agp_tex_handle))) {
			DRM_ERROR("could not find agp texture map !\n");
			goto error;
		}
		dev_priv->agp_size = init->agp_tex_size;
		dev_priv->agp_offset = init->agp_tex_handle;
	}
	/* find mmio/dma mappings */
	dev_priv->mmio = drm_core_findmap(dev, init->mmio_handle);
	if (!dev_priv->mmio) {
		DRM_ERROR("failed to find mmio region!\n");
		goto error;
		}

	dev_priv->hostBlt = drm_core_findmap(dev, init->hostBlt_handle);
	if (!dev_priv->hostBlt) {
		DRM_ERROR("failed to find host bitblt region!\n");
		goto error;
		}

	dev_priv->drm_agp_type = init->agp_type;
	if (init->agp_type != AGP_DISABLED && init->chip_agp != CHIP_PCIE) {
		dev->agp_buffer_map = drm_core_findmap(dev, init->dma_handle);
		if (!dev->agp_buffer_map) {
			DRM_ERROR("failed to find dma buffer region!\n");
			goto error;
			}
		}

	dev_priv->bci  = (char *)dev_priv->mmio->handle + 0x10000;

	return 0;

error:
	/* do cleanup here, refine_later */
	return -EINVAL;
}

int via_chrome9_heap_management_init(struct drm_device *dev,
	struct drm_via_chrome9_init *init)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	int ret = 0;

	/* video memory management. range: 0 ---- video_whole_size */
	mutex_lock(&dev->struct_mutex);
	ret = drm_sman_set_range(&dev_priv->sman, VIA_CHROME9_MEM_VIDEO,
		0, dev_priv->available_fb_size >> VIA_CHROME9_MM_GRANULARITY);
	if (ret) {
		DRM_ERROR("VRAM memory manager initialization ******ERROR "
			"!******\n");
		mutex_unlock(&dev->struct_mutex);
		goto error;
	}
	dev_priv->vram_initialized = 1;
	/* agp/pcie heap management.
	note:because agp is contradict with pcie, so only one is enough
	for managing both of them.*/
	init->agp_type = dev_priv->drm_agp_type;
	if (init->agp_type != AGP_DISABLED && dev_priv->agp_size) {
		ret = drm_sman_set_range(&dev_priv->sman, VIA_CHROME9_MEM_AGP,
		0, dev_priv->agp_size >> VIA_CHROME9_MM_GRANULARITY);
	if (ret) {
		DRM_ERROR("AGP/PCIE memory manager initialization ******ERROR "
			"!******\n");
		mutex_unlock(&dev->struct_mutex);
		goto error;
		}
	dev_priv->agp_initialized = 1;
	}
	mutex_unlock(&dev->struct_mutex);
	return 0;

error:
	/* Do error recover here, refine_later */
	return -EINVAL;
}

/* This function initialize branch_buf_pool, and put all of them
 * prepared dma buffer array
 */
static int init_branch_buffer_pool(struct drm_device *dev,
	struct drm_via_chrome9_mem *mem)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	u32 i, branch_buf_offset;

	dev_priv->sysfence_readback_buf =
		kmalloc((FENCE_NUM + 1) * FENCE_SIZE, GFP_KERNEL);
	if (!dev_priv->sysfence_readback_buf) {
		DRM_ERROR("could not allocate system for "
			"sysfence_readback_buf!\n");
		return -ENOMEM;
	}
	memset(dev_priv->sysfence_readback_buf, 0x00,
		(FENCE_NUM + 1) * FENCE_SIZE);

	if ((dev_priv->chip_sub_index == CHIP_H5) ||
		(dev_priv->chip_sub_index == CHIP_H5S1)) {
		dev_priv->branch_buf_kernel_virtual = (void *)
			ioremap(dev_priv->agp_offset + mem->offset,
			BRANCH_BUFFER_NUMBER * BRANCH_BUFFER_SIZE);
	} else {
		dev_priv->branch_buf_kernel_virtual =
			(void *)(dev->sg->handle + dev_priv->agp_offset
			+ mem->offset);
	}

	if (!dev_priv->branch_buf_kernel_virtual) {
		DRM_ERROR("could not ioremap branch buffer in kernel mode!\n");
		return -ENOMEM;
	}

	/* initialize the branch buffer array */
	for (i = 0; i < BRANCH_BUF_ARRAY_NUMBER; i++)
		branch_buf_array[i].next = branch_buf_array[i].prev =
		&branch_buf_array[i];
	dev_priv->freebuf_num_in_prepared_array = 0;

	/* add all the dma buffers into the prepared list */
	for (i = 0; i < BRANCH_BUFFER_NUMBER; i++) {
		branch_buf_pool[i].id = i;
		branch_buf_pool[i].offset = branch_buf_offset =
			mem->offset + i * BRANCH_BUFFER_SIZE;
		if (branch_buf_offset + BRANCH_BUFFER_SIZE >
			mem->offset + mem->size)
			return -ENOMEM;
		branch_buf_pool[i].size = BRANCH_BUFFER_SIZE;
		branch_buf_pool[i].status = uninitialized;

		branch_buf_pool[i].kernel_virtual =
			(unsigned char *)dev_priv->branch_buf_kernel_virtual +
			i * BRANCH_BUFFER_SIZE;

		/* used for fence id readback */
		branch_buf_pool[i].sysfence_viraddr =
			(u32 *)(align_to_16_byte
			(dev_priv->sysfence_readback_buf) + i * FENCE_SIZE);
		/* used for fence id write back to system memory */
		branch_buf_pool[i].sysfence_phyaddr = (u32)
			virt_to_phys(branch_buf_pool[i].sysfence_viraddr);

		add_into_branch_buf_array(dev_priv,
			&branch_buf_pool[i], PREPARED_BUF_ARRAY);
	}

	return 0;
}

void
set_device_capablity(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;

	switch (dev_priv->chip_sub_index) {
	case CHIP_H5:
	case CHIP_H5S1:
		/* temporarily disable
		 * fence system memory write for CR busy issue */
		dev_priv->capabilities = CAPS_FENCE_INT_WAIT /*|
			CAPS_FENCE_IN_SYSTEM*/;
		/* from VT3336/3364 CR's specification */
		dev_priv->fenceid_readback_register = 0x474;
		break;

	case CHIP_H6S1:
	case CHIP_H6S2:
		dev_priv->capabilities = CAPS_FENCE_INT_WAIT |
			CAPS_FENCE_IN_SYSTEM;
		/* from VT3353 CR's specification */
		dev_priv->fenceid_readback_register = 0x494;

		break;
	default:
		break;
	}
}

/* This function clean branch_buf_pool.
 */
static int cleanup_branch_buffer_pool(struct drm_device *dev)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	u32 i;

	kfree(dev_priv->sysfence_readback_buf);
	dev_priv->sysfence_readback_buf = NULL;

	if (dev_priv->branch_buf_kernel_virtual) {
		if ((dev_priv->chip_sub_index == CHIP_H5) ||
			(dev_priv->chip_sub_index == CHIP_H5S1)) {
			iounmap(dev_priv->branch_buf_kernel_virtual);
		}
		dev_priv->branch_buf_kernel_virtual = NULL;
	}

	for (i = 0; i < BRANCH_BUF_ARRAY_NUMBER; i++)
		memset(&branch_buf_pool[i], 0x00,
				sizeof(struct kmd_branch_buf_rec));

	/* reset the branch buffer array */
	for (i = 0; i < BRANCH_BUF_ARRAY_NUMBER; i++)
		branch_buf_array[i].next = branch_buf_array[i].prev =
		&branch_buf_array[i];

	return 0;
}


/* This function allocates branch dma buffers preserved for branch mechanism
 * and call function init_branch_buffer_pool to initilize prepared branch
 * buffer array for later use */
int via_chrome9_branch_buf_init(struct drm_device *dev,
	struct drm_via_chrome9_init *init, struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_mem *mem = &dev_priv->branch_buf_mem;
	unsigned long interrupt_status;
	unsigned int dwreg69;

	/* already initialized before, just return here */
	if (dev_priv->branch_buf_enabled)
		return 0;

	/* Before we initialize branch buffer, we initialize necessary
	 * primive routine and process wait queue.
	 */
	init_waitqueue_head(&dev_priv->prepared_array_wait_head);
	mutex_init(&dev_priv->branch_buffer_mutex);
#if BRANCH_PRIVATE_WORKQUEUE
	dev_priv->branch_buf_wq =
		create_singlethread_workqueue("AGP_Branch_Irq");
#else
	dev_priv->branch_buf_wq = NULL;
#endif
	INIT_WORK(&dev_priv->branch_buf_work, branch_buf_bottom_half_interrupt);
	sys_fenceid_init(dev_priv);
	set_device_capablity(dev);

	if (request_irq(dev->pdev->irq, &via_chrome9_interrupt, IRQF_SHARED,
		"AGP_Branch_Irq", dev)) {
		DRM_ERROR("can not register the agp irq!.\n");
		return -EAGAIN;
	}

	mem->size = BRANCH_BUFFER_NUMBER * BRANCH_BUFFER_SIZE;
	mem->type = VIA_CHROME9_MEM_AGP;
	dev_priv->alignment = 0x80000100;/* make branch buf 256 bytes align */

	if (via_chrome9_ioctl_allocate_mem_base(dev, mem, file_priv)) {
		DRM_ERROR("Allocate memory error!.\n");
		sys_fenceid_reset(dev_priv);
		free_irq(dev->pdev->irq, dev);
		return -ENOMEM;
	}

	if (init_branch_buffer_pool(dev, mem)) {
		DRM_ERROR("init_branch_buffer_pool error!.\n");
		sys_fenceid_reset(dev_priv);
		free_irq(dev->pdev->irq, dev);
		cleanup_branch_buffer_pool(dev);
		via_chrome9_ioctl_freemem_base(dev, mem, file_priv);
		return -ENOMEM;
	}

	/* query hw gfx's interrupt status */
	interrupt_status = getmmioregister(dev_priv->mmio->handle,
		InterruptControlReg);
	/* if hw gfx's interrupt has not been enabled, then enable it here */
	if (!(interrupt_status & InterruptEnable)) {
		/* enable hw gfx's interrupt */
		#if 0
		setmmioregister(dev_priv->mmio->handle, InterruptControlReg,
			interrupt_status|InterruptEnable);
		#else
		/* temporarily only enable CR's interrupt,
		* disable all others */
		setmmioregister(dev_priv->mmio->handle, InterruptControlReg,
			InterruptEnable);
		#endif
	}

	/*enable agp branch buffer*/
	if ((dev_priv->chip_sub_index == CHIP_H6S1) ||
		(dev_priv->chip_sub_index == CHIP_H6S2)) {
		dwreg69 = 0x01;
		setmmioregister(dev_priv->mmio->handle, INV_REG_CR_TRANS,
			INV_ParaType_PreCR);
		setmmioregister(dev_priv->mmio->handle,
			INV_REG_CR_BEGIN, dwreg69);
	}

	/* drm agp branch buffer init successfully */
	dev_priv->branch_buf_enabled = 1;

	return 0;
}

int via_chrome9_branch_buf_cleanup(struct drm_device *dev,
	struct drm_via_chrome9_init *init, struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_mem *mem = &dev_priv->branch_buf_mem;
	unsigned long interrupt_status;

	/* already cleanuped before, just return here */
	if (!dev_priv->branch_buf_enabled)
		return 0;

	sys_fenceid_reset(dev_priv);
	free_irq(dev->pdev->irq, dev);
#if BRANCH_PRIVATE_WORKQUEUE
	if (dev_priv->branch_buf_wq)
		destroy_workqueue(dev_priv->branch_buf_wq);
#endif
	if (cleanup_branch_buffer_pool(dev)) {
		DRM_ERROR("init_branch_buffer_pool error!.\n");
		return -EINVAL;
	}

	if (via_chrome9_ioctl_freemem_base(dev, mem, file_priv)) {
		DRM_ERROR("Free memory error!.\n");
		return -EINVAL;
	}

	/* query hw gfx's interrupt status */
	interrupt_status = getmmioregister(dev_priv->mmio->handle,
		InterruptControlReg);
	/* if other interrput enable bits are not set,
	 * then disable hw gfx's interrupt */
	if (!(interrupt_status & Interrput_Enable_Mask)) {
		/* disable hw gfx's interrupt */
		setmmioregister(dev_priv->mmio->handle, InterruptControlReg,
			interrupt_status & (~InterruptEnable));
	}

	/* drm agp branch buffer clean up successfully */
	dev_priv->branch_buf_enabled = 0;

	return 0;
}


void via_chrome9_memory_destroy_heap(struct drm_device *dev,
	struct drm_via_chrome9_private *dev_priv)
{
	mutex_lock(&dev->struct_mutex);
	drm_sman_cleanup(&dev_priv->sman);
	dev_priv->vram_initialized = 0;
	dev_priv->agp_initialized = 0;
	mutex_unlock(&dev->struct_mutex);
}

void via_chrome9_reclaim_buffers_locked(struct drm_device *dev,
				struct drm_file *file_priv)
{
	return;
}

int via_chrome9_ioctl_allocate_aperture(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	return 0;
}

int via_chrome9_ioctl_free_aperture(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	return 0;
}


/* Allocate memory from DRM module for video playing */
int via_chrome9_ioctl_allocate_mem_base(struct drm_device *dev,
void *data, struct drm_file *file_priv)
{
	struct drm_via_chrome9_mem *mem = data;
	struct drm_memblock_item *item;
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	unsigned long tmpSize = 0, offset = 0, alignment = 0;
	/* modify heap_type to agp for pcie, since we treat pcie/agp heap
	no difference in heap management */
	if (mem->type == memory_heap_pcie) {
		if (dev_priv->chip_agp != CHIP_PCIE) {
			DRM_ERROR("User want to alloc memory from pcie heap "
			"but via_chrome9.ko has no this heap exist.\n");
			return -EINVAL;
		}
	mem->type = memory_heap_agp;
	}

	if (mem->type > VIA_CHROME9_MEM_AGP) {
		DRM_ERROR("Unknown memory type allocation\n");
		return -EINVAL;
	}
	mutex_lock(&dev->struct_mutex);
	if (0 == ((mem->type == VIA_CHROME9_MEM_VIDEO) ?
		dev_priv->vram_initialized : dev_priv->agp_initialized)) {
		DRM_ERROR("Attempt to allocate from uninitialized"
			"memory manager.\n");
		mutex_unlock(&dev->struct_mutex);
		return -EINVAL;
		}
	tmpSize = (mem->size + VIA_CHROME9_MM_GRANULARITY_MASK) >>
		VIA_CHROME9_MM_GRANULARITY;
	mem->size = tmpSize << VIA_CHROME9_MM_GRANULARITY;
	alignment = (dev_priv->alignment & 0x80000000) ? dev_priv->
	alignment & 0x7FFFFFFF : 0;
	alignment /= (1 << VIA_CHROME9_MM_GRANULARITY);
	item = drm_sman_alloc(&dev_priv->sman, mem->type, tmpSize, alignment,
	(unsigned long)file_priv);
	mutex_unlock(&dev->struct_mutex);
	/* alloc failed */
	if (!item) {
		DRM_ERROR("Allocate memory failed ******ERROR******.\n");
		return -ENOMEM;
	}
	/* Till here every thing is ok, we check the memory type allocated
	and return appropriate value to user mode  Here the value return to
	user is very difficult to operate. BE CAREFULLY!!! */
	/* offset is used by user mode ap to calculate the virtual address
	which is used to access the memory allocated */
	mem->index = item->user_hash.key;
	offset = item->mm->offset(item->mm, item->mm_info) <<
	VIA_CHROME9_MM_GRANULARITY;
	switch (mem->type) {
	case VIA_CHROME9_MEM_VIDEO:
		mem->offset = offset + dev_priv->back_offset;
		break;
	case VIA_CHROME9_MEM_AGP:
		mem->offset = offset;
		break;
	default:
	/* Strange thing happen! Faint. Code bug! */
		DRM_ERROR("Enter here is impossible ******"
		"RROR******.\n");
		return -EINVAL;
	}
	/*DONE. Need we call function copy_to_user ?NO. We can't even
	touch user's space.But we are lucky, since kernel drm:drm_ioctl
	will to the job for us.  */
	return 0;
}

/* Allocate video/AGP/PCIE memory from heap management */
int via_chrome9_ioctl_allocate_mem_wrapper(struct drm_device
	*dev, void *data, struct drm_file *file_priv)
{
	struct drm_via_chrome9_memory_alloc *memory_alloc =
		(struct drm_via_chrome9_memory_alloc *)data;
	struct drm_via_chrome9_private *dev_priv =
		(struct drm_via_chrome9_private *) dev->dev_private;
	struct drm_via_chrome9_mem mem;

	mem.size = memory_alloc->size;
	mem.type = memory_alloc->heap_type;
	dev_priv->alignment = memory_alloc->align | 0x80000000;
	if (via_chrome9_ioctl_allocate_mem_base(dev, &mem, file_priv)) {
		DRM_ERROR("Allocate memory error!.\n");
		return -ENOMEM;
		}
	dev_priv->alignment = 0;
	/* Till here every thing is ok, we check the memory type allocated and
	return appropriate value to user mode Here the value return to user is
	very difficult to operate. BE CAREFULLY!!!*/
	/* offset is used by user mode ap to calculate the virtual address
	which is used to access the memory allocated */
	memory_alloc->offset = mem.offset;
	memory_alloc->heap_info.lpL1Node = (void *)mem.index;
	memory_alloc->size = mem.size;
	switch (memory_alloc->heap_type) {
	case VIA_CHROME9_MEM_VIDEO:
		memory_alloc->physaddress = memory_alloc->offset +
		dev_priv->fb_base_address;
		memory_alloc->linearaddress = (void *)memory_alloc->physaddress;
		break;
	case VIA_CHROME9_MEM_AGP:
		/* return different value to user according to the chip type */
		if (dev_priv->chip_agp == CHIP_PCIE) {
			memory_alloc->physaddress = memory_alloc->offset +
			((struct drm_via_chrome9_dma_manager *)dev_priv->
			dma_manager)->dmasize * sizeof(unsigned long);
			memory_alloc->linearaddress = (void *)memory_alloc->
			physaddress;
		} else {
			memory_alloc->physaddress = dev->agp->base +
			memory_alloc->offset +
			((struct drm_via_chrome9_dma_manager *)
			dev_priv->dma_manager)->dmasize * sizeof(unsigned long);
			memory_alloc->linearaddress =
			(void *)memory_alloc->physaddress;
		}
		break;
	default:
		/* Strange thing happen! Faint. Code bug! */
		DRM_ERROR("Enter here is impossible ******ERROR******.\n");
		return -EINVAL;
	}
	return 0;
}

int via_chrome9_ioctl_free_mem_wrapper(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	struct drm_via_chrome9_memory_alloc *memory_alloc = data;
	struct drm_via_chrome9_mem mem;

	mem.index = (unsigned long)memory_alloc->heap_info.lpL1Node;
	if (via_chrome9_ioctl_freemem_base(dev, &mem, file_priv)) {
		DRM_ERROR("function free_mem_wrapper error.\n");
		return -EINVAL;
	}

	return 0;
}

int via_chrome9_ioctl_freemem_base(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	struct drm_via_chrome9_private *dev_priv = dev->dev_private;
	struct drm_via_chrome9_mem *mem = data;
	int ret;

	mutex_lock(&dev->struct_mutex);
	ret = drm_sman_free_key(&dev_priv->sman, mem->index);
	mutex_unlock(&dev->struct_mutex);
	DRM_DEBUG("free = 0x%lx\n", mem->index);

	return ret;
}

int via_chrome9_ioctl_check_vidmem_size(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	return 0;
}

int via_chrome9_ioctl_pciemem_ctrl(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	int result = 0;
	struct drm_via_chrome9_private *dev_priv = dev->dev_private;
	struct drm_via_chrome9_pciemem_ctrl *pcie_memory_ctrl = data;
	switch (pcie_memory_ctrl->ctrl_type) {
	case pciemem_copy_from_user:
		result = copy_from_user((void *)(
		dev_priv->pcie_vmalloc_nocache +
		pcie_memory_ctrl->pcieoffset +
		((struct drm_via_chrome9_dma_manager *)dev_priv->
		dma_manager)->dmasize * sizeof(unsigned long)),
		pcie_memory_ctrl->usermode_data,
		pcie_memory_ctrl->size);
		break;
	case pciemem_copy_to_user:
		result = copy_to_user(pcie_memory_ctrl->usermode_data,
		(void *)(dev_priv->pcie_vmalloc_nocache +
		pcie_memory_ctrl->pcieoffset +
		((struct drm_via_chrome9_dma_manager *)dev_priv->
		dma_manager)->dmasize * sizeof(unsigned long)),
		pcie_memory_ctrl->size);
		break;
	case pciemem_memset:
		memset((void *)(dev_priv->pcie_vmalloc_nocache +
		pcie_memory_ctrl->pcieoffset +
		((struct drm_via_chrome9_dma_manager *)dev_priv->
		dma_manager)->dmasize * sizeof(unsigned long)),
		pcie_memory_ctrl->memsetdata,
		pcie_memory_ctrl->size);
		break;
	default:
		break;
	}
	return 0;
}

int via_chrome9_branch_buf_request(struct drm_device *dev,
	void *data, struct drm_file *file_priv)
{
	struct umd_branch_buf_rec *p_umd = (struct umd_branch_buf_rec *)data;
	struct kmd_branch_buf_rec *p_kmd;
	struct drm_via_chrome9_private *dev_priv = dev->dev_private;

#if BRANCH_AGP_DEBUG
	DRM_INFO("IN via_chrome9_branch_buf_request :\n");
	DRM_INFO(" free branch buffer num : %x\n",
		dev_priv->freebuf_num_in_prepared_array);
	DRM_INFO("check the counters : request counter:%x, "
		"flush valid counter:%x, flush empty counter:%x, "
		"irq counter:%x, wq bottom half:%x\n",
		dev_priv->branch_requestcount,
		dev_priv->branch_flushcount,
		dev_priv->branch_empty_flushcount,
		dev_priv->branch_irqcount,
		dev_priv->branch_wqcount);
#endif

	while (!(p_kmd =
		get_from_branch_buf_array(dev_priv, 0, PREPARED_BUF_ARRAY))) {
#if BRANCH_AGP_DEBUG
		DRM_INFO(" free branch buffer num : %x\n",
			dev_priv->freebuf_num_in_prepared_array);
		DRM_INFO("can not get free branch buffer,"
			"current process will wait:\n");
#endif

		if (wait_event_interruptible(dev_priv->prepared_array_wait_head,
			(dev_priv->freebuf_num_in_prepared_array > 0)))
			return -ERESTARTSYS;
#if BRANCH_AGP_DEBUG
		DRM_INFO(" free branch buffer num : %x\n",
			dev_priv->freebuf_num_in_prepared_array);
		DRM_INFO("current process is wakeup\n");
#endif

	}
	/* here means that allocation has been successful.
	 * then:
	 * 1) copy the dma buffer info to user mode
	 * 2) add this dma buffer into "allocated" buffer array
	 */
	memcpy(p_umd, p_kmd, sizeof(struct umd_branch_buf_rec));
	add_into_branch_buf_array(dev_priv, p_kmd, ALLOCATED_BUF_ARRAY);

#if BRANCH_AGP_DEBUG
	dev_priv->branch_requestcount++;
	DRM_INFO("request branch buffer succeed: id = %x "
		"status  = %x offset = %x\n",
		p_kmd->id, p_kmd->status, p_kmd->offset);
#endif
	return 0;
}


int via_chrome9_fb_alloc(struct drm_via_chrome9_mem *mem)
{
	struct drm_device *dev = (struct drm_device *)via_chrome9_dev_v4l;
	struct drm_via_chrome9_private *dev_priv;

	if (!dev || !dev->dev_private || !via_chrome9_filepriv_v4l) {
		DRM_ERROR("V4L work before X initialize DRM module !!!\n");
		return -EINVAL;
	}

	dev_priv = (struct drm_via_chrome9_private *)dev->dev_private;
	if (!dev_priv->vram_initialized ||
		mem->type != VIA_CHROME9_MEM_VIDEO) {
		DRM_ERROR("the memory type from V4L is error !!!\n");
		return -EINVAL;
	}

	if (via_chrome9_ioctl_allocate_mem_base(dev,
		mem, via_chrome9_filepriv_v4l)) {
		DRM_ERROR("DRM module allocate memory error for V4L!!!\n");
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(via_chrome9_fb_alloc);

int via_chrome9_fb_free(struct drm_via_chrome9_mem *mem)
{
	struct drm_device *dev = (struct drm_device *)via_chrome9_dev_v4l;
	struct drm_via_chrome9_private *dev_priv;

	if (!dev || !dev->dev_private || !via_chrome9_filepriv_v4l)
		return -EINVAL;

	dev_priv = (struct drm_via_chrome9_private *)dev->dev_private;
	if (!dev_priv->vram_initialized ||
		mem->type != VIA_CHROME9_MEM_VIDEO)
		return -EINVAL;

	if (via_chrome9_ioctl_freemem_base(dev, mem, via_chrome9_filepriv_v4l))
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL(via_chrome9_fb_free);
