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
 * Authors: Super Zhang <super.zhang@intel.com>
 *          John Ye <john.ye@intel.com>
 */

#ifndef _PSB_DETEAR_H_
#define _PSB_DETEAR_H_

#include "drmP.h"
#include "drm.h"
#include "psb_drm.h"
#include "psb_drv.h"

#define VIDEO_BLIT_2D_SIZE 40

typedef struct kern_blit_info
{
	int vdc_bit;
	int cmd_ready;
	unsigned char cmdbuf[40]; /* Video blit 2D cmd size is 40 bytes */
} kern_blit_info;

extern kern_blit_info psb_blit_info;
extern void psb_blit_2d_reg_write(struct drm_psb_private *dev_priv, uint32_t * cmdbuf);

#endif
