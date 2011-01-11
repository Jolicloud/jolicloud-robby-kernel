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
#ifndef __RTL871X_MLME_H_
#define __RTL871X_MLME_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <wlan_bssdef.h>


#define	MAX_BSS_CNT	64
//#define   MAX_JOIN_TIMEOUT	2500
//#define   MAX_JOIN_TIMEOUT	2000
#define   MAX_JOIN_TIMEOUT	6500

#define 	SCANNING_TIMEOUT 	6000

#ifdef PALTFORM_OS_WINCE
#define	SCANQUEUE_LIFETIME 12000000 // unit:us
#else
#define	SCANQUEUE_LIFETIME 20 // unit:sec
#endif

#define 	WIFI_NULL_STATE		0x00000000
#define	WIFI_ASOC_STATE		0x00000001		// Under Linked state...
#define 	WIFI_REASOC_STATE	       0x00000002
#define	WIFI_SLEEP_STATE	       0x00000004
#define	WIFI_STATION_STATE	0x00000008
#define	WIFI_AP_STATE				0x00000010
#define	WIFI_ADHOC_STATE			0x00000020
#define   WIFI_ADHOC_MASTER_STATE 0x00000040
#define   WIFI_UNDER_LINKING		0x00000080
//#define WIFI_UNDER_CMD			0x00000200
#define WIFI_SITE_MONITOR		0x00000800		//to indicate the station is under site surveying

#ifdef WDS
#define	WIFI_WDS				0x00001000
#define	WIFI_WDS_RX_BEACON	0x00002000		// already rx WDS AP beacon
#endif
#ifdef AUTO_CONFIG
#define	WIFI_AUTOCONF			0x00004000
#define	WIFI_AUTOCONF_IND	0x00008000
#endif

//#ifdef UNDER_MPTEST
#define	WIFI_MP_STATE						0x00010000
#define	WIFI_MP_CTX_BACKGROUND			0x00020000	// in continous tx background
#define	WIFI_MP_CTX_ST					0x00040000	// in continous tx with single-tone
#define	WIFI_MP_CTX_BACKGROUND_PENDING	0x00080000	// pending in continous tx background due to out of skb
#define	WIFI_MP_CTX_CCK_HW				0x00100000	// in continous tx
#define	WIFI_MP_CTX_CCK_CS				0x00200000	// in continous tx with carrier suppression
#define   WIFI_MP_LPBK_STATE				0x00400000
//#endif

//#define _FW_UNDER_CMD		WIFI_UNDER_CMD
#define _FW_UNDER_LINKING	WIFI_UNDER_LINKING
#define _FW_LINKED			WIFI_ASOC_STATE
#define _FW_UNDER_SURVEY	WIFI_SITE_MONITOR

enum dot11AuthAlgrthmNum {
 dot11AuthAlgrthm_Open = 0,
 dot11AuthAlgrthm_Shared,
 dot11AuthAlgrthm_8021X,
 dot11AuthAlgrthm_Auto,
 dot11AuthAlgrthm_MaxNum
};


/*

there are several "locks" in mlme_priv,
since mlme_priv is a shared resource between many threads,
like ISR/Call-Back functions, the OID handlers, and even timer functions.


Each _queue has its own locks, already.
Other items are protected by mlme_priv.lock.

To avoid possible dead lock, any thread trying to modifiying mlme_priv
SHALL not lock up more than one locks at a time!

*/


#define traffic_threshold	10
#define	traffic_scan_period	500

struct sitesurvey_ctrl {
	u64	last_tx_pkts;
	uint	last_rx_pkts;
	sint	traffic_busy;
	_timer	sitesurvey_ctrl_timer;
};

struct mlme_priv {

	_lock	lock;
	sint	fw_state;	//shall we protect this variable? maybe not necessarily...

	u8 to_join; //flag
	u8 *nic_hdl;

	_list		*pscanned;
	_queue	free_bss_pool;
	_queue	scanned_queue;
	u8	*free_bss_buf;
	unsigned long	num_of_scanned;

	NDIS_802_11_SSID	assoc_ssid;
	u8	assoc_bssid[6];

	struct wlan_network	cur_network;

	struct sitesurvey_ctrl sitesurveyctrl;

	//uint wireless_mode; no used, remove it

	u32 scan_interval;


	_timer assoc_timer;

	uint assoc_by_bssid;
	uint assoc_by_rssi;

	_timer scan_to_timer; // driver itself handles scan_timeout status.	

	struct qos_priv qospriv;

#ifdef CONFIG_80211N_HT

	struct ht_priv	htpriv;

#endif

	_timer	dynamic_chk_timer; //dynamic/periodic check timer
 
	u8	ChannelPlan;
 
	u8 	passive_mode; // active: 1, passive: 0
 
};

#ifdef CONFIG_AP_MODE

struct hostapd_priv
{
	_adapter *padapter;
	
#ifdef CONFIG_HOSTAPD_MLME
	struct net_device *pmgnt_netdev;
	struct usb_anchor anchored;
#endif	
	
};

extern int hostapd_mode_init(_adapter *padapter);
extern void hostapd_mode_unload(_adapter *padapter);
#endif

extern void rtw_survey_event_callback(_adapter *adapter, u8 *pbuf);
extern void rtw_surveydone_event_callback(_adapter *adapter, u8 *pbuf);
extern void rtw_joinbss_event_callback(_adapter *adapter, u8 *pbuf);
extern void rtw_stassoc_event_callback(_adapter *adapter, u8 *pbuf);
extern void rtw_stadel_event_callback(_adapter *adapter, u8 *pbuf);
extern void rtw_atimdone_event_callback(_adapter *adapter, u8 *pbuf);
extern void rtw_cpwm_event_callback(_adapter *adapter, u8 *pbuf);

#ifdef PLATFORM_WINDOWS
extern thread_return event_thread(void *context);
extern void sitesurvey_ctrl_handler(
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	);
extern void rtw_join_timeout_handler (
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	);

extern void _rtw_scan_timeout_handler (
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
	);

#endif

#ifdef PLATFORM_LINUX
extern int event_thread(void *context);
extern void sitesurvey_ctrl_handler(void* FunctionContext);
extern void rtw_join_timeout_handler(void* FunctionContext);
extern void _rtw_scan_timeout_handler(void* FunctionContext);
#endif

extern void rtw_free_network_queue(_adapter *adapter,u8 isfreeall);
extern int rtw_init_mlme_priv(_adapter *adapter);// (struct mlme_priv *pmlmepriv);

extern void rtw_free_mlme_priv (struct mlme_priv *pmlmepriv);


extern sint rtw_select_and_join_from_scanned_queue(struct mlme_priv *pmlmepriv);
extern sint rtw_set_key(_adapter *adapter,struct security_priv *psecuritypriv,sint keyid);
extern sint rtw_set_auth(_adapter *adapter,struct security_priv *psecuritypriv);

__inline static u8 *get_bssid(struct mlme_priv *pmlmepriv)
{	//if sta_mode:pmlmepriv->cur_network.network.MacAddress=> bssid
	// if adhoc_mode:pmlmepriv->cur_network.network.MacAddress=> ibss mac address
	return pmlmepriv->cur_network.network.MacAddress;
}

__inline static sint check_fwstate(struct mlme_priv *pmlmepriv, sint state)
{
	if (pmlmepriv->fw_state & state)
		return _TRUE;

	return _FALSE;
}

__inline static sint get_fwstate(struct mlme_priv *pmlmepriv)
{
	return pmlmepriv->fw_state;
}

/*
 * No Limit on the calling context,
 * therefore set it to be the critical section...
 *
 * ### NOTE:#### (!!!!)
 * MUST TAKE CARE THAT BEFORE CALLING THIS FUNC, YOU SHOULD HAVE LOCKED pmlmepriv->lock
 */
__inline static void set_fwstate(struct mlme_priv *pmlmepriv, sint state)
{
	pmlmepriv->fw_state |= state;
}

__inline static void _clr_fwstate_(struct mlme_priv *pmlmepriv, sint state)
{
	pmlmepriv->fw_state &= ~state;
}

/*
 * No Limit on the calling context,
 * therefore set it to be the critical section...
 */
__inline static void clr_fwstate(struct mlme_priv *pmlmepriv, sint state)
{
	_irqL irqL;

	_enter_critical(&pmlmepriv->lock, &irqL);
	if (check_fwstate(pmlmepriv, state) == _TRUE)
		pmlmepriv->fw_state ^= state;
	_exit_critical(&pmlmepriv->lock, &irqL);
}

__inline static void clr_fwstate_ex(struct mlme_priv *pmlmepriv, sint state)
{
	_irqL irqL;

	_enter_critical(&pmlmepriv->lock, &irqL);
	_clr_fwstate_(pmlmepriv, state);
	_exit_critical(&pmlmepriv->lock, &irqL);
}

__inline static void up_scanned_network(struct mlme_priv *pmlmepriv)
{
	_irqL irqL;

	_enter_critical(&pmlmepriv->lock, &irqL);
	pmlmepriv->num_of_scanned++;
	_exit_critical(&pmlmepriv->lock, &irqL);
}

__inline static void down_scanned_network(struct mlme_priv *pmlmepriv)
{
	_irqL irqL;

	_enter_critical(&pmlmepriv->lock, &irqL);
	pmlmepriv->num_of_scanned--;
	_exit_critical(&pmlmepriv->lock, &irqL);
}

__inline static void set_scanned_network_val(struct mlme_priv *pmlmepriv, sint val)
{
	_irqL irqL;

	_enter_critical(&pmlmepriv->lock, &irqL);
	pmlmepriv->num_of_scanned = val;
	_exit_critical(&pmlmepriv->lock, &irqL);
}

extern u16 rtw_get_capability(WLAN_BSSID_EX *bss);
extern void rtw_update_scanned_network(_adapter *adapter, WLAN_BSSID_EX *target);
extern void disconnect_hdl_under_linked(_adapter* adapter, struct sta_info *psta, u8 free_assoc);
extern void rtw_generate_random_ibss(u8 *pibss);
static struct wlan_network* find_network(_queue *scanned_queue, u8 *addr);
extern struct wlan_network* rtw_get_oldest_wlan_network(_queue *scanned_queue);

extern void rtw_free_assoc_resources(_adapter* adapter);
extern void rtw_indicate_disconnect(_adapter* adapter);
extern void rtw_indicate_connect(_adapter* adapter);

extern int rtw_restruct_sec_ie(_adapter *adapter,u8 *in_ie,u8 *out_ie,uint in_len);
extern int rtw_restruct_wmm_ie(_adapter *adapter, u8 *in_ie, u8 *out_ie, uint in_len, uint initial_out_len);
extern void rtw_init_registrypriv_dev_network(_adapter *adapter);

extern void rtw_update_registrypriv_dev_network(_adapter *adapter);

extern void rtw_get_encrypt_decrypt_from_registrypriv(_adapter *adapter);

extern void _rtw_sitesurvey_ctrl_handler(_adapter *adapter);
extern void _rtw_join_timeout_handler(_adapter *adapter);
extern void rtw_scan_timeout_handler(_adapter *adapter);

extern void dynamic_check_timer_handlder(_adapter *adapter);


extern int _rtw_init_mlme_priv(_adapter *padapter);

static void _free_mlme_priv(struct mlme_priv *pmlmepriv);

extern int _rtw_enqueue_network(_queue *queue, struct wlan_network *pnetwork);

extern struct wlan_network* _rtw_dequeue_network(_queue *queue);

extern struct wlan_network* _rtw_alloc_network(struct mlme_priv *pmlmepriv);


extern void _rtw_free_network(struct mlme_priv *pmlmepriv, struct wlan_network *pnetwork,u8 isfreeall);
static void _free_network_nolock(struct mlme_priv *pmlmepriv, struct wlan_network *pnetwork);


extern struct wlan_network* _rtw_find_network(_queue *scanned_queue, u8 *addr);

extern void _rtw_free_network_queue(_adapter* padapter,u8 isfreeall);

extern sint rtw_if_up(_adapter *padapter);


u8 *rtw_get_capability_from_ie(u8 *ie);
u8 *rtw_get_timestampe_from_ie(u8 *ie);
u8 *rtw_get_beacon_interval_from_ie(u8 *ie);


void rtw_joinbss_reset(_adapter *padapter);

#ifdef CONFIG_80211N_HT
unsigned int rtw_restructure_ht_ie(_adapter *padapter, u8 *in_ie, u8 *out_ie, uint in_len, uint *pout_len);
void rtw_update_ht_cap(_adapter *padapter, u8 *pie, uint ie_len);
void rtw_issue_addbareq_cmd(_adapter *padapter, struct xmit_frame *pxmitframe);
#endif


int rtw_is_same_ibss(_adapter *adapter, struct wlan_network *pnetwork);

#endif //__RTL871X_MLME_H_

