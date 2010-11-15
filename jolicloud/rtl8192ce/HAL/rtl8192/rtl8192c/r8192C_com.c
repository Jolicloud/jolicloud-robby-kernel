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

#include "r8192C_com.h"

u8 GetEEPROMSize8192C(struct net_device *dev)
{
	u8	size = 0;
	u32	curRCR;

	curRCR = read_nic_word(dev, REG_9346CR);
	size = (curRCR & BOOT_FROM_EEPROM) ? 6 : 4; 
	
	RT_TRACE(COMP_INIT, "EEPROM type is %s\n", size==4 ? "E-FUSE" : "93C46");
	return size;
}


VERSION_8192C ReadChipVersion(struct net_device *dev)
{
	u32			value32;
	VERSION_8192C	version = 0;
	struct r8192_priv 	*priv = rtllib_priv(dev);

	value32 = read_nic_dword(dev, REG_SYS_CFG);

#if 0
	if(value32 & TRP_VAUX_EN){
	switch(((value32 & CHIP_VER_RTL_MASK) >> CHIP_VER_RTL_SHIFT))
	{
		case 0: 
			version = VERSION_8192C_TEST_CHIP_91C;
			break;
		case 1: 
			version = VERSION_8192C_TEST_CHIP_88C;
			break;
		default:
			RT_ASSERT(false,("Chip Version can't be recognized.\n"));
			break;
	}
	}
	else{

		version = VERSION_8192C_NORMAL_CHIP;
	}
#else
	if (value32 & TRP_VAUX_EN){		
		version = (value32 & TYPE_ID) ?VERSION_TEST_CHIP_92C :VERSION_TEST_CHIP_88C;		
	}
	else{
		version = (value32 & TYPE_ID) ?VERSION_NORMAL_CHIP_92C :VERSION_NORMAL_CHIP_88C;

		if(version == VERSION_NORMAL_CHIP_92C)
		{
			value32 = read_nic_dword(dev, REG_HPON_FSM);

			if(CHIP_BONDING_IDENTIFIER(value32) == CHIP_BONDING_92C_1T2R)
				version = VERSION_NORMAL_CHIP_92C_1T2R;
		}
	}
#endif

	switch(version)
	{
		case VERSION_NORMAL_CHIP_92C_1T2R:
			RT_TRACE(COMP_INIT, "Chip Version ID: VERSION_NORMAL_CHIP_92C_1T2R.\n");
			break;
		case VERSION_NORMAL_CHIP_92C:
			RT_TRACE(COMP_INIT, "Chip Version ID: VERSION_NORMAL_CHIP_92C.\n");
			printk("Chip Version ID: VERSION_NORMAL_CHIP_92C.\n");
			break;
		case VERSION_NORMAL_CHIP_88C:
			RT_TRACE(COMP_INIT, "Chip Version ID: VERSION_NORMAL_CHIP_88C.\n");
			printk("Chip Version ID: VERSION_NORMAL_CHIP_88C.\n");
			break;
		case VERSION_TEST_CHIP_92C:
			RT_TRACE(COMP_INIT, "Chip Version ID: VERSION_TEST_CHIP_92C.\n");
			printk("Chip Version ID: VERSION_TEST_CHIP_92C.\n");
			break;
		case VERSION_TEST_CHIP_88C:
			RT_TRACE(COMP_INIT, "Chip Version ID: VERSION_TEST_CHIP_88C.\n");
			printk("Chip Version ID: VERSION_TEST_CHIP_88C.\n");
			break;
		default:
			RT_TRACE(COMP_INIT, "Chip Version ID: ???????????????.\n");
			printk("Chip Version ID: ???????????????.\n");
			break;
	}

	priv->card_8192_version= version;

	switch(version & 0x3 )
	{
		case CHIP_88C:
			priv->rf_type = RF_1T1R;
			break;
		case CHIP_92C:
			priv->rf_type = RF_2T2R;
			break;
		case CHIP_92C_1T2R:
			priv->rf_type = RF_1T2R;
			break;
		default:
			priv->rf_type = RF_1T1R;
			RT_TRACE(COMP_INIT, "ERROR RF_Type is set!!");
			break;
	}


	printk("Chip RF Type: %s\n", (priv->rf_type == RF_2T2R)?"RF_2T2R":"RF_1T1R");

	return version;
}


static RT_STATUS _LLTWrite(
	struct net_device 	*dev,
	u32				address,
	u32				data
	)
{
	RT_STATUS	status = RT_STATUS_SUCCESS;
	long 			count = 0;
	u32 			value = _LLT_INIT_ADDR(address) | _LLT_INIT_DATA(data) | _LLT_OP(_LLT_WRITE_ACCESS);

	write_nic_dword(dev, REG_LLT_INIT, value);
	
	do{
		
		value = read_nic_dword(dev, REG_LLT_INIT);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			break;
		}
		
		if(count > POLLING_LLT_THRESHOLD){
			RT_TRACE(COMP_INIT,"Failed to polling write LLT done at address %d!\n", address);
			status = RT_STATUS_FAILURE;
			break;
		}
	}while(count++);

	return status;
	
}


u8 _LLTRead(struct net_device *dev, u32	address)
{
	long		count = 0;
	u32		value = _LLT_INIT_ADDR(address) | _LLT_OP(_LLT_READ_ACCESS);

	write_nic_dword(dev, REG_LLT_INIT, value);

	do{
		
		value = read_nic_dword(dev, REG_LLT_INIT);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			return (u8)value;
		}
		
		if(count > POLLING_LLT_THRESHOLD){
			RT_TRACE(COMP_INIT,"Failed to polling read LLT done at address %d!\n", address);
			break;
		}
	}while(count++);

	return 0xFF;

}


RT_STATUS InitLLTTable(struct net_device *dev, u32 boundary)
{
	RT_STATUS	status = RT_STATUS_SUCCESS;
	u32			i;

	for(i = 0 ; i < (boundary - 1) ; i++){
		status = _LLTWrite(dev, i , i + 1);
		if(RT_STATUS_SUCCESS != status){
			return status;
		}
	}

	status = _LLTWrite(dev, (boundary - 1), 0xFF); 
	if(RT_STATUS_SUCCESS != status){
		return status;
	}

	for(i = boundary ; i < LAST_ENTRY_OF_TX_PKT_BUFFER ; i++){
		status = _LLTWrite(dev, i, (i + 1)); 
		if(RT_STATUS_SUCCESS != status){
			return status;
		}
	}
	
	status = _LLTWrite(dev, LAST_ENTRY_OF_TX_PKT_BUFFER, boundary);
	if(RT_STATUS_SUCCESS != status){
		return status;
	}

	return status;
	
}

bool IsSwChnlInProgress8192C(struct net_device *dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	return priv->SwChnlInProgress;
}

u8 GetRFType8192C(struct net_device *dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	return (priv->rf_type);
}


void SetTxAntenna8192C(struct net_device *dev, u8	 SelectedAntenna)
{
	
	if(IS_HARDWARE_TYPE_8192CE(priv) || IS_HARDWARE_TYPE_8192CU(priv)){
		SetAntennaConfig92C( dev, SelectedAntenna);
	}
}


u16
_HalMapChannelPlan8192C(
	struct net_device *dev,
	u16		HalChannelPlan
	)
{
	u16	rtChannelDomain;

	switch(HalChannelPlan)
	{
		case EEPROM_CHANNEL_PLAN_GLOBAL_DOMAIN:
			rtChannelDomain = COUNTRY_CODE_GLOBAL_DOMAIN;
			break;
		case EEPROM_CHANNEL_PLAN_WORLD_WIDE_13:
			rtChannelDomain = COUNTRY_CODE_WORLD_WIDE_13;
			break;			
		default:
			rtChannelDomain = (u16)HalChannelPlan;
			break;
	}
	
	return 	rtChannelDomain;

}


void
ReadChannelPlan(
	struct net_device 	*dev,
	u8*				PROMContent,
	bool				AutoLoadFail
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8			channelPlan;

	if(AutoLoadFail){
		channelPlan = CHPL_FCC;
	}
	else{
		 channelPlan = PROMContent[EEPROM_CHANNEL_PLAN];
	}

	if((priv->RegChannelPlan >= COUNTRY_CODE_MAX) || (channelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK)){
		priv->ChannelPlan = _HalMapChannelPlan8192C(dev, (channelPlan & (~(EEPROM_CHANNEL_PLAN_BY_HW_MASK))));
		priv->bChnlPlanFromHW = (channelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK) ? true : false; 
	}
	else{
		priv->ChannelPlan = (u16)priv->RegChannelPlan;
	}
#ifdef ENABLE_DOT11D
	switch(priv->ChannelPlan)
	{
		case COUNTRY_CODE_GLOBAL_DOMAIN:
		{
			PRT_DOT11D_INFO	pDot11dInfo = GET_DOT11D_INFO(priv->rtllib);
	
			pDot11dInfo->bEnabled = true;
		}
		RT_TRACE(COMP_INIT, "Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n");
		break;
	}
#endif
	RT_TRACE(COMP_INIT, "RegChannelPlan(%d) EEPROMChannelPlan(%d)", priv->RegChannelPlan, (u32)channelPlan);
	RT_TRACE(COMP_INIT, "ChannelPlan = %d\n" , priv->ChannelPlan);

}




void
_ReadPowerValueFromPROM(
	PTxPowerInfo	pwrInfo,
	u8*			PROMContent,
	bool			AutoLoadFail
	)
{
	u32 rfPath, eeAddr, group;

	memset(pwrInfo, 0, sizeof(TxPowerInfo));

	if(AutoLoadFail){		
		for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
			for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
				pwrInfo->CCKIndex[rfPath][group]		= EEPROM_Default_TxPowerLevel;	
				pwrInfo->HT40_1SIndex[rfPath][group]	= EEPROM_Default_TxPowerLevel;
				pwrInfo->HT40_2SIndexDiff[rfPath][group]	= EEPROM_Default_HT40_2SDiff;
				pwrInfo->HT20IndexDiff[rfPath][group]		= EEPROM_Default_HT20_Diff;
				pwrInfo->OFDMIndexDiff[rfPath][group]	= EEPROM_Default_LegacyHTTxPowerDiff;
				pwrInfo->HT40MaxOffset[rfPath][group]	= EEPROM_Default_HT40_PwrMaxOffset;		
				pwrInfo->HT20MaxOffset[rfPath][group]	= EEPROM_Default_HT20_PwrMaxOffset;
			}
		}

		pwrInfo->TSSI_A = EEPROM_Default_TSSI;
		pwrInfo->TSSI_B = EEPROM_Default_TSSI;
		
		return;
	}
	
	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
			eeAddr = EEPROM_CCK_TX_PWR_INX + (rfPath * 3) + group;
			pwrInfo->CCKIndex[rfPath][group] = PROMContent[eeAddr];

			eeAddr = EEPROM_HT40_1S_TX_PWR_INX + (rfPath * 3) + group;
			pwrInfo->HT40_1SIndex[rfPath][group] = PROMContent[eeAddr];
		}
	}

	for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
		for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
			pwrInfo->HT40_2SIndexDiff[rfPath][group] = 
			(PROMContent[EEPROM_HT40_2S_TX_PWR_INX_DIFF + group] >> (rfPath * 4)) & 0xF;

			pwrInfo->HT20IndexDiff[rfPath][group] =
			(PROMContent[EEPROM_HT20_TX_PWR_INX_DIFF + group] >> (rfPath * 4)) & 0xF;
			
			if(pwrInfo->HT20IndexDiff[rfPath][group] & BIT3)	
				pwrInfo->HT20IndexDiff[rfPath][group] |= 0xF0;

			pwrInfo->OFDMIndexDiff[rfPath][group] =
			(PROMContent[EEPROM_OFDM_TX_PWR_INX_DIFF+ group] >> (rfPath * 4)) & 0xF;

			pwrInfo->HT40MaxOffset[rfPath][group] =
			(PROMContent[EEPROM_HT40_MAX_PWR_OFFSET+ group] >> (rfPath * 4)) & 0xF;

			pwrInfo->HT20MaxOffset[rfPath][group] =
			(PROMContent[EEPROM_HT20_MAX_PWR_OFFSET+ group] >> (rfPath * 4)) & 0xF;
		}
	}

	pwrInfo->TSSI_A = PROMContent[EEPROM_TSSI_A];
	pwrInfo->TSSI_B = PROMContent[EEPROM_TSSI_B];

}


u32 _GetChannelGroup(u32	channel)
{

	if(channel < 3){ 	
		return 0;
	}
	else if(channel < 9){ 
		return 1;
	}

	return 2;				
}


void
ReadTxPowerInfo(
	struct net_device 	*dev,
	u8*				PROMContent,
	bool				AutoLoadFail
	)
{	
	struct r8192_priv 	*priv = rtllib_priv(dev);
	TxPowerInfo		pwrInfo;
	u32			rfPath, ch, group;
	u8			pwr, diff;

	_ReadPowerValueFromPROM(&pwrInfo, PROMContent, AutoLoadFail);

	if(!AutoLoadFail)
		priv->bTXPowerDataReadFromEEPORM = true;
	
	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
			group = _GetChannelGroup(ch);

			priv->TxPwrLevelCck[rfPath][ch]		= pwrInfo.CCKIndex[rfPath][group];
			priv->TxPwrLevelHT40_1S[rfPath][ch]	= pwrInfo.HT40_1SIndex[rfPath][group];

			priv->TxPwrHt20Diff[rfPath][ch]		= pwrInfo.HT20IndexDiff[rfPath][group];
			priv->TxPwrLegacyHtDiff[rfPath][ch]	= pwrInfo.OFDMIndexDiff[rfPath][group];
			priv->PwrGroupHT20[rfPath][ch]		= pwrInfo.HT20MaxOffset[rfPath][group];
			priv->PwrGroupHT40[rfPath][ch]		= pwrInfo.HT40MaxOffset[rfPath][group];

			pwr		= pwrInfo.HT40_1SIndex[rfPath][group];
			diff		= pwrInfo.HT40_2SIndexDiff[rfPath][group];

			priv->TxPwrLevelHT40_2S[rfPath][ch]  	= (pwr > diff) ? (pwr - diff) : 0;
		}
	}

#ifdef DBG

	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
			RTPRINT(FINIT, INIT_TxPower, 
				("RF(%d)-Ch(%d) [CCK / HT40_1S / HT40_2S] = [0x%x / 0x%x / 0x%x]\n", 
				rfPath, ch, priv->TxPwrLevelCck[rfPath][ch], 
				priv->TxPwrLevelHT40_1S[rfPath][ch], 
				priv->TxPwrLevelHT40_2S[rfPath][ch]));

		}
	}

	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Ht20 to HT40 Diff[%d] = 0x%x\n", ch, priv->TxPwrHt20Diff[RF90_PATH_A][ch]));
	}

	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Legacy to Ht40 Diff[%d] = 0x%x\n", ch, priv->TxPwrLegacyHtDiff[RF90_PATH_A][ch]));
	}
	
	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Ht20 to HT40 Diff[%d] = 0x%x\n", ch, priv->TxPwrHt20Diff[RF90_PATH_B][ch]));
	}
	
	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Legacy to HT40 Diff[%d] = 0x%x\n", ch, priv->TxPwrLegacyHtDiff[RF90_PATH_B][ch]));
	}
	
#endif
	
}

void
WKFMCAMAddOneEntry(
	struct net_device 	*dev,
	u8			Index, 
	u16			usConfig
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(priv);
	PRT_PM_WOL_PATTERN_INFO	pWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);

	u32 CamCmd = 0;
	u32 CamContent = 0;
	u8 Addr_i=0;

	RT_TRACE( COMP_CMD, "===>WKFMCAMAddOneEntry(): usConfig=0x%02X\n", usConfig );
	
	RT_TRACE(COMP_CMD, "The mask index is %d\n", Index);

	for(Addr_i=0; Addr_i<CAM_CONTENT_COUNT; Addr_i++)
	{
		CamCmd= Addr_i+CAM_CONTENT_COUNT*Index;
		CamCmd= CamCmd |BIT31|BIT16;

		if(Addr_i < 4) 
		{
			CamContent = pWoLPatternInfo[Index].Mask[Addr_i];

			write_nic_dword(dev, REG_WKFMCAM_RWD, CamContent);  
			RT_TRACE(COMP_CMD, "WKFMCAMAddOneEntry(): WRITE %x: %x \n", REG_WKFMCAM_RWD, CamContent);
			write_nic_dword(dev, REG_WKFMCAM_CMD, CamCmd);  
			RT_TRACE(COMP_CMD, "WKFMCAMAddOneEntry(): WRITE %x: %x \n", REG_WKFMCAM_CMD, CamCmd);
		}
		else if(Addr_i == 4)
		{
			CamContent = BIT31 | pWoLPatternInfo[Index].CrcRemainder;

			write_nic_dword(dev, REG_WKFMCAM_RWD, CamContent);  
			RT_TRACE(COMP_CMD, "WKFMCAMAddOneEntry(): WRITE %x: %x \n", REG_WKFMCAM_RWD, CamContent);
			write_nic_dword(dev, REG_WKFMCAM_CMD, CamCmd);  
			RT_TRACE(COMP_CMD, "WKFMCAMAddOneEntry(): WRITE %x: %x \n", REG_WKFMCAM_CMD, CamCmd);
		}

	}
	
}

void
WKFMCAMDeleteOneEntry(
		struct net_device 	*dev,
		u32 			Index
)
{
	u32	Content = 0;
	u32	Command = 0;
	u8	i;
												
	for(i=0; i<8; i++)
	{
		Command = Index*CAM_CONTENT_COUNT + i;
		Command= Command | BIT31|BIT16;
	
		write_nic_dword(dev, REG_WKFMCAM_RWD, Content);
		RT_TRACE(COMP_CMD, "WKFMCAMDeleteOneEntry(): WRITE %x: %x \n", REG_WKFMCAM_RWD, Content);
		write_nic_dword(dev, REG_WKFMCAM_CMD, Command);
		RT_TRACE(COMP_CMD, "WKFMCAMDeleteOneEntry(): WRITE %x: %x \n", REG_WKFMCAM_CMD, Command);			
	}
}

void
SetBcnCtrlReg(
	struct net_device* dev,
	u8		SetBits,
	u8		ClearBits
	)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);

	priv->RegBcnCtrlVal |= SetBits;
	priv->RegBcnCtrlVal &= ~ClearBits;

	write_nic_byte(dev, REG_BCN_CTRL, (u8)priv->RegBcnCtrlVal);
}
