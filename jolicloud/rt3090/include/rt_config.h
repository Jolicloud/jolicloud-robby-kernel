/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	rt_config.h

	Abstract:
	Central header file to maintain all include files for all NDIS
	miniport driver routines.

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
	Paul Lin    08-01-2002    created

*/
#ifndef	__RT_CONFIG_H__
#define	__RT_CONFIG_H__

#define VENDOR_FEATURE1_SUPPORT
//#define VENDOR_FEATURE2_SUPPORT
//#define VENDOR_FEATURE3_SUPPORT
//#define VENDOR_FEATURE4_SUPPORT

//#define MONITOR_FLAG_11N_SNIFFER_SUPPORT

#define AUTO_RETRY_CNT_LIMIT

#ifdef CONFIG_STA_SUPPORT
//#define AGS_SUPPORT
#endif // CONFIG_STA_SUPPORT //

#ifdef VENDOR_FEATURE3_SUPPORT 
#ifdef DOT1X_SUPPORT
#undef DOT1X_SUPPORT
#endif // DOT1X_SUPPORT //
#ifdef SYSTEM_LOG_SUPPORT
#undef SYSTEM_LOG_SUPPORT
#endif // SYSTEM_LOG_SUPPORT //
#ifdef LED_CONTROL_SUPPORT
#undef LED_CONTROL_SUPPORT
#endif // LED_CONTROL_SUPPORT //
#ifdef WSC_LED_SUPPORT
#undef WSC_LED_SUPPORT
#endif // WSC_LED_SUPPORT //
#endif // VENDOR_FEATURE3_SUPPORT //



#include "rtmp_type.h"
#include "rtmp_os.h"

#include "rtmp_def.h"
#include "rtmp_chip.h"
#include "rtmp_timer.h"


#ifdef LINUX
#ifdef RT_CFG80211_SUPPORT
#include "cfg80211extr.h"
#endif // RT_CFG80211_SUPPORT //
#endif // LINUX //

#ifdef CONFIG_STA_SUPPORT
#ifdef AGS_SUPPORT
#include "ags.h"
#endif // AGS_SUPPORT //
#endif // CONFIG_STA_SUPPORT //

#include "oid.h"
#include "mlme.h"
#include "crypt_md5.h"
#include "crypt_sha2.h"
#include "crypt_hmac.h"
#include "crypt_aes.h"
#include "crypt_arc4.h"
#include "rtmp_cmd.h"
#include "rtmp.h"
#include "ap.h"
#include "wpa.h"
#include "dfs.h"
#include "chlist.h"
#include "spectrum.h"

#include "eeprom.h"
#if defined(RTMP_PCI_SUPPORT) || defined(RTMP_USB_SUPPORT)
#include "rtmp_mcu.h"
#endif



#undef AP_WSC_INCLUDED
#undef STA_WSC_INCLUDED
#undef WSC_INCLUDED



#ifdef CONFIG_STA_SUPPORT
#endif // CONFIG_STA_SUPPORT //

#ifdef BLOCK_NET_IF
#include "netif_block.h"
#endif // BLOCK_NET_IF //

#ifdef IGMP_SNOOP_SUPPORT
#include "igmp_snoop.h"
#endif // IGMP_SNOOP_SUPPORT //

#ifdef RALINK_ATE
#include "rt_ate.h"
#ifdef LINUX
#include <linux/time.h>
#endif // LINUX //
#endif // RALINK_ATE //

#ifdef RALINK_QA
#ifndef RALINK_ATE
#error "For supporting QA GUI, please set HAS_ATE=y and HAS_QA_SUPPORT=y."
#endif // RALINK_ATE //
#endif // RALINK_QA //






#if defined(AP_WSC_INCLUDED) || defined(STA_WSC_INCLUDED)
#define WSC_INCLUDED
#endif


#ifdef CONFIG_STA_SUPPORT
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
#ifndef WPA_SUPPLICANT_SUPPORT
#error "Build for being controlled by NetworkManager or wext, please set HAS_WPA_SUPPLICANT=y and HAS_NATIVE_WPA_SUPPLICANT_SUPPORT=y"
#endif // WPA_SUPPLICANT_SUPPORT //
#endif // NATIVE_WPA_SUPPLICANT_SUPPORT //

#endif // CONFIG_STA_SUPPORT //



#ifdef IKANOS_VX_1X0
#include "vr_ikans.h"
#endif // IKANOS_VX_1X0 //






#ifdef CONFIG_STA_SUPPORT
#include "sta_cfg.h"
#endif // CONFIG_STA_SUPPORT //

#ifdef LINUX
#ifdef RT_CFG80211_SUPPORT
#include "cfg80211.h"
#endif // RT_CFG80211_SUPPORT //

#ifdef BG_FT_SUPPORT
#include "br_ftph.h"
#endif // BG_FT_SUPPORT //
#endif // LINUX //

#include	"misc.h"


#ifdef WLAN_LED
#include	"led.h"
#endif // WLAN_LED //

#endif	// __RT_CONFIG_H__

