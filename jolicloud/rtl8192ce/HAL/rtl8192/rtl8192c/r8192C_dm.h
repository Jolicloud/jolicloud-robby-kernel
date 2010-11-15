/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
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

#ifndef	__HAL8192CPCIDM_H__
#define __HAL8192CPCIDM_H__

#define init_hal_dm				InitHalDm
#define deinit_hal_dm				DeInitHalDm
#define hal_dm_watchdog				HalDmWatchDog
#define dm_txpower_trackingcallback 		dm_TXPowerTrackingCallback_Dummy
#define dm_init_edca_turbo			DM_InitEdcaTurbo
#define dm_rf_pathcheck_workitemcallback	DM_RFPathCheckWorkItemCallBack
#define dm_backup_dynamic_mechanism_state	DM_BackupDynamicMechanismState

#define OFDM_Table_Length 37 
#define CCK_Table_length 33 

typedef struct _Dynamic_Power_Saving_
{
	u8		PreCCAState;
	u8		CurCCAState;

	u8		PreRFState;
	u8		CurRFState;

	long		Rssi_val_min;
	
}PS_T;

typedef struct _Dynamic_Initial_Gain_Threshold_
{
	u8		Dig_Enable_Flag;
	u8		Dig_Ext_Port_Stage;
	
	u32		RssiLowThresh;
	u32		RssiHighThresh;

	u32		FALowThresh;
	u32		FAHighThresh;

	u8		CurSTAConnectState;
	u8		PreSTAConnectState;
	u8		CurMultiSTAConnectState;

	u8		PreIGValue;
	u8		CurIGValue;

	char		BackoffVal;
	char		BackoffVal_range_max;
	char		BackoffVal_range_min;
	u8		rx_gain_range_max;
	u8		rx_gain_range_min;
	u8		Rssi_val_min;

	u8		PreCCKPDState;
	u8		CurCCKPDState;

	u8		PreCCKFAState;
	u8		CurCCKFAState;
	
	u8		PreCCAState;
	u8		CurCCAState;
	
}dig_t, DIG_T;

typedef struct _SW_Antenna_Switch_
{
	u8		failure_cnt;
	u8		try_flag;
	u8		stop_trying;
	long		PreRSSI;
	long		Trying_Threshold;
	u8		CurAntenna;
	u8		PreAntenna;

}SWAT_T;

typedef enum tag_Dynamic_Init_Gain_Operation_Type_Definition
{
	DIG_TYPE_THRESH_HIGH	= 0,
	DIG_TYPE_THRESH_LOW	= 1,
	DIG_TYPE_BACKOFF		= 2,
	DIG_TYPE_RX_GAIN_MIN	= 3,
	DIG_TYPE_RX_GAIN_MAX	= 4,
	DIG_TYPE_ENABLE 		= 5,
	DIG_TYPE_DISABLE 		= 6,
	DIG_OP_TYPE_MAX
}dm_dig_op_e, DM_DIG_OP_E;

typedef enum tag_CCK_Packet_Detection_Threshold_Type_Definition
{
	CCK_PD_STAGE_LowRssi = 0,
	CCK_PD_STAGE_HighRssi = 1,
	CCK_FA_STAGE_Low = 2,
	CCK_FA_STAGE_High = 3,
	CCK_PD_STAGE_MAX = 4,
}DM_CCK_PDTH_E;

typedef enum tag_1R_CCA_Type_Definition
{
	CCA_1R =0,
	CCA_2R = 1,
	CCA_MAX = 2,
}DM_1R_CCA_E;

typedef enum tag_RF_Type_Definition
{
	RF_Save =0,
	RF_Normal = 1,
	RF_MAX = 2,
}DM_RF_E;

typedef enum tag_SW_Antenna_Switch_Definition
{
	Antenna_B = 1,
	Antenna_A = 2,
	Antenna_MAX = 3,
}DM_SWAS_E;

typedef enum tag_DIG_EXT_PORT_ALGO_Definition
{
	DIG_EXT_PORT_STAGE_0 = 0,
	DIG_EXT_PORT_STAGE_1 = 1,
	DIG_EXT_PORT_STAGE_2 = 2,
	DIG_EXT_PORT_STAGE_3 = 3,
	DIG_EXT_PORT_STAGE_MAX = 4,
}DM_DIG_EXT_PORT_ALG_E;


typedef enum tag_DIG_Connect_Definition
{
	DIG_STA_DISCONNECT = 0,	
	DIG_STA_CONNECT = 1,
	DIG_STA_BEFORE_CONNECT = 2,
	DIG_MultiSTA_DISCONNECT = 3,
	DIG_MultiSTA_CONNECT = 4,
	DIG_CONNECT_MAX
}dm_dig_connect_e, DM_DIG_CONNECT_E;

#define	OFDM_TABLE_SIZE 	37
#define	CCK_TABLE_SIZE		33


#define		BW_AUTO_SWITCH_HIGH_LOW		25
#define		BW_AUTO_SWITCH_LOW_HIGH		30

#define		DM_DIG_THRESH_HIGH			40
#define		DM_DIG_THRESH_LOW			35

#define		DM_FALSEALARM_THRESH_LOW		400
#define		DM_FALSEALARM_THRESH_HIGH	1000

#define		DM_DIG_MAX						0x3e
#define		DM_DIG_MIN						0x1e 

#define		DM_DIG_FA_UPPER				0x32
#define		DM_DIG_FA_LOWER				0x20
#define		DM_DIG_FA_TH0					0x20
#define		DM_DIG_FA_TH1					0x100
#define		DM_DIG_FA_TH2					0x200

#define		DM_DIG_BACKOFF_MAX			12
#define		DM_DIG_BACKOFF_MIN			-4
#define		DM_DIG_BACKOFF_DEFAULT		10

#define		RxPathSelection_SS_TH_low		30
#define		RxPathSelection_diff_TH			18

#define		DM_RATR_STA_INIT				0
#define		DM_RATR_STA_HIGH				1
#define 		DM_RATR_STA_MIDDLE			2
#define 		DM_RATR_STA_LOW				3

#define		CTSToSelfTHVal					30
#define		RegC38_TH						20

#define		WAIotTHVal						25

#define		TX_POWER_NEAR_FIELD_THRESH_LVL2	74
#define		TX_POWER_NEAR_FIELD_THRESH_LVL1	67

#define		TxHighPwrLevel_Normal			0	
#define		TxHighPwrLevel_Level1			1
#define		TxHighPwrLevel_Level2			2

#define		DM_Type_ByFW					0
#define		DM_Type_ByDriver				1

/*------------------------Export global variable----------------------------*/
extern	DIG_T	DM_DigTable;
/*------------------------Export global variable----------------------------*/


/*------------------------Export Marco Definition---------------------------*/

#define DM_MultiSTA_InitGainChangeNotify(Event) {DM_DigTable.CurMultiSTAConnectState = Event;}
/*------------------------Export Marco Definition---------------------------*/


void InitHalDm(struct net_device *dev);
void DeInitHalDm(struct net_device *dev);
void HalDmWatchDog(struct net_device *dev);
void dm_DIGInit(struct net_device *dev);
void DM_Write_DIG(struct net_device *dev);
void DM_ChangeDynamicInitGainThresh(struct net_device *dev,
		u32		DM_Type,
		u32		DM_Value);
void dm_TXPowerTrackingCallback_Dummy(void *data);
void DM_InitEdcaTurbo(struct net_device *dev);
void DM_RFPathCheckWorkItemCallBack(void *data);
void DM_BackupDynamicMechanismState(struct net_device *dev);
void dm_CheckTXPowerTracking(struct net_device *dev);
void dm_InitRateAdaptiveMask(struct net_device *dev);
void DM_TXPowerTracking92CDirectCall(struct net_device *dev);
#endif	


