/******************************************************************************
 *
 * Copyright(c) 2007 - 2010 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#ifndef __HAL_INIT_H__
#define __HAL_INIT_H__

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>



enum _CHIP_TYPE {

	NULL_CHIP_TYPE,
	RTL8712_8188S_8191S_8192S,
	RTL8188C_8192C,
	RTL8192D,
	MAX_CHIP_TYPE
};

uint rtw_hal_init(_adapter *padapter);
uint rtw_hal_deinit(_adapter *padapter);
void rtw_hal_stop(_adapter *padapter);

#ifdef CONFIG_RTL8711
#include "rtl8711_hal.h"
#endif

#ifdef CONFIG_RTL8712
#include "rtl8712_hal.h"
#endif

#ifdef CONFIG_RTL8192C
#include "rtl8192c_hal.h"
#endif

#endif //__HAL_INIT_H__

