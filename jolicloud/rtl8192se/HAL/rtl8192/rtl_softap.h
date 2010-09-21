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
#ifndef RTL_SOFTAP_H
#define RTL_SOFTAP_H

#ifdef ASL
#include <linux/string.h>
#include "../../rtllib/rtllib.h"
#define RTL_IOCTL_SHOSTAP SIOCDEVPRIVATE
#define RTL_IOCTL_CHOSTAP SIOCDEVPRIVATE+1
#define RTL_IOCTL_GHOSTAP SIOCDEVPRIVATE+2
#define RTL_IOCTL_GSEQNUM SIOCDEVPRIVATE+5
#define RTL_IOCTL_SAVE_PID SIOCDEVPRIVATE+6
#define RTL_IOCTL_DEQUEUE SIOCDEVPRIVATE+7
#define RTL_IOCTL_SHIDDENSSID SIOCDEVPRIVATE+8
#define RTL_IOCTL_CHIDDENSSID SIOCDEVPRIVATE+9
#define RTL_IOCTL_SMODE SIOCDEVPRIVATE+11
#define RTL_IOCTL_SHTCONF SIOCDEVPRIVATE+12
#define RTL_IOCTL_STXBW SIOCDEVPRIVATE+13 
#define RTL_IOCTL_START_BN	 SIOCDEVPRIVATE+14
#define RTL_IOCTL_GET_RFTYPE SIOCDEVPRIVATE+15
#define RTL_CUSTOM_TYPE_WRITE 0
#define RTL_CUSTOM_TYPE_READ 1
#define RTL_CUSTOM_SUBTYPE_SET_RSNIE 0
#define RTL_CUSTOM_SUBTYPE_RESET_RSNIE 1
#define RTL_CUSTOM_SUBTYPE_SET_ENC_WEP 2
#define RTL_CUSTOM_SUBTYPE_SET_ENC_TKIP 3
#define RTL_CUSTOM_SUBTYPE_SET_ENC_CKIP 4
#define RTL_CUSTOM_SUBTYPE_SET_ENC_CCMP 5
#define RTL_CUSTOM_SUBTYPE_STASC 6
#define RTL_CUSTOM_SUBTYPE_SET_ASSOC 7
#define RTL_CUSTOM_SUBTYPE_CLEAR_ASSOC 8
#define RTL_CUSTOM_SUBTYPE_SET_WME 9
#define RTL_CUSTOM_SUBTYPE_CLOSE 10 /*close softap*/

struct rtl_packet {
	u8 ether_head[14];
	u8 is_ioctl;
	u16 type;
	u16 subtype;
	u16 length;
	u8 data[0];
} __attribute__ ((packed));
struct rtl_seqnum {
	int idx;
	u8 seqnum[6];
} __attribute__ ((packed));
typedef struct {
	u8				bEnableHT;
	u8				bRegBW40MHz;				
	u8				bRegShortGI40MHz;			
	u8				bRegShortGI20MHz;			
	u8				bRegSuppCCK;				
	u8				bCurTxBW40MHz;	

	u8				SelfHTCap[26];					
	u8				SelfHTInfo[22];					

	u8				bAMSDU_Support;			
	u16				nAMSDU_MaxSize;			
	
	u8				bAMPDUEnable;				
	u8				AMPDU_Factor;				
	u8				MPDU_Density;				

u8					ForcedAMSDUMode;
	u16				ForcedAMSDUMaxSize;

	u8				SelfMimoPs;

	u8				bRegRxReorderEnable;
	u8				RxReorderWinSize;
	u8				RxReorderPendingTime;

	u8				bRegRT2RTAggregation;
	
	u8				Regdot11HTOperationalRateSet[16];		
	
	u8 				OptMode;
}rtl_ht_conf_t, *p_rtl_ht_conf_t;
typedef enum
{
	E_APDEV_OK = 0,
	E_APDEV_ITEM_TOO_LARGE = 1,
	E_APDEV_QFULL = 2,
	E_APDEV_QEMPTY = 3,
	E_APDEV_QNULL = 4		
}WAPI_QUEUE_RET_VAL;
#pragma pack(1)
typedef struct _APDEV_EVENT_T{
    u16	BuffLength;
    u8	Buff[0];
}APDEV_EVENT_T;
#pragma pack()
extern void apdev_init(struct net_device* apdev);
extern int apdev_up(struct net_device *apdev,bool is_silent_reset);
extern void ap_send_to_hostapd(struct rtllib_device *ieee, struct sk_buff *skb);
extern struct sta_info *ap_get_stainfo(struct rtllib_device *ieee, u8 *addr);
extern void ap_ps_on_pspoll(struct rtllib_device *ieee, u16 fc, u8 *pSaddr, struct sta_info *pEntry);
extern struct sta_info *ap_ps_update_station_psstate(struct rtllib_device *ieee, u8 *MacAddr, u8 bPowerSave);
extern int ap_stations_cryptlist_is_empty(struct rtllib_device *ieee);
extern int ap_cryptlist_find_station(struct rtllib_device *ieee,const u8 *addr,u8 justfind);
extern void ap_update_stainfo(struct rtllib_device * ieee,struct rtllib_hdr_4addr *hdr,struct rtllib_rx_stats *rx_stats);
extern u16 ap_ps_fill_tim(struct rtllib_device *ieee);
extern u8 * ap_eid_wme(struct rtllib_device *ieee,u8 *eid);
extern void ap_start_aprequest(struct net_device* dev);
extern void ap_cam_restore_allentry(struct net_device* dev);
extern void ap_set_dataframe_txrate( struct rtllib_device *ieee, struct sta_info *pEntry );
extern void ap_ps_return_allqueuedpackets(struct rtllib_device *rtllib, u8 bMulticastOnly);
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
extern void clear_sta_hw_sec(struct work_struct * work);
#else
extern void clear_sta_hw_sec(struct net_device *dev);
#endif
extern struct net_device_stats *apdev_stats(struct net_device *apdev);
extern int ap_is_data_frame_authorized(struct rtllib_device *ieee, struct net_device *dev, u8* src, u8* dst);
extern int apdev_create_event_send(struct rtllib_device *ieee, u8 *Buff, u16 BufLen);
extern void apdev_init_queue(APDEV_QUEUE * q, int szMaxItem, int szMaxData);
#define APDEV_IsEmptyQueue(q) (q->NumItem==0 ? 1:0)
#define APDEV_IsFullQueue(q) (q->NumItem==q->MaxItem? 1:0)
#define APDEV_NumItemQueue(q) q->NumItem
#endif

#endif
