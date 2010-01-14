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
 * Jerry chuang <wlanfae@realtek.com>
******************************************************************************/
#include "r8192U.h"
#include "r8192S_rtl6052.h"

#ifdef RTL8192SU
#include "r8192S_hw.h"
#include "r8192S_phyreg.h"
#include "r8192S_phy.h"
#else
#include "r8192U_hw.h"
#include "r819xU_phyreg.h"
#include "r819xU_phy.h"
#endif


typedef struct RF_Shadow_Compare_Map {
	u32		Value;
	u8		Compare;
	u8		ErrorOrNot;
	u8		Recorver;
	u8		Driver_Write;
}RF_SHADOW_T;






void phy_RF6052_Config_HardCode(struct net_device* dev);

RT_STATUS phy_RF6052_Config_ParaFile(struct net_device* dev);

extern void RF_ChangeTxPath(struct net_device* dev,  u16 DataRate);


static	RF_SHADOW_T	RF_Shadow[RF6052_MAX_PATH][RF6052_MAX_REG];

extern void RF_ChangeTxPath(struct net_device* dev,  u16 DataRate)
{
#if 0
	static	u1Byte	RF_Path_Type = 2;	
	static	u4Byte	tx_gain_tbl1[6] 
			= {0x17f50, 0x11f40, 0x0cf30, 0x08720, 0x04310, 0x00100};
	static	u4Byte	tx_gain_tbl2[6] 
			= {0x15ea0, 0x10e90, 0x0c680, 0x08250, 0x04040, 0x00030};
	u1Byte	i;
	
	if (RF_Path_Type == 2 && (DataRate&0xF) <= 0x7)
	{
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G2, bMask20Bits, 0x0f000);
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G3, bMask20Bits, 0xeacf1);

		for (i = 0; i < 6; i++)					
			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
						RF_TX_AGC, bMask20Bits, tx_gain_tbl1[i]);

		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G2, bMask20Bits, 0x01e39);
	}
	else if (RF_Path_Type == 1 && (DataRate&0xF) >= 0x8)
	{
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G2, bMask20Bits, 0x04440);
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G3, bMask20Bits, 0xea4f1);

		for (i = 0; i < 6; i++)
			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
						RF_TX_AGC, bMask20Bits, tx_gain_tbl2[i]);

		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)RF90_PATH_A, 
					RF_TXPA_G2, bMask20Bits, 0x01e19);
	}
#endif	
	
}	


void PHY_RF6052SetBandwidth(struct net_device* dev, HT_CHANNEL_WIDTH Bandwidth)	
{	
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	

#ifdef RTL8192SU  
	{		
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:

				priv->RfRegChnlVal[0] = ((priv->RfRegChnlVal[0] & 0xfffff3ff) | 0x0400);
				rtl8192_phy_SetRFReg(dev, RF90_PATH_A, RF_CHNLBW, bRFRegOffsetMask, priv->RfRegChnlVal[0]);

				break;
			case HT_CHANNEL_WIDTH_20_40:
				priv->RfRegChnlVal[0] = ((priv->RfRegChnlVal[0] & 0xfffff3ff));
				rtl8192_phy_SetRFReg(dev, RF90_PATH_A, RF_CHNLBW, bRFRegOffsetMask, priv->RfRegChnlVal[0]);

				break;
			default:
				RT_TRACE(COMP_DBG, "PHY_SetRF6052Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth);
				break;			
		}
	}
#else
	{
	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:
				break;
			case HT_CHANNEL_WIDTH_20_40:
				break;
			default:
					RT_TRACE(COMP_DBG, "PHY_SetRF8225Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth );
				break;
				
		}
	}
	}
#endif
}


extern void PHY_RF6052SetCckTxPower(struct net_device* dev, u8	powerlevel)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32				TxAGC=0;

	if(priv->ieee80211->scanning == 1)
		TxAGC = 0x3f;
	else if(priv->bDynamicTxLowPower == true)
		TxAGC = 0x22;
	else
		TxAGC = powerlevel;

	if(priv->bIgnoreDiffRateTxPowerOffset)
		TxAGC = powerlevel;

	if(TxAGC > RF6052_MAX_TX_PWR)
		TxAGC = RF6052_MAX_TX_PWR;

	rtl8192_setBBreg(dev, rTxAGC_CCK_Mcs32, bTxAGCRateCCK, TxAGC);

}	/* PHY_RF6052SetCckTxPower */



 #if 1
extern void PHY_RF6052SetOFDMTxPower(struct net_device* dev, u8 powerlevel)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32 	writeVal, powerBase0, powerBase1;
	u8 	index = 0;
	u16 	RegOffset[6] = {0xe00, 0xe04, 0xe10, 0xe14, 0xe18, 0xe1c};
	u8    Channel = priv->ieee80211->current_network.channel;
	u8	rfa_pwr[4];
	u8	rfa_lower_bound = 0, rfa_upper_bound = 0 ;
	u8	i;
	u8	rf_pwr_diff = 0;
	u8	Legacy_pwrdiff=0, HT20_pwrdiff=0, BandEdge_Pwrdiff=0;
	u8	ofdm_bandedge_chnl_low=0, ofdm_bandedge_chnl_high=0;


	if (priv->EEPROMVersion != 2)
	powerBase0 = powerlevel + (priv->LegacyHTTxPowerDiff & 0xf); 
	else if (priv->EEPROMVersion == 2)	
	{
		Legacy_pwrdiff = priv->TxPwrLegacyHtDiff[RF90_PATH_A][Channel-1];
		powerBase0 = powerlevel + Legacy_pwrdiff; 

		if (priv->TxPwrbandEdgeFlag == 1/* && pHalData->ChannelPlan == 0*/)		
		{
			ofdm_bandedge_chnl_low = 1;
			ofdm_bandedge_chnl_high = 11;
		#if 0
			if (pHalData->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
			{	
				if(pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER)
				else if(pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER;
				else
				pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;	
			}
		#endif
			BandEdge_Pwrdiff = 0;
			if (Channel <= ofdm_bandedge_chnl_low)
				BandEdge_Pwrdiff = priv->TxPwrbandEdgeLegacyOfdm[RF90_PATH_A][0];
			else if (Channel >= ofdm_bandedge_chnl_high)
			{
				BandEdge_Pwrdiff = priv->TxPwrbandEdgeLegacyOfdm[RF90_PATH_A][1];
			}
			powerBase0 -= BandEdge_Pwrdiff;
			if (Channel <= ofdm_bandedge_chnl_low || Channel >= ofdm_bandedge_chnl_high)
			{
			}
		}
	}
	powerBase0 = (powerBase0<<24) | (powerBase0<<16) |(powerBase0<<8) |powerBase0;
	
	if(priv->EEPROMVersion == 2)
	{	
	
		if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
		{
			HT20_pwrdiff = priv->TxPwrHt20Diff[RF90_PATH_A][Channel-1];

			if (HT20_pwrdiff < 8)	
				powerlevel += HT20_pwrdiff;	
			else				
				powerlevel -= (16-HT20_pwrdiff);

		}

		if (priv->TxPwrbandEdgeFlag == 1/* && pHalData->ChannelPlan == 0*/)		
		{
			BandEdge_Pwrdiff = 0;
			if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
			{
				if (Channel <= 3)
					BandEdge_Pwrdiff = priv->TxPwrbandEdgeHt40[RF90_PATH_A][0];	
				else if (Channel >= 9)					
					BandEdge_Pwrdiff = priv->TxPwrbandEdgeHt40[RF90_PATH_A][1];
				if (Channel <= 3 || Channel >= 9)
				{
				}
			}
			else if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
			{
				if (Channel <= 1)
					BandEdge_Pwrdiff = priv->TxPwrbandEdgeHt20[RF90_PATH_A][0];	
				else if (Channel >= 11)
					BandEdge_Pwrdiff = priv->TxPwrbandEdgeHt20[RF90_PATH_A][1];
				if (Channel <= 1 || Channel >= 11)
				{
				}
			}
			powerlevel -= BandEdge_Pwrdiff;
		}
	}
	powerBase1 = powerlevel;							
	powerBase1 = (powerBase1<<24) | (powerBase1<<16) |(powerBase1<<8) |powerBase1;

	
	for(index=0; index<6; index++)
	{
		if(priv->bIgnoreDiffRateTxPowerOffset)
			writeVal = ((index<2)?powerBase0:powerBase1);
		else
		writeVal = priv->MCSTxPowerLevelOriginalOffset[index] + ((index<2)?powerBase0:powerBase1);


		if (priv->rf_type == RF_2T2R)
		{			
		#if 0
			if (index > 1)
			{
				rf_pwr_diff = pHalData->AntennaTxPwDiff[0];					
			}
			else
			{
				rf_pwr_diff = pHalData->AntTxPwDiffLegacy[0];					
			}
		#endif
			rf_pwr_diff = priv->AntennaTxPwDiff[0];

			if (rf_pwr_diff >= 8)		
			{	
				rfa_lower_bound = 0x10-rf_pwr_diff;
			}
			else if (rf_pwr_diff >= 0)	
			{
				rfa_upper_bound = RF6052_MAX_TX_PWR-rf_pwr_diff;
			}			
		}

		for (i=  0; i <4; i++)
		{
			rfa_pwr[i] = (u8)((writeVal & (0x7f<<(i*8)))>>(i*8));
			if (rfa_pwr[i]  > RF6052_MAX_TX_PWR)
				rfa_pwr[i]  = RF6052_MAX_TX_PWR;

			if (priv->rf_type == RF_2T2R)
			{
				if (rf_pwr_diff >= 8)		
				{	
					if (rfa_pwr[i] <rfa_lower_bound)
					{
						rfa_pwr[i] = rfa_lower_bound;
					}
				}
				else if (rf_pwr_diff >= 1)	
				{	
					if (rfa_pwr[i] > rfa_upper_bound)
					{
						rfa_pwr[i] = rfa_upper_bound;
					}
				}
			}

		}

		if(priv->bDynamicTxHighPower == TRUE)     
		{	
			if(index > 1)	
			{
				writeVal = 0x03030303;
			}
			else
			{
				writeVal = (rfa_pwr[3]<<24) | (rfa_pwr[2]<<16) |(rfa_pwr[1]<<8) |rfa_pwr[0];
			}
		}
		else
		{
			writeVal = (rfa_pwr[3]<<24) | (rfa_pwr[2]<<16) |(rfa_pwr[1]<<8) |rfa_pwr[0];
		}

		rtl8192_setBBreg(dev, RegOffset[index], 0x7f7f7f7f, writeVal);
	}

}	/* PHY_RF6052SetOFDMTxPower */
#else
extern void PHY_RF6052SetOFDMTxPower(struct net_device* dev, u8 powerlevel)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32 	writeVal, powerBase0, powerBase1;
	u8 	index = 0;
	u16 	RegOffset[6] = {0xe00, 0xe04, 0xe10, 0xe14, 0xe18, 0xe1c};
	u8 	byte0, byte1, byte2, byte3;
	u8    channel = priv->ieee80211->current_network.channel;

	powerBase0 = powerlevel + (priv->LegacyHTTxPowerDiff & 0xf);	
	powerBase0 = (powerBase0<<24) | (powerBase0<<16) |(powerBase0<<8) |powerBase0;
	
	powerBase1 = powerlevel;							
	powerBase1 = (powerBase1<<24) | (powerBase1<<16) |(powerBase1<<8) |powerBase1;

	
	for(index=0; index<6; index++)
	{
		writeVal = priv->MCSTxPowerLevelOriginalOffset[index] +  ((index<2)?powerBase0:powerBase1);


		byte0 = (u8)(writeVal & 0x7f);
		byte1 = (u8)((writeVal & 0x7f00)>>8);
		byte2 = (u8)((writeVal & 0x7f0000)>>16);
		byte3 = (u8)((writeVal & 0x7f000000)>>24);

		if(byte0 > RF6052_MAX_TX_PWR)	
			byte0 = RF6052_MAX_TX_PWR;
		if(byte1 > RF6052_MAX_TX_PWR)
			byte1 = RF6052_MAX_TX_PWR;
		if(byte2 > RF6052_MAX_TX_PWR)
			byte2 = RF6052_MAX_TX_PWR;
		if(byte3 > RF6052_MAX_TX_PWR)
			byte3 = RF6052_MAX_TX_PWR;

		if(priv->bDynamicTxHighPower == true)     
		{	
			if(index > 1)	
			{
				writeVal = 0x03030303;
			}
			else
			{
				writeVal = (byte3<<24) | (byte2<<16) |(byte1<<8) |byte0;
			}
		}
		else
		{
			writeVal = (byte3<<24) | (byte2<<16) |(byte1<<8) |byte0;
		}

		rtl8192_setBBreg(dev, RegOffset[index], 0x7f7f7f7f, writeVal);
	}

}	/* PHY_RF6052SetOFDMTxPower */
#endif

RT_STATUS PHY_RF6052_Config(struct net_device* dev)
{
	struct r8192_priv 			*priv = ieee80211_priv(dev);
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;	
	
	if(priv->rf_type == RF_1T1R)
		priv->NumTotalRFPath = 1;
	else
		priv->NumTotalRFPath = 2;


			rtStatus = phy_RF6052_Config_ParaFile(dev);


	return rtStatus;
		
}

void phy_RF6052_Config_HardCode(struct net_device* dev)
{
	

	
}

RT_STATUS phy_RF6052_Config_ParaFile(struct net_device* dev)
{
	u32			u4RegValue = 0;
	u8			eRFPath;
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg;	
	

	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{

		pPhyReg = &priv->PHYRegDef[eRFPath];
		
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			u4RegValue = rtl8192_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			u4RegValue = rtl8192_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16);
			break;
		}

		rtl8192_setBBreg(dev, pPhyReg->rfintfe, bRFSI_RFENV<<16, 0x1);
		
		rtl8192_setBBreg(dev, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);

		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0); 	
		rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0);	


		switch(eRFPath)
		{
		case RF90_PATH_A:
#if	RTL8190_Download_Firmware_From_Header
			rtStatus= rtl8192_phy_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath);
#else
			rtStatus = PHY_ConfigRFWithParaFile(Adapter, (char* )&szRadioAFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
			break;
		case RF90_PATH_B:
#if	RTL8190_Download_Firmware_From_Header
			rtStatus= rtl8192_phy_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath);
#else
			if(priv->rf_type == RF_2T2R_GREEN)
				rtStatus = PHY_ConfigRFWithParaFile(Adapter, (ps1Byte)&szRadioBGMFile, (RF90_RADIO_PATH_E)eRFPath);
			else
				rtStatus = PHY_ConfigRFWithParaFile(Adapter, (char* )&szRadioBFile, (RF90_RADIO_PATH_E)eRFPath);
#endif
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		}

		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV, u4RegValue);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16, u4RegValue);
			break;
		}

		if(rtStatus != RT_STATUS_SUCCESS){
			printk("phy_RF6052_Config_ParaFile():Radio[%d] Fail!!", eRFPath);
			goto phy_RF6052_Config_ParaFile_Fail;
		}

	}

	RT_TRACE(COMP_INIT, "<---phy_RF6052_Config_ParaFile()\n");
	return rtStatus;
	
phy_RF6052_Config_ParaFile_Fail:	
	return rtStatus;
}


extern u32 PHY_RFShadowRead(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset)
{
	return	RF_Shadow[eRFPath][Offset].Value;
	
}	/* PHY_RFShadowRead */


extern void PHY_RFShadowWrite(
	struct net_device		* dev,
	u32	eRFPath,
	u32					Offset,
	u32					Data)
{
	RF_Shadow[eRFPath][Offset].Value = (Data & bRFRegOffsetMask);
	RF_Shadow[eRFPath][Offset].Driver_Write = true;
	
}	/* PHY_RFShadowWrite */


extern void PHY_RFShadowCompare(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset)
{
	u32	reg;
	
	if (RF_Shadow[eRFPath][Offset].Compare == true)
	{
		reg = rtl8192_phy_QueryRFReg(dev, eRFPath, Offset, bRFRegOffsetMask);
		if (RF_Shadow[eRFPath][Offset].Value != reg)
		{
			RF_Shadow[eRFPath][Offset].ErrorOrNot = true;
			RT_TRACE(COMP_INIT, "PHY_RFShadowCompare RF-%d Addr%02xErr = %05x", eRFPath, Offset, reg);
		}
	}

}	/* PHY_RFShadowCompare */

extern void PHY_RFShadowRecorver(
	struct net_device		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset)
{
	if (RF_Shadow[eRFPath][Offset].ErrorOrNot == true)
	{
		if (RF_Shadow[eRFPath][Offset].Recorver == true)
		{
			rtl8192_phy_SetRFReg(dev, eRFPath, Offset, bRFRegOffsetMask, RF_Shadow[eRFPath][Offset].Value);
			RT_TRACE(COMP_INIT, "PHY_RFShadowRecorver RF-%d Addr%02x=%05x", 
			eRFPath, Offset, RF_Shadow[eRFPath][Offset].Value);
		}
	}
	
}	/* PHY_RFShadowRecorver */


extern void PHY_RFShadowCompareAll(struct net_device * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			PHY_RFShadowCompare(dev, (RF90_RADIO_PATH_E)eRFPath, Offset);
		}
	}
	
}	/* PHY_RFShadowCompareAll */


extern void PHY_RFShadowRecorverAll(struct net_device * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			PHY_RFShadowRecorver(dev, (RF90_RADIO_PATH_E)eRFPath, Offset);
		}
	}
	
}	/* PHY_RFShadowRecorverAll */


extern void PHY_RFShadowCompareFlagSet(
	struct net_device 		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset,
	u8					Type)
{
	RF_Shadow[eRFPath][Offset].Compare = Type;
		
}	/* PHY_RFShadowCompareFlagSet */


extern void PHY_RFShadowRecorverFlagSet(
	struct net_device 		* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32					Offset,
	u8					Type)
{
	RF_Shadow[eRFPath][Offset].Recorver= Type;
		
}	/* PHY_RFShadowRecorverFlagSet */


extern void PHY_RFShadowCompareFlagSetAll(struct net_device  * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			if (Offset != 0x26 && Offset != 0x27)
				PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, FALSE);
			else
				PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, TRUE);
		}
	}
		
}	/* PHY_RFShadowCompareFlagSetAll */


extern void PHY_RFShadowRecorverFlagSetAll(struct net_device  * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			if (Offset != 0x26 && Offset != 0x27)
				PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, FALSE);
			else
				PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, TRUE);
		}
	}
		
}	/* PHY_RFShadowCompareFlagSetAll */



extern void PHY_RFShadowRefresh(struct net_device  * dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			RF_Shadow[eRFPath][Offset].Value = 0;
			RF_Shadow[eRFPath][Offset].Compare = false;
			RF_Shadow[eRFPath][Offset].Recorver  = false;
			RF_Shadow[eRFPath][Offset].ErrorOrNot = false;
			RF_Shadow[eRFPath][Offset].Driver_Write = false;
		}
	}
	
}	/* PHY_RFShadowRead */

