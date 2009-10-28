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

#include "psb_detear.h"

kern_blit_info psb_blit_info;

void psb_blit_2d_reg_write(struct drm_psb_private *dev_priv, uint32_t * cmdbuf)
{
	int i;

	for (i = 0; i < VIDEO_BLIT_2D_SIZE; i += 4) {
		PSB_WSGX32(*cmdbuf++, PSB_SGX_2D_SLAVE_PORT + i);
	}
	(void)PSB_RSGX32(PSB_SGX_2D_SLAVE_PORT + i - 4);
}
