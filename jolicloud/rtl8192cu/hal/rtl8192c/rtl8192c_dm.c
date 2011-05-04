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

//============================================================
// Description:
//
// This file is for 92CE/92CU dynamic mechanism only
//
//
//============================================================

//============================================================
// include files
//============================================================
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_byteorder.h>

#include <hal_init.h>
//#include <linux/math64.h>

#include "Hal8192CPhyReg.h"
#include "Hal8192CPhyCfg.h"
#include "rtl8192c_dm.h"

//============================================================
// Global var
//============================================================
/*
static u32 edca_setting_DL[HT_IOT_PEER_MAX] = 
// UNKNOWN		REALTEK_90	REALTEK_92SE	BROADCOM		RALINK		ATHEROS		CISCO		MARVELL		92U_AP		SELF_AP
{ 0xa44f, 		0x5ea44f, 	0x5e4322, 		0x5ea42b, 		0xa44f, 		0xa630, 		0xa630,		0xa44f,		0x5e4322,	0x5ea42b};

static u32 edca_setting_DL_GMode[HT_IOT_PEER_MAX] = 
// UNKNOWN		REALTEK_90	REALTEK_92SE	BROADCOM		RALINK		ATHEROS		CISCO		MARVELL		92U_AP		SELF_AP
{ 0x4322, 		0xa44f, 		0x5e4322,		0xa42b, 			0x5e4322, 	0x4322, 		0xa430,		0xa44f,		0x5e4322,	0x5ea42b};

static u32 edca_setting_UL[HT_IOT_PEER_MAX] = 
// UNKNOWN		REALTEK_90	REALTEK_92SE	BROADCOM		RALINK		ATHEROS		CISCO		MARVELL		92U_AP		SELF_AP
{ 0x5e4322, 		0xa44f, 		0x5e4322,		0x5ea32b,  		0x5ea422, 	0x5ea322,	0x5e4322,	0x5ea44f,	0x5e4322,	0x5e4322};
*/

#define	OFDM_TABLE_SIZE 	37
#define	CCK_TABLE_SIZE		33

static u32 OFDMSwingTable[OFDM_TABLE_SIZE] = {
	0x7f8001fe, // 0, +6.0dB
	0x788001e2, // 1, +5.5dB
	0x71c001c7, // 2, +5.0dB
	0x6b8001ae, // 3, +4.5dB
	0x65400195, // 4, +4.0dB
	0x5fc0017f, // 5, +3.5dB
	0x5a400169, // 6, +3.0dB
	0x55400155, // 7, +2.5dB
	0x50800142, // 8, +2.0dB
	0x4c000130, // 9, +1.5dB
	0x47c0011f, // 10, +1.0dB
	0x43c0010f, // 11, +0.5dB
	0x40000100, // 12, +0dB
	0x3c8000f2, // 13, -0.5dB
	0x390000e4, // 14, -1.0dB
	0x35c000d7, // 15, -1.5dB
	0x32c000cb, // 16, -2.0dB
	0x300000c0, // 17, -2.5dB
	0x2d4000b5, // 18, -3.0dB
	0x2ac000ab, // 19, -3.5dB
	0x288000a2, // 20, -4.0dB
	0x26000098, // 21, -4.5dB
	0x24000090, // 22, -5.0dB
	0x22000088, // 23, -5.5dB
	0x20000080, // 24, -6.0dB
	0x1e400079, // 25, -6.5dB
	0x1c800072, // 26, -7.0dB
	0x1b00006c, // 27. -7.5dB
	0x19800066, // 28, -8.0dB
	0x18000060, // 29, -8.5dB
	0x16c0005b, // 30, -9.0dB
	0x15800056, // 31, -9.5dB
	0x14400051, // 32, -10.0dB
	0x1300004c, // 33, -10.5dB
	0x12000048, // 34, -11.0dB
	0x11000044, // 35, -11.5dB
	0x10000040, // 36, -12.0dB
};

static u8 CCKSwingTable_Ch1_Ch13[CCK_TABLE_SIZE][8] = {
{0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04},	// 0, +0dB
{0x33, 0x32, 0x2b, 0x23, 0x1a, 0x11, 0x08, 0x04},	// 1, -0.5dB
{0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03},	// 2, -1.0dB
{0x2d, 0x2d, 0x27, 0x1f, 0x18, 0x0f, 0x08, 0x03},	// 3, -1.5dB
{0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03},	// 4, -2.0dB 
{0x28, 0x28, 0x22, 0x1c, 0x15, 0x0d, 0x07, 0x03},	// 5, -2.5dB
{0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03},	// 6, -3.0dB
{0x24, 0x23, 0x1f, 0x19, 0x13, 0x0c, 0x06, 0x03},	// 7, -3.5dB
{0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02},	// 8, -4.0dB 
{0x20, 0x20, 0x1b, 0x16, 0x11, 0x08, 0x05, 0x02},	// 9, -4.5dB
{0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02},	// 10, -5.0dB 
{0x1d, 0x1c, 0x18, 0x14, 0x0f, 0x0a, 0x05, 0x02},	// 11, -5.5dB
{0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02},	// 12, -6.0dB 
{0x1a, 0x19, 0x16, 0x12, 0x0d, 0x09, 0x04, 0x02},	// 13, -6.5dB
{0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02},	// 14, -7.0dB 
{0x17, 0x16, 0x13, 0x10, 0x0c, 0x08, 0x04, 0x02},	// 15, -7.5dB
{0x16, 0x15, 0x12, 0x0f, 0x0b, 0x07, 0x04, 0x01},	// 16, -8.0dB 
{0x14, 0x14, 0x11, 0x0e, 0x0b, 0x07, 0x03, 0x02},	// 17, -8.5dB
{0x13, 0x13, 0x10, 0x0d, 0x0a, 0x06, 0x03, 0x01},	// 18, -9.0dB 
{0x12, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	// 19, -9.5dB
{0x11, 0x11, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	// 20, -10.0dB
{0x10, 0x10, 0x0e, 0x0b, 0x08, 0x05, 0x03, 0x01},	// 21, -10.5dB
{0x0f, 0x0f, 0x0d, 0x0b, 0x08, 0x05, 0x03, 0x01},	// 22, -11.0dB
{0x0e, 0x0e, 0x0c, 0x0a, 0x08, 0x05, 0x02, 0x01},	// 23, -11.5dB
{0x0d, 0x0d, 0x0c, 0x0a, 0x07, 0x05, 0x02, 0x01},	// 24, -12.0dB
{0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x04, 0x02, 0x01},	// 25, -12.5dB
{0x0c, 0x0c, 0x0a, 0x09, 0x06, 0x04, 0x02, 0x01},	// 26, -13.0dB
{0x0b, 0x0b, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x01},	// 27, -13.5dB
{0x0b, 0x0a, 0x09, 0x08, 0x06, 0x04, 0x02, 0x01},	// 28, -14.0dB
{0x0a, 0x0a, 0x09, 0x07, 0x05, 0x03, 0x02, 0x01},	// 29, -14.5dB
{0x0a, 0x09, 0x08, 0x07, 0x05, 0x03, 0x02, 0x01},	// 30, -15.0dB
{0x09, 0x09, 0x08, 0x06, 0x05, 0x03, 0x01, 0x01},	// 31, -15.5dB
{0x09, 0x08, 0x07, 0x06, 0x04, 0x03, 0x01, 0x01}	// 32, -16.0dB
};

static u8 CCKSwingTable_Ch14 [CCK_TABLE_SIZE][8]= {
{0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00},	// 0, +0dB	
{0x33, 0x32, 0x2b, 0x19, 0x00, 0x00, 0x00, 0x00},	// 1, -0.5dB 
{0x30, 0x2f, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00},	// 2, -1.0dB  
{0x2d, 0x2d, 0x17, 0x17, 0x00, 0x00, 0x00, 0x00},	// 3, -1.5dB
{0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00},	// 4, -2.0dB  
{0x28, 0x28, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00},	// 5, -2.5dB
{0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00},	// 6, -3.0dB  
{0x24, 0x23, 0x1f, 0x12, 0x00, 0x00, 0x00, 0x00},	// 7, -3.5dB  
{0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00},	// 8, -4.0dB  
{0x20, 0x20, 0x1b, 0x10, 0x00, 0x00, 0x00, 0x00},	// 9, -4.5dB
{0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00},	// 10, -5.0dB  
{0x1d, 0x1c, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00},	// 11, -5.5dB
{0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00},	// 12, -6.0dB  
{0x1a, 0x19, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00},	// 13, -6.5dB 
{0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00},	// 14, -7.0dB  
{0x17, 0x16, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00},	// 15, -7.5dB
{0x16, 0x15, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00},	// 16, -8.0dB  
{0x14, 0x14, 0x11, 0x0a, 0x00, 0x00, 0x00, 0x00},	// 17, -8.5dB
{0x13, 0x13, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00},	// 18, -9.0dB  
{0x12, 0x12, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	// 19, -9.5dB
{0x11, 0x11, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	// 20, -10.0dB
{0x10, 0x10, 0x0e, 0x08, 0x00, 0x00, 0x00, 0x00},	// 21, -10.5dB
{0x0f, 0x0f, 0x0d, 0x08, 0x00, 0x00, 0x00, 0x00},	// 22, -11.0dB
{0x0e, 0x0e, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	// 23, -11.5dB
{0x0d, 0x0d, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	// 24, -12.0dB
{0x0d, 0x0c, 0x0b, 0x06, 0x00, 0x00, 0x00, 0x00},	// 25, -12.5dB
{0x0c, 0x0c, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	// 26, -13.0dB
{0x0b, 0x0b, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	// 27, -13.5dB
{0x0b, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	// 28, -14.0dB
{0x0a, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	// 29, -14.5dB
{0x0a, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	// 30, -15.0dB
{0x09, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	// 31, -15.5dB
{0x09, 0x08, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00}	// 32, -16.0dB
};	

/*-----------------------------------------------------------------------------
 * Function:	dm_DIGInit()
 *
 * Overview:	Set DIG scheme init value.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *
 *---------------------------------------------------------------------------*/
void	dm_DIGInit(
	IN	PADAPTER	pAdapter
)
{	
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	

	pDigTable->Dig_Enable_Flag = _TRUE;
	pDigTable->Dig_Ext_Port_Stage = DIG_EXT_PORT_STAGE_MAX;
	
	pDigTable->CurIGValue = 0x20;
	pDigTable->PreIGValue = 0x0;

	pDigTable->CurSTAConnectState = pDigTable->PreSTAConnectState = DIG_STA_DISCONNECT;
	pDigTable->CurMultiSTAConnectState = DIG_MultiSTA_DISCONNECT;

	pDigTable->RssiLowThresh 	= DM_DIG_THRESH_LOW;
	pDigTable->RssiHighThresh 	= DM_DIG_THRESH_HIGH;

	pDigTable->FALowThresh	= DM_FALSEALARM_THRESH_LOW;
	pDigTable->FAHighThresh	= DM_FALSEALARM_THRESH_HIGH;

	
	pDigTable->rx_gain_range_max = DM_DIG_MAX;
	pDigTable->rx_gain_range_min = DM_DIG_MIN;

	pDigTable->BackoffVal = DM_DIG_BACKOFF_DEFAULT;
	pDigTable->BackoffVal_range_max = DM_DIG_BACKOFF_MAX;
	pDigTable->BackoffVal_range_min = DM_DIG_BACKOFF_MIN;

	pDigTable->PreCCKPDState = CCK_PD_STAGE_MAX;
	pDigTable->CurCCKPDState = CCK_PD_STAGE_MAX;
	
}


u8 dm_initial_gain_MinPWDB(
	IN	PADAPTER	pAdapter
	)
{
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	
	int	Rssi_val_min = 0;
	
	if((pDigTable->CurMultiSTAConnectState == DIG_MultiSTA_CONNECT) &&
		(pDigTable->CurSTAConnectState == DIG_STA_CONNECT) )
	{
		if(pdmpriv->EntryMinUndecoratedSmoothedPWDB != 0)
			Rssi_val_min  =  (pdmpriv->EntryMinUndecoratedSmoothedPWDB > pdmpriv->UndecoratedSmoothedPWDB)?
					pdmpriv->UndecoratedSmoothedPWDB:pdmpriv->EntryMinUndecoratedSmoothedPWDB;		
		else
			Rssi_val_min = pdmpriv->UndecoratedSmoothedPWDB;
	}
	else if(pDigTable->CurSTAConnectState == DIG_STA_CONNECT || 
			pDigTable->CurSTAConnectState == DIG_STA_BEFORE_CONNECT) 
		Rssi_val_min = pdmpriv->UndecoratedSmoothedPWDB;
	else if(pDigTable->CurMultiSTAConnectState == DIG_MultiSTA_CONNECT)
		Rssi_val_min = pdmpriv->EntryMinUndecoratedSmoothedPWDB;

	return (u8)Rssi_val_min;
}


VOID 
dm_FalseAlarmCounterStatistics(
	IN	PADAPTER	Adapter
	)
{
	u32 ret_value;
	struct dm_priv *pdmpriv = &Adapter->dmpriv;	
	PFALSE_ALARM_STATISTICS FalseAlmCnt = &(pdmpriv->FalseAlmCnt);
	
	ret_value = PHY_QueryBBReg(Adapter, rOFDM_PHYCounter1, bMaskDWord);
       FalseAlmCnt->Cnt_Parity_Fail = ((ret_value&0xffff0000)>>16);	

       ret_value = PHY_QueryBBReg(Adapter, rOFDM_PHYCounter2, bMaskDWord);
	FalseAlmCnt->Cnt_Rate_Illegal = (ret_value&0xffff);
	FalseAlmCnt->Cnt_Crc8_fail = ((ret_value&0xffff0000)>>16);
	ret_value = PHY_QueryBBReg(Adapter, rOFDM_PHYCounter3, bMaskDWord);
	FalseAlmCnt->Cnt_Mcs_fail = (ret_value&0xffff);

	FalseAlmCnt->Cnt_Ofdm_fail = 	FalseAlmCnt->Cnt_Parity_Fail + FalseAlmCnt->Cnt_Rate_Illegal +
								FalseAlmCnt->Cnt_Crc8_fail + FalseAlmCnt->Cnt_Mcs_fail;

	
	//hold cck counter
	PHY_SetBBReg(Adapter, rCCK0_FalseAlarmReport, BIT(14), 1);
	
	ret_value = PHY_QueryBBReg(Adapter, rCCK0_FACounterLower, bMaskByte0);
	FalseAlmCnt->Cnt_Cck_fail = ret_value;

	ret_value = PHY_QueryBBReg(Adapter, rCCK0_FACounterUpper, bMaskByte3);
	FalseAlmCnt->Cnt_Cck_fail +=  (ret_value& 0xff)<<8;
	
	FalseAlmCnt->Cnt_all = (	FalseAlmCnt->Cnt_Parity_Fail +
						FalseAlmCnt->Cnt_Rate_Illegal +
						FalseAlmCnt->Cnt_Crc8_fail +
						FalseAlmCnt->Cnt_Mcs_fail +
						FalseAlmCnt->Cnt_Cck_fail);	
	
	//reset false alarm counter registers
	PHY_SetBBReg(Adapter, rOFDM1_LSTF, 0x08000000, 1);
	PHY_SetBBReg(Adapter, rOFDM1_LSTF, 0x08000000, 0);
	//reset cck counter
	PHY_SetBBReg(Adapter, rCCK0_FalseAlarmReport, 0x0000c000, 0);
	//enable cck counter
	PHY_SetBBReg(Adapter, rCCK0_FalseAlarmReport, 0x0000c000, 2);

	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("Cnt_Parity_Fail = %ld, Cnt_Rate_Illegal = %ld, Cnt_Crc8_fail = %ld, Cnt_Mcs_fail = %ld\n", 
	//			FalseAlmCnt->Cnt_Parity_Fail, FalseAlmCnt->Cnt_Rate_Illegal, FalseAlmCnt->Cnt_Crc8_fail, FalseAlmCnt->Cnt_Mcs_fail) );	
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("Cnt_Ofdm_fail = %ld, Cnt_Cck_fail = %ld, Cnt_all = %ld\n", 
	//			FalseAlmCnt->Cnt_Ofdm_fail, FalseAlmCnt->Cnt_Cck_fail, FalseAlmCnt->Cnt_all) );		
}


VOID
DM_Write_DIG(
	IN	PADAPTER	pAdapter
	)
{
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("CurIGValue = 0x%lx, PreIGValue = 0x%lx, BackoffVal = %d\n", 
	//			DM_DigTable.CurIGValue, DM_DigTable.PreIGValue, DM_DigTable.BackoffVal));
	
	if(pDigTable->PreIGValue != pDigTable->CurIGValue)
	{
		// Set initial gain.
		//PHY_SetBBReg(pAdapter, rOFDM0_XAAGCCore1, bMaskByte0, pDigTable->CurIGValue);
		//PHY_SetBBReg(pAdapter, rOFDM0_XBAGCCore1, bMaskByte0, pDigTable->CurIGValue);	
		PHY_SetBBReg(pAdapter, rOFDM0_XAAGCCore1, 0x7f, pDigTable->CurIGValue);
		PHY_SetBBReg(pAdapter, rOFDM0_XBAGCCore1, 0x7f, pDigTable->CurIGValue);
		
		pDigTable->PreIGValue = pDigTable->CurIGValue;
	}
}


VOID 
dm_CtrlInitGainByFA(
	IN	PADAPTER	pAdapter
)	
{
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	PFALSE_ALARM_STATISTICS FalseAlmCnt = &(pdmpriv->FalseAlmCnt);

	u8	value_IGI = pDigTable->CurIGValue;
	
	if(FalseAlmCnt->Cnt_all < DM_DIG_FA_TH0)	
		value_IGI --;
	else if(FalseAlmCnt->Cnt_all < DM_DIG_FA_TH1)	
		value_IGI += 0;
	else if(FalseAlmCnt->Cnt_all < DM_DIG_FA_TH2)	
		value_IGI ++;
	else if(FalseAlmCnt->Cnt_all >= DM_DIG_FA_TH2)	
		value_IGI +=2;
	
	if(value_IGI > DM_DIG_FA_UPPER)			
		value_IGI = DM_DIG_FA_UPPER;
	else if(value_IGI < DM_DIG_FA_LOWER)		
		value_IGI = DM_DIG_FA_LOWER;

	if(FalseAlmCnt->Cnt_all > 10000)
		value_IGI = 0x32;
	
	pDigTable->CurIGValue = value_IGI;
	
	DM_Write_DIG(pAdapter);
	
}


VOID 
dm_CtrlInitGainByRssi(
	IN	PADAPTER	pAdapter
)	
{
	u32 isBT;
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	PFALSE_ALARM_STATISTICS FalseAlmCnt = &(pdmpriv->FalseAlmCnt);
	
	if(FalseAlmCnt->Cnt_all < 250)
	{
		//DBG_8192C("===> dm_CtrlInitGainByRssi, Enter DIG by SS mode\n");
		
	isBT = rtw_read8(pAdapter, 0x4fd) & 0x01;

	if(!isBT){
			
		if(FalseAlmCnt->Cnt_all > pDigTable->FAHighThresh)
		{
			if((pDigTable->BackoffVal -2) < pDigTable->BackoffVal_range_min)
				pDigTable->BackoffVal = pDigTable->BackoffVal_range_min;
			else
				pDigTable->BackoffVal -= 2; 
		}	
		else if(FalseAlmCnt->Cnt_all < pDigTable->FALowThresh)
		{
			if((pDigTable->BackoffVal+2) > pDigTable->BackoffVal_range_max)
				pDigTable->BackoffVal = pDigTable->BackoffVal_range_max;
			else
				pDigTable->BackoffVal +=2;
		}	
	}
	else
		pDigTable->BackoffVal = DM_DIG_BACKOFF_DEFAULT;
		
	if((pDigTable->Rssi_val_min+10-pDigTable->BackoffVal) > pDigTable->rx_gain_range_max)
		pDigTable->CurIGValue = pDigTable->rx_gain_range_max;
	else if((pDigTable->Rssi_val_min+10-pDigTable->BackoffVal) < pDigTable->rx_gain_range_min)
		pDigTable->CurIGValue = pDigTable->rx_gain_range_min;
	else
		pDigTable->CurIGValue = pDigTable->Rssi_val_min+10-pDigTable->BackoffVal;

		//DBG_8192C("Rssi_val_min = %x BackoffVal %x\n",pDigTable->Rssi_val_min, pDigTable->BackoffVal);
	
	}
	else
	{		
		//DBG_8192C("===> dm_CtrlInitGainByRssi, Enter DIG by FA mode\n");
		//DBG_8192C("RSSI = 0x%x", pDigTable->Rssi_val_min);

		//Adjust initial gain by false alarm		
		if(FalseAlmCnt->Cnt_all > 1000)
			pDigTable->CurIGValue = pDigTable ->PreIGValue+2;
		else if (FalseAlmCnt->Cnt_all > 750)
			pDigTable->CurIGValue = pDigTable->PreIGValue+1;
		else if(FalseAlmCnt->Cnt_all < 500)
			pDigTable->CurIGValue = pDigTable->PreIGValue-1;	

		//Check initial gain by RSSI
		if(pDigTable->CurIGValue > (pDigTable->Rssi_val_min + 30))
			pDigTable->CurIGValue =	pDigTable->PreIGValue-2;	
		else if(pDigTable->CurIGValue < pDigTable->Rssi_val_min)
			pDigTable->CurIGValue =	pDigTable->Rssi_val_min;

		//Check initial gain by upper/lower bound
		if(pDigTable->CurIGValue >pDigTable->rx_gain_range_max)
			pDigTable->CurIGValue = pDigTable->rx_gain_range_max;
		else if(pDigTable->CurIGValue < pDigTable->rx_gain_range_min)
			pDigTable->CurIGValue = pDigTable->rx_gain_range_min;
		
	}

	DM_Write_DIG(pAdapter);

}


VOID
dm_initial_gain_Multi_STA(
	IN	PADAPTER	pAdapter)
{
	
	struct mlme_priv *pmlmepriv = &(pAdapter->mlmepriv);
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	int 	rssi_strength =  pdmpriv->EntryMinUndecoratedSmoothedPWDB;	
	BOOLEAN	 bMulti_STA = _FALSE;

	 if ((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		       (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE))
	 {
		bMulti_STA = _TRUE;
	 }

	 //todo: AP Mode
		
	
	if((bMulti_STA == _FALSE) 
		|| (pDigTable->CurSTAConnectState != DIG_STA_DISCONNECT))
	{
		pdmpriv->initial_gain_Multi_STA_binitialized = _FALSE;
		pDigTable->Dig_Ext_Port_Stage = DIG_EXT_PORT_STAGE_MAX;
		return;
	}	
	else if(pdmpriv->initial_gain_Multi_STA_binitialized == _FALSE)
	{
		pdmpriv->initial_gain_Multi_STA_binitialized = _TRUE;
		pDigTable->Dig_Ext_Port_Stage = DIG_EXT_PORT_STAGE_0;
		pDigTable->CurIGValue = 0x20;
		DM_Write_DIG(pAdapter);
	}

	// Initial gain control by ap mode 
	if(pDigTable->CurMultiSTAConnectState == DIG_MultiSTA_CONNECT)
	{
		if (	(rssi_strength < pDigTable->RssiLowThresh) 	&& 
			(pDigTable->Dig_Ext_Port_Stage != DIG_EXT_PORT_STAGE_1))
		{					
			// Set to dig value to 0x20 for Luke's opinion after disable dig
			if(pDigTable->Dig_Ext_Port_Stage == DIG_EXT_PORT_STAGE_2)
			{
				pDigTable->CurIGValue = 0x20;
				DM_Write_DIG(pAdapter);				
			}	
			pDigTable->Dig_Ext_Port_Stage = DIG_EXT_PORT_STAGE_1;	
		}	
		else if (rssi_strength > pDigTable->RssiHighThresh)
		{
			pDigTable->Dig_Ext_Port_Stage = DIG_EXT_PORT_STAGE_2;
			dm_CtrlInitGainByFA(pAdapter);
		} 
	}	
	else if(pDigTable->Dig_Ext_Port_Stage != DIG_EXT_PORT_STAGE_0)
	{
		pDigTable->Dig_Ext_Port_Stage = DIG_EXT_PORT_STAGE_0;
		pDigTable->CurIGValue = 0x20;
		DM_Write_DIG(pAdapter);
	}

	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("CurMultiSTAConnectState = %x Dig_Ext_Port_Stage %x\n", 
	//			DM_DigTable.CurMultiSTAConnectState, DM_DigTable.Dig_Ext_Port_Stage));
}


VOID 
dm_initial_gain_STA(
	IN	PADAPTER	pAdapter)
{
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("PreSTAConnectState = %x, CurSTAConnectState = %x\n", 
	//			DM_DigTable.PreSTAConnectState, DM_DigTable.CurSTAConnectState));


	if(pDigTable->PreSTAConnectState == pDigTable->CurSTAConnectState||
	   pDigTable->CurSTAConnectState == DIG_STA_BEFORE_CONNECT ||
	   pDigTable->CurSTAConnectState == DIG_STA_CONNECT)
	{
		// beforeconnect -> beforeconnect or  connect -> connect
		// (dis)connect -> beforeconnect
		// disconnect -> connecct or beforeconnect -> connect
		if(pDigTable->CurSTAConnectState != DIG_STA_DISCONNECT)
		{
			pDigTable->Rssi_val_min = dm_initial_gain_MinPWDB(pAdapter);
			dm_CtrlInitGainByRssi(pAdapter);
		}	
	}
	else	
	{		
		// connect -> disconnect or beforeconnect -> disconnect
		pDigTable->Rssi_val_min = 0;
		pDigTable->Dig_Ext_Port_Stage = DIG_EXT_PORT_STAGE_MAX;
		pDigTable->BackoffVal = DM_DIG_BACKOFF_DEFAULT;
		pDigTable->CurIGValue = 0x20;
		//pDigTable->PreIGValue = 0;	
		DM_Write_DIG(pAdapter);
	}

}


void dm_CCK_PacketDetectionThresh(
	IN	PADAPTER	pAdapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	PFALSE_ALARM_STATISTICS FalseAlmCnt = &(pdmpriv->FalseAlmCnt);
	
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;

	if(pDigTable->CurSTAConnectState == DIG_STA_CONNECT)
	{
		pDigTable->Rssi_val_min = dm_initial_gain_MinPWDB(pAdapter);
		if(pDigTable->PreCCKPDState == CCK_PD_STAGE_LowRssi)
		{
			if(pDigTable->Rssi_val_min <= 25)
				pDigTable->CurCCKPDState = CCK_PD_STAGE_LowRssi;
			else
				pDigTable->CurCCKPDState = CCK_PD_STAGE_HighRssi;
		}
		else{
			if(pDigTable->Rssi_val_min <= 20)
				pDigTable->CurCCKPDState = CCK_PD_STAGE_LowRssi;
			else
				pDigTable->CurCCKPDState = CCK_PD_STAGE_HighRssi;
		}
	}
	else
		pDigTable->CurCCKPDState=CCK_PD_STAGE_MAX;
	
	if(pDigTable->PreCCKPDState != pDigTable->CurCCKPDState)
	{
		if(pDigTable->CurCCKPDState == CCK_PD_STAGE_LowRssi)
		{
			if(FalseAlmCnt->Cnt_Cck_fail > 800)
				pDigTable->CurCCKFAState = CCK_FA_STAGE_High;				
			else
				pDigTable->CurCCKFAState = CCK_FA_STAGE_Low;
			
			if(pDigTable->PreCCKFAState != pDigTable->CurCCKFAState)
			{
				if(pDigTable->CurCCKFAState == CCK_FA_STAGE_Low)
			PHY_SetBBReg(pAdapter, rCCK0_CCA, bMaskByte2, 0x83);
				else
					PHY_SetBBReg(pAdapter, rCCK0_CCA, bMaskByte2, 0xcd);
				
				pDigTable->PreCCKFAState = pDigTable->CurCCKFAState;
			}
			
			PHY_SetBBReg(pAdapter, rCCK0_System, bMaskByte1, 0x40);
			
			//if(IS_92C_SERIAL(pHalData->VersionID))
			if(pHalData->rf_type != RF_1T1R)	
				PHY_SetBBReg(pAdapter, rCCK0_FalseAlarmReport , bMaskByte2, 0xd7);
			
		}
		else
		{
			PHY_SetBBReg(pAdapter, rCCK0_CCA, bMaskByte2, 0xcd);
			PHY_SetBBReg(pAdapter, rCCK0_System, bMaskByte1, 0x47);
			
			//if(IS_92C_SERIAL(pHalData->VersionID))
			if(pHalData->rf_type != RF_1T1R)	
				PHY_SetBBReg(pAdapter, rCCK0_FalseAlarmReport , bMaskByte2, 0xd3);
		}
		
		pDigTable->PreCCKPDState = pDigTable->CurCCKPDState;
		
	}
	
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("CCKPDStage=%x\n",DM_DigTable.CurCCKPDState));
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("is92C=%x\n",IS_92C_SERIAL(pHalData->VersionID)));
	
}


void dm_1R_CCA(
	IN	PADAPTER	pAdapter)
{	
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;

	if(pDigTable->CurSTAConnectState == DIG_STA_CONNECT)
	{
		pDigTable->Rssi_val_min = dm_initial_gain_MinPWDB(pAdapter);
		if(pDigTable->PreCCAState == CCA_2R)
		{
			if(pDigTable->Rssi_val_min >= 35)
				pDigTable->CurCCAState = CCA_1R;
			else
				pDigTable->CurCCAState = CCA_2R;
			
		}
		else{
			if(pDigTable->Rssi_val_min <= 30)
				pDigTable->CurCCAState = CCA_2R;
			else
				pDigTable->CurCCAState = CCA_1R;
		}
	}
	else
		pDigTable->CurCCAState=CCA_MAX;
	
	if(pDigTable->PreCCAState != pDigTable->CurCCAState)
	{
		if(pDigTable->CurCCAState == CCA_1R)
		{
			PHY_SetBBReg(pAdapter, rOFDM0_TRxPathEnable, bMaskByte0, 0x13);
			PHY_SetBBReg(pAdapter, 0xe70, bMaskByte3, 0x20);			
		}
		else
		{
			PHY_SetBBReg(pAdapter, rOFDM0_TRxPathEnable, bMaskByte0, 0x33);
			PHY_SetBBReg(pAdapter,0xe70, bMaskByte3, 0x63);
		}
		pDigTable->PreCCAState = pDigTable->CurCCAState;
	}
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("CCAStage=%x\n",DM_DigTable.CurCCAState));
}


static	void
dm_CtrlInitGainByTwoPort(
	IN	PADAPTER	pAdapter)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(pAdapter);
	struct mlme_priv *pmlmepriv = &(pAdapter->mlmepriv);
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;

	if (check_fwstate(pmlmepriv, _FW_UNDER_SURVEY) == _TRUE)
		return;

	// Decide the current status and if modify initial gain or not
	if (check_fwstate(pmlmepriv, _FW_UNDER_LINKING) == _TRUE)
	{
		pDigTable->CurSTAConnectState = DIG_STA_BEFORE_CONNECT;
	}	
	else if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) 
	{
		pDigTable->CurSTAConnectState = DIG_STA_CONNECT;
	}	
	else
	{
		pDigTable->CurSTAConnectState = DIG_STA_DISCONNECT;
	}

	dm_FalseAlarmCounterStatistics(pAdapter);
	
	dm_initial_gain_STA(pAdapter);	
	
	dm_initial_gain_Multi_STA(pAdapter);
	
	dm_CCK_PacketDetectionThresh(pAdapter);
	
	// We only perform 1R CCA on test chip only, added by Roger, 2010.03.08.
	//if(IS_92C_SERIAL(pHalData->VersionID))
	if((pHalData->rf_type != RF_1T1R) && !IS_NORMAL_CHIP(pHalData->VersionID))
	{
		dm_1R_CCA(pAdapter);
	}
	
	pDigTable->PreSTAConnectState = pDigTable->CurSTAConnectState;
	
}


void dm_DIG(
	IN	PADAPTER	pAdapter)
{
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	
	//RTPRINT(FDM, DM_Monitor, ("dm_DIG() ==>\n"));
	
	if(pdmpriv->bDMInitialGainEnable == _FALSE)
		return;
	
	if(pDigTable->Dig_Enable_Flag == _FALSE)
		return;
	
	if(!(pdmpriv->DMFlag & DYNAMIC_FUNC_DIG))
		return;
	
	//RTPRINT(FDM, DM_Monitor, ("dm_DIG() progress \n"));

	dm_CtrlInitGainByTwoPort(pAdapter);
	
	//RTPRINT(FDM, DM_Monitor, ("dm_DIG() <==\n"));
}


void dm_InitDynamicTxPower(IN	PADAPTER	Adapter)
{
	struct dm_priv *pdmpriv = &Adapter->dmpriv;

	pdmpriv->bDynamicTxPowerEnable = _FALSE;

	pdmpriv->LastDTPLvl = TxHighPwrLevel_Normal;
	pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Normal;
}


void dm_DynamicTxPower(IN	PADAPTER	Adapter)
{
	struct mlme_priv *pmlmepriv = &(Adapter->mlmepriv);
	struct mlme_ext_priv *pmlmeext = &Adapter->mlmeextpriv;
	struct dm_priv *pdmpriv = &Adapter->dmpriv;	
	//HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	int	UndecoratedSmoothedPWDB;

	if(pdmpriv->bDynamicTxPowerEnable != _TRUE)
		return;

	// If dynamic high power is disabled.
	if(  !(pdmpriv->DMFlag & DYNAMIC_FUNC_HP) )		
	{
		pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Normal;
		return;
	}

	// STA not connected and AP not connected
	if((check_fwstate(pmlmepriv, _FW_LINKED) != _TRUE) &&	
		(pdmpriv->EntryMinUndecoratedSmoothedPWDB == 0))
	{
		//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("Not connected to any \n"));
		pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Normal;

		//the LastDTPlvl should reset when disconnect, 
		//otherwise the tx power level wouldn't change when disconnect and connect again.
		// Maddest 20091220.
		pdmpriv->LastDTPLvl=TxHighPwrLevel_Normal;
		return;
	}
	
	if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)	// Default port
	{
		//todo: AP Mode
		if ((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		       (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE))
		{
			UndecoratedSmoothedPWDB = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("AP Client PWDB = 0x%x \n", UndecoratedSmoothedPWDB));
		}
		else
		{
			UndecoratedSmoothedPWDB = pdmpriv->UndecoratedSmoothedPWDB;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("STA Default Port PWDB = 0x%x \n", UndecoratedSmoothedPWDB));
		}
	}
	else // associated entry pwdb
	{	
		UndecoratedSmoothedPWDB = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
		//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("AP Ext Port PWDB = 0x%x \n", UndecoratedSmoothedPWDB));
	}
		
	if(UndecoratedSmoothedPWDB >= TX_POWER_NEAR_FIELD_THRESH_LVL2)
	{
		pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Level2;
		//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("TxHighPwrLevel_Level1 (TxPwr=0x0)\n"));
	}
	else if((UndecoratedSmoothedPWDB < (TX_POWER_NEAR_FIELD_THRESH_LVL2-3)) &&
		(UndecoratedSmoothedPWDB >= TX_POWER_NEAR_FIELD_THRESH_LVL1) )
	{
		pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Level1;
		//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("TxHighPwrLevel_Level1 (TxPwr=0x10)\n"));
	}
	else if(UndecoratedSmoothedPWDB < (TX_POWER_NEAR_FIELD_THRESH_LVL1-5))
	{
		pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Normal;
		//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("TxHighPwrLevel_Normal\n"));
	}

	if( (pdmpriv->DynamicTxHighPowerLvl != pdmpriv->LastDTPLvl) )
	{
		//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("PHY_SetTxPowerLevel8192S() Channel = %d \n" , pHalData->CurrentChannel));
		//PHY_SetTxPowerLevel8192C(Adapter, pHalData->CurrentChannel);
		set_channel_bwmode(Adapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
	}
	
	pdmpriv->LastDTPLvl = pdmpriv->DynamicTxHighPowerLvl;
	
}


VOID
DM_ChangeDynamicInitGainThresh(
	IN	PADAPTER	pAdapter,
	IN	u32		DM_Type,
	IN	u32		DM_Value)
{
	struct dm_priv *pdmpriv = &pAdapter->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;

	if (DM_Type == DIG_TYPE_THRESH_HIGH)
	{
		pDigTable->RssiHighThresh = DM_Value;		
	}
	else if (DM_Type == DIG_TYPE_THRESH_LOW)
	{
		pDigTable->RssiLowThresh = DM_Value;
	}
	else if (DM_Type == DIG_TYPE_ENABLE)
	{
		pDigTable->Dig_Enable_Flag = _TRUE;
	}	
	else if (DM_Type == DIG_TYPE_DISABLE)
	{
		pDigTable->Dig_Enable_Flag = _FALSE;
	}	
	else if (DM_Type == DIG_TYPE_BACKOFF)
	{
		if(DM_Value > 30)
			DM_Value = 30;
		pDigTable->BackoffVal = (u8)DM_Value;
	}
	else if(DM_Type == DIG_TYPE_RX_GAIN_MIN)
	{
		if(DM_Value == 0)
			DM_Value = 0x1;
		pDigTable->rx_gain_range_min = (u8)DM_Value;
	}
	else if(DM_Type == DIG_TYPE_RX_GAIN_MAX)
	{
		if(DM_Value > 0x50)
			DM_Value = 0x50;
		pDigTable->rx_gain_range_max = (u8)DM_Value;
	}
}	/* DM_ChangeDynamicInitGainThresh */


VOID PWDB_Monitor(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	//u8		i;
	int	tmpEntryMaxPWDB=0, tmpEntryMinPWDB=0xff;

	if(check_fwstate(&Adapter->mlmepriv, _FW_LINKED) != _TRUE)
		return;

#if 0 //todo:
	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		if(Adapter->MgntInfo.NdisVersion == RT_NDIS_VERSION_6_2)
		{
			if(ACTING_AS_AP(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE)))
				pEntry = AsocEntry_EnumStation(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE), i);
			else
				pEntry = AsocEntry_EnumStation(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, TRUE), i);
		}
		else
		{
			pEntry = AsocEntry_EnumStation(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, TRUE), i);	
		}

		if(pEntry!=NULL)
		{
			if(pEntry->bAssociated)
			{
				RTPRINT_ADDR(FDM, DM_PWDB, ("pEntry->MacAddr ="), pEntry->MacAddr);
				RTPRINT(FDM, DM_PWDB, ("pEntry->rssi = 0x%x(%d)\n", 
					pEntry->rssi_stat.UndecoratedSmoothedPWDB,
					pEntry->rssi_stat.UndecoratedSmoothedPWDB));
				if(pEntry->rssi_stat.UndecoratedSmoothedPWDB < tmpEntryMinPWDB)
					tmpEntryMinPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;
				if(pEntry->rssi_stat.UndecoratedSmoothedPWDB > tmpEntryMaxPWDB)
					tmpEntryMaxPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;
			}
		}
		else
		{
			break;
		}
	}
#endif

	if(tmpEntryMaxPWDB != 0)	// If associated entry is found
	{
		pdmpriv->EntryMaxUndecoratedSmoothedPWDB = tmpEntryMaxPWDB;		
	}
	else
	{
		pdmpriv->EntryMaxUndecoratedSmoothedPWDB = 0;
	}
	
	if(tmpEntryMinPWDB != 0xff) // If associated entry is found
	{
		pdmpriv->EntryMinUndecoratedSmoothedPWDB = tmpEntryMinPWDB;		
	}
	else
	{
		pdmpriv->EntryMinUndecoratedSmoothedPWDB = 0;
	}

#if 0
	// Indicate Rx signal strength to FW.
	if(Adapter->MgntInfo.bUseRAMask)
	{
		u4Byte	temp = 0;
		DbgPrint("RxSS: %x =%d\n", pHalData->UndecoratedSmoothedPWDB, pHalData->UndecoratedSmoothedPWDB);
		temp = pHalData->UndecoratedSmoothedPWDB;
		temp = temp << 16;
		//temp = temp | 0x800000;

		FillH2CCmd92C(Adapter, H2C_RSSI_REPORT, 3, (pu1Byte)(&temp));
	}
	else
	{
		PlatformEFIOWrite1Byte(Adapter, 0x4fe, (u1Byte)pHalData->UndecoratedSmoothedPWDB);
		//DbgPrint("0x4fe write %x %d\n", pHalData->UndecoratedSmoothedPWDB, pHalData->UndecoratedSmoothedPWDB);
	}
#else

	if(pHalData->fw_ractrl == _TRUE)
	{		
		u32 param = (u32)(pdmpriv->UndecoratedSmoothedPWDB<<16);

		//printk("######## RSSI(%d) ##############\n",pdmpriv->UndecoratedSmoothedPWDB);
		param |= 0;//macid=0 for sta mode;
		set_rssi_cmd(Adapter, (u8*)&param);
	}
#endif

}


void
DM_InitEdcaTurbo(
	IN	PADAPTER	Adapter
	)
{
#if 0
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	pHalData->bCurrentTurboEDCA = _FALSE;
	pHalData->bIsAnyNonBEPkts = _FALSE;
	pHalData->bIsCurRDLState = _FALSE;
#endif	
}	// DM_InitEdcaTurbo


void
dm_CheckEdcaTurbo(
	IN	PADAPTER	Adapter
	)
{
#if 0
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PSTA_QOS			pStaQos = Adapter->MgntInfo.pStaQos;

	// Keep past Tx/Rx packet count for RT-to-RT EDCA turbo.
	static u8Byte			lastTxOkCnt = 0;
	static u8Byte			lastRxOkCnt = 0;
	u8Byte				curTxOkCnt = 0;
	u8Byte				curRxOkCnt = 0;	
	u4Byte				EDCA_BE_UL = edca_setting_UL[pMgntInfo->IOTPeer];
	u4Byte				EDCA_BE_DL = edca_setting_DL[pMgntInfo->IOTPeer];
	u4Byte				EDCA_BE_DL_GMODE = edca_setting_DL_GMode[pMgntInfo->IOTPeer];
	static u4Byte			EDCA_BE_reg=0;
	static u1Byte			LastHighPwrLvl = 0xff;
	static u1Byte			LastBtOn = 0xff;
	static u1Byte			LastBtSco = 0xff;

#if (DEV_BUS_TYPE==DEV_BUS_PCI_INTERFACE)

	if( (pHalData->bt_coexist.BluetoothCoexist) &&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR) && 
		(pHalData->bt_coexist.BT_Ant_isolation))
	{
		if(LastHighPwrLvl != pHalData->DynamicTxHighPowerLvl ||
		    LastBtOn != pHalData->bt_coexist.BT_CUR_State ||
		    LastBtSco != pHalData->bt_coexist.BT_Service)
			pHalData->bCurrentTurboEDCA = FALSE;
		LastHighPwrLvl = pHalData->DynamicTxHighPowerLvl;
		LastBtOn = pHalData->bt_coexist.BT_CUR_State;
		LastBtSco = pHalData->bt_coexist.BT_Service;


		if(pHalData->DynamicTxHighPowerLvl == TxHighPwrLevel_Normal)
		{

			if(pHalData->bt_coexist.BT_Service==BT_OtherBusy)
			{
				RTPRINT(FBT, BT_TRACE, ("BT in BT_OtherBusy state Tx %d  \n",pHalData->bt_coexist.Ratio_Tx));
					//if(pHalData->bt_coexist.Ratio_Tx>350)
				{
						EDCA_BE_UL = 0x5ea72b;
						EDCA_BE_DL = 0x5ea72b;
/*
						//We set the EDCA every 2 seconds when Ratio_tx>350, other state are change with 6 seconds.
						//So the BT_Service state should change here.
						pHalData->bt_coexist.BT_Service=BT_OtherAction;
						if(pHalData->bCurrentTurboEDCA == TRUE)
							pHalData->bCurrentTurboEDCA = FALSE;
*/			
						RTPRINT(FBT, BT_TRACE, ("BT in BT_OtherBusy state Tx (%d) >350 parameter(0x%x) = 0x%x\n", REG_EDCA_BE_PARAM, 0x5ea72f));	
				}	
			}
			else if(pHalData->bt_coexist.BT_Service==BT_Busy)
			{
				EDCA_BE_UL = 0x5eb82f;
				EDCA_BE_DL = 0x5eb82f;
				RTPRINT(FBT, BT_TRACE, ("BT in BT_Busy state parameter(0x%x) = 0x%x\n", REG_EDCA_BE_PARAM, 0x5eb82f));		
			}
			else if(pHalData->bt_coexist.BT_Service==BT_SCO)
			{
				if(pHalData->bt_coexist.Ratio_Tx>160)
				{
					EDCA_BE_UL = 0x5ea72f;
					EDCA_BE_DL = 0x5ea72f;
					RTPRINT(FBT, BT_TRACE, ("BT in BT_SCO state Tx (%d) >160 parameter(0x%x) = 0x%x\n",pHalData->bt_coexist.Ratio_Tx, REG_EDCA_BE_PARAM, 0x5ea72f));
				}
				else
				{
					EDCA_BE_UL = 0x5ea32b;
					EDCA_BE_DL = 0x5ea42b;	
					RTPRINT(FBT, BT_TRACE, ("BT in BT_SCO state Tx (%d) <160 parameter(0x%x) = 0x%x\n", pHalData->bt_coexist.Ratio_Tx,REG_EDCA_BE_PARAM, 0x5ea32f));
				}

						
			}
			else if((pHalData->bt_coexist.BT_Service==BT_Idle) ||(pHalData->bt_coexist.BT_Service==BT_OtherAction))
			{

					//EDCA_BE_UL = 0x5ea32b;
					//EDCA_BE_DL = 0x5ea42b;
					RTPRINT(FBT, BT_TRACE, ("BT in BT_Idle  or BT_OtherAction state parameter(0x%x) = 0x%x\n", REG_EDCA_BE_PARAM, EDCA_BE_UL));		
			}		
			else
			{
				RTPRINT(FBT, BT_TRACE, ("BT in  unknow  %d\n",pHalData->bt_coexist.BT_Service));
			}
		}


/*
		else if((pHalData->DynamicTxHighPowerLvl == TxHighPwrLevel_Normal) &&
			(pHalData->bt_coexist.BT_CUR_State==1) &&
			(pHalData->bt_coexist.BT_Service==BT_Other))
		{
			//EDCA_BE_UL = 0x5ea72f;
			//EDCA_BE_DL = 0x5ea72f;
			//EDCA_BE_UL = 0x5eb82f;
			//EDCA_BE_DL = 0x5eb82f;	
			RTPRINT(FBT, BT_TRACE, ("BT None-SCO parameter(0x%x) = 0x%x\n", REG_EDCA_BE_PARAM, 0x5ea72f));
		}
*/
	}
	
#endif
	//
	// Do not be Turbo if it's under WiFi config and Qos Enabled, because the EDCA parameters 
	// should follow the settings from QAP. By Bruce, 2007-12-07.
	//
	if(Adapter->MgntInfo.bWiFiConfg)
		goto dm_CheckEdcaTurbo_EXIT;
	
	// 1. We do not turn on EDCA turbo mode for some AP that has IOT issue
	// 2. User may disable EDCA Turbo mode with OID settings.
	if((pMgntInfo->IOTAction & HT_IOT_ACT_DISABLE_EDCA_TURBO) ||pHalData->bForcedDisableTurboEDCA)
		goto dm_CheckEdcaTurbo_EXIT;

#if (DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE)
	if((!pMgntInfo->bDisableFrameBursting) && (!pHalData->bt_coexist.BluetoothCoexist) &&
		(pMgntInfo->IOTAction & (HT_IOT_ACT_FORCED_ENABLE_BE_TXOP|HT_IOT_ACT_AMSDU_ENABLE)))
#else
	if((!pMgntInfo->bDisableFrameBursting) &&
		(pMgntInfo->IOTAction & (HT_IOT_ACT_FORCED_ENABLE_BE_TXOP|HT_IOT_ACT_AMSDU_ENABLE)))
#endif
	{// To check whether we shall force turn on TXOP configuration.
		if(!(EDCA_BE_UL & 0xffff0000))
			EDCA_BE_UL |= 0x005e0000; // Force TxOP limit to 0x005e for UL.
		if(!(EDCA_BE_DL & 0xffff0000))
			EDCA_BE_DL |= 0x005e0000; // Force TxOP limit to 0x005e for DL.
	}
	
	// Check if the status needs to be changed.
	if((!pHalData->bIsAnyNonBEPkts) && (!pMgntInfo->bDisableFrameBursting))
	{
		RTPRINT(FDM, DM_EDCA_Turbo, ("Turn on Turbo EDCA \n"));
		//
		// Turn On EDCA turbo here. 
		// In this point, 2 condition needs to be checked:
		// (1) What peer STA do we link to.
		// (2) Check Tx/Rx count to determine if it is in uplink/downlink.
		// Use specific EDCA parameter for each different combination.
		//
		curTxOkCnt = Adapter->TxStats.NumTxBytesUnicast - lastTxOkCnt;
		curRxOkCnt = Adapter->RxStats.NumRxBytesUnicast - lastRxOkCnt;

		// Modify EDCA parameters selection bias
		// For some APs, use downlink EDCA parameters for uplink+downlink 
		if(pMgntInfo->IOTAction & HT_IOT_ACT_EDCA_BIAS_ON_RX)
		{
			RTPRINT(FDM, DM_EDCA_Turbo, ("EDCA IOT state : BIAS on Rx\n"));
			if(curTxOkCnt > 4*curRxOkCnt)
			{// Uplink TP is present.
				RTPRINT(FDM, DM_EDCA_Turbo, ("EDCA Current Uplink state\n"));
				if(pHalData->bIsCurRDLState || !pHalData->bCurrentTurboEDCA)
				{
					PlatformEFIOWrite4Byte(Adapter, REG_EDCA_BE_PARAM, EDCA_BE_UL);
					pHalData->bIsCurRDLState = FALSE;
				}
			}
			else
			{// Balance TP is present.
				RTPRINT(FDM, DM_EDCA_Turbo, ("EDCA Current Balance state\n"));
				if(!pHalData->bIsCurRDLState || !pHalData->bCurrentTurboEDCA)
				{
//					if(pMgntInfo->dot11CurrentWirelessMode == WIRELESS_MODE_G)
//						PlatformEFIOWrite4Byte(Adapter, REG_EDCA_BE_PARAM, EDCA_BE_DL_GMODE);
//					else
						PlatformEFIOWrite4Byte(Adapter, REG_EDCA_BE_PARAM, EDCA_BE_DL);
					pHalData->bIsCurRDLState = TRUE;
				}
			}
			pHalData->bCurrentTurboEDCA = TRUE;
		}
		else
		{// For generic IOT Action.
			RTPRINT(FDM, DM_EDCA_Turbo, ("EDCA IOT state : Generic\n"));
			if(curRxOkCnt > 4*curTxOkCnt)
			{// Downlink TP is present.
				RTPRINT(FDM, DM_EDCA_Turbo, ("EDCA Current Downlink state\n"));
				if(!pHalData->bIsCurRDLState || !pHalData->bCurrentTurboEDCA)
				{
//					if(pMgntInfo->dot11CurrentWirelessMode == WIRELESS_MODE_G)
//						PlatformEFIOWrite4Byte(Adapter, REG_EDCA_BE_PARAM, EDCA_BE_DL_GMODE);
//					else
						PlatformEFIOWrite4Byte(Adapter, REG_EDCA_BE_PARAM, EDCA_BE_DL);
					pHalData->bIsCurRDLState = TRUE;
				}
			}
			else
			{// Balance TP is present.
				RTPRINT(FDM, DM_EDCA_Turbo, ("EDCA Current Balance state\n"));
				if(pHalData->bIsCurRDLState || !pHalData->bCurrentTurboEDCA)
				{
					PlatformEFIOWrite4Byte(Adapter, REG_EDCA_BE_PARAM, EDCA_BE_UL);
					pHalData->bIsCurRDLState = FALSE;
				}
#if 0//(DEV_BUS_TYPE==DEV_BUS_PCI_INTERFACE)
				if( (pHalData->bt_coexist.BluetoothCoexist) &&
					(pHalData->bt_coexist.BT_CoexistType == BT_CSR) &&
					(pHalData->bt_coexist.BT_Ant_isolation) &&
					(pHalData->DynamicTxHighPowerLvl == TxHighPwrLevel_Normal))
				{
					EDCA_BE_reg = PlatformEFIORead4Byte(Adapter, REG_EDCA_BE_PARAM);
					RTPRINT(FBT, BT_TRACE, ("BT CSR check EDCA(0x%x) = 0x%x\n", REG_EDCA_BE_PARAM, EDCA_BE_reg));
					if(EDCA_BE_reg != EDCA_BE_UL)
					{
						PlatformEFIOWrite4Byte(Adapter, REG_EDCA_BE_PARAM, EDCA_BE_UL);
						RTPRINT(FBT, BT_TRACE, ("BT update parameter(0x%x) = 0x%x\n", REG_EDCA_BE_PARAM, EDCA_BE_UL));
					}
				}
#endif

			}
			pHalData->bCurrentTurboEDCA = TRUE;
		}
	}
	else
	{
		RTPRINT(FDM, DM_EDCA_Turbo, ("Turn off Turbo EDCA \n"));
		//
		// Turn Off EDCA turbo here.
		// Restore original EDCA according to the declaration of AP.
		//
		 if(pHalData->bCurrentTurboEDCA)
		{
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, GET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pStaQos->WMMParamEle, AC0_BE) );
			pHalData->bCurrentTurboEDCA = FALSE;
		}
	}

dm_CheckEdcaTurbo_EXIT:
	// Set variables for next time.
	pHalData->bIsAnyNonBEPkts = FALSE;
	lastTxOkCnt = Adapter->TxStats.NumTxBytesUnicast;
	lastRxOkCnt = Adapter->RxStats.NumRxBytesUnicast;
#endif	
}	// dm_CheckEdcaTurbo

#define		index_mapping_HP_NUM	15	
//091212 chiyokolin
static	VOID
dm_TXPowerTrackingCallback_ThermalMeter_92C(
            IN PADAPTER	Adapter)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	u8			ThermalValue = 0, delta, delta_LCK, delta_IQK,TimeOut = 100;
	int 			ele_A, ele_D, TempCCk, X, value32;
	int			Y, ele_C;
	s8			OFDM_index[2], CCK_index, OFDM_index_old[2], CCK_index_old;
	int i = 0,CCKSwingNeedUpdate = 0;
	BOOLEAN			is2T;
	//PMPT_CONTEXT	pMptCtx = &(Adapter->MptCtx);	
	//u8	*TxPwrLevel = pMptCtx->TxPwrLevel;//???
	u8	OFDM_min_index = 6, rf; //OFDM BB Swing should be less than +3.0dB, which is required by Arthur
	
	pdmpriv->TXPowerTrackingCallbackCnt++;	//cosa add for debug
	pdmpriv->bTXPowerTrackingInit = _TRUE;

	if(pHalData->CurrentChannel == 14 && !pHalData->bCCKinCH14)
		pHalData->bCCKinCH14 = _TRUE;
	else if(pHalData->CurrentChannel != 14 && pHalData->bCCKinCH14)
		pHalData->bCCKinCH14 = _FALSE;

	//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("===>dm_TXPowerTrackingCallback_ThermalMeter_92C\n"));

	ThermalValue = (u8)PHY_QueryRFReg(Adapter, RF90_PATH_A, RF_T_METER, 0x1f);	// 0x24: RF Reg[4:0]	

	/*

	printk("\n\nReadback Thermal Meter = 0x%x pre thermal meter 0x%x EEPROMthermalmeter 0x%x\n",\
			 ThermalValue,pdmpriv->ThermalValue,  pEEPROM->EEPROMThermalMeter);

	*/

	PHY_APCalibrate(Adapter, (ThermalValue - pEEPROM->EEPROMThermalMeter));//notes:EEPROMThermalMeter is a fixed value from efuse/eeprom

//	if(!pHalData->TxPowerTrackControl)
//		return;

	if(pHalData->rf_type == RF_1T1R)
	{
		rf = 1;
		is2T = _FALSE;
	}	
	else
	{
		rf = 2;
		is2T = _TRUE;
	}	
	
	if(ThermalValue)
	{
//		if(!pHalData->ThermalValue)
		{
			//Query OFDM path A default setting 		
			ele_D = PHY_QueryBBReg(Adapter, rOFDM0_XATxIQImbalance, bMaskDWord)&0xffc00000;
			for(i=0; i<OFDM_TABLE_SIZE; i++)	//find the index
			{
				if(ele_D == (OFDMSwingTable[i]&0xffc00000))
				{
					OFDM_index_old[0] = (s8)i;
					//printk("Initial pathA ele_D reg0x%x = 0x%x, OFDM_index=0x%x\n", rOFDM0_XATxIQImbalance, ele_D, OFDM_index_old[0]);
					break;
				}
			}

			//Query OFDM path B default setting 
			if(is2T)
			{
				ele_D = PHY_QueryBBReg(Adapter, rOFDM0_XBTxIQImbalance, bMaskDWord)&0xffc00000;
				for(i=0; i<OFDM_TABLE_SIZE; i++)	//find the index
				{
					if(ele_D == (OFDMSwingTable[i]&0xffc00000))
					{
						OFDM_index_old[1] = (s8)i;
						//printk("Initial pathB ele_D reg0x%x = 0x%x, OFDM_index=0x%x\n",rOFDM0_XBTxIQImbalance, ele_D, OFDM_index_old[1]);
						break;
					}
				}
			}

			//Query CCK default setting From 0xa24
			TempCCk = PHY_QueryBBReg(Adapter, rCCK0_TxFilter2, bMaskDWord)&0x3f3f3f3f;
			for(i=0 ; i<CCK_TABLE_SIZE ; i++)
			{
				if(pHalData->bCCKinCH14)
				{
					if(_rtw_memcmp((void*)&TempCCk, (void*)&CCKSwingTable_Ch14[i][2], 4)==_TRUE)
					{
						CCK_index_old =(s8)i;
						//printk("Initial reg0x%x = 0x%x, CCK_index=0x%x, ch 14 %d\n", rCCK0_TxFilter2, TempCCk, CCK_index_old, pHalData->bCCKinCH14);
						break;
					}
				}
				else
				{
					if(_rtw_memcmp((void*)&TempCCk, (void*)&CCKSwingTable_Ch1_Ch13[i][2], 4)==_TRUE)
					{
						CCK_index_old =(s8)i;
						//printk("Initial reg0x%x = 0x%x, CCK_index=0x%x, ch14 %d\n", rCCK0_TxFilter2, TempCCk, CCK_index_old, pHalData->bCCKinCH14);
						break;
					}			
				}
			}	

			if(!pdmpriv->ThermalValue)
			{
				pdmpriv->ThermalValue = pEEPROM->EEPROMThermalMeter;
				pdmpriv->ThermalValue_LCK = ThermalValue;				
				pdmpriv->ThermalValue_IQK = ThermalValue;								
				
				for(i = 0; i < rf; i++)
					pdmpriv->OFDM_index[i] = OFDM_index_old[i];
				
				pdmpriv->CCK_index = CCK_index_old;
				//printk("==>init_value , ThermalValue = 0x%x,CCK_Index = 0x%x\n",pdmpriv->ThermalValue,pdmpriv->CCK_index);
			}			
		}
		
		delta = (ThermalValue > pdmpriv->ThermalValue)?(ThermalValue - pdmpriv->ThermalValue):(pdmpriv->ThermalValue - ThermalValue);
		delta_LCK = (ThermalValue > pdmpriv->ThermalValue_LCK)?(ThermalValue - pdmpriv->ThermalValue_LCK):(pdmpriv->ThermalValue_LCK - ThermalValue);
		delta_IQK = (ThermalValue > pdmpriv->ThermalValue_IQK)?(ThermalValue - pdmpriv->ThermalValue_IQK):(pdmpriv->ThermalValue_IQK - ThermalValue);

		/*
		printk("Readback Thermal Meter = 0x%x EEPROMthermalmeter 0x%x delta 0x%x delta_LCK 0x%x delta_IQK 0x%x\n", \
				ThermalValue, pEEPROM->EEPROMThermalMeter, delta, delta_LCK, delta_IQK);

		*/


		if(delta_LCK > 1)
		{
			pdmpriv->ThermalValue_LCK = ThermalValue;
			PHY_LCCalibrate(Adapter);
		}
		
		if(delta > 0 && pdmpriv->TxPowerTrackControl)
		{
			if(ThermalValue > pdmpriv->ThermalValue)
			{ 
				for(i = 0; i < rf; i++)
				 	pdmpriv->OFDM_index[i] -= delta;
				
				pdmpriv->CCK_index -= delta;
			}
			else
			{
				for(i = 0; i < rf; i++)			
					pdmpriv->OFDM_index[i] += delta;
				
				pdmpriv->CCK_index += delta;
			}

	/*		
			if(is2T)
			{
				printk("temp OFDM_A_index=0x%x, OFDM_B_index=0x%x, CCK_index=0x%x\n", 
					pdmpriv->OFDM_index[0], pdmpriv->OFDM_index[1], pdmpriv->CCK_index);			
			}
			else
			{
				//printk("temp OFDM_A_index=0x%x, CCK_index=0x%x\n",pdmpriv->OFDM_index[0], pdmpriv->CCK_index);			
			}
	*/		
			
			if(ThermalValue > pEEPROM->EEPROMThermalMeter)
			{
				for(i = 0; i < rf; i++)			
					OFDM_index[i] = pdmpriv->OFDM_index[i]+1;
				
				CCK_index = pdmpriv->CCK_index+1;			
			}
			else
			{
				for(i = 0; i < rf; i++)			
					OFDM_index[i] = pdmpriv->OFDM_index[i];
				
				CCK_index = pdmpriv->CCK_index;						
			}

#if 0 //todo:
			for(i = 0; i < rf; i++)
			{
				if(TxPwrLevel[i] >=0 && TxPwrLevel[i] <=26)
				{
					if(ThermalValue > pEEPROM->EEPROMThermalMeter)
					{
						if (delta < 5)
							OFDM_index[i] -= 1;					
						else 
							OFDM_index[i] -= 2;					
					}
					else if(delta > 5 && ThermalValue < pEEPROM->EEPROMThermalMeter)
					{
						OFDM_index[i] += 1;
					}
				}
				else if (TxPwrLevel[i] >= 27 && TxPwrLevel[i] <= 32 && ThermalValue > pEEPROM->EEPROMThermalMeter)
				{
					if (delta < 5)
						OFDM_index[i] -= 1;					
					else 
						OFDM_index[i] -= 2;								
				}
				else if (TxPwrLevel[i] >= 32 && TxPwrLevel[i] <= 38 && ThermalValue > pEEPROM->EEPROMThermalMeter && delta > 5)
				{
					OFDM_index[i] -= 1;								
				}
			}

			{
				if(TxPwrLevel[i] >=0 && TxPwrLevel[i] <=26)
				{
					if(ThermalValue > pEEPROM->EEPROMThermalMeter)
					{
						if (delta < 5)
							CCK_index -= 1; 				
						else 
							CCK_index -= 2; 				
					}
					else if(delta > 5 && ThermalValue < pEEPROM->EEPROMThermalMeter)
					{
						CCK_index += 1;
					}
				}
				else if (TxPwrLevel[i] >= 27 && TxPwrLevel[i] <= 32 && ThermalValue > pEEPROM->EEPROMThermalMeter)
				{
					if (delta < 5)
						CCK_index -= 1; 				
					else 
						CCK_index -= 2; 							
				}
				else if (TxPwrLevel[i] >= 32 && TxPwrLevel[i] <= 38 && ThermalValue > pEEPROM->EEPROMThermalMeter && delta > 5)
				{
					CCK_index -= 1; 							
				}
			}
#endif

			for(i = 0; i < rf; i++)
			{
				if(OFDM_index[i] > OFDM_TABLE_SIZE-1)
					OFDM_index[i] = OFDM_TABLE_SIZE-1;
				else if (OFDM_index[i] < OFDM_min_index)
					OFDM_index[i] = OFDM_min_index;
			}
						
			if(CCK_index > CCK_TABLE_SIZE-1)
				CCK_index = CCK_TABLE_SIZE-1;
			else if (CCK_index < 0)
				CCK_index = 0;		

	/*		
			if(is2T)
			{
				printk("new OFDM_A_index=0x%x, OFDM_B_index=0x%x, CCK_index=0x%x\n", OFDM_index[0], OFDM_index[1], CCK_index);
			}
			else
			{
				//printk("new OFDM_A_index=0x%x, CCK_index=0x%x\n",	OFDM_index[0], CCK_index);			
			}
	*/
			
		}
		else if (delta == 0)
		{
			//return;
		}

		if(pdmpriv->TxPowerTrackControl && delta != 0)
		{
			//Adujst OFDM Ant_A according to IQK result
			ele_D = (OFDMSwingTable[(u8)OFDM_index[0]] & 0xFFC00000)>>22;
			//X = (PHY_QueryBBReg(Adapter, 0xe94, bMaskDWord) >> 16) & 0x3FF;
			//Y = (PHY_QueryBBReg(Adapter, 0xe9c, bMaskDWord) >> 16) & 0x3FF;		
			X = pHalData->RegE94;
			Y = pHalData->RegE9C;		
if(X != 0)
			{
				if ((X & 0x00000200) != 0)
					X = X | 0xFFFFFC00;
				ele_A = ((X * ele_D)>>8)&0x000003FF;
					
				//new element C = element D x Y
				if ((Y & 0x00000200) != 0)
					Y = Y | 0xFFFFFC00;
				ele_C = ((Y * ele_D)>>8)&0x000003FF;
				
				//wirte new elements A, C, D to regC80 and regC94, element B is always 0
				value32 = (ele_D<<22)|((ele_C&0x3F)<<16)|ele_A;
				PHY_SetBBReg(Adapter, rOFDM0_XATxIQImbalance, bMaskDWord, value32);
				
				value32 = (ele_C&0x000003C0)>>6;
				PHY_SetBBReg(Adapter, rOFDM0_XCTxAFE, bMaskH4Bits, value32);

				value32 = ((X * ele_D)>>7)&0x01;
				PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT31, value32);

				value32 = ((Y * ele_D)>>7)&0x01;
				PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT29, value32);
				
			}
			else
			{
				PHY_SetBBReg(Adapter, rOFDM0_XATxIQImbalance, bMaskDWord, OFDMSwingTable[OFDM_index[0]]);				
				PHY_SetBBReg(Adapter, rOFDM0_XCTxAFE, bMaskH4Bits, 0x00);
				PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT31|BIT29, 0x00);			
			}

			//RTPRINT(FINIT, INIT_IQK, ("TxPwrTracking path A: X = 0x%x, Y = 0x%x ele_A = 0x%x ele_C = 0x%x ele_D = 0x%x\n", X, Y, ele_A, ele_C, ele_D));		

			//Adjust CCK according to IQK result
			if(!pHalData->bCCKinCH14){
				rtw_write8(Adapter, 0xa22, CCKSwingTable_Ch1_Ch13[(u8)CCK_index][0]);
				rtw_write8(Adapter, 0xa23, CCKSwingTable_Ch1_Ch13[(u8)CCK_index][1]);
				rtw_write8(Adapter, 0xa24, CCKSwingTable_Ch1_Ch13[(u8)CCK_index][2]);
				rtw_write8(Adapter, 0xa25, CCKSwingTable_Ch1_Ch13[(u8)CCK_index][3]);
				rtw_write8(Adapter, 0xa26, CCKSwingTable_Ch1_Ch13[(u8)CCK_index][4]);
				rtw_write8(Adapter, 0xa27, CCKSwingTable_Ch1_Ch13[(u8)CCK_index][5]);
				rtw_write8(Adapter, 0xa28, CCKSwingTable_Ch1_Ch13[(u8)CCK_index][6]);
				rtw_write8(Adapter, 0xa29, CCKSwingTable_Ch1_Ch13[(u8)CCK_index][7]);		

/*
				printk("Adjust CCK_index=0x%02x 0xa22=0x%02x, 0xa23=0x%02x,0xa24=0x%02x,0xa25=0x%02x,0xa26=0x%02x,0xa27=0x%02x,0xa28=0x%02x,0xa29=0x%02x\n", 
					CCK_index,CCKSwingTable_Ch1_Ch13[(u8)CCK_index][0],CCKSwingTable_Ch1_Ch13[(u8)CCK_index][1],
					CCKSwingTable_Ch1_Ch13[(u8)CCK_index][2],CCKSwingTable_Ch1_Ch13[(u8)CCK_index][3],
					CCKSwingTable_Ch1_Ch13[(u8)CCK_index][4],CCKSwingTable_Ch1_Ch13[(u8)CCK_index][5],
					CCKSwingTable_Ch1_Ch13[(u8)CCK_index][6],CCKSwingTable_Ch1_Ch13[(u8)CCK_index][7]);
*/					
			}
			else{
				rtw_write8(Adapter, 0xa22, CCKSwingTable_Ch14[(u8)CCK_index][0]);
				rtw_write8(Adapter, 0xa23, CCKSwingTable_Ch14[(u8)CCK_index][1]);
				rtw_write8(Adapter, 0xa24, CCKSwingTable_Ch14[(u8)CCK_index][2]);
				rtw_write8(Adapter, 0xa25, CCKSwingTable_Ch14[(u8)CCK_index][3]);
				rtw_write8(Adapter, 0xa26, CCKSwingTable_Ch14[(u8)CCK_index][4]);
				rtw_write8(Adapter, 0xa27, CCKSwingTable_Ch14[(u8)CCK_index][5]);
				rtw_write8(Adapter, 0xa28, CCKSwingTable_Ch14[(u8)CCK_index][6]);
				rtw_write8(Adapter, 0xa29, CCKSwingTable_Ch14[(u8)CCK_index][7]);	
			}		

			if(is2T)
			{						
				ele_D = (OFDMSwingTable[(u8)OFDM_index[1]] & 0xFFC00000)>>22;
				
				//new element A = element D x X
				//X = (PHY_QueryBBReg(Adapter, 0xeb4, bMaskDWord) >> 16) & 0x3FF;
				//Y = (PHY_QueryBBReg(Adapter, 0xebc, bMaskDWord) >> 16) & 0x3FF;
				X = pHalData->RegEB4;
				Y = pHalData->RegEBC;
				
				if(X != 0){
					if ((X & 0x00000200) != 0)	//consider minus
						X = X | 0xFFFFFC00;
					ele_A = ((X * ele_D)>>8)&0x000003FF;
					
					//new element C = element D x Y
					if ((Y & 0x00000200) != 0)
						Y = Y | 0xFFFFFC00;
					ele_C = ((Y * ele_D)>>8)&0x00003FF;
					
					//wirte new elements A, C, D to regC88 and regC9C, element B is always 0
					value32=(ele_D<<22)|((ele_C&0x3F)<<16) |ele_A;
					PHY_SetBBReg(Adapter, rOFDM0_XBTxIQImbalance, bMaskDWord, value32);

					value32 = (ele_C&0x000003C0)>>6;
					PHY_SetBBReg(Adapter, rOFDM0_XDTxAFE, bMaskH4Bits, value32);	
					
					value32 = ((X * ele_D)>>7)&0x01;
					PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT27, value32);

					value32 = ((Y * ele_D)>>7)&0x01;
					PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT25, value32);

				}
				else{
					PHY_SetBBReg(Adapter, rOFDM0_XBTxIQImbalance, bMaskDWord, OFDMSwingTable[OFDM_index[1]]);										
					PHY_SetBBReg(Adapter, rOFDM0_XDTxAFE, bMaskH4Bits, 0x00);	
					PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT27|BIT25, 0x00);				
				}
/*
				printk("TxPwrTracking path B: X = 0x%x, Y = 0x%x ele_A = 0x%x ele_C = 0x%x ele_D = 0x%x\n", \
											X, Y, ele_A, ele_C, ele_D);			
*/
			}


			/*

			printk("TxPwrTracking 0xc80 = 0x%x, 0xc94 = 0x%x RF 0x24 = 0x%x\n", \
					PHY_QueryBBReg(Adapter, 0xc80, bMaskDWord),\
					PHY_QueryBBReg(Adapter, 0xc94, bMaskDWord), \
					PHY_QueryRFReg(Adapter, RF90_PATH_A, 0x24, bMaskDWord));
			*/

		}
		
		if(delta_IQK > 3)
		{
			pdmpriv->ThermalValue_IQK = ThermalValue;
			PHY_IQCalibrate(Adapter,_FALSE);
		}

		//update thermal meter value
		//if(pdmpriv->TxPowerTrackControl)
		//	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_THERMAL_VALUE, &ThermalValue);//???
		pdmpriv->ThermalValue = ThermalValue;
			
	}

	//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("<===dm_TXPowerTrackingCallback_ThermalMeter_92C\n"));
	
	pdmpriv->TXPowercount = 0;

}


static	VOID
dm_InitializeTXPowerTracking_ThermalMeter(
	IN	PADAPTER		Adapter)
{	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv *pdmpriv = &Adapter->dmpriv;

	//pMgntInfo->bTXPowerTracking = _TRUE;
	pdmpriv->TXPowercount = 0;
	pdmpriv->bTXPowerTrackingInit = _FALSE;
	pdmpriv->ThermalValue = 0;
	
#if	(MP_DRIVER != 1)	//for mp driver, turn off txpwrtracking as default
	pdmpriv->TxPowerTrackControl = _TRUE;
#endif
	
	MSG_8192C("pdmpriv->TxPowerTrackControl = %d\n", pdmpriv->TxPowerTrackControl);
}


VOID
DM_InitializeTXPowerTracking(
	IN	PADAPTER		Adapter)
{
	dm_InitializeTXPowerTracking_ThermalMeter(Adapter);	
}	

//
//	Description:
//		- Dispatch TxPower Tracking direct call ONLY for 92s.
//		- We shall NOT schedule Workitem within PASSIVE LEVEL, which will cause system resource
//		   leakage under some platform.
//
//	Assumption:
//		PASSIVE_LEVEL when this routine is called.
//
//	Added by Roger, 2009.06.18.
//
VOID
DM_TXPowerTracking92CDirectCall(
            IN	PADAPTER		Adapter)
{	
	dm_TXPowerTrackingCallback_ThermalMeter_92C(Adapter);
}

VOID
dm_CheckTXPowerTracking_ThermalMeter(
	IN	PADAPTER		Adapter)
{	
	//HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv *pdmpriv = &Adapter->dmpriv;	
	//u1Byte					TxPowerCheckCnt = 5;	//10 sec

	//if(!pMgntInfo->bTXPowerTracking /*|| (!pHalData->TxPowerTrackControl && pHalData->bAPKdone)*/)
	if(!(pdmpriv->DMFlag & DYNAMIC_FUNC_SS))
	{
		return;
	}

	if(!pdmpriv->TM_Trigger)		//at least delay 1 sec
	{
		//pHalData->TxPowerCheckCnt++;	//cosa add for debug
		PHY_SetRFReg(Adapter, RF90_PATH_A, RF_T_METER, bRFRegOffsetMask, 0x60);
		//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Trigger 92C Thermal Meter!!\n"));
		
		pdmpriv->TM_Trigger = 1;
		return;
		
	}
	else
	{
		//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Schedule TxPowerTracking direct call!!\n"));		
		DM_TXPowerTracking92CDirectCall(Adapter); //Using direct call is instead, added by Roger, 2009.06.18.
		pdmpriv->TM_Trigger = 0;
	}

}


VOID
dm_CheckTXPowerTracking(
	IN	PADAPTER		Adapter)
{
	dm_CheckTXPowerTracking_ThermalMeter(Adapter);
}

#ifdef CONFIG_BT_COEXIST
BOOLEAN BT_BTStateChange(PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct btcoexist_priv	 *pbtpriv = &(Adapter->halpriv.bt_coexist);
	struct registry_priv  *registry_par = &Adapter->registrypriv;
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	
	struct mlme_priv *pmlmepriv = &(Adapter->mlmepriv);
	
	u32 		Polling, Ratio_Tx, Ratio_PRI;
	u32 			BT_Tx, BT_PRI;
	u8			BT_State;

	u8			CurServiceType;
	
	if(check_fwstate(pmlmepriv, _FW_LINKED) == _FALSE)	
		return _FALSE;
	
	BT_State = rtw_read8(Adapter, 0x4fd);
/*
	temp = PlatformEFIORead4Byte(Adapter, 0x488);
	BT_Tx = (u2Byte)(((temp<<8)&0xff00)+((temp>>8)&0xff));
	BT_PRI = (u2Byte)(((temp>>8)&0xff00)+((temp>>24)&0xff));

	temp = PlatformEFIORead4Byte(Adapter, 0x48c);
	Polling = ((temp<<8)&0xff000000) + ((temp>>8)&0x00ff0000) + 
			((temp<<8)&0x0000ff00) + ((temp>>8)&0x000000ff);
	
*/
	BT_Tx = rtw_read32(Adapter, 0x488);
	
	printk("Ratio 0x488  =%x\n", BT_Tx);
	BT_Tx =BT_Tx & 0x00ffffff;
	//RTPRINT(FBT, BT_TRACE, ("Ratio BT_Tx  =%x\n", BT_Tx));

	BT_PRI = rtw_read32(Adapter, 0x48c);
	
	printk("Ratio 0x48c  =%x\n", BT_PRI);
	BT_PRI =BT_PRI & 0x00ffffff;
	//RTPRINT(FBT, BT_TRACE, ("Ratio BT_PRI  =%x\n", BT_PRI));


	Polling = rtw_read32(Adapter, 0x490);
	//RTPRINT(FBT, BT_TRACE, ("Ratio 0x490  =%x\n", Polling));


	if(BT_Tx==0xffffffff && BT_PRI==0xffffffff && Polling==0xffffffff && BT_State==0xff)
		return _FALSE;

	BT_State &= BIT0;

	if(BT_State != pbtpriv->BT_CUR_State)
	{
		pbtpriv->BT_CUR_State = BT_State;
	
		if(registry_par->bt_sco == 3)
		{
			pdmpriv->BT_ServiceTypeCnt = 0;
		
			pbtpriv->BT_Service = BT_Idle;

			printk("BT_%s\n", BT_State?"ON":"OFF");

			BT_State = BT_State | 
					((pbtpriv->BT_Ant_isolation==1)?0:BIT1) |BIT2;

			rtw_write8(Adapter, 0x4fd, BT_State);
			printk("BT set 0x4fd to %x\n", BT_State);
		}
		
		return _TRUE;
	}
	printk("bRegBT_Sco =  %d\n",registry_par->bt_sco);

	Ratio_Tx = BT_Tx*1000/Polling;
	Ratio_PRI = BT_PRI*1000/Polling;

	pbtpriv->Ratio_Tx=Ratio_Tx;
	pbtpriv->Ratio_PRI=Ratio_PRI;
	
	printk("Ratio_Tx=%d\n", Ratio_Tx);
	printk("Ratio_PRI=%d\n", Ratio_PRI);

	
	if(BT_State && registry_par->bt_sco==3)
	{
		printk("bt_sco  ==3 Follow Counter\n");
//		if(BT_Tx==0xffff && BT_PRI==0xffff && Polling==0xffffffff)
//		{
//			ServiceTypeCnt = 0;
//			return FALSE;
//		}
//		else
		{
		/*
			Ratio_Tx = BT_Tx*1000/Polling;
			Ratio_PRI = BT_PRI*1000/Polling;

			pHalData->bt_coexist.Ratio_Tx=Ratio_Tx;
			pHalData->bt_coexist.Ratio_PRI=Ratio_PRI;
			
			RTPRINT(FBT, BT_TRACE, ("Ratio_Tx=%d\n", Ratio_Tx));
			RTPRINT(FBT, BT_TRACE, ("Ratio_PRI=%d\n", Ratio_PRI));

		*/	
			if((Ratio_Tx < 30)  && (Ratio_PRI < 30)) 
			  	CurServiceType = BT_Idle;
			else if((Ratio_PRI > 110) && (Ratio_PRI < 250))
				CurServiceType = BT_SCO;
			else if((Ratio_Tx >= 200)&&(Ratio_PRI >= 200))
				CurServiceType = BT_Busy;
			else if((Ratio_Tx >=350) && (Ratio_Tx < 500))
				CurServiceType = BT_OtherBusy;
			else if(Ratio_Tx >=500)
				CurServiceType = BT_PAN;
			else
				CurServiceType=BT_OtherAction;
		}

/*		if(pHalData->bt_coexist.bStopCount)
		{
			ServiceTypeCnt=0;
			pHalData->bt_coexist.bStopCount=FALSE;
		}
*/
//		if(CurServiceType == BT_OtherBusy)
		{
			pdmpriv->BT_ServiceTypeCnt=2;
			pdmpriv->BT_LastServiceType=CurServiceType;
		}
#if 0
		else if(CurServiceType == LastServiceType)
		{
			if(ServiceTypeCnt<3)
				ServiceTypeCnt++;
		}
		else
		{
			ServiceTypeCnt = 0;
			LastServiceType = CurServiceType;
		}
#endif

		if(pdmpriv->BT_ServiceTypeCnt==2)
		{
			pbtpriv->BT_Service = pdmpriv->BT_LastServiceType;
			BT_State = BT_State | 
					((pbtpriv->BT_Ant_isolation==1)?0:BIT1) |
					//((pbtpriv->BT_Service==BT_SCO)?0:BIT2);
					((pbtpriv->BT_Service!=BT_Idle)?0:BIT2);

			//if(pbtpriv->BT_Service==BT_Busy)
			//	BT_State&= ~(BIT2);

			if(pbtpriv->BT_Service==BT_SCO)
			{
				printk("BT TYPE Set to  ==> BT_SCO\n");
			}
			else if(pbtpriv->BT_Service==BT_Idle)
			{
				printk("BT TYPE Set to  ==> BT_Idle\n");
			}
			else if(pbtpriv->BT_Service==BT_OtherAction)
			{
				printk("BT TYPE Set to  ==> BT_OtherAction\n");
			}
			else if(pbtpriv->BT_Service==BT_Busy)
			{
				printk("BT TYPE Set to  ==> BT_Busy\n");
			}
			else if(pbtpriv->BT_Service==BT_PAN)
			{
				printk("BT TYPE Set to  ==> BT_PAN\n");
			}
			else
			{
				printk("BT TYPE Set to ==> BT_OtherBusy\n");
			}
				
			//Add interrupt migration when bt is not in idel state (no traffic).
			//suggestion by Victor.
			if(pbtpriv->BT_Service!=BT_Idle)//EDCA_VI_PARAM modify
			{
			
				rtw_write16(Adapter, 0x504, 0x0ccc);
				rtw_write8(Adapter, 0x506, 0x54);
				rtw_write8(Adapter, 0x507, 0x54);
			
			}
			else
			{
				rtw_write8(Adapter, 0x506, 0x00);
				rtw_write8(Adapter, 0x507, 0x00);			
			}
				
			rtw_write8(Adapter, 0x4fd, BT_State);
			printk("BT_SCO set 0x4fd to %x\n", BT_State);
			return _TRUE;
		}
	}

	return _FALSE;

}

BOOLEAN
BT_WifiConnectChange(
	IN	PADAPTER	Adapter
	)
{
	struct mlme_priv *pmlmepriv = &(Adapter->mlmepriv);
//	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	struct dm_priv *pdmpriv = &Adapter->dmpriv;	

	//if(!pMgntInfo->bMediaConnect || MgntRoamingInProgress(pMgntInfo))
	if(check_fwstate(pmlmepriv, _FW_LINKED) == _FALSE)	
	{
		pdmpriv->BT_bMediaConnect = _FALSE;
	}
	else
	{
		if(!pdmpriv->BT_bMediaConnect)
		{
			pdmpriv->BT_bMediaConnect = _TRUE;
			return _TRUE;
		}
		pdmpriv->BT_bMediaConnect = _TRUE;
	}

	return _FALSE;
}

#define BT_RSSI_STATE_NORMAL_POWER	BIT0
#define BT_RSSI_STATE_AMDPU_OFF		BIT1
#define BT_RSSI_STATE_SPECIAL_LOW	BIT2
#define BT_RSSI_STATE_BG_EDCA_LOW	BIT3

s32 GET_UNDECORATED_AVERAGE_RSSI(PADAPTER	Adapter)	
{
	struct mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
	struct dm_priv 		*pdmpriv = &Adapter->dmpriv;
	s32 	average_rssi;
	
	if(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE|WIFI_ADHOC_MASTER_STATE|WIFI_AP_STATE))
	{	
		average_rssi = 	pdmpriv->EntryMinUndecoratedSmoothedPWDB;	
	}
	else
	{
		average_rssi = 	pdmpriv->UndecoratedSmoothedPWDB;
	}
	return average_rssi;
}
u8
BT_RssiStateChange(
	IN	PADAPTER	Adapter
	)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv	 *pmlmepriv = &(Adapter->mlmepriv);
	struct btcoexist_priv	 *pbtpriv = &(Adapter->halpriv.bt_coexist);
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	//PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	s32			UndecoratedSmoothedPWDB;
	u8			CurrBtRssiState = 0x00;

	//if(pMgntInfo->bMediaConnect)	// Default port
	if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)	
	{
		UndecoratedSmoothedPWDB = GET_UNDECORATED_AVERAGE_RSSI(Adapter);
	}
	else // associated entry pwdb
	{
		if(pdmpriv->EntryMinUndecoratedSmoothedPWDB == 0)
			UndecoratedSmoothedPWDB = 100;	// No any RSSI information. Assume to be MAX.
		else
			UndecoratedSmoothedPWDB = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
	}

	// Check RSSI to determine HighPower/NormalPower state for BT coexistence.
	if(UndecoratedSmoothedPWDB >= 67)
		CurrBtRssiState &= (~BT_RSSI_STATE_NORMAL_POWER);
	else if(UndecoratedSmoothedPWDB < 62)
		CurrBtRssiState |= BT_RSSI_STATE_NORMAL_POWER;

	// Check RSSI to determine AMPDU setting for BT coexistence.
	if(UndecoratedSmoothedPWDB >= 40)
		CurrBtRssiState &= (~BT_RSSI_STATE_AMDPU_OFF);
	else if(UndecoratedSmoothedPWDB <= 32)
		CurrBtRssiState |= BT_RSSI_STATE_AMDPU_OFF;

	// Marked RSSI state. It will be used to determine BT coexistence setting later.
	if(UndecoratedSmoothedPWDB < 35)
		CurrBtRssiState |=  BT_RSSI_STATE_SPECIAL_LOW;
	else
		CurrBtRssiState &= (~BT_RSSI_STATE_SPECIAL_LOW);

	// Check BT state related to BT_Idle in B/G mode.
	if(UndecoratedSmoothedPWDB < 15)
		CurrBtRssiState |=  BT_RSSI_STATE_BG_EDCA_LOW;
	else
		CurrBtRssiState &= (~BT_RSSI_STATE_BG_EDCA_LOW);
	
	if(CurrBtRssiState != pbtpriv->BtRssiState)
	{
		pbtpriv->BtRssiState = CurrBtRssiState;
		return _TRUE;
	}
	else
	{
		return _FALSE;
	}
}

void dm_BTCoexist(PADAPTER Adapter )
{
	HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv	 		*pmlmepriv = &(Adapter->mlmepriv);
	struct mlme_ext_info		*pmlmeinfo = &Adapter->mlmeextpriv.mlmext_info;
	struct mlme_ext_priv		*pmlmeext = &Adapter->mlmeextpriv;

	struct btcoexist_priv	 		*pbtpriv = &(Adapter->halpriv.bt_coexist);
	//PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	//PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	
	//PRX_TS_RECORD	pRxTs = NULL;
	u8			BT_gpio_mux;

	BOOLEAN		bWifiConnectChange, bBtStateChange,bRssiStateChange;

	if(pbtpriv->bCOBT == _FALSE)		return;
	if( (pbtpriv->BT_Coexist) &&((pbtpriv->BT_CoexistType == BT_CSR_BC4) ||(pbtpriv->BT_CoexistType == BT_CSR_BC8)) && (check_fwstate(pmlmepriv, WIFI_AP_STATE) == _FALSE)	)
	{
		bWifiConnectChange = BT_WifiConnectChange(Adapter);
		bBtStateChange	= BT_BTStateChange(Adapter);
		bRssiStateChange 	= BT_RssiStateChange(Adapter);
		
		printk("bWifiConnectChange %d, bBtStateChange  %d,bRssiStateChange  %d\n",
			bWifiConnectChange,bBtStateChange,bRssiStateChange);

		// add by hpfan for debug message
		BT_gpio_mux = rtw_read8(Adapter, REG_GPIO_MUXCFG);
		printk("BTCoexit Reg_0x40 (%2x)\n", BT_gpio_mux);

		if( bWifiConnectChange ||bBtStateChange  ||bRssiStateChange )
		{
			if(pbtpriv->BT_CUR_State)
			{
				
				// Do not allow receiving A-MPDU aggregation.
				if(pbtpriv->BT_Ampdu)// 0:Disable BT control A-MPDU, 1:Enable BT control A-MPDU.
				{
			
					if(pmlmeinfo->assoc_AP_vendor == ciscoAP)	
					{
						if(pbtpriv->BT_Service!=BT_Idle)
						{
							if(pmlmeinfo->bAcceptAddbaReq)
							{
								printk("BT_Disallow AMPDU \n");	
								pmlmeinfo->bAcceptAddbaReq = _FALSE;
								send_delba(Adapter,0, get_my_bssid(&(pmlmeinfo->network)));
							}
						}
						else
						{
							if(!pmlmeinfo->bAcceptAddbaReq)
							{
								printk("BT_Allow AMPDU  RSSI >=40\n");	
								pmlmeinfo->bAcceptAddbaReq = _TRUE;
							}
						}
					}
					else
					{
						if(!pmlmeinfo->bAcceptAddbaReq)
						{
							printk("BT_Allow AMPDU BT Idle\n");	
							pmlmeinfo->bAcceptAddbaReq = _TRUE;
						}
					}
				}
				
#if 0
				else if((pHalData->bt_coexist.BT_Service==BT_SCO) || (pHalData->bt_coexist.BT_Service==BT_Busy))
				{				
					if(pHalData->bt_coexist.BtRssiState & BT_RSSI_STATE_AMDPU_OFF)
					{
						if(pMgntInfo->bBT_Ampdu && pHTInfo->bAcceptAddbaReq)
						{
							RTPRINT(FBT, BT_TRACE, ("BT_Disallow AMPDU RSSI <=32\n"));	
							pHTInfo->bAcceptAddbaReq = FALSE;
							if(GetTs(Adapter, (PTS_COMMON_INFO*)(&pRxTs), pMgntInfo->Bssid, 0, RX_DIR, FALSE))
								TsInitDelBA(Adapter, (PTS_COMMON_INFO)pRxTs, RX_DIR);
						}
					}
					else
					{
						if(pMgntInfo->bBT_Ampdu && !pHTInfo->bAcceptAddbaReq)
						{
							RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU  RSSI >=40\n"));	
							pHTInfo->bAcceptAddbaReq = TRUE;
						}
					}
				}
				else
				{
					if(pMgntInfo->bBT_Ampdu && !pHTInfo->bAcceptAddbaReq)
					{
						RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU BT not in SCO or BUSY\n"));	
						pHTInfo->bAcceptAddbaReq = TRUE;
					}
				}
#endif

				if(pbtpriv->BT_Ant_isolation)
				{			
					printk("BT_IsolationLow\n");

// 20100427 Joseph: Do not adjust Rate adaptive for BT coexist suggested by SD3.
#if 0
					RTPRINT(FBT, BT_TRACE, ("BT_Update Rate table\n"));
					if(pMgntInfo->bUseRAMask)
					{
						// 20100407 Joseph: Fix rate adaptive modification for BT coexist.
						// This fix is not complete yet. It shall also consider VWifi and Adhoc case,
						// which connect with multiple STAs.
						Adapter->HalFunc.UpdateHalRAMaskHandler(
												Adapter,
												FALSE,
												0,
												NULL,
												NULL,
												pMgntInfo->RateAdaptive.RATRState,
												RAMask_Normal);
					}
					else
					{
						Adapter->HalFunc.UpdateHalRATRTableHandler(
									Adapter, 
									&pMgntInfo->dot11OperationalRateSet,
									pMgntInfo->dot11HTOperationalRateSet,NULL);
					}
#endif

					// 20100415 Joseph: Modify BT coexist mechanism suggested by Yaying.
					// Now we only enable HW BT coexist when BT in "Busy" state.
					if(1)//pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod >= 20)
					{
						if((pmlmeinfo->assoc_AP_vendor == ciscoAP)	&&
							pbtpriv->BT_Service==BT_OtherAction)
						{
							printk("BT_Turn ON Coexist\n");
							rtw_write8(Adapter, REG_GPIO_MUXCFG, 0xa0);	
						}
						else
						{
							if((pbtpriv->BT_Service==BT_Busy) &&
								(pbtpriv->BtRssiState & BT_RSSI_STATE_NORMAL_POWER))
							{
								printk("BT_Turn ON Coexist\n");
								rtw_write8(Adapter, REG_GPIO_MUXCFG, 0xa0);
							}
							else if((pbtpriv->BT_Service==BT_OtherAction) &&
									(pbtpriv->BtRssiState & BT_RSSI_STATE_SPECIAL_LOW))
							{
								printk("BT_Turn ON Coexist\n");
								rtw_write8(Adapter, REG_GPIO_MUXCFG, 0xa0);	
							}
							else if(pbtpriv->BT_Service==BT_PAN)
							{
								printk("BT_Turn ON Coexist\n");
								rtw_write8(Adapter, REG_GPIO_MUXCFG, 0x00);	
							}
							else
							{
								printk("BT_Turn OFF Coexist\n");
								rtw_write8(Adapter, REG_GPIO_MUXCFG, 0x00);
							}
						}
					}
					else
					{
						printk("BT: There is no Wifi traffic!! Turn off Coexist\n");
						rtw_write8(Adapter, REG_GPIO_MUXCFG, 0x00);
					}

					if(1)//pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod >= 20)
					{
						if(pbtpriv->BT_Service==BT_PAN)
						{
							printk("BT_Turn ON Coexist(Reg0x44 = 0x10100)\n");
							rtw_write32(Adapter, REG_GPIO_PIN_CTRL, 0x10100);	
						}
						else
						{
							printk("BT_Turn OFF Coexist(Reg0x44 = 0x0)\n");
							rtw_write32(Adapter, REG_GPIO_PIN_CTRL, 0x0);	
						}
					}
					else
					{
						printk("BT: There is no Wifi traffic!! Turn off Coexist(Reg0x44 = 0x0)\n");
						rtw_write32(Adapter, REG_GPIO_PIN_CTRL, 0x0);	
					}

					// 20100430 Joseph: Integrate the BT coexistence EDCA tuning here.
					if(pbtpriv->BtRssiState & BT_RSSI_STATE_NORMAL_POWER)
					{
						if(pbtpriv->BT_Service==BT_OtherBusy)
						{
							//pbtpriv->BtEdcaUL = 0x5ea72b;
							//pbtpriv->BtEdcaDL = 0x5ea72b;
							pbtpriv->BT_EDCA[UP_LINK] = 0x5ea72b;
							pbtpriv->BT_EDCA[DOWN_LINK] = 0x5ea72b;							
							
							printk("BT in BT_OtherBusy state Tx (%d) >350 parameter(0x%x) = 0x%x\n", pbtpriv->Ratio_Tx ,REG_EDCA_BE_PARAM, 0x5ea72b);
						}
						else if(pbtpriv->BT_Service==BT_Busy)
						{
							//pbtpriv->BtEdcaUL = 0x5eb82f;
							//pbtpriv->BtEdcaDL = 0x5eb82f;

							pbtpriv->BT_EDCA[UP_LINK] = 0x5eb82f;
							pbtpriv->BT_EDCA[DOWN_LINK] = 0x5eb82f;							
							
							printk("BT in BT_Busy state parameter(0x%x) = 0x%x\n", REG_EDCA_BE_PARAM, 0x5eb82f);		
						}
						else if(pbtpriv->BT_Service==BT_SCO)
						{
							if(pbtpriv->Ratio_Tx>160)
							{
								//pbtpriv->BtEdcaUL = 0x5ea72f;
								//pbtpriv->BtEdcaDL = 0x5ea72f;
								pbtpriv->BT_EDCA[UP_LINK] = 0x5ea72f;
								pbtpriv->BT_EDCA[DOWN_LINK] = 0x5ea72f;							
								printk("BT in BT_SCO state Tx (%d) >160 parameter(0x%x) = 0x%x\n",pbtpriv->Ratio_Tx, REG_EDCA_BE_PARAM, 0x5ea72f);
							}
							else
							{
								//pbtpriv->BtEdcaUL = 0x5ea32b;
								//pbtpriv->BtEdcaDL = 0x5ea42b;

								pbtpriv->BT_EDCA[UP_LINK] = 0x5ea32b;
								pbtpriv->BT_EDCA[DOWN_LINK] = 0x5ea42b;						
								
								printk("BT in BT_SCO state Tx (%d) <160 parameter(0x%x) = 0x%x\n", pbtpriv->Ratio_Tx,REG_EDCA_BE_PARAM, 0x5ea32f);
							}									
						}
						else
						{
							// BT coexistence mechanism does not control EDCA parameter.
							//pbtpriv->BtEdcaUL = 0;
							//pbtpriv->BtEdcaDL = 0;

							pbtpriv->BT_EDCA[UP_LINK] = 0;
							pbtpriv->BT_EDCA[DOWN_LINK] = 0;							
							printk("BT in  State  %d  and parameter(0x%x) use original setting.\n",pbtpriv->BT_Service, REG_EDCA_BE_PARAM);
						}

						if((pbtpriv->BT_Service!=BT_Idle) &&
							(pmlmeext->cur_wireless_mode  == WIRELESS_MODE_G) &&
							(pbtpriv->BtRssiState & BT_RSSI_STATE_BG_EDCA_LOW))
						{
							//pbtpriv->BtEdcaUL = 0x5eb82b;
							//pbtpriv->BtEdcaDL = 0x5eb82b;

							pbtpriv->BT_EDCA[UP_LINK] = 0x5eb82b;
							pbtpriv->BT_EDCA[DOWN_LINK] = 0x5eb82b;							
							
							printk("BT set parameter(0x%x) = 0x%x\n", REG_EDCA_BE_PARAM, 0x5eb82b);		
						}
					}
					else
					{
						// BT coexistence mechanism does not control EDCA parameter.
						//pbtpriv->BtEdcaUL = 0;
						//pbtpriv->BtEdcaDL = 0;

						pbtpriv->BT_EDCA[UP_LINK] = 0;
						pbtpriv->BT_EDCA[DOWN_LINK] = 0;					
					}

					// 20100415 Joseph: Set RF register 0x1E and 0x1F for BT coexist suggested by Yaying.
					if(pbtpriv->BT_Service!=BT_Idle)
					{
						printk("BT Set RfReg0x1E[7:4] = 0x%x \n", 0xf);
						PHY_SetRFReg(Adapter, PathA, 0x1e, 0xf0, 0xf);
						//RTPRINT(FBT, BT_TRACE, ("BT Set RfReg0x1E[7:4] = 0x%x \n", 0xf));
						//PHY_SetRFReg(Adapter, PathA, 0x1f, 0xf0, 0xf);
					}
					else
					{
						printk("BT Set RfReg0x1E[7:4] = 0x%x \n",pbtpriv->BtRfRegOrigin1E);
						PHY_SetRFReg(Adapter, PathA, 0x1e, 0xf0, pbtpriv->BtRfRegOrigin1E);
						//RTPRINT(FBT, BT_TRACE, ("BT Set RfReg0x1F[7:4] = 0x%x \n", pHalData->bt_coexist.BtRfRegOrigin1F));
						//PHY_SetRFReg(Adapter, PathA, 0x1f, 0xf0, pHalData->bt_coexist.BtRfRegOrigin1F);
					}	
				}
				else
				{
					printk("BT_IsolationHigh\n");
					// Do nothing.
				}			
			}
			else
			{
			
				if(pbtpriv->BT_Ampdu && !pmlmeinfo->bAcceptAddbaReq)
				{
					printk("BT_Allow AMPDU bt is off\n");	
					pmlmeinfo->bAcceptAddbaReq = _TRUE;
				}
			
				printk("BT_Turn OFF Coexist bt is off \n");
				rtw_write8(Adapter, REG_GPIO_MUXCFG, 0x00);

				printk("BT Set RfReg0x1E[7:4] = 0x%x \n", pbtpriv->BtRfRegOrigin1E);
				PHY_SetRFReg(Adapter, PathA, 0x1e, 0xf0, pbtpriv->BtRfRegOrigin1E);
				//RTPRINT(FBT, BT_TRACE, ("BT Set RfReg0x1F[7:4] = 0x%x \n", pHalData->bt_coexist.BtRfRegOrigin1F));
				//PHY_SetRFReg(Adapter, PathA, 0x1f, 0xf0, pHalData->bt_coexist.BtRfRegOrigin1F);

				// BT coexistence mechanism does not control EDCA parameter since BT is disabled.
				//pbtpriv->BtEdcaUL = 0;
				//pbtpriv->BtEdcaDL = 0;
				pbtpriv->BT_EDCA[UP_LINK] = 0;
				pbtpriv->BT_EDCA[DOWN_LINK] = 0;				
				

// 20100427 Joseph: Do not adjust Rate adaptive for BT coexist suggested by SD3.
#if 0
				RTPRINT(FBT, BT_TRACE, ("BT_Update Rate table\n"));
				if(pMgntInfo->bUseRAMask)
				{
					// 20100407 Joseph: Fix rate adaptive modification for BT coexist.
					// This fix is not complete yet. It shall also consider VWifi and Adhoc case,
					// which connect with multiple STAs.
					Adapter->HalFunc.UpdateHalRAMaskHandler(
											Adapter,
											FALSE,
											0,
											NULL,
											NULL,
											pMgntInfo->RateAdaptive.RATRState,
											RAMask_Normal);
				}
				else
				{
					Adapter->HalFunc.UpdateHalRATRTableHandler(
								Adapter, 
								&pMgntInfo->dot11OperationalRateSet,
								pMgntInfo->dot11HTOperationalRateSet,NULL);
				}
#endif
			}
		}
	}
}

void dm_InitBtCoexistDM(	PADAPTER	Adapter)
{
	//HAL_DATA_TYPE* pHalData = GET_HAL_DATA(Adapter);
	struct btcoexist_priv	 *pbtpriv = &(Adapter->halpriv.bt_coexist);

	if( !pbtpriv->BT_Coexist ) return;
	
	pbtpriv->BtRfRegOrigin1E = (u8)PHY_QueryRFReg(Adapter, PathA, 0x1e, 0xf0);
	pbtpriv->BtRfRegOrigin1F = (u8)PHY_QueryRFReg(Adapter, PathA, 0x1f, 0xf0);
}

void set_dm_bt_coexist(_adapter *padapter, u8 bStart)
{
	struct mlme_ext_info	*pmlmeinfo = &padapter->mlmeextpriv.mlmext_info;
	struct btcoexist_priv	 *pbtpriv = &(padapter->halpriv.bt_coexist);
	pbtpriv->bCOBT = bStart;
	send_delba(padapter,0, get_my_bssid(&(pmlmeinfo->network)));
	send_delba(padapter,1, get_my_bssid(&(pmlmeinfo->network)));
	
}

void issue_delete_ba(_adapter *padapter, u8 dir)
{
	struct mlme_ext_info		*pmlmeinfo = &padapter->mlmeextpriv.mlmext_info;
	printk("issue_delete_ba : %s...\n",(dir==0)?"RX_DIR":"TX_DIR");
	send_delba(padapter,dir, get_my_bssid(&(pmlmeinfo->network)));
}

#endif
//
// Initialize GPIO setting registers
//
void dm_InitGPIOSetting(IN PADAPTER	Adapter)
{
	u8	tmp1byte;	
	
	tmp1byte =rtw_read8(Adapter, REG_GPIO_MUXCFG);
	tmp1byte &= (GPIOSEL_GPIO | ~GPIOSEL_ENBT);
	rtw_write8(Adapter,  REG_GPIO_MUXCFG, tmp1byte);
}

#if 0//(DEV_BUS_TYPE==DEV_BUS_PCI_INTERFACE)

BOOLEAN
BT_BTStateChange(
	IN	PADAPTER	Adapter
	)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	
	u4Byte 			temp, Polling, Ratio_Tx, Ratio_PRI;
	u4Byte 			BT_Tx, BT_PRI;
	u1Byte			BT_State;
	static u1Byte		ServiceTypeCnt = 0;
	u1Byte			CurServiceType;
	static u1Byte		LastServiceType = BT_Idle;

	if(!pMgntInfo->bMediaConnect)
		return FALSE;
	
	BT_State = PlatformEFIORead1Byte(Adapter, 0x4fd);
/*
	temp = PlatformEFIORead4Byte(Adapter, 0x488);
	BT_Tx = (u2Byte)(((temp<<8)&0xff00)+((temp>>8)&0xff));
	BT_PRI = (u2Byte)(((temp>>8)&0xff00)+((temp>>24)&0xff));

	temp = PlatformEFIORead4Byte(Adapter, 0x48c);
	Polling = ((temp<<8)&0xff000000) + ((temp>>8)&0x00ff0000) + 
			((temp<<8)&0x0000ff00) + ((temp>>8)&0x000000ff);
	
*/
	BT_Tx = PlatformEFIORead4Byte(Adapter, 0x488);
	
	RTPRINT(FBT, BT_TRACE, ("Ratio 0x488  =%x\n", BT_Tx));
	BT_Tx =BT_Tx & 0x00ffffff;
	//RTPRINT(FBT, BT_TRACE, ("Ratio BT_Tx  =%x\n", BT_Tx));

	BT_PRI = PlatformEFIORead4Byte(Adapter, 0x48c);
	
	RTPRINT(FBT, BT_TRACE, ("Ratio Ratio 0x48c  =%x\n", BT_PRI));
	BT_PRI =BT_PRI & 0x00ffffff;
	//RTPRINT(FBT, BT_TRACE, ("Ratio BT_PRI  =%x\n", BT_PRI));


	Polling = PlatformEFIORead4Byte(Adapter, 0x490);
	//RTPRINT(FBT, BT_TRACE, ("Ratio 0x490  =%x\n", Polling));


	if(BT_Tx==0xffffffff && BT_PRI==0xffffffff && Polling==0xffffffffff && BT_State==0xff)
		return FALSE;

	BT_State &= BIT0;

	if(BT_State != pHalData->bt_coexist.BT_CUR_State)
	{
		pHalData->bt_coexist.BT_CUR_State = BT_State;
	
		if(pMgntInfo->bRegBT_Sco == 3)
		{
			ServiceTypeCnt = 0;
		
			pHalData->bt_coexist.BT_Service = BT_Idle;

			RTPRINT(FBT, BT_TRACE, ("BT_%s\n", BT_State?"ON":"OFF"));

			BT_State = BT_State | 
					((pHalData->bt_coexist.BT_Ant_isolation==1)?0:BIT1) |BIT2;

			PlatformEFIOWrite1Byte(Adapter, 0x4fd, BT_State);
			RTPRINT(FBT, BT_TRACE, ("BT set 0x4fd to %x\n", BT_State));
		}
		
		return TRUE;
	}
	RTPRINT(FBT, BT_TRACE, ("bRegBT_Sco   %d\n", pMgntInfo->bRegBT_Sco));

	Ratio_Tx = BT_Tx*1000/Polling;
	Ratio_PRI = BT_PRI*1000/Polling;

	pHalData->bt_coexist.Ratio_Tx=Ratio_Tx;
	pHalData->bt_coexist.Ratio_PRI=Ratio_PRI;
	
	RTPRINT(FBT, BT_TRACE, ("Ratio_Tx=%d\n", Ratio_Tx));
	RTPRINT(FBT, BT_TRACE, ("Ratio_PRI=%d\n", Ratio_PRI));

	
	if(BT_State && pMgntInfo->bRegBT_Sco==3)
	{
		RTPRINT(FBT, BT_TRACE, ("bRegBT_Sco  ==3 Follow Counter\n"));
//		if(BT_Tx==0xffff && BT_PRI==0xffff && Polling==0xffffffff)
//		{
//			ServiceTypeCnt = 0;
//			return FALSE;
//		}
//		else
		{
		/*
			Ratio_Tx = BT_Tx*1000/Polling;
			Ratio_PRI = BT_PRI*1000/Polling;

			pHalData->bt_coexist.Ratio_Tx=Ratio_Tx;
			pHalData->bt_coexist.Ratio_PRI=Ratio_PRI;
			
			RTPRINT(FBT, BT_TRACE, ("Ratio_Tx=%d\n", Ratio_Tx));
			RTPRINT(FBT, BT_TRACE, ("Ratio_PRI=%d\n", Ratio_PRI));

		*/	
			if((Ratio_Tx <= 50)  && (Ratio_PRI <= 50)) 
			  	CurServiceType = BT_Idle;
			else if((Ratio_PRI > 150) && (Ratio_PRI < 200))
				CurServiceType = BT_SCO;
			else if((Ratio_Tx >= 200)&&(Ratio_PRI >= 200))
				CurServiceType = BT_Busy;
			else if(Ratio_Tx >= 350)
				CurServiceType = BT_OtherBusy;
			else
				CurServiceType=BT_OtherAction;

		}
/*		if(pHalData->bt_coexist.bStopCount)
		{
			ServiceTypeCnt=0;
			pHalData->bt_coexist.bStopCount=FALSE;
		}
*/
		if(CurServiceType == BT_OtherBusy)
		{
			ServiceTypeCnt=2;
			LastServiceType=CurServiceType;
		}
		else if(CurServiceType == LastServiceType)
		{
			if(ServiceTypeCnt<3)
				ServiceTypeCnt++;
		}
		else
		{
			ServiceTypeCnt = 0;
			LastServiceType = CurServiceType;
		}

		if(ServiceTypeCnt==2)
		{
			pHalData->bt_coexist.BT_Service = LastServiceType;
			BT_State = BT_State | 
					((pHalData->bt_coexist.BT_Ant_isolation==1)?0:BIT1) |
					((pHalData->bt_coexist.BT_Service==BT_SCO)?0:BIT2);

			if(pHalData->bt_coexist.BT_Service==BT_Busy)
				BT_State&= ~(BIT2);

			if(pHalData->bt_coexist.BT_Service==BT_SCO)
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to  ==> BT_SCO\n"));
			}
			else if(pHalData->bt_coexist.BT_Service==BT_Idle)
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to  ==> BT_Idle\n"));
			}
			else if(pHalData->bt_coexist.BT_Service==BT_OtherAction)
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to  ==> BT_OtherAction\n"));
			}
			else if(pHalData->bt_coexist.BT_Service==BT_Busy)
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to  ==> BT_Busy\n"));
			}
			else
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to ==> BT_OtherBusy\n"));
			}
				
			//Add interrupt migration when bt is not in idel state (no traffic).
			//suggestion by Victor.
			if(pHalData->bt_coexist.BT_Service!=BT_Idle)
			{
			
				PlatformEFIOWrite2Byte(Adapter, 0x504, 0x0ccc);
				PlatformEFIOWrite1Byte(Adapter, 0x506, 0x54);
				PlatformEFIOWrite1Byte(Adapter, 0x507, 0x54);
			
			}
			else
			{
				PlatformEFIOWrite1Byte(Adapter, 0x506, 0x00);
				PlatformEFIOWrite1Byte(Adapter, 0x507, 0x00);			
			}
				
			PlatformEFIOWrite1Byte(Adapter, 0x4fd, BT_State);
			RTPRINT(FBT, BT_TRACE, ("BT_SCO set 0x4fd to %x\n", BT_State));
			return TRUE;
		}
	}

	return FALSE;

}

BOOLEAN
BT_WifiConnectChange(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	static BOOLEAN	bMediaConnect = FALSE;

	if(!pMgntInfo->bMediaConnect || MgntRoamingInProgress(pMgntInfo))
	{
		bMediaConnect = FALSE;
	}
	else
	{
		if(!bMediaConnect)
		{
			bMediaConnect = TRUE;
			return TRUE;
		}
		bMediaConnect = TRUE;
	}

	return FALSE;
}

BOOLEAN
BT_RSSIChangeWithAMPDU(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(!Adapter->pNdisCommon->bRegBT_Ampdu || !Adapter->pNdisCommon->bRegAcceptAddbaReq)
		return FALSE;

	RTPRINT(FBT, BT_TRACE, ("RSSI is %d\n",pHalData->UndecoratedSmoothedPWDB));
	
	if((pHalData->UndecoratedSmoothedPWDB<=32) && pMgntInfo->pHTInfo->bAcceptAddbaReq)
	{
		RTPRINT(FBT, BT_TRACE, ("BT_Disallow AMPDU RSSI <=32  Need change\n"));				
		return TRUE;

	}
	else if((pHalData->UndecoratedSmoothedPWDB>=40) && !pMgntInfo->pHTInfo->bAcceptAddbaReq )
	{
		RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU RSSI >=40, Need change\n"));
		return TRUE;
	}
	else 
		return FALSE;

}


VOID
dm_BTCoexist(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	static u1Byte		LastTxPowerLvl = 0xff;
	PRX_TS_RECORD	pRxTs = NULL;

	BOOLEAN			bWifiConnectChange, bBtStateChange,bRSSIChangeWithAMPDU;

	if( (pHalData->bt_coexist.BluetoothCoexist) &&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR) && 
		(!ACTING_AS_AP(Adapter))	)
	{
		bWifiConnectChange = BT_WifiConnectChange(Adapter);
		bBtStateChange = BT_BTStateChange(Adapter);
		bRSSIChangeWithAMPDU = BT_RSSIChangeWithAMPDU(Adapter);
		RTPRINT(FBT, BT_TRACE, ("bWifiConnectChange %d, bBtStateChange  %d,LastTxPowerLvl  %x,  DynamicTxHighPowerLvl  %x\n",
			bWifiConnectChange,bBtStateChange,LastTxPowerLvl,pHalData->DynamicTxHighPowerLvl));
		if( bWifiConnectChange ||bBtStateChange  ||
			(LastTxPowerLvl != pHalData->DynamicTxHighPowerLvl)	||bRSSIChangeWithAMPDU)
		{
			LastTxPowerLvl = pHalData->DynamicTxHighPowerLvl;

			if(pHalData->bt_coexist.BT_CUR_State)
			{
				// Do not allow receiving A-MPDU aggregation.
				if((pHalData->bt_coexist.BT_Service==BT_SCO) || (pHalData->bt_coexist.BT_Service==BT_Busy))
				{
					if(pHalData->UndecoratedSmoothedPWDB<=32)
					{
						if(Adapter->pNdisCommon->bRegBT_Ampdu && Adapter->pNdisCommon->bRegAcceptAddbaReq)
						{
							RTPRINT(FBT, BT_TRACE, ("BT_Disallow AMPDU RSSI <=32\n"));	
							pMgntInfo->pHTInfo->bAcceptAddbaReq = FALSE;
							if(GetTs(Adapter, (PTS_COMMON_INFO*)(&pRxTs), pMgntInfo->Bssid, 0, RX_DIR, FALSE))
								TsInitDelBA(Adapter, (PTS_COMMON_INFO)pRxTs, RX_DIR);
						}
					}
					else if(pHalData->UndecoratedSmoothedPWDB>=40)
					{
						if(Adapter->pNdisCommon->bRegBT_Ampdu && Adapter->pNdisCommon->bRegAcceptAddbaReq)
						{
							RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU  RSSI >=40\n"));	
							pMgntInfo->pHTInfo->bAcceptAddbaReq = TRUE;
						}
					}
				}
				else
				{
					if(Adapter->pNdisCommon->bRegBT_Ampdu && Adapter->pNdisCommon->bRegAcceptAddbaReq)
					{
						RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU BT not in SCO or BUSY\n"));	
						pMgntInfo->pHTInfo->bAcceptAddbaReq = TRUE;
					}
				}

				if(pHalData->bt_coexist.BT_Ant_isolation)
				{			
					RTPRINT(FBT, BT_TRACE, ("BT_IsolationLow\n"));
					RTPRINT(FBT, BT_TRACE, ("BT_Update Rate table\n"));
					Adapter->HalFunc.UpdateHalRATRTableHandler(
								Adapter, 
								&pMgntInfo->dot11OperationalRateSet,
								pMgntInfo->dot11HTOperationalRateSet,NULL);
					
					if(pHalData->bt_coexist.BT_Service==BT_SCO)
					{

						RTPRINT(FBT, BT_TRACE, ("BT_Turn OFF Coexist with SCO \n"));
						PlatformEFIOWrite1Byte(Adapter, REG_GPIO_MUXCFG, 0x14);					
					}
					else if(pHalData->DynamicTxHighPowerLvl == TxHighPwrLevel_Normal)
					{
						RTPRINT(FBT, BT_TRACE, ("BT_Turn ON Coexist\n"));
						PlatformEFIOWrite1Byte(Adapter, REG_GPIO_MUXCFG, 0xb4);
					}
					else
					{
						RTPRINT(FBT, BT_TRACE, ("BT_Turn OFF Coexist\n"));
						PlatformEFIOWrite1Byte(Adapter, REG_GPIO_MUXCFG, 0x14);
					}
				}
				else
				{
					RTPRINT(FBT, BT_TRACE, ("BT_IsolationHigh\n"));
					// Do nothing.
				}
			}
			else
			{
				if(Adapter->pNdisCommon->bRegBT_Ampdu && Adapter->pNdisCommon->bRegAcceptAddbaReq)
				{
					RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU bt is off\n"));	
					pMgntInfo->pHTInfo->bAcceptAddbaReq = TRUE;
				}

				RTPRINT(FBT, BT_TRACE, ("BT_Turn OFF Coexist bt is off \n"));
				PlatformEFIOWrite1Byte(Adapter, REG_GPIO_MUXCFG, 0x14);

				RTPRINT(FBT, BT_TRACE, ("BT_Update Rate table\n"));
				Adapter->HalFunc.UpdateHalRATRTableHandler(
							Adapter, 
							&pMgntInfo->dot11OperationalRateSet,
							pMgntInfo->dot11HTOperationalRateSet,NULL);
			}
		}
	}
}
#endif

#if 0
//#if USB_RX_AGGREGATION_92C
void
dm_CheckRxAggregation(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

	// Keep past Tx/Rx packet count for RT-to-RT EDCA turbo.
	static u8Byte			lastTxOkCnt = 0;
	static u8Byte			lastRxOkCnt = 0;
	u8Byte				curTxOkCnt = 0;
	u8Byte				curRxOkCnt = 0;	

#if (RTL8192SU_FPGA_2MAC_VERIFICATION||RTL8192SU_ASIC_VERIFICATION)
	return;
#endif

	if (pHalData->bForcedUsbRxAggr)
	{
		if (pHalData->ForcedUsbRxAggrInfo == 0)
		{
			if (pHalData->bCurrentRxAggrEnable)
			{
				Adapter->HalFunc.HalUsbRxAggrHandler(Adapter, FALSE);
			}
		}
		else
		{
			if (!pHalData->bCurrentRxAggrEnable || (pHalData->ForcedUsbRxAggrInfo != pHalData->LastUsbRxAggrInfoSetting))
			{
				Adapter->HalFunc.HalUsbRxAggrHandler(Adapter, TRUE);
			}
		}
		return;
	}

	//
	// <Roger_Notes> We simply decrease Rx page aggregation threshold in B/G mode.
	// 2008.10.29
	//
	if( IS_WIRELESS_MODE_B(Adapter) || 
			IS_WIRELESS_MODE_G(Adapter))
	{
		if (pHalData->bCurrentRxAggrEnable)
		{
			RT_TRACE(COMP_RECV, DBG_LOUD, ("dm_CheckRxAggregation() :  Disable Rx Aggregation!!\n"));
			Adapter->HalFunc.HalUsbRxAggrHandler(Adapter, FALSE);
			return;
		}
	}

	curTxOkCnt = Adapter->TxStats.NumTxBytesUnicast - lastTxOkCnt;
	curRxOkCnt = Adapter->RxStats.NumRxBytesUnicast - lastRxOkCnt;

	if((curTxOkCnt + curRxOkCnt) < 15000000)
	{
		return;
	}

	if(curTxOkCnt > 4*curRxOkCnt)
	{
		if (pHalData->bCurrentRxAggrEnable)
		{
			Adapter->HalFunc.HalUsbRxAggrHandler(Adapter, FALSE);
		}
	}	
	else
	{
		if (!pHalData->bCurrentRxAggrEnable || (pHalData->RegUsbRxAggrInfo != pHalData->LastUsbRxAggrInfoSetting))
		{
			Adapter->HalFunc.HalUsbRxAggrHandler(Adapter, TRUE);
		}
	}

	lastTxOkCnt = Adapter->TxStats.NumTxBytesUnicast;
	lastRxOkCnt = Adapter->RxStats.NumRxBytesUnicast;
}	// dm_CheckEdcaTurbo
#endif

/*-----------------------------------------------------------------------------
 * Function:	dm_CheckRfCtrlGPIO()
 *
 * Overview:	Copy 8187B template for 9xseries.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/10/2008	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
VOID
dm_CheckRfCtrlGPIO(
	IN	PADAPTER	Adapter
	)
{
#if 0
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

#if ( (HAL_CODE_BASE == RTL8190) || \
 	 ((HAL_CODE_BASE == RTL8192) && (DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE)))
	//return;
#endif
	
	// Walk around for DTM test, we will not enable HW - radio on/off because r/w
	// page 1 register before Lextra bus is enabled cause system fails when resuming
	// from S4. 20080218, Emily
	if(Adapter->bInHctTest)
		return;

//#if ((HAL_CODE_BASE == RTL8192_S) )
	//Adapter->HalFunc.GPIOChangeRFHandler(Adapter, GPIORF_POLLING);
//#else
	PlatformScheduleWorkItem( &(pHalData->GPIOChangeRFWorkItem) );
//#endif

#endif
}	/* dm_CheckRfCtrlGPIO */

VOID
dm_InitRateAdaptiveMask(
	IN	PADAPTER	Adapter	
	)
{
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	PRATE_ADAPTIVE	pRA = (PRATE_ADAPTIVE)&pdmpriv->RateAdaptive;
	
	pRA->RATRState = DM_RATR_STA_INIT;
	pRA->PreRATRState = DM_RATR_STA_INIT;

	if (pdmpriv->DM_Type == DM_Type_ByDriver)
		pdmpriv->bUseRAMask = _TRUE;
	else
		pdmpriv->bUseRAMask = _FALSE;	
}

#if 0
VOID
AP_InitRateAdaptiveState(
	IN	PADAPTER	Adapter	,
	IN	PRT_WLAN_STA  pEntry
	)
{
	PRATE_ADAPTIVE	pRA = (PRATE_ADAPTIVE)&pEntry->RateAdaptive;

	pRA->RATRState = DM_RATR_STA_INIT;
	pRA->PreRATRState = DM_RATR_STA_INIT;
}
#endif

/*-----------------------------------------------------------------------------
 * Function:	dm_RefreshRateAdaptiveMask()
 *
 * Overview:	Update rate table mask according to rssi
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/27/2009	hpfan	Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
VOID
dm_RefreshRateAdaptiveMask(	IN	PADAPTER	pAdapter)
{
#if 0
	PADAPTER 				pTargetAdapter;
	HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(pAdapter);
	PMGNT_INFO				pMgntInfo = &(ADJUST_TO_ADAPTIVE_ADAPTER(pAdapter, TRUE)->MgntInfo);
	PRATE_ADAPTIVE			pRA = (PRATE_ADAPTIVE)&pMgntInfo->RateAdaptive;
	u4Byte					LowRSSIThreshForRA = 0, HighRSSIThreshForRA = 0;

	if(pAdapter->bDriverStopped)
	{
		RT_TRACE(COMP_RATR, DBG_TRACE, ("<---- dm_RefreshRateAdaptiveMask(): driver is going to unload\n"));
		return;
	}

	if(!pMgntInfo->bUseRAMask)
	{
		RT_TRACE(COMP_RATR, DBG_LOUD, ("<---- dm_RefreshRateAdaptiveMask(): driver does not control rate adaptive mask\n"));
		return;
	}

	// if default port is connected, update RA table for default port (infrastructure mode only)
	if(pAdapter->MgntInfo.mAssoc && (!ACTING_AS_AP(pAdapter)))
	{
		
		// decide rastate according to rssi
		switch (pRA->PreRATRState)
		{
			case DM_RATR_STA_HIGH:
				HighRSSIThreshForRA = 50;
				LowRSSIThreshForRA = 20;
				break;
			
			case DM_RATR_STA_MIDDLE:
				HighRSSIThreshForRA = 55;
				LowRSSIThreshForRA = 20;
				break;
			
			case DM_RATR_STA_LOW:
				HighRSSIThreshForRA = 50;
				LowRSSIThreshForRA = 25;
				break;

			default:
				HighRSSIThreshForRA = 50;
				LowRSSIThreshForRA = 20;
				break;
		}

		if(pHalData->UndecoratedSmoothedPWDB > (s4Byte)HighRSSIThreshForRA)
			pRA->RATRState = DM_RATR_STA_HIGH;
		else if(pHalData->UndecoratedSmoothedPWDB > (s4Byte)LowRSSIThreshForRA)
			pRA->RATRState = DM_RATR_STA_MIDDLE;
		else
			pRA->RATRState = DM_RATR_STA_LOW;

		if(pRA->PreRATRState != pRA->RATRState)
		{
			RT_PRINT_ADDR(COMP_RATR, DBG_LOUD, ("Target AP addr : "), pMgntInfo->Bssid);
			RT_TRACE(COMP_RATR, DBG_LOUD, ("RSSI = %d\n", pHalData->UndecoratedSmoothedPWDB));
			RT_TRACE(COMP_RATR, DBG_LOUD, ("RSSI_LEVEL = %d\n", pRA->RATRState));
			RT_TRACE(COMP_RATR, DBG_LOUD, ("PreState = %d, CurState = %d\n", pRA->PreRATRState, pRA->RATRState));
			pAdapter->HalFunc.UpdateHalRAMaskHandler(
									pAdapter,
									FALSE,
									0,
									NULL,
									NULL,
									pRA->RATRState);
			pRA->PreRATRState = pRA->RATRState;
		}
	}

	//
	// The following part configure AP/VWifi/IBSS rate adaptive mask.
	//
	if(ACTING_AS_AP(pAdapter) || ACTING_AS_IBSS(pAdapter))
	{
		pTargetAdapter = pAdapter;
	}
	else
	{
		pTargetAdapter = ADJUST_TO_ADAPTIVE_ADAPTER(pAdapter, FALSE);
		if(!ACTING_AS_AP(pTargetAdapter))
			pTargetAdapter = NULL;
	}

	// if extension port (softap) is started, updaet RA table for more than one clients associate
	if(pTargetAdapter != NULL)
	{
		int	i;
		PRT_WLAN_STA	pEntry;
		PRATE_ADAPTIVE     pEntryRA;

		for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
		{
			if(	pTargetAdapter->MgntInfo.AsocEntry[i].bUsed && pTargetAdapter->MgntInfo.AsocEntry[i].bAssociated)
			{
				pEntry = pTargetAdapter->MgntInfo.AsocEntry+i;
				pEntryRA = &pEntry->RateAdaptive;

				switch (pEntryRA->PreRATRState)
				{
					case DM_RATR_STA_HIGH:
					{
						HighRSSIThreshForRA = 50;
						LowRSSIThreshForRA = 20;
					}
					break;
					
					case DM_RATR_STA_MIDDLE:
					{
						HighRSSIThreshForRA = 55;
						LowRSSIThreshForRA = 20;
					}
					break;
					
					case DM_RATR_STA_LOW:
					{
						HighRSSIThreshForRA = 50;
						LowRSSIThreshForRA = 25;
					}
					break;

					default:
					{
						HighRSSIThreshForRA = 50;
						LowRSSIThreshForRA = 20;
					}
				}

				if(pEntry->rssi_stat.UndecoratedSmoothedPWDB > (s4Byte)HighRSSIThreshForRA)
					pEntryRA->RATRState = DM_RATR_STA_HIGH;
				else if(pEntry->rssi_stat.UndecoratedSmoothedPWDB > (s4Byte)LowRSSIThreshForRA)
					pEntryRA->RATRState = DM_RATR_STA_MIDDLE;
				else
					pEntryRA->RATRState = DM_RATR_STA_LOW;

				if(pEntryRA->PreRATRState != pEntryRA->RATRState)
				{
					RT_PRINT_ADDR(COMP_RATR, DBG_LOUD, ("AsocEntry addr : "), pEntry->MacAddr);
					RT_TRACE(COMP_RATR, DBG_LOUD, ("RSSI = %d\n", pEntry->rssi_stat.UndecoratedSmoothedPWDB));
					RT_TRACE(COMP_RATR, DBG_LOUD, ("RSSI_LEVEL = %d\n", pEntryRA->RATRState));
					RT_TRACE(COMP_RATR, DBG_LOUD, ("PreState = %d, CurState = %d\n", pEntryRA->PreRATRState, pEntryRA->RATRState));
					pAdapter->HalFunc.UpdateHalRAMaskHandler(
											pTargetAdapter,
											FALSE,
											pEntry->AID+1,
											pEntry->MacAddr,
											pEntry,
											pEntryRA->RATRState);
					pEntryRA->PreRATRState = pEntryRA->RATRState;
				}

			}
		}
	}
#endif	
}

VOID
dm_CheckProtection(
	IN	PADAPTER	Adapter
	)
{
#if 0
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	u1Byte			CurRate, RateThreshold;

	if(pMgntInfo->pHTInfo->bCurBW40MHz)
		RateThreshold = MGN_MCS1;
	else
		RateThreshold = MGN_MCS3;

	if(Adapter->TxStats.CurrentInitTxRate <= RateThreshold)
	{
		pMgntInfo->bDmDisableProtect = TRUE;
		DbgPrint("Forced disable protect: %x\n", Adapter->TxStats.CurrentInitTxRate);
	}
	else
	{
		pMgntInfo->bDmDisableProtect = FALSE;
		DbgPrint("Enable protect: %x\n", Adapter->TxStats.CurrentInitTxRate);
	}
#endif
}

VOID
dm_CheckStatistics(
	IN	PADAPTER	Adapter
	)
{
#if 0
	if(!Adapter->MgntInfo.bMediaConnect)
		return;

	//2008.12.10 tynli Add for getting Current_Tx_Rate_Reg flexibly.
	Adapter->HalFunc.GetHwRegHandler( Adapter, HW_VAR_INIT_TX_RATE, (pu1Byte)(&Adapter->TxStats.CurrentInitTxRate) );

	// Calculate current Tx Rate(Successful transmited!!)

	// Calculate current Rx Rate(Successful received!!)
	
	//for tx tx retry count
	Adapter->HalFunc.GetHwRegHandler( Adapter, HW_VAR_RETRY_COUNT, (pu1Byte)(&Adapter->TxStats.NumTxRetryCount) );
#endif	
}

void dm_CheckPbcGPIO(_adapter *padapter)
{	
	u8 tmp1byte;

	tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
	tmp1byte |= (HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	//enable GPIO[2] as output mode

	tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(padapter,  GPIO_IN, tmp1byte);		//reset the floating voltage level

	tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
	tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	//enable GPIO[2] as input mode

	tmp1byte =rtw_read8(padapter, GPIO_IN);
	
	if (tmp1byte == 0xff)
		return ;

	if (tmp1byte&HAL_8192C_HW_GPIO_WPS_BIT)
	{
		
                // Here we only set bPbcPressed to true
                // After trigger PBC, the variable will be set to false		
                printk("CheckPbcGPIO - PBC is pressed (%x)\n",tmp1byte);
                
                if ( padapter->pid == 0 )
                {	//	0 is the default value and it means the application monitors the HW PBC doesn't privde its pid to driver.
			return;
                }
#ifdef RTK_DMP_PLATFORM
		kobject_hotplug(&padapter->pnetdev->class_dev.kobj, KOBJ_NET_PBC);
		//kobject_hotplug(&dev->class_dev.kobj, KOBJ_NET_PBC);
#else

#ifdef PLATFORM_LINUX

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
		   kill_pid(find_vpid(padapter->pid), SIGUSR1, 1);
#endif

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,26))
		   kill_proc(padapter->pid, SIGUSR1, 1);
#endif

#endif

#endif
        }

}

#ifdef CONFIG_ANTENNA_DIVERSITY
// Add new function to reset the state of antenna diversity before link.
u8 SwAntDivBeforeLink8192C(IN PADAPTER Adapter)
{
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	SWAT_T *pDM_SWAT_Table = &pdmpriv->DM_SWAT_Table;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv *pmlmepriv = &(Adapter->mlmepriv);
	
	// Condition that does not need to use antenna diversity.
	if(IS_92C_SERIAL(pHalData->VersionID) ||(pHalData->AntDivCfg==0))
	{
		printk("SwAntDivBeforeLink8192C(): No AntDiv Mechanism.\n");
		return _FALSE;
	}

	if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)	
	{
		pDM_SWAT_Table->SWAS_NoLink_State = 0;
		return _FALSE;
	}
	// Since driver is going to set BB register, it shall check if there is another thread controlling BB/RF.
/*	
	if(pHalData->eRFPowerState!=eRfOn || pMgntInfo->RFChangeInProgress || pMgntInfo->bMediaConnect)
	{
	
	
		RT_TRACE(COMP_SWAS, DBG_LOUD, 
				("SwAntDivCheckBeforeLink8192C(): RFChangeInProgress(%x), eRFPowerState(%x)\n", 
				pMgntInfo->RFChangeInProgress,
				pHalData->eRFPowerState));
	
		pDM_SWAT_Table->SWAS_NoLink_State = 0;
		
		return FALSE;
	}
*/	
	
	if(pDM_SWAT_Table->SWAS_NoLink_State == 0){
		//switch channel
		pDM_SWAT_Table->SWAS_NoLink_State = 1;
		pDM_SWAT_Table->CurAntenna = (pDM_SWAT_Table->CurAntenna==Antenna_A)?Antenna_B:Antenna_A;
	
		//PHY_SetRFPath(Adapter,pDM_SWAT_Table->CurAntenna);		
		
		antenna_select_cmd(Adapter, pDM_SWAT_Table->CurAntenna, 0);
		printk("%s change antenna to ANT_( %s ).....\n",__FUNCTION__,(GET_HAL_DATA(Adapter)->CurAntenna==Antenna_A)?"A":"B");		
		return _TRUE;
	}
	else
	{
		pDM_SWAT_Table->SWAS_NoLink_State = 0;
		return _FALSE;
	}
		


}


//
// 20100514 Luke/Joseph:
// Add new function to reset antenna diversity state after link.
//
void
SwAntDivRestAfterLink(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	SWAT_T	*pDM_SWAT_Table = &pdmpriv->DM_SWAT_Table;
	if(IS_92C_SERIAL(pHalData->VersionID) ||(pHalData->AntDivCfg==0))	
		return;
	printk("======>   SwAntDivRestAfterLink <========== \n");
	pHalData->RSSI_cnt_A= 0;
	pHalData->RSSI_cnt_B= 0;
	pHalData->RSSI_test = _FALSE;
	
	pDM_SWAT_Table->try_flag = 0xff;
	pDM_SWAT_Table->RSSI_Trying = 0;	
	pDM_SWAT_Table->SelectAntennaMap=0xAA;
    	pDM_SWAT_Table->CurAntenna = pHalData->CurAntenna;
    	pDM_SWAT_Table->PreAntenna = pHalData->CurAntenna;
		
	pdmpriv->lastTxOkCnt=0;
	pdmpriv->lastRxOkCnt=0;

	pdmpriv->TXByteCnt_A=0;
	pdmpriv->TXByteCnt_B=0;
	pdmpriv->RXByteCnt_A=0;
	pdmpriv->RXByteCnt_B=0;
	pdmpriv->DoubleComfirm=0;	
	pdmpriv->TrafficLoad = TRAFFIC_LOW;
	
}


//
// 20100514 Luke/Joseph:
// Add new function for antenna diversity after link.
// This is the main function of antenna diversity after link.
// This function is called in HalDmWatchDog() and dm_SW_AntennaSwitchCallback().
// HalDmWatchDog() calls this function with SWAW_STEP_PEAK to initialize the antenna test.
// In SWAW_STEP_PEAK, another antenna and a 500ms timer will be set for testing.
// After 500ms, dm_SW_AntennaSwitchCallback() calls this function to compare the signal just
// listened on the air with the RSSI of original antenna.
// It chooses the antenna with better RSSI.
// There is also a aged policy for error trying. Each error trying will cost more 5 seconds waiting 
// penalty to get next try.
//
VOID
dm_SW_AntennaSwitch(
	PADAPTER		Adapter,
	u8			Step
)
{
	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	SWAT_T	*pDM_SWAT_Table = &pdmpriv->DM_SWAT_Table;
	s32			curRSSI=100, RSSI_A, RSSI_B;
	u64			curTxOkCnt, curRxOkCnt;
	u64			CurByteCnt, PreByteCnt;	
	u8			nextAntenna;
	u8			Score_A=0, Score_B=0;
	u8			i;

	// Condition that does not need to use antenna diversity.
	if(IS_92C_SERIAL(pHalData->VersionID) ||(pHalData->AntDivCfg==0))
	{
		//RT_TRACE(COMP_SWAS, DBG_LOUD, ("dm_SW_AntennaSwitch(): No AntDiv Mechanism.\n"));
		return;
	}

	
	if (check_fwstate(&Adapter->mlmepriv, _FW_LINKED)	==_FALSE)
		return;
#if 0 //to do
	// Radio off: Status reset to default and return.
	if(pHalData->eRFPowerState==eRfOff)
	{
		SwAntDivRestAfterLink(Adapter);
		return;
	}
#endif
	//printk("\n............................ %s.........................\n",__FUNCTION__);
	// Handling step mismatch condition.
	// Peak step is not finished at last time. Recover the variable and check again.
	if( Step != pDM_SWAT_Table->try_flag	)
	{
		SwAntDivRestAfterLink(Adapter);
	}


	if(pDM_SWAT_Table->try_flag == 0xff)
	{
#if 0
		// Select RSSI checking target
		if(pMgntInfo->mAssoc && !ACTING_AS_AP(Adapter))
		{
			// Target: Infrastructure mode AP.
			pHalData->RSSI_target = NULL;
			RT_TRACE(COMP_SWAS, DBG_LOUD, ("dm_SW_AntennaSwitch(): RSSI_target is DEF AP!\n"));
		}
		else
		{
			u1Byte			index = 0;
			PRT_WLAN_STA	pEntry = NULL;
			PADAPTER		pTargetAdapter = NULL;
		
			if(	pMgntInfo->mIbss || ACTING_AS_AP(Adapter) )
			{
				// Target: AP/IBSS peer.
				pTargetAdapter = Adapter;
			}
			else if(ACTING_AS_AP(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE)))
			{
				// Target: VWIFI peer.
				pTargetAdapter = ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE);
			}

			if(pTargetAdapter != NULL)
			{
				for(index=0; index<ASSOCIATE_ENTRY_NUM; index++)
				{
					pEntry = AsocEntry_EnumStation(pTargetAdapter, index);
					if(pEntry != NULL)
					{
						if(pEntry->bAssociated)
							break;			
					}
				}
			}

			if(pEntry == NULL)
			{
				SwAntDivRestAfterLink(Adapter);
				RT_TRACE(COMP_SWAS, DBG_LOUD, ("dm_SW_AntennaSwitch(): No Link.\n"));
				return;
			}
			else
			{
				pHalData->RSSI_target = pEntry;
				RT_TRACE(COMP_SWAS, DBG_LOUD, ("dm_SW_AntennaSwitch(): RSSI_target is PEER STA\n"));
			}
		}
				
			
#endif
		
		pHalData->RSSI_cnt_A= 0;
		pHalData->RSSI_cnt_B= 0;
		pDM_SWAT_Table->try_flag = 0;
		//printk("dm_SW_AntennaSwitch(): Set try_flag to 0 prepare for peak!\n");
		return;
	}
	else
	{		
		curTxOkCnt = Adapter->xmitpriv.tx_bytes - pdmpriv->lastTxOkCnt;
		curRxOkCnt = Adapter->recvpriv.rx_bytes - pdmpriv->lastRxOkCnt;
	
		pdmpriv->lastTxOkCnt = Adapter->xmitpriv.tx_bytes ;
		pdmpriv->lastRxOkCnt = Adapter->recvpriv.rx_bytes ;
	
		if(pDM_SWAT_Table->try_flag == 1)
		{
			if(pDM_SWAT_Table->CurAntenna == Antenna_A)
			{
				pdmpriv->TXByteCnt_A += curTxOkCnt;
				pdmpriv->RXByteCnt_A += curRxOkCnt;
				//printk("#####  TXByteCnt_A(%lld) , RXByteCnt_A(%lld) ####\n",pdmpriv->TXByteCnt_A,pdmpriv->RXByteCnt_A);
			}
			else
			{
				pdmpriv->TXByteCnt_B += curTxOkCnt;
				pdmpriv->RXByteCnt_B += curRxOkCnt;
				//printk("#####  TXByteCnt_B(%lld) , RXByteCnt_B(%lld) ####\n",pdmpriv->TXByteCnt_B,pdmpriv->RXByteCnt_B);
			}
		
			nextAntenna = (pDM_SWAT_Table->CurAntenna == Antenna_A)? Antenna_B : Antenna_A;
			pDM_SWAT_Table->RSSI_Trying--;
			//printk("RSSI_Trying = %d\n",pDM_SWAT_Table->RSSI_Trying);
			
			if(pDM_SWAT_Table->RSSI_Trying == 0)
			{
				CurByteCnt = (pDM_SWAT_Table->CurAntenna == Antenna_A)? (pdmpriv->TXByteCnt_A+pdmpriv->RXByteCnt_A) : (pdmpriv->TXByteCnt_B+pdmpriv->RXByteCnt_B);
				PreByteCnt = (pDM_SWAT_Table->CurAntenna == Antenna_A)? (pdmpriv->TXByteCnt_B+pdmpriv->RXByteCnt_B) : (pdmpriv->TXByteCnt_A+pdmpriv->RXByteCnt_A);

				//printk("CurByteCnt = %lld\n", CurByteCnt);
				//printk("PreByteCnt = %lld\n",PreByteCnt);		
				
				if(pdmpriv->TrafficLoad == TRAFFIC_HIGH)
				{
					PreByteCnt = PreByteCnt*9;	//normalize:Cur=90ms:Pre=10ms					
				}
				else if(pdmpriv->TrafficLoad == TRAFFIC_LOW)
				{					
					//CurByteCnt = CurByteCnt/2;
					CurByteCnt = CurByteCnt>>1;//normalize:100ms:50ms					
				}


				//printk("After DIV=>CurByteCnt = %lld\n", CurByteCnt);
				//printk("PreByteCnt = %lld\n",PreByteCnt);		

				if(pHalData->RSSI_cnt_A > 0)
					RSSI_A = pHalData->RSSI_sum_A/pHalData->RSSI_cnt_A; 
				else
					RSSI_A = 0;
				if(pHalData->RSSI_cnt_B > 0)
					RSSI_B = pHalData->RSSI_sum_B/pHalData->RSSI_cnt_B; 
				else
					RSSI_B = 0;
				
				curRSSI = (pDM_SWAT_Table->CurAntenna == Antenna_A)? RSSI_A : RSSI_B;
				pDM_SWAT_Table->PreRSSI =  (pDM_SWAT_Table->CurAntenna == Antenna_A)? RSSI_B : RSSI_A;
				//printk("Luke:PreRSSI = %d, CurRSSI = %d\n",pDM_SWAT_Table->PreRSSI, curRSSI);
				//printk("SWAS: preAntenna= %s, curAntenna= %s \n", 
				//(pDM_SWAT_Table->PreAntenna == Antenna_A?"A":"B"), (pDM_SWAT_Table->CurAntenna == Antenna_A?"A":"B"));
				//printk("Luke:RSSI_A= %d, RSSI_cnt_A = %d, RSSI_B= %d, RSSI_cnt_B = %d\n",
					//RSSI_A, pHalData->RSSI_cnt_A, RSSI_B, pHalData->RSSI_cnt_B);
			}

		}
		else
		{
		
			if(pHalData->RSSI_cnt_A > 0)
				RSSI_A = pHalData->RSSI_sum_A/pHalData->RSSI_cnt_A; 
			else
				RSSI_A = 0;
			if(pHalData->RSSI_cnt_B > 0)
				RSSI_B = pHalData->RSSI_sum_B/pHalData->RSSI_cnt_B; 
			else
				RSSI_B = 0;
			curRSSI = (pDM_SWAT_Table->CurAntenna == Antenna_A)? RSSI_A : RSSI_B;
			pDM_SWAT_Table->PreRSSI =  (pDM_SWAT_Table->PreAntenna == Antenna_A)? RSSI_A : RSSI_B;
			//printk("Ekul:PreRSSI = %d, CurRSSI = %d\n", pDM_SWAT_Table->PreRSSI, curRSSI);
			//printk("SWAS: preAntenna= %s, curAntenna= %s \n", 
			//(pDM_SWAT_Table->PreAntenna == Antenna_A?"A":"B"), (pDM_SWAT_Table->CurAntenna == Antenna_A?"A":"B"));

			//printk("Ekul:RSSI_A= %d, RSSI_cnt_A = %d, RSSI_B= %d, RSSI_cnt_B = %d\n",
			//	RSSI_A, pHalData->RSSI_cnt_A, RSSI_B, pHalData->RSSI_cnt_B);
			//RT_TRACE(COMP_SWAS, DBG_LOUD, ("Ekul:curTxOkCnt = %d\n", curTxOkCnt));
			//RT_TRACE(COMP_SWAS, DBG_LOUD, ("Ekul:curRxOkCnt = %d\n", curRxOkCnt));
		}

		//1 Trying State
		if((pDM_SWAT_Table->try_flag == 1)&&(pDM_SWAT_Table->RSSI_Trying == 0))
		{

			if(pDM_SWAT_Table->TestMode == TP_MODE)
			{
				printk("SWAS: TestMode = TP_MODE\n");
				//printk("TRY:CurByteCnt = %lld\n", CurByteCnt);
				//printk("TRY:PreByteCnt = %lld\n",PreByteCnt);		
				if(CurByteCnt < PreByteCnt)
				{
					if(pDM_SWAT_Table->CurAntenna == Antenna_A)
						pDM_SWAT_Table->SelectAntennaMap=pDM_SWAT_Table->SelectAntennaMap<<1;
					else
						pDM_SWAT_Table->SelectAntennaMap=(pDM_SWAT_Table->SelectAntennaMap<<1)+1;
				}
				else
				{
					if(pDM_SWAT_Table->CurAntenna == Antenna_A)
						pDM_SWAT_Table->SelectAntennaMap=(pDM_SWAT_Table->SelectAntennaMap<<1)+1;
					else
						pDM_SWAT_Table->SelectAntennaMap=pDM_SWAT_Table->SelectAntennaMap<<1;
				}
				for (i= 0; i<8; i++)
				{
					if(((pDM_SWAT_Table->SelectAntennaMap>>i)&BIT0) == 1)
						Score_A++;
					else
						Score_B++;
				}
				//printk("SelectAntennaMap=%x\n ",pDM_SWAT_Table->SelectAntennaMap);
				//printk("Score_A=%d, Score_B=%d\n", Score_A, Score_B);
				
				if(pDM_SWAT_Table->CurAntenna == Antenna_A)
				{
					nextAntenna = (Score_A > Score_B)?Antenna_A:Antenna_B;
				}
				else
				{
					nextAntenna = (Score_B > Score_A)?Antenna_B:Antenna_A;
				}
				//RT_TRACE(COMP_SWAS, DBG_LOUD, ("nextAntenna=%s\n",(nextAntenna==Antenna_A)?"A":"B"));
				//RT_TRACE(COMP_SWAS, DBG_LOUD, ("preAntenna= %s, curAntenna= %s \n", 
					//(DM_SWAT_Table.PreAntenna == Antenna_A?"A":"B"), (DM_SWAT_Table.CurAntenna == Antenna_A?"A":"B")));

				if(nextAntenna != pDM_SWAT_Table->CurAntenna)
				{
					printk("SWAS: Switch back to another antenna\n");
				}
				else
				{
					printk("SWAS: current anntena is good\n");
				}	
			}

			if(pDM_SWAT_Table->TestMode == RSSI_MODE)
			{	
				printk("SWAS: TestMode = RSSI_MODE\n");
				pDM_SWAT_Table->SelectAntennaMap=0xAA;
				if(curRSSI < pDM_SWAT_Table->PreRSSI) //Current antenna is worse than previous antenna
				{
					printk("SWAS: Switch back to another antenna\n");
					nextAntenna = (pDM_SWAT_Table->CurAntenna == Antenna_A)? Antenna_B : Antenna_A;
				}
				else // current anntena is good
				{
					nextAntenna = pDM_SWAT_Table->CurAntenna;
					printk("SWAS: current anntena is good\n");
				}
			}
			pDM_SWAT_Table->try_flag = 0;
			pHalData->RSSI_test = _FALSE;
			pHalData->RSSI_sum_A = 0;
			pHalData->RSSI_cnt_A = 0;
			pHalData->RSSI_sum_B = 0;
			pHalData->RSSI_cnt_B = 0;
			pdmpriv->TXByteCnt_A = 0;
			pdmpriv->TXByteCnt_B = 0;
			pdmpriv->RXByteCnt_A = 0;
			pdmpriv->RXByteCnt_B = 0;
			
		}

		//1 Normal State
		else if(pDM_SWAT_Table->try_flag == 0)
		{
			if(pdmpriv->TrafficLoad == TRAFFIC_HIGH)
			{
				if(((curTxOkCnt+curRxOkCnt)>>1) > 1875000)
					pdmpriv->TrafficLoad = TRAFFIC_HIGH;
				else
					pdmpriv->TrafficLoad = TRAFFIC_LOW;
			}
			else if(pdmpriv->TrafficLoad == TRAFFIC_LOW)
				{
				if(((curTxOkCnt+curRxOkCnt)>>1) > 1875000)
					pdmpriv->TrafficLoad = TRAFFIC_HIGH;
				else
					pdmpriv->TrafficLoad = TRAFFIC_LOW;
			}
			if(pdmpriv->TrafficLoad == TRAFFIC_HIGH)
				pDM_SWAT_Table->bTriggerAntennaSwitch = 0;
			//printk("Normal:TrafficLoad = %lld\n", curTxOkCnt+curRxOkCnt);

			//Prepare To Try Antenna		
			nextAntenna = (pDM_SWAT_Table->CurAntenna == Antenna_A)? Antenna_B : Antenna_A;
			pDM_SWAT_Table->try_flag = 1;
			pHalData->RSSI_test = _TRUE;
			if((curRxOkCnt+curTxOkCnt) > 1000)
			{
				pDM_SWAT_Table->RSSI_Trying = 4;
				pDM_SWAT_Table->TestMode = TP_MODE;
			}
			else
			{
				pDM_SWAT_Table->RSSI_Trying = 2;
				pDM_SWAT_Table->TestMode = RSSI_MODE;

			}
			//printk("SWAS: Normal State -> Begin Trying! TestMode=%s\n",(pDM_SWAT_Table->TestMode == TP_MODE)?"TP":"RSSI");
			
			
			pHalData->RSSI_sum_A = 0;
			pHalData->RSSI_cnt_A = 0;
			pHalData->RSSI_sum_B = 0;
			pHalData->RSSI_cnt_B = 0;
		}
	}

	//1 4.Change TRX antenna
	if(nextAntenna != pDM_SWAT_Table->CurAntenna)
	{
		printk("@@@@@@@@ SWAS: Change TX Antenna!\n ");		
		antenna_select_cmd(Adapter, nextAntenna, 1);
	}

	//1 5.Reset Statistics
	pDM_SWAT_Table->PreAntenna = pDM_SWAT_Table->CurAntenna;
	pDM_SWAT_Table->CurAntenna = nextAntenna;
	pDM_SWAT_Table->PreRSSI = curRSSI;
	

	//1 6.Set next timer

	if(pDM_SWAT_Table->RSSI_Trying == 0)
		return;

	if(pDM_SWAT_Table->RSSI_Trying%2 == 0)
	{
		if(pDM_SWAT_Table->TestMode == TP_MODE)
		{
			if(pdmpriv->TrafficLoad == TRAFFIC_HIGH)
			{
				_set_timer(&pdmpriv->SwAntennaSwitchTimer,10 ); //ms
				//printk("dm_SW_AntennaSwitch(): Test another antenna for 10 ms\n");
			}
			else if(pdmpriv->TrafficLoad == TRAFFIC_LOW)
			{
				_set_timer(&pdmpriv->SwAntennaSwitchTimer, 50 ); //ms
				//printk("dm_SW_AntennaSwitch(): Test another antenna for 50 ms\n");
			}
		}
		else
		{
			_set_timer(&pdmpriv->SwAntennaSwitchTimer, 500 ); //ms
			//printk("dm_SW_AntennaSwitch(): Test another antenna for 500 ms\n");
		}
	}
	else
	{
		if(pDM_SWAT_Table->TestMode == TP_MODE)
		{
			if(pdmpriv->TrafficLoad == TRAFFIC_HIGH)			
				_set_timer(&pdmpriv->SwAntennaSwitchTimer,90 ); //ms			
			else if(pdmpriv->TrafficLoad == TRAFFIC_LOW)
				_set_timer(&pdmpriv->SwAntennaSwitchTimer,100 ); //ms
		}
		else
		{
			_set_timer(&pdmpriv->SwAntennaSwitchTimer,500 ); //ms
			//printk("dm_SW_AntennaSwitch(): Test another antenna for 500 ms\n");
		}
	}

//	RT_TRACE(COMP_SWAS, DBG_LOUD, ("SWAS: -----The End-----\n "));
}

//
// 20100514 Luke/Joseph:
// Callback function for 500ms antenna test trying.
//
void dm_SW_AntennaSwitchCallback(void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;

	if(padapter->net_closed == _TRUE)
			return;
	// Only 
	dm_SW_AntennaSwitch(padapter, SWAW_STEP_DETERMINE);
}


//
// 20100722
// This function is used to gather the RSSI information for antenna testing.
// It selects the RSSI of the peer STA that we want to know.
//
void SwAntDivRSSICheck(_adapter *padapter ,u32 RxPWDBAll)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);	
	struct	mlme_priv *pmlmepriv = &padapter->mlmepriv;	
	struct dm_priv *pdmpriv = &padapter->dmpriv;
	SWAT_T	*pDM_SWAT_Table = &pdmpriv->DM_SWAT_Table;

	if(IS_92C_SERIAL(pHalData->VersionID) ||pHalData->AntDivCfg==0)
		return;
	
	if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)		
	{			
		if(pDM_SWAT_Table->CurAntenna == Antenna_A)
		{			
			pHalData->RSSI_sum_A += RxPWDBAll;
			pHalData->RSSI_cnt_A++;
		}
		else
		{
			pHalData->RSSI_sum_B+= RxPWDBAll;
			pHalData->RSSI_cnt_B++;
		
		}
		//printk("%s Ant_(%s),RSSI_sum(%d),RSSI_cnt(%d)\n",__FUNCTION__,(2==pHalData->CurAntenna)?"A":"B",pHalData->RSSI_sum,pHalData->RSSI_cnt);
	}
	
}


VOID
dm_SW_AntennaSwitchInit(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	SWAT_T	*pDM_SWAT_Table = &pdmpriv->DM_SWAT_Table;
	
	pHalData->RSSI_sum_A = 0;	
	pHalData->RSSI_sum_B = 0;
	pHalData->RSSI_cnt_A = 0;
	pHalData->RSSI_cnt_B = 0;
	
	pDM_SWAT_Table->CurAntenna = pHalData->CurAntenna;
	pDM_SWAT_Table->PreAntenna = pHalData->CurAntenna;
	pDM_SWAT_Table->try_flag = 0xff;
	pDM_SWAT_Table->PreRSSI = 0;
	pDM_SWAT_Table->bTriggerAntennaSwitch = 0;	
	pDM_SWAT_Table->SelectAntennaMap=0xAA;
	
	// Move the timer initialization to InitializeVariables function.
	//PlatformInitializeTimer(Adapter, &pMgntInfo->SwAntennaSwitchTimer, (RT_TIMER_CALL_BACK)dm_SW_AntennaSwitchCallback, NULL, "SwAntennaSwitchTimer");	
}

#endif
//============================================================
// functions
//============================================================

void init_dm_priv(_adapter *padapter)
{
	struct dm_priv *pdmpriv = &padapter->dmpriv;
	_rtw_memset(pdmpriv, 0, sizeof(struct dm_priv));
#ifdef CONFIG_ANTENNA_DIVERSITY
	_init_timer(&(pdmpriv->SwAntennaSwitchTimer),  padapter->pnetdev , dm_SW_AntennaSwitchCallback, padapter);
#endif
}

void
rtl8192c_InitHalDm(
	IN	PADAPTER	Adapter
	)
{
	struct dm_priv *pdmpriv = &Adapter->dmpriv;
	pdmpriv->DM_Type = DM_Type_ByDriver;	
	pdmpriv->DMFlag = DYNAMIC_FUNC_DISABLE;
	pdmpriv->UndecoratedSmoothedPWDB = (-1);
	
	//.1 DIG INIT
	pdmpriv->bDMInitialGainEnable = _TRUE;
	pdmpriv->DMFlag |= DYNAMIC_FUNC_DIG;
	dm_DIGInit(Adapter);

	//.2 DynamicTxPower INIT
	pdmpriv->DMFlag |= DYNAMIC_FUNC_HP;
	dm_InitDynamicTxPower(Adapter);

	//.3
	//DM_InitEdcaTurbo(Adapter);//moved to  linked_status_chk()

	//.4 RateAdaptive INIT
	dm_InitRateAdaptiveMask(Adapter);


	//.5 Tx Power Tracking Init.
	pdmpriv->DMFlag |= DYNAMIC_FUNC_SS;
	DM_InitializeTXPowerTracking(Adapter);

#if DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE
	dm_InitGPIOSetting(Adapter);
#endif

#ifdef CONFIG_BT_COEXIST
	pdmpriv->DMFlag |= DYNAMIC_FUNC_BT;
	dm_InitBtCoexistDM(Adapter);
#endif

#ifdef CONFIG_ANTENNA_DIVERSITY
	dm_SW_AntennaSwitchInit(Adapter);
#endif

	pdmpriv->DMFlag_tmp = pdmpriv->DMFlag;


//..............static variable init.......................
	pdmpriv->initial_gain_Multi_STA_binitialized = _FALSE;
	pdmpriv->TM_Trigger = 0;
	pdmpriv->BT_ServiceTypeCnt = 0;
	pdmpriv->BT_LastServiceType = BT_Idle;
	pdmpriv->BT_bMediaConnect = _FALSE;
	
}

VOID
rtl8192c_HalDmWatchDog(
	IN	PADAPTER	Adapter
	)
{
	BOOLEAN		bFwCurrentInPSMode = _FALSE;
	BOOLEAN		bFwPSAwake = _TRUE;

#ifdef CONFIG_LPS
	bFwCurrentInPSMode = Adapter->pwrctrlpriv.bFwCurrentInPSMode;
	bFwPSAwake = FWLPS_RF_ON(Adapter);
#endif


	// Stop dynamic mechanism if RF is OFF.	
	//if(rfState == eRfOn)
	if( (Adapter->hw_init_completed == _TRUE) 
		&& ((!bFwCurrentInPSMode) && bFwPSAwake)
#ifdef CONFIG_IPS
		&& (rf_on == Adapter->pwrctrlpriv.current_rfpwrstate)
#endif
	)
	{

//printk(".......%s.....\n",__FUNCTION__);
		//
		// Calculate Tx/Rx statistics.
		//
		dm_CheckStatistics(Adapter);

		//
		// For PWDB monitor and record some value for later use.
		//
		PWDB_Monitor(Adapter);

		//
		// Dynamic Initial Gain mechanism.
		//
		dm_DIG(Adapter);

		//
		// Dynamic Tx Power mechanism.
		//
		dm_DynamicTxPower(Adapter);

		//
		// Tx Power Tracking.
		//
		dm_CheckTXPowerTracking(Adapter);

		//
		// Rate Adaptive by Rx Signal Strength mechanism.
		//
		dm_RefreshRateAdaptiveMask(Adapter);

#ifdef CONFIG_BT_COEXIST
		//BT-Coexist
		dm_BTCoexist(Adapter);
#endif

		// EDCA turbo
		//update the EDCA paramter according to the Tx/RX mode
		update_EDCA_param(Adapter);
		//dm_CheckEdcaTurbo(Adapter);

		//
		// Dynamically switch RTS/CTS protection.
		//
		//dm_CheckProtection(Adapter);
		
#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE	
		dm_CheckPbcGPIO(Adapter);				// Add by hpfan 2008-03-11	
#endif

#ifdef CONFIG_ANTENNA_DIVERSITY
		//
		// Software Antenna diversity
		//
		dm_SW_AntennaSwitch(Adapter, SWAW_STEP_PEAK);
#endif

		
	}

#if 0
	// Check GPIO to determine current RF on/off and Pbc status.
	// Not enable for 92CU now!!!
#if 0// DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	if(Adapter->HalFunc.GetInterfaceSelectionHandler(Adapter)==INTF_SEL1_MINICARD)

	{
#endif
		// Check Hardware Radio ON/OFF or not	
		if(Adapter->MgntInfo.PowerSaveControl.bGpioRfSw)
		{
			RTPRINT(FPWR, PWRHW, ("dm_CheckRfCtrlGPIO \n"));
			dm_CheckRfCtrlGPIO(Adapter);
		}
#if 0//DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	}
	else if(Adapter->HalFunc.GetInterfaceSelectionHandler(Adapter)==INTF_SEL0_USB)
	{
		dm_CheckPbcGPIO(Adapter);				// Add by hpfan 2008-03-11
	}
#endif
#endif

}

