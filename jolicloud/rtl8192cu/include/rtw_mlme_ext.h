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
#ifndef __RTL871X_MLME_EXT_H_
#define __RTL871X_MLME_EXT_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <wlan_bssdef.h>

#define SURVEY_TO			(100)
#define REAUTH_TO			(50)
#define REASSOC_TO		(50)
//#define DISCONNECT_TO	(3000)
#define ADDBA_TO			(2000)

#define LINKED_TO (1) //unit:2 sec, 1x2=2 sec

#define REAUTH_LIMIT	(2)
#define REASSOC_LIMIT	(2)
#define READDBA_LIMIT	(2)

//#define	IOCMD_REG0		0x10250370		 	
//#define	IOCMD_REG1		0x10250374
//#define	IOCMD_REG2		0x10250378

//#define	FW_DYNAMIC_FUN_SWITCH	0x10250364

//#define	WRITE_BB_CMD		0xF0000001
//#define	SET_CHANNEL_CMD	0xF3000000
//#define	UPDATE_RA_CMD	0xFD0000A2

#define	DYNAMIC_FUNC_DISABLE		(0x0)
#define	DYNAMIC_FUNC_DIG			BIT(0)
#define	DYNAMIC_FUNC_HP			BIT(1)
#define	DYNAMIC_FUNC_SS			BIT(2) //Tx Power Tracking
#define DYNAMIC_FUNC_BT			BIT(3)

#define _HW_STATE_NOLINK_		0x00
#define _HW_STATE_ADHOC_		0x01
#define _HW_STATE_STATION_ 	0x02
#define _HW_STATE_AP_			0x03


#define		_1M_RATE_	0
#define		_2M_RATE_	1
#define		_5M_RATE_	2
#define		_11M_RATE_	3
#define		_6M_RATE_	4
#define		_9M_RATE_	5
#define		_12M_RATE_	6
#define		_18M_RATE_	7
#define		_24M_RATE_	8
#define		_36M_RATE_	9
#define		_48M_RATE_	10
#define		_54M_RATE_	11


//
// Channel Plan Type.
// Note: 
//	We just add new channel plan when the new channel plan is different from any of the following 
//	channel plan. 
//	If you just wnat to customize the acitions(scan period or join actions) about one of the channel plan,
//	customize them in RT_CHANNEL_INFO in the RT_CHANNEL_LIST.
// 
typedef enum _RT_CHANNEL_DOMAIN
{
	RT_CHANNEL_DOMAIN_FCC = 0,
	RT_CHANNEL_DOMAIN_IC = 1,
	RT_CHANNEL_DOMAIN_ETSI = 2,
	RT_CHANNEL_DOMAIN_SPAIN = 3,
	RT_CHANNEL_DOMAIN_FRANCE = 4,
	RT_CHANNEL_DOMAIN_MKK = 5,
	RT_CHANNEL_DOMAIN_MKK1 = 6,
	RT_CHANNEL_DOMAIN_ISRAEL = 7,
	RT_CHANNEL_DOMAIN_TELEC = 8,
	RT_CHANNEL_DOMAIN_MIC = 9,				// Be compatible with old channel plan. No good!
	RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN = 10,		// Be compatible with old channel plan. No good!
	RT_CHANNEL_DOMAIN_WORLD_WIDE_13 = 11,		// Be compatible with old channel plan. No good!
	RT_CHANNEL_DOMAIN_TELEC_NETGEAR = 12,		// Be compatible with old channel plan. No good!
	RT_CHANNEL_DOMAIN_NCC = 13,
	//===== Add new channel plan above this line===============//
	RT_CHANNEL_DOMAIN_MAX,
}RT_CHANNEL_DOMAIN, *PRT_CHANNEL_DOMAIN;

typedef struct _RT_CHANNEL_PLAN
{
	u8	Channel[32];
	u8	Len;
}RT_CHANNEL_PLAN, *PRT_CHANNEL_PLAN;


// Scan type including active and passive scan.
typedef enum _RT_SCAN_TYPE
{
	SCAN_ACTIVE,
	SCAN_PASSIVE,
	SCAN_MIX,
}RT_SCAN_TYPE, *PRT_SCAN_TYPE;


enum Associated_AP
{
	atherosAP	= 0,
	broadcomAP	= 1,
	ciscoAP		= 2,
	marvellAP	= 3,
	ralinkAP	= 4,
	realtekAP	= 5,
	airgocapAP 	= 6,
	unknownAP	= 7,
	maxAP,
};


struct mlme_handler {
	unsigned int   num;
	char* str;
	unsigned int (*func)(_adapter *padapter, union recv_frame *precv_frame);
};

struct action_handler {
	unsigned int   num;
	char* str;
	unsigned int (*func)(_adapter *padapter, union recv_frame *precv_frame);
};

struct	ss_res	
{
	int							state;
	int							bss_cnt;
	int							channel_idx;
	int							active_mode;
	int							ss_ssidlen;
	unsigned char 	ss_ssid[IW_ESSID_MAX_SIZE + 1];
};

//#define AP_MODE				0x0C
//#define STATION_MODE	0x08
//#define AD_HOC_MODE		0x04
//#define NO_LINK_MODE	0x00

#define 	WIFI_FW_NULL_STATE			_HW_STATE_NOLINK_
#define	WIFI_FW_STATION_STATE		_HW_STATE_STATION_
#define	WIFI_FW_AP_STATE				_HW_STATE_AP_
#define	WIFI_FW_ADHOC_STATE			_HW_STATE_ADHOC_

#define	WIFI_FW_AUTH_NULL			0x00000100
#define	WIFI_FW_AUTH_STATE			0x00000200
#define	WIFI_FW_AUTH_SUCCESS			0x00000400

#define	WIFI_FW_ASSOC_STATE			0x00002000
#define	WIFI_FW_ASSOC_SUCCESS		0x00004000

#define	WIFI_FW_LINKING_STATE		(WIFI_FW_AUTH_NULL | WIFI_FW_AUTH_STATE | WIFI_FW_ASSOC_STATE)

struct FW_Sta_Info
{
	struct sta_info	*psta;
	unsigned char		status;
	unsigned int		rx_pkt;
	unsigned int		retry;
	NDIS_802_11_RATES_EX  SupportedRates;
};

struct mlme_ext_info
{
	unsigned int		state;
	unsigned int		reauth_count;
	unsigned int		reassoc_count;
	unsigned int		link_count;
	unsigned int		auth_seq;
	unsigned int		auth_algo;	// 802.11 auth, could be open, shared, auto
	unsigned int 		authModeToggle;
	unsigned int		enc_algo;//encrypt algorithm;
	unsigned int		key_index;	// this is only valid for legendary wep, 0~3 for key id.
	unsigned char 		key_mask;
	unsigned int		iv;
	unsigned char		chg_txt[128];
	unsigned short		aid;
	unsigned short	bcn_interval;
	unsigned char		assoc_AP_vendor;
	unsigned char		slotTime;
	unsigned char		ERP_enable;
	unsigned char		ERP_IE;
	unsigned char		turboMode_cts2self;
	unsigned char		turboMode_rtsen;
	unsigned char		WMM_enable;
	unsigned char		HT_enable;
	unsigned char		HT_caps_enable;
	unsigned char		HT_info_enable;
	unsigned char		HT_protection;
	unsigned char		agg_enable_bitmap;
	unsigned char		ADDBA_retry_count;
	unsigned char		candidate_tid_bitmap;
	struct ADDBA_request		ADDBA_req;
	struct WMM_para_element	WMM_param;
	struct HT_caps_element	HT_caps;
	struct HT_info_element	HT_info;
	WLAN_BSSID_EX network;//join network or bss_network, if in ap mode, it is the same to cur_network.network
	struct FW_Sta_Info FW_sta_info[NUM_STA];
	// Accept ADDBA Request
	BOOLEAN				bAcceptAddbaReq;	
};
// The channel information about this channel including joining, scanning, and power constraints.
typedef struct _RT_CHANNEL_INFO
{
	u8				ChannelNum;		// The channel number.
	RT_SCAN_TYPE	ScanType;		// Scan type such as passive or active scan.
	//u16				ScanPeriod;		// Listen time in millisecond in this channel.
	//s32				MaxTxPwrDbm;	// Max allowed tx power.
	//u32				ExInfo;			// Extended Information for this channel.
}RT_CHANNEL_INFO, *PRT_CHANNEL_INFO;

struct mlme_ext_priv
{
	_adapter			*padapter;
	unsigned char		mlmeext_init;
	u8				event_seq;
	unsigned short	mgnt_seq;
	
	//struct fw_priv 	fwpriv;
	
	unsigned char		cur_channel;
	unsigned char		cur_bwmode;
	unsigned char 		cur_ch_offset;//PRIME_CHNL_OFFSET
	unsigned char 		cur_wireless_mode;
	//unsigned char		channel_set[NUM_CHANNELS];
	unsigned char			max_chan_nums;
	RT_CHANNEL_INFO		channel_set[NUM_CHANNELS];
	
	unsigned char		basicrate[NumRates];
	unsigned char		datarate[NumRates];
	
	struct ss_res		sitesurvey_res;		
	struct mlme_ext_info	mlmext_info;//for sta/adhoc mode, including current scanning/connecting/connected related info.
                                                     //for ap mode, network includes ap's cap_info
	_timer		survey_timer;
	_timer		link_timer;
	//_timer		ADDBA_timer;
	u8			chan_scan_time;

	unsigned int	linked_to;//linked timeout
	
	u64 TSFValue;
	
#ifdef CONFIG_AP_MODE	
	unsigned char bstart_bss;
#endif

};

int init_mlme_ext_priv(_adapter* padapter);
void free_mlme_ext_priv (struct mlme_ext_priv *pmlmeext);
extern void init_mlme_ext_timer(_adapter *padapter);
extern void init_addba_retry_timer(_adapter *padapter, struct sta_info *psta);

extern struct xmit_frame *alloc_mgtxmitframe(struct xmit_priv *pxmitpriv);

//void fill_fwpriv(_adapter * padapter, struct fw_priv *pfwpriv);

unsigned char networktype_to_raid(unsigned char network_type);
int judge_network_type(_adapter *padapter, unsigned char *rate, int ratelen);
void get_rate_set(_adapter *padapter, unsigned char *pbssrate, int *bssrate_len);

void Save_DM_Func_Flag(_adapter *padapter);
void Restore_DM_Func_Flag(_adapter *padapter);
void Switch_DM_Func(_adapter *padapter, u8 mode, u8 enable);

void Set_NETYPE1_MSR(_adapter *padapter, u8 type);
void Set_NETYPE0_MSR(_adapter *padapter, u8 type);

void set_channel_bwmode(_adapter *padapter, unsigned char channel, unsigned char channel_offset, unsigned short bwmode);
void SelectChannel(_adapter *padapter, unsigned char channel);
void SetBWMode(_adapter *padapter, unsigned short bwmode, unsigned char channel_offset);

unsigned int decide_wait_for_beacon_timeout(unsigned int bcn_interval);

void write_cam(_adapter *padapter, u8 entry, u16 ctrl, u8 *mac, u8 *key);
void read_cam(_adapter *padapter ,u8 entry);

void invalidate_cam_all(_adapter *padapter);
void CAM_mark_invalid(PADAPTER Adapter, u8 ucIndex);
void CAM_empty_entry(PADAPTER Adapter, u8 ucIndex);


int allocate_cam_entry(_adapter *padapter);
void flush_all_cam_entry(_adapter *padapter);


void site_survey(_adapter *padapter);
u8 collect_bss_info(_adapter *padapter, union recv_frame *precv_frame, WLAN_BSSID_EX *bssid);

int get_bsstype(unsigned short capability);
u8* get_my_bssid(WLAN_BSSID_EX *pnetwork);
u16 get_beacon_interval(WLAN_BSSID_EX *bss);

int is_client_associated_to_ap(_adapter *padapter);
int is_client_associated_to_ibss(_adapter *padapter);
int is_IBSS_empty(_adapter *padapter);

unsigned char check_assoc_AP(u8 *pframe, uint len);

int WMM_param_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs	pIE);
void WMMOnAssocRsp(_adapter *padapter);

void HT_caps_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs pIE);
void HT_info_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs pIE);
void HTOnAssocRsp(_adapter *padapter);

void ERP_IE_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs pIE);
void VCS_update(_adapter *padapter, struct sta_info *psta);

void update_beacon_info(_adapter *padapter, u8 *pframe, uint len, struct sta_info *psta);

void update_IOT_info(_adapter *padapter);
void update_EDCA_param(_adapter *padapter);
int update_sta_support_rate(_adapter *padapter, u8* pvar_ie, uint var_ie_len, int cam_idx);

void Update_RA_Entry(_adapter *padapter, unsigned int mac_id);
void enable_rate_adaptive(_adapter *padapter);
void set_sta_rate(_adapter *padapter);

unsigned int receive_disconnect(_adapter *padapter, unsigned char *MacAddr);

unsigned char get_highest_rate_idx(u32 mask);
int support_short_GI(_adapter *padapter, struct HT_caps_element *pHT_caps);
unsigned int is_ap_in_tkip(_adapter *padapter);


void report_join_res(_adapter *padapter, int res);
void report_survey_event(_adapter *padapter, union recv_frame *precv_frame);
void report_surveydone_event(_adapter *padapter);
void report_del_sta_event(_adapter *padapter, unsigned char* MacAddr);
void report_add_sta_event(_adapter *padapter, unsigned char* MacAddr, int cam_idx);

void beacon_timing_control(_adapter *padapter);
extern u8 set_tx_beacon_cmd(_adapter*padapter);
unsigned int setup_beacon_frame(_adapter *padapter, unsigned char *beacon_frame);
void update_mgntframe_attrib(_adapter *padapter, struct pkt_attrib *pattrib);
void dump_mgntframe(_adapter *padapter, struct xmit_frame *pmgntframe);

void issue_beacon(_adapter *padapter);
void issue_probersp(_adapter *padapter, unsigned char *da);
void issue_assocreq(_adapter *padapter);
void issue_auth(_adapter *padapter, struct sta_info *psta, unsigned short status);
//	Added by Albert 2010/07/26
//	blnbc: 1 -> broadcast probe request
//	blnbc: 0 -> unicast probe request. The address 1 will be the BSSID.
void issue_probereq(_adapter *padapter, u8 blnbc);
void issue_nulldata(_adapter *padapter, unsigned int power_mode);
void issue_deauth(_adapter *padapter, unsigned char *da, unsigned short reason);
void issue_action_BA(_adapter *padapter, unsigned char *raddr, unsigned char action, unsigned short status);
unsigned int send_delba(_adapter *padapter, u8 initiator, u8 *addr);

void start_clnt_assoc(_adapter *padapter);
void start_clnt_auth(_adapter* padapter);
void start_clnt_join(_adapter* padapter);
void start_create_ibss(_adapter* padapter);

unsigned int OnAssocReq(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAssocRsp(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnProbeReq(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnProbeRsp(_adapter *padapter, union recv_frame *precv_frame);
unsigned int DoReserved(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnBeacon(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAtim(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnDisassoc(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAuth(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAuthClient(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnDeAuth(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction(_adapter *padapter, union recv_frame *precv_frame);

unsigned int OnAction_qos(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_dls(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_back(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_p2p(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_ht(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_wmm(_adapter *padapter, union recv_frame *precv_frame);

void mlmeext_joinbss_event_callback(_adapter *padapter);
void mlmeext_sta_del_event_callback(_adapter *padapter);
void mlmeext_sta_add_event_callback(_adapter *padapter, struct sta_info *psta);

void linked_status_chk(_adapter *padapter);

void survey_timer_hdl (_adapter *padapter);
void link_timer_hdl (_adapter *padapter);
void addba_timer_hdl(struct sta_info *psta);
//void reauth_timer_hdl(_adapter *padapter);
//void reassoc_timer_hdl(_adapter *padapter);


extern int cckrates_included(unsigned char *rate, int ratelen);
extern int cckratesonly_included(unsigned char *rate, int ratelen);

extern void process_addba_req(_adapter *padapter, u8 *paddba_req, u8 *addr);

extern void update_TSF(struct mlme_ext_priv *pmlmeext, u8 *pframe, uint len);
extern void correct_TSF(_adapter *padapter, struct mlme_ext_priv *pmlmeext);

extern void ResumeTxBeacon(_adapter *padapter);
extern void StopTxBeacon(_adapter *padapter);

#ifdef CONFIG_AP_MODE
void init_mlme_ap_info(_adapter *padapter);
#ifdef CONFIG_NATIVEAP_MLME
void	expire_timeout_chk(_adapter *padapter);
#endif //end of CONFIG_NATIVEAP_MLME
	
#endif //end of CONFIG_AP_MODE

#endif

