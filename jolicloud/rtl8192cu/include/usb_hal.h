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
#ifndef __USB_HAL_H__
#define __USB_HAL_H__

//u32 rtl8192cu_hal_init(_adapter * adapter);
//u32 rtl8192cu_hal_deinit(_adapter * adapter);

//unsigned int rtl8192cu_inirp_init(_adapter * padapter);
//unsigned int rtl8192cu_inirp_deinit(_adapter * padapter);

//void rtl8192cu_interface_configure(_adapter *padapter);

void rtl8192cu_set_hal_ops(_adapter * padapter);

#endif //__USB_HAL_H__

