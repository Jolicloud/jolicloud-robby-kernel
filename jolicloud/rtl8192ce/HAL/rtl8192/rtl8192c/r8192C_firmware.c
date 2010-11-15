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
#if defined(RTL8192CE)
#include "../rtl_core.h"

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) && defined(USE_FW_SOURCE_IMG_FILE)
#include <linux/firmware.h>
#endif

#define   byte(x,n)  ( (x >> (8 * n)) & 0xff  )

bool
_IsFWDownloaded(struct net_device *dev)
{
	return ((read_nic_dword(dev, REG_MCUFWDL) & MCUFWDL_RDY) ? true : false);
}



void
_FWDownloadEnalbe(
	struct net_device *dev,
	bool			enable
	)
{
	u8	tmp;

	if(enable)
	{
		tmp = read_nic_byte(dev, REG_SYS_FUNC_EN+1);
		write_nic_byte(dev, REG_SYS_FUNC_EN+1, tmp|0x04);

		tmp = read_nic_byte(dev, REG_MCUFWDL);
		write_nic_byte(dev, REG_MCUFWDL, tmp|0x01);

		tmp = read_nic_byte(dev, REG_MCUFWDL+2);
		write_nic_byte(dev, REG_MCUFWDL+2, tmp&0xf7);
	}
	else
	{
		tmp = read_nic_byte(dev, REG_MCUFWDL);
		write_nic_byte(dev, REG_MCUFWDL, tmp&0xfe);

		write_nic_byte(dev, REG_MCUFWDL+1, 0x00);
	}
}


void
_BlockWrite(
	struct net_device *dev,
	void*			buffer,
	u32				size
	)
{

	u32			blockSize	= sizeof(u32);	
	u8*			bufferPtr	= (u8*)buffer;
	u32*		pu4BytePtr	= (u32*)buffer;
	u32			i, offset, blockCount, remainSize;

	blockCount = size / blockSize;
	remainSize = size % blockSize;

	for(i = 0 ; i < blockCount ; i++){
		offset = i * blockSize;
		write_nic_dword(dev, (FW_8192C_START_ADDRESS + offset), *(pu4BytePtr + i));
	}

	if(remainSize){
		offset = blockCount * blockSize;
		bufferPtr += offset;
		
		for(i = 0 ; i < remainSize ; i++){
			write_nic_byte(dev, (FW_8192C_START_ADDRESS + offset + i), *(bufferPtr + i));
		}
	}
}

void
_PageWrite(
	struct net_device *dev,
	u32			page,
	void*		buffer,
	u32			size
	)
{
	u8 value8;
	u8 u8Page = (u8) (page & 0x07) ;

	value8 = (read_nic_byte(dev, REG_MCUFWDL+2)& 0xF8 ) | u8Page ;
	write_nic_byte(dev,REG_MCUFWDL+2,value8);
	_BlockWrite(dev,buffer,size);
}

void _FillDummy(
	u8*		pFwBuf,
	u32*	pFwLen
	)
{
	u32	FwLen = *pFwLen;
	u8	remain = (u8)(FwLen%4);
	remain = (remain==0)?0:(4-remain);

	while(remain>0)
	{
		pFwBuf[FwLen] = 0;
		FwLen++;
		remain--;
	}

	*pFwLen = FwLen;
}


void
_WriteFW(
	struct net_device *dev,
	void*			buffer,
	u32			size
	)
{
	
	struct r8192_priv   *priv = rtllib_priv(dev);
	bool			isNormalChip;	
	isNormalChip = IS_NORMAL_CHIP(priv->card_8192_version);

	if(isNormalChip){
		u32 	pageNums,remainSize ;
		u32 	page,offset;
		u8*	bufferPtr = (u8*)buffer;
		
		_FillDummy(bufferPtr, &size);
		
		pageNums = size / MAX_PAGE_SIZE ;		
		RT_ASSERT((pageNums <= 4), ("Page numbers should not greater then 4 \n"));			
		remainSize = size % MAX_PAGE_SIZE;		
		
		for(page = 0; page < pageNums;  page++){
			offset = page *MAX_PAGE_SIZE;
			_PageWrite(dev,page, (bufferPtr+offset),MAX_PAGE_SIZE);			
		}
		if(remainSize){
			offset = pageNums *MAX_PAGE_SIZE;
			page = pageNums;
			_PageWrite(dev,page, (bufferPtr+offset),remainSize);
		}	
		RT_TRACE(COMP_INIT, "_WriteFW Done- for Normal chip.\n");
	}
	else	{
		_BlockWrite(dev,buffer,size);
		RT_TRACE(COMP_INIT, "_WriteFW Done- for Test chip.\n");
	}
}


RT_STATUS
_FWFreeToGo(
	struct net_device *dev
	)
{
	u32			counter = 0;
	u32			value32;

	do{
		value32 = read_nic_dword(dev, REG_MCUFWDL);
	}while((counter ++ < POLLING_READY_TIMEOUT_COUNT) && (!(value32 & FWDL_ChkSum_rpt  )));	

	if(counter >= POLLING_READY_TIMEOUT_COUNT){	
		RT_TRACE(COMP_INIT, "chksum report faill ! REG_MCUFWDL:0x%08x .\n",value32);		
		return RT_STATUS_FAILURE;
	}
	RT_TRACE(COMP_INIT, "Checksum report OK ! REG_MCUFWDL:0x%08x .\n",value32);

	value32 = read_nic_dword(dev, REG_MCUFWDL);
	value32 |= MCUFWDL_RDY;
	value32 &= ~WINTINI_RDY;
	write_nic_dword(dev, REG_MCUFWDL, value32);

	do
	{
		if(read_nic_dword(dev, REG_MCUFWDL) & WINTINI_RDY){
			return RT_STATUS_SUCCESS;
		}
		udelay(5);
	}
	while(counter++ < POLLING_READY_TIMEOUT_COUNT);

	RT_TRACE(COMP_INIT, "Polling FW ready fail!! \n");
	return RT_STATUS_FAILURE;
	
}


bool
FirmwareDownload92C(
	struct net_device *dev
)
{	
	struct r8192_priv   *priv = rtllib_priv(dev);
	bool		        rtStatus = true;
	
	char* 		R88CFwImageFileName ={"RTL8192CE/rtl8192cfw.bin"};
	char*		R92CFwImageFileName ={"RTL8192CE/rtl8192cfw.bin"};
	
	char*		TestChipFwFile = {"RTL8192CE/rtl8192cfw.bin"};
	u8*		FwImage;
	u32		FwImageLen;
	
	char*		pFwImageFileName[1] = {"RTL8192CE/rtl8192cfw.bin"};
	PRT_8192C_FIRMWARE_HDR		pFwHdr = NULL;
	u8		*pFirmwareBuf;
	u32		FirmwareLen;
	PRT_FIRMWARE_92C	pFirmware = priv->pFirmware;;

	
	RT_TRACE(COMP_INIT, " --->FirmwareDownload91C()\n");

	if(IS_NORMAL_CHIP(priv->card_8192_version))
	{
		if(IS_92C_SERIAL(priv->card_8192_version))
			pFwImageFileName[0] = R92CFwImageFileName;
		else
			pFwImageFileName[0] = R88CFwImageFileName;

		FwImage = Rtl819XFwImageArray;
		FwImageLen = ImgArrayLength;
		RT_TRACE(COMP_INIT, " ===> FirmwareDownload91C() fw:Rtl819XFwImageArray\n");
	}
	else
	{
		pFwImageFileName[0] = TestChipFwFile;
		FwImage = Rtl8192CTestFwImg;
		FwImageLen = Rtl8192CTestFwImgLen;
		RT_TRACE(COMP_INIT, " ===> FirmwareDownload91C() fw:Rtl8192CTestFwImg\n");
	}
	
#ifndef USE_FW_SOURCE_IMG_FILE
	pFirmware->eFWSource = FW_SOURCE_HEADER_FILE;
#else
	pFirmware->eFWSource = FW_SOURCE_IMG_FILE; 
#endif

	switch(pFirmware->eFWSource)
	{
		case FW_SOURCE_IMG_FILE:
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) && defined(USE_FW_SOURCE_IMG_FILE)			
			if(pFirmware->ulFwLength == 0)
			{
				const struct firmware 	*fw_entry = NULL;
				u32 ulInitStep = 0;
				int 			rc = 0;
				rc = request_firmware(&fw_entry, pFwImageFileName[ulInitStep],&priv->pdev->dev);
				if(rc < 0 ) {
					RT_TRACE(COMP_ERR, "request firmware fail!\n");
					goto Exit;
				} 

				if(fw_entry->size > sizeof(pFirmware->szFwBuffer)) {
					RT_TRACE(COMP_ERR, "img file size exceed the container buffer fail!\n");
					release_firmware(fw_entry);
					goto Exit;
				}	

				memcpy(pFirmware->szFwBuffer,fw_entry->data,fw_entry->size);
				pFirmware->ulFwLength = fw_entry->size;
				release_firmware(fw_entry);

			}
#endif
			break;
		case FW_SOURCE_HEADER_FILE:
			if(ImgArrayLength > FW_8192C_SIZE){
				rtStatus = false;
				RT_TRACE(COMP_INIT, "Firmware size exceed 0x%X. Check it.\n", FW_8192C_SIZE);
				goto Exit;
			}

			memcpy(pFirmware->szFwBuffer, FwImage, FwImageLen);
			pFirmware->ulFwLength = FwImageLen;

			break;
	}

	pFirmwareBuf = pFirmware->szFwBuffer;
	FirmwareLen = pFirmware->ulFwLength;

	pFwHdr = (PRT_8192C_FIRMWARE_HDR)pFirmware->szFwBuffer;

	priv->firmware_version =  byte(pFwHdr->Version ,0); 
	priv->FirmwareSubVersion = byte(pFwHdr->Subversion, 0); 

	RT_TRACE(COMP_INIT, " FirmwareVersion(%#x), Signature(%#x)\n", 
		priv->firmware_version, pFwHdr->Signature);

	if(IS_FW_HEADER_EXIST(pFwHdr))
	{
		RT_TRACE(COMP_INIT, "Shift 32 bytes for FW header!!\n");
		pFirmwareBuf = pFirmwareBuf + 32;
		FirmwareLen = FirmwareLen -32;
	}
	
	if(read_nic_byte(dev, REG_MCUFWDL)&BIT7) 
	{	
		FirmwareSelfReset(dev);
		write_nic_byte(dev, REG_MCUFWDL, 0x00);		
	}
	
	_FWDownloadEnalbe(dev, true);
	_WriteFW(dev, pFirmwareBuf,FirmwareLen);
	_FWDownloadEnalbe(dev, false);
	if(RT_STATUS_SUCCESS != _FWFreeToGo(dev)){
                rtStatus = false;
		RT_TRACE(COMP_INIT, "DL Firmware failed!\n");
		goto Exit;
	}
	

Exit:
	return rtStatus;

}

bool
CheckWriteH2C(
	struct net_device *	dev,
	u8				BoxNum
)
{
	u8	valHMETFR;
	bool	Result = false;
	
	valHMETFR = read_nic_byte(dev, REG_HMETFR);


	if(((valHMETFR>>BoxNum)&BIT0) == 1)
		Result = true;
	
	return Result;

}

bool
CheckFwReadLastH2C(
	struct net_device *	dev,
	u8				BoxNum
)
{
	struct r8192_priv   *priv = rtllib_priv(dev);
	u8			valHMETFR, valMCUTST_1;
	bool			Result = false;
	
	valHMETFR = read_nic_byte(dev, REG_HMETFR);


	if(IS_NORMAL_CHIP(priv->card_8192_version))
	{
		if(((valHMETFR>>BoxNum)&BIT0) == 0)
			Result = true;
	}
	else
	{
		valMCUTST_1 = read_nic_byte(dev, (REG_MCUTST_1+BoxNum));

		if((((valHMETFR>>BoxNum)&BIT0) == 0) && (valMCUTST_1 == 0))
		{
			Result = true;
		}
	}
	
	return Result;
}

#if 1
void FillH2CCommand8192C(struct net_device *dev,
	u8 	ElementID,
	u32 	CmdLen,
	u8*	pCmdBuffer
)
{
	struct r8192_priv   *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(priv);
	u8	BoxNum;
	u16	BOXReg = 0, BOXExtReg = 0;
	u8	U1btmp; 
	bool	IsFwRead = false;
	u8	BufIndex=0;
	bool	bWriteSucess = false;
	u8	WaitH2cLimmit = 100;
	u8	WaitWriteH2cLimmit = 100;
	u8	BoxContent[4], BoxExtContent[2];
	u32	H2CWaitCounter = 0;
	RT_RF_POWER_STATE rtState;

	unsigned long flag;

	priv->rtllib->GetHwRegHandler(dev, HW_VAR_RF_STATE, (u8*)(&rtState));
	if(rtState == eRfOff || pPSC->eInactivePowerState == eRfOff)
	{
		RT_TRACE(COMP_CMD, "FillH2CCmd92C(): Return because RF is off!!!\n");
		return;
	}

	RT_TRACE(COMP_CMD, "=======>FillH2CCmd92C() \n");

	while(true)
	{
		spin_lock_irqsave(&priv->rt_h2c_lock,flag);
		if(priv->bH2CSetInProgress)
		{
			RT_TRACE(COMP_CMD, "FillH2CCmd92C(): H2C set in progress! Wait to set..ElementID(%d).\n", ElementID);
			while(priv->bH2CSetInProgress)
			{
				spin_unlock_irqrestore(&priv->rt_h2c_lock,flag);
				H2CWaitCounter ++;
				RT_TRACE(COMP_CMD, "FillH2CCmd92C(): Wait 100 us (%d times)...\n", H2CWaitCounter);
				udelay(100); 

				if(H2CWaitCounter > 1000)
				{
					RT_ASSERT(false, ("MgntActSet_RF_State(): Wait too logn to set RF\n"));
					return;
				}
				spin_lock_irqsave(&priv->rt_h2c_lock,flag);				
			}
			spin_unlock_irqrestore(&priv->rt_h2c_lock,flag);
		}
		else
		{
			priv->bH2CSetInProgress = true;
			spin_unlock_irqrestore(&priv->rt_h2c_lock,flag);
			break;
		}
	}


	while(!bWriteSucess)
	{
		WaitWriteH2cLimmit--;
		if(WaitWriteH2cLimmit == 0)
		{	
			RT_TRACE(COMP_CMD, "FillH2CCmd92C():Write H2C fail because no trigger for FW INT!!!!!!!!\n");
			break;
		}
		
		BoxNum = priv->LastHMEBoxNum;
		switch(BoxNum)
		{
			case 0:
				BOXReg = REG_HMEBOX_0;
				BOXExtReg = REG_HMEBOX_EXT_0;
				break;
			case 1:
				BOXReg = REG_HMEBOX_1;
				BOXExtReg = REG_HMEBOX_EXT_1;
				break;
			case 2:
				BOXReg = REG_HMEBOX_2;
				BOXExtReg = REG_HMEBOX_EXT_2;
				break;
			case 3:
				BOXReg = REG_HMEBOX_3;
				BOXExtReg = REG_HMEBOX_EXT_3;
				break;
			default:
				break;
		}

		IsFwRead = CheckFwReadLastH2C(dev, BoxNum);
		while(!IsFwRead)
		{
			WaitH2cLimmit--;
			if(WaitH2cLimmit == 0)
			{
				RT_TRACE(COMP_CMD, "FillH2CCmd92C(): Wating too long for FW read clear HMEBox(%d)!!!\n", BoxNum);
				break;
			}
			udelay(10); 
			IsFwRead = CheckFwReadLastH2C(dev, BoxNum);
			U1btmp = read_nic_byte(dev, 0x1BF);
			RT_TRACE(COMP_CMD, "FillH2CCmd92C(): Wating for FW read clear HMEBox(%d)!!! 0x1BF = %2x\n", BoxNum, U1btmp);
		}

		if(!IsFwRead)
		{
			RT_TRACE(COMP_CMD, "FillH2CCmd92C():  Write H2C register BOX[%d] fail!!!!! Fw do not read. \n", BoxNum);
			break;
		}

		memset(BoxContent, 0, sizeof(BoxContent));
		memset(BoxExtContent, 0, sizeof(BoxExtContent));
		
		BoxContent[0] = ElementID; 

		RT_TRACE(COMP_CMD, "FillH2CCmd92C():Write ElementID BOXReg(%4x) = %2x \n", BOXReg, ElementID);

		switch(CmdLen)
		{
		case 1:
		{
			BoxContent[0] &= ~(BIT7);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 1);
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		case 2:
		{	
			BoxContent[0] &= ~(BIT7);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 2);
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		case 3:
		{
			BoxContent[0] &= ~(BIT7);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 3);
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		case 4:
		{
			BoxContent[0] |= (BIT7);
			memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 2);
			write_nic_word(dev, BOXExtReg, *((u16*)BoxExtContent));
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		case 5:
		{
			BoxContent[0] |= (BIT7);
			memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 3);
			write_nic_word(dev, BOXExtReg, *((u16*)BoxExtContent));
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		default:
			break;
		}
			
		RT_PRINT_DATA(COMP_CMD, DBG_LOUD, "FillH2CCmd(): BoxExtContent\n", BoxExtContent, 2);		
		RT_PRINT_DATA(COMP_CMD, DBG_LOUD, "FillH2CCmd(): BoxContent\n", 	BoxContent, 4);
			
		if(IS_NORMAL_CHIP(priv->card_8192_version))
		{
			bWriteSucess = true;
		}
		else
		{	
			bWriteSucess = CheckWriteH2C(dev, BoxNum);
			if(!bWriteSucess) 
				continue;
			

			write_nic_byte(dev, REG_MCUTST_1+BoxNum, 0xFF);
			RT_TRACE(COMP_CMD, "FillH2CCmd92C():Write Reg(%4x) = 0xFF \n", REG_MCUTST_1+BoxNum);
		}

		priv->LastHMEBoxNum = BoxNum+1;
		if(priv->LastHMEBoxNum == 4) 
			priv->LastHMEBoxNum = 0;
		
		RT_TRACE(COMP_CMD, "FillH2CCmd92C():pHalData->LastHMEBoxNum  = %d\n", priv->LastHMEBoxNum);
	}

	spin_lock_irqsave(&priv->rt_h2c_lock,flag);
	priv->bH2CSetInProgress = false;
	spin_unlock_irqrestore(&priv->rt_h2c_lock,flag);

	
	RT_TRACE(COMP_CMD, "<=======FillH2CCmd92C() \n");

}

void
FillH2CCmd92C(	
	struct net_device *	dev,
	u8 				ElementID,
	u32 				CmdLen,
	u8*				pCmdBuffer
)
{
	u32	tmpCmdBuf[2];	
	
	memset(tmpCmdBuf, 0, 8);
	memcpy(tmpCmdBuf, pCmdBuffer, CmdLen);
	

	FillH2CCommand8192C(dev, ElementID, CmdLen, (u8*)&tmpCmdBuf);

	return;
}
#else
void
FillH2CCmd92C(	
	struct net_device *	dev,
	u8 				ElementID,
	u32 				CmdLen,
	u8*				pCmdBuffer
)
{
	struct r8192_priv   *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(priv);
	
	u8	BoxNum;
	u16	BOXReg =0 , BOXExtReg = 0;
	u8	U1btmp; 
	bool	IsFwRead = false;
	u8	BufIndex=0;
	bool	bWriteSucess = false;
	u8	WaitH2cLimmit = 100;
	u8	WaitWriteH2cLimmit = 100;
	u8	BoxContent[4], BoxExtContent[2];
	u32	H2CWaitCounter = 0;
	RT_RF_POWER_STATE rtState;

	unsigned long flag;

	RT_TRACE(COMP_CMD, "=======>FillH2CCmd92C() \n");

	priv->rtllib->GetHwRegHandler(dev, HW_VAR_RF_STATE, (u8*)(&rtState));
	if(rtState == eRfOff || pPSC->eInactivePowerState == eRfOff)
	{
		RT_TRACE(COMP_CMD, "FillH2CCmd92C(): Return because RF is off!!!\n");
		return;
	}
	
	while(true)
	{
		spin_lock_irqsave(&priv->rt_h2c_lock,flag);
		if(priv->bH2CSetInProgress)
		{
			RT_TRACE(COMP_CMD, "FillH2CCmd92C(): H2C set in progress! Wait to set..ElementID(%d).\n", ElementID);
			while(priv->bH2CSetInProgress)
			{
				spin_unlock_irqrestore(&priv->rt_h2c_lock,flag);
				H2CWaitCounter ++;
				RT_TRACE(COMP_CMD, "FillH2CCmd92C(): Wait 100 us (%d times)...\n", H2CWaitCounter);
				udelay(100); 

				if(H2CWaitCounter > 1000)
				{
					RT_ASSERT(false, ("MgntActSet_RF_State(): fail because no trigger for FW INT!!!!!!!!\n"));
					return;
				}
				spin_lock_irqsave(&priv->rt_h2c_lock,flag);
			}
			spin_unlock_irqrestore(&priv->rt_h2c_lock,flag);
		}
		else
		{
			priv->bH2CSetInProgress = true;
			spin_unlock_irqrestore(&priv->rt_h2c_lock,flag);
			break;
		}
	}


	while(!bWriteSucess)
	{
		WaitWriteH2cLimmit--;
		if(WaitWriteH2cLimmit == 0)
		{	
			RT_TRACE(COMP_CMD, "FillH2CCmd92C():Wait write H2C too long!!!!!!!!\n");
			break;
		}
		
		BoxNum = priv->LastHMEBoxNum;
		switch(BoxNum)
		{
			case 0:
				BOXReg = REG_HMEBOX_0;
				BOXExtReg = REG_HMEBOX_EXT_0;
				break;
			case 1:
				BOXReg = REG_HMEBOX_1;
				BOXExtReg = REG_HMEBOX_EXT_1;
				break;
			case 2:
				BOXReg = REG_HMEBOX_2;
				BOXExtReg = REG_HMEBOX_EXT_2;
				break;
			case 3:
				BOXReg = REG_HMEBOX_3;
				BOXExtReg = REG_HMEBOX_EXT_3;
				break;
			default:
				break;
		}

		IsFwRead = CheckFwReadLastH2C(dev, BoxNum);
		while(!IsFwRead)
		{
			WaitH2cLimmit--;
			if(WaitH2cLimmit == 0)
			{
				RT_TRACE(COMP_CMD, "FillH2CCmd92C(): Wating too long for FW read clear  HMEBox(%d)!!!\n", BoxNum);
				break;
			}
			udelay(10); 
			IsFwRead = CheckFwReadLastH2C(dev, BoxNum);
			U1btmp = read_nic_byte(dev, 0x1BF);
			RT_TRACE(COMP_CMD, "FillH2CCmd92C(): Wating for FW read clear  HMEBox(%d)!!! 0x1BF = %2x\n", BoxNum, U1btmp);
		}

		if(!IsFwRead)
		{
			RT_TRACE(COMP_CMD, "FillH2CCmd92C():  Write H2C register BOX[%d] fail!!!!! Fw do not read. \n", BoxNum);
			break;
		}

		memset(BoxContent, 0, sizeof(BoxContent));
		memset(BoxContent, 0, sizeof(BoxExtContent));
		
		BoxContent[0] = ElementID; 

		RT_TRACE(COMP_CMD, "FillH2CCmd92C():Write ElementID BOXReg(%4x) = %2x \n", BOXReg, ElementID);

		switch(CmdLen)
		{
		case 1:
		{
			BoxContent[0] &= ~(BIT7);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 1);
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		case 2:
		{	
			BoxContent[0] &= ~(BIT7);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 2);
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		case 3:
		{
			BoxContent[0] &= ~(BIT7);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 3);
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		case 4:
		{
			BoxContent[0] |= (BIT7);
			memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 2);
			write_nic_word(dev, BOXExtReg, *((u16*)BoxExtContent));
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		case 5:
		{
			BoxContent[0] |= (BIT7);
			memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
			memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 3);
			write_nic_word(dev, BOXExtReg, *((u16*)BoxExtContent));
			write_nic_dword(dev, BOXReg, *((u32*)BoxContent));
			break;
		}
		default:
			break;
		}
			
		RT_PRINT_DATA(COMP_CMD, DBG_LOUD,"FillH2CCmd(): BoxExtContent\n", 	BoxExtContent, 2);		
		RT_PRINT_DATA(COMP_CMD, DBG_LOUD,"FillH2CCmd(): BoxContent\n", 	BoxContent, 4);
			
		if(IS_NORMAL_CHIP(priv->card_8192_version))
		{
			bWriteSucess = true;
		}
		else
		{
			bWriteSucess = CheckWriteH2C(dev, BoxNum);
			if(!bWriteSucess) 
				continue;

			write_nic_byte(dev, REG_MCUTST_1+BoxNum, 0xFF);
			RT_TRACE(COMP_CMD, "FillH2CCmd92C():Write Reg(%4x) = 0xFF \n", REG_MCUTST_1+BoxNum);
		}

		priv->LastHMEBoxNum = BoxNum+1;
		if(priv->LastHMEBoxNum == 4) 
			priv->LastHMEBoxNum = 0;
		
		RT_TRACE(COMP_CMD, "FillH2CCmd92C():pHalData->LastHMEBoxNum  = %d\n", priv->LastHMEBoxNum);
	}

	spin_lock_irqsave(&priv->rt_h2c_lock,flag);
	priv->bH2CSetInProgress = false;
	spin_unlock_irqrestore(&priv->rt_h2c_lock,flag);
	
	RT_TRACE(COMP_CMD, "<=======FillH2CCmd92C() \n");
}
#endif

void
SetFwPwrModeCmd(
	struct net_device *	dev,
	u8				Mode
	)
{
	struct r8192_priv   *priv = rtllib_priv(dev);
	SETPWRMODE_PARM H2CSetPwrMode;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(priv);

	H2CSetPwrMode.Mode = Mode;
	H2CSetPwrMode.SmartPS = 1;
	H2CSetPwrMode.BcnPassTime = pPSC->RegMaxLPSAwakeIntvl;
	
	FillH2CCmd92C(dev, H2C_SETPWRMODE, sizeof(H2CSetPwrMode), (u8*)&H2CSetPwrMode);

}

void FirmwareSelfReset(struct net_device *dev)
{
	u8	u1bTmp;
	u8	Delay = 100;
	struct r8192_priv   *priv = rtllib_priv(dev);

	if((priv->firmware_version > 0x21) ||
		(priv->firmware_version == 0x21 && 
		priv->FirmwareSubVersion >= 0x01))
	{
		write_nic_byte(dev, REG_HMETFR+3, 0x20);
	
		u1bTmp = read_nic_byte(dev, REG_SYS_FUNC_EN+1);
		while(u1bTmp&BIT2)
		{
			Delay--;
			RT_TRACE(COMP_INIT, "PowerOffAdapter8192CE(): polling 0x03[2] Delay = %d \n", Delay);
			if(Delay == 0)
				break;
			udelay(50);
			u1bTmp = read_nic_byte(dev, REG_SYS_FUNC_EN+1);
		}
	
		if((u1bTmp&BIT2) && (Delay == 0))
		{
		}
	}
}

#endif
