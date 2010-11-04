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

#ifdef RTL8192CE

#include "../rtl_core.h"

/*---------------------------Define Local Constant---------------------------*/
typedef struct RF_Shadow_Compare_Map {
	u32		Value;
	u8		Compare;
	u8		ErrorOrNot;
	u8		Recorver;
	u8		Driver_Write;
}RF_SHADOW_T;
/*---------------------------Define Local Constant---------------------------*/


/*------------------------Define global variable-----------------------------*/
/*------------------------Define global variable-----------------------------*/


/*------------------------Define local variable------------------------------*/
static	RF_SHADOW_T	RF_Shadow[RF6052_MAX_PATH][RF6052_MAX_REG] ;
/*------------------------Define local variable------------------------------*/


/*---------------------Define local function prototype-----------------------*/
void
phy_RF6052_Config_HardCode(
	struct net_device* dev
	);

RT_STATUS
phy_RF6052_Config_ParaFile(
	struct net_device* dev
	);
/*---------------------Define local function prototype-----------------------*/

/*------------------------Define function prototype--------------------------*/
extern	void		RF_ChangeTxPath(	struct net_device* dev, 
										u16		DataRate);

/*------------------------Define function prototype--------------------------*/


/*------------------------Define function prototype--------------------------*/
/*-----------------------------------------------------------------------------
 * Function:	RF_ChangeTxPath
 *
 * Overview:	For RL6052, we must change some RF settign for 1T or 2T.
 *
 * Input:		u16 DataRate		
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 09/25/2008 	MHC		Create Version 0.
 *						Firmwaer support the utility later.
 *
 *---------------------------------------------------------------------------*/
extern	void		RF_ChangeTxPath(	struct net_device* dev, 
										u16		DataRate)
{
	
}	/* RF_ChangeTxPath */


/*-----------------------------------------------------------------------------
 * Function:    PHY_RF6052SetBandwidth()
 *
 * Overview:    This function is called by SetBWModeCallback8190Pci() only
 *
 * Input:       PADAPTER				Adapter
 *			WIRELESS_BANDWIDTH_E	Bandwidth	
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Note:		For RF type 0222D
 *---------------------------------------------------------------------------*/
void
PHY_RF6052SetBandwidth(
	struct net_device* dev,
	HT_CHANNEL_WIDTH		Bandwidth)	
{	
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
		switch(Bandwidth)
		{
			case HT_CHANNEL_WIDTH_20:
				priv->RfRegChnlVal[0] = ((priv->RfRegChnlVal[0] & 0xfffff3ff) | 0x0400);
				PHY_SetRFReg(dev, RF90_PATH_A, RF_CHNLBW, bRFRegOffsetMask, priv->RfRegChnlVal[0]);
				break;
				
			case HT_CHANNEL_WIDTH_20_40:
				priv->RfRegChnlVal[0] = ((priv->RfRegChnlVal[0] & 0xfffff3ff));
				PHY_SetRFReg(dev, RF90_PATH_A, RF_CHNLBW, bRFRegOffsetMask, priv->RfRegChnlVal[0]);			
                                break;
				
			default:
				RT_TRACE(COMP_DBG, "PHY_SetRF8225Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth);
				break;			
		}
}


/*-----------------------------------------------------------------------------
 * Function:	PHY_RF6052SetCckTxPower
 *
 * Overview:	
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/05/2008 	MHC		Simulate 8192series..
 *
 *---------------------------------------------------------------------------*/
#define		TxHighPwrLevel_Normal		0	
#define		TxHighPwrLevel_Level1		1
#define		TxHighPwrLevel_Level2		2

extern	void
PHY_RF6052SetCckTxPower(
	struct net_device* dev,
	u8*			pPowerlevel)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32				TxAGC[2]={0, 0}, tmpval=0;
	bool				TurboScanOff=false;
	u8				idx1, idx2;
	u8*				ptr;
	
	if (priv->EEPROMRegulatory != 0)
		TurboScanOff = true;
	
	if(rtllib_act_scanning(priv->rtllib,true) == true)
	{
		TxAGC[RF90_PATH_A] = 0x3f3f3f3f;
		TxAGC[RF90_PATH_B] = 0x3f3f3f3f;
		if(TurboScanOff)
		{
			for(idx1=RF90_PATH_A; idx1<=RF90_PATH_B; idx1++)
			{
				TxAGC[idx1] = 
					pPowerlevel[idx1] | (pPowerlevel[idx1]<<8) |
					(pPowerlevel[idx1]<<16) | (pPowerlevel[idx1]<<24);
			}
		}
	}
	else
	{
#ifdef ENABLE_DYNAMIC_TXPOWER
		if(priv->DynamicTxHighPowerLvl == TxHighPwrLevel_Level1)
		{	
			TxAGC[RF90_PATH_A] = 0x10101010;
			TxAGC[RF90_PATH_B] = 0x10101010;
		}
		else if(priv->DynamicTxHighPowerLvl == TxHighPwrLevel_Level1)
		{	
			TxAGC[RF90_PATH_A] = 0x00000000;
			TxAGC[RF90_PATH_B] = 0x00000000;
		}
		else
#endif
		{
			for(idx1=RF90_PATH_A; idx1<=RF90_PATH_B; idx1++)
			{
				TxAGC[idx1] = 
					pPowerlevel[idx1] | (pPowerlevel[idx1]<<8) |
					(pPowerlevel[idx1]<<16) | (pPowerlevel[idx1]<<24);
			}

			if(priv->EEPROMRegulatory==0)
			{
				tmpval = (priv->MCSTxPowerLevelOriginalOffset[0][6]) + 
						(priv->MCSTxPowerLevelOriginalOffset[0][7]<<8);
				TxAGC[RF90_PATH_A] += tmpval;
				
				tmpval = (priv->MCSTxPowerLevelOriginalOffset[0][14]) + 
						(priv->MCSTxPowerLevelOriginalOffset[0][15]<<24);
				TxAGC[RF90_PATH_B] += tmpval;
			}
		}
	}

	for(idx1=RF90_PATH_A; idx1<=RF90_PATH_B; idx1++)
	{
		ptr = (u8*)(&(TxAGC[idx1]));
		for(idx2=0; idx2<4; idx2++)
		{
			if(*ptr > RF6052_MAX_TX_PWR)
				*ptr = RF6052_MAX_TX_PWR;
			ptr++;
		}
	}

	tmpval = TxAGC[RF90_PATH_A]&0xff;
	PHY_SetBBReg(dev, rTxAGC_A_CCK1_Mcs32, bMaskByte1, tmpval);
	RTPRINT(FPHY, PHY_TXPWR, ("CCK PWR 1M (rf-A) = 0x%x (reg 0x%x)\n", tmpval, rTxAGC_A_CCK1_Mcs32));
	tmpval = TxAGC[RF90_PATH_A]>>8;
	PHY_SetBBReg(dev, rTxAGC_B_CCK11_A_CCK2_11, 0xffffff00, tmpval);
	RTPRINT(FPHY, PHY_TXPWR, ("CCK PWR 2~11M (rf-A) = 0x%x (reg 0x%x)\n", tmpval, rTxAGC_B_CCK11_A_CCK2_11));

	tmpval = TxAGC[RF90_PATH_B]>>24;
	PHY_SetBBReg(dev, rTxAGC_B_CCK11_A_CCK2_11, bMaskByte0, tmpval);
	RTPRINT(FPHY, PHY_TXPWR, ("CCK PWR 11M (rf-B) = 0x%x (reg 0x%x)\n", tmpval, rTxAGC_B_CCK11_A_CCK2_11));
	tmpval = TxAGC[RF90_PATH_B]&0x00ffffff;
	PHY_SetBBReg(dev, rTxAGC_B_CCK1_55_Mcs32, 0xffffff00, tmpval);
	RTPRINT(FPHY, PHY_TXPWR, ("CCK PWR 1~5.5M (rf-B) = 0x%x (reg 0x%x)\n", 
		tmpval, rTxAGC_B_CCK1_55_Mcs32));
}	/* PHY_RF6052SetCckTxPower */


void getPowerBase(
	struct net_device* dev,
	u8*		pPowerLevel,
	u8		Channel,
	u32*	OfdmBase,
	u32*	MCSBase
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32			powerBase0, powerBase1;
	u8			Legacy_pwrdiff=0, HT20_pwrdiff=0;
	u8			i, powerlevel[2];

	for(i=0; i<2; i++)
	{
		powerlevel[i] = pPowerLevel[i];
		Legacy_pwrdiff = priv->TxPwrLegacyHtDiff[i][Channel-1];
		powerBase0 = powerlevel[i] + Legacy_pwrdiff; 

		powerBase0 = (powerBase0<<24) | (powerBase0<<16) |(powerBase0<<8) |powerBase0;
		*(OfdmBase+i) = powerBase0;
		RTPRINT(FPHY, PHY_TXPWR, (" [OFDM power base index rf(%c) = 0x%x]\n", ((i==0)?'A':'B'), *(OfdmBase+i)));
	}

	for(i=0; i<2; i++)
	{
		if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
		{
			HT20_pwrdiff = priv->TxPwrHt20Diff[i][Channel-1];
			powerlevel[i] += HT20_pwrdiff;
		}
		powerBase1 = powerlevel[i];
		powerBase1 = (powerBase1<<24) | (powerBase1<<16) |(powerBase1<<8) |powerBase1;
		*(MCSBase+i) = powerBase1;
		RTPRINT(FPHY, PHY_TXPWR, (" [MCS power base index rf(%c) = 0x%x]\n", ((i==0)?'A':'B'), *(MCSBase+i)));
	}
}

void getTxPowerWriteValByRegulatory(
	struct net_device* dev,
	u8		Channel,
	u8		index,
	u32*		powerBase0,
	u32*		powerBase1,
	u32*		pOutWriteVal
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8	i, chnlGroup = 0, pwr_diff_limit[4];
	u32 	writeVal, customer_limit, rf;
	
	for(rf=0; rf<2; rf++)
	{
		switch(priv->EEPROMRegulatory)
		{
			case 0:	
				chnlGroup = 0;
				RTPRINT(FPHY, PHY_TXPWR, ("MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n", 
					chnlGroup, index, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index+(rf?8:0)]));
				writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index+(rf?8:0)] + 
					((index<2)?powerBase0[rf]:powerBase1[rf]);
				RTPRINT(FPHY, PHY_TXPWR, ("RTK better performance, writeVal(%c) = 0x%x\n", ((rf==0)?'A':'B'), writeVal));
				break;
			case 1:	
				if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
				{
					writeVal = ((index<2)?powerBase0[rf]:powerBase1[rf]);
					RTPRINT(FPHY, PHY_TXPWR, ("Realtek regulatory, 40MHz, writeVal(%c) = 0x%x\n", ((rf==0)?'A':'B'), writeVal));
				}
				else
				{
					if(priv->pwrGroupCnt == 1)
						chnlGroup = 0;
					if(priv->pwrGroupCnt >= 3)
					{
						if(Channel <= 3)
							chnlGroup = 0;
						else if(Channel >= 4 && Channel <= 9)
							chnlGroup = 1;
						else if(Channel > 9)
							chnlGroup = 2;
						if(priv->pwrGroupCnt == 4)
							chnlGroup++;
					}
					RTPRINT(FPHY, PHY_TXPWR, ("MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n", 
						chnlGroup, index, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index+(rf?8:0)]));
					writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index+(rf?8:0)] + 
						((index<2)?powerBase0[rf]:powerBase1[rf]);
					RTPRINT(FPHY, PHY_TXPWR, ("Realtek regulatory, 20MHz, writeVal(%c) = 0x%x\n", ((rf==0)?'A':'B'), writeVal));
				}
				break;
			case 2:	
				writeVal = ((index<2)?powerBase0[rf]:powerBase1[rf]);
				RTPRINT(FPHY, PHY_TXPWR, ("Better regulatory, writeVal(%c) = 0x%x\n", ((rf==0)?'A':'B'), writeVal));
				break;
			case 3:	
				chnlGroup = 0;
				RTPRINT(FPHY, PHY_TXPWR, ("MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n", 
					chnlGroup, index, priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index+(rf?8:0)]));

				if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
				{
					RTPRINT(FPHY, PHY_TXPWR, ("customer's limit, 40MHz rf(%c) = 0x%x\n", 
						((rf==0)?'A':'B'), priv->PwrGroupHT40[rf][Channel-1]));
				}
				else
				{
					RTPRINT(FPHY, PHY_TXPWR, ("customer's limit, 20MHz rf(%c) = 0x%x\n", 
						((rf==0)?'A':'B'), priv->PwrGroupHT20[rf][Channel-1]));
				}
				for (i=0; i<4; i++)
				{
					pwr_diff_limit[i] = (u8)((priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index+(rf?8:0)]&(0x7f<<(i*8)))>>(i*8));
					if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
					{
						if(pwr_diff_limit[i] > priv->PwrGroupHT40[rf][Channel-1])
							pwr_diff_limit[i] = priv->PwrGroupHT40[rf][Channel-1];
					}
					else
					{
						if(pwr_diff_limit[i] > priv->PwrGroupHT20[rf][Channel-1])
							pwr_diff_limit[i] = priv->PwrGroupHT20[rf][Channel-1];
					}
				}
				customer_limit = (pwr_diff_limit[3]<<24) | (pwr_diff_limit[2]<<16) |
								(pwr_diff_limit[1]<<8) | (pwr_diff_limit[0]);
				RTPRINT(FPHY, PHY_TXPWR, ("Customer's limit rf(%c) = 0x%x\n", ((rf==0)?'A':'B'), customer_limit));

				writeVal = customer_limit + ((index<2)?powerBase0[rf]:powerBase1[rf]);
				RTPRINT(FPHY, PHY_TXPWR, ("Customer, writeVal rf(%c)= 0x%x\n", ((rf==0)?'A':'B'), writeVal));
				break;
			default:
				chnlGroup = 0;
				writeVal = priv->MCSTxPowerLevelOriginalOffset[chnlGroup][index+(rf?8:0)] + 
						((index<2)?powerBase0[rf]:powerBase1[rf]);
				RTPRINT(FPHY, PHY_TXPWR, ("RTK better performance, writeVal rf(%c) = 0x%x\n", ((rf==0)?'A':'B'), writeVal));
				break;
		}

#ifdef ENABLE_DYNAMIC_TXPOWER
		{
			if(priv->DynamicTxHighPowerLvl == TxHighPwrLevel_Level1)
				writeVal = 0x14141414;
			else if(priv->DynamicTxHighPowerLvl == TxHighPwrLevel_Level2)
				writeVal = 0x00000000;
		}
#endif
		*(pOutWriteVal+rf) = writeVal;
	}
}

void writeOFDMPowerReg(
	struct net_device* dev,
	u8		index,
	u32*		pValue
	)
{
	u16 RegOffset_A[6] = {	rTxAGC_A_Rate18_06, rTxAGC_A_Rate54_24, 
							rTxAGC_A_Mcs03_Mcs00, rTxAGC_A_Mcs07_Mcs04, 
							rTxAGC_A_Mcs11_Mcs08, rTxAGC_A_Mcs15_Mcs12};
	u16 RegOffset_B[6] = {	rTxAGC_B_Rate18_06, rTxAGC_B_Rate54_24, 
							rTxAGC_B_Mcs03_Mcs00, rTxAGC_B_Mcs07_Mcs04,
							rTxAGC_B_Mcs11_Mcs08, rTxAGC_B_Mcs15_Mcs12};
	u8 i, rf, pwr_val[4];
	u32 writeVal;
	u16 RegOffset;

	for(rf=0; rf<2; rf++)
	{
		writeVal = pValue[rf];
		for(i=0; i<4; i++)
		{
			pwr_val[i] = (u8)((writeVal & (0x7f<<(i*8)))>>(i*8));
			if (pwr_val[i]  > RF6052_MAX_TX_PWR)
				pwr_val[i]  = RF6052_MAX_TX_PWR;
		}
		writeVal = (pwr_val[3]<<24) | (pwr_val[2]<<16) |(pwr_val[1]<<8) |pwr_val[0];

		if(rf == 0)
			RegOffset = RegOffset_A[index];
		else
			RegOffset = RegOffset_B[index];
		PHY_SetBBReg(dev, RegOffset, bMaskDWord, writeVal);
		RTPRINT(FPHY, PHY_TXPWR, ("Set 0x%x = %08x\n", RegOffset, writeVal));	
	}
}
/*-----------------------------------------------------------------------------
 * Function:	PHY_RF6052SetOFDMTxPower
 *
 * Overview:	For legacy and HY OFDM, we must read EEPROM TX power index for 
 *			different channel and read original value in TX power register area from
 *			0xe00. We increase offset and original value to be correct tx pwr.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/05/2008 	MHC		Simulate 8192 series method.
 * 01/06/2009	MHC		1. Prevent Path B tx power overflow or underflow dure to
 *						A/B pwr difference or legacy/HT pwr diff.
 *						2. We concern with path B legacy/HT OFDM difference.
 * 01/22/2009	MHC		Support new EPRO format from SD3.
 *
 *---------------------------------------------------------------------------*/
extern	void 
PHY_RF6052SetOFDMTxPower(
	struct net_device* dev,
	u8*		pPowerLevel,
	u8		Channel)
{
	u32 writeVal[2], powerBase0[2], powerBase1[2];
	u8 index = 0;

	getPowerBase(dev, pPowerLevel, Channel, &powerBase0[0], &powerBase1[0]);

	for(index=0; index<6; index++)
	{
		getTxPowerWriteValByRegulatory(dev, Channel, index, 
			&powerBase0[0], &powerBase1[0], &writeVal[0]);

		writeOFDMPowerReg(dev, index, &writeVal[0]);
	}
}


bool
PHY_RF6052_Config(
	struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	RT_STATUS					rtStatus = RT_STATUS_SUCCESS;	
	u8	bRegHwParaFile = 1;
	
	if(priv->rf_chip == RF_1T1R)
		priv->NumTotalRFPath = 1;
	else
		priv->NumTotalRFPath = 2;

	switch( bRegHwParaFile )
	{
		case 0:
			phy_RF6052_Config_HardCode(dev);
			break;

		case 1:
			rtStatus = phy_RF6052_Config_ParaFile(dev);
			break;

		case 2:
			phy_RF6052_Config_HardCode(dev);
			phy_RF6052_Config_ParaFile(dev);
			break;

		default:
			phy_RF6052_Config_HardCode(dev);
			break;
	}
	return (rtStatus == RT_STATUS_SUCCESS)?1:0;
		
}

void
phy_RF6052_Config_HardCode(
	struct net_device* dev
	)
{
	

	
}

RT_STATUS
phy_RF6052_Config_ParaFile(
	struct net_device* dev
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32					u4RegValue = 0;
	u8					eRFPath;
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	BB_REGISTER_DEFINITION_T	*pPhyReg;	


	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{

		pPhyReg = &priv->PHYRegDef[eRFPath];
		
		/*----Store original RFENV control type----*/		
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			u4RegValue = PHY_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			u4RegValue = PHY_QueryBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16);
			break;
		}

		/*----Set RF_ENV enable----*/		
		PHY_SetBBReg(dev, pPhyReg->rfintfe, bRFSI_RFENV<<16, 0x1);
		udelay(1);
		
		/*----Set RF_ENV output high----*/
		PHY_SetBBReg(dev, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);
		udelay(1);
		
		/* Set bit number of Address and Data for RF register */
		PHY_SetBBReg(dev, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0); 	
		udelay(1);

		PHY_SetBBReg(dev, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0);	
		udelay(1);

		/*----Initialize RF fom connfiguration file----*/
		switch(eRFPath)
		{
		case RF90_PATH_A:
#if	RTL8190_Download_Firmware_From_Header
			rtStatus= (PHY_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath) == true)?RT_STATUS_SUCCESS:RT_STATUS_FAILURE;
#else
			rtStatus = (PHY_ConfigRFWithParaFile(dev, szRadioAFile, (RF90_RADIO_PATH_E)eRFPath) == true)?RT_STATUS_SUCCESS:RT_STATUS_FAILURE;
#endif
			break;
		case RF90_PATH_B:
#if	RTL8190_Download_Firmware_From_Header
			rtStatus= (PHY_ConfigRFWithHeaderFile(dev,(RF90_RADIO_PATH_E)eRFPath) == true)?RT_STATUS_SUCCESS:RT_STATUS_FAILURE;
#else

			rtStatus = (PHY_ConfigRFWithParaFile(dev, szRadioBFile, (RF90_RADIO_PATH_E)eRFPath) == true)?RT_STATUS_SUCCESS:RT_STATUS_FAILURE;
#endif
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		}

		/*----Restore RFENV control type----*/;
		switch(eRFPath)
		{
		case RF90_PATH_A:
		case RF90_PATH_C:
			PHY_SetBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV, u4RegValue);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			PHY_SetBBReg(dev, pPhyReg->rfintfs, bRFSI_RFENV<<16, u4RegValue);
			break;
		}

		if(rtStatus != RT_STATUS_SUCCESS){
			RT_TRACE(COMP_INIT, "phy_RF6052_Config_ParaFile():Radio[%d] Fail!!", eRFPath);
			goto phy_RF6052_Config_ParaFile_Fail;
		}

	}

	RT_TRACE(COMP_INIT, "<---phy_RF6052_Config_ParaFile()\n");
	return rtStatus;
	
phy_RF6052_Config_ParaFile_Fail:	
	return rtStatus;
}


/*-----------------------------------------------------------------------------
 * Function:	PHY_RFShadowRead
 *				PHY_RFShadowWrite
 *				PHY_RFShadowCompare
 *				PHY_RFShadowRecorver
 *				PHY_RFShadowCompareAll
 *				PHY_RFShadowRecorverAll
 *				PHY_RFShadowCompareFlagSet
 *				PHY_RFShadowRecorverFlagSet
 *
 * Overview:	When we set RF register, we must write shadow at first.
 *			When we are running, we must compare shadow abd locate error addr.
 *			Decide to recorver or not.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/20/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	u32
PHY_RFShadowRead(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset)
{
	return	RF_Shadow[eRFPath][Offset].Value;
	
}	/* PHY_RFShadowRead */


extern	void
PHY_RFShadowWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data)
{
	RF_Shadow[eRFPath][Offset].Value = (Data & bRFRegOffsetMask);
	RF_Shadow[eRFPath][Offset].Driver_Write = true;
	
}	/* PHY_RFShadowWrite */


extern	bool
PHY_RFShadowCompare(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset)
{
	u32	reg;
	if (RF_Shadow[eRFPath][Offset].Compare == true)
	{
		reg = PHY_QueryRFReg(dev, eRFPath, Offset, bRFRegOffsetMask);
		if (RF_Shadow[eRFPath][Offset].Value != reg)
		{
			RF_Shadow[eRFPath][Offset].ErrorOrNot = true;
			RT_TRACE(COMP_INIT, 
			"PHY_RFShadowCompare RF-%d Addr%02x Err = %05x\n", 
			eRFPath, Offset, reg);
		}
		return RF_Shadow[eRFPath][Offset].ErrorOrNot ;
	}
	return false;
}	/* PHY_RFShadowCompare */


extern	void
PHY_RFShadowRecorver(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset)
{
	if (RF_Shadow[eRFPath][Offset].ErrorOrNot == true)
	{
		if (RF_Shadow[eRFPath][Offset].Recorver == true)
		{
			PHY_SetRFReg(	dev, eRFPath, Offset, bRFRegOffsetMask, 
							RF_Shadow[eRFPath][Offset].Value);
			RT_TRACE(COMP_INIT,  
			"PHY_RFShadowRecorver RF-%d Addr%02x=%05x", 
			eRFPath, Offset, RF_Shadow[eRFPath][Offset].Value);
		}
	}
	
}	/* PHY_RFShadowRecorver */


extern	void
PHY_RFShadowCompareAll(
	struct net_device* dev)
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


extern	void
PHY_RFShadowRecorverAll(
	struct net_device* dev)
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


extern	void
PHY_RFShadowCompareFlagSet(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u8				Type)
{
	RF_Shadow[eRFPath][Offset].Compare = Type;
		
}	/* PHY_RFShadowCompareFlagSet */


extern	void
PHY_RFShadowRecorverFlagSet(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u8				Type)
{
	RF_Shadow[eRFPath][Offset].Recorver= Type;
		
}	/* PHY_RFShadowRecorverFlagSet */


extern	void
PHY_RFShadowCompareFlagSetAll(
	struct net_device* dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			if (Offset != 0x26 && Offset != 0x27)
				PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, false);
			else
				PHY_RFShadowCompareFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, true);
		}
	}
		
}	/* PHY_RFShadowCompareFlagSetAll */


extern	void
PHY_RFShadowRecorverFlagSetAll(
	struct net_device* dev)
{
	u32		eRFPath;
	u32		Offset;

	for (eRFPath = 0; eRFPath < RF6052_MAX_PATH; eRFPath++)
	{
		for (Offset = 0; Offset <= RF6052_MAX_REG; Offset++)
		{
			if (Offset != 0x26 && Offset != 0x27)
				PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, false);
			else
				PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)eRFPath, Offset, true);
		}
	}
		
}	/* PHY_RFShadowCompareFlagSetAll */



extern	void
PHY_RFShadowRefresh(
	struct net_device* dev)
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

/* End of HalRf6052.c */


extern  void    PHY_SetRF8225OfdmTxPower(struct net_device* dev, u8 powerlevel)
{}
extern  void    PHY_SetRF8225CckTxPower(struct net_device* dev, u8 powerlevel)
{}
extern  void    PHY_SetRF8225Bandwidth(struct net_device* dev, HT_CHANNEL_WIDTH Bandwidth)
{}


extern  void    PHY_SetRF8256OFDMTxPower(struct net_device* dev, u8 powerlevel)
{}
extern  void    PHY_SetRF8256CCKTxPower(struct net_device* dev,  u8 powerlevel)
{}
extern  void    PHY_SetRF8256Bandwidth( struct net_device* dev, HT_CHANNEL_WIDTH Bandwidth)
{}
extern  bool    PHY_RF8256_Config(struct net_device* dev)
{
	return false;
}

#endif
