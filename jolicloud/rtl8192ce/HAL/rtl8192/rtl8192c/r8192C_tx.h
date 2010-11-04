/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * Based on the r8180 driver, which is:
 * Copyright 2004-2005 Andrea Merello <andreamrl@tiscali.it>, et al.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
******************************************************************************/
#ifndef __INC_HAL8192C_RX_H
#define __INC_HAL8192C_RX_H

u8 rtl8192ce_MRateToHwRate(
	struct net_device*dev, 
	u8 rate
	);

u8 rtl8192ce_QueryIsShort(
	u8 TxHT, 
	u8 TxRate, 
	cb_desc *tcb_desc
	);

u8 rtl8192ce_MapHwQueueToFirmwareQueue(
	u8 QueueID, 
	u8 priority
	);

void  rtl8192ce_tx_fill_desc(
	struct net_device* dev, 
	tx_desc * pDesc_tx, 
	cb_desc * cb_desc, 
	struct sk_buff* skb
	);

void  rtl8192ce_tx_fill_cmd_desc(
	struct net_device* dev, 
	tx_desc_cmd * entry, 
	cb_desc * cb_desc, 
	struct sk_buff* skb
	);


#endif

