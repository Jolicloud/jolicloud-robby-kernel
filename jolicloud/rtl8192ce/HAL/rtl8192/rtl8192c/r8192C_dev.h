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
#ifndef _RTL8192CE_H
#define _RTL8192CE_H

#include "r8192C_def.h"

u8 rtl8192ce_QueryIsShort(u8 TxHT, u8 TxRate, cb_desc *tcb_desc);
bool rtl8192ce_HalTxCheckStuck(struct net_device *dev);
bool rtl8192ce_HalRxCheckStuck(struct net_device *dev);
void rtl8192ce_interrupt_recognized(struct net_device *dev, u32 *p_inta, u32 *p_intb);
void rtl8192ce_enable_rx(struct net_device *dev);
void rtl8192ce_enable_tx(struct net_device *dev);
void rtl8192ce_EnableInterrupt(struct net_device *dev);
void rtl8192ce_DisableInterrupt(struct net_device *dev);
void rtl8192ce_ClearInterrupt(struct net_device *dev);
void rtl8192ce_InitializeVariables(struct net_device  *dev);
void rtl8192ce_get_eeprom_size(struct net_device* dev);
bool rtl8192ce_adapter_start(struct net_device* dev);
void rtl8192ce_link_change(struct net_device *dev);
void rtl8192ce_AllowAllDestAddr(struct net_device* dev, bool bAllowAllDA, bool WriteIntoReg);
void rtl8192ce_tx_fill_desc(struct net_device *dev, tx_desc *pDesc, cb_desc *cb_desc, struct sk_buff *skb);
void rtl8192ce_tx_fill_cmd_desc(struct net_device *dev, tx_desc_cmd *entry, cb_desc *cb_desc, 
		struct sk_buff *skb);
bool rtl8192ce_rx_query_status_desc(struct net_device* dev, struct rtllib_rx_stats*  stats, 
		rx_desc *pdesc, struct sk_buff* skb);
void rtl8192ce_halt_adapter(struct net_device *dev, bool bReset);
void rtl8192ce_UpdateHalRATRTable(struct net_device* dev,u8* pMcsRate,struct sta_info* pEntry);
void rtl8192ce_SetBeaconRelatedRegisters( struct net_device 	*dev);
bool rtl8192ce_GetNmodeSupportBySWSec(struct net_device* dev);
bool rtl8192ce_GetNmodeSupportBySecCfg(struct net_device* dev);
bool rtl8192ce_GetHalfNmodeSupportByAPs(struct net_device* dev);
void rtl8192ce_GetHwReg(struct net_device *dev,u8 variable,u8* val);
void rtl8192ce_SetHwReg(struct net_device *dev,u8 variable,u8* val);
void rtl8192ce_gen_RefreshLedState(struct net_device *dev);
void rtl8192ce_GPIOChangeRFWorkItemCallBack(struct net_device *dev);

void
rtl8192ce_UpdateHalRAMask(
	struct net_device*			dev,
	bool					bMulticast,
	u8					macId,
	u8  					MimoPs,
	u8  					WirelessMode,
	u8  		 			bCurTxBW40MHz,
	u8					rssi_level);
void
rtl8192ce_UpdateBeaconInterruptMask(
	struct net_device*	dev,
	bool			start
	);

void
rtl8192ce_UpdateInterruptMask(
	struct net_device*		dev,
	u32				AddMSR,
	u32				RemoveMSR
	);

void StopTxBeacon(struct net_device* dev);
void ResumeTxBeacon(struct net_device* dev);
void EnableBcnSubFunc(struct net_device* dev);
void DisableBcnSubFunc(struct net_device* dev);

void rtl8192ce_BT_REG_INIT(struct net_device* dev);
void  rtl8192ce_BT_VAR_INIT(struct net_device* dev);
void  rtl8192ce_BT_HW_INIT(struct net_device* dev);
u8  rtl8192ce_BT_FW_PRIVE_NOTIFY(struct net_device* dev);
void  rtl8192ce_BT_SWITCH(struct net_device* dev);
void  rtl8192ce_BT_ServiceChange(struct net_device* dev);
	
#endif 

