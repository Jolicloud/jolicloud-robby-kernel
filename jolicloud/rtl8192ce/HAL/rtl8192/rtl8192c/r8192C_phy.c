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

#ifdef ENABLE_DOT11D
#include "../../../rtllib/dot11d.h"
#endif

/*---------------------------Define Local Constant---------------------------*/
/* Channel switch:The size of command tables for switch channel*/
#define MAX_PRECMD_CNT 16
#define MAX_RFDEPENDCMD_CNT 16
#define MAX_POSTCMD_CNT 16

#define MAX_DOZE_WAITING_TIMES_9x 64

/*---------------------------Define Local Constant---------------------------*/


/*------------------------Define global variable-----------------------------*/

/*------------------------Define local variable------------------------------*/


/*--------------------Define export function prototype-----------------------*/
/*--------------------Define export function prototype-----------------------*/


/*---------------------Define local function prototype-----------------------*/
/* RF serial read/write by firmware 3wire. */
static	u32	phy_FwRFSerialRead(struct net_device* dev,
									RF90_RADIO_PATH_E	eRFPath,
									u32				Offset	);
static	void	phy_FwRFSerialWrite(struct net_device* dev,
										RF90_RADIO_PATH_E	eRFPath,
										u32				Offset,
										u32				Data	);

/* RF serial read/write */
static	u32	phy_RFSerialRead(	struct net_device* dev,
									RF90_RADIO_PATH_E	eRFPath,
									u32				Offset	);
static	void	phy_RFSerialWrite(	struct net_device* dev,
									RF90_RADIO_PATH_E	eRFPath,
									u32				Offset,
									u32				Data	);

static	u32	phy_CalculateBitShift(	u32 BitMask	);

static	RT_STATUS	phy_BB8190_Config_HardCode(struct net_device* dev	);
static	RT_STATUS	phy_BB8192C_Config_ParaFile(	struct net_device* dev	);
/* MAC config */
#if !RTL8190_Download_Firmware_From_Header
static	RT_STATUS	phy_ConfigMACWithParaFile(	struct net_device* dev,
        u8 	pFileName);
#endif
static	RT_STATUS	phy_ConfigMACWithHeaderFile(struct net_device* dev);
/* BB config */
#if !RTL8190_Download_Firmware_From_Header
static	RT_STATUS	phy_ConfigBBWithParaFile(	struct net_device* dev,
        u8 	pFileName);
#endif
static	RT_STATUS	phy_ConfigBBWithHeaderFile(	struct net_device* dev,
												u8 		ConfigType);
static	RT_STATUS
phy_ConfigBBWithPgHeaderFile(struct net_device* dev,u8	ConfigType);
#if !RTL8190_Download_Firmware_From_Header
static	RT_STATUS	phy_SetBBtoDiffRFWithParaFile(	
        struct net_device* dev,
        u8 		pFileName);
#endif
#if !RTL8190_Download_Firmware_From_Header
static	RT_STATUS	phy_SetBBtoDiffRFWithHeaderFile(	
												struct net_device* dev,
												u8 		ConfigType);
#endif
/*Initialize Register definition*/
static	void	phy_InitBBRFRegisterDefinition(struct net_device* dev	);

/* Channel switch related */
static	bool	phy_SetSwChnlCmdArray(	SwChnlCmd*		CmdTable,
										u32			CmdTableIdx,
										u32			CmdTableSz,
										SwChnlCmdID		CmdID,
										u32			Para1,
										u32			Para2,
										u32			msDelay	);
static	bool	phy_SwChnlStepByStep(	struct net_device* dev,
										u8		channel,
										u8		*stage,
										u8		*step,
										u32		*delay	);
static	void	phy_FinishSwChnlNow(	struct net_device* dev,
										u8		channel		);

static	u8	phy_DbmToTxPwrIdx(	struct net_device* dev,
									WIRELESS_MODE	WirelessMode,
									long			PowerInDbm	);

						

bool
phy_SetRFPowerState8192CE(
	struct net_device* dev,
	RT_RF_POWER_STATE	eRFPowerState
	);

bool
phy_SetRFPowerState8192SU(
	struct net_device* dev,
	RT_RF_POWER_STATE	eRFPowerState
	);


#if 1
#endif
						
RT_STATUS
PHY_ConfigRFExternalPA(
	struct net_device* dev,
	RF90_RADIO_PATH_E		eRFPath
);						

void
phy_ConfigBBExternalPA(
	struct net_device* dev
);

void
phy_SetRTL8192CERfOn(struct net_device* dev);
void
phy_SetRTL8192CERfSleep(struct net_device* dev);

bool
HalSetIO8192C(
	struct net_device* dev,
	IO_TYPE		IOType
);
void
phy_SetIO(
    struct net_device* dev
    );
/*----------------------------Function Body----------------------------------*/
/**
* Function:	PHY_QueryBBReg
*
* OverView:	Read "sepcific bits" from BB register
*
* Input:
*			PADAPTER		Adapter,
*			u32			RegAddr,		
*			u32			BitMask		
*										
* Output:	None
* Return:		u32			Data			
* Note:		This function is equal to "GetRegSetting" in PHY programming guide
*/
u32
rtl8192_QueryBBReg(
	struct net_device* dev,
	u32		RegAddr,
	u32		BitMask
	)
{
  	u32	ReturnValue = 0, OriginalValue, BitShift;

#if (DISABLE_BB_RF == 1)
	return 0;
#endif

	RT_TRACE(COMP_RF, "--->PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x)\n", RegAddr, BitMask);

	OriginalValue = read_nic_dword(dev, RegAddr);
	BitShift = phy_CalculateBitShift(BitMask);
	ReturnValue = (OriginalValue & BitMask) >> BitShift;

	RT_TRACE(COMP_RF, "BBR MASK=0x%x Addr[0x%x]=0x%x\n", BitMask, RegAddr, OriginalValue);
	RT_TRACE(COMP_RF, "<---PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x), OriginalValue(%#x)\n", RegAddr, BitMask, OriginalValue);

	return (ReturnValue);

}

/**
* Function:	PHY_SetBBReg
*
* OverView:	Write "Specific bits" to BB register (page 8~) 
*
* Input:
*			PADAPTER		Adapter,
*			u32			RegAddr,		
*			u32			BitMask		
*										
*			u32			Data			
*										
*
* Output:	None
* Return:		None
* Note:		This function is equal to "PutRegSetting" in PHY programming guide
*/

void
PHY_SetBBReg(
	struct net_device* dev,
	u32		RegAddr,
	u32		BitMask,
	u32		Data
	)
{
	u32			OriginalValue, BitShift;

#if (DISABLE_BB_RF == 1)
	return;
#endif

	RT_TRACE(COMP_RF, "--->PHY_SetBBReg(): RegAddr(%#x), BitMask(%#x), Data(%#x)\n", RegAddr, BitMask, Data);

	if(BitMask!= bMaskDWord){
		OriginalValue = read_nic_dword(dev, RegAddr);
		BitShift = phy_CalculateBitShift(BitMask);
		Data = ((OriginalValue & (~BitMask)) | (Data << BitShift));
	}

	write_nic_dword(dev, RegAddr, Data);

	RT_TRACE(COMP_RF, "<---PHY_SetBBReg(): RegAddr(%#x), BitMask(%#x), Data(%#x)\n", RegAddr, BitMask, Data);
	
}


/**
* Function:	PHY_QueryRFReg
*
* OverView:	Query "Specific bits" to RF register (page 8~) 
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	
*			u32			RegAddr,		
*			u32			BitMask		
*										
*
* Output:	None
* Return:		u32			Readback value
* Note:		This function is equal to "GetRFRegSetting" in PHY programming guide
*/
u32
rtl8192_phy_QueryRFReg(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				RegAddr,
	u32				BitMask
	)
{
	u32				Original_Value, Readback_Value, BitShift;	
	struct r8192_priv 	*priv = rtllib_priv(dev);
	unsigned long flags;

#if (DISABLE_BB_RF == 1)
	return 0;
#endif
	
	RT_TRACE(COMP_RF, "--->PHY_QueryRFReg(): RegAddr(%#x), eRFPath(%#x), BitMask(%#x)\n", RegAddr, eRFPath,BitMask);
	
#if 0
	PlatformAcquireMutex(&pHalData->mxRFOperate);
#else
	spin_lock_irqsave(&priv->rf_lock, flags);
#endif

	if (priv->Rf_Mode != RF_OP_By_FW){	
		Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);	   	
	}
	else{	
		Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
	}
	
	BitShift =  phy_CalculateBitShift(BitMask);
	Readback_Value = (Original_Value & BitMask) >> BitShift;	

#if 0
	PlatformReleaseMutex(&pHalData->mxRFOperate);
#else
	spin_unlock_irqrestore(&priv->rf_lock, flags);
#endif

	RT_TRACE(COMP_RF, "<---PHY_QueryRFReg(): RegAddr(%#x), eRFPath(%#x), BitMask(%#x), Original_Value(%#x)\n", 
					RegAddr, eRFPath,BitMask, Original_Value);
	
	return (Readback_Value);
}

/**
* Function:	PHY_SetRFReg
*
* OverView:	Write "Specific bits" to RF register (page 8~) 
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	
*			u32			RegAddr,		
*			u32			BitMask		
*										
*			u32			Data			
*										
*
* Output:	None
* Return:		None
* Note:		This function is equal to "PutRFRegSetting" in PHY programming guide
*/
void
PHY_SetRFReg(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				RegAddr,
	u32				BitMask,
	u32				Data
	)
{

	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32 			Original_Value, BitShift;
	unsigned long flags;

#if (DISABLE_BB_RF == 1)
	return;
#endif
	
	RT_TRACE(COMP_RF, "--->PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
		RegAddr, BitMask, Data, eRFPath);


#if 0
	PlatformAcquireMutex(&pHalData->mxRFOperate);
#else
	spin_lock_irqsave(&priv->rf_lock, flags);
#endif

	if(priv->Rf_Mode != RF_OP_By_FW){
		if (BitMask != bRFRegOffsetMask) {
			Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			Data = ((Original_Value & (~BitMask)) | (Data<< BitShift));
		}
		
		phy_RFSerialWrite(dev, eRFPath, RegAddr, Data);
	} else {
		if (BitMask != bRFRegOffsetMask){
			Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			Data = ((Original_Value & (~BitMask)) | (Data<< BitShift));
		}		

		phy_FwRFSerialWrite(dev, eRFPath, RegAddr, Data);
	}

#if 0
	PlatformReleaseMutex(&pHalData->mxRFOperate);
#else
	spin_unlock_irqrestore(&priv->rf_lock, flags);
#endif


	RT_TRACE(COMP_RF, "<---PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
			RegAddr, BitMask, Data, eRFPath);
	
}


/*-----------------------------------------------------------------------------
 * Function:	phy_FwRFSerialRead()
 *
 * Overview:	We support firmware to execute RF-R/W.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/21/2008	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
static	u32
phy_FwRFSerialRead(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset	)
{
	u32		retValue = 0;		
	RT_ASSERT(false,("deprecate!\n"));
	return	(retValue);

}	/* phy_FwRFSerialRead */


/*-----------------------------------------------------------------------------
 * Function:	phy_FwRFSerialWrite()
 *
 * Overview:	We support firmware to execute RF-R/W.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/21/2008	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
static	void
phy_FwRFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data	)
{
	RT_ASSERT(false,("deprecate!\n"));
}


/**
* Function:	phy_RFSerialRead
*
* OverView:	Read regster from RF chips 
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	
*			u32			Offset,		
*
* Output:	None
* Return:		u32			reback value
* Note:		Threre are three types of serial operations: 
*			1. Software serial write
*			2. Hardware LSSI-Low Speed Serial Interface 
*			3. Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
*/
static	u32
phy_RFSerialRead(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset
	)
{

	u32						retValue = 0;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;
	u32 						tmplong,tmplong2;
	u8					RfPiEnable=0;
#if 0
	if(pHalData->RFChipID == RF_8225 && Offset > 0x24) 
		return	retValue;
	if(pHalData->RFChipID == RF_8256 && Offset > 0x2D) 
		return	retValue;
#endif
	Offset &= 0x3f;

	NewOffset = Offset;

	if(RT_CANNOT_IO(dev))
	{
		printk("phy_RFSerialRead return all one\n");
		return	0xFFFFFFFF;
	}

	tmplong = PHY_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord);
	if(eRFPath == RF90_PATH_A)
		tmplong2 = tmplong;
	else
	tmplong2 = PHY_QueryBBReg(dev, pPhyReg->rfHSSIPara2, bMaskDWord);
	tmplong2 = (tmplong2 & (~bLSSIReadAddress)) | (NewOffset<<23) | bLSSIReadEdge;	
	
	PHY_SetBBReg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong&(~bLSSIReadEdge));	
	mdelay(1);
	
	PHY_SetBBReg(dev, pPhyReg->rfHSSIPara2, bMaskDWord, tmplong2);	
	mdelay(1);
	
	
	PHY_SetBBReg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong|bLSSIReadEdge);	
	mdelay(1);
	

	if(eRFPath == RF90_PATH_A)
		RfPiEnable = (u8)PHY_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT8);
	else if(eRFPath == RF90_PATH_B)
		RfPiEnable = (u8)PHY_QueryBBReg(dev, rFPGA0_XB_HSSIParameter1, BIT8);
	
	if(RfPiEnable)
	{	
		retValue = PHY_QueryBBReg(dev, pPhyReg->rfLSSIReadBackPi, bLSSIReadBackData);
	}
	else
	{	
		retValue = PHY_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	}
	RT_TRACE(COMP_RF,"RFR-%d Addr[0x%x]=0x%x\n", eRFPath, pPhyReg->rfLSSIReadBack, retValue);
	
	return retValue;	
		
}



/**
* Function:	phy_RFSerialWrite
*
* OverView:	Write data to RF register (page 8~) 
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	
*			u32			Offset,		
*			u32			Data			
*										
*
* Output:	None
* Return:		None
* Note:		Threre are three types of serial operations: 
*			1. Software serial write
*			2. Hardware LSSI-Low Speed Serial Interface 
*			3. Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
 *
 * Note: 		  For RF8256 only
 *			 The total count of RTL8256(Zebra4) register is around 36 bit it only employs 
 *			 4-bit RF address. RTL8256 uses "register mode control bit" (Reg00[12], Reg00[10]) 
 *			 to access register address bigger than 0xf. See "Appendix-4 in PHY Configuration
 *			 programming guide" for more details. 
 *			 Thus, we define a sub-finction for RTL8526 register address conversion
 *		       ===========================================================
 *			 Register Mode		RegCTL[1]		RegCTL[0]		Note
 *								(Reg00[12])		(Reg00[10])
 *		       ===========================================================
 *			 Reg_Mode0				0				x			Reg 0 ~15(0x0 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode1				1				0			Reg 16 ~30(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode2				1				1			Reg 31 ~ 45(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
 *
 *	2008/09/02	MH	Add 92S RF definition
 *	
 *
 *
*/
static	void
phy_RFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data
	)
{
	u32						DataAndAddr = 0;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;
	
#if 0
	if(pHalData->RFChipID == RF_8225 && Offset > 0x24) 
		return;
	if(pHalData->RFChipID == RF_8256 && Offset > 0x2D) 
		return;
#endif

	if(RT_CANNOT_IO(dev))
	{
		printk("phy_RFSerialWrite stop\n");
		return;
	}

	Offset &= 0x3f;

	PHY_RFShadowWrite(dev, eRFPath, Offset, Data);	

		NewOffset = Offset;

	DataAndAddr = ((NewOffset<<20) | (Data&0x000fffff)) & 0x0fffffff;	

	PHY_SetBBReg(dev, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);
	RT_TRACE(COMP_RF, "RFW-%d Addr[0x%x]=0x%x\n", eRFPath, pPhyReg->rf3wireOffset, DataAndAddr);

}

/**
* Function:	phy_CalculateBitShift
*
* OverView:	Get shifted position of the BitMask
*
* Input:
*			u32		BitMask,	
*
* Output:	none
* Return:		u32		Return the shift bit bit position of the mask
*/
static	u32
phy_CalculateBitShift(
	u32 BitMask
	)
{
	u32 i;

	for(i=0; i<=31; i++)
	{
		if ( ((BitMask>>i) &  0x1 ) == 1)
			break;
	}

	return (i);
}

void phy_BB8192C_Config_1T(
	struct net_device* dev
	)
{
	PHY_SetBBReg(dev, rFPGA0_TxInfo, 0x3, 0x1);
	PHY_SetBBReg(dev, rFPGA1_TxInfo, 0x0303, 0x0101);
	PHY_SetBBReg(dev, 0xe74, 0x0c000000, 0x1);
	PHY_SetBBReg(dev, 0xe78, 0x0c000000, 0x1);
	PHY_SetBBReg(dev, 0xe7c, 0x0c000000, 0x1);
	PHY_SetBBReg(dev, 0xe80, 0x0c000000, 0x1);
	PHY_SetBBReg(dev, 0xe88, 0x0c000000, 0x1);
	
}

/*-----------------------------------------------------------------------------
 * Function:    PHY_MACConfig8192C
 *
 * Overview:	Condig MAC by header file or parameter file.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 *  When		Who		Remark
 *  08/12/2008	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	bool
PHY_MACConfig8192C(
	struct net_device* dev
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool			isNormal = IS_NORMAL_CHIP(priv->card_8192_version);
	bool			is92C = IS_92C_SERIAL(priv->card_8192_version);

	u8			sz88CMACRegFile = RTL8188C_PHY_MACREG;
	u8			sz92CMACRegFile = RTL8192C_PHY_MACREG;
	
	u8			pszMACRegFile;

	if(isNormal)
	{
		if(is92C)
			pszMACRegFile = sz92CMACRegFile;
		else
			pszMACRegFile = sz88CMACRegFile;
	}
	else
	{
		;
	}

#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigMACWithHeaderFile(dev);
#else
	
	RT_TRACE(COMP_INIT, "Read MACREG.txt\n");
	rtStatus = phy_ConfigMACWithParaFile(dev, pszMACRegFile);
#endif
	if(isNormal && is92C)
		write_nic_byte(dev, 0x14,0x71);
	
	return (rtStatus == RT_STATUS_SUCCESS) ? 1:0;

}


bool
PHY_BBConfig8192C(
	struct net_device* dev
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u16	RegVal;
	u32	RegValdw;
	u8 bRegHwParaFile = 1;

	phy_InitBBRFRegisterDefinition(dev);

	RegVal = read_nic_word(dev, REG_SYS_FUNC_EN);
	write_nic_word(dev, REG_SYS_FUNC_EN, RegVal|BIT13|BIT0|BIT1);

	write_nic_byte(dev, REG_AFE_PLL_CTRL, 0x83);
	write_nic_byte(dev, REG_AFE_PLL_CTRL+1, 0xdb);
	write_nic_byte(dev, REG_RF_CTRL, RF_EN|RF_RSTB|RF_SDMRSTB);

#if 0
	write_nic_byte(dev, REG_SYS_FUNC_EN, FEN_USBA | FEN_USBD | FEN_BB_GLB_RSTn | FEN_BBRSTB);
#else
	write_nic_byte(dev, REG_SYS_FUNC_EN, FEN_PPLL|FEN_PCIEA|FEN_DIO_PCIE|FEN_BB_GLB_RSTn|FEN_BBRSTB);
#endif

	if(!IS_NORMAL_CHIP(priv->card_8192_version))
	{
#if 0
		write_nic_byte(dev, REG_LDOHCI12_CTRL, 0x1f);
#else
		write_nic_byte(dev, REG_LDOHCI12_CTRL, 0x1b); 
#endif
	}
	
	write_nic_byte(dev, REG_AFE_XTAL_CTRL+1, 0x80);

	if(!IS_92C_SERIAL(priv->card_8192_version) || IS_92C_1T2R(priv->card_8192_version))
	{
		RegValdw = read_nic_dword(dev, REG_LEDCFG0);
		write_nic_dword(dev, REG_LEDCFG0, RegValdw|BIT23);
	}
	
#ifdef MERGE_TO_DO
	switch(priv->bRegHwParaFile )
#else
       switch(bRegHwParaFile )
#endif
	{
		case 0:
			phy_BB8190_Config_HardCode(dev);
			break;

		case 1:
			rtStatus = phy_BB8192C_Config_ParaFile(dev);
			break;

		case 2:
			phy_BB8190_Config_HardCode(dev);
			phy_BB8192C_Config_ParaFile(dev);
			break;

		default:
			phy_BB8190_Config_HardCode(dev);
			break;
	}
	
#if 0
	PathMap = (u8)(PHY_QueryBBReg(Adapter, rFPGA0_TxInfo, 0xf) |
				PHY_QueryBBReg(Adapter, rOFDM0_TRxPathEnable, 0xf));
	pHalData->RF_PathMap = PathMap;
	for(index = 0; index<4; index++)
	{
		if((PathMap>>index)&0x1)
			rf_num++;
	}

	if((GET_RF_TYPE(priv) ==RF_1T1R && rf_num!=1) ||
		(GET_RF_TYPE(priv)==RF_1T2R && rf_num!=2) ||
		(GET_RF_TYPE(priv)==RF_2T2R && rf_num!=2) ||
		(GET_RF_TYPE(priv)==RF_2T2R_GREEN && rf_num!=2) ||
		(GET_RF_TYPE(priv)==RF_2T4R && rf_num!=4))
	{
		RT_TRACE(
			COMP_INIT, 
			DBG_LOUD, 
			("PHY_BBConfig8192C: RF_Type(%x) does not match RF_Num(%x)!!\n", pHalData->RF_Type, rf_num));
	}
#endif

	return (rtStatus == RT_STATUS_SUCCESS)?1:0;
}


extern	bool
PHY_RFConfig8192C(
	struct net_device* dev
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool		rtStatus = 1;

	switch(priv->rf_chip)
	{
		case RF_6052:
			rtStatus = PHY_RF6052_Config(dev);
			break;
		case RF_8225:
			break;
		case RF_8256:			
			rtStatus = PHY_RF8256_Config(dev);
			break;
		case RF_8258:
			break;
		case RF_PSEUDO_11N:
			break;
		default: 
			break;
	}

	return rtStatus;
}


static	RT_STATUS
phy_BB8190_Config_HardCode(
	struct net_device* dev
	)
{
	RT_ASSERT(false, ("This function is not implement yet!! \n"));
	return RT_STATUS_SUCCESS;
}


static	RT_STATUS
phy_BB8192C_Config_ParaFile(
	struct net_device* dev
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;	
	
	RT_TRACE(COMP_INIT, "==>phy_BB8192C_Config_ParaFile\n");

#if	RTL8190_Download_Firmware_From_Header
#else
	if(IS_92C_SERIAL(priv->card_8192_version)){
		pszBBRegFile=sz92CBBRegFile ;
		pszAGCTableFile =sz92CAGCTableFile;
	}
	else{
		pszBBRegFile=sz88CBBRegFile ;
		pszAGCTableFile =sz88CAGCTableFile;
	}
#endif

#if	RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigBBWithHeaderFile(dev, BaseBand_Config_PHY_REG);
#else
	rtStatus = phy_ConfigBBWithParaFile(dev,pszBBRegFile);
#endif

	if(rtStatus != RT_STATUS_SUCCESS){
		RT_TRACE(COMP_INIT, "phy_BB8192C_Config_ParaFile():Write BB Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	if(priv->rf_type == RF_1T2R)
	{
		phy_BB8192C_Config_1T(dev);
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():Config to 1T!!\n");
	}

	if (priv->AutoloadFailFlag == false)
	{
		priv->pwrGroupCnt = 0;

#if	RTL8190_Download_Firmware_From_Header
		rtStatus = phy_ConfigBBWithPgHeaderFile(dev,BaseBand_Config_PHY_REG);
#else
		rtStatus = phy_ConfigBBWithPgParaFile(dev, (ps1Byte)&szBBRegPgFile);
#endif
	}
	if(rtStatus != RT_STATUS_SUCCESS){
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():BB_PG Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}
	
#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigBBWithHeaderFile(dev,BaseBand_Config_AGC_TAB);
#else
	RT_TRACE(COMP_INIT, "phy_BB8192C_Config_ParaFile AGC_TAB.txt\n");
	rtStatus = phy_ConfigBBWithParaFile(dev, szAGCTableFile);
#endif

	if(rtStatus != RT_STATUS_SUCCESS){
		RT_TRACE(COMP_INIT, "phy_BB8192C_Config_ParaFile():AGC Table Fail\n");
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	priv->bCckHighPower = (bool)(PHY_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, 0x200));


phy_BB8190_Config_ParaFile_Fail:	
	
	return rtStatus;
}

#if !RTL8190_Download_Firmware_From_Header
/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigMACWithParaFile()
 *
 * Overview:    This function read BB parameters from general file format, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		Adapter
 *			ps1Byte 			pFileName			
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note: 		The format of MACPHY_REG.txt is different from PHY and RF. 
 *			[Register][Mask][Value]
 *---------------------------------------------------------------------------*/
static	RT_STATUS
phy_ConfigMACWithParaFile(
	struct net_device* 	dev,
	u8 				pFileName
)
{
	u32*			ptrArray;	
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	u32				u4bRegOffset, u4bRegValue;
	u32				ithLine=0;

	if(pFileName == RTL8192C_PHY_MACREG){
		ptrArray = AGC_TAB_ARRY;
	} else if (pFileName == RTL8188C_PHY_MACREG){
		ptrArray = AGC_TAB_ARRY;
	}else{
		return RT_STATUS_FAILURE;
	}

#if 0
	if(ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT))
	{
		rtStatus = PlatformReadFile(
						Adapter, 
						pFileName,
						(pu1Byte)(pHalData->BufOfLines),
						MAX_LINES_HWCONFIG_TXT,
						MAX_BYTES_LINE_HWCONFIG_TXT,
						&nLinesRead
						);

		if(rtStatus == RT_STATUS_SUCCESS)
		{	
			PlatformMoveMemory(pHalData->BufOfLines3, pHalData->BufOfLines, nLinesRead*MAX_BYTES_LINE_HWCONFIG_TXT);
			pHalData->nLinesRead3 = nLinesRead;
		}
		else
		{
			pHalData->nLinesRead3 = 0;
			RT_TRACE(COMP_INIT, DBG_LOUD, ("phy_ConfigMACWithParaFile(): File %s is not present!!\n", pFileName));
			return RT_STATUS_SUCCESS;
		}

	}
	else
	{	
		PlatformMoveMemory(pHalData->BufOfLines, pHalData->BufOfLines3, MAX_LINES_HWCONFIG_TXT*MAX_BYTES_LINE_HWCONFIG_TXT);
		nLinesRead = pHalData->nLinesRead3;
		rtStatus = RT_STATUS_SUCCESS;
	}
#else

	if(rtStatus == RT_STATUS_SUCCESS)
	{

		for(ithLine = 0; ; ithLine=ithLine+2)
		{
		
			{
				u4bRegOffset = ptrArray[ithLine];
				{
					if(u4bRegOffset == 0xff)
					{ 
                                            printk("###################>%s Ending\n", __func__);
						break;
					}
					
					u4bRegValue = ptrArray[ithLine+1];
					{						
						write_nic_byte(dev, u4bRegOffset, (u8)u4bRegValue);
					}
				}
				
			}
			
		}
		
	}
	else
	{
		;
	}
#endif



	return rtStatus;
}
#endif

/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigMACWithHeaderFile()
 *
 * Overview:    This function read BB parameters from Header file we gen, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		Adapter
 *			char* 			pFileName			
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note: 		The format of MACPHY_REG.txt is different from PHY and RF. 
 *			[Register][Mask][Value]
 *---------------------------------------------------------------------------*/
static	RT_STATUS
phy_ConfigMACWithHeaderFile(
	struct net_device* dev
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32					i = 0;
	u32					ArrayLength = 0;
	u32*					ptrArray;	
	
	RT_TRACE(COMP_INIT, "Read Rtl819XMACPHY_Array\n");


	if(IS_NORMAL_CHIP(priv->card_8192_version))
	{
		ArrayLength = MAC_2T_ArrayLength;
		ptrArray = Rtl819XMAC_Array;			
		RT_TRACE(COMP_INIT, " ===> phy_ConfigMACWithHeaderFile() Img:Rtl819XMAC_Array\n");
	}
	else
	{
		ArrayLength = Rtl8192CTestMAC_2TLen;
		ptrArray = Rtl8192CTestMAC_2T;
		RT_TRACE(COMP_INIT, " ===> phy_ConfigMACWithHeaderFile() Img:Rtl8192CTestMAC_2T\n");
	}		

	for(i = 0 ;i < ArrayLength;i=i+2){ 
		write_nic_byte(dev, ptrArray[i], (u8)ptrArray[i+1]);
	}

	return RT_STATUS_SUCCESS;
}

#if !RTL8190_Download_Firmware_From_Header
/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigBBWithParaFile()
 *
 * Overview:    This function read BB parameters from general file format, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		Adapter
 *			ps1Byte 			pFileName			
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *	2008/11/06	MH	For 92S we do not support silent reset now. Disable 
 *					parameter file compare!!!!!!??
 *			
 *---------------------------------------------------------------------------*/
static	RT_STATUS
phy_ConfigBBWithParaFile(
	struct net_device* dev,
	u8 			pFileName
)
{
	u32*			ptrArray;	
	long				ithLine;
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	u32				u4bRegOffset, u4bRegValue;



	if(pFileName == RTL819X_PHY_REG){
		ptrArray = PHY_REG_ARRY;
	}else if(pFileName == RTL819X_AGC_TAB){
		ptrArray = AGC_TAB_ARRY;
	}else{
		return RT_STATUS_FAILURE;
	}

#if 0
	if(ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT))
	{
		rtStatus = PlatformReadFile(
						Adapter, 
						pFileName,
						(pu1Byte)(pHalData->BufOfLines),
						MAX_LINES_HWCONFIG_TXT,
						MAX_BYTES_LINE_HWCONFIG_TXT,
						&nLinesRead
						);
#if 1	
		if(rtStatus == RT_STATUS_SUCCESS)
		{
		
			if( (PlatformCompareMemory(szBBRegFile, pFileName, phy_PHYREG_LEN) == 0 ) ||
				(PlatformCompareMemory(szBBRegFile1T2R, pFileName, phy_PHYREG1T2R_LEN) == 0 ) )
			{
				PlatformMoveMemory(pHalData->BufOfLines1, pHalData->BufOfLines, nLinesRead*MAX_BYTES_LINE_HWCONFIG_TXT);
				pHalData->nLinesRead1 = nLinesRead;
			}
			else if(PlatformCompareMemory(szAGCTableFile, pFileName, phy_PHYREG_LEN) == 0)
			{
				PlatformMoveMemory(pHalData->BufOfLines2, pHalData->BufOfLines, nLinesRead*MAX_BYTES_LINE_HWCONFIG_TXT);
				pHalData->nLinesRead2 = nLinesRead;
			}
			else
			{
				RT_TRACE(COMP_INIT, DBG_LOUD, ("No matched file \r\n"));
			}
		}
#endif
	}
	else
	{
		if( (PlatformCompareMemory(szBBRegFile, pFileName, phy_PHYREG_LEN) == 0 ) ||
			(PlatformCompareMemory(szBBRegFile1T2R, pFileName, phy_PHYREG1T2R_LEN) == 0 ) )
		{
			PlatformMoveMemory(pHalData->BufOfLines, pHalData->BufOfLines1, MAX_LINES_HWCONFIG_TXT*MAX_BYTES_LINE_HWCONFIG_TXT);
			nLinesRead = pHalData->nLinesRead1;
			rtStatus = RT_STATUS_SUCCESS;
		}
		else if(PlatformCompareMemory(szAGCTableFile, pFileName, phy_PHYREG_LEN) == 0)
		{
			PlatformMoveMemory(pHalData->BufOfLines, pHalData->BufOfLines2, MAX_LINES_HWCONFIG_TXT*MAX_BYTES_LINE_HWCONFIG_TXT);
			nLinesRead = pHalData->nLinesRead2;
			rtStatus = RT_STATUS_SUCCESS;
		}
		else
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("No matched file \r\n"));
		}
	}
#endif

	if(rtStatus == RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, "phy_ConfigBBWithParaFile(): read %d OK\n", pFileName);

		for(ithLine = 0; ; ithLine=ithLine+2)
		{
			u4bRegOffset = ptrArray[ithLine];

			{
				{
					if(u4bRegOffset == 0xff)
					{ 
                                            printk("###################>%s Ending\n", __func__);
						break;
					} else if (u4bRegOffset == 0xfe){
						mdelay(50);
						continue;
					} else if (u4bRegOffset == 0xfd){
						mdelay(5);
						continue;
					} else if (u4bRegOffset == 0xfc){
						mdelay(1);
						continue;
					} else if (u4bRegOffset == 0xfb){
						udelay(50);
						continue;
					} else if (u4bRegOffset == 0xfa){
						udelay(5);
						continue;
					} else if (u4bRegOffset == 0xf9){
						udelay(1);
						continue;
					}

					u4bRegValue = ptrArray[ithLine+1];
					{
						PHY_SetBBReg(dev, u4bRegOffset, bMaskDWord, u4bRegValue);
						
						udelay(1);
					}
				}
			}
		}
		phy_ConfigBBExternalPA(dev);
	}
	else
	{
		RT_TRACE(COMP_INIT, "phy_ConfigBBWithParaFile(): Failed to read %d!\n", pFileName);
	}

	return rtStatus;	
}
#endif

#if !RTL8190_Download_Firmware_From_Header
/*-----------------------------------------------------------------------------
 * Function:    phy_SetBBtoDiffRFWithParaFile()
 *
 * Overview:    This function read BB parameters from general file format, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		Adapter
 *			ps1Byte 			pFileName			
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *
 * 2008/11/10	tynli	
 * 2009/07/29	tynli (porting from 92SE branch) Add copy parameter file to buffer for silent reset
 *			
 *---------------------------------------------------------------------------*/
static	RT_STATUS
phy_SetBBtoDiffRFWithParaFile(
	struct net_device* dev,
	u8 			pFileName
)
{
    u32*			ptrArray;
    long			ithLine;
    RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
    u32				u4bRegOffset, u4bRegMask, u4bRegValue;




	if(pFileName == RTL819X_PHY_REG_to1T1R)
		ptrArray = phy_to1T1R_a_ARRY;
	else if(pFileName == RTL819X_PHY_REG_to1T2R)
		ptrArray = phy_to1T2R_ARRY;
	else
		return RT_STATUS_FAILURE;
	
#if 0
	if(ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT))
	{
		rtStatus = PlatformReadFile(
						Adapter, 
						pFileName,
						(pu1Byte)(pHalData->BufOfLines),
						MAX_LINES_HWCONFIG_TXT,
						MAX_BYTES_LINE_HWCONFIG_TXT,
						&nLinesRead
						);
		
		if(rtStatus == RT_STATUS_SUCCESS)
		{
		
			if( (PlatformCompareMemory(szBBRegto1T1RFile, pFileName, phy_PHYREG_TO1T1R_LEN) == 0 ) ||
				(PlatformCompareMemory(szBBRegto1T2RFile, pFileName, phy_PHYREG_TO1T2R_LEN) == 0 ) )
			{
				PlatformMoveMemory(pHalData->BufOfLines4, pHalData->BufOfLines, nLinesRead*MAX_BYTES_LINE_HWCONFIG_TXT);
				pHalData->nLinesRead4 = nLinesRead;
			}
			else
			{
				RT_TRACE(COMP_INIT, DBG_LOUD, ("No matched file \r\n"));
			}
		}
	}
	else
	{
	
		if( (PlatformCompareMemory(szBBRegto1T1RFile, pFileName, phy_PHYREG_TO1T1R_LEN) == 0 ) ||
			(PlatformCompareMemory(szBBRegto1T2RFile, pFileName, phy_PHYREG_TO1T2R_LEN) == 0 ) )
		{
			PlatformMoveMemory(pHalData->BufOfLines, pHalData->BufOfLines4, MAX_LINES_HWCONFIG_TXT*MAX_BYTES_LINE_HWCONFIG_TXT);
			nLinesRead = pHalData->nLinesRead4;
			rtStatus = RT_STATUS_SUCCESS;
		}
		else
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("No matched file \r\n"));
		}
	}
#endif

	if(rtStatus == RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, "phy_SetBBtoDiffRFWithParaFile(): read %d OK\n", pFileName);

		for(ithLine = 0;  ; ithLine=ithLine+3)
		{
			u4bRegOffset = ptrArray[ithLine];

			{
				{
					if(u4bRegOffset == 0xff)
					{ 
                                            printk("###################>%s Ending\n", __func__);
						break;
					}
					else if (u4bRegOffset == 0xfe){
						mdelay(50);
						continue;
					}else if (u4bRegOffset == 0xfd){
						mdelay(5);
						continue;
					}else if (u4bRegOffset == 0xfc){
						mdelay(1);
						continue;
					}else if (u4bRegOffset == 0xfb){
						udelay(50);
						continue;
					}else if (u4bRegOffset == 0xfa){
						udelay(5);
						continue;
					}else if (u4bRegOffset == 0xf9){
						udelay(1);
						continue;
					}
					
					u4bRegMask = ptrArray[ithLine+1];
					
					u4bRegValue = ptrArray[ithLine+2];
					{
						printk("[ADDR] %08x=%08x Mask=%08x\n", u4bRegOffset, u4bRegValue, u4bRegMask);
						PHY_SetBBReg(dev, u4bRegOffset, u4bRegMask, u4bRegValue);
					}
				}
			}
		}
	}
	else
	{
		RT_TRACE(COMP_INIT, "phy_ConfigBBWithParaFile(): Failed to read %d!\n", pFileName);
	}

	return rtStatus;	
}
#endif

void
phy_ConfigBBExternalPA(
	struct net_device* dev
)
{
#if 0
	u32 temp=0;

#ifdef MERGE_TO_DO
	if(!priv->ExternalPA)
	{
		return;
	}
#endif

	PHY_SetBBReg(dev, 0xee8, BIT28, 1);
	temp = PHY_QueryBBReg(dev, 0x860, bMaskDWord);
	temp |= (BIT26|BIT21|BIT10|BIT5);
	PHY_SetBBReg(dev, 0x860, bMaskDWord, temp);
	PHY_SetBBReg(dev, 0x870, BIT10, 0);
	PHY_SetBBReg(dev, 0xc80, bMaskDWord, 0x20000080);
	PHY_SetBBReg(dev, 0xc88, bMaskDWord, 0x40000100);
#endif
}

/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigBBWithHeaderFile()
 *
 * Overview:    This function read BB parameters from general file format, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		Adapter
 *			u8 			ConfigType     0 => PHY_CONFIG
 *										 1 =>AGC_TAB
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 *---------------------------------------------------------------------------*/
static	RT_STATUS
phy_ConfigBBWithHeaderFile(
	struct net_device* dev,
	u8 			ConfigType
)
{
	int i;
	u32*	Rtl819XPHY_REGArray_Table;
	u32*	Rtl819XAGCTAB_Array_Table;
	u16	PHY_REGArrayLen, AGCTAB_ArrayLen;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
	if(IS_92C_SERIAL(priv->card_8192_version))
	{
		if(IS_NORMAL_CHIP(priv->card_8192_version))
		{
			AGCTAB_ArrayLen = AGCTAB_2TArrayLength;
			Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_2TArray;
			PHY_REGArrayLen = PHY_REG_2TArrayLength;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_2TArray;
		}
		else
		{
			AGCTAB_ArrayLen = Rtl8192CTestAGCTAB_2TLen;
			Rtl819XAGCTAB_Array_Table = Rtl8192CTestAGCTAB_2T;
			PHY_REGArrayLen = Rtl8192CTestPHY_REG_2TLen;
			Rtl819XPHY_REGArray_Table = Rtl8192CTestPHY_REG_2T;
		}
	}
	else
	{
		if(IS_NORMAL_CHIP(priv->card_8192_version))
		{
			AGCTAB_ArrayLen = AGCTAB_1TArrayLength;
			Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_1TArray;
			PHY_REGArrayLen = PHY_REG_1TArrayLength;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_1TArray;
		}
		else
		{
			AGCTAB_ArrayLen = Rtl8192CTestAGCTAB_1TLen;
			Rtl819XAGCTAB_Array_Table = Rtl8192CTestAGCTAB_1T;
			PHY_REGArrayLen = Rtl8192CTestPHY_REG_1TLen;
			Rtl819XPHY_REGArray_Table = Rtl8192CTestPHY_REG_1T;
		}
	}

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArrayLen;i=i+2)
		{
			if (Rtl819XPHY_REGArray_Table[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xf9)
				udelay(1);
			PHY_SetBBReg(dev, Rtl819XPHY_REGArray_Table[i], bMaskDWord, Rtl819XPHY_REGArray_Table[i+1]);		

			udelay(1);
			
		}
		phy_ConfigBBExternalPA(dev);
	}
	else if(ConfigType == BaseBand_Config_AGC_TAB)
	{
		for(i=0;i<AGCTAB_ArrayLen;i=i+2)
		{
			PHY_SetBBReg(dev, Rtl819XAGCTAB_Array_Table[i], bMaskDWord, Rtl819XAGCTAB_Array_Table[i+1]);		

			udelay(1);
			
		}
	}
	return RT_STATUS_SUCCESS;
}

#if !RTL8190_Download_Firmware_From_Header
/*-----------------------------------------------------------------------------
 * Function:    phy_SetBBtoDiffRFWithHeaderFile()
 *
 * Overview:    This function 
 *			
 *
 * Input:      	PADAPTER		Adapter
 *			u8 			ConfigType     0 => PHY_CONFIG
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 * When			Who		Remark
 * 2008/11/10	tynli
 *			
 *---------------------------------------------------------------------------*/
static	RT_STATUS
phy_SetBBtoDiffRFWithHeaderFile(
	struct net_device* dev,
	u8 			ConfigType
)
{
#if 0
	int i;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32* 			Rtl819XPHY_REGArraytoXTXR_Table;
	u16			PHY_REGArraytoXTXRLen;
	

	if(GET_RF_TYPE(priv) == RF_1T1R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T1R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T1RArrayLength;
		RT_TRACE(COMP_INIT, "phy_SetBBtoDiffRFWithHeaderFile: Set to 1T1R..\n");
	} 
	else if(GET_RF_TYPE(priv) == RF_1T2R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T2R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T2RArrayLength;
		RT_TRACE(COMP_INIT, "phy_SetBBtoDiffRFWithHeaderFile: Set to 1T2R..\n");
	}
	else
	{
		return RT_STATUS_FAILURE;
	}

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArraytoXTXRLen;i=i+3)
		{
			if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArraytoXTXR_Table[i] == 0xf9)
				udelay(1);
			PHY_SetBBReg(dev, Rtl819XPHY_REGArraytoXTXR_Table[i], Rtl819XPHY_REGArraytoXTXR_Table[i+1], Rtl819XPHY_REGArraytoXTXR_Table[i+2]);		
			RT_TRACE(COMP_INIT,  
			"The Rtl819XPHY_REGArraytoXTXR_Table[0] is %x Rtl819XPHY_REGArraytoXTXR_Table[1] is %x Rtl819XPHY_REGArraytoXTXR_Table[2] is %x \n",
			Rtl819XPHY_REGArraytoXTXR_Table[i],Rtl819XPHY_REGArraytoXTXR_Table[i+1], Rtl819XPHY_REGArraytoXTXR_Table[i+2]);
		}
	}
	else {
		RT_TRACE(COMP_INIT, "phy_SetBBtoDiffRFWithHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
#endif
	return RT_STATUS_SUCCESS;
}
#endif

void
storePwrIndexDiffRateOffset(
	struct net_device* dev,
	u32		RegAddr,
	u32		BitMask,
	u32		Data
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
	if(RegAddr == rTxAGC_A_Rate18_06)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][0] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][0] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][0]);
	}
	if(RegAddr == rTxAGC_A_Rate54_24)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][1] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][1] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][1]);
	}
	if(RegAddr == rTxAGC_A_CCK1_Mcs32)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][6] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][6] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][6]);
	}
	if(RegAddr == rTxAGC_B_CCK11_A_CCK2_11 && BitMask == 0xffffff00)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][7] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][7] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][7]);
	}
	if(RegAddr == rTxAGC_A_Mcs03_Mcs00)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][2] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][2] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][2]);
	}
	if(RegAddr == rTxAGC_A_Mcs07_Mcs04)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][3] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][3] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][3]);
	}
	if(RegAddr == rTxAGC_A_Mcs11_Mcs08)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][4] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][4] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][4]);
	}
	if(RegAddr == rTxAGC_A_Mcs15_Mcs12)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][5] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][5] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][5]);
	}
	if(RegAddr == rTxAGC_B_Rate18_06)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][8] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][8] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][8]);
	}
	if(RegAddr == rTxAGC_B_Rate54_24)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][9] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][9] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][9]);
	}
	if(RegAddr == rTxAGC_B_CCK1_55_Mcs32)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][14] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][14] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][14]);
	}
	if(RegAddr == rTxAGC_B_CCK11_A_CCK2_11 && BitMask == 0x000000ff)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][15] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][15] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][15]);
	}	
	if(RegAddr == rTxAGC_B_Mcs03_Mcs00)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][10] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][10] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][10]);
	}
	if(RegAddr == rTxAGC_B_Mcs07_Mcs04)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][11] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][11] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][11]);
	}
	if(RegAddr == rTxAGC_B_Mcs11_Mcs08)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][12] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][12] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][12]);
	}
	if(RegAddr == rTxAGC_B_Mcs15_Mcs12)
	{
		priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][13] = Data;
		RT_TRACE(COMP_INIT, "MCSTxPowerLevelOriginalOffset[%d][13] = 0x%x\n", priv->pwrGroupCnt,
			priv->MCSTxPowerLevelOriginalOffset[priv->pwrGroupCnt][13]);

		priv->pwrGroupCnt++;
	}
}

/*-----------------------------------------------------------------------------
 * Function:	phy_ConfigBBWithPgHeaderFile
 *
 * Overview:	Config PHY_REG_PG array 
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/06/2008 	MHC		Add later!!!!!!.. Please modify for new files!!!!
 * 11/10/2008	tynli		Modify to mew files.
 *---------------------------------------------------------------------------*/
static	RT_STATUS
phy_ConfigBBWithPgHeaderFile(
	struct net_device* dev,
	u8 			ConfigType)
{
	int i;
	u32*	Rtl819XPHY_REGArray_Table_PG;
	u16	PHY_REGArrayPGLen;
	struct r8192_priv 	*priv = rtllib_priv(dev);

	if(IS_NORMAL_CHIP(priv->card_8192_version))
	{	
		PHY_REGArrayPGLen = PHY_REG_Array_PGLength;
		Rtl819XPHY_REGArray_Table_PG = Rtl819XPHY_REG_Array_PG;
	}
	else
	{
		PHY_REGArrayPGLen = Rtl8192CTestPHY_REG_PGLen;
		Rtl819XPHY_REGArray_Table_PG = Rtl8192CTestPHY_REG_PG;
	}

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArrayPGLen;i=i+3)
		{
			if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfe)
				mdelay(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfd)
				mdelay(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfc)
				mdelay(1);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfb)
				udelay(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfa)
				udelay(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xf9)
				udelay(1);
			storePwrIndexDiffRateOffset(dev, Rtl819XPHY_REGArray_Table_PG[i], 
				Rtl819XPHY_REGArray_Table_PG[i+1], 
				Rtl819XPHY_REGArray_Table_PG[i+2]);
		}
	}
	else
	{

		RT_TRACE(COMP_SEND, "phy_ConfigBBWithPgHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
	return RT_STATUS_SUCCESS;
	
}	/* phy_ConfigBBWithPgHeaderFile */

/*-----------------------------------------------------------------------------
 * Function:    PHY_ConfigRFWithParaFile()
 *
 * Overview:    This function read RF parameters from general file format, and do RF 3-wire
 *
 * Input:      	PADAPTER			Adapter
 *			ps1Byte 				pFileName			
 *			RF90_RADIO_PATH_E	eRFPath
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note:		Delay may be required for RF configuration
 *---------------------------------------------------------------------------*/
#if !RTL8190_Download_Firmware_From_Header
bool
PHY_ConfigRFWithParaFile(
	struct net_device* 		dev,
	u8 					pFileName,
	RF90_RADIO_PATH_E	eRFPath
)
{
	long			ithLine;
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	u32				u4bRegOffset, u4bRegValue;

	u32*			ptrArray;	



	switch(pFileName)
	{
	case RTL819X_PHY_RADIO_A:
		ptrArray = radio_a_ARRY;
		break;
	case RTL819X_PHY_RADIO_B:
		ptrArray = radio_b_ARRY;
		break;
	case RTL819X_PHY_RADIO_B_GM:
		ptrArray = radio_b_gm_ARRY;
		break;
	default:
		return false;
		break;			
	}

#if 0
	switch(eRFPath)
	{
	case RF90_PATH_A:
		pBuf = (ps1Byte)pHalData->BufRadioA;
		pBufLen = &pHalData->nLinesBufRadioA;
		break;
	case RF90_PATH_B:
		pBuf = (ps1Byte)pHalData->BufRadioB;
		pBufLen = &pHalData->nLinesBufRadioB;
		break;
	case RF90_PATH_C:
		pBuf = (ps1Byte)pHalData->BufRadioC;
		pBufLen = &pHalData->nLinesBufRadioC;
		break;
	case RF90_PATH_D:
		pBuf = (ps1Byte)pHalData->BufRadioD;
		pBufLen = &pHalData->nLinesBufRadioD;
		break;
	default:
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Unknown RF path!! %d\r\n", eRFPath));
		break;			
	}

	if(ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT))
	{
		rtStatus = PlatformReadFile(
						Adapter, 
						pFileName,
						(pu1Byte)(pHalData->BufOfLines),
						MAX_LINES_HWCONFIG_TXT,
						MAX_BYTES_LINE_HWCONFIG_TXT,
						&nLinesRead
						);
		if(rtStatus == RT_STATUS_SUCCESS)
		{
			PlatformMoveMemory(pBuf, pHalData->BufOfLines, nLinesRead*MAX_BYTES_LINE_HWCONFIG_TXT);
			*pBufLen = nLinesRead;
		}
	}
	else
	{
		PlatformMoveMemory(pHalData->BufOfLines, pBuf, MAX_LINES_HWCONFIG_TXT*MAX_BYTES_LINE_HWCONFIG_TXT);
		nLinesRead = *pBufLen;
		rtStatus = RT_STATUS_SUCCESS;
	}
#endif

	if(rtStatus == RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, "phy_RF8225_Config_ParaFile(): read %d successfully\n", pFileName);

		
		for(ithLine = 0; ; ithLine=ithLine+2)
		{
			u4bRegOffset = ptrArray[ithLine];

			{
				{
					if(u4bRegOffset == 0xff)	{ 
                                            printk("###################>%s Ending\n", __func__);
						break;
					}else if(u4bRegOffset == 0xfe){ 
						mdelay(50);
						continue;
					}else if (u4bRegOffset == 0xfd){
						mdelay(5);
						continue;
					}else if (u4bRegOffset == 0xfc){
						mdelay(1);
						continue;
					}else if (u4bRegOffset == 0xfb){
						udelay(50);
						continue;
					}else if (u4bRegOffset == 0xfa){
						udelay(5);
						continue;
					}else if (u4bRegOffset == 0xf9){
						udelay(1);
						continue;
					}
					
					u4bRegValue = ptrArray[ithLine+1];
					{
						PHY_SetRFReg(dev, eRFPath, u4bRegOffset, bRFRegOffsetMask, u4bRegValue);
						mdelay(1);
					}
				}
			}
		}
		PHY_ConfigRFExternalPA(dev, eRFPath);
	}
	else
	{
	}

	return true;
	
}
#endif

#define HighPowerRadioAArrayLen 22
u32 Rtl8192S_HighPower_RadioA_Array[HighPowerRadioAArrayLen] = {
0x013,0x00029ea4,
0x013,0x00025e74,
0x013,0x00020ea4,
0x013,0x0001ced0,
0x013,0x00019f40,
0x013,0x00014e70,
0x013,0x000106a0,
0x013,0x0000c670,
0x013,0x000082a0,
0x013,0x00004270,
0x013,0x00000240,
};

RT_STATUS
PHY_ConfigRFExternalPA(
	struct net_device* dev,
	RF90_RADIO_PATH_E		eRFPath
)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
#if 0
	u16 i=0;

#ifdef MERGE_TO_DO
	if(!priv->ExternalPA)
	{
		return rtStatus;
	}
#endif

	for(i = 0;i<HighPowerRadioAArrayLen; i=i+2)
	{
		RT_TRACE(COMP_INIT, "External PA, write RF 0x%x=0x%x\n", Rtl8192S_HighPower_RadioA_Array[i], Rtl8192S_HighPower_RadioA_Array[i+1]);
		PHY_SetRFReg(dev, eRFPath, Rtl8192S_HighPower_RadioA_Array[i], bRFRegOffsetMask, Rtl8192S_HighPower_RadioA_Array[i+1]);
	}
#endif
	return rtStatus;
}
/*-----------------------------------------------------------------------------
 * Function:    PHY_ConfigRFWithHeaderFile()
 *
 * Overview:    This function read RF parameters from general file format, and do RF 3-wire
 *
 * Input:      	PADAPTER			Adapter
 *			char* 				pFileName			
 *			RF90_RADIO_PATH_E	eRFPath
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note:		Delay may be required for RF configuration
 *---------------------------------------------------------------------------*/
bool
PHY_ConfigRFWithHeaderFile(
	struct net_device* dev,
	RF90_RADIO_PATH_E		eRFPath
)
{

	int			i;
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	u32*		Rtl819XRadioA_Array_Table;
	u32*		Rtl819XRadioB_Array_Table;
	u16		RadioA_ArrayLen,RadioB_ArrayLen;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
	if(IS_92C_SERIAL(priv->card_8192_version))
	{
		if(IS_NORMAL_CHIP(priv->card_8192_version))
		{
			RadioA_ArrayLen = RadioA_2TArrayLength;
			Rtl819XRadioA_Array_Table=Rtl819XRadioA_2TArray;
			RadioB_ArrayLen = RadioB_2TArrayLength;	
			Rtl819XRadioB_Array_Table = Rtl819XRadioB_2TArray;
			RT_TRACE(COMP_INIT, " ===> PHY_ConfigRFWithHeaderFile() Radio_A:Rtl819XRadioA_2TArray\n");
			RT_TRACE(COMP_INIT, " ===> PHY_ConfigRFWithHeaderFile() Radio_B:Rtl819XRadioB_2TArray\n");
		}
		else
		{
			RadioA_ArrayLen = Rtl8192CTestRadioA_2TLen;
			Rtl819XRadioA_Array_Table=Rtl8192CTestRadioA_2T;
			RadioB_ArrayLen = Rtl8192CTestRadioB_2TLen;	
			Rtl819XRadioB_Array_Table = Rtl8192CTestRadioB_2T;
			RT_TRACE(COMP_INIT, " ===> PHY_ConfigRFWithHeaderFile() Radio_A:Rtl8192CTestRadioA_2T\n");
			RT_TRACE(COMP_INIT, " ===> PHY_ConfigRFWithHeaderFile() Radio_B:Rtl8192CTestRadioB_2T\n");
		}
	}
	else
	{
		if(IS_NORMAL_CHIP(priv->card_8192_version))
		{
			RadioA_ArrayLen = RadioA_1TArrayLength;
			Rtl819XRadioA_Array_Table=Rtl819XRadioA_1TArray;
			RadioB_ArrayLen = RadioB_1TArrayLength;	
			Rtl819XRadioB_Array_Table = Rtl819XRadioB_1TArray;
			RT_TRACE(COMP_INIT, " ===> PHY_ConfigRFWithHeaderFile() Radio_A:Rtl819XRadioA_1TArray\n");
			RT_TRACE(COMP_INIT, " ===> PHY_ConfigRFWithHeaderFile() Radio_B:Rtl819XRadioB_1TArray\n");
		}
		else
		{
			RadioA_ArrayLen = Rtl8192CTestRadioA_1TLen;
			Rtl819XRadioA_Array_Table=Rtl8192CTestRadioA_1T;
			RadioB_ArrayLen = Rtl8192CTestRadioB_1TLen;	
			Rtl819XRadioB_Array_Table = Rtl8192CTestRadioB_1T;
			RT_TRACE(COMP_INIT, " ===> PHY_ConfigRFWithHeaderFile() Radio_A:Rtl8192CTestRadioA_1T\n");
			RT_TRACE(COMP_INIT, " ===> PHY_ConfigRFWithHeaderFile() Radio_B:Rtl8192CTestRadioB_1T\n");
		}
	}

	RT_TRACE(COMP_INIT, "PHY_ConfigRFWithHeaderFile: Radio No %x\n", eRFPath);
	rtStatus = RT_STATUS_SUCCESS;

	switch(eRFPath){
		case RF90_PATH_A:
			for(i = 0;i<RadioA_ArrayLen; i=i+2)
			{
				if(Rtl819XRadioA_Array_Table[i] == 0xfe)
					mdelay(50);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfd)
					mdelay(5);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfc)
					mdelay(1);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfb)
					udelay(50);
				else if (Rtl819XRadioA_Array_Table[i] == 0xfa)
					udelay(5);
				else if (Rtl819XRadioA_Array_Table[i] == 0xf9)
					udelay(1);
				else
				{
					PHY_SetRFReg(dev, eRFPath, Rtl819XRadioA_Array_Table[i], bRFRegOffsetMask, Rtl819XRadioA_Array_Table[i+1]);
					udelay(1);
				}
			}			
			PHY_ConfigRFExternalPA(dev, eRFPath);
			break;
		case RF90_PATH_B:
			for(i = 0;i<RadioB_ArrayLen; i=i+2)
			{
				if(Rtl819XRadioB_Array_Table[i] == 0xfe)
				{ 
#if 0
					mdelay(1000);
#else
					mdelay(50);
#endif
				}
				else if (Rtl819XRadioB_Array_Table[i] == 0xfd)
					mdelay(5);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfc)
					mdelay(1);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfb)
					udelay(50);
				else if (Rtl819XRadioB_Array_Table[i] == 0xfa)
					udelay(5);
				else if (Rtl819XRadioB_Array_Table[i] == 0xf9)
					udelay(1);
				else
				{
					PHY_SetRFReg(dev, eRFPath, Rtl819XRadioB_Array_Table[i], bRFRegOffsetMask, Rtl819XRadioB_Array_Table[i+1]);
					udelay(1);
				}	
			}			
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
	}
	
	return 1;

}


/*-----------------------------------------------------------------------------
 * Function:    PHY_CheckBBAndRFOK()
 *
 * Overview:    This function is write register and then readback to make sure whether
 *			  BB[PHY0, PHY1], RF[Patha, path b, path c, path d] is Ok
 *
 * Input:      	PADAPTER			Adapter
 *			HW90_BLOCK_E		CheckBlock
 *			RF90_RADIO_PATH_E	eRFPath		
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: PHY is OK
 *			
 * Note:		This function may be removed in the ASIC
 *---------------------------------------------------------------------------*/
RT_STATUS
PHY_CheckBBAndRFOK(
	struct net_device* dev,
	HW90_BLOCK_E		CheckBlock,
	RF90_RADIO_PATH_E	eRFPath
	)
{
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;

	u32				i, CheckTimes = 4,ulRegRead = 0;

	u32				WriteAddr[4];
	u32				WriteData[] = {0xfffff027, 0xaa55a02f, 0x00000027, 0x55aa502f};

	WriteAddr[HW90_BLOCK_MAC] = 0x100;
	WriteAddr[HW90_BLOCK_PHY0] = 0x900;
	WriteAddr[HW90_BLOCK_PHY1] = 0x800;
	WriteAddr[HW90_BLOCK_RF] = 0x3;
	
	for(i=0 ; i < CheckTimes ; i++)
	{

		switch(CheckBlock)
		{
		case HW90_BLOCK_MAC:
			RT_TRACE(COMP_INIT, "PHY_CheckBBRFOK(): Never Write 0x100 here!\n");
			break;
			
		case HW90_BLOCK_PHY0:
		case HW90_BLOCK_PHY1:
			write_nic_dword(dev, WriteAddr[CheckBlock], WriteData[i]);
			ulRegRead = read_nic_dword(dev, WriteAddr[CheckBlock]);
			break;

		case HW90_BLOCK_RF:
			WriteData[i] &= 0xfff;
			PHY_SetRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bRFRegOffsetMask, WriteData[i]);
			mdelay(10);
			ulRegRead = PHY_QueryRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bMaskDWord);				
			mdelay(10);
			break;
			
		default:
			rtStatus = RT_STATUS_FAILURE;
			break;
		}


		if(ulRegRead != WriteData[i])
		{
			RT_TRACE(COMP_INIT, "ulRegRead: %x, WriteData: %x \n", ulRegRead, WriteData[i]);
			rtStatus = RT_STATUS_FAILURE;			
			break;
		}
	}

	return rtStatus;
}

extern	void
PHY_GetHWRegOriginalValue(
	struct net_device* dev
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
	priv->DefaultInitialGain[0] = (u8)PHY_QueryBBReg(dev, rOFDM0_XAAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[1] = (u8)PHY_QueryBBReg(dev, rOFDM0_XBAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[2] = (u8)PHY_QueryBBReg(dev, rOFDM0_XCAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[3] = (u8)PHY_QueryBBReg(dev, rOFDM0_XDAGCCore1, bMaskByte0);
	RT_TRACE(COMP_INIT, 
	"Default initial gain (c50=0x%x, c58=0x%x, c60=0x%x, c68=0x%x \n", 
	priv->DefaultInitialGain[0], priv->DefaultInitialGain[1], 
	priv->DefaultInitialGain[2], priv->DefaultInitialGain[3]);

	priv->framesync = (u8)PHY_QueryBBReg(dev, rOFDM0_RxDetector3, bMaskByte0);	 
	priv->framesyncC34 = PHY_QueryBBReg(dev, rOFDM0_RxDetector2, bMaskDWord);
	RT_TRACE(COMP_INIT, "Default framesync (0x%x) = 0x%x \n", 
		rOFDM0_RxDetector3, priv->framesync);
}



/**
* Function:	phy_InitBBRFRegisterDefinition
*
* OverView:	Initialize Register definition offset for Radio Path A/B/C/D
*
* Input:
*			PADAPTER		Adapter,
*
* Output:	None
* Return:		None
* Note:		The initialization value is constant and it should never be changes
*/
static	void
phy_InitBBRFRegisterDefinition(
	struct net_device* dev
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);

	priv->PHYRegDef[RF90_PATH_A].rfintfs = rFPGA0_XAB_RFInterfaceSW; 
	priv->PHYRegDef[RF90_PATH_B].rfintfs = rFPGA0_XAB_RFInterfaceSW; 
	priv->PHYRegDef[RF90_PATH_C].rfintfs = rFPGA0_XCD_RFInterfaceSW;
	priv->PHYRegDef[RF90_PATH_D].rfintfs = rFPGA0_XCD_RFInterfaceSW;

	priv->PHYRegDef[RF90_PATH_A].rfintfi = rFPGA0_XAB_RFInterfaceRB; 
	priv->PHYRegDef[RF90_PATH_B].rfintfi = rFPGA0_XAB_RFInterfaceRB;
	priv->PHYRegDef[RF90_PATH_C].rfintfi = rFPGA0_XCD_RFInterfaceRB;
	priv->PHYRegDef[RF90_PATH_D].rfintfi = rFPGA0_XCD_RFInterfaceRB;

	priv->PHYRegDef[RF90_PATH_A].rfintfo = rFPGA0_XA_RFInterfaceOE; 
	priv->PHYRegDef[RF90_PATH_B].rfintfo = rFPGA0_XB_RFInterfaceOE; 

	priv->PHYRegDef[RF90_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE; 
	priv->PHYRegDef[RF90_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE; 

	priv->PHYRegDef[RF90_PATH_A].rf3wireOffset = rFPGA0_XA_LSSIParameter; 
	priv->PHYRegDef[RF90_PATH_B].rf3wireOffset = rFPGA0_XB_LSSIParameter;

	priv->PHYRegDef[RF90_PATH_A].rfLSSI_Select = rFPGA0_XAB_RFParameter;  
	priv->PHYRegDef[RF90_PATH_B].rfLSSI_Select = rFPGA0_XAB_RFParameter;
	priv->PHYRegDef[RF90_PATH_C].rfLSSI_Select = rFPGA0_XCD_RFParameter;
	priv->PHYRegDef[RF90_PATH_D].rfLSSI_Select = rFPGA0_XCD_RFParameter;

	priv->PHYRegDef[RF90_PATH_A].rfTxGainStage = rFPGA0_TxGainStage; 
	priv->PHYRegDef[RF90_PATH_B].rfTxGainStage = rFPGA0_TxGainStage; 
	priv->PHYRegDef[RF90_PATH_C].rfTxGainStage = rFPGA0_TxGainStage; 
	priv->PHYRegDef[RF90_PATH_D].rfTxGainStage = rFPGA0_TxGainStage; 

	priv->PHYRegDef[RF90_PATH_A].rfHSSIPara1 = rFPGA0_XA_HSSIParameter1;  
	priv->PHYRegDef[RF90_PATH_B].rfHSSIPara1 = rFPGA0_XB_HSSIParameter1;  

	priv->PHYRegDef[RF90_PATH_A].rfHSSIPara2 = rFPGA0_XA_HSSIParameter2;  
	priv->PHYRegDef[RF90_PATH_B].rfHSSIPara2 = rFPGA0_XB_HSSIParameter2;  

	priv->PHYRegDef[RF90_PATH_A].rfSwitchControl = rFPGA0_XAB_SwitchControl; 
	priv->PHYRegDef[RF90_PATH_B].rfSwitchControl = rFPGA0_XAB_SwitchControl;
	priv->PHYRegDef[RF90_PATH_C].rfSwitchControl = rFPGA0_XCD_SwitchControl;
	priv->PHYRegDef[RF90_PATH_D].rfSwitchControl = rFPGA0_XCD_SwitchControl;

	priv->PHYRegDef[RF90_PATH_A].rfAGCControl1 = rOFDM0_XAAGCCore1;
	priv->PHYRegDef[RF90_PATH_B].rfAGCControl1 = rOFDM0_XBAGCCore1;
	priv->PHYRegDef[RF90_PATH_C].rfAGCControl1 = rOFDM0_XCAGCCore1;
	priv->PHYRegDef[RF90_PATH_D].rfAGCControl1 = rOFDM0_XDAGCCore1;

	priv->PHYRegDef[RF90_PATH_A].rfAGCControl2 = rOFDM0_XAAGCCore2;
	priv->PHYRegDef[RF90_PATH_B].rfAGCControl2 = rOFDM0_XBAGCCore2;
	priv->PHYRegDef[RF90_PATH_C].rfAGCControl2 = rOFDM0_XCAGCCore2;
	priv->PHYRegDef[RF90_PATH_D].rfAGCControl2 = rOFDM0_XDAGCCore2;

	priv->PHYRegDef[RF90_PATH_A].rfRxIQImbalance = rOFDM0_XARxIQImbalance;
	priv->PHYRegDef[RF90_PATH_B].rfRxIQImbalance = rOFDM0_XBRxIQImbalance;
	priv->PHYRegDef[RF90_PATH_C].rfRxIQImbalance = rOFDM0_XCRxIQImbalance;
	priv->PHYRegDef[RF90_PATH_D].rfRxIQImbalance = rOFDM0_XDRxIQImbalance;	

	priv->PHYRegDef[RF90_PATH_A].rfRxAFE = rOFDM0_XARxAFE;
	priv->PHYRegDef[RF90_PATH_B].rfRxAFE = rOFDM0_XBRxAFE;
	priv->PHYRegDef[RF90_PATH_C].rfRxAFE = rOFDM0_XCRxAFE;
	priv->PHYRegDef[RF90_PATH_D].rfRxAFE = rOFDM0_XDRxAFE;	

	priv->PHYRegDef[RF90_PATH_A].rfTxIQImbalance = rOFDM0_XATxIQImbalance;
	priv->PHYRegDef[RF90_PATH_B].rfTxIQImbalance = rOFDM0_XBTxIQImbalance;
	priv->PHYRegDef[RF90_PATH_C].rfTxIQImbalance = rOFDM0_XCTxIQImbalance;
	priv->PHYRegDef[RF90_PATH_D].rfTxIQImbalance = rOFDM0_XDTxIQImbalance;	

	priv->PHYRegDef[RF90_PATH_A].rfTxAFE = rOFDM0_XATxAFE;
	priv->PHYRegDef[RF90_PATH_B].rfTxAFE = rOFDM0_XBTxAFE;
	priv->PHYRegDef[RF90_PATH_C].rfTxAFE = rOFDM0_XCTxAFE;
	priv->PHYRegDef[RF90_PATH_D].rfTxAFE = rOFDM0_XDTxAFE;	

	priv->PHYRegDef[RF90_PATH_A].rfLSSIReadBack = rFPGA0_XA_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_B].rfLSSIReadBack = rFPGA0_XB_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_C].rfLSSIReadBack = rFPGA0_XC_LSSIReadBack;
	priv->PHYRegDef[RF90_PATH_D].rfLSSIReadBack = rFPGA0_XD_LSSIReadBack;	

	priv->PHYRegDef[RF90_PATH_A].rfLSSIReadBackPi = TransceiverA_HSPI_Readback;
	priv->PHYRegDef[RF90_PATH_B].rfLSSIReadBackPi = TransceiverB_HSPI_Readback;

}


extern	bool
PHY_SetRFPowerState(
	struct net_device* dev, 
	RT_RF_POWER_STATE	eRFPowerState
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool			bResult = false;

	RT_TRACE(COMP_RF, "---------> PHY_SetRFPowerState(): eRFPowerState(%d)\n", eRFPowerState);
	if(eRFPowerState == priv->rtllib->eRFPowerState)
	{
		RT_TRACE(COMP_RF, "<--------- PHY_SetRFPowerState(): discard the request for eRFPowerState(%d) is the same.\n", eRFPowerState);
		return bResult;
	}
#if 1
	bResult = phy_SetRFPowerState8192CE(dev, eRFPowerState);
#else
	bResult = phy_SetRFPowerState8192SU(dev, eRFPowerState);
#endif
		
	RT_TRACE(COMP_RF, "<--------- PHY_SetRFPowerState(): bResult(%d)\n", bResult);

	return bResult;
}


#if 1
/*-----------------------------------------------------------------------------
 * Function:	PHY_SetRtl8192seRfHalt()
 *
 * Overview:	For different power save scheme. Reboot/Halt(S3/S4)/SW radio/
 *				SW radio/IPS will call the scheme.
 *
 * Input:		IN	PADAPTER	pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	03/27/2009	MHC		Merge from Macro SET_RTL8192SE_RF_HALT
 *						Because we need to send some parameter to the funtion.
 *						Macro is hard to maintain larger code.
 *
 *---------------------------------------------------------------------------*/
extern	void	
PHY_SetRtl8192seRfHalt(		struct net_device* dev)
{
	
	
}	


bool
phy_SetRFPowerState8192CE(
	struct net_device* dev,
	RT_RF_POWER_STATE	eRFPowerState
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool			bResult = true;
	u8			i, QueueID;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(priv);
	struct rtl8192_tx_ring  *ring = NULL;
	
	priv->SetRFPowerStateInProgress = true;
	
	switch(priv->rf_chip )
	{
		default:	
		switch( eRFPowerState )
		{
			case eRfOn:
				{
				#if(MUTUAL_AUTHENTICATION == 1)
					if(priv->MutualAuthenticationFail)
						break;
				#endif
					if((priv->rtllib->eRFPowerState == eRfOff) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC))
					{ 
						RT_STATUS rtstatus;
						u32 InitializeCount = 0;
						do
						{	
							InitializeCount++;
							rtstatus = NicIFEnableNIC( dev );
						}while( (rtstatus != true) &&(InitializeCount <10) );
						RT_ASSERT(rtstatus == true,("Nic Initialize Fail\n"));
						RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
					}
					else
					{ 
						phy_SetRTL8192CERfOn(dev);
					}

					if( priv->rtllib->state == RTLLIB_LINKED )
					{
						priv->rtllib->LedControlHandler(dev, LED_CTL_LINK); 
					}
					else
					{
						priv->rtllib->LedControlHandler(dev, LED_CTL_NO_LINK); 
					}
				}
				break;
			case eRfOff:
				{					
				for(QueueID = 0, i = 0; QueueID < MAX_TX_QUEUE; )
				{
					ring = &priv->tx_ring[QueueID];
					if(skb_queue_len(&ring->queue) == 0)
					{
						QueueID++;
						continue;
					}
#ifdef MERGE_TO_DO						
					#if 1
					else if(IsLowPowerState(Adapter))
					{
							RT_TRACE(COMP_POWER,  
							"eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 but lower power state!\n", (i+1), QueueID);
						break;
					}
					#endif
#endif
					else
					{
							RT_TRACE(COMP_POWER,  
							"eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 before doze!\n", (i+1), QueueID);
							udelay(10);
						i++;
					}
					
					if(i >= MAX_DOZE_WAITING_TIMES_9x)
					{
						RT_TRACE(COMP_POWER, "\n\n\n SetZebraRFPowerState8185B(): eRfOff: %d times TcbBusyQueue[%d] != 0 !!!\n\n\n", MAX_DOZE_WAITING_TIMES_9x, QueueID);
						break;
					}
				}			

					if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_HALT_NIC)
					{ 
						NicIFDisableNIC(dev);
						RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
					} 
					else
					{ 
						if(priv->rtllib->RfOffReason==RF_CHANGE_BY_IPS )
						{
							priv->rtllib->LedControlHandler(dev,LED_CTL_NO_LINK); 
						}
						else
						{
							priv->rtllib->LedControlHandler(dev, LED_CTL_POWER_OFF); 
						}
					}
				}
					break;
				
			case eRfSleep:
				{
					if(priv->rtllib->eRFPowerState == eRfOff)
						break;
					
					for(QueueID = 0, i = 0; QueueID < MAX_TX_QUEUE; )
					{
						ring = &priv->tx_ring[QueueID];
						if(skb_queue_len(&ring->queue) == 0)
						{
							QueueID++;
							continue;
						}

#ifdef MERGE_TO_TO						
						#if 1;
						else if(IsLowPowerState(dev))
						{
							RT_TRACE(COMP_POWER, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 but lower power state!\n", (i+1), QueueID);
							break;
						}
						#endif
#endif
						else
						{
							RT_TRACE(COMP_POWER, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 before doze!\n", (i+1), QueueID);
							udelay(10);
							i++;
						}
						
						if(i >= MAX_DOZE_WAITING_TIMES_9x)
						{
							RT_TRACE(COMP_POWER, "\n\n\n SetZebraRFPowerState8185B(): eRfOff: %d times TcbBusyQueue[%d] != 0 !!!\n\n\n", MAX_DOZE_WAITING_TIMES_9x, QueueID);
							break;
						}
					}		

					phy_SetRTL8192CERfSleep(dev);
				}
				break;
				
				default:
				bResult = false;
				RT_ASSERT(false, ("phy_SetRFPowerState8192S(): unknow state to set: 0x%X!!!\n", eRFPowerState));
					break;
		} 
				break;
	}

	if(bResult)
	{
		priv->rtllib->eRFPowerState = eRFPowerState;
	}
	
	priv->SetRFPowerStateInProgress = false;

	return bResult;
}


/*-----------------------------------------------------------------------------
 * Function:	PHY_SwitchEphyParameter()
 *
 * Overview:	To prevent ASPM error. We need to change some EPHY parameter to 
 *			replace HW autoload value..
 *
 * Input:		IN	PADAPTER			pAdapter
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	12/26/2008	MHC		Create. The flow is refered to DD PCIE.
 *
 *---------------------------------------------------------------------------*/
void
PHY_SwitchEphyParameter(struct net_device* dev,bool	bSleep)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u16	U2bTmp;
	u32 	delay1=100;
	u8	U1bTmp;

	if(!IS_NORMAL_CHIP(priv->card_8192_version))
	{
		write_nic_byte(dev, 0x358, 0x48);
		udelay(100);
		U1bTmp = read_nic_byte(dev, 0x358);
		while((U1bTmp&BIT6) && (delay1>0))
		{
			udelay(100);
			delay1--;
			U1bTmp = read_nic_byte(dev, 0x358);
			printk("1 PHY_SwitchEphyParameter(): 0x358 = %2x, delay1 = %d\n", U1bTmp, delay1);
		}
		U2bTmp = read_nic_word(dev, 0x356);
		printk("1 PHY_SwitchEphyParameter(): 0x356 = %4x\n", U2bTmp);
		if(bSleep)
		{
			write_nic_word(dev, 0x354, (U2bTmp|BIT3));
		}
		else
		{
			U2bTmp &= ~(BIT3);
			write_nic_word(dev, 0x354, U2bTmp);
		}
		write_nic_byte(dev, 0x358, 0x28);
		udelay(100);
		write_nic_byte(dev, 0x358, 0x48);
		udelay(100);
		U1bTmp = read_nic_byte(dev, 0x358);
		delay1 = 100;
		while((U1bTmp&BIT6) && (delay1>0))
		{
			udelay(100);
			delay1--;
			U1bTmp = read_nic_byte(dev, 0x358);
			printk("1 PHY_SwitchEphyParameter(): 0x358 = %2x, delay1 = %d\n", U1bTmp, delay1);
		}
		U2bTmp = read_nic_word(dev, 0x356);
		printk("2 PHY_SwitchEphyParameter(): 0x356 = %4x\n", U2bTmp);
#if 0
		while((U2bTmp&BIT3) && (delay > 0))
		{
			U2bTmp = PlatformEFIORead2Byte(Adapter, 0x356);
			delay--;
			delay_us(100);

		}
		PlatformEFIOWrite1Byte(Adapter, 0x358, 0x28);
		DbgPrint("PHY_SwitchEphyParameter(): 0x356 = %4x, delay = %d\n", U2bTmp, delay);
#endif
	}
	
}	



#endif


/*-----------------------------------------------------------------------------
 * Function:    GetTxPowerLevel8190()
 *
 * Overview:    This function is export to "common" moudule
 *
 * Input:       PADAPTER		Adapter
 *			psByte			Power Level
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 *---------------------------------------------------------------------------*/
extern	void
PHY_GetTxPowerLevel8192C(
	struct net_device* dev,
	long*    		powerlevel
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8			TxPwrLevel = 0;
	long			TxPwrDbm;
	

	TxPwrLevel = priv->CurrentCckTxPwrIdx;
	TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_B, TxPwrLevel);

	TxPwrLevel = priv->CurrentOfdm24GTxPwrIdx + priv->LegacyHTTxPowerDiff;

	if(phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_G, TxPwrLevel);

	TxPwrLevel = priv->CurrentOfdm24GTxPwrIdx;
	
	if(phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_N_24G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(dev, WIRELESS_MODE_N_24G, TxPwrLevel);

	*powerlevel = TxPwrDbm;
}


void getTxPowerIndex(
	struct net_device* dev,
	u8			channel,
	u8*		cckPowerLevel,
	u8*		ofdmPowerLevel
	)

{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8				index = (channel -1);
	cckPowerLevel[RF90_PATH_A] = priv->TxPwrLevelCck[RF90_PATH_A][index];	
	cckPowerLevel[RF90_PATH_B] = priv->TxPwrLevelCck[RF90_PATH_B][index];	

	if (GET_RF_TYPE(priv) == RF_1T2R || GET_RF_TYPE(priv) == RF_1T1R)
	{
		ofdmPowerLevel[RF90_PATH_A] = priv->TxPwrLevelHT40_1S[RF90_PATH_A][index];
		ofdmPowerLevel[RF90_PATH_B] = priv->TxPwrLevelHT40_1S[RF90_PATH_B][index];
	}
	else if (GET_RF_TYPE(priv) == RF_2T2R)
	{
		ofdmPowerLevel[RF90_PATH_A] = priv->TxPwrLevelHT40_2S[RF90_PATH_A][index];
		ofdmPowerLevel[RF90_PATH_B] = priv->TxPwrLevelHT40_2S[RF90_PATH_B][index];
	}
}


void ccxPowerIndexCheck(
	struct net_device* dev,
	u8			channel,
	u8*		cckPowerLevel,
	u8*		ofdmPowerLevel
	)
{

	struct r8192_priv 		*priv = rtllib_priv(dev);

#ifdef TODO 
	if (priv->rtllib->iw_mode != IW_MODE_INFRA && priv->bWithCcxCellPwr &&
		channel == priv->rtllib->current_network.channel)
	{
		u8	CckCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, priv->CcxCellPwr);
		u8	LegacyOfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_G, priv->CcxCellPwr);
		u8	OfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, priv->CcxCellPwr);

		RT_TRACE(COMP_TXAGC,  
		"CCX Cell Limit: %d dbm => CCK Tx power index : %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n", 
		priv->CcxCellPwr, CckCellPwrIdx, LegacyOfdmCellPwrIdx, OfdmCellPwrIdx);
		RT_TRACE(COMP_TXAGC,  
		"EEPROM channel(%d) => CCK Tx power index: %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n",
		channel, cckPowerLevel[0], ofdmPowerLevel[0] + priv->LegacyHTTxPowerDiff, ofdmPowerLevel[0]); 

		if(cckPowerLevel[0] > CckCellPwrIdx)
			cckPowerLevel[0] = CckCellPwrIdx;
		if(ofdmPowerLevel[0] + priv->LegacyHTTxPowerDiff > LegacyOfdmCellPwrIdx)
		{
			if((OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff) > 0)
			{
				ofdmPowerLevel[0] = OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff;
			}
			else
			{
				ofdmPowerLevel[0] = 0;
			}
		}

		RT_TRACE(COMP_TXAGC,  
		"Altered CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		cckPowerLevel[0], ofdmPowerLevel[0] + priv->LegacyHTTxPowerDiff, ofdmPowerLevel[0]);
	}
#endif

	priv->CurrentCckTxPwrIdx = cckPowerLevel[0];
	priv->CurrentOfdm24GTxPwrIdx = ofdmPowerLevel[0];

	RT_TRACE(COMP_TXAGC,  
		"PHY_SetTxPowerLevel8192S(): CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		cckPowerLevel[0], ofdmPowerLevel[0] + priv->LegacyHTTxPowerDiff, ofdmPowerLevel[0]);

}

extern void PHY_GetTxPowerIndex8192C(
	struct net_device* dev,
	u8		channel,
	u8*		cckPowerLevel,
	u8*		ofdmPowerLevel
	)
{
	getTxPowerIndex(dev, channel, cckPowerLevel, ofdmPowerLevel);	
	ccxPowerIndexCheck(dev, channel, cckPowerLevel, ofdmPowerLevel);
}

/*-----------------------------------------------------------------------------
 * Function:    SetTxPowerLevel8190()
 *
 * Overview:    This function is export to "HalCommon" moudule
 *			We must consider RF path later!!!!!!!
 *
 * Input:       PADAPTER		Adapter
 *			u8		channel
 *
 * Output:      NONE
 *
 * Return:      NONE
 *	2008/11/04	MHC		We remove EEPROM_93C56.
 *						We need to move CCX relative code to independet file.
 *	2009/01/21	MHC		Support new EEPROM format from SD3 requirement.
 *
 *---------------------------------------------------------------------------*/
extern	void
PHY_SetTxPowerLevel8192C(
	struct net_device* dev,
	u8			channel
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8	cckPowerLevel[2], ofdmPowerLevel[2];	

#if 0
	return;
#endif

	if(priv->bTXPowerDataReadFromEEPORM == false)
		return;

	getTxPowerIndex(dev, channel, &cckPowerLevel[0], &ofdmPowerLevel[0]);
	RTPRINT(FPHY, PHY_TXPWR, ("Channel-%d, cckPowerLevel (A / B) = 0x%x / 0x%x,   ofdmPowerLevel (A / B) = 0x%x / 0x%x\n", 
		channel, cckPowerLevel[0], cckPowerLevel[1], ofdmPowerLevel[0], ofdmPowerLevel[1]));

	ccxPowerIndexCheck(dev, channel, &cckPowerLevel[0], &ofdmPowerLevel[0]);

	switch(priv->rf_chip)
	{
		case RF_8225:
			PHY_SetRF8225CckTxPower(dev, cckPowerLevel[0]);
			PHY_SetRF8225OfdmTxPower(dev, ofdmPowerLevel[0]);
		break;

		case RF_8256:
			PHY_SetRF8256CCKTxPower(dev, cckPowerLevel[0]);
			PHY_SetRF8256OFDMTxPower(dev, ofdmPowerLevel[0]);
			break;

		case RF_6052:
			PHY_RF6052SetCckTxPower(dev, &cckPowerLevel[0]);
			PHY_RF6052SetOFDMTxPower(dev, &ofdmPowerLevel[0], channel);
			break;

		case RF_8258:
			break;
		default:
			break;
	}
}


extern	bool
PHY_UpdateTxPowerDbm8192C(
	struct net_device* dev,
	long		powerInDbm
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8				idx;
	u8			rf_path;

	u8	CckTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, powerInDbm);
	u8	OfdmTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, powerInDbm);

	if(OfdmTxPwrIdx - priv->LegacyHTTxPowerDiff > 0)
		OfdmTxPwrIdx -= priv->LegacyHTTxPowerDiff;
	else
		OfdmTxPwrIdx = 0;

	RT_TRACE(COMP_TXAGC, "PHY_UpdateTxPowerDbm8192S(): %lx dBm , CckTxPwrIdx = %d, OfdmTxPwrIdx = %d\n", powerInDbm, CckTxPwrIdx, OfdmTxPwrIdx);

	for(idx = 0; idx < 14; idx++)
	{
		for (rf_path = 0; rf_path < 2; rf_path++)
		{
			priv->TxPwrLevelCck[rf_path][idx] = CckTxPwrIdx;
			priv->TxPwrLevelHT40_1S[rf_path][idx] = 
			priv->TxPwrLevelHT40_2S[rf_path][idx] = OfdmTxPwrIdx;
		}
	}

#ifdef MERGE_TO_DO
	priv->SetTxPowerLevelHandler(dev, priv->chan);
#else
	PHY_SetTxPowerLevel8192C(dev, priv->chan);
#endif

	return true;	
}


/*
	Description:
		When beacon interval is changed, the values of the 
		hw registers should be modified.
	By tynli, 2008.10.24.

*/


extern void	
PHY_SetBeaconHwReg(	
	struct net_device* dev,
	u16			BeaconInterval	
	)
{

}

static	u8
phy_DbmToTxPwrIdx(
	struct net_device* dev,
	WIRELESS_MODE	WirelessMode,
	long			PowerInDbm
	)
{
	u8				TxPwrIdx = 0;
	long				Offset = 0;
	

	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;
		break;
	default:
		Offset = -8;
		break;
	}

	if((PowerInDbm - Offset) > 0)
	{
		TxPwrIdx = (u8)((PowerInDbm - Offset) * 2);
	}
	else
	{
		TxPwrIdx = 0;
	}

	if(TxPwrIdx > MAX_TXPWR_IDX_NMODE_92S)
		TxPwrIdx = MAX_TXPWR_IDX_NMODE_92S;

	return TxPwrIdx;
}

long
phy_TxPwrIdxToDbm(
	struct net_device* dev,
	WIRELESS_MODE	WirelessMode,
	u8			TxPwrIdx
	)
{
	long				Offset = 0;
	long				PwrOutDbm = 0;
	
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;
		break;
	default:
		Offset = -8;
		break;
	}

	PwrOutDbm = TxPwrIdx / 2 + Offset; 

	return PwrOutDbm;
}


extern	void 
PHY_ScanOperationBackup8192C(
	struct net_device* dev,
	u8		Operation
	)
{

	struct r8192_priv 	*priv = rtllib_priv(dev);
	IO_TYPE	IoType;
	
	if(priv->up)
	{
		switch(Operation)
		{
			case SCAN_OPT_BACKUP:
				IoType = IO_CMD_PAUSE_DM_BY_SCAN;
				priv->rtllib->SetHwRegHandler(dev,HW_VAR_IO_CMD,  (u8*)&IoType);

				break;

			case SCAN_OPT_RESTORE:
				IoType = IO_CMD_RESUME_DM_BY_SCAN;
				priv->rtllib->SetHwRegHandler(dev,HW_VAR_IO_CMD,  (u8*)&IoType);
				break;

			default:
				RT_TRACE(COMP_SCAN, "Unknown Scan Backup Operation. \n");
				break;
		}
	}
}


/*-----------------------------------------------------------------------------
 * Function:    PHY_SetBWModeCallback8192C()
 *
 * Overview:    Timer callback function for SetSetBWMode
 *
 * Input:       	PRT_TIMER		pTimer
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Note:		(1) We do not take j mode into consideration now
 *			(2) Will two workitem of "switch channel" and "switch channel bandwidth" run
 *			     concurrently?
 *---------------------------------------------------------------------------*/
extern	void
PHY_SetBWModeCallback8192C(struct net_device *dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8	 			regBwOpMode;
	u8				regRRSR_RSC;



	RT_TRACE(COMP_SCAN, "==>PHY_SetBWModeCallback8192C()  Switch to %s bandwidth\n", \
					priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz")

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SetBWModeInProgress= false;
		return;
	}

	if(priv->rf_chip==RF_8225)
		return;

	if(IS_NIC_DOWN(priv))
		return;

		
	
	regBwOpMode = read_nic_byte(dev, REG_BWOPMODE);
	regRRSR_RSC = read_nic_byte(dev, REG_RRSR+2);
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
			write_nic_byte(dev, REG_BWOPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
			write_nic_byte(dev, REG_BWOPMODE, regBwOpMode);

			regRRSR_RSC = (regRRSR_RSC&0x90) |(priv->nCur40MhzPrimeSC<<5);
			write_nic_byte(dev, REG_RRSR+2, regRRSR_RSC);
			break;

		default:
			RT_TRACE(COMP_DBG, "PHY_SetBWModeCallback8192C():\
						unknown Bandwidth: %#X\n",priv->CurrentChannelBW);
			break;
	}
	
	switch(priv->CurrentChannelBW)
	{
		/* 20 MHz channel*/
		case HT_CHANNEL_WIDTH_20:
			PHY_SetBBReg(dev, rFPGA0_RFMOD, bRFMOD, 0x0);
			PHY_SetBBReg(dev, rFPGA1_RFMOD, bRFMOD, 0x0);
			PHY_SetBBReg(dev, rFPGA0_AnalogParameter2, BIT10, 1);
			
			break;


		/* 40 MHz channel*/
		case HT_CHANNEL_WIDTH_20_40:
			PHY_SetBBReg(dev, rFPGA0_RFMOD, bRFMOD, 0x1);
			PHY_SetBBReg(dev, rFPGA1_RFMOD, bRFMOD, 0x1);
			
			PHY_SetBBReg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));
			PHY_SetBBReg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);
			PHY_SetBBReg(dev, rFPGA0_AnalogParameter2, BIT10, 0);
			
			PHY_SetBBReg(dev, 0x818, (BIT26|BIT27), (priv->nCur40MhzPrimeSC==HAL_PRIME_CHNL_OFFSET_LOWER)?2:1);
			
			break;


			
		default:
			RT_TRACE(COMP_DBG, "PHY_SetBWModeCallback8192C(): unknown Bandwidth: %#X\n"\
						,priv->CurrentChannelBW);
			break;
			
	}


	switch( priv->rf_chip )
	{
		case RF_8225:		
			PHY_SetRF8225Bandwidth(dev, priv->CurrentChannelBW);
			break;	
			
		case RF_8256:
			PHY_SetRF8256Bandwidth(dev, priv->CurrentChannelBW);
			break;
			
		case RF_8258:
			break;

		case RF_PSEUDO_11N:
			break;
			
		case RF_6052:
			PHY_RF6052SetBandwidth(dev, priv->CurrentChannelBW);
			break;	
			
		default:
			RT_ASSERT(false, ("Unknown RFChipID: %d\n", priv->rf_chip));
			break;
	}

	priv->SetBWModeInProgress= false;

	RT_TRACE(COMP_SCAN, "<==PHY_SetBWModeCallback8192C() \n" );
}


 /*-----------------------------------------------------------------------------
 * Function:   SetBWMode8190Pci()
 *
 * Overview:  This function is export to "HalCommon" moudule
 *
 * Input:       	PADAPTER			Adapter
 *			HT_CHANNEL_WIDTH	Bandwidth	
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Note:		We do not take j mode into consideration now
 *---------------------------------------------------------------------------*/
extern	void
PHY_SetBWMode8192C(
	struct net_device* dev,
	HT_CHANNEL_WIDTH	Bandwidth,	
	HT_EXTCHNL_OFFSET	Offset		
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	HT_CHANNEL_WIDTH 	tmpBW= priv->CurrentChannelBW;

	


	if(priv->SetBWModeInProgress)
		return;

	priv->SetBWModeInProgress= true;
	
	priv->CurrentChannelBW = Bandwidth;
	
	if(Offset==HT_EXTCHNL_OFFSET_LOWER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
	else if(Offset==HT_EXTCHNL_OFFSET_UPPER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
	else
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

	if((!IS_NIC_DOWN(priv)) && !(RT_CANNOT_IO(dev)))
	{
		PHY_SetBWModeCallback8192C(dev);
	}
	else
	{
		RT_TRACE(COMP_SCAN, "PHY_SetBWMode8192C() SetBWModeInProgress FALSE driver sleep or unload\n");	
		priv->SetBWModeInProgress= false;	
		priv->CurrentChannelBW = tmpBW;
	}
	
}


extern	void
PHY_SwChnlCallback8192C(struct net_device *dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32			delay;
	
	RT_TRACE(COMP_SCAN, "==>PHY_SwChnlCallback8192C(), switch to channel\
				%d\n", priv->chan);
	
	if(IS_NIC_DOWN(priv))
		return;
	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=false;
		return; 								
	}
	

	do{
		if(!priv->SwChnlInProgress)
			break;

		if(!phy_SwChnlStepByStep(dev, priv->chan, &priv->SwChnlStage, &priv->SwChnlStep, &delay))
		{
			if(delay>0)
				mdelay(delay);
			else
				continue;
		}
		else
		{
			priv->SwChnlInProgress=false;
		}
		break;
	}while(true);

	RT_TRACE(COMP_SCAN, "<==PHY_SwChnlCallback8192C()\n");
}


extern	u8
PHY_SwChnl8192C(	
	struct net_device* dev,
	u8		channel
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool	bResult=false;

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=false;
		return 0; 								
	}
	
	if(priv->SwChnlInProgress){
		return 0;
	}

	if(priv->SetBWModeInProgress){
		return 0;
	}

	switch(priv->rtllib->mode)
	{
	case WIRELESS_MODE_A:
	case WIRELESS_MODE_N_5G:
			RT_ASSERT((channel>14), ("WIRELESS_MODE_A but channel<=14"));		
		break;
		
	case WIRELESS_MODE_B:
			RT_ASSERT((channel<=14), ("WIRELESS_MODE_B but channel>14"));
		break;
		
	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
			RT_ASSERT((channel<=14), ("WIRELESS_MODE_G but channel>14"));
			break;

		default:
			RT_ASSERT(false, ("Invalid WirelessMode(%#x)!!\n", priv->rtllib->mode));
		break;
	}
	
	priv->SwChnlInProgress = true;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;

	priv->SwChnlStage=0;
	priv->SwChnlStep=0;

	if(!(IS_NIC_DOWN(priv)) && !(RT_CANNOT_IO(dev)) /*&& Adapter->bInSetPower)*/)
	{
		PHY_SwChnlCallback8192C(dev);
		if(bResult)
		{
			RT_TRACE(COMP_SCAN, "PHY_SwChnl8192C SwChnlInProgress TRUE schdule workitem done\n");
		}
		else
		{
			RT_TRACE(COMP_SCAN, "PHY_SwChnl8192C SwChnlInProgress FALSE schdule workitem error\n");		
			priv->SwChnlInProgress = false; 	
		}

	}
	else
	{
		RT_TRACE(COMP_SCAN, "PHY_SwChnl8192C SwChnlInProgress FALSE driver sleep or unload\n");	
		priv->SwChnlInProgress = false;		
	}
	return 1;
}


extern	void
PHY_SwChnlPhy8192C(	
	struct net_device* dev,
	u8		channel
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);

	RT_TRACE(COMP_SCAN , "==>PHY_SwChnlPhy8192S(), switch from channel %d to channel %d.\n", priv->chan, channel);

	if(RT_CANNOT_IO(dev))
		return;

	if(priv->SwChnlInProgress)
		return;
	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=false;
		return;
	}
	
	priv->SwChnlInProgress = true;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;
	
	priv->SwChnlStage = 0;
	priv->SwChnlStep = 0;
	
	phy_FinishSwChnlNow(dev,channel);
	
	priv->SwChnlInProgress = false;
}


static	bool
phy_SwChnlStepByStep(
	struct net_device* dev,
	u8		channel,
	u8		*stage,
	u8		*step,
	u32		*delay
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	SwChnlCmd				PreCommonCmd[MAX_PRECMD_CNT];
	u32					PreCommonCmdCnt;
	SwChnlCmd				PostCommonCmd[MAX_POSTCMD_CNT];
	u32					PostCommonCmdCnt;
	SwChnlCmd				RfDependCmd[MAX_RFDEPENDCMD_CNT];
	u32					RfDependCmdCnt;
	SwChnlCmd				*CurrentCmd =NULL;	
	u8					eRFPath;	
	
	RT_ASSERT((dev != NULL), ("Adapter should not be NULL\n"));
#if 0
	RT_ASSERT(((IsLegalChannel(priv->rtllib, channel) == 1)?true:false), ("illegal channel: %d\n", channel));
#endif
	RT_ASSERT((priv != NULL), ("pHalData should not be NULL\n"));

#ifdef MERGE_TO_DO
#endif

	PreCommonCmdCnt = 0;
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_SetTxPowerLevel, 0, 0, 0);
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_End, 0, 0, 0);
	
	PostCommonCmdCnt = 0;

	phy_SetSwChnlCmdArray(PostCommonCmd, PostCommonCmdCnt++, MAX_POSTCMD_CNT, 
				CmdID_End, 0, 0, 0);
	
	RfDependCmdCnt = 0;
	switch( priv->rf_chip )
	{
		case RF_8225:		
		RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		if(channel==14) channel++;
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rZebra1_Channel, (0x10+channel-1), 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;	
		
	case RF_8256:
		RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;
		
	case RF_6052:
		RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, RF_CHNLBW, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);		
		break;

	case RF_8258:
		break;

	case RF_PSEUDO_11N:
		return true;
	default:
		RT_ASSERT(false, ("Unknown RFChipID: %d\n", priv->rf_chip));
		return false;
		break;
	}

	
	do{
		switch(*stage)
		{
		case 0:
			CurrentCmd=&PreCommonCmd[*step];
			break;
		case 1:
			CurrentCmd=&RfDependCmd[*step];
			break;
		case 2:
			CurrentCmd=&PostCommonCmd[*step];
			break;
		}
		
		if(CurrentCmd->CmdID==CmdID_End)
		{
			if((*stage)==2)
			{
				return true;
			}
			else
			{
				(*stage)++;
				(*step)=0;
				continue;
			}
		}
		
		switch(CurrentCmd->CmdID)
		{
		case CmdID_SetTxPowerLevel:
			PHY_SetTxPowerLevel8192C(dev,channel);
			break;
		case CmdID_WritePortUlong:
			write_nic_dword(dev, CurrentCmd->Para1, CurrentCmd->Para2);
			break;
		case CmdID_WritePortUshort:
			write_nic_word(dev, CurrentCmd->Para1, (u16)CurrentCmd->Para2);
			break;
		case CmdID_WritePortUchar:
			write_nic_byte(dev, CurrentCmd->Para1, (u8)CurrentCmd->Para2);
			break;
		case CmdID_RF_WriteReg:	
			for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
			{
#if 1
				priv->RfRegChnlVal[eRFPath] = ((priv->RfRegChnlVal[eRFPath] & 0xfffffc00) | CurrentCmd->Para2);
				PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, priv->RfRegChnlVal[eRFPath]);
#else
				PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, (CurrentCmd->Para2));
#endif
			}
			break;
		default:
			printk("##################>%s() FIX ME\n", __func__);
			break;
		}
		
		break;
	}while(true);

	(*delay)=CurrentCmd->msDelay;
	(*step)++;
	return false;
}


static	bool
phy_SetSwChnlCmdArray(
	SwChnlCmd*		CmdTable,
	u32			CmdTableIdx,
	u32			CmdTableSz,
	SwChnlCmdID		CmdID,
	u32			Para1,
	u32			Para2,
	u32			msDelay
	)
{
	SwChnlCmd* pCmd;

	if(CmdTable == NULL)
	{
		RT_ASSERT(false, ("phy_SetSwChnlCmdArray(): CmdTable cannot be NULL.\n"));
		return false;
	}
	if(CmdTableIdx >= CmdTableSz)
	{
		RT_ASSERT(false, 
				("phy_SetSwChnlCmdArray(): Access invalid index, please check size of the table, CmdTableIdx:%x, CmdTableSz:%x\n",
				CmdTableIdx, CmdTableSz));
		return false;
	}

	pCmd = CmdTable + CmdTableIdx;
	pCmd->CmdID = CmdID;
	pCmd->Para1 = Para1;
	pCmd->Para2 = Para2;
	pCmd->msDelay = msDelay;

	return true;
}


static	void
phy_FinishSwChnlNow(	
		struct net_device* dev,
		u8		channel
		)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32			delay;
  
	while(!phy_SwChnlStepByStep(dev,channel,&priv->SwChnlStage,&priv->SwChnlStep,&delay))
	{
		if(delay>0)
			mdelay(delay);
	}
}

/*-----------------------------------------------------------------------------
 * Function:	PHYCheckIsLegalRfPath8190Pci()
 *
 * Overview:	Check different RF type to execute legal judgement. If RF Path is illegal
 *			We will return false.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	11/15/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
extern	bool	
rtl8192_phy_CheckIsLegalRFPath(	
	struct net_device* dev,
	u32	eRFPath)
{
	bool				rtValue = true;

#if 0	
	if (pHalData->RF_Type == RF_1T2R && eRFPath != RF90_PATH_A)
	{		
		rtValue = FALSE;
	}
	if (pHalData->RF_Type == RF_1T2R && eRFPath != RF90_PATH_A)
	{

	}
#endif
	return	rtValue;

}	/* PHY_CheckIsLegalRfPath8192C */

#define IQK_ADDA_REG_NUM	16
#define MAX_TOLERANCE		5
#define IQK_DELAY_TIME		1 	

u8 	
_PHY_PathA_IQK(
	struct net_device* dev,
	bool		configPathB
	)
{
	u32 regEAC, regE94, regE9C, regEA4;
	u8 result = 0x00;

	RTPRINT(FINIT, INIT_IQK, ("Path A IQK!\n"));

	RTPRINT(FINIT, INIT_IQK, ("Path-A IQK setting!\n"));
	PHY_SetBBReg(dev, 0xe30, bMaskDWord, 0x10008c1f);
	PHY_SetBBReg(dev, 0xe34, bMaskDWord, 0x10008c1f);
	PHY_SetBBReg(dev, 0xe38, bMaskDWord, 0x82140102);

	PHY_SetBBReg(dev, 0xe3c, bMaskDWord, configPathB ? 0x28160202 : 0x28160502);

#if 1
	if(configPathB)
	{
		PHY_SetBBReg(dev, 0xe50, bMaskDWord, 0x10008c22);
		PHY_SetBBReg(dev, 0xe54, bMaskDWord, 0x10008c22);
		PHY_SetBBReg(dev, 0xe58, bMaskDWord, 0x82140102);
		PHY_SetBBReg(dev, 0xe5c, bMaskDWord, 0x28160202);
	}
#endif
	RTPRINT(FINIT, INIT_IQK, ("LO calibration setting!\n"));
	PHY_SetBBReg(dev, 0xe4c, bMaskDWord, 0x001028d1);

	RTPRINT(FINIT, INIT_IQK, ("One shot, path A LOK & IQK!\n"));
	PHY_SetBBReg(dev, 0xe48, bMaskDWord, 0xf9000000);
	PHY_SetBBReg(dev, 0xe48, bMaskDWord, 0xf8000000);
	
	RTPRINT(FINIT, INIT_IQK, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME));
	mdelay(IQK_DELAY_TIME);

	regEAC = PHY_QueryBBReg(dev, 0xeac, bMaskDWord);
	RTPRINT(FINIT, INIT_IQK, ("0xeac = 0x%x\n", regEAC));
	regE94 = PHY_QueryBBReg(dev, 0xe94, bMaskDWord);
	RTPRINT(FINIT, INIT_IQK, ("0xe94 = 0x%x\n", regE94));
	regE9C= PHY_QueryBBReg(dev, 0xe9c, bMaskDWord);
	RTPRINT(FINIT, INIT_IQK, ("0xe9c = 0x%x\n", regE9C));
	regEA4= PHY_QueryBBReg(dev, 0xea4, bMaskDWord);
	RTPRINT(FINIT, INIT_IQK, ("0xea4 = 0x%x\n", regEA4));

	if(!(regEAC & BIT28) &&		
		(((regE94 & 0x03FF0000)>>16) != 0x142) &&
		(((regE9C & 0x03FF0000)>>16) != 0x42) )
		result |= 0x01;
	else							
		return result;

	if(!(regEAC & BIT27) &&		
		(((regEA4 & 0x03FF0000)>>16) != 0x132) &&
		(((regEAC & 0x03FF0000)>>16) != 0x36))
		result |= 0x02;
	else
		RTPRINT(FINIT, INIT_IQK, ("Path A Rx IQK fail!!\n"));
	
	return result;


}

u8				
_PHY_PathB_IQK(
	struct net_device* dev
	)
{
	u32 regEAC, regEB4, regEBC, regEC4, regECC;
	u8	result = 0x00;

	RTPRINT(FINIT, INIT_IQK, ("Path B IQK!\n"));
#if 0
	RTPRINT(FINIT, INIT_IQK, ("Path-B IQK setting!\n"));
	PHY_SetBBReg(pAdapter, 0xe50, bMaskDWord, 0x10008c22);
	PHY_SetBBReg(pAdapter, 0xe54, bMaskDWord, 0x10008c22);
	PHY_SetBBReg(pAdapter, 0xe58, bMaskDWord, 0x82140102);
	PHY_SetBBReg(pAdapter, 0xe5c, bMaskDWord, 0x28160202);

	RTPRINT(FINIT, INIT_IQK, ("LO calibration setting!\n"));
	PHY_SetBBReg(pAdapter, 0xe4c, bMaskDWord, 0x001028d1);
#endif
	RTPRINT(FINIT, INIT_IQK, ("One shot, path A LOK & IQK!\n"));
	PHY_SetBBReg(dev, 0xe60, bMaskDWord, 0x00000002);
	PHY_SetBBReg(dev, 0xe60, bMaskDWord, 0x00000000);

	RTPRINT(FINIT, INIT_IQK, ("Delay %d ms for One shot, path B LOK & IQK.\n", IQK_DELAY_TIME));
	mdelay(IQK_DELAY_TIME);

	regEAC = PHY_QueryBBReg(dev, 0xeac, bMaskDWord);
	RTPRINT(FINIT, INIT_IQK, ("0xeac = 0x%x\n", regEAC));
	regEB4 = PHY_QueryBBReg(dev, 0xeb4, bMaskDWord);
	RTPRINT(FINIT, INIT_IQK, ("0xeb4 = 0x%x\n", regEB4));
	regEBC= PHY_QueryBBReg(dev, 0xebc, bMaskDWord);
	RTPRINT(FINIT, INIT_IQK, ("0xebc = 0x%x\n", regEBC));
	regEC4= PHY_QueryBBReg(dev, 0xec4, bMaskDWord);
	RTPRINT(FINIT, INIT_IQK, ("0xec4 = 0x%x\n", regEC4));
	regECC= PHY_QueryBBReg(dev, 0xecc, bMaskDWord);
	RTPRINT(FINIT, INIT_IQK, ("0xecc = 0x%x\n", regECC));

	if(!(regEAC & BIT31) &&
		(((regEB4 & 0x03FF0000)>>16) != 0x142) &&
		(((regEBC & 0x03FF0000)>>16) != 0x42))
		result |= 0x01;
	else
		return result;

	if(!(regEAC & BIT30) &&
		(((regEC4 & 0x03FF0000)>>16) != 0x132) &&
		(((regECC & 0x03FF0000)>>16) != 0x36))
		result |= 0x02;
	else
		RTPRINT(FINIT, INIT_IQK, ("Path B Rx IQK fail!!\n"));
	

	return result;


}

void
_PHY_PathAFillIQKMatrix(
	struct net_device* dev,
	bool     	bIQKOK,
	long		result[][8],
	u8		final_candidate,
	bool		bTxOnly
	)
{
	u32	Oldval_0, X, TX0_A, reg;
	long	Y, TX0_C;
	
	RTPRINT(FINIT, INIT_IQK, ("Path A IQ Calibration %s !\n",(bIQKOK)?"Success":"Failed"));

	if(final_candidate == 0xFF)
		return;	
	else if(bIQKOK)
	{
		Oldval_0 = (PHY_QueryBBReg(dev, 0xc80, bMaskDWord) >> 22) & 0x3FF;

		X = result[final_candidate][0];
		if ((X & 0x00000200) != 0)
			X = X | 0xFFFFFC00;
		
		TX0_A = (X * Oldval_0) >> 8;
		RTPRINT(FINIT, INIT_IQK, ("X = 0x%x, TX0_A = 0x%x\n", X, TX0_A));
		PHY_SetBBReg(dev, 0xc80, 0x3FF, TX0_A);
		PHY_SetBBReg(dev, 0xc4c, BIT(24), ((X* Oldval_0>>7) & 0x1));

		Y = result[final_candidate][1];
		if ((Y & 0x00000200) != 0)
			Y = Y | 0xFFFFFC00;
		
		TX0_C = (Y * Oldval_0) >> 8;
		RTPRINT(FINIT, INIT_IQK, ("Y = 0x%lx, TX = 0x%lx\n", Y, TX0_C));
		PHY_SetBBReg(dev, 0xc94, 0xF0000000, ((TX0_C&0x3C0)>>6));
		PHY_SetBBReg(dev, 0xc80, 0x003F0000, (TX0_C&0x3F));
		PHY_SetBBReg(dev, 0xc4c, BIT(26), ((Y* Oldval_0>>7) & 0x1));

		if(bTxOnly)
		{
			RTPRINT(FINIT, INIT_IQK, ("_PHY_PathAFillIQKMatrix only Tx OK\n"));		
			return;
		}
		
		reg = result[final_candidate][2];
		PHY_SetBBReg(dev, 0xc14, 0x3FF, reg);
	
		reg = result[final_candidate][3] & 0x3F;
		PHY_SetBBReg(dev, 0xc14, 0xFC00, reg);

		reg = (result[final_candidate][3] >> 6) & 0xF;
		PHY_SetBBReg(dev, 0xca0, 0xF0000000, reg);
	}
}

void
_PHY_PathBFillIQKMatrix(
	struct net_device* dev,
	bool     	bIQKOK,
	long		result[][8],
	u8		final_candidate,
	bool		bTxOnly			
	)
{
	u32	Oldval_1, X, TX1_A, reg;
	long	Y, TX1_C;
	
	RTPRINT(FINIT, INIT_IQK, ("Path B IQ Calibration %s !\n",(bIQKOK)?"Success":"Failed"));

	if(final_candidate == 0xFF)
		return;
	else if(bIQKOK)
	{
		Oldval_1 = (PHY_QueryBBReg(dev, 0xc88, bMaskDWord) >> 22) & 0x3FF;

		X = result[final_candidate][4];
		if ((X & 0x00000200) != 0)
			X = X | 0xFFFFFC00;
		
		TX1_A = (X * Oldval_1) >> 8;
		RTPRINT(FINIT, INIT_IQK, ("X = 0x%x, TX1_A = 0x%x\n", X, TX1_A));
		PHY_SetBBReg(dev, 0xc88, 0x3FF, TX1_A);
		PHY_SetBBReg(dev, 0xc4c, BIT(28), ((X* Oldval_1>>7) & 0x1));

		Y = result[final_candidate][5];
		if ((Y & 0x00000200) != 0)
			Y = Y | 0xFFFFFC00;
		
		TX1_C = (Y * Oldval_1) >> 8;
		RTPRINT(FINIT, INIT_IQK, ("Y = 0x%lx, TX1_C = 0x%lx\n", Y, TX1_C));
		PHY_SetBBReg(dev, 0xc9c, 0xF0000000, ((TX1_C&0x3C0)>>6));
		PHY_SetBBReg(dev, 0xc88, 0x003F0000, (TX1_C&0x3F));
		PHY_SetBBReg(dev, 0xc4c, BIT(30), ((Y* Oldval_1>>7) & 0x1));

		if(bTxOnly)
			return;
		
		reg = result[final_candidate][6];
		PHY_SetBBReg(dev, 0xc1c, 0x3FF, reg);
	
		reg = result[final_candidate][7] & 0x3F;
		PHY_SetBBReg(dev, 0xc1c, 0xFC00, reg);

		reg = (result[final_candidate][7] >> 6) & 0xF;
		PHY_SetBBReg(dev, 0xc78, 0x0000F000, reg);
	}
}

void
_PHY_SaveADDARegisters(
	struct net_device* dev,
	u32*		ADDAReg,
	u32*		ADDABackup
	)
{
	u32	i;
	
	RTPRINT(FINIT, INIT_IQK, ("Save ADDA parameters.\n"));
	for( i = 0 ; i < IQK_ADDA_REG_NUM ; i++){
		ADDABackup[i] = PHY_QueryBBReg(dev, ADDAReg[i], bMaskDWord);
	}
}


void
_PHY_SaveMACRegisters(
	struct net_device* dev,
	u32*		MACReg,
	u32*		MACBackup
	)
{
	u32	i;
	
	RTPRINT(FINIT, INIT_IQK, ("Save MAC parameters.\n"));
	for( i = 0 ; i < (IQK_MAC_REG_NUM - 1); i++){
		MACBackup[i] = read_nic_byte(dev, MACReg[i]);		
	}
	MACBackup[i] = read_nic_dword(dev, MACReg[i]);		

}

void
_PHY_ReloadADDARegisters(
	struct net_device* dev,
	u32*		ADDAReg,
	u32*		ADDABackup
	)
{
	u32	i;

	RTPRINT(FINIT, INIT_IQK, ("Reload ADDA power saving parameters !\n"));
	for(i = 0 ; i < IQK_ADDA_REG_NUM ; i++){
		PHY_SetBBReg(dev, ADDAReg[i], bMaskDWord, ADDABackup[i]);
	}
}

void
_PHY_ReloadMACRegisters(
	struct net_device* dev,
	u32*		MACReg,
	u32*		MACBackup
	)
{
	u32	i;

	RTPRINT(FINIT, INIT_IQK, ("Reload MAC parameters !\n"));
	for(i = 0 ; i < (IQK_MAC_REG_NUM - 1); i++){
		write_nic_byte(dev, MACReg[i], (u8)MACBackup[i]);
	}
	write_nic_dword(dev, MACReg[i], MACBackup[i]);	
}

void
_PHY_PathADDAOn(
	struct net_device* dev,
	u32*	ADDAReg,
	bool		isPathAOn,
	bool		is2T
	)
{
	u32	pathOn;
	u32	i;

	RTPRINT(FINIT, INIT_IQK, ("ADDA ON.\n"));

	pathOn = isPathAOn ? 0x04db25a4 : 0x0b1b25a4;
	if(false == is2T){
		pathOn = 0x0bdb25a0;
		PHY_SetBBReg(dev, ADDAReg[0], bMaskDWord, 0x0b1b25a0);
	}
	else{
		PHY_SetBBReg(dev, ADDAReg[0], bMaskDWord, pathOn);
	}
	
	for( i = 1 ; i < IQK_ADDA_REG_NUM ; i++){
		PHY_SetBBReg(dev, ADDAReg[i], bMaskDWord, pathOn);
	}
	
}

void
_PHY_MACSettingCalibration(
	struct net_device* dev,
	u32*		MACReg,
	u32*		MACBackup	
	)
{
	u32	i = 0;

	RTPRINT(FINIT, INIT_IQK, ("MAC settings for Calibration.\n"));
	write_nic_byte(dev, MACReg[i], 0x3F);
	
	for(i = 1 ; i < (IQK_MAC_REG_NUM - 1); i++){
		write_nic_byte(dev, MACReg[i], (u8)(MACBackup[i]&(~BIT3)));
	}
	write_nic_byte(dev, MACReg[i], (u8)(MACBackup[i]&(~BIT5)));	

}

void
_PHY_PathAStandBy(
	struct net_device* dev
	)
{
	RTPRINT(FINIT, INIT_IQK, ("Path-A standby mode!\n"));

	PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0x0);
	PHY_SetBBReg(dev, 0x840, bMaskDWord, 0x00010000);
	PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0x80800000);
}

void
_PHY_PIModeSwitch(
	struct net_device* dev,
	bool		PIMode
	)
{
	u32	mode;

	RTPRINT(FINIT, INIT_IQK, ("BB Switch to %s mode!\n", (PIMode ? "PI" : "SI")));

	mode = PIMode ? 0x01000100 : 0x01000000;
	PHY_SetBBReg(dev, 0x820, bMaskDWord, mode);
	PHY_SetBBReg(dev, 0x828, bMaskDWord, mode);
}

/*
return FALSE => do IQK again
*/
bool
_PHY_SimularityCompare(
	struct net_device* dev,
	long 		result[][8],
	u8		 c1,
	u8		 c2
	)
{
	u32		i, j, diff, SimularityBitMap, bound = 0;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8		final_candidate[2] = {0xFF, 0xFF};	
	bool		bResult = true, is2T = IS_92C_SERIAL( priv->card_8192_version);
	
	if(is2T)
		bound = 8;
	else
		bound = 4;

	SimularityBitMap = 0;
	
	for( i = 0; i < bound; i++ )
	{
		diff = (result[c1][i] > result[c2][i]) ? (result[c1][i] - result[c2][i]) : (result[c2][i] - result[c1][i]);
		if (diff > MAX_TOLERANCE)
		{
			if((i == 2 || i == 6) && !SimularityBitMap)
			{
				if(result[c1][i]+result[c1][i+1] == 0)
					final_candidate[(i/4)] = c2;
				else if (result[c2][i]+result[c2][i+1] == 0)
					final_candidate[(i/4)] = c1;
				else
					SimularityBitMap = SimularityBitMap|(1<<i);					
			}
			else
				SimularityBitMap = SimularityBitMap|(1<<i);
		}
	}
	
	if ( SimularityBitMap == 0)
	{
		for( i = 0; i < (bound/4); i++ )
		{
			if(final_candidate[i] != 0xFF)
			{
				for( j = i*4; j < (i+1)*4-2; j++)
					result[3][j] = result[final_candidate[i]][j];
				bResult = false;
			}
		}
		return bResult;
	}
	else if (!(SimularityBitMap & 0x0F))			
	{
		for(i = 0; i < 4; i++)
			result[3][i] = result[c1][i];
		return false;
	}
	else if (!(SimularityBitMap & 0xF0) && is2T)	
	{
		for(i = 4; i < 8; i++)
			result[3][i] = result[c1][i];
		return false;
	}	
	else		
		return false;
	
}

void	
_PHY_IQCalibrate(struct net_device* dev, 	
	long 		result[][8],
	u8		t,
	bool		is2T)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u32			i;
	u8			PathAOK, PathBOK;
	u32			ADDA_REG[IQK_ADDA_REG_NUM] = {	0x85c, 0xe6c, 0xe70, 0xe74,
												0xe78, 0xe7c, 0xe80, 0xe84,
												0xe88, 0xe8c, 0xed0, 0xed4,
												0xed8, 0xedc, 0xee0, 0xeec };
	u32			IQK_MAC_REG[IQK_MAC_REG_NUM] = {
						0x522, 0x550,	0x551,	0x040};
						
#if 0
	const u32	retryCount = 9;
#else
	const u32	retryCount = 2;
#endif

	
	u32 bbvalue;
	bool	isNormal = IS_NORMAL_CHIP(priv->card_8192_version);

	if(t==0)
	{
	 	 bbvalue = PHY_QueryBBReg(dev, 0x800, bMaskDWord);
			RTPRINT(FINIT, INIT_IQK, ("PHY_IQCalibrate()==>0x%x\n",bbvalue));

			RTPRINT(FINIT, INIT_IQK, ("IQ Calibration for %s\n", (is2T ? "2T2R" : "1T1R")));
	
	 	_PHY_SaveADDARegisters(dev, ADDA_REG, priv->ADDA_backup);
		_PHY_SaveMACRegisters(dev, IQK_MAC_REG, priv->IQK_MAC_backup);
	}
	
 	_PHY_PathADDAOn(dev, ADDA_REG, true, is2T);
		
	if(t==0)
	{
		priv->bRfPiEnable = (u8)PHY_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT(8));
	}
	
	if(!priv->bRfPiEnable){
		_PHY_PIModeSwitch(dev, true);
	}
	
	if(t==0)
	{
		priv->RegC04 = PHY_QueryBBReg(dev, 0xc04, bMaskDWord);
		priv->RegC08 = PHY_QueryBBReg(dev, 0xc08, bMaskDWord);
		priv->Reg874 = PHY_QueryBBReg(dev, 0x874, bMaskDWord);
	}
	
	PHY_SetBBReg(dev, 0xc04, bMaskDWord, 0x03a05600);
	PHY_SetBBReg(dev, 0xc08, bMaskDWord, 0x000800e4);
	PHY_SetBBReg(dev, 0x874, bMaskDWord, 0x22204000);

	if(is2T)
	{
		PHY_SetBBReg(dev, 0x840, bMaskDWord, 0x00010000);
		PHY_SetBBReg(dev, 0x844, bMaskDWord, 0x00010000);
	}
	
	_PHY_MACSettingCalibration(dev, IQK_MAC_REG, priv->IQK_MAC_backup);
	
	if(isNormal)
		PHY_SetBBReg(dev, 0xb68, bMaskDWord, 0x00080000);		
	else
		PHY_SetBBReg(dev, 0xb68, bMaskDWord, 0x0f600000);
	
	if(is2T)
	{
		if(isNormal)	
			PHY_SetBBReg(dev, 0xb6c, bMaskDWord, 0x00080000);
		else
			PHY_SetBBReg(dev, 0xb6c, bMaskDWord, 0x0f600000);
	}
	
	RTPRINT(FINIT, INIT_IQK, ("IQK setting!\n"));		
	PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0x80800000);
	PHY_SetBBReg(dev, 0xe40, bMaskDWord, 0x01007c00);
	PHY_SetBBReg(dev, 0xe44, bMaskDWord, 0x01004800);

	for(i = 0 ; i < retryCount ; i++){
		PathAOK = _PHY_PathA_IQK(dev, is2T);
		if(PathAOK == 0x03){
			RTPRINT(FINIT, INIT_IQK, ("Path A IQK Success!!\n"));
				result[t][0] = (PHY_QueryBBReg(dev, 0xe94, bMaskDWord)&0x3FF0000)>>16;
				result[t][1] = (PHY_QueryBBReg(dev, 0xe9c, bMaskDWord)&0x3FF0000)>>16;
				result[t][2] = (PHY_QueryBBReg(dev, 0xea4, bMaskDWord)&0x3FF0000)>>16;
				result[t][3] = (PHY_QueryBBReg(dev, 0xeac, bMaskDWord)&0x3FF0000)>>16;
			break;
		}
		else if (i == (retryCount-1) && PathAOK == 0x01)	
		{
			RTPRINT(FINIT, INIT_IQK, ("Path A IQK Only  Tx Success!!\n"));
			
			result[t][0] = (PHY_QueryBBReg(dev, 0xe94, bMaskDWord)&0x3FF0000)>>16;
			result[t][1] = (PHY_QueryBBReg(dev, 0xe9c, bMaskDWord)&0x3FF0000)>>16;			
		}
	}

	if(0x00 == PathAOK){		
		RTPRINT(FINIT, INIT_IQK, ("Path A IQK failed!!\n"));		
	}

	if(is2T){
		_PHY_PathAStandBy(dev);

		_PHY_PathADDAOn(dev, ADDA_REG, false, is2T);

		for(i = 0 ; i < retryCount ; i++){
			PathBOK = _PHY_PathB_IQK(dev);
			if(PathBOK == 0x03){
				RTPRINT(FINIT, INIT_IQK, ("Path B IQK Success!!\n"));
				result[t][4] = (PHY_QueryBBReg(dev, 0xeb4, bMaskDWord)&0x3FF0000)>>16;
				result[t][5] = (PHY_QueryBBReg(dev, 0xebc, bMaskDWord)&0x3FF0000)>>16;
				result[t][6] = (PHY_QueryBBReg(dev, 0xec4, bMaskDWord)&0x3FF0000)>>16;
				result[t][7] = (PHY_QueryBBReg(dev, 0xecc, bMaskDWord)&0x3FF0000)>>16;
				break;
			}
			else if (i == (retryCount - 1) && PathBOK == 0x01)	
			{
				RTPRINT(FINIT, INIT_IQK, ("Path B Only Tx IQK Success!!\n"));
				result[t][4] = (PHY_QueryBBReg(dev, 0xeb4, bMaskDWord)&0x3FF0000)>>16;
				result[t][5] = (PHY_QueryBBReg(dev, 0xebc, bMaskDWord)&0x3FF0000)>>16;				
			}
		}

		if(0x00 == PathBOK){		
			RTPRINT(FINIT, INIT_IQK, ("Path B IQK failed!!\n"));			
		}
	}



#if 0
	_PHY_PathAFillIQKMatrix(pAdapter,bPathAOK);
	if(is2T){
		_PHY_PathBFillIQKMatrix(pAdapter,bPathBOK);
	}
#endif
	RTPRINT(FINIT, INIT_IQK, ("Back to BB mode, load original value!\n"));
	PHY_SetBBReg(dev, 0xc04, bMaskDWord, priv->RegC04);
	PHY_SetBBReg(dev, 0x874, bMaskDWord, priv->Reg874);
	PHY_SetBBReg(dev, 0xc08, bMaskDWord, priv->RegC08);

	PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0);

	PHY_SetBBReg(dev, 0x840, bMaskDWord, 0x00032ed3);
	if(is2T){
		PHY_SetBBReg(dev, 0x844, bMaskDWord, 0x00032ed3);
	}
	
	if(t!=0)
	{
		if(!priv->bRfPiEnable){
		_PHY_PIModeSwitch(dev, false);
	}

	_PHY_ReloadADDARegisters(dev, ADDA_REG, priv->ADDA_backup);

		_PHY_ReloadMACRegisters(dev, IQK_MAC_REG, priv->IQK_MAC_backup);
		
	}
	RTPRINT(FINIT, INIT_IQK, ("_PHY_IQCalibrate() <==\n"));
	
}


void	
_PHY_LCCalibrate(
	struct net_device* dev, 	
	bool		is2T
	)
{
	u8	tmpReg;
	u32 	RF_Amode = 0, RF_Bmode = 0, LC_Cal;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool	isNormal = IS_NORMAL_CHIP(priv->card_8192_version);

	tmpReg = read_nic_byte(dev, 0xd03);

	if((tmpReg&0x70) != 0)			
		write_nic_byte(dev, 0xd03, tmpReg&0x8F);	
	else 							
		write_nic_byte(dev, 0x42, 0xFF);			

	if((tmpReg&0x70) != 0)
	{
		RF_Amode = PHY_QueryRFReg(dev, RF90_PATH_A, 0x00, bMask12Bits);

		if(is2T)
			RF_Bmode = PHY_QueryRFReg(dev, RF90_PATH_B, 0x00, bMask12Bits);	

		PHY_SetRFReg(dev, RF90_PATH_A, 0x00, bMask12Bits, (RF_Amode&0x8FFFF)|0x10000);

		if(is2T)
			PHY_SetRFReg(dev, RF90_PATH_B, 0x00, bMask12Bits, (RF_Bmode&0x8FFFF)|0x10000);			
	}
	
	LC_Cal = PHY_QueryRFReg(dev, RF90_PATH_A, 0x18, bMask12Bits);
	
	PHY_SetRFReg(dev, RF90_PATH_A, 0x18, bMask12Bits, LC_Cal|0x08000);

	if(isNormal)
		mdelay(100);		
	else
		mdelay(3);

	if((tmpReg&0x70) != 0)	
	{  
		write_nic_byte(dev, 0xd03, tmpReg);
		PHY_SetRFReg(dev, RF90_PATH_A, 0x00, bMask12Bits, RF_Amode);
		
		if(is2T)
			PHY_SetRFReg(dev, RF90_PATH_B, 0x00, bMask12Bits, RF_Bmode);
	}
	else 
	{
		write_nic_byte(dev, 0x42, 0x00);	
	}
}



#define		APK_BB_REG_NUM	5
#define		APK_AFE_REG_NUM	16
#define		APK_CURVE_REG_NUM 4
#define		PATH_NUM		2

void	
_PHY_APCalibrate(
	struct net_device* dev, 	
	char 		delta,
	bool		is2T
	)
{
#if 1
	struct r8192_priv 	*priv = rtllib_priv(dev);

	u32 			regD[PATH_NUM];
	u32			tmpReg, index, offset, path, i, pathbound = PATH_NUM;
			
	u32			BB_backup[APK_BB_REG_NUM];
	u32			BB_REG[APK_BB_REG_NUM] = {	
						0x904, 0xc04, 0x800, 0xc08, 0x874 };
	u32			BB_AP_MODE[APK_BB_REG_NUM] = {	
						0x00000020, 0x00a05430, 0x02040000, 
						0x000800e4, 0x00204000 };
	u32			BB_normal_AP_MODE[APK_BB_REG_NUM] = {	
						0x00000020, 0x00a05430, 0x02040000, 
						0x000800e4, 0x22204000 };						

	u32			AFE_backup[APK_AFE_REG_NUM];
	u32			AFE_REG[APK_AFE_REG_NUM] = {	
						0x85c, 0xe6c, 0xe70, 0xe74, 0xe78, 
						0xe7c, 0xe80, 0xe84, 0xe88, 0xe8c, 
						0xed0, 0xed4, 0xed8, 0xedc, 0xee0,
						0xeec};

	u32			MAC_backup[IQK_MAC_REG_NUM];
	u32			MAC_REG[IQK_MAC_REG_NUM] = {
						0x522, 0x550, 0x551, 0x040};

	u32			APK_RF_init_value[PATH_NUM][APK_BB_REG_NUM] = {
					{0x0852c, 0x1852c, 0x5852c, 0x1852c, 0x5852c},
					{0x2852e, 0x0852e, 0x3852e, 0x0852e, 0x0852e}
					};	

	u32			APK_normal_RF_init_value[PATH_NUM][APK_BB_REG_NUM] = {
					{0x0852c, 0x5a52c, 0x0a52c, 0x5a52c, 0x4a52c},	
					{0x0852c, 0x5a52c, 0x0a52c, 0x5a52c, 0x4a52c}
					};
	
	u32			APK_RF_value_0[PATH_NUM][APK_BB_REG_NUM] = {
					{0x52019, 0x52014, 0x52013, 0x5200f, 0x5208d},
					{0x5201a, 0x52019, 0x52016, 0x52033, 0x52050}
					};

	u32			APK_normal_RF_value_0[PATH_NUM][APK_BB_REG_NUM] = {
					{0x52019, 0x52017, 0x52010, 0x5200d, 0x5200a},	
					{0x52019, 0x52017, 0x52010, 0x5200d, 0x5200a}
					};
	
	u32			APK_RF_value_A[PATH_NUM][APK_BB_REG_NUM] = {
					{0x1adb0, 0x1adb0, 0x1ada0, 0x1ad90, 0x1ad80},		
					{0x00fb0, 0x00fb0, 0x00fa0, 0x00f90, 0x00f80}						
					};

	u32			AFE_on_off[PATH_NUM] = {
					0x04db25a4, 0x0b1b25a4};	

	u32			APK_offset[PATH_NUM] = {
					0xb68, 0xb6c};

	u32			APK_normal_offset[PATH_NUM] = {
					0xb28, 0xb98};
					
	u32			APK_value[PATH_NUM] = {
					0x92fc0000, 0x12fc0000};					

	u32			APK_normal_value[PATH_NUM] = {
					0x92680000, 0x12680000};					

	char			APK_delta_mapping[APK_BB_REG_NUM][13] = {
					{-4, -3, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},
					{-4, -3, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},											
					{-6, -4, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},
					{-1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6},
					{-11, -9, -7, -5, -3, -1, 0, 0, 0, 0, 0, 0, 0}
					};
	
	u32			APK_normal_setting_value_1[13] = {
					0x01017018, 0xf7ed8f84, 0x40372d20, 0x5b554e48, 0x6f6a6560,
					0x807c7873, 0x8f8b8884, 0x9d999693, 0xa9a6a3a0, 0xb5b2afac,
					0x12680000, 0x00880000, 0x00880000
					};

	u32			APK_normal_setting_value_2[16] = {
					0x00810100, 0x00400056, 0x002b0032, 0x001f0024, 0x0019001c,
					0x00150017, 0x00120013, 0x00100011, 0x000e000f, 0x000c000d,
					0x000b000c, 0x000a000b, 0x0009000a, 0x00090009, 0x00080008,
					0x00080008
					};
	
	u32			APK_result[PATH_NUM][APK_BB_REG_NUM];	
	u32			AP_curve[PATH_NUM][APK_CURVE_REG_NUM];

	long			BB_offset, delta_V, delta_offset;

	bool			isNormal = IS_NORMAL_CHIP(priv->card_8192_version);

#if 0
	PMPT_CONTEXT	pMptCtx = &(pAdapter->MptCtx);	

	pMptCtx->APK_bound[0] = 45;
	pMptCtx->APK_bound[1] = 52;		
#endif

	RTPRINT(FINIT, INIT_IQK, ("==>PHY_APCalibrate() delta %d\n", delta));
	
	RTPRINT(FINIT, INIT_IQK, ("AP Calibration for %s %s\n", (is2T ? "2T2R" : "1T1R"), (isNormal ? "Normal chip" : "Test chip")));

	if(!is2T)
		pathbound = 1;

	if(isNormal)	
	{
#if 1
		return;
#endif
	
		for(index = 0; index < PATH_NUM; index ++)
		{
 			APK_offset[index] = APK_normal_offset[index];
			APK_value[index] = APK_normal_value[index];
			AFE_on_off[index] = 0x6fdb25a4;
		}

		for(index = 0; index < APK_BB_REG_NUM; index ++)
		{
			for(path = 0; path < pathbound; path++)
			{
				APK_RF_init_value[path][index] = APK_normal_RF_init_value[path][index];
				APK_RF_value_0[path][index] = APK_normal_RF_value_0[path][index];
			}
			BB_AP_MODE[index] = BB_normal_AP_MODE[index];
		}			
	}
	else
	{
		PHY_SetBBReg(dev, 0xb68, bMaskDWord, 0x0fe00000);
		if(is2T)
			PHY_SetBBReg(dev, 0xb68, bMaskDWord, 0x0fe00000);
	}
	
	for(index = 0; index < APK_BB_REG_NUM ; index++)
	{
		if(index == 0 && isNormal)		
			continue;				
		BB_backup[index] = PHY_QueryBBReg(dev, BB_REG[index], bMaskDWord);
	}
	
	_PHY_SaveMACRegisters(dev, MAC_REG, MAC_backup);
	
	_PHY_SaveADDARegisters(dev, AFE_REG, AFE_backup);

	for(path = 0; path < pathbound; path++)
	{
		if(isNormal)
		{
			if(path == RF90_PATH_A)
			{
				offset = 0xb00;
				for(index = 0; index < 11; index ++)			
				{
					PHY_SetBBReg(dev, offset, bMaskDWord, APK_normal_setting_value_1[index]);
					RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", offset, PHY_QueryBBReg(dev, offset, bMaskDWord))); 	
					
					offset += 0x04;
				}
				
				PHY_SetBBReg(dev, 0xb98, bMaskDWord, 0x12680000);
				
				offset = 0xb68;
				for(; index < 13; index ++) 		
				{
					PHY_SetBBReg(dev, offset, bMaskDWord, APK_normal_setting_value_1[index]);
					RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", offset, PHY_QueryBBReg(dev, offset, bMaskDWord))); 	
					
					offset += 0x04;
				}	
				
				PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0x40000000);
				
				offset = 0xb00;
				for(index = 0; index < 16; index++)
				{
					PHY_SetBBReg(dev, offset, bMaskDWord, APK_normal_setting_value_2[index]);		
					RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", offset, PHY_QueryBBReg(dev, offset, bMaskDWord))); 	
					
					offset += 0x04;
				}				
				PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0x00000000);							
			}
			else if(path == RF90_PATH_B)
			{
				offset = 0xb70;
				for(index = 0; index < 10; index ++)			
				{
					PHY_SetBBReg(dev, offset, bMaskDWord, APK_normal_setting_value_1[index]);
					RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", offset, PHY_QueryBBReg(dev, offset, bMaskDWord))); 	
					
					offset += 0x04;
				}
				PHY_SetBBReg(dev, 0xb28, bMaskDWord, 0x12680000);
				
				PHY_SetBBReg(dev, 0xb98, bMaskDWord, 0x12680000);
				
				offset = 0xb68;
				index = 11;
				for(; index < 13; index ++) 
				{
					PHY_SetBBReg(dev, offset, bMaskDWord, APK_normal_setting_value_1[index]);
					RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", offset, PHY_QueryBBReg(dev, offset, bMaskDWord))); 	
					
					offset += 0x04;
				}	
				
				PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0x40000000);
				
				offset = 0xb60;
				for(index = 0; index < 16; index++)
				{
					PHY_SetBBReg(dev, offset, bMaskDWord, APK_normal_setting_value_2[index]);		
					RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", offset, PHY_QueryBBReg(dev, offset, bMaskDWord))); 	
					
					offset += 0x04;
				}				
				PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0x00000000);							
			}
		
			tmpReg = PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)path, 0x3, bMaskDWord);
			AP_curve[path][0] = tmpReg & 0x1F;				

			tmpReg = PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)path, 0x4, bMaskDWord);			
			AP_curve[path][1] = (tmpReg & 0xF8000) >> 15; 	
			AP_curve[path][2] = (tmpReg & 0x7C00) >> 10;	
			AP_curve[path][3] = (tmpReg & 0x3E0) >> 5;		
		}
		else
		{
			tmpReg = PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)path, 0xe, bMaskDWord);
		
			AP_curve[path][0] = (tmpReg & 0xF8000) >> 15; 	
			AP_curve[path][1] = (tmpReg & 0x7C00) >> 10;	
			AP_curve[path][2] = (tmpReg & 0x3E0) >> 5;		
			AP_curve[path][3] = tmpReg & 0x1F;				
		}
		
		regD[path] = PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)path, 0xd, bMaskDWord);
		
		for(index = 0; index < APK_AFE_REG_NUM ; index++)
			PHY_SetBBReg(dev, AFE_REG[index], bMaskDWord, AFE_on_off[path]);
		RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0xe70 %x\n", PHY_QueryBBReg(dev, 0xe70, bMaskDWord)));		

		if(path == 0)
		{				
			for(index = 0; index < APK_BB_REG_NUM ; index++)
			{
				if(index == 0 && isNormal)		
					continue;			
				PHY_SetBBReg(dev, BB_REG[index], bMaskDWord, BB_AP_MODE[index]);
			}
		}

		RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x800 %x\n", PHY_QueryBBReg(dev, 0x800, bMaskDWord)));				

		_PHY_MACSettingCalibration(dev, MAC_REG, MAC_backup);
		
		if(path == 0)	
		{
			PHY_SetRFReg(dev, RF90_PATH_B, 0x0, bMaskDWord, 0x10000);			
		}
		else			
		{
			PHY_SetRFReg(dev, RF90_PATH_A, 0x00, bMaskDWord, 0x10000);			
			PHY_SetRFReg(dev, RF90_PATH_A, 0x10, bMaskDWord, 0x1000f);			
			PHY_SetRFReg(dev, RF90_PATH_A, 0x11, bMaskDWord, 0x20103);						
		}

		delta_offset = ((delta+14)/2);
		if(delta_offset < 0)
			delta_offset = 0;
		else if (delta_offset > 12)
			delta_offset = 12;
			
		for(index = 0; index < APK_BB_REG_NUM; index++)
		{
			if(index == 0 && isNormal)		
				continue;
					
			tmpReg = APK_RF_init_value[path][index];
#if 1			
			if(!priv->bAPKThermalMeterIgnore)
			{
				BB_offset = (tmpReg & 0xF0000) >> 16;

				if(!(tmpReg & BIT15)) 
				{
					BB_offset = -BB_offset;
				}

				delta_V = APK_delta_mapping[index][delta_offset];
				
				BB_offset += delta_V;

				
				if(BB_offset < 0)
				{
					tmpReg = tmpReg & (~BIT15);
					BB_offset = -BB_offset;
				}
				else
				{
					tmpReg = tmpReg | BIT15;
				}
				tmpReg = (tmpReg & 0xFFF0FFFF) | (BB_offset << 16);
			}
#endif


			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0xc, bMaskDWord, 0x8992f);
			RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0xc %x\n", PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)path, 0xc, bMaskDWord)));		

			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0x0, bMaskDWord, APK_RF_value_0[path][index]);
			RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x0 %x\n", PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)path, 0x0, bMaskDWord)));		
			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0xd, bMaskDWord, tmpReg);
			RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0xd %x\n", PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)path, 0xd, bMaskDWord)));					
			if(!isNormal)
			{
				PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0xa, bMaskDWord, APK_RF_value_A[path][index]);
				RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0xa %x\n", PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)path, 0xa, bMaskDWord)));					
			}
			
			i = 0;
			do
			{
				PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0x80000000);
				{
					PHY_SetBBReg(dev, APK_offset[path], bMaskDWord, APK_value[0]);		
					RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", APK_offset[path], PHY_QueryBBReg(dev, APK_offset[path], bMaskDWord)));
					mdelay(3);				
					PHY_SetBBReg(dev, APK_offset[path], bMaskDWord, APK_value[1]);
					RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", APK_offset[path], PHY_QueryBBReg(dev, APK_offset[path], bMaskDWord)));
					if(isNormal)
						mdelay(20);
					else
						mdelay(3);
				}
				PHY_SetBBReg(dev, 0xe28, bMaskDWord, 0x00000000);
				tmpReg = PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)path, 0xb, bMaskDWord);
				RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0xb %x\n", tmpReg));		
				
				tmpReg = (tmpReg & 0x3E00) >> 9;
				i++;
			}
			while(tmpReg > 12 && i < 4);

			APK_result[path][index] = tmpReg;
		}
	}

	_PHY_ReloadMACRegisters(dev, MAC_REG, MAC_backup);
	
	for(index = 0; index < APK_BB_REG_NUM ; index++)
	{
		if(index == 0 && isNormal)		
			continue;					
		PHY_SetBBReg(dev, BB_REG[index], bMaskDWord, BB_backup[index]);
	}

	_PHY_ReloadADDARegisters(dev, AFE_REG, AFE_backup);

	for(path = 0; path < pathbound; path++)
	{
		PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0xd, bMaskDWord, regD[path]);
		if(path == RF90_PATH_B)
		{
			PHY_SetRFReg(dev, RF90_PATH_A, 0x10, bMaskDWord, 0x1000f);			
			PHY_SetRFReg(dev, RF90_PATH_A, 0x11, bMaskDWord, 0x20101);						
		}
#if 1
		if(!isNormal)
		{
			for(index = 0; index < APK_BB_REG_NUM ; index++)
			{
				if(APK_result[path][index] > 12)
					APK_result[path][index] = AP_curve[path][index-1];
				RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", index, APK_result[path][index]));
			}
		}
		else
		{		
			if(APK_result[path][1] < 1)
				APK_result[path][1] = 1;
			else if (APK_result[path][1] > 5)
				APK_result[path][1] = 5;
			RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", 1, APK_result[path][1]));			

			if(APK_result[path][2] < 2)
				APK_result[path][2] = 2;
			else if (APK_result[path][2] > 6)
				APK_result[path][2] = 6;			
			RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", 2, APK_result[path][2]));			

			if(APK_result[path][3] < 2)
				APK_result[path][3] = 2;
			else if (APK_result[path][3] > 6)
				APK_result[path][3] = 6;			
			RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", 3, APK_result[path][3]));			

			if(APK_result[path][4] < 5)
				APK_result[path][4] = 5;
			else if (APK_result[path][4] > 9)
				APK_result[path][4] = 9;			
			RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", 4, APK_result[path][4]));			
		
		}
#endif		
		
		
	}

	RTPRINT(FINIT, INIT_IQK, ("\n"));
	

	for(path = 0; path < pathbound; path++)
	{
		if(isNormal)
		{
			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0x3, bMaskDWord, 
			((APK_result[path][1] << 15) | (APK_result[path][1] << 10) | (APK_result[path][1] << 5) | APK_result[path][1]));
			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0x4, bMaskDWord, 
			((APK_result[path][1] << 15) | (APK_result[path][1] << 10) | (APK_result[path][2] << 5) | APK_result[path][3]));		
			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0xe, bMaskDWord, 
			((APK_result[path][4] << 15) | (APK_result[path][4] << 10) | (APK_result[path][4] << 5) | APK_result[path][4]));			
		}
		else
		{
			for(index = 0; index < 2; index++)
				priv->APKoutput[path][index] = ((APK_result[path][index] << 15) | (APK_result[path][2] << 10) | (APK_result[path][3] << 5) | APK_result[path][4]);

#if 0
			if(pMptCtx->TxPwrLevel[path] > pMptCtx->APK_bound[path])	
			{
				PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0xe, bMaskDWord, 
				pHalData->APKoutput[path][0]);
			}
			else
			{
				PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0xe, bMaskDWord, 
				pHalData->APKoutput[path][1]);		
			}
#else
			PHY_SetRFReg(dev, (RF90_RADIO_PATH_E)path, 0xe, bMaskDWord, 
			priv->APKoutput[path][0]);
#endif
		}
	}

	priv->bAPKdone = true;

	RTPRINT(FINIT, INIT_IQK, ("<==PHY_APCalibrate()\n"));
#endif		
}

void _PHY_SetRFPathSwitch(
	struct net_device* dev, 	
	bool		bMain,
	bool		is2T
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
	
	if(!priv->up)
	{
		PHY_SetBBReg(dev, 0x4C, BIT23, 0x01);
		PHY_SetBBReg(dev, rFPGA0_XAB_RFParameter, BIT13, 0x01);
	}
	
	if(bMain)
		PHY_SetBBReg(dev, rFPGA0_XA_RFInterfaceOE, 0x300, 0x2);	
	else
		PHY_SetBBReg(dev, rFPGA0_XA_RFInterfaceOE, 0x300, 0x1);	


}


bool _PHY_QueryRFPathSwitch(
	struct net_device* dev, 	
	bool		is2T
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
	
	if(!priv->up)
	{
		PHY_SetBBReg(dev, 0x4C, BIT23, 0x01);
		PHY_SetBBReg(dev, rFPGA0_XAB_RFParameter, BIT13, 0x01);
	}


	if(PHY_QueryBBReg(dev, rFPGA0_XA_RFInterfaceOE, 0x300) == 0x02)
		return true;
	else 
		return false;

	
}

#undef IQK_ADDA_REG_NUM
#undef IQK_DELAY_TIME

extern void
PHY_IQCalibrate(
	struct net_device* dev
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	long			result[3][8];
	u8			i, final_candidate;
	bool			bPathAOK, bPathBOK;
	long			RegE94, RegE9C, RegEA4, RegEAC, RegEB4, RegEBC, RegEC4, RegECC, RegTmp = 0;
	bool			is12simular, is13simular, is23simular;
	bool 			bStartContTx = false, bSingleTone = false;

	RTPRINT(FINIT, INIT_IQK, ("IQK:Start!!!\n"));
	
	if(bStartContTx || bSingleTone)
		return;
	
#ifdef DISABLE_BB_RF
	return;
#endif
	for(i = 0; i < 8; i++)
	{
		result[0][i] = 0;
		result[1][i] = 0;
		result[2][i] = 0;
		result[3][i] = 0;

	}
	final_candidate = 0xff;
	bPathAOK = false;
	bPathBOK = false;
	is12simular = false;
	is23simular = false;
	is13simular = false;

	for (i=0; i<3; i++)
	{
	if(IS_92C_SERIAL( priv->card_8192_version)){
			_PHY_IQCalibrate(dev, result, i, true);
	}
	else{
			_PHY_IQCalibrate(dev, result, i, false);
	}
		if(i == 1)
		{
			is12simular = _PHY_SimularityCompare(dev, result, 0, 1);
			if(is12simular)
			{
				final_candidate = 0;
				break;
			}
		}
		if(i == 2)
		{
			is13simular = _PHY_SimularityCompare(dev, result, 0, 2);
			if(is13simular)
			{
				final_candidate = 0;
				break;
			}
			
			is23simular = _PHY_SimularityCompare(dev, result, 1, 2);
			if(is23simular)
				final_candidate = 1;
			else
			{
				for(i = 0; i < 8; i++)
					RegTmp += result[3][i];

				if(RegTmp != 0)
					final_candidate = 3;			
				else
				final_candidate = 0xFF;
	}
}
	}

	for (i=0; i<4; i++)
	{
		RegE94 = result[i][0];
		RegE9C = result[i][1];
		RegEA4 = result[i][2];
		RegEAC = result[i][3];
		RegEB4 = result[i][4];
		RegEBC = result[i][5];
		RegEC4 = result[i][6];
		RegECC = result[i][7];
		RTPRINT(FINIT, INIT_IQK, ("IQK: RegE94=%lx RegE9C=%lx RegEA4=%lx RegEAC=%lx RegEB4=%lx RegEBC=%lx RegEC4=%lx RegECC=%lx\n ", RegE94, RegE9C, RegEA4, RegEAC, RegEB4, RegEBC, RegEC4, RegECC));
	}
	

	if(final_candidate != 0xff)
	{
		priv->RegE94 = RegE94 = result[final_candidate][0];
		priv->RegE9C = RegE9C = result[final_candidate][1];
		RegEA4 = result[final_candidate][2];
		RegEAC = result[final_candidate][3];
		priv->RegEB4 = RegEB4 = result[final_candidate][4];
		priv->RegEBC = RegEBC = result[final_candidate][5];
		RegEC4 = result[final_candidate][6];
		RegECC = result[final_candidate][7];
		RTPRINT(FINIT, INIT_IQK, ("IQK: final_candidate is %x\n",final_candidate));
		RTPRINT(FINIT, INIT_IQK, ("IQK: RegE94=%lx RegE9C=%lx RegEA4=%lx RegEAC=%lx RegEB4=%lx RegEBC=%lx RegEC4=%lx RegECC=%lx\n ", RegE94, RegE9C, RegEA4, RegEAC, RegEB4, RegEBC, RegEC4, RegECC));
		bPathAOK = bPathBOK = true;
	}
	else
	{
		priv->RegE94 = priv->RegEB4 = 0x100;	
		priv->RegE9C = priv->RegEBC = 0x0;		
	}
	
	if((RegE94 != 0)/*&&(RegEA4 != 0)*/)
		_PHY_PathAFillIQKMatrix(dev, bPathAOK, result, final_candidate, (RegEA4 == 0));
	if(IS_92C_SERIAL( priv->card_8192_version)){
		if((RegEB4 != 0)/*&&(RegEC4 != 0)*/)
		_PHY_PathBFillIQKMatrix(dev, bPathBOK, result, final_candidate, (RegEC4 == 0));
	}
	
}

void
PHY_LCCalibrate(
	struct net_device* dev
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool 		bStartContTx = false, bSingleTone = false;

#ifdef DISABLE_BB_RF
	return;
#endif

	if(bStartContTx || bSingleTone)
		return;

	if(IS_92C_SERIAL( priv->card_8192_version)){
		_PHY_LCCalibrate(dev, true);
	}
	else{
		_PHY_LCCalibrate(dev, false);
	}
}

void
PHY_APCalibrate(
	struct net_device* dev,
	char 		delta	
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);

#if DISABLE_BB_RF
	return;
#endif

	if(priv->bAPKdone)
		return;

	
	if(IS_92C_SERIAL( priv->card_8192_version)){
		_PHY_APCalibrate(dev, delta, true);
	}
	else{
		_PHY_APCalibrate(dev, delta, false);
	}
}

void PHY_SetRFPathSwitch(
	struct net_device* dev,
	bool		bMain
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
#ifdef DISABLE_BB_RF
	return;
#endif

	if(IS_92C_SERIAL( priv->card_8192_version)){
		_PHY_SetRFPathSwitch(dev, bMain, true);
	}
	else{
		_PHY_SetRFPathSwitch(dev, bMain, false);
	}
}

bool PHY_QueryRFPathSwitch(	
	struct net_device* dev
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);

#ifdef DISABLE_BB_RF
	return true;
#endif

	if(IS_92C_SERIAL( priv->card_8192_version)){
		return _PHY_QueryRFPathSwitch(dev, true);
	}
	else{
		return _PHY_QueryRFPathSwitch(dev, false);
	}
}


bool
HalSetIO8192C(
	struct net_device* dev,
	IO_TYPE		IOType
)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	bool			bPostProcessing = false;


	RT_TRACE(COMP_CMD, "-->HalSetIO8192C(): Set IO Cmd(%#x), SetIOInProgress(%d)\n", 
		IOType, priv->SetIOInProgress);

	do{
		switch(IOType)
		{
			case IO_CMD_RESUME_DM_BY_SCAN:
				RT_TRACE(COMP_CMD, "[IO CMD] Resume DM after scan.\n");
				bPostProcessing = true;
				break;
			case IO_CMD_PAUSE_DM_BY_SCAN:
				RT_TRACE(COMP_CMD, "[IO CMD] Pause DM before scan.\n");
				bPostProcessing = true;
				break;
			default:				
				break;
		}
	}while(false);

	if(bPostProcessing && !priv->SetIOInProgress)
	{
		priv->SetIOInProgress = true;
		priv->CurrentIOType = IOType; 
	}
	else
	{
		return false;
	}

	phy_SetIO(dev);
	RT_TRACE(COMP_CMD, "<--HalSetIO8192C(): Set IO Type(%#x)\n", IOType);

	return true;
}

void
phy_SetIO(
    struct net_device* dev
    )
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
	if(priv->bResetInProgress)	
	{			
		RT_TRACE(COMP_CMD, "phy_SetIO(): USB can NOT IO return\n");
		priv->SetIOInProgress = false;
		return;
	}

	RT_TRACE(COMP_CMD, "--->phy_SetIO(): Cmd(%#x), SetIOInProgress(%d)\n", 
			priv->CurrentIOType, priv->SetIOInProgress);
			
	switch(priv->CurrentIOType)
	{
		case IO_CMD_RESUME_DM_BY_SCAN:
			DM_DigTable.CurIGValue = priv->initgain_backup.xaagccore1;
			DM_Write_DIG(dev);
			PHY_SetTxPowerLevel8192C(dev, priv->chan);
			break;		
		case IO_CMD_PAUSE_DM_BY_SCAN:
			priv->initgain_backup.xaagccore1 = DM_DigTable.CurIGValue;
			DM_DigTable.CurIGValue = 0x17;
			DM_Write_DIG(dev);
			break;
		default:
			break;
	}	
	priv->SetIOInProgress = false;
	RT_TRACE(COMP_CMD, "<---phy_SetIO(): CurrentIOType(%#x)\n", priv->CurrentIOType);
	
}	

bool
rtl8192ce_phy_SetFwCmdIO(struct net_device* dev, FW_CMD_IO_TYPE		FwCmdIO)
{
	return true;
}
			
bool
SetAntennaConfig92C(
	struct net_device* dev,
	u8			DefaultAnt		
)
{

#ifdef MERGE_TO_DO
	if(!TestStart)
	{
		TestStart = 1;
		rfChg = 1;
	}

	priv->AntennaTest = 1;
	priv->AntennaTxPath = ANTENNA_A;
	if(DefaultAnt == 0)	
	{
		if(priv->AntennaRxPath == ANTENNA_B)	
			rfChg = 1;
		priv->AntennaRxPath = ANTENNA_A;
	}
	else
	{
		if(priv->AntennaRxPath == ANTENNA_A)	
			rfChg = 1;
		priv->AntennaRxPath = ANTENNA_B;
	}

	ulAntennaTx = priv->AntennaTxPath;
	ulAntennaRx = priv->AntennaRxPath;
	
	p_ofdm_tx = (R_ANTENNA_SELECT_OFDM *)&r_ant_select_ofdm_val;
	p_cck_txrx = (R_ANTENNA_SELECT_CCK *)&r_ant_select_cck_val;

	
	p_ofdm_tx->r_ant_ht1			= 0x1;
	p_ofdm_tx->r_ant_ht2			= 0x2;	
	p_ofdm_tx->r_ant_non_ht 		= 0x3;	

	// 原因是Tx 3-wire enable需隨Tx Ant path打開才會開啟，
	// 所以需在設BB 0x824與0x82C時，同時將BB 0x804[3:0]設為3(同時打開Ant. A and B)。
	// 要省電的情況下，A Tx時 0x90C=0x11111111，B Tx時 0x90C=0x22222222，AB同時開就維持原來設定0x3321333
	
	switch(ulAntennaTx)
	{
	case ANTENNA_A:
		p_ofdm_tx->r_tx_antenna		= 0x1;
		r_ofdm_tx_en_val			= 0x1;
		p_ofdm_tx->r_ant_l 			= 0x1;
		p_ofdm_tx->r_ant_ht_s1 		= 0x1;
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x1;
		p_cck_txrx->r_ccktx_enable	= 0x8;
		chgTx = 1;
		if (IS_HARDWARE_TYPE_8192S(dev))
		{
			PHY_SetBBReg(dev, rFPGA0_XA_HSSIParameter2, 0xe, 2);
			PHY_SetBBReg(dev, rFPGA0_XB_HSSIParameter2, 0xe, 1);
			r_ofdm_tx_en_val			= 0x3;
			r_ant_select_ofdm_val = 0x11111111;

			if (priv->rf_type == RF_2T2R)
			{
				PHY_SetBBReg(dev, rFPGA0_XAB_RFInterfaceSW, BIT10, 0);
				PHY_SetBBReg(dev, rFPGA0_XAB_RFInterfaceSW, BIT26, 1);
				PHY_SetBBReg(dev, rFPGA0_XB_RFInterfaceOE, BIT10, 0);
				PHY_SetBBReg(dev, rFPGA0_XAB_RFParameter, BIT1, 1);
				PHY_SetBBReg(dev, rFPGA0_XAB_RFParameter, BIT17, 0);
			}
		}
		break;
	case ANTENNA_B:
		p_ofdm_tx->r_tx_antenna		= 0x2;
		r_ofdm_tx_en_val			= 0x2;
		p_ofdm_tx->r_ant_l 			= 0x2;	
		p_ofdm_tx->r_ant_ht_s1 		= 0x2;	
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x2;
		p_cck_txrx->r_ccktx_enable	= 0x4;
		chgTx = 1;
		if (IS_HARDWARE_TYPE_8192S(dev))
		{
			PHY_SetBBReg(dev, rFPGA0_XA_HSSIParameter2, 0xe, 1);
			PHY_SetBBReg(dev, rFPGA0_XB_HSSIParameter2, 0xe, 2);
			r_ofdm_tx_en_val			= 0x3;
			r_ant_select_ofdm_val = 0x22222222;

			if (priv->rf_type == RF_2T2R)
			{
				PHY_SetBBReg(dev, rFPGA0_XAB_RFInterfaceSW, BIT10, 1);
				PHY_SetBBReg(dev, rFPGA0_XA_RFInterfaceOE, BIT10, 0);
				PHY_SetBBReg(dev, rFPGA0_XAB_RFInterfaceSW, BIT26, 0);
				PHY_SetBBReg(dev, rFPGA0_XAB_RFParameter, BIT1, 0);
				PHY_SetBBReg(dev, rFPGA0_XAB_RFParameter, BIT17, 1);
			}
		}
		break;

	case ANTENNA_AB:	
		p_ofdm_tx->r_tx_antenna		= 0x3;
		r_ofdm_tx_en_val			= 0x3;
		p_ofdm_tx->r_ant_l 			= 0x3;
		p_ofdm_tx->r_ant_ht_s1 		= 0x3;
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x3;
		p_cck_txrx->r_ccktx_enable	= 0xC;
		chgTx = 1;
		if (IS_HARDWARE_TYPE_8192S(dev))
		{
			PHY_SetBBReg(dev, rFPGA0_XA_HSSIParameter2, 0xe, 2);
			PHY_SetBBReg(dev, rFPGA0_XB_HSSIParameter2, 0xe, 2);
			r_ant_select_ofdm_val = 0x3321333;
			
			if (priv->rf_type == RF_2T2R)
			{
				PHY_SetBBReg(dev, rFPGA0_XAB_RFInterfaceSW, BIT10, 0);

				PHY_SetBBReg(dev, rFPGA0_XAB_RFInterfaceSW, BIT26, 0);
				PHY_SetBBReg(dev, rFPGA0_XAB_RFParameter, BIT1, 1);
				PHY_SetBBReg(dev, rFPGA0_XAB_RFParameter, BIT17, 1);
			}
		}
		break;
			default:
				break;
	}
	
	switch(ulAntennaRx)
	{
	case ANTENNA_A:
		r_rx_antenna_ofdm 			= 0x1;	
		p_cck_txrx->r_cckrx_enable 	= 0x0;	
		p_cck_txrx->r_cckrx_enable_2	= 0x0;	
		chgRx = 1;
		break;
	case ANTENNA_B:
		r_rx_antenna_ofdm 			= 0x2;	
		p_cck_txrx->r_cckrx_enable 	= 0x1;	
		p_cck_txrx->r_cckrx_enable_2	= 0x1;	
		chgRx = 1;
		break;
	case ANTENNA_AB:	
		r_rx_antenna_ofdm 			= 0x3;	
		p_cck_txrx->r_cckrx_enable 	= 0x0;	
		p_cck_txrx->r_cckrx_enable_2= 0x1;		
		chgRx = 1;
		break;
	default:
		break;
	}

	
	if(chgTx && chgRx)
	{
		switch(priv->rf_chip)
		{
			case RF_8225:
			case RF_8256:
			case RF_6052:
				r_ant_sel_cck_val = r_ant_select_cck_val;
				if(rfChg)
				{
					PHY_SetBBReg(dev, rFPGA1_TxInfo, 0xffffffff, r_ant_select_ofdm_val);		
					PHY_SetBBReg(dev, rFPGA0_TxInfo, 0x0000000f, r_ofdm_tx_en_val);		
					PHY_SetBBReg(dev, rOFDM0_TRxPathEnable, 0x0000000f, r_rx_antenna_ofdm);	
					PHY_SetBBReg(dev, rOFDM1_TRxPathEnable, 0x0000000f, r_rx_antenna_ofdm);	
					PHY_SetBBReg(dev, rCCK0_AFESetting, bMaskByte3, r_ant_sel_cck_val);		
				}
				break;
			default:
				RT_ASSERT(false, ("Unsupported RFChipID for switching antenna.\n"));
				break;
		}
	}
#endif
	return true;
}

void
FillA2Entry8192C(
	struct net_device* dev,
	u8				index,
	u8*				val
)
{
	u32				A2entry_index;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	
	return;
	
	write_nic_dword(dev, 0x2c4, ((u32*)(val))[0]);
	write_nic_word(dev, 0x2c8, ((u16*)(val+4))[0]);

	A2entry_index = (u32)index;
	priv->rtllib->SetFwCmdHandler(dev, FW_CMD_ADD_A2_ENTRY);
}

void
phy_SetRTL8192CERfSleep(struct net_device* dev)
{
	u32	U4bTmp;
	u8	delay = 5; 

	write_nic_byte(dev, REG_TXPAUSE, 0xFF);

	PHY_SetRFReg(dev, RF90_PATH_A, 0x00, bRFRegOffsetMask, 0x00);

	write_nic_byte(dev, REG_APSD_CTRL, 0x40);

	U4bTmp = PHY_QueryRFReg(dev, RF90_PATH_A, 0 ,bRFRegOffsetMask);
	while(U4bTmp != 0 && delay > 0 )
	{	
		write_nic_byte(dev, REG_APSD_CTRL, 0x0);
		PHY_SetRFReg(dev, RF90_PATH_A, 0x00, bRFRegOffsetMask, 0x00);
		write_nic_byte(dev, REG_APSD_CTRL, 0x40);
		U4bTmp = PHY_QueryRFReg(dev, RF90_PATH_A, 0 ,bRFRegOffsetMask);
		delay--;
	}

	if(delay == 0)
	{
		write_nic_byte(dev, REG_APSD_CTRL, 0x00);
	
#if 1
		write_nic_byte(dev, REG_SYS_FUNC_EN, 0xE2);
		write_nic_byte(dev, REG_SYS_FUNC_EN, 0xE3);
#else
		write_nic_byte(dev, REG_SYS_FUNC_EN, 0x16);
		write_nic_byte(dev, REG_SYS_FUNC_EN, 0x17);
#endif
		write_nic_byte(dev, REG_TXPAUSE, 0x00);
		RT_TRACE(COMP_POWER, "phy_SetRTL8192CERfSleep(): Fail !!! Switch RF timeout.\n");
		return;
	}

#if 1
	write_nic_byte(dev, REG_SYS_FUNC_EN, 0xE2);
#else
	write_nic_byte(dev, REG_SYS_FUNC_EN, 0x16);
#endif

	write_nic_byte(dev, REG_SPS0_CTRL, 0x22);


	
}	


void
phy_SetRTL8192CERfOn(struct net_device* dev)
{

	write_nic_byte(dev, REG_SPS0_CTRL, 0x2b);

#if 1
	write_nic_byte(dev, REG_SYS_FUNC_EN, 0xE3);
#else
	PlatformEFIOWrite1Byte(dev, REG_SYS_FUNC_EN, 0x17);
#endif

	write_nic_byte(dev, REG_APSD_CTRL, 0x00);

#if 1
	write_nic_byte(dev, REG_SYS_FUNC_EN, 0xE2);
	write_nic_byte(dev, REG_SYS_FUNC_EN, 0xE3);
#else
	PlatformEFIOWrite1Byte(dev, REG_SYS_FUNC_EN, 0x16);
	PlatformEFIOWrite1Byte(dev, REG_SYS_FUNC_EN, 0x17);
#endif

	write_nic_byte(dev, REG_TXPAUSE, 0x00);

	
}	


#endif
