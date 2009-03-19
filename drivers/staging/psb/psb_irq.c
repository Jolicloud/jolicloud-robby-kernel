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

#include <drm/drmP.h>
#include "psb_drv.h"
#include "psb_reg.h"
#include "psb_msvdx.h"
#include "lnc_topaz.h"

/*
 * Video display controller interrupt.
 */

static void psb_vdc_interrupt(struct drm_device *dev, uint32_t vdc_stat)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	int wake = 0;

	if (!drm_psb_disable_vsync && (vdc_stat & _PSB_VSYNC_PIPEA_FLAG)) {
#ifdef PSB_FIXME
		atomic_inc(&dev->vbl_received);
#endif
		wake = 1;
		PSB_WVDC32(_PSB_VBLANK_INTERRUPT_ENABLE |
			   _PSB_VBLANK_CLEAR, PSB_PIPEASTAT);
	}

	if (!drm_psb_disable_vsync && (vdc_stat & _PSB_VSYNC_PIPEB_FLAG)) {
#ifdef PSB_FIXME
		atomic_inc(&dev->vbl_received2);
#endif
		wake = 1;
		PSB_WVDC32(_PSB_VBLANK_INTERRUPT_ENABLE |
			   _PSB_VBLANK_CLEAR, PSB_PIPEBSTAT);
	}

	PSB_WVDC32(vdc_stat, PSB_INT_IDENTITY_R);
	(void) PSB_RVDC32(PSB_INT_IDENTITY_R);
	DRM_READMEMORYBARRIER();

#ifdef PSB_FIXME
	if (wake) {
		DRM_WAKEUP(&dev->vbl_queue);
		drm_vbl_send_signals(dev);
	}
#endif
}

/*
 * SGX interrupt source 1.
 */

static void psb_sgx_interrupt(struct drm_device *dev, uint32_t sgx_stat,
			      uint32_t sgx_stat2)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	if (sgx_stat & _PSB_CE_TWOD_COMPLETE) {
		DRM_WAKEUP(&dev_priv->event_2d_queue);
		psb_fence_handler(dev, PSB_ENGINE_2D);
	}

	if (unlikely(sgx_stat2 & _PSB_CE2_BIF_REQUESTER_FAULT))
		psb_print_pagefault(dev_priv);

	psb_scheduler_handler(dev_priv, sgx_stat);
}

/*
 * MSVDX interrupt.
 */
static void psb_msvdx_interrupt(struct drm_device *dev,
				uint32_t msvdx_stat)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	if (msvdx_stat & MSVDX_INTERRUPT_STATUS_CR_MMU_FAULT_IRQ_MASK) {
		/*Ideally we should we should never get to this */
		PSB_DEBUG_IRQ("MSVDX:MMU Fault:0x%x fence2_irq_on=%d\n",
			      msvdx_stat, dev_priv->fence2_irq_on);

		/* Pause MMU */
		PSB_WMSVDX32(MSVDX_MMU_CONTROL0_CR_MMU_PAUSE_MASK,
			     MSVDX_MMU_CONTROL0);
		DRM_WRITEMEMORYBARRIER();

		/* Clear this interupt bit only */
		PSB_WMSVDX32(MSVDX_INTERRUPT_STATUS_CR_MMU_FAULT_IRQ_MASK,
			     MSVDX_INTERRUPT_CLEAR);
		PSB_RMSVDX32(MSVDX_INTERRUPT_CLEAR);
		DRM_READMEMORYBARRIER();

		dev_priv->msvdx_needs_reset = 1;
	} else if (msvdx_stat & MSVDX_INTERRUPT_STATUS_CR_MTX_IRQ_MASK) {
		PSB_DEBUG_IRQ
		    ("MSVDX: msvdx_stat: 0x%x fence2_irq_on=%d(MTX)\n",
		     msvdx_stat, dev_priv->fence2_irq_on);

		/* Clear all interupt bits */
		PSB_WMSVDX32(0xffff, MSVDX_INTERRUPT_CLEAR);
		PSB_RMSVDX32(MSVDX_INTERRUPT_CLEAR);
		DRM_READMEMORYBARRIER();

		psb_msvdx_mtx_interrupt(dev);
	}
}

irqreturn_t psb_irq_handler(DRM_IRQ_ARGS)
{
	struct drm_device *dev = (struct drm_device *) arg;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	uint32_t vdc_stat,msvdx_int = 0, topaz_int = 0;
	uint32_t sgx_stat = 0;
	uint32_t sgx_stat2 = 0;
	uint32_t sgx_int = 0;
	int handled = 0;

	spin_lock(&dev_priv->irqmask_lock);

	vdc_stat = PSB_RVDC32(PSB_INT_IDENTITY_R);

	if (vdc_stat & _PSB_IRQ_SGX_FLAG) {
		PSB_DEBUG_IRQ("Got SGX interrupt\n");
		sgx_int = 1;
	}
	if (vdc_stat & _PSB_IRQ_MSVDX_FLAG) {
		PSB_DEBUG_IRQ("Got MSVDX interrupt\n");
		msvdx_int = 1;
	}

	if (vdc_stat & _LNC_IRQ_TOPAZ_FLAG) {
		PSB_DEBUG_IRQ("Got TOPAX interrupt\n");
		topaz_int = 1;
	}
	if (sgx_int && (dev_priv->graphics_state == PSB_PWR_STATE_D0i0)) {
		sgx_stat = PSB_RSGX32(PSB_CR_EVENT_STATUS);
		sgx_stat2 = PSB_RSGX32(PSB_CR_EVENT_STATUS2);

		sgx_stat2 &= dev_priv->sgx2_irq_mask;
		sgx_stat &= dev_priv->sgx_irq_mask;
		PSB_WSGX32(sgx_stat2, PSB_CR_EVENT_HOST_CLEAR2);
		PSB_WSGX32(sgx_stat, PSB_CR_EVENT_HOST_CLEAR);
		(void) PSB_RSGX32(PSB_CR_EVENT_HOST_CLEAR);
	} else if (unlikely(PSB_D_PM & drm_psb_debug)) {
		if (sgx_int)
			PSB_DEBUG_PM("sgx int in down mode\n");
	}
	vdc_stat &= dev_priv->vdc_irq_mask;
	spin_unlock(&dev_priv->irqmask_lock);

	if (msvdx_int) {
		uint32_t msvdx_stat = 0;

		msvdx_stat = PSB_RMSVDX32(MSVDX_INTERRUPT_STATUS);
		psb_msvdx_interrupt(dev, msvdx_stat);
		handled = 1;
	}

	if (IS_MRST(dev) && topaz_int) {
		uint32_t topaz_stat = 0;

		TOPAZ_READ32(TOPAZ_CR_IMG_TOPAZ_INTSTAT,&topaz_stat);
		lnc_topaz_interrupt (dev, topaz_stat);
		handled = 1;
	}

	if (vdc_stat) {
		/* MSVDX IRQ status is part of vdc_irq_mask */
		psb_vdc_interrupt(dev, vdc_stat);
		handled = 1;
	}

	if (sgx_stat || sgx_stat2) {

		psb_sgx_interrupt(dev, sgx_stat, sgx_stat2);
		handled = 1;
	}

	if (!handled)
		return IRQ_NONE;


	return IRQ_HANDLED;
}

void psb_msvdx_irq_preinstall(struct drm_psb_private *dev_priv)
{
	unsigned long mtx_int = 0;
	dev_priv->vdc_irq_mask |= _PSB_IRQ_MSVDX_FLAG;

	/* Clear MTX interrupt */
	REGIO_WRITE_FIELD_LITE(mtx_int, MSVDX_INTERRUPT_STATUS, CR_MTX_IRQ,
			       1);
	PSB_WMSVDX32(mtx_int, MSVDX_INTERRUPT_CLEAR);
}

void psb_irq_preinstall(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long mtx_int = 0;
	unsigned long irqflags;
	PSB_DEBUG_PM("psb_irq_preinstall\n");

	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	PSB_WVDC32(0xFFFFFFFF, PSB_HWSTAM);
	PSB_WVDC32(0x00000000, PSB_INT_MASK_R);
	PSB_WVDC32(0x00000000, PSB_INT_ENABLE_R);
	PSB_WSGX32(0x00000000, PSB_CR_EVENT_HOST_ENABLE);
	(void) PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE);

	dev_priv->sgx_irq_mask = _PSB_CE_PIXELBE_END_RENDER |
	    _PSB_CE_DPM_3D_MEM_FREE |
	    _PSB_CE_TA_FINISHED |
	    _PSB_CE_DPM_REACHED_MEM_THRESH |
	    _PSB_CE_DPM_OUT_OF_MEMORY_GBL |
	    _PSB_CE_DPM_OUT_OF_MEMORY_MT |
	    _PSB_CE_TA_TERMINATE | _PSB_CE_SW_EVENT;

	dev_priv->sgx2_irq_mask = _PSB_CE2_BIF_REQUESTER_FAULT;

	dev_priv->vdc_irq_mask = _PSB_IRQ_SGX_FLAG | _PSB_IRQ_MSVDX_FLAG;

	if (!drm_psb_disable_vsync)
		dev_priv->vdc_irq_mask |= _PSB_VSYNC_PIPEA_FLAG |
		    _PSB_VSYNC_PIPEB_FLAG;

	/* Clear MTX interrupt */
	REGIO_WRITE_FIELD_LITE(mtx_int, MSVDX_INTERRUPT_STATUS,
			       CR_MTX_IRQ, 1);
	PSB_WMSVDX32(mtx_int, MSVDX_INTERRUPT_CLEAR);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
	up_read(&dev_priv->sgx_sem);
}

void psb_msvdx_irq_postinstall(struct drm_psb_private *dev_priv)
{
	/* Enable Mtx Interupt to host */
	unsigned long enables = 0;
	PSB_DEBUG_GENERAL("Setting up MSVDX IRQs.....\n");
	REGIO_WRITE_FIELD_LITE(enables, MSVDX_INTERRUPT_STATUS, CR_MTX_IRQ,
			       1);
	PSB_WMSVDX32(enables, MSVDX_HOST_INTERRUPT_ENABLE);
}

int psb_irq_postinstall(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;
	unsigned long enables = 0;

	PSB_DEBUG_PM("psb_irq_postinstall\n");
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
	PSB_WSGX32(dev_priv->sgx2_irq_mask, PSB_CR_EVENT_HOST_ENABLE2);
	PSB_WSGX32(dev_priv->sgx_irq_mask, PSB_CR_EVENT_HOST_ENABLE);
	(void) PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE);

	/* MSVDX IRQ Setup, Enable Mtx Interupt to host */
	PSB_DEBUG_GENERAL("Setting up MSVDX IRQs.....\n");
	REGIO_WRITE_FIELD_LITE(enables, MSVDX_INTERRUPT_STATUS,
			       CR_MTX_IRQ, 1);
	PSB_WMSVDX32(enables, MSVDX_HOST_INTERRUPT_ENABLE);

	dev_priv->irq_enabled = 1;
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
	up_read(&dev_priv->sgx_sem);
	return 0;
}

void psb_irq_uninstall(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;
	PSB_DEBUG_PM("psb_irq_uninstall\n");
	down_read(&dev_priv->sgx_sem);
	psb_check_power_state(dev, PSB_DEVICE_SGX);
	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	dev_priv->sgx_irq_mask = 0x00000000;
	dev_priv->sgx2_irq_mask = 0x00000000;
	dev_priv->vdc_irq_mask = 0x00000000;

	PSB_WVDC32(0xFFFFFFFF, PSB_HWSTAM);
	PSB_WVDC32(0xFFFFFFFF, PSB_INT_MASK_R);
	PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
	PSB_WSGX32(dev_priv->sgx_irq_mask, PSB_CR_EVENT_HOST_ENABLE);
	PSB_WSGX32(dev_priv->sgx2_irq_mask, PSB_CR_EVENT_HOST_ENABLE2);
	wmb();
	PSB_WVDC32(PSB_RVDC32(PSB_INT_IDENTITY_R), PSB_INT_IDENTITY_R);
	PSB_WSGX32(PSB_RSGX32(PSB_CR_EVENT_STATUS),
		   PSB_CR_EVENT_HOST_CLEAR);
	PSB_WSGX32(PSB_RSGX32(PSB_CR_EVENT_STATUS2),
		   PSB_CR_EVENT_HOST_CLEAR2);

	/* MSVDX IRQ Setup */
	/* Clear interrupt enabled flag */
	PSB_WMSVDX32(0, MSVDX_HOST_INTERRUPT_ENABLE);

	if (IS_MRST(dev))
		TOPAZ_WRITE32(TOPAZ_CR_IMG_TOPAZ_INTENAB, 0);

	dev_priv->irq_enabled = 0;
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
	up_read(&dev_priv->sgx_sem);
}

void psb_2D_irq_off(struct drm_psb_private *dev_priv)
{
	unsigned long irqflags;
	uint32_t old_mask;
	uint32_t cleared_mask;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);
	--dev_priv->irqen_count_2d;
	if (dev_priv->irq_enabled && dev_priv->irqen_count_2d == 0) {

		old_mask = dev_priv->sgx_irq_mask;
		dev_priv->sgx_irq_mask &= ~_PSB_CE_TWOD_COMPLETE;
		PSB_WSGX32(dev_priv->sgx_irq_mask,
			   PSB_CR_EVENT_HOST_ENABLE);
		(void) PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE);

		cleared_mask =
		    (old_mask ^ dev_priv->sgx_irq_mask) & old_mask;
		PSB_WSGX32(cleared_mask, PSB_CR_EVENT_HOST_CLEAR);
		(void) PSB_RSGX32(PSB_CR_EVENT_HOST_CLEAR);
	}
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

void psb_2D_irq_on(struct drm_psb_private *dev_priv)
{
	unsigned long irqflags;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);
	if (dev_priv->irq_enabled && dev_priv->irqen_count_2d == 0) {
		dev_priv->sgx_irq_mask |= _PSB_CE_TWOD_COMPLETE;
		PSB_WSGX32(dev_priv->sgx_irq_mask,
			   PSB_CR_EVENT_HOST_ENABLE);
		(void) PSB_RSGX32(PSB_CR_EVENT_HOST_ENABLE);
	}
	++dev_priv->irqen_count_2d;
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

#ifdef PSB_FIXME
static int psb_vblank_do_wait(struct drm_device *dev,
			      unsigned int *sequence, atomic_t *counter)
{
	unsigned int cur_vblank;
	int ret = 0;
	DRM_WAIT_ON(ret, dev->vbl_queue, 3 * DRM_HZ,
		    (((cur_vblank = atomic_read(counter))
		      - *sequence) <= (1 << 23)));
	*sequence = cur_vblank;

	return ret;
}
#endif

void psb_msvdx_irq_off(struct drm_psb_private *dev_priv)
{
	unsigned long irqflags;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);
	if (dev_priv->irq_enabled) {
		dev_priv->vdc_irq_mask &= ~_PSB_IRQ_MSVDX_FLAG;
		PSB_WSGX32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
		(void) PSB_RSGX32(PSB_INT_ENABLE_R);
	}
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

void psb_msvdx_irq_on(struct drm_psb_private *dev_priv)
{
	unsigned long irqflags;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);
	if (dev_priv->irq_enabled) {
		dev_priv->vdc_irq_mask |= _PSB_IRQ_MSVDX_FLAG;
		PSB_WSGX32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
		(void) PSB_RSGX32(PSB_INT_ENABLE_R);
	}
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}
