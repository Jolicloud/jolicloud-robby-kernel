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
#ifndef __RTL8192C_CMD_H_
#define __RTL8192C_CMD_H_


enum cmd_msg_element_id
{	
	NONE_CMDMSG_EID,
	AP_OFFLOAD_EID=0,
	SET_PWRMODE_EID=1,
	JOINBSS_RPT_EID=2,
	RSVD_PAGE_EID=3,
	RSSI_4_EID = 4,
	RSSI_SETTING_EID=5,
	MACID_CONFIG_EID=6,
	MACID_PS_MODE_EID=7,
	P2P_PS_OFFLOAD_EID=8,
	SELECTIVE_SUSPEND_ROF_CMD=9,
	MAX_CMDMSG_EID	 
};

struct cmd_msg_parm {
	u8 eid; //element id
	u8 sz; // sz
	u8 buf[6];
};

#ifdef CONFIG_LPS
enum LPS_CTRL_TYPE
{
	LPS_CTRL_SCAN=0,
	LPS_CTRL_JOINBSS=1,
	LPS_CTRL_CONNECT=2,
	LPS_CTRL_DISCONNECT=3,
	LPS_CTRL_SPECIAL_PACKET=4,
};

typedef struct _SETPWRMODE_PARM{
	u8 	Mode;
	u8 	SmartPS;
	unsigned char  AwakeInterval; //Unit: beacon interval, this field is only valid in PS_DTIM mode
}SETPWRMODE_PARM, *PSETPWRMODE_PARM;

typedef struct JOINBSSRPT_PARM{
	u8	OpMode;	// RT_MEDIA_STATUS
}JOINBSSRPT_PARM, *PJOINBSSRPT_PARM;

typedef struct _RSVDPAGE_LOC{
	u8 	LocProbeRsp;
	u8 	LocPsPoll;
	u8	LocNullData;
}RSVDPAGE_LOC, *PRSVDPAGE_LOC;

void lps_ctrl_wk_hdl(_adapter *padapter, u8 *pbuf, int sz);
u8 lps_ctrl_wk_cmd(_adapter*padapter, u8 lps_ctrl_type, u8 enqueue);
void set_FwPwrMode_cmd(_adapter*padapter, u8 Mode);
void set_FwJoinBssReport_cmd(_adapter* padapter, u8 mstatus);
#endif

#ifdef CONFIG_AUTOSUSPEND
#ifdef SUPPORT_HW_RFOFF_DETECTED
struct H2C_SS_RFOFF_PARAM{
	u8 	ROFOn; // 1: on, 0:off
	u16	gpio_period; // unit: 1024 us
}__attribute__ ((packed));
u8 set_FWSelectSuspend_cmd(_adapter*padapter,u8 bfwpoll, u16 period);
#endif
#endif

#ifdef CONFIG_ANTENNA_DIVERSITY
void antenna_select_wk_hdl(_adapter *padapter, u8 *pbuf, int antenna);
u8 antenna_select_cmd(_adapter*padapter, u8 antenna, u8 enqueue);
#endif

// host message to firmware cmd
u8 set_rssi_cmd(_adapter*padapter, u8 *param);
u8 set_raid_cmd(_adapter*padapter, u32 mask, u8 arg);


/* these calls are synchronous, and may not be used in an interrupt context.*/
u32 read_macreg(_adapter *padapter, u32 addr, u32 sz);
void write_macreg(_adapter *padapter, u32 addr, u32 val, u32 sz);
u32 read_bbreg(_adapter *padapter, u32 addr, u32 bitmask);
void write_bbreg(_adapter *padapter, u32 addr, u32 bitmask, u32 val);
u32 read_rfreg(_adapter *padapter, u8 rfpath, u32 addr, u32 bitmask);
void write_rfreg(_adapter *padapter, u8 rfpath, u32 addr, u32 bitmask, u32 val);

/* these calls are asynchronous, and can be used in an interrupt context.*/
//u32 read_macreg_cmd(_adapter *padapter, u32 addr, u32 sz);
void write_macreg_cmd(_adapter *padapter, u32 addr, u32 val, u32 sz);
//u32 read_bbreg_cmd(_adapter *padapter, u32 addr, u32 bitmask);
void write_bbreg_cmd(_adapter *padapter, u32 addr, u32 bitmask, u32 val);
//u32 read_rfreg_cmd(_adapter *padapter, u8 rfpath, u32 addr, u32 bitmask);
void write_rfreg_cmd(_adapter *padapter, u8 rfpath, u32 addr, u32 bitmask, u32 val);

enum rtl8192c_h2c_cmd
{
	GEN_CMD_CODE(_Read_MACREG) ,	/*0*/
 	GEN_CMD_CODE(_Write_MACREG) ,    
 	GEN_CMD_CODE(_Read_BBREG) ,  
 	GEN_CMD_CODE(_Write_BBREG) ,  
 	GEN_CMD_CODE(_Read_RFREG) ,  
 	GEN_CMD_CODE(_Write_RFREG) , /*5*/
 	GEN_CMD_CODE(_Read_EEPROM) ,  
 	GEN_CMD_CODE(_Write_EEPROM) ,  
 	GEN_CMD_CODE(_Read_EFUSE) ,  
 	GEN_CMD_CODE(_Write_EFUSE) , 
 	
 	GEN_CMD_CODE(_Read_CAM) ,	/*10*/
 	GEN_CMD_CODE(_Write_CAM) ,   
 	GEN_CMD_CODE(_setBCNITV),
 	GEN_CMD_CODE(_setMBIDCFG),
 	GEN_CMD_CODE(_JoinBss),   /*14*/
 	GEN_CMD_CODE(_DisConnect) , /*15*/
 	GEN_CMD_CODE(_CreateBss) ,
	GEN_CMD_CODE(_SetOpMode) , 
	GEN_CMD_CODE(_SiteSurvey),  /*18*/
 	GEN_CMD_CODE(_SetAuth) ,
 	
 	GEN_CMD_CODE(_SetKey) ,	/*20*/
 	GEN_CMD_CODE(_SetStaKey) ,
 	GEN_CMD_CODE(_SetAssocSta) ,
 	GEN_CMD_CODE(_DelAssocSta) ,
 	GEN_CMD_CODE(_SetStaPwrState) , 
 	GEN_CMD_CODE(_SetBasicRate) , /*25*/
 	GEN_CMD_CODE(_GetBasicRate) ,
 	GEN_CMD_CODE(_SetDataRate) ,
 	GEN_CMD_CODE(_GetDataRate) ,
	GEN_CMD_CODE(_SetPhyInfo) ,
	
 	GEN_CMD_CODE(_GetPhyInfo) ,	/*30*/
	GEN_CMD_CODE(_SetPhy) ,
 	GEN_CMD_CODE(_GetPhy) ,
 	GEN_CMD_CODE(_readRssi) ,
 	GEN_CMD_CODE(_readGain) ,
 	GEN_CMD_CODE(_SetAtim) , /*35*/
 	GEN_CMD_CODE(_SetPwrMode) , 
 	GEN_CMD_CODE(_JoinbssRpt),
 	GEN_CMD_CODE(_SetRaTable) ,
 	GEN_CMD_CODE(_GetRaTable) ,  	
 	
 	GEN_CMD_CODE(_GetCCXReport), /*40*/
 	GEN_CMD_CODE(_GetDTMReport),
 	GEN_CMD_CODE(_GetTXRateStatistics),
 	GEN_CMD_CODE(_SetUsbSuspend),
 	GEN_CMD_CODE(_SetH2cLbk),
 	GEN_CMD_CODE(_AddBAReq) , /*45*/
	GEN_CMD_CODE(_SetChannel), /*46*/
	GEN_CMD_CODE(_SetTxPower), 
	GEN_CMD_CODE(_SwitchAntenna),
	GEN_CMD_CODE(_SetCrystalCap),
	GEN_CMD_CODE(_SetSingleCarrierTx), /*50*/
	
	GEN_CMD_CODE(_SetSingleToneTx),/*51*/
	GEN_CMD_CODE(_SetCarrierSuppressionTx),
	GEN_CMD_CODE(_SetContinuousTx),
	GEN_CMD_CODE(_SwitchBandwidth), /*54*/
	GEN_CMD_CODE(_TX_Beacon), /*55*/
	
	GEN_CMD_CODE(_Set_MLME_EVT), /*56*/
	GEN_CMD_CODE(_Set_Drv_Extra), /*57*/
	GEN_CMD_CODE(_Set_H2C_MSG), /*58*/

#if 1
	//To do, modify these h2c cmd, add or delete
	GEN_CMD_CODE(_GetH2cLbk) ,

	// WPS extra IE
	GEN_CMD_CODE(_SetProbeReqExtraIE) ,
	GEN_CMD_CODE(_SetAssocReqExtraIE) ,
	GEN_CMD_CODE(_SetProbeRspExtraIE) ,
	GEN_CMD_CODE(_SetAssocRspExtraIE) ,
	
	// the following is driver will do
	GEN_CMD_CODE(_GetCurDataRate) , 

	GEN_CMD_CODE(_GetTxRetrycnt),  // to record times that Tx retry to transmmit packet after association
	GEN_CMD_CODE(_GetRxRetrycnt), // to record total number of the received frame with ReTry bit set in the WLAN header

	GEN_CMD_CODE(_GetBCNOKcnt),
	GEN_CMD_CODE(_GetBCNERRcnt),
	GEN_CMD_CODE(_GetCurTxPwrLevel),

	GEN_CMD_CODE(_SetDIG),
	GEN_CMD_CODE(_SetRA),
	GEN_CMD_CODE(_SetPT),
	GEN_CMD_CODE(_ReadTSSI),
	GEN_CMD_CODE(_SetRFIntFs),
 #endif	
	
	MAX_H2CCMD
};


#define _GetBBReg_CMD_		_Read_BBREG_CMD_
#define _SetBBReg_CMD_ 		_Write_BBREG_CMD_
#define _GetRFReg_CMD_ 		_Read_RFREG_CMD_
#define _SetRFReg_CMD_ 		_Write_RFREG_CMD_

#ifdef _RTL8192C_CMD_C_
struct _cmd_callback 	rtw_cmd_callback[] = 
{
	{GEN_CMD_CODE(_Read_MACREG), NULL}, /*0*/
	{GEN_CMD_CODE(_Write_MACREG), NULL}, 
	{GEN_CMD_CODE(_Read_BBREG), &rtw_getbbrfreg_cmdrsp_callback},
	{GEN_CMD_CODE(_Write_BBREG), NULL},
	{GEN_CMD_CODE(_Read_RFREG), &rtw_getbbrfreg_cmdrsp_callback},
	{GEN_CMD_CODE(_Write_RFREG), NULL}, /*5*/
	{GEN_CMD_CODE(_Read_EEPROM), NULL},
	{GEN_CMD_CODE(_Write_EEPROM), NULL},
	{GEN_CMD_CODE(_Read_EFUSE), NULL},
	{GEN_CMD_CODE(_Write_EFUSE), NULL},
	
	{GEN_CMD_CODE(_Read_CAM),	NULL},	/*10*/
	{GEN_CMD_CODE(_Write_CAM),	 NULL},	
	{GEN_CMD_CODE(_setBCNITV), NULL},
 	{GEN_CMD_CODE(_setMBIDCFG), NULL},
	{GEN_CMD_CODE(_JoinBss), &rtw_joinbss_cmd_callback},  /*14*/
	{GEN_CMD_CODE(_DisConnect), &rtw_disassoc_cmd_callback}, /*15*/
	{GEN_CMD_CODE(_CreateBss), &rtw_createbss_cmd_callback},
	{GEN_CMD_CODE(_SetOpMode), NULL},
	{GEN_CMD_CODE(_SiteSurvey), &rtw_survey_cmd_callback}, /*18*/
	{GEN_CMD_CODE(_SetAuth), NULL},
	
	{GEN_CMD_CODE(_SetKey), NULL},	/*20*/
	{GEN_CMD_CODE(_SetStaKey), &rtw_setstaKey_cmdrsp_callback},
	{GEN_CMD_CODE(_SetAssocSta), &rtw_setassocsta_cmdrsp_callback},
	{GEN_CMD_CODE(_DelAssocSta), NULL},	
	{GEN_CMD_CODE(_SetStaPwrState), NULL},	
	{GEN_CMD_CODE(_SetBasicRate), NULL}, /*25*/
	{GEN_CMD_CODE(_GetBasicRate), NULL},
	{GEN_CMD_CODE(_SetDataRate), NULL},
	{GEN_CMD_CODE(_GetDataRate), NULL},
	{GEN_CMD_CODE(_SetPhyInfo), NULL},
	
	{GEN_CMD_CODE(_GetPhyInfo), NULL}, /*30*/
	{GEN_CMD_CODE(_SetPhy), NULL},
	{GEN_CMD_CODE(_GetPhy), NULL},	
	{GEN_CMD_CODE(_readRssi), NULL},
	{GEN_CMD_CODE(_readGain), NULL},
	{GEN_CMD_CODE(_SetAtim), NULL}, /*35*/
	{GEN_CMD_CODE(_SetPwrMode), NULL},
	{GEN_CMD_CODE(_JoinbssRpt), NULL},
	{GEN_CMD_CODE(_SetRaTable), NULL},
	{GEN_CMD_CODE(_GetRaTable) , NULL},
 	
	{GEN_CMD_CODE(_GetCCXReport), NULL}, /*40*/
 	{GEN_CMD_CODE(_GetDTMReport),	NULL},
 	{GEN_CMD_CODE(_GetTXRateStatistics), NULL}, 
 	{GEN_CMD_CODE(_SetUsbSuspend), NULL}, 
 	{GEN_CMD_CODE(_SetH2cLbk), NULL},
 	{GEN_CMD_CODE(_AddBAReq), NULL}, /*45*/
	{GEN_CMD_CODE(_SetChannel), NULL},		/*46*/
	{GEN_CMD_CODE(_SetTxPower), NULL},
	{GEN_CMD_CODE(_SwitchAntenna), NULL},
	{GEN_CMD_CODE(_SetCrystalCap), NULL},
	{GEN_CMD_CODE(_SetSingleCarrierTx), NULL},	/*50*/
	
	{GEN_CMD_CODE(_SetSingleToneTx), NULL}, /*51*/
	{GEN_CMD_CODE(_SetCarrierSuppressionTx), NULL},
	{GEN_CMD_CODE(_SetContinuousTx), NULL},
	{GEN_CMD_CODE(_SwitchBandwidth), NULL},		/*54*/
	{GEN_CMD_CODE(_TX_Beacon), NULL},/*55*/

	{GEN_CMD_CODE(_Set_MLME_EVT), NULL},/*56*/
	{GEN_CMD_CODE(_Set_Drv_Extra), NULL},/*57*/
	{GEN_CMD_CODE(_Set_H2C_MSG), NULL},/*58*/
	

#if 1//To do, modify these h2c cmd, add or delete
	{GEN_CMD_CODE(_GetH2cLbk), NULL},
	{_SetProbeReqExtraIE_CMD_, NULL},
	{_SetAssocReqExtraIE_CMD_, NULL},
	{_SetProbeRspExtraIE_CMD_, NULL},
	{_SetAssocRspExtraIE_CMD_, NULL},	
	{_GetCurDataRate_CMD_, NULL},
	{_GetTxRetrycnt_CMD_, NULL},
	{_GetRxRetrycnt_CMD_, NULL},	
	{_GetBCNOKcnt_CMD_, NULL},	
	{_GetBCNERRcnt_CMD_, NULL},	
	{_GetCurTxPwrLevel_CMD_, NULL},	
	{_SetDIG_CMD_, NULL},	
	{_SetRA_CMD_, NULL},		
	{_SetPT_CMD_,NULL},
	{GEN_CMD_CODE(_ReadTSSI), &rtw_readtssi_cmdrsp_callback},
	{GEN_CMD_CODE(_SetRFIntFs), NULL},
#endif

};
#endif


struct cmd_hdl {
	uint	parmsize;
	u8 (*h2cfuns)(struct _ADAPTER *padapter, u8 *pbuf);	
};


u8 read_macreg_hdl(_adapter *padapter, u8 *pbuf);
u8 write_macreg_hdl(_adapter *padapter, u8 *pbuf);
u8 read_bbreg_hdl(_adapter *padapter, u8 *pbuf);
u8 write_bbreg_hdl(_adapter *padapter, u8 *pbuf);
u8 read_rfreg_hdl(_adapter *padapter, u8 *pbuf);
u8 write_rfreg_hdl(_adapter *padapter, u8 *pbuf);


u8 rtl8192c_NULL_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_join_cmd_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_disconnect_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_createbss_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_setopmode_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_sitesurvey_cmd_hdl(_adapter *padapter, u8 *pbuf);	
u8 rtl8192c_setauth_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_setkey_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_set_stakey_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_set_assocsta_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_del_assocsta_hdl(_adapter *padapter, u8 *pbuf);
u8 rtl8192c_add_ba_hdl(_adapter *padapter, unsigned char *pbuf);

u8 rtl8192c_mlme_evt_hdl(_adapter *padapter, unsigned char *pbuf);
u8 rtl8192c_drvextra_cmd_hdl(_adapter *padapter, unsigned char *pbuf);
u8 rtl8192c_h2c_msg_hdl(_adapter *padapter, unsigned char *pbuf);

#define GEN_DRV_CMD_HANDLER(size, cmd)	{size, &rtl8192c_ ## cmd ## _hdl},
//#define GEN_DRV_CMD_HANDLER(size, cmd)		{size, &cmd ## _hdl},
//#define GEN_MLME_EXT_HANDLER(size, cmd)	{size, &rtl8192c_ ## cmd ## _hdl},
#define GEN_MLME_EXT_HANDLER(size, cmd)	{size, cmd},

#ifdef _RTL8192C_CMD_C_

struct cmd_hdl wlancmds[] = 
{
	GEN_DRV_CMD_HANDLER(0, NULL) /*0*/
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL) /*10*/
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)		
	GEN_MLME_EXT_HANDLER(sizeof (struct joinbss_parm), rtl8192c_join_cmd_hdl) /*14*/
	GEN_MLME_EXT_HANDLER(sizeof (struct disconnect_parm), rtl8192c_disconnect_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct createbss_parm), rtl8192c_createbss_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct setopmode_parm), rtl8192c_setopmode_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct sitesurvey_parm), rtl8192c_sitesurvey_cmd_hdl) /*18*/
	GEN_MLME_EXT_HANDLER(sizeof (struct setauth_parm), rtl8192c_setauth_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct setkey_parm), rtl8192c_setkey_hdl) /*20*/
	GEN_MLME_EXT_HANDLER(sizeof (struct set_stakey_parm), rtl8192c_set_stakey_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct set_assocsta_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct del_assocsta_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct setstapwrstate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct setbasicrate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct getbasicrate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct setdatarate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct getdatarate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct setphyinfo_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct getphyinfo_parm), NULL)  /*30*/
	GEN_MLME_EXT_HANDLER(sizeof (struct setphy_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct getphy_parm), NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)	/*40*/
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(sizeof(struct addBaReq_parm), rtl8192c_add_ba_hdl)	
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL) /*50*/
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL) 
	GEN_MLME_EXT_HANDLER(sizeof(struct Tx_Beacon_param), NULL) /*55*/

	GEN_MLME_EXT_HANDLER(0, rtl8192c_mlme_evt_hdl) /*56*/
	GEN_MLME_EXT_HANDLER(0, rtl8192c_drvextra_cmd_hdl) /*57*/

	GEN_MLME_EXT_HANDLER(0, rtl8192c_h2c_msg_hdl) /*58*/

};

#endif

#endif


