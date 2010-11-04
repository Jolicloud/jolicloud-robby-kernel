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

u8 rtl8192ce_HwRateToMRate(
	bool bIsHT,	
	u8 rate
	);
			 
void rtl8192ce_UpdateReceivedRateHistogramStatistics(
	struct net_device *dev,
	struct rtllib_rx_stats* pstats
	);
void rtl8192ce_UpdateRxPktTimeStamp (
	struct net_device *dev, 
	struct rtllib_rx_stats *stats);

long rtl8192ce_signal_scale_mapping(
	struct r8192_priv * priv,
	long currsig
	);

void rtl8192ce_query_rxphystatus(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pstats,
	prx_desc  pdesc,	
	prx_fwinfo   pDrvInfo,
	struct rtllib_rx_stats * precord_stats,
	bool bpacket_match_bssid,
	bool bpacket_toself,
	bool bPacketBeacon,
	bool bToSelfBA
	);


void rtl8192ce_update_rxsignalstatistics(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pstats
	);

void Process_UI_RSSI_8192ce(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pstats);

void Process_PWDB_8192ce(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pstats,
	struct rtllib_network* pnet, 
	struct sta_info *pEntry
	);


void Process_UiLinkQuality8192ce(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pstats
	);

void rtl8192ce_process_phyinfo(
	struct r8192_priv * priv, u8* buffer,
	struct rtllib_rx_stats * pcurrent_stats,
	struct rtllib_network * pnet, 
	struct sta_info *pEntry
	);


void rtl8192ce_TranslateRxSignalStuff(
	struct net_device *dev, 
        struct sk_buff *skb,
        struct rtllib_rx_stats * pstats,
        prx_desc pdesc,	
        prx_fwinfo pdrvinfo);

bool rtl8192ce_rx_query_status_desc(
	struct net_device* dev, 
	struct rtllib_rx_stats*  stats,
	rx_desc *pdesc, 
	struct sk_buff* skb);


#endif

