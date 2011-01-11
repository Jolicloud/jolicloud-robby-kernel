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

#define _RTL8192C_D_HAL_INIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_byteorder.h>

#include <hal_init.h>
#include <rtl8712_efuse.h>

#ifdef CONFIG_SDIO_HCI
			#include <sdio_hal.h>
#ifdef PLATFORM_LINUX
	#include <linux/mmc/sdio_func.h>
#endif
#elif defined(CONFIG_USB_HCI)
			#include <usb_hal.h>
#endif	


static VOID
_FWDownloadEnable(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			enable
	)
{
	u8	tmp;
	if(enable)
	{
		// 8051 with wrapper enable
		tmp = rtw_read8(Adapter, REG_SYS_FUNC_EN+1);
		rtw_write8(Adapter, REG_SYS_FUNC_EN+1, tmp|0x04);

		// MCU firmware download enable.
		tmp = rtw_read8(Adapter, REG_MCUFWDL);
		rtw_write8(Adapter, REG_MCUFWDL, tmp|0x01);

		// 8051 enable
		tmp = rtw_read8(Adapter, REG_MCUFWDL+2);
		rtw_write8(Adapter, REG_MCUFWDL+2, tmp&0xf7);
	}
	else
	{
		// MCU firmware download enable.
		tmp = rtw_read8(Adapter, REG_MCUFWDL);
		rtw_write8(Adapter, REG_MCUFWDL, tmp&0xfe);

		// Reserved for fw extension.
		rtw_write8(Adapter, REG_MCUFWDL+1, 0x00);
	}

}


#define MAX_REG_BOLCK_SIZE	196
#define MIN_REG_BOLCK_SIZE	4


static VOID
_BlockWrite(
	IN		PADAPTER		Adapter,
	IN		PVOID		buffer,
	IN		u32			size
	)
{
 #if 1
  
#ifdef SUPPORTED_BLOCK_IO
	u32			blockSize		= MAX_REG_BOLCK_SIZE;	// Use 196-byte write to download FW	
	u32			blockSize2  	= MIN_REG_BOLCK_SIZE;	
#else
	u32			blockSize		= sizeof(u32);	// Use 4-byte write to download FW
	u32*			pu4BytePtr	= (u32*)buffer;
	u32			blockSize2  	= sizeof(u8);
#endif
	u8*			bufferPtr	= (u8*)buffer;
	u32			i, offset, offset2, blockCount, remainSize, remainSize2;

	blockCount = size / blockSize;
	remainSize = size % blockSize;

	for(i = 0 ; i < blockCount ; i++){
		offset = i * blockSize;
		#ifdef SUPPORTED_BLOCK_IO
		writeN(Adapter, (FW_8192C_START_ADDRESS + offset), blockSize, (bufferPtr + offset));
		#else
		rtw_write32(Adapter, (FW_8192C_START_ADDRESS + offset), le32_to_cpu(*(pu4BytePtr + i)));
		#endif
	}

	if(remainSize){
		offset2 = blockCount * blockSize;		
		blockCount = remainSize / blockSize2;
		remainSize2 = remainSize % blockSize2;

		for(i = 0 ; i < blockCount ; i++){
			offset = offset2 + i * blockSize2;
			#ifdef SUPPORTED_BLOCK_IO
			writeN(Adapter, (FW_8192C_START_ADDRESS + offset), blockSize2, (bufferPtr + offset));
			#else
			rtw_write8(Adapter, (FW_8192C_START_ADDRESS + offset ), *(bufferPtr + offset));
			#endif
		}		

		if(remainSize2)
		{
			offset += blockSize2;
			bufferPtr += offset;
			
			for(i = 0 ; i < remainSize2 ; i++){
				rtw_write8(Adapter, (FW_8192C_START_ADDRESS + offset + i), *(bufferPtr + i));
			}
		}
	}

#else
	u32			blockSize	= sizeof(u32);	// Use 4-byte write to download FW
	u8*			bufferPtr	= (u8*)buffer;
	u32*			pu4BytePtr	= (u32*)buffer;
	u32			i, offset, blockCount, remainSize;

	blockCount = size / blockSize;
	remainSize = size % blockSize;

	for(i = 0 ; i < blockCount ; i++){
		offset = i * blockSize;
		rtw_write32(Adapter, (FW_8192C_START_ADDRESS + offset), le32_to_cpu(*(pu4BytePtr + i)));
	}

	if(remainSize){
		offset = blockCount * blockSize;
		bufferPtr += offset;
		
		for(i = 0 ; i < remainSize ; i++){
			rtw_write8(Adapter, (FW_8192C_START_ADDRESS + offset + i), *(bufferPtr + i));
		}
	}
#endif
}

static VOID
_PageWrite(
	IN		PADAPTER		Adapter,
	IN		u32			page,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	u8 value8;
	u8 u8Page = (u8) (page & 0x07) ;

	value8 = rtw_read8(Adapter, REG_MCUFWDL+2);
	//printk("%s,REG_%02x(0x%02x),wvalue(0x%02x)\n",__FUNCTION__,(REG_MCUFWDL+2),value8,(value8 & 0xF8) |u8Page);

	value8 = (value8 & 0xF8) |u8Page;
	 
	rtw_write8(Adapter, REG_MCUFWDL+2,value8);
	_BlockWrite(Adapter,buffer,size);
}

static VOID
_WriteFW(
	IN		PADAPTER		Adapter,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	// Since we need dynamic decide method of dwonload fw, so we call this function to get chip version.
	// We can remove _ReadChipVersion from ReadAdapterInfo8192C later.

	BOOLEAN			isNormalChip;	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	
	isNormalChip = IS_NORMAL_CHIP(pHalData->VersionID);

	if(isNormalChip){
		u32 	pageNums,remainSize ;
		u32 	page,offset;
		u8*	bufferPtr = (u8*)buffer;
		
		pageNums = size / MAX_PAGE_SIZE ;		
		//RT_ASSERT((pageNums <= 4), ("Page numbers should not greater then 4 \n"));			
		remainSize = size % MAX_PAGE_SIZE;		
		
		for(page = 0; page < pageNums;  page++){
			offset = page *MAX_PAGE_SIZE;
			_PageWrite(Adapter,page, (bufferPtr+offset),MAX_PAGE_SIZE);			
		}
		if(remainSize){
			offset = pageNums *MAX_PAGE_SIZE;
			page = pageNums;
			_PageWrite(Adapter,page, (bufferPtr+offset),remainSize);
		}	
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("_WriteFW Done- for Normal chip.\n"));
	}
	else	{
		_BlockWrite(Adapter,buffer,size);
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("_WriteFW Done- for Test chip.\n"));
	}
}

static int _FWFreeToGo(
	IN		PADAPTER		Adapter
	)
{
	u32			counter = 0;
	u32			value32;
	
	// polling CheckSum report
	do{
		value32 = rtw_read32(Adapter, REG_MCUFWDL);
	}while((counter ++ < POLLING_READY_TIMEOUT_COUNT) && (!(value32 & FWDL_ChkSum_rpt)));	

	if(counter >= POLLING_READY_TIMEOUT_COUNT){	
		DBG_8192C("chksum report faill ! REG_MCUFWDL:0x%08x .\n",value32);		
		return _FAIL;
	}
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Checksum report OK ! REG_MCUFWDL:0x%08x .\n",value32));


	value32 = rtw_read32(Adapter, REG_MCUFWDL);
	value32 |= MCUFWDL_RDY;
	value32 &= ~WINTINI_RDY;
	rtw_write32(Adapter, REG_MCUFWDL, value32);
	
	// polling for FW ready
	counter = 0;
	do
	{
		if(rtw_read32(Adapter, REG_MCUFWDL) & WINTINI_RDY){
			//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Polling FW ready success!! REG_MCUFWDL:0x%08x .\n",PlatformIORead4Byte(Adapter, REG_MCUFWDL)) );
			return _SUCCESS;
		}
		rtw_mdelay_os(5);
	}while(counter++ < POLLING_READY_TIMEOUT_COUNT);

	DBG_8192C("Polling FW ready fail!! REG_MCUFWDL:0x%08x .\n",rtw_read32(Adapter, REG_MCUFWDL));
	return _FAIL;
	
}


static void  _FirmwareSelfReset(PADAPTER Adapter)
{
	u8	u1bTmp;
	u8	Delay = 100;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
		
	
	if((pHalData->FirmwareVersion > 0x21) ||
		(pHalData->FirmwareVersion == 0x21 &&
		pHalData->FirmwareSubVersion >= 0x01))
	{
/*
	printk("==> %s REG 02(0x%04x),80(0x%08x),130(0x%08x),134(0x%08x),138(0x%08x),13c(0x%08x),\
		1c0(0x%08x),1c4(0x%08x),1c8(0x%08x),1cc(0x%08x),\n",
		__FUNCTION__,rtw_read16(Adapter,0x02),rtw_read32(Adapter,0x80),
		rtw_read32(Adapter,0x130),
		rtw_read32(Adapter,0x134),
		rtw_read32(Adapter,0x138),
		rtw_read32(Adapter,0x13c),
		rtw_read32(Adapter,0x1c0),
		rtw_read32(Adapter,0x1c4),
		rtw_read32(Adapter,0x1c8),
		rtw_read32(Adapter,0x1cc));
*/		
		//0x1cf=0x20. Inform 8051 to reset. 2009.12.25. tynli_test
		rtw_write8(Adapter, REG_HMETFR+3, 0x20);
	
		u1bTmp = rtw_read8(Adapter, REG_SYS_FUNC_EN+1);
		while(u1bTmp&BIT2)
		{
			Delay--;
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("PowerOffAdapter8192CE(): polling 0x03[2] Delay = %d \n", Delay));
			if(Delay == 0)
				break;
			//delay_us(50);
			rtw_udelay_os(50);
			u1bTmp = rtw_read8(Adapter, REG_SYS_FUNC_EN+1);
		}
	
		if((u1bTmp&BIT2) && (Delay == 0))
		{
			DBG_8192C("FirmwareDownload92C():fw reset by itself Fail!!!!!! 0x03 = %x\n", u1bTmp);
			//RT_ASSERT(FALSE, ("PowerOffAdapter8192CE(): 0x03 = %x\n", u1bTmp));
			rtw_write8(Adapter,REG_SYS_FUNC_EN+1,(rtw_read8(Adapter, REG_SYS_FUNC_EN+1)&~BIT2));
		}
/*
		printk("==> %s REG 02(0x%04x),80(0x%08x),130(0x%08x),134(0x%08x),138(0x%08x),13c(0x%08x),\
		1c0(0x%08x),1c4(0x%08x),1c8(0x%08x),1cc(0x%08x),\n",
		__FUNCTION__,rtw_read16(Adapter,0x02),rtw_read32(Adapter,0x80),
		rtw_read32(Adapter,0x130),
		rtw_read32(Adapter,0x134),
		rtw_read32(Adapter,0x138),
		rtw_read32(Adapter,0x13c),
		rtw_read32(Adapter,0x1c0),
		rtw_read32(Adapter,0x1c4),
		rtw_read32(Adapter,0x1c8),
		rtw_read32(Adapter,0x1cc));
*/		
	}
}

//
//	Description:
//		Download 8192C firmware code.
//
//
int FirmwareDownload92C(
	IN	PADAPTER			Adapter
)
{	
	int	rtStatus = _SUCCESS;	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	char 			R92CFwImageFileName_TSMC[] ={RTL8192C_FW_TSMC_IMG};
	char 			R92CFwImageFileName_UMC[] ={RTL8192C_FW_UMC_IMG};
	char 			R8723FwImageFileName_UMC[] ={RTL8723_FW_UMC_IMG};

	char *			FwImage;
	u32				FwImageLen;	
	
	char*			pFwImageFileName;	

	//vivi, merge 92c and 92s into one driver, 20090817
	//vivi modify this temply, consider it later!!!!!!!!
	//PRT_FIRMWARE	pFirmware = GET_FIRMWARE_819X(Adapter);	
	//PRT_FIRMWARE_92C	pFirmware = GET_FIRMWARE_8192C(Adapter);
	PRT_FIRMWARE_92C	pFirmware = NULL;
	PRT_8192C_FIRMWARE_HDR		pFwHdr = NULL;
	u8		*pFirmwareBuf;
	u32		FirmwareLen;

	pFirmware = (PRT_FIRMWARE_92C)_rtw_malloc(sizeof(RT_FIRMWARE_92C));
	if(!pFirmware)
	{
		rtStatus = _FAIL;
		goto Exit;
	}
	
	

	//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> FirmwareDownload91C() fw:%s\n", pFwImageFileName));

#ifdef CONFIG_EMBEDDED_FWIMG
	pFirmware->eFWSource = FW_SOURCE_HEADER_FILE;
#else
	pFirmware->eFWSource = FW_SOURCE_IMG_FILE; 
#endif
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		if(IS_VENDOR_UMC_A_CUT(pHalData->VersionID) && !IS_92C_SERIAL(pHalData->VersionID))// UMC , 8188
		{						
			pFwImageFileName = R92CFwImageFileName_UMC;
			FwImage = Rtl819XFwUMCImageArray;
			FwImageLen = UMCImgArrayLength;
			DBG_871X(" ===> FirmwareDownload91C() fw:Rtl819XFwImageArray_UMC\n");
				
		}
		else
		{
			pFwImageFileName = R92CFwImageFileName_TSMC;
			FwImage = Rtl819XFwTSMCImageArray;
			FwImageLen = TSMCImgArrayLength;
			DBG_871X(" ===> FirmwareDownload91C() fw:Rtl819XFwImageArray_TSMC\n");
		}
	}
	else
	{
	#if 0
		pFwImageFileName = TestChipFwFile;
		FwImage = Rtl8192CTestFwImg;
		FwImageLen = Rtl8192CTestFwImgLen;
		RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> FirmwareDownload91C() fw:Rtl8192CTestFwImg\n"));
	#endif
	}
		

	switch(pFirmware->eFWSource)
	{
		case FW_SOURCE_IMG_FILE:
			//TODO:load fw bin file
			break;
		case FW_SOURCE_HEADER_FILE:
			if(TSMCImgArrayLength > FW_8192C_SIZE){
				rtStatus = _FAIL;
				//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Firmware size exceed 0x%X. Check it.\n", FW_8192C_SIZE) );
				DBG_871X("Firmware size exceed 0x%X. Check it.\n", FW_8192C_SIZE);
				goto Exit;
			}
			_rtw_memcpy(pFirmware->szFwBuffer, FwImage, FwImageLen);
			pFirmware->ulFwLength = FwImageLen;
			break;
	}

	pFirmwareBuf = pFirmware->szFwBuffer;
	FirmwareLen = pFirmware->ulFwLength;

	// To Check Fw header. 
	pFwHdr = (PRT_8192C_FIRMWARE_HDR)pFirmware->szFwBuffer;

	pHalData->FirmwareVersion =  le16_to_cpu(pFwHdr->Version); 
	pHalData->FirmwareSubVersion = le16_to_cpu(pFwHdr->Subversion); 

	//RT_TRACE(COMP_INIT, DBG_LOUD, (" FirmwareVersion(%#x), Signature(%#x)\n", 
	//	Adapter->MgntInfo.FirmwareVersion, pFwHdr->Signature));

	DBG_8192C("fw_ver=v%d, fw_subver=%d, sig=0x%x\n", 
		pHalData->FirmwareVersion, pHalData->FirmwareSubVersion, le16_to_cpu(pFwHdr->Signature)&0xFFF0);

	if(IS_FW_HEADER_EXIST(pFwHdr))
	{
		//RT_TRACE(COMP_INIT, DBG_LOUD,("Shift 32 bytes for FW header!!\n"));
		pFirmwareBuf = pFirmwareBuf + 32;
		FirmwareLen = FirmwareLen -32;
	}
		
	// Suggested by Filen. If 8051 is running in RAM code, driver should inform Fw to reset by itself,
	// or it will cause download Fw fail. 2010.02.01. by tynli.
	if(rtw_read8(Adapter, REG_MCUFWDL)&BIT7) //8051 RAM code
	{	
		DBG_8192C("8051 in Ram......prepare to reset by itself\n");
		_FirmwareSelfReset(Adapter);
		rtw_write8(Adapter, REG_MCUFWDL, 0x00);		
	}

		
	_FWDownloadEnable(Adapter, _TRUE);
	_WriteFW(Adapter, pFirmwareBuf, FirmwareLen);
	_FWDownloadEnable(Adapter, _FALSE);

	rtStatus = _FWFreeToGo(Adapter);
	if(_SUCCESS != rtStatus){
		//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("DL Firmware failed!\n") );	
		goto Exit;
	}	
	//RT_TRACE(COMP_INIT, DBG_LOUD, (" Firmware is ready to run!\n"));

Exit:

	if(pFirmware)
		_rtw_mfree((u8*)pFirmware, sizeof(RT_FIRMWARE_92C));

	//RT_TRACE(COMP_INIT, DBG_LOUD, (" <=== FirmwareDownload91C()\n"));
	return rtStatus;

}


//-------------------------------------------------------------------------
//
//	Channel Plan
//
//-------------------------------------------------------------------------

RT_CHANNEL_DOMAIN
_HalMapChannelPlan8192C(
	IN	PADAPTER	Adapter,
	IN	u8		HalChannelPlan
	)
{
	RT_CHANNEL_DOMAIN	rtChannelDomain;

	switch(HalChannelPlan)
	{
		case EEPROM_CHANNEL_PLAN_GLOBAL_DOMAIN:
			rtChannelDomain = RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN;
			break;
		case EEPROM_CHANNEL_PLAN_WORLD_WIDE_13:
			rtChannelDomain = RT_CHANNEL_DOMAIN_WORLD_WIDE_13;
			break;			
		default:
			rtChannelDomain = (RT_CHANNEL_DOMAIN)HalChannelPlan;
			break;
	}
	
	return 	rtChannelDomain;

}

static VOID
ReadChannelPlan(
	IN	PADAPTER 		Adapter,
	IN	u8*			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	)
{

#define EEPROM_TEST_CHANNEL_PLAN	 (0x7D)
#define EEPROM_NORMAL_CHANNEL_PLAN (0x75)

	struct mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u8			channelPlan;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(AutoLoadFail){
		channelPlan = CHPL_FCC;
	}
	else{
#if (DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE)		
		if(IS_NORMAL_CHIP(pHalData->VersionID))
		 	channelPlan = PROMContent[EEPROM_NORMAL_CHANNEL_PLAN];
		else
			channelPlan = PROMContent[EEPROM_TEST_CHANNEL_PLAN];
#else
		 channelPlan = PROMContent[EEPROM_CHANNEL_PLAN];
#endif
	}

	if((pregistrypriv->channel_plan>= RT_CHANNEL_DOMAIN_MAX) || (channelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK))
	{
		pmlmepriv->ChannelPlan = _HalMapChannelPlan8192C(Adapter, (channelPlan & (~(EEPROM_CHANNEL_PLAN_BY_HW_MASK))));
		//pMgntInfo->bChnlPlanFromHW = (channelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK) ? _TRUE : _FALSE; // User cannot change  channel plan.
	}
	else
	{
		pmlmepriv->ChannelPlan = (RT_CHANNEL_DOMAIN)pregistrypriv->channel_plan;
	}

#if 0 //todo:
	switch(pMgntInfo->ChannelPlan)
	{
		case RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN:
		{
			PRT_DOT11D_INFO	pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);
	
			pDot11dInfo->bEnabled = _TRUE;
		}
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n"));
		break;
	}
#endif

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("RegChannelPlan(%d) EEPROMChannelPlan(%ld)", pMgntInfo->RegChannelPlan, (u4Byte)channelPlan));
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("ChannelPlan = %d\n" , pMgntInfo->ChannelPlan));

	MSG_8192C("RT_ChannelPlan: 0x%02x\n", pmlmepriv->ChannelPlan);

}


//-------------------------------------------------------------------------
//
//	EEPROM Power index mapping
//
//-------------------------------------------------------------------------

 static VOID
_ReadPowerValueFromPROM(
	IN	PTxPowerInfo	pwrInfo,
	IN	u8*			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	)
{
	u32 rfPath, eeAddr, group;

	_rtw_memset(pwrInfo, 0, sizeof(TxPowerInfo));

	if(AutoLoadFail){		
		for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
			for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
				pwrInfo->CCKIndex[rfPath][group]		= EEPROM_Default_TxPowerLevel;	
				pwrInfo->HT40_1SIndex[rfPath][group]	= EEPROM_Default_TxPowerLevel;
				pwrInfo->HT40_2SIndexDiff[rfPath][group]= EEPROM_Default_HT40_2SDiff;
				pwrInfo->HT20IndexDiff[rfPath][group]	= EEPROM_Default_HT20_Diff;
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


static u32
_GetChannelGroup(
	IN	u32	channel
	)
{
	//RT_ASSERT((channel < 14), ("Channel %d no is supported!\n"));

	if(channel < 3){ 	// Channel 1~3
		return 0;
	}
	else if(channel < 9){ // Channel 4~9
		return 1;
	}

	return 2;				// Channel 10~14	
}


static VOID
ReadTxPowerInfo(
	IN	PADAPTER 		Adapter,
	IN	u8*			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	)
{	
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	TxPowerInfo		pwrInfo;
	u32			rfPath, ch, group;
	u8			pwr, diff;

	_ReadPowerValueFromPROM(&pwrInfo, PROMContent, AutoLoadFail);

	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
			group = _GetChannelGroup(ch);

			pEEPROM->TxPwrLevelCck[rfPath][ch]		= pwrInfo.CCKIndex[rfPath][group];
			pEEPROM->TxPwrLevelHT40_1S[rfPath][ch]	= pwrInfo.HT40_1SIndex[rfPath][group];

			pEEPROM->TxPwrHt20Diff[rfPath][ch]		= pwrInfo.HT20IndexDiff[rfPath][group];
			pEEPROM->TxPwrLegacyHtDiff[rfPath][ch]	= pwrInfo.OFDMIndexDiff[rfPath][group];
			pEEPROM->PwrGroupHT20[rfPath][ch]		= pwrInfo.HT20MaxOffset[rfPath][group];
			pEEPROM->PwrGroupHT40[rfPath][ch]		= pwrInfo.HT40MaxOffset[rfPath][group];

			pwr		= pwrInfo.HT40_1SIndex[rfPath][group];
			diff	= pwrInfo.HT40_2SIndexDiff[rfPath][group];

			pEEPROM->TxPwrLevelHT40_2S[rfPath][ch]  = (pwr > diff) ? (pwr - diff) : 0;
		}
	}

	if(AutoLoadFail)
	{
		pEEPROM->EEPROMRegulatory= 0;	
	}
	else
	{
		pEEPROM->EEPROMRegulatory = (PROMContent[EEPROM_RF_OPT1]&0x7);	//bit0~2
		
	}
	
#if DBG

	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
			RTPRINT(FINIT, INIT_TxPower, 
				("RF(%d)-Ch(%d) [CCK / HT40_1S / HT40_2S] = [0x%x / 0x%x / 0x%x]\n", 
				rfPath, ch, pHalData->TxPwrLevelCck[rfPath][ch], 
				pHalData->TxPwrLevelHT40_1S[rfPath][ch], 
				pHalData->TxPwrLevelHT40_2S[rfPath][ch]));

		}
	}

	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Ht20 to HT40 Diff[%d] = 0x%x\n", ch, pHalData->TxPwrHt20Diff[RF90_PATH_A][ch]));
	}

	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Legacy to Ht40 Diff[%d] = 0x%x\n", ch, pHalData->TxPwrLegacyHtDiff[RF90_PATH_A][ch]));
	}
	
	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Ht20 to HT40 Diff[%d] = 0x%x\n", ch, pHalData->TxPwrHt20Diff[RF90_PATH_B][ch]));
	}
	
	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Legacy to HT40 Diff[%d] = 0x%x\n", ch, pHalData->TxPwrLegacyHtDiff[RF90_PATH_B][ch]));
	}
	
#endif

}


//-------------------------------------------------------------------
//
//	EEPROM/EFUSE Content Parsing
//
//-------------------------------------------------------------------
static VERSION_8192C
ReadChipVersion(
	IN	PADAPTER	Adapter
	)
{
	u32				value32;
	VERSION_8192C	version;
	u8				ChipVersion=0;	
	
	value32 = rtw_read32(Adapter, REG_SYS_CFG);
#if 0
	if(value32 & TRP_VAUX_EN){		
		//Test chip
		switch(((value32 & CHIP_VER_RTL_MASK) >> CHIP_VER_RTL_SHIFT))
		{
			case 0: //8191C
				version = VERSION_TEST_CHIP_91C;
				break;
			case 1: //8188C
				version = VERSION_TEST_CHIP_88C;
				break;
			default:
				// TODO: set default to 1T1R?
				RT_ASSERT(FALSE,("Chip Version can't be recognized.\n"));
				break;
		}
		
	}
	else{		
		//Normal chip
		version = VERSION_8192C_NORMAL_CHIP;

	}
#else
	if (value32 & TRP_VAUX_EN){		
		version = (value32 & TYPE_ID) ?VERSION_TEST_CHIP_92C :VERSION_TEST_CHIP_88C;		
	}
	else{
		// Normal mass production chip.
		ChipVersion = NORMAL_CHIP;
		ChipVersion |= ((value32 & TYPE_ID) ? CHIP_92C : 0);
		ChipVersion |= ((value32 & VENDOR_ID) ? CHIP_VENDOR_UMC : 0);
		ChipVersion |= ((value32 & BT_FUNC) ? CHIP_8723: 0); // RTL8723 with BT function.
		if(IS_VENDOR_UMC(ChipVersion))
			ChipVersion |= ((value32 & CHIP_VER_RTL_MASK) ? CHIP_VENDOR_UMC_B_CUT : 0);

		if(IS_92C_SERIAL(ChipVersion))
		{
			value32 = rtw_read32(Adapter, REG_HPON_FSM);
			ChipVersion |= ((CHIP_BONDING_IDENTIFIER(value32) == CHIP_BONDING_92C_1T2R) ? CHIP_92C_1T2R : 0);			
		}
		else if(IS_8723_SERIES(ChipVersion))
		{
			value32 = rtw_read32(Adapter, REG_GPIO_OUTSTS);
			ChipVersion |= ((value32 & RF_RL_ID) ? CHIP_8723_DRV_REV : 0);			
		}
		version = (VERSION_8192C)ChipVersion;
	}
#endif

	switch(version)
	{
		case VERSION_NORMAL_TSMC_CHIP_92C_1T2R:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_TSMC_CHIP_92C_1T2R.\n");
			break;
		case VERSION_NORMAL_TSMC_CHIP_92C:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_TSMC_CHIP_92C.\n");
			break;
		case VERSION_NORMAL_TSMC_CHIP_88C:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_TSMC_CHIP_88C.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_92C_1T2R_A_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_92C_1T2R_A_CUT.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_92C_A_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_92C_A_CUT.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_88C_A_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_88C_A_CUT.\n");
			break;			
		case VERSION_NORMAL_UMC_CHIP_92C_1T2R_B_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_92C_1T2R_B_CUT.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_92C_B_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_92C_B_CUT.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_88C_B_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_88C_B_CUT.\n");
			break;
		case VERSION_TEST_CHIP_92C:
			MSG_8192C("Chip Version ID: VERSION_TEST_CHIP_92C.\n");
			break;
		case VERSION_TEST_CHIP_88C:
			MSG_8192C("Chip Version ID: VERSION_TEST_CHIP_88C.\n");
			break;
		case VERSION_NORMA_UMC_CHIP_8723_1T1R_A_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMA_UMC_CHIP_8723_1T1R_A_CUT.\n");
			break;
		case VERSION_NORMA_UMC_CHIP_8723_1T1R_B_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMA_UMC_CHIP_8723_1T1R_B_CUT.\n");
			break;
		default:
			MSG_8192C("Chip Version ID: ???????????????.\n");
			break;
	}


	return version;
}

static void
_ReadIDs(
	IN	PADAPTER	Adapter,
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	//PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(_FALSE == AutoloadFail){
		// VID, PID 
		#if 1 //for Funai BD's alignment error		
		u16 tmp = 0;		
		_rtw_memcpy( &tmp , (void*) &PROMContent[EEPROM_VID], 2 );		
		tmp = le16_to_cpu( tmp );		
		pEEPROM->EEPROMVID = tmp;		
		tmp = 0;		
		_rtw_memcpy( &tmp , (void*) &PROMContent[EEPROM_PID], 2 );		
		tmp = le16_to_cpu( tmp );		
		pEEPROM->EEPROMPID = tmp;	
		#else
		pEEPROM->EEPROMVID = le16_to_cpu( *(u16 *)&PROMContent[EEPROM_VID]);
		pEEPROM->EEPROMPID = le16_to_cpu( *(u16 *)&PROMContent[EEPROM_PID]);		
		#endif
		
		// Customer ID, 0x00 and 0xff are reserved for Realtek. 		
		pEEPROM->EEPROMCustomerID = *(u8 *)&PROMContent[EEPROM_CUSTOMER_ID];
		pEEPROM->EEPROMSubCustomerID = *(u8 *)&PROMContent[EEPROM_SUBCUSTOMER_ID];

	}
	else{
		pEEPROM->EEPROMVID	 = EEPROM_Default_VID;
		pEEPROM->EEPROMPID	 = EEPROM_Default_PID;

		// Customer ID, 0x00 and 0xff are reserved for Realtek. 		
		pEEPROM->EEPROMCustomerID	= EEPROM_Default_CustomerID;
		pEEPROM->EEPROMSubCustomerID = EEPROM_Default_SubCustomerID;

	}


	switch(pEEPROM->EEPROMCustomerID)
	{
		case EEPROM_CID_ALPHA:
				pHalData->CustomerID = RT_CID_819x_ALPHA;
				break;
				
		case EEPROM_CID_CAMEO:
				pHalData->CustomerID = RT_CID_819x_CAMEO;
				break;			
					
		case EEPROM_CID_SITECOM:
				pHalData->CustomerID = RT_CID_819x_Sitecom;
				break;	
					
		case EEPROM_CID_COREGA:
				pHalData->CustomerID = RT_CID_COREGA;						
				break;			
			
		case EEPROM_CID_Senao:
				pHalData->CustomerID = RT_CID_819x_Senao;
				break;
		
		case EEPROM_CID_EDIMAX_BELKIN:
				pHalData->CustomerID = RT_CID_819x_Edimax_Belkin;
				break;
		
		case EEPROM_CID_SERCOMM_BELKIN:
				pHalData->CustomerID = RT_CID_819x_Sercomm_Belkin;
				break;
					
		case EEPROM_CID_WNC_COREGA:
				pHalData->CustomerID = RT_CID_819x_WNC_COREGA;
				break;
		
		case EEPROM_CID_WHQL:
/*			
			Adapter->bInHctTest = TRUE;

			pMgntInfo->bSupportTurboMode = FALSE;
			pMgntInfo->bAutoTurboBy8186 = FALSE;

			pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
			pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
			pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
				
			pMgntInfo->keepAliveLevel = 0;

			Adapter->bUnloadDriverwhenS3S4 = FALSE;
*/				
				break;
					
		case EEPROM_CID_NetCore:
				pHalData->CustomerID = RT_CID_819x_Netcore;
				break;
		
		case EEPROM_CID_CAMEO1:
				pHalData->CustomerID = RT_CID_819x_CAMEO1;
				break;
					
		case EEPROM_CID_CLEVO:
				pHalData->CustomerID = RT_CID_819x_CLEVO;
			break;			
		
		default:
				pHalData->CustomerID = RT_CID_DEFAULT;
			break;
			
	}

	// For customized behavior.
	if((pEEPROM->EEPROMVID == 0x103C) && (pEEPROM->EEPROMPID == 0x1629))// HP Lite-On for RTL8188CUS Slim Combo.
		pHalData->CustomerID = RT_CID_819x_HP;	

	MSG_8192C("EEPROMVID = 0x%04x\n", pEEPROM->EEPROMVID);
	MSG_8192C("EEPROMPID = 0x%04x\n", pEEPROM->EEPROMPID);
	MSG_8192C("EEPROMCustomerID : 0x%02x\n", pEEPROM->EEPROMCustomerID);
	MSG_8192C("EEPROMSubCustomerID: 0x%02x\n", pEEPROM->EEPROMSubCustomerID);

	MSG_8192C("RT_CustomerID: 0x%02x\n", pHalData->CustomerID);

}


static VOID
_ReadMACAddress(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);

	if(_FALSE == AutoloadFail){
		//Read Permanent MAC address and set value to hardware
		_rtw_memcpy(pEEPROM->mac_addr, &PROMContent[EEPROM_MAC_ADDR], ETH_ALEN);		
	}
	else{
		//Random assigh MAC address
		u8 sMacAddr[MAC_ADDR_LEN] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
		//sMacAddr[5] = (u8)GetRandomNumber(1, 254);		
		_rtw_memcpy(pEEPROM->mac_addr, sMacAddr, ETH_ALEN);	
	}
	
	//NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);
	//RT_PRINT_ADDR(COMP_INIT|COMP_EFUSE, DBG_LOUD, "MAC Addr: %s", Adapter->PermanentAddress);

}
#ifdef CONFIG_BT_COEXIST
static void _update_bt_param(_adapter *padapter)
{
	struct btcoexist_priv	 *pbtpriv = &(padapter->halpriv.bt_coexist);
	struct registry_priv  	*registry_par = &padapter->registrypriv;
	if(2 != registry_par->bt_iso)
		pbtpriv->BT_Ant_isolation = registry_par->bt_iso;// 0:Low, 1:High, 2:From Efuse


	if(registry_par->bt_sco == 1) // 0:Idle, 1:None-SCO, 2:SCO, 3:From Counter, 4.Busy, 5.OtherBusy
		pbtpriv->BT_Service = BT_OtherAction;
	else if(registry_par->bt_sco==2)
		pbtpriv->BT_Service = BT_SCO;
	else if(registry_par->bt_sco==4)
		pbtpriv->BT_Service = BT_Busy;
	else if(registry_par->bt_sco==5)
		pbtpriv->BT_Service = BT_OtherBusy;		
	else
		pbtpriv->BT_Service = BT_Idle;

	pbtpriv->BT_Ampdu = registry_par->bt_ampdu;
	pbtpriv->bCOBT = _TRUE;

	pbtpriv->BtEdcaUL = 0;
	pbtpriv->BtEdcaDL = 0;
	pbtpriv->BtRssiState = 0xff;

	pbtpriv->bInitSet = _FALSE;
	pbtpriv->bBTBusyTraffic = _FALSE;
	pbtpriv->bBTTrafficModeSet = _FALSE;
	pbtpriv->bBTNonTrafficModeSet = _FALSE;
	pbtpriv->CurrentState = 0;
	pbtpriv->PreviousState = 0;
		
	
#if 1
	printk("BT Coexistance = %s\n", (pbtpriv->BT_Coexist==_TRUE)?"enable":"disable");
	if(pbtpriv->BT_Coexist)
	{
		if(pbtpriv->BT_Ant_Num == Ant_x2)
		{
			printk("BlueTooth BT_Ant_Num = Antx2\n");
		}
		else if(pbtpriv->BT_Ant_Num == Ant_x1)
		{
			printk("BlueTooth BT_Ant_Num = Antx1\n");
		}
		switch(pbtpriv->BT_CoexistType)
		{
			case BT_2Wire:
				printk("BlueTooth BT_CoexistType = BT_2Wire\n");
				break;
			case BT_ISSC_3Wire:
				printk("BlueTooth BT_CoexistType = BT_ISSC_3Wire\n");
				break;
			case BT_Accel:
				printk("BlueTooth BT_CoexistType = BT_Accel\n");
				break;
			case BT_CSR_BC4:
				printk("BlueTooth BT_CoexistType = BT_CSR_BC4\n");
				break;
			case BT_CSR_BC8:
				printk("BlueTooth BT_CoexistType = BT_CSR_BC8\n");
				break;			
			case BT_RTL8756:
				printk("BlueTooth BT_CoexistType = BT_RTL8756\n");
				break;
			default:
				printk("BlueTooth BT_CoexistType = Unknown\n");
				break;
		}
		printk("BlueTooth BT_Ant_isolation = %d\n", pbtpriv->BT_Ant_isolation);


		switch(pbtpriv->BT_Service)
		{
			case BT_OtherAction:
				printk("BlueTooth BT_Service = BT_OtherAction\n");
				break;
			case BT_SCO:
				printk("BlueTooth BT_Service = BT_SCO\n");
				break;
			case BT_Busy:
				printk("BlueTooth BT_Service = BT_Busy\n");
				break;
			case BT_OtherBusy:
				printk("BlueTooth BT_Service = BT_OtherBusy\n");
				break;			
			default:
				printk("BlueTooth BT_Service = BT_Idle\n");
				break;
		}

		printk("BT_RadioSharedType = 0x%x\n", pbtpriv->BT_RadioSharedType);
	}
#endif

}


#define GET_BT_COEXIST(priv) (&priv->bt_coexist)

void _ReadBluetoothCoexistInfo(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	BOOLEAN			isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	struct btcoexist_priv	 *pbtpriv = &pHalData->bt_coexist;
	
	u8	rf_opt4;
	_rtw_memset(pbtpriv, 0,sizeof(struct btcoexist_priv));

	if(AutoloadFail){
		pbtpriv->BT_Coexist = _FALSE;
		pbtpriv->BT_CoexistType= BT_2Wire;
		pbtpriv->BT_Ant_Num = Ant_x2;
		pbtpriv->BT_Ant_isolation= 0;
		pbtpriv->BT_RadioSharedType = BT_Radio_Shared;		
		return;
	}

	if(isNormal)
	{
		if(pHalData->BoardType == BOARD_USB_COMBO)
			pbtpriv->BT_Coexist = _TRUE;
		else
			pbtpriv->BT_Coexist = ((PROMContent[EEPROM_RF_OPT3]&0x20)>>5);	//bit[5]
		
		rf_opt4 = PROMContent[EEPROM_RF_OPT4];
		pbtpriv->BT_CoexistType 	= ((rf_opt4&0xe)>>1);				// bit [3:1]
		pbtpriv->BT_Ant_Num 		= (rf_opt4&0x1);					// bit [0]
		pbtpriv->BT_Ant_isolation 	= ((rf_opt4&0x10)>>4);			// bit [4]
		pbtpriv->BT_RadioSharedType 	= ((rf_opt4&0x20)>>5);		// bit [5]
	}
	else
	{
		pbtpriv->BT_Coexist = (PROMContent[EEPROM_RF_OPT4] >> 4) ? _TRUE : _FALSE;	
	}
	_update_bt_param(Adapter);

}
#endif
static VOID
_ReadBoardType(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{	
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	BOOLEAN			isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	struct registry_priv  *registry_par = &Adapter->registrypriv;
	u8			boardType;

	if(AutoloadFail){
		pHalData->rf_type = RF_1T1R;
		pHalData->BluetoothCoexist = _FALSE;
		return;
	}

	if(isNormal) 
	{
		boardType = ((PROMContent[EEPROM_RF_OPT1])&BOARD_TYPE_NORMAL_MASK)>>5 ; //bit[7:5]
	}
	else
	{
		boardType = PROMContent[EEPROM_RF_OPT4];
		boardType &= BOARD_TYPE_TEST_MASK;		
		}

	pHalData->BoardType = boardType;
	printk("_ReadBoardType(%x)\n",pHalData->BoardType);

	
#ifdef CONFIG_ANTENNA_DIVERSITY
	// Antenna Diversity setting. 
	if(registry_par->antdiv_cfg == 2) // 2: From Efuse
		pHalData->AntDivCfg = (PROMContent[EEPROM_RF_OPT1]&0x18)>>3;
	else
		pHalData->AntDivCfg = registry_par->antdiv_cfg ;  // 0:OFF , 1:ON,

	printk("### AntDivCfg(%x)\n",pHalData->AntDivCfg);	
#endif
	

}


static VOID
_ReadLEDSetting(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	struct led_priv *pledpriv = &(Adapter->ledpriv);
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	//
	// Led mode
	//
	switch(pHalData->CustomerID)
	{
		case RT_CID_DEFAULT:
			pledpriv->LedStrategy = SW_LED_MODE1;
			pledpriv->bRegUseLed = _TRUE;			
			break;

		case RT_CID_819x_HP:
			pledpriv->LedStrategy = SW_LED_MODE6; // Customize Led mode	
			pledpriv->bLedOpenDrain = _TRUE;// Support Open-drain arrangement for controlling the LED. Added by Roger, 2009.10.16.
			break;
			
		default:
			pledpriv->LedStrategy = SW_LED_MODE1;
			break;			
	}

	if( BOARD_MINICARD == pHalData->BoardType )
	{
		pledpriv->LedStrategy = SW_LED_MODE6;
	}
}

static VOID
_ReadThermalMeter(
	IN	PADAPTER	Adapter,	
	IN	u8* 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);	
	u8			tempval;

	//
	// ThermalMeter from EEPROM
	//
	if(!AutoloadFail)	
		tempval = PROMContent[EEPROM_THERMAL_METER];
	else
		tempval = EEPROM_Default_ThermalMeter;
	
	pEEPROM->EEPROMThermalMeter = (tempval&0x1f);	//[4:0]

	if(pEEPROM->EEPROMThermalMeter < 0x06 || pEEPROM->EEPROMThermalMeter > 0x1c)
		pEEPROM->EEPROMThermalMeter = 0x12;
	
	//pHalData->ThermalMeter[0] = pEEPROM->EEPROMThermalMeter;//?
	pHalData->ThermalValue = pEEPROM->EEPROMThermalMeter;

//	pHalData->ThermalValue = 0;//set to 0, will be update when do dm_txpower_tracking
	
	//RTPRINT(FINIT, INIT_TxPower, ("ThermalMeter = 0x%x\n", pHalData->EEPROMThermalMeter));
	
}

static VOID
_ReadRFSetting(
	IN	PADAPTER	Adapter,	
	IN	u8* 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	)
{
}

static void
_ReadPROMVersion(
	IN	PADAPTER	Adapter,	
	IN	u8* 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(AutoloadFail){
		pEEPROM->EEPROMVersion = EEPROM_Default_Version;		
	}
	else{
		pEEPROM->EEPROMVersion = *(u8 *)&PROMContent[EEPROM_VERSION];
	}
}

// Read HW power down mode selection 
static void _ReadHWPDSelection(IN PADAPTER Adapter,IN u8*PROMContent,IN	u8	AutoloadFail)
{
	if(AutoloadFail){
		Adapter->pwrctrlpriv.bHWPowerdown = _FALSE;
		Adapter->pwrctrlpriv.bSupportRemoteWakeup = _FALSE;
	}
	else	{
		if(SUPPORT_HW_RADIO_DETECT(Adapter))
			Adapter->pwrctrlpriv.bHWPwrPindetect = Adapter->registrypriv.hwpwrp_detect;
		else
			Adapter->pwrctrlpriv.bHWPwrPindetect = _FALSE;//dongle not support new
			
			
		//hw power down mode selection , 0:rf-off / 1:power down

		if(Adapter->registrypriv.hwpdn_mode==2)
			Adapter->pwrctrlpriv.bHWPowerdown = (PROMContent[EEPROM_RF_OPT3] & BIT4);
		else
			Adapter->pwrctrlpriv.bHWPowerdown = Adapter->registrypriv.hwpdn_mode;
				
		// decide hw if support remote wakeup function
		// if hw supported, 8051 (SIE) will generate WeakUP frame when autoresume
		Adapter->pwrctrlpriv.bSupportRemoteWakeup = (PROMContent[EEPROM_TEST_USB_OPT] & BIT1)?_TRUE :_FALSE;

		//if(SUPPORT_HW_RADIO_DETECT(Adapter))	
			//Adapter->registrypriv.usbss_enable = Adapter->pwrctrlpriv.bSupportRemoteWakeup ;
		
		DBG_8192C("%s...bHWPwrPindetect(%d) bSupportRemoteWakeup(%x)\n",__FUNCTION__,Adapter->pwrctrlpriv.bHWPwrPindetect,Adapter->pwrctrlpriv.bSupportRemoteWakeup);
	}
	
}


static void _InitAdapterVariablesByPROM(IN	PADAPTER Adapter)
{
	u8 *PROMContent = Adapter->eeprompriv.efuse_eeprom_data;
	u8 bAutoload = Adapter->eeprompriv.bAutoload ;
	
	_ReadPROMVersion(Adapter, PROMContent, !bAutoload);
	_ReadIDs(Adapter, PROMContent, !bAutoload);
	_ReadMACAddress(Adapter, PROMContent, !bAutoload);	
	
	ReadTxPowerInfo(Adapter, PROMContent, !bAutoload);
	
	ReadChannelPlan(Adapter, PROMContent, !bAutoload);//todo:	
	_ReadBoardType(Adapter, PROMContent, !bAutoload);//get rf_type !!!
#ifdef CONFIG_BT_COEXIST
	_ReadBluetoothCoexistInfo(Adapter, PROMContent, !bAutoload);
#endif
	
	_ReadThermalMeter(Adapter, PROMContent, !bAutoload);
	_ReadLEDSetting(Adapter, PROMContent, !bAutoload);	
	_ReadRFSetting(Adapter, PROMContent, !bAutoload);
	_ReadHWPDSelection(Adapter, PROMContent, !bAutoload);

	
}

static void efuse_ReadAllMap(
	IN		PADAPTER	pAdapter, 
	IN OUT	u8		*Efuse)
{	
	int i, offset;	

	rtw_efuse_reg_init(pAdapter);
	rtw_efuse_get_max_phy_size(pAdapter);//init
	
	for(i=0, offset=0 ; i<128; i+=8, offset++)
	{		
		rtw_efuse_pg_packet_read(pAdapter, offset, Efuse+i);			
/*
		printk("offset(%d),data:[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:]\n",offset,
			*(Efuse+i),*(Efuse+i+1),*(Efuse+i+2),*(Efuse+i+3),
			*(Efuse+i+4),	*(Efuse+i+5),*(Efuse+i+6),*(Efuse+i+7));
*/
	}
	
	rtw_efuse_reg_uninit(pAdapter);

}// efuse_ReadAllMap
static void EFUSE_ShadowMapUpdate(
	IN		PADAPTER	pAdapter)
{	
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(pAdapter);	
		
	if (pEEPROM->bAutoload == _SUCCESS)
	{			
		efuse_ReadAllMap(pAdapter, pEEPROM->efuse_eeprom_data);		
	}	
	
}// EFUSE_ShadowMapUpdate

static void _ReadPROMContent(
	IN PADAPTER 		Adapter
	)
{	
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	u8			eeValue;
	u32			i;

	_rtw_memset(pEEPROM->efuse_eeprom_data, 0xff, HWSET_MAX_SIZE);	
	
	eeValue = rtw_read8(Adapter, REG_9346CR);
	// To check system boot selection.
	pEEPROM->bBootFromEEPROM	= (eeValue & BOOT_FROM_EEPROM) ? _TRUE : _FALSE;
	pEEPROM->bAutoload			= (eeValue & EEPROM_EN) ? _SUCCESS:_FAIL;


	DBG_8192C("Boot from %s, Autoload %s !\n", (pEEPROM->bBootFromEEPROM ? "EEPROM" : "EFUSE"),
				(pEEPROM->bAutoload ? "Success" : "Fail") );

	//pHalData->EEType = IS_BOOT_FROM_EEPROM(Adapter) ? EEPROM_93C46 : EEPROM_BOOT_EFUSE;

	if(_SUCCESS == pEEPROM->bAutoload)
	{
		if (_TRUE == pEEPROM->bBootFromEEPROM)
		{
			// Read all Content from EEPROM or EFUSE.
			for(i = 0; i < HWSET_MAX_SIZE; i += 2)
			{
				//todo:
				//value16 = EF2Byte(ReadEEprom(Adapter, (u16) (i>>1)));
				//*((u16*)(&PROMContent[i])) = value16; 				
			}
		}
		else
		{
			// Read EFUSE real map to shadow.
			EFUSE_ShadowMapUpdate(Adapter);						
		}		
	}	
	
	_InitAdapterVariablesByPROM(Adapter);
	
}


static VOID
_InitOtherVariable(
	IN PADAPTER 		Adapter
	)
{
#if 0
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	


	if(Adapter->bInHctTest){
		pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
		pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
		pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
		pMgntInfo->keepAliveLevel = 0;
	}

	// 2009/06/10 MH For 92S 1*1=1R/ 1*2&2*2 use 2R. We default set 1*1 use radio A
	// So if you want to use radio B. Please modify RF path enable bit for correct signal
	// strength calculate.
	if (pHalData->rf_type == RF_1T1R){
		pHalData->bRFPathRxEnable[0] = TRUE;
	}
	else{
		pHalData->bRFPathRxEnable[0] = pHalData->bRFPathRxEnable[1] = TRUE;
	}

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("RegChannelPlan(%d) EEPROMChannelPlan(%d)", pMgntInfo->RegChannelPlan, pHalData->EEPROMChannelPlan));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("ChannelPlan = %d\n" , pMgntInfo->ChannelPlan));
#endif
}

static VOID
_ReadRFChipType(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

#if DISABLE_BB_RF
	pHalData->rf_chip = RF_PSEUDO_11N;
#else
	pHalData->rf_chip = RF_6052;
#endif
}

void rtl8192c_ReadChipVersion(
	IN PADAPTER			Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	pHalData->VersionID = ReadChipVersion(Adapter);
}


int ReadAdapterInfo8192C(PADAPTER	Adapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	MSG_8192C("====> ReadAdapterInfo8192C\n");

#ifdef CONFIG_USB_HCI
	//_ReadChipVersion(Adapter);

	_ReadRFChipType(Adapter);//rf_chip -> _InitRFType()
	
	_ReadPROMContent(Adapter);

	_InitOtherVariable(Adapter);
#endif

	//MSG_8192C("%s()(done), rf_chip=0x%x, rf_type=0x%x\n",  __FUNCTION__, pHalData->rf_chip, pHalData->rf_type);

	MSG_8192C("<==== ReadAdapterInfo8192C\n");
	
	return _SUCCESS;
}

u8 GetEEPROMSize8192C(PADAPTER Adapter)
{
	u8	size = 0;
	u32	curRCR;

	curRCR = rtw_read16(Adapter, REG_9346CR);
	size = (curRCR & BOOT_FROM_EEPROM) ? 6 : 4; // 6: EEPROM used is 93C46, 4: boot from E-Fuse.
	
	MSG_8192C("EEPROM type is %s\n", size==4 ? "E-FUSE" : "93C46");
	
	return size;
}

void NicIFReadAdapterInfo8192C(PADAPTER Adapter)
{
	// Read EEPROM size before call any EEPROM function
	//Adapter->EepromAddressSize=Adapter->HalFunc.GetEEPROMSizeHandler(Adapter);
	Adapter->EepromAddressSize = GetEEPROMSize8192C(Adapter);
	
	ReadAdapterInfo8192C(Adapter);
}

