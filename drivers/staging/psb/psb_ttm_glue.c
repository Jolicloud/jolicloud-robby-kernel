/**************************************************************************
 * Copyright (c) 2008, Intel Corporation.
 * All Rights Reserved.
 * Copyright (c) 2008, Tungsten Graphics Inc.  Cedar Park, TX., USA.
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
#include "psb_drv.h"
#include "ttm/ttm_userobj_api.h"

static struct vm_operations_struct psb_ttm_vm_ops;

int psb_open(struct inode *inode, struct file *filp)
{
	struct drm_file *file_priv;
	struct drm_psb_private *dev_priv;
	struct psb_fpriv *psb_fp;
	int ret;

	ret = drm_open(inode, filp);
	if (unlikely(ret))
		return ret;

	psb_fp = kcalloc(1, sizeof(*psb_fp), GFP_KERNEL);

	if (unlikely(psb_fp == NULL))
		goto out_err0;

	file_priv = (struct drm_file *) filp->private_data;
	dev_priv = psb_priv(file_priv->minor->dev);


	psb_fp->tfile = ttm_object_file_init(dev_priv->tdev,
					     PSB_FILE_OBJECT_HASH_ORDER);
	if (unlikely(psb_fp->tfile == NULL))
		goto out_err1;

	file_priv->driver_priv = psb_fp;

	if (unlikely(dev_priv->bdev.dev_mapping == NULL))
		dev_priv->bdev.dev_mapping = dev_priv->dev->dev_mapping;

	return 0;

out_err1:
	kfree(psb_fp);
out_err0:
	(void) drm_release(inode, filp);
	return ret;
}

int psb_release(struct inode *inode, struct file *filp)
{
	struct drm_file *file_priv;
	struct psb_fpriv *psb_fp;
	struct drm_psb_private *dev_priv;
	int ret;

	file_priv = (struct drm_file *) filp->private_data;
	psb_fp = psb_fpriv(file_priv);
	dev_priv = psb_priv(file_priv->minor->dev);

	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(file_priv->minor->dev, PSB_DEVICE_SGX);

	ttm_object_file_release(&psb_fp->tfile);
	kfree(psb_fp);

	if (dev_priv && dev_priv->xhw_file)
		psb_xhw_init_takedown(dev_priv, file_priv, 1);

	ret = drm_release(inode, filp);
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev_priv->dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 0);
	return ret;
}

int psb_fence_signaled_ioctl(struct drm_device *dev, void *data,
			     struct drm_file *file_priv)
{
	int ret;
	struct drm_psb_private *dev_priv = psb_priv(dev);
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	ret = ttm_fence_signaled_ioctl(psb_fpriv(file_priv)->tfile, data);
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	return ret;
}

int psb_fence_finish_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv)
{
	int ret;
	struct drm_psb_private *dev_priv = psb_priv(dev);
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	ret = ttm_fence_finish_ioctl(psb_fpriv(file_priv)->tfile, data);
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	return ret;
}

int psb_fence_unref_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	int ret;
	struct drm_psb_private *dev_priv = psb_priv(dev);
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	ret = ttm_fence_unref_ioctl(psb_fpriv(file_priv)->tfile, data);
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	return ret;
}

int psb_pl_waitidle_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	PSB_DEBUG_PM("ioctl: psb_pl_reference\n");
	return ttm_pl_waitidle_ioctl(psb_fpriv(file_priv)->tfile, data);
}

int psb_pl_setstatus_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv)
{
	int ret;
	struct drm_psb_private *dev_priv = psb_priv(dev);
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	ret = ttm_pl_setstatus_ioctl(psb_fpriv(file_priv)->tfile,
				      &psb_priv(dev)->ttm_lock, data);
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	return ret;
}

int psb_pl_synccpu_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *file_priv)
{
	int ret;
	struct drm_psb_private *dev_priv = psb_priv(dev);
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	ret = ttm_pl_synccpu_ioctl(psb_fpriv(file_priv)->tfile, data);
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	return ret;
}

int psb_pl_unref_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	int ret;
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	ret = ttm_pl_unref_ioctl(psb_fpriv(file_priv)->tfile, data);
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	return ret;
}

int psb_pl_reference_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	int ret;
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	ret = ttm_pl_reference_ioctl(psb_fpriv(file_priv)->tfile, data);
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	return ret;
}

int psb_pl_create_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	int ret;
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	ret = ttm_pl_create_ioctl(psb_fpriv(file_priv)->tfile,
				   &dev_priv->bdev, &dev_priv->ttm_lock, data);
	up_read(&dev_priv->sgx_sem);
	if (drm_psb_ospm && IS_MRST(dev))
		schedule_delayed_work(&dev_priv->scheduler.wq, 1);
	return ret;
}

/**
 * psb_ttm_fault - Wrapper around the ttm fault method.
 *
 * @vma: The struct vm_area_struct as in the vm fault() method.
 * @vmf: The struct vm_fault as in the vm fault() method.
 *
 * Since ttm_fault() will reserve buffers while faulting,
 * we need to take the ttm read lock around it, as this driver
 * relies on the ttm_lock in write mode to exclude all threads from
 * reserving and thus validating buffers in aperture- and memory shortage
 * situations.
 */

static int psb_ttm_fault(struct vm_area_struct *vma,
			 struct vm_fault *vmf)
{
	struct ttm_buffer_object *bo = (struct ttm_buffer_object *)
		vma->vm_private_data;
	struct drm_psb_private *dev_priv =
		container_of(bo->bdev, struct drm_psb_private, bdev);
	int ret;

	ret = ttm_read_lock(&dev_priv->ttm_lock, true);
	if (unlikely(ret != 0))
		return VM_FAULT_NOPAGE;

	ret = dev_priv->ttm_vm_ops->fault(vma, vmf);

	ttm_read_unlock(&dev_priv->ttm_lock);
	return ret;
}


int psb_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_file *file_priv;
	struct drm_psb_private *dev_priv;
	int ret;

	if (unlikely(vma->vm_pgoff < DRM_PSB_FILE_PAGE_OFFSET))
		return drm_mmap(filp, vma);

	file_priv = (struct drm_file *) filp->private_data;
	dev_priv = psb_priv(file_priv->minor->dev);

	ret = ttm_bo_mmap(filp, vma, &dev_priv->bdev);
	if (unlikely(ret != 0))
		return ret;

	if (unlikely(dev_priv->ttm_vm_ops == NULL)) {
		dev_priv->ttm_vm_ops = vma->vm_ops;
		psb_ttm_vm_ops = *vma->vm_ops;
		psb_ttm_vm_ops.fault = &psb_ttm_fault;
	}

	vma->vm_ops = &psb_ttm_vm_ops;

	return 0;
}

ssize_t psb_ttm_write(struct file *filp, const char __user *buf,
		      size_t count, loff_t *f_pos)
{
	struct drm_file *file_priv = (struct drm_file *)filp->private_data;
	struct drm_psb_private *dev_priv = psb_priv(file_priv->minor->dev);

	return ttm_bo_io(&dev_priv->bdev, filp, buf, NULL, count, f_pos, 1);
}

ssize_t psb_ttm_read(struct file *filp, char __user *buf,
		     size_t count, loff_t *f_pos)
{
	struct drm_file *file_priv = (struct drm_file *)filp->private_data;
	struct drm_psb_private *dev_priv = psb_priv(file_priv->minor->dev);

	return ttm_bo_io(&dev_priv->bdev, filp, NULL, buf, count, f_pos, 1);
}

int psb_verify_access(struct ttm_buffer_object *bo,
		      struct file *filp)
{
	struct drm_file *file_priv = (struct drm_file *)filp->private_data;

	if (capable(CAP_SYS_ADMIN))
		return 0;

	if (unlikely(!file_priv->authenticated))
		return -EPERM;

	return ttm_pl_verify_access(bo, psb_fpriv(file_priv)->tfile);
}

static int psb_ttm_mem_global_init(struct drm_global_reference *ref)
{
	return ttm_mem_global_init(ref->object);
}

static void psb_ttm_mem_global_release(struct drm_global_reference *ref)
{
	ttm_mem_global_release(ref->object);
}

int psb_ttm_global_init(struct drm_psb_private *dev_priv)
{
	struct drm_global_reference *global_ref;
	int ret;

	global_ref = &dev_priv->mem_global_ref;
	global_ref->global_type = DRM_GLOBAL_TTM_MEM;
	global_ref->size = sizeof(struct ttm_mem_global);
	global_ref->init = &psb_ttm_mem_global_init;
	global_ref->release = &psb_ttm_mem_global_release;

	ret = drm_global_item_ref(global_ref);
	if (unlikely(ret != 0)) {
		DRM_ERROR("Failed referencing a global TTM memory object.\n");
		return ret;
	}

	return 0;
}

void psb_ttm_global_release(struct drm_psb_private *dev_priv)
{
	drm_global_item_unref(&dev_priv->mem_global_ref);
}
