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
#include "r8192U_dm.h"
#include "r8192S_rtl6052.h"

#ifdef RTL8192SU
#include "r8192S_hw.h"
#include "r8192S_phy.h"
#include "r8192S_phyreg.h"
#include "r8192SU_HWImg.h"
#else
#include "r8192U_hw.h"
#include "r819xU_phy.h"
#include "r819xU_phyreg.h"
#endif

#ifdef ENABLE_DOT11D
#include "dot11d.h"
#endif

#define MAX_PRECMD_CNT 16
#define MAX_RFDEPENDCMD_CNT 16
#define MAX_POSTCMD_CNT 16
#define MAX_DOZE_WAITING_TIMES_9x 64

#ifndef RTL8192SU
static u32	RF_CHANNEL_TABLE_ZEBRA[]={
		0,
		0x085c,
		0x08dc,
		0x095c,
		0x09dc,
		0x0a5c,
		0x0adc,
		0x0b5c,
		0x0bdc,
		0x0c5c,
		0x0cdc,
		0x0d5c,
		0x0ddc,
		0x0e5c,
		0x0f72,
};
#endif

static	u32 
phy_CalculateBitShift(u32 BitMask);
static	RT_STATUS 
phy_ConfigMACWithHeaderFile(struct net_device* dev);
static void 
phy_InitBBRFRegisterDefinition(struct net_device* dev);
static	RT_STATUS 
phy_BB8192S_Config_ParaFile(struct net_device* dev);
static	RT_STATUS 
phy_ConfigBBWithHeaderFile(struct net_device* dev,u8 ConfigType);
static bool 
phy_SetRFPowerState8192SU(struct net_device* dev,RT_RF_POWER_STATE eRFPowerState);
void 	
SetBWModeCallback8192SUsbWorkItem(struct net_device *dev);
void 
SetBWModeCallback8192SUsbWorkItem(struct net_device *dev);
void 
SwChnlCallback8192SUsbWorkItem(struct net_device *dev );
static void 
phy_FinishSwChnlNow(struct net_device* dev,u8 channel);
static bool
phy_SwChnlStepByStep(
	struct net_device* dev,
	u8		channel,
	u8		*stage,
	u8		*step,
	u32		*delay
	);
static RT_STATUS 
phy_ConfigBBWithPgHeaderFile(struct net_device* dev,u8 ConfigType);
#ifdef RTL8192SE
static  u32 phy_FwRFSerialRead( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset); 
static u32 phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset);
static	void phy_FwRFSerialWrite( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset, u32 Data);
static void phy_RFSerialWrite( struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 Offset, u32 Data);
#endif
static long phy_TxPwrIdxToDbm( struct net_device* dev, WIRELESS_MODE   WirelessMode, u8 TxPwrIdx);
static u8 phy_DbmToTxPwrIdx( struct net_device* dev, WIRELESS_MODE WirelessMode, long PowerInDbm);
void phy_SetFwCmdIOCallback(struct net_device* dev);
		
#ifdef RTL8192SU
u32 phy_QueryUsbBBReg(struct net_device* dev, u32	RegAddr)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32	ReturnValue = 0xffffffff;
	u8	PollingCnt = 50;
	u8	BBWaitCounter = 0;
	

	while(priv->bChangeBBInProgress)
	{
		BBWaitCounter ++;
		RT_TRACE(COMP_RF, "phy_QueryUsbBBReg(): Wait 1 ms (%d times)...\n", BBWaitCounter);
		msleep(1); 

		if((BBWaitCounter > 100) )
		{
			RT_TRACE(COMP_RF, "phy_QueryUsbBBReg(): (%d) Wait too logn to query BB!!\n", BBWaitCounter);
			return ReturnValue;
		}
	}

	priv->bChangeBBInProgress = true;
	
	read_nic_dword(dev, RegAddr);	

	do
	{
		if((read_nic_byte(dev, PHY_REG)&HST_RDBUSY) == 0)
			break;
	}while( --PollingCnt );

	if(PollingCnt == 0)
	{
		RT_TRACE(COMP_RF, "Fail!!!phy_QueryUsbBBReg(): RegAddr(%#x) = %#x\n", RegAddr, ReturnValue);
	}
	else
	{
		ReturnValue = read_nic_dword(dev, PHY_REG_DATA);	
		RT_TRACE(COMP_RF, "phy_QueryUsbBBReg(): RegAddr(%#x) = %#x, PollingCnt(%d)\n", RegAddr, ReturnValue, PollingCnt);
	}
	
	priv->bChangeBBInProgress = false;

	return ReturnValue;
}



void
phy_SetUsbBBReg(struct net_device* dev,u32	RegAddr,u32 Data)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8	BBWaitCounter = 0;
	
	RT_TRACE(COMP_RF, "phy_SetUsbBBReg(): RegAddr(%#x) <= %#x\n", RegAddr, Data);

	while(priv->bChangeBBInProgress)
	{
		BBWaitCounter ++;
		RT_TRACE(COMP_RF, "phy_SetUsbBBReg(): Wait 1 ms (%d times)...\n", BBWaitCounter);
		msleep(1); 
	
		if((BBWaitCounter > 100))
		{
			RT_TRACE(COMP_RF, "phy_SetUsbBBReg(): (%d) Wait too logn to query BB!!\n", BBWaitCounter);
			return;
		}
	}

	priv->bChangeBBInProgress = true;
	write_nic_dword(dev, RegAddr, Data);
	
	priv->bChangeBBInProgress = false;
}



u32 phy_QueryUsbRFReg(	struct net_device* dev, RF90_RADIO_PATH_E eRFPath,	u32	Offset)
{
	
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32	ReturnValue = 0;
	u8	PollingCnt = 50;
	u8	RFWaitCounter = 0;


	while(priv->bChangeRFInProgress)
	{
		down(&priv->rf_sem);

		RFWaitCounter ++;
		RT_TRACE(COMP_RF, "phy_QueryUsbRFReg(): Wait 1 ms (%d times)...\n", RFWaitCounter);
		msleep(1); 

		if((RFWaitCounter > 100)) 
		{
			RT_TRACE(COMP_RF, "phy_QueryUsbRFReg(): (%d) Wait too logn to query BB!!\n", RFWaitCounter);
			return 0xffffffff;
		}
		else
		{
		}
	}

	priv->bChangeRFInProgress = true;


	Offset &= 0x3f; 
	
	write_nic_dword(dev, RF_BB_CMD_ADDR, 0xF0000002|
						(Offset<<8)|	
						(eRFPath<<16)); 	
	
	do
	{
		if(read_nic_dword(dev, RF_BB_CMD_ADDR) == 0)
			break;
	}while( --PollingCnt );

	ReturnValue = read_nic_dword(dev, RF_BB_CMD_DATA);
	
	up(&priv->rf_sem);
	priv->bChangeRFInProgress = false;
	
	RT_TRACE(COMP_RF, "phy_QueryUsbRFReg(): eRFPath(%d), Offset(%#x) = %#x\n", eRFPath, Offset, ReturnValue);
	
	return ReturnValue;

}


void phy_SetUsbRFReg(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32	RegAddr,u32 Data)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	u8	PollingCnt = 50;
	u8	RFWaitCounter = 0;


	while(priv->bChangeRFInProgress)
	{
		down(&priv->rf_sem);

		RFWaitCounter ++;
		RT_TRACE(COMP_RF, "phy_SetUsbRFReg(): Wait 1 ms (%d times)...\n", RFWaitCounter);
		msleep(1); 
	
		if((RFWaitCounter > 100))
		{
			RT_TRACE(COMP_RF, "phy_SetUsbRFReg(): (%d) Wait too logn to query BB!!\n", RFWaitCounter);
			return;
		}
		else
		{
		}
	}

	priv->bChangeRFInProgress = true;

	
	RegAddr &= 0x3f; 
	
	write_nic_dword(dev, RF_BB_CMD_DATA, Data);	
	write_nic_dword(dev, RF_BB_CMD_ADDR, 0xF0000003|
					(RegAddr<<8)| 
					(eRFPath<<16));  
	
	do
	{
		if(read_nic_dword(dev, RF_BB_CMD_ADDR) == 0)
				break;
	}while( --PollingCnt );		

	if(PollingCnt == 0)
	{		
		RT_TRACE(COMP_RF, "phy_SetUsbRFReg(): Set RegAddr(%#x) = %#x Fail!!!\n", RegAddr, Data);
	}

	up(&priv->rf_sem);
	priv->bChangeRFInProgress = false;

}

#endif
						


u32 rtl8192_QueryBBReg(struct net_device* dev, u32 RegAddr, u32 BitMask)
{

  	u32	ReturnValue = 0, OriginalValue, BitShift;

#if (DISABLE_BB_RF == 1)
	return 0;
#endif

	RT_TRACE(COMP_RF, "--->PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x)\n", RegAddr, BitMask);

#ifdef RTL8192SU
	if(IS_BB_REG_OFFSET_92S(RegAddr))
	{

		if((RegAddr & 0x03) != 0)
		{
			printk("%s: Not DWORD alignment!!\n", __FUNCTION__);
			return 0;
		}
	
	OriginalValue = phy_QueryUsbBBReg(dev, RegAddr);
	}
	else
#endif
	{
	OriginalValue = read_nic_dword(dev, RegAddr);
	}

	BitShift = phy_CalculateBitShift(BitMask);
	ReturnValue = (OriginalValue & BitMask) >> BitShift;

	RT_TRACE(COMP_RF, "<---PHY_QueryBBReg(): RegAddr(%#x), BitMask(%#x), OriginalValue(%#x)\n", RegAddr, BitMask, OriginalValue);
	return (ReturnValue);
}

void rtl8192_setBBreg(struct net_device* dev, u32 RegAddr, u32 BitMask, u32 Data)
{
	u32	OriginalValue, BitShift, NewValue;

#if (DISABLE_BB_RF == 1)
	return;
#endif

	RT_TRACE(COMP_RF, "--->PHY_SetBBReg(): RegAddr(%#x), BitMask(%#x), Data(%#x)\n", RegAddr, BitMask, Data);

#ifdef RTL8192SU
	if(IS_BB_REG_OFFSET_92S(RegAddr))
	{
		if((RegAddr & 0x03) != 0)
		{
			printk("%s: Not DWORD alignment!!\n", __FUNCTION__);
			return;
		}
	
		if(BitMask!= bMaskDWord)
		{
			OriginalValue = phy_QueryUsbBBReg(dev, RegAddr);
			BitShift = phy_CalculateBitShift(BitMask);
            NewValue = (((OriginalValue) & (~BitMask))|(Data << BitShift));
			phy_SetUsbBBReg(dev, RegAddr, NewValue);	
		}else
			phy_SetUsbBBReg(dev, RegAddr, Data);
	}	
	else
#endif
	{
		if(BitMask!= bMaskDWord)
		{
			OriginalValue = read_nic_dword(dev, RegAddr);
			BitShift = phy_CalculateBitShift(BitMask);
			NewValue = (((OriginalValue) & (~BitMask)) | (Data << BitShift));
			write_nic_dword(dev, RegAddr, NewValue);	
		}else
			write_nic_dword(dev, RegAddr, Data);	
	}


	return;
}


u32 rtl8192_phy_QueryRFReg(struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask)
{
	u32 Original_Value, Readback_Value, BitShift;
	struct r8192_priv *priv = ieee80211_priv(dev);
	
#if (DISABLE_BB_RF == 1) 
	return 0;
#endif
	
	RT_TRACE(COMP_RF, "--->PHY_QueryRFReg(): RegAddr(%#x), eRFPath(%#x), BitMask(%#x)\n", RegAddr, eRFPath,BitMask);
	printk("%s(): rf_pathmap=%x eRFPath=%x\n", __func__,priv->rf_pathmap, eRFPath);
	
	if (!((priv->rf_pathmap >> eRFPath) & 0x1))
	{
		printk("%s()EEEEEError: rfpath off! rf_pathmap=%x eRFPath=%x\n", __func__,priv->rf_pathmap, eRFPath);
		return 0;
	}
	
	if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))
	{
		printk("%s()EEEEEError: not legal rfpath! eRFPath=%x\n",__func__, eRFPath);
		return 0;
	}
	
	down(&priv->rf_sem);
#ifdef RTL8192SU
	Original_Value = phy_QueryUsbRFReg(dev, eRFPath, RegAddr);	
#else
	if (priv->Rf_Mode == RF_OP_By_FW)
	{	
		Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
	}
	else
	{	
		Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);	   	
	}
#endif
	
	BitShift =  phy_CalculateBitShift(BitMask);
	Readback_Value = (Original_Value & BitMask) >> BitShift;	
	up(&priv->rf_sem);

	
	return (Readback_Value);
}

void rtl8192_phy_SetRFReg(struct net_device* dev, RF90_RADIO_PATH_E eRFPath, u32 RegAddr, u32 BitMask, u32 Data)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	u32 Original_Value, BitShift, New_Value;
#if (DISABLE_BB_RF == 1)
	return;
#endif
	
	RT_TRACE(COMP_RF, "--->PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
		RegAddr, BitMask, Data, eRFPath);

	if (!((priv->rf_pathmap >> eRFPath) & 0x1))
	{
		printk("EEEEEError: rfpath off! rf_pathmap=%x eRFPath=%x\n", priv->rf_pathmap, eRFPath);
		return ;
	}
	if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))
	{
		printk("EEEEEError: not legal rfpath! eRFPath=%x\n", eRFPath);
		return;
	}
	
	down(&priv->rf_sem);
#ifdef RTL8192SU

		if (BitMask != bRFRegOffsetMask) 
		{
			Original_Value = phy_QueryUsbRFReg(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value)&(~BitMask))|(Data<< BitShift));
			phy_SetUsbRFReg(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_SetUsbRFReg(dev, eRFPath, RegAddr, Data);
#else
	if (priv->Rf_Mode == RF_OP_By_FW)	
	{		
		if (BitMask != bRFRegOffsetMask) 
		{
			Original_Value = phy_FwRFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value) & (~BitMask)) | (Data<< BitShift));
		
			phy_FwRFSerialWrite(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_FwRFSerialWrite(dev, eRFPath, RegAddr, Data);		
	}
	else
	{		
		if (BitMask != bRFRegOffsetMask) 
		{
			Original_Value = phy_RFSerialRead(dev, eRFPath, RegAddr);
			BitShift =  phy_CalculateBitShift(BitMask);
			New_Value = (((Original_Value) & (~BitMask)) | (Data<< BitShift));
		
			phy_RFSerialWrite(dev, eRFPath, RegAddr, New_Value);
		}
		else
			phy_RFSerialWrite(dev, eRFPath, RegAddr, Data);
	
	}
#endif	
	up(&priv->rf_sem);
	RT_TRACE(COMP_RF, "<---PHY_SetRFReg(): RegAddr(%#x), BitMask(%#x), Data(%#x), eRFPath(%#x)\n", 
			RegAddr, BitMask, Data, eRFPath);
	
}

#ifdef RTL8192SE
static	u32
phy_FwRFSerialRead(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset	)
{
	u32		retValue = 0;		
#if 0	
	   not execute the scheme in the initial step. Otherwise, RF-R/W will waste
	   much time. This is only for site survey. */
	Data |= ((Offset&0xFF)<<12);
	Data |= ((eRFPath&0x3)<<20);
	Data |= 0x80000000;		
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		if (time++ < 100)
		{
			delay_us(10);
		}
		else
			break;
	}
	PlatformIOWrite4Byte(dev, QPNR, Data);		
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		if (time++ < 100)
		{
			delay_us(10);
		}
		else
			return	(0);
	}
	retValue = PlatformIORead4Byte(dev, RF_DATA);
#endif	
	return	(retValue);

}	

static	void
phy_FwRFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data	)
{
#if 0	
	u8	time = 0;
	DbgPrint("N FW RF CTRL RF-%d OF%02x DATA=%03x\n\r", eRFPath, Offset, Data);
	
	Data |= ((Offset&0xFF)<<12);
	Data |= ((eRFPath&0x3)<<20);
	Data |= 0x400000;
	Data |= 0x80000000;
	
	while (PlatformIORead4Byte(dev, QPNR)&0x80000000)
	{
		if (time++ < 100)
		{
			delay_us(10);
		}
		else
			break;
	}
	PlatformIOWrite4Byte(dev, QPNR, Data);
#endif		
}	

static u32 phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset)
{

	u32						retValue = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;
	u8						RfPiEnable=0;


	Offset &= 0x3f;

	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||	
		priv->rf_chip == RF_6052)
	{
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0x0);

		if(Offset>=31)
		{
			priv->RFReadPageCnt[2]++;
			priv->RfReg0Value[eRFPath] |= 0x140;

			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			priv->RFReadPageCnt[1]++;
			priv->RfReg0Value[eRFPath] |= 0x100;
			priv->RfReg0Value[eRFPath] &= (~0x40);

			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 15;
		}
		else
		{
			priv->RFReadPageCnt[0]++;
			NewOffset = Offset;
	}	
	}	
	else
		NewOffset = Offset;

	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, bLSSIReadAddress, NewOffset);

	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x0);	
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2,  bLSSIReadEdge, 0x1);	
	
	mdelay(1);

	retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	
	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_0222D)
	{
		if (Offset >= 0x10)
		{
			priv->RfReg0Value[eRFPath] &= 0xebf;
		
			rtl8192_setBBreg(
				dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);
		}

		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0xf);
	}
	
	return retValue;	
}



static void
phy_RFSerialWrite(
	struct net_device* dev,
	RF90_RADIO_PATH_E	eRFPath,
	u32				Offset,
	u32				Data
	)
{
	u32					DataAndAddr = 0;
	struct r8192_priv 			*priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32					NewOffset;

	Offset &= 0x3f;

	PHY_RFShadowWrite(dev, eRFPath, Offset, Data);	


	if( 	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_0222D)
	{
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0x0);
				
		if(Offset>=31)
		{
			priv->RFWritePageCnt[2]++;
			priv->RfReg0Value[eRFPath] |= 0x140;
			
			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 30;
			
		}else if(Offset>=16)
		{
			priv->RFWritePageCnt[1]++;
			priv->RfReg0Value[eRFPath] |= 0x100;
			priv->RfReg0Value[eRFPath] &= (~0x40);			
							

			rtl8192_setBBreg(dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);

			NewOffset = Offset - 15;
		}
		else
		{
			priv->RFWritePageCnt[0]++;
			NewOffset = Offset;
	}
	}
	else
		NewOffset = Offset;

	DataAndAddr = (Data<<16) | (NewOffset&0x3f);

	rtl8192_setBBreg(dev, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);


	if(Offset==0x0)
		priv->RfReg0Value[eRFPath] = Data;

 	if(	priv->rf_chip == RF_8256 || 
		priv->rf_chip == RF_8225 ||
		priv->rf_chip == RF_0222D)
	{
		if (Offset >= 0x10)
		{
			if(Offset != 0)
			{
				priv->RfReg0Value[eRFPath] &= 0xebf;
				rtl8192_setBBreg(
				dev, 
				pPhyReg->rf3wireOffset, 
				bMaskDWord, 
				(priv->RfReg0Value[eRFPath] << 16)	);
			}
		}	
		rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00, 0xf);
	}
	
}
#else
#if 0
static	u32
phy_RFSerialRead(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset)
{

	u32						retValue = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;
	u32 						tmplong,tmplong2;
	u32						RfPiEnable=0;
#if 0
	if(pHalData->RFChipID == RF_8225 && Offset > 0x24) 
		return	retValue;
	if(pHalData->RFChipID == RF_8256 && Offset > 0x2D) 
		return	retValue;
#endif
	Offset &= 0x3f;

	NewOffset = Offset;

	tmplong = rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord);
	tmplong2 = rtl8192_QueryBBReg(dev, pPhyReg->rfHSSIPara2, bMaskDWord);
	tmplong2 = (tmplong2 & (~bLSSIReadAddress)) | (NewOffset<<23) | bLSSIReadEdge;	
	
	rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong&(~bLSSIReadEdge));	
	mdelay(1);
	
	rtl8192_setBBreg(dev, pPhyReg->rfHSSIPara2, bMaskDWord, tmplong2);	
	mdelay(1);
	
	rtl8192_setBBreg(dev, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong|bLSSIReadEdge);	
	mdelay(1);

	if(eRFPath == RF90_PATH_A)
		RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT8);
	else if(eRFPath == RF90_PATH_B)
		RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XB_HSSIParameter1, BIT8);
	
	if(RfPiEnable)
	{	
		retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBackPi, bLSSIReadBackData);
	}
	else
	{	
		retValue = rtl8192_QueryBBReg(dev, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
	}
	
	return retValue;	
		
}
4


static	void
phy_RFSerialWrite(struct net_device* dev,RF90_RADIO_PATH_E eRFPath,u32 Offset,u32	Data)
{
	u32						DataAndAddr = 0;
	struct r8192_priv *priv = ieee80211_priv(dev);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &priv->PHYRegDef[eRFPath];
	u32						NewOffset;
	
#if 0
	if(pHalData->RFChipID == RF_8225 && Offset > 0x24) 
		return;
	if(pHalData->RFChipID == RF_8256 && Offset > 0x2D) 
		return;
#endif

	Offset &= 0x3f;

	PHY_RFShadowWrite(dev, eRFPath, Offset, Data);	

		NewOffset = Offset;

	DataAndAddr = ((NewOffset<<20) | (Data&0x000fffff)) & 0x0fffffff;	

	rtl8192_setBBreg(dev, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);

}
#endif
#endif

static u32 phy_CalculateBitShift(u32 BitMask)
{
	u32 i;

	for(i=0; i<=31; i++)
	{
		if ( ((BitMask>>i) &  0x1 ) == 1)
			break;
	}

	return (i);
}


extern bool PHY_MACConfig8192S(struct net_device* dev)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;

#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigMACWithHeaderFile(dev);
#else
	RT_TRACE(COMP_INIT, "Read MACREG.txt\n");
#endif
	return (rtStatus == RT_STATUS_SUCCESS) ? true:false;

}

extern	bool
PHY_BBConfig8192S(struct net_device* dev)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;

	u8 PathMap = 0, index = 0, rf_num = 0;
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	phy_InitBBRFRegisterDefinition(dev);


			rtStatus = phy_BB8192S_Config_ParaFile(dev);


	PathMap = (u8)(rtl8192_QueryBBReg(dev, rFPGA0_TxInfo, 0xf) |
				rtl8192_QueryBBReg(dev, rOFDM0_TRxPathEnable, 0xf));
	priv->rf_pathmap = PathMap;
	for(index = 0; index<4; index++)
	{
		if((PathMap>>index)&0x1)
			rf_num++;
	}

	if((priv->rf_type==RF_1T1R && rf_num!=1) ||
		(priv->rf_type==RF_1T2R && rf_num!=2) ||
		(priv->rf_type==RF_2T2R && rf_num!=2) ||
		(priv->rf_type==RF_2T2R_GREEN && rf_num!=2) ||
		(priv->rf_type==RF_2T4R && rf_num!=4))
	{
		RT_TRACE( COMP_INIT, "PHY_BBConfig8192S: RF_Type(%x) does not match RF_Num(%x)!!\n", priv->rf_type, rf_num);
	}
	return (rtStatus == RT_STATUS_SUCCESS) ? 1:0;
}

extern	bool
PHY_RFConfig8192S(struct net_device* dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	RT_STATUS    		rtStatus = RT_STATUS_SUCCESS;


	priv->rf_chip = RF_6052;

	switch(priv->rf_chip)
	{
	case RF_8225:
	case RF_6052:
		rtStatus = PHY_RF6052_Config(dev);
		break;
		
	case RF_8256:			
		break;
		
	case RF_8258:
		break;
		
	case RF_PSEUDO_11N:
		break;
        default:
            break;
	}

	return (rtStatus == RT_STATUS_SUCCESS) ? 1:0;
}


#ifdef TO_DO_LIST
static RT_STATUS
phy_BB8190_Config_HardCode(struct net_device* dev)
{
	return RT_STATUS_SUCCESS;
}
#endif

static RT_STATUS
phy_SetBBtoDiffRFWithHeaderFile(struct net_device* dev, u8 ConfigType)
{
	int i;
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u32* 			Rtl819XPHY_REGArraytoXTXR_Table;
	u16				PHY_REGArraytoXTXRLen;
	

	if(priv->rf_type == RF_1T1R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T1R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T1RArrayLength;
	} 
	else if(priv->rf_type == RF_1T2R)
	{
		Rtl819XPHY_REGArraytoXTXR_Table = Rtl819XPHY_REG_to1T2R_Array;
		PHY_REGArraytoXTXRLen = PHY_ChangeTo_1T2RArrayLength;
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
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArraytoXTXR_Table[i], Rtl819XPHY_REGArraytoXTXR_Table[i+1], Rtl819XPHY_REGArraytoXTXR_Table[i+2]);		
		}
	}
	else {
		RT_TRACE(COMP_SEND, "phy_SetBBtoDiffRFWithHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
	return RT_STATUS_SUCCESS;
}


static	RT_STATUS
phy_BB8192S_Config_ParaFile(struct net_device* dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;	
	
	RT_TRACE(COMP_INIT, "==>phy_BB8192S_Config_ParaFile\n");

#if RTL8190_Download_Firmware_From_Header
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_2T2R ||
	    priv->rf_type == RF_1T1R ||priv->rf_type == RF_2T2R_GREEN)
	{
		rtStatus = phy_ConfigBBWithHeaderFile(dev,BaseBand_Config_PHY_REG);
		if(priv->rf_type != RF_2T2R && priv->rf_type != RF_2T2R_GREEN)
		{
			rtStatus = phy_SetBBtoDiffRFWithHeaderFile(dev,BaseBand_Config_PHY_REG);
		}
	}else
		rtStatus = RT_STATUS_FAILURE;
#else
	RT_TRACE(COMP_INIT, "RF_Type == %d\n", priv->rf_type);		
	if (priv->rf_type == RF_1T2R || priv->rf_type == RF_2T2R ||
	    priv->rf_type == RF_1T1R ||priv->rf_type == RF_2T2R_GREEN)
	{
		rtStatus = phy_ConfigBBWithParaFile(dev, (char* )&szBBRegFile);
		if(priv->rf_type != RF_2T2R && priv->rf_type != RF_2T2R_GREEN)
		{
			if(priv->rf_type == RF_1T1R)
				rtStatus = phy_SetBBtoDiffRFWithParaFile(dev, (char* )&szBBRegto1T1RFile);
			else if(priv->rf_type == RF_1T2R)
				rtStatus = phy_SetBBtoDiffRFWithParaFile(dev, (char* )&szBBRegto1T2RFile);
		}

	}else
		rtStatus = RT_STATUS_FAILURE;
#endif

	if(rtStatus != RT_STATUS_SUCCESS){
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():Write BB Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	if (priv->AutoloadFailFlag == false)
	{
#if	RTL8190_Download_Firmware_From_Header
		rtStatus = phy_ConfigBBWithPgHeaderFile(dev,BaseBand_Config_PHY_REG);
#else
		rtStatus = phy_ConfigBBWithPgParaFile(dev, (char* )&szBBRegPgFile);
#endif
	}
	if(rtStatus != RT_STATUS_SUCCESS){
		RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile():BB_PG Reg Fail!!");
		goto phy_BB8190_Config_ParaFile_Fail;
	}
	
#if RTL8190_Download_Firmware_From_Header
	rtStatus = phy_ConfigBBWithHeaderFile(dev,BaseBand_Config_AGC_TAB);
#else
	RT_TRACE(COMP_INIT, "phy_BB8192S_Config_ParaFile AGC_TAB.txt\n");
	rtStatus = phy_ConfigBBWithParaFile(Adapter, (char* )&szAGCTableFile);
#endif

	if(rtStatus != RT_STATUS_SUCCESS){
		printk( "phy_BB8192S_Config_ParaFile():AGC Table Fail\n");
		goto phy_BB8190_Config_ParaFile_Fail;
	}


#if 0	
	if(pHalData->VersionID > VERSION_8190_BD)
	{
		u4RegValue = (  pHalData->AntennaTxPwDiff[2]<<8 | 
						pHalData->AntennaTxPwDiff[1]<<4 | 
						pHalData->AntennaTxPwDiff[0]);
		
		PHY_SetBBReg(dev, rFPGA0_TxGainStage, 
			(bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);
    	
		u4RegValue = pHalData->CrystalCap;
		PHY_SetBBReg(dev, rFPGA0_AnalogParameter1, bXtalCap92x, u4RegValue);

	}
#endif

	priv->bCckHighPower = (bool)(rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter2, 0x200));


phy_BB8190_Config_ParaFile_Fail:	
	return rtStatus;
}

static	RT_STATUS
phy_ConfigMACWithHeaderFile(struct net_device* dev)
{
	u32					i = 0;
	u32					ArrayLength = 0;
	u32*					ptrArray;	
	

	{ 
		RT_TRACE(COMP_INIT, "Read Rtl819XMACPHY_Array\n");
		ArrayLength = MAC_2T_ArrayLength;
		ptrArray = Rtl819XMAC_Array;	
	}
		
	for(i = 0 ;i < ArrayLength;i=i+2){ 
		write_nic_byte(dev, ptrArray[i], (u8)ptrArray[i+1]);
	}
	return RT_STATUS_SUCCESS;
}

static	RT_STATUS
phy_ConfigBBWithHeaderFile(struct net_device* dev,u8 ConfigType)
{
	int 		i;
	u32*	Rtl819XPHY_REGArray_Table;
	u32*	Rtl819XAGCTAB_Array_Table;
	u16		PHY_REGArrayLen, AGCTAB_ArrayLen;
	/*if(Adapter->bInHctTest)
	{	
	
		AGCTAB_ArrayLen = AGCTAB_ArrayLengthDTM;
		Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_ArrayDTM;

		if(pHalData->RF_Type == RF_2T4R)
		{
			PHY_REGArrayLen = PHY_REGArrayLengthDTM;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REGArrayDTM;
		}
		else if (pHalData->RF_Type == RF_1T2R)
		{
			PHY_REGArrayLen = PHY_REG_1T2RArrayLengthDTM;
			Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_1T2RArrayDTM;
		}		
	
	}
	else
	*/
	AGCTAB_ArrayLen = AGCTAB_ArrayLength;
	Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_Array;
	PHY_REGArrayLen = PHY_REG_2T2RArrayLength; 
	Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_Array;

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
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArray_Table[i], bMaskDWord, Rtl819XPHY_REGArray_Table[i+1]);		

		}
	}
	else if(ConfigType == BaseBand_Config_AGC_TAB){
		for(i=0;i<AGCTAB_ArrayLen;i=i+2)
		{
			rtl8192_setBBreg(dev, Rtl819XAGCTAB_Array_Table[i], bMaskDWord, Rtl819XAGCTAB_Array_Table[i+1]);		
		}
	}
	return RT_STATUS_SUCCESS;
}

static RT_STATUS
phy_ConfigBBWithPgHeaderFile(struct net_device* dev,u8 ConfigType)
{
	int i;
	u32*	Rtl819XPHY_REGArray_Table_PG;
	u16	PHY_REGArrayPGLen;
	
	PHY_REGArrayPGLen = PHY_REG_Array_PGLength;
	Rtl819XPHY_REGArray_Table_PG = Rtl819XPHY_REG_Array_PG;

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
			rtl8192_setBBreg(dev, Rtl819XPHY_REGArray_Table_PG[i], Rtl819XPHY_REGArray_Table_PG[i+1], Rtl819XPHY_REGArray_Table_PG[i+2]);		
		}
	}else{
		RT_TRACE(COMP_SEND, "phy_ConfigBBWithPgHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n");
	}
	return RT_STATUS_SUCCESS;
	
}	

u8 rtl8192_phy_ConfigRFWithHeaderFile(struct net_device* dev, RF90_RADIO_PATH_E	eRFPath)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	int			i;
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	u32			*Rtl819XRadioA_Array_Table;
	u32			*Rtl819XRadioB_Array_Table;
	u16			RadioA_ArrayLen,RadioB_ArrayLen;

	{	
		RadioA_ArrayLen = RadioA_1T_ArrayLength;
		Rtl819XRadioA_Array_Table=Rtl819XRadioA_Array;
		Rtl819XRadioB_Array_Table=Rtl819XRadioB_Array;
		RadioB_ArrayLen = RadioB_ArrayLength;
	}

	if( priv->rf_type == RF_2T2R_GREEN )
	{
		Rtl819XRadioB_Array_Table = Rtl819XRadioB_GM_Array;
		RadioB_ArrayLen = RadioB_GM_ArrayLength;
	}
	else
	{		
		Rtl819XRadioB_Array_Table = Rtl819XRadioB_Array;
		RadioB_ArrayLen = RadioB_ArrayLength;	
	}
	
	rtStatus = RT_STATUS_SUCCESS;

	
	switch(eRFPath){
		case RF90_PATH_A:
			for(i = 0;i<RadioA_ArrayLen; i=i+2){
				if(Rtl819XRadioA_Array_Table[i] == 0xfe)
					{ 
#ifdef RTL8192SU
						mdelay(1000);
#else
						mdelay(50);
#endif
				}
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
					rtl8192_phy_SetRFReg(dev, eRFPath, Rtl819XRadioA_Array_Table[i], bRFRegOffsetMask, Rtl819XRadioA_Array_Table[i+1]);
					}
			}			
			break;
		case RF90_PATH_B:
			for(i = 0;i<RadioB_ArrayLen; i=i+2){
				if(Rtl819XRadioB_Array_Table[i] == 0xfe)
					{ 
#ifdef RTL8192SU
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
					rtl8192_phy_SetRFReg(dev, eRFPath, Rtl819XRadioB_Array_Table[i], bRFRegOffsetMask, Rtl819XRadioB_Array_Table[i+1]);
					}
			}			
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		default:
			break;
	}

	return rtStatus;

}

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
			rtl8192_phy_SetRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bRFRegOffsetMask, WriteData[i]);
			mdelay(10);
			ulRegRead = rtl8192_phy_QueryRFReg(dev, eRFPath, WriteAddr[HW90_BLOCK_RF], bMaskDWord);				
			mdelay(10);
			break;
			
		default:
			rtStatus = RT_STATUS_FAILURE;
			break;
		}


		if(ulRegRead != WriteData[i])
		{
			RT_TRACE(COMP_ERR, "read back error(read:%x, write:%x)\n", ulRegRead, WriteData[i]);
			rtStatus = RT_STATUS_FAILURE;			
			break;
		}
	}

	return rtStatus;
}

#ifdef TO_DO_LIST
void
PHY_SetRFPowerState8192SUsb(
	struct net_device* dev,
	RF_POWER_STATE	RFPowerState
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool			WaitShutDown = FALSE;
	u32			DWordContent;
	u8				eRFPath;
	BB_REGISTER_DEFINITION_T	*pPhyReg;
	
	if(priv->SetRFPowerStateInProgress == TRUE)
		return;
	
	priv->SetRFPowerStateInProgress = TRUE;
	

	if(RFPowerState==RF_SHUT_DOWN)
	{
		RFPowerState=RF_OFF;
		WaitShutDown=TRUE;
	}
	
	
	priv->RFPowerState = RFPowerState;
	switch( priv->rf_chip )
	{
	case RF_8225:
	case RF_6052:
		switch( RFPowerState )
		{
		case RF_ON:
			break;
	
		case RF_SLEEP:
			break;
	
		case RF_OFF:
			break;
		}
		break;

	case RF_8256:
		switch( RFPowerState )
		{
		case RF_ON:
			break;
	
		case RF_SLEEP:
			break;
	
		case RF_OFF:
			for(eRFPath=(RF90_RADIO_PATH_E)RF90_PATH_A; eRFPath < RF90_PATH_MAX; eRFPath++)
			{
				if (!rtl8192_phy_CheckIsLegalRFPath(dev, eRFPath))		
					continue;	
				
				pPhyReg = &priv->PHYRegDef[eRFPath];
				rtl8192_setBBreg(dev, pPhyReg->rfintfs, bRFSI_RFENV, bRFSI_RFENV);
				rtl8192_setBBreg(dev, pPhyReg->rfintfo, bRFSI_RFENV, 0);
			}
			break;
		}
		break;
		
	case RF_8258:
		break;
	}

	priv->SetRFPowerStateInProgress = FALSE;
}
#endif

#ifdef RTL8192U
void
PHY_UpdateInitialGain(
	struct net_device* dev
	)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	
	switch(priv->rf_chip)
	{
	case RF_8225:
		break;
	case RF_8256:
		break;
	case RF_8258:
		break;
	case RF_PSEUDO_11N:
		break;
	case RF_6052:
		break;
	default:
		RT_TRACE(COMP_DBG, "PHY_UpdateInitialGain(): unknown rf_chip: %#X\n", priv->rf_chip);
		break;
	}
}
#endif

void PHY_GetHWRegOriginalValue(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	priv->MCSTxPowerLevelOriginalOffset[0] =
		rtl8192_QueryBBReg(dev, rTxAGC_Rate18_06, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[1] =
		rtl8192_QueryBBReg(dev, rTxAGC_Rate54_24, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[2] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs03_Mcs00, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[3] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs07_Mcs04, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[4] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs11_Mcs08, bMaskDWord);
	priv->MCSTxPowerLevelOriginalOffset[5] =
		rtl8192_QueryBBReg(dev, rTxAGC_Mcs15_Mcs12, bMaskDWord);

	priv->MCSTxPowerLevelOriginalOffset[6] =
		rtl8192_QueryBBReg(dev, rTxAGC_CCK_Mcs32, bMaskDWord);
	RT_TRACE(COMP_INIT, "Legacy OFDM =%08x/%08x HT_OFDM=%08x/%08x/%08x/%08x\n", 
	priv->MCSTxPowerLevelOriginalOffset[0], priv->MCSTxPowerLevelOriginalOffset[1] ,
	priv->MCSTxPowerLevelOriginalOffset[2], priv->MCSTxPowerLevelOriginalOffset[3] ,
	priv->MCSTxPowerLevelOriginalOffset[4], priv->MCSTxPowerLevelOriginalOffset[5] );

	priv->DefaultInitialGain[0] = rtl8192_QueryBBReg(dev, rOFDM0_XAAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[1] = rtl8192_QueryBBReg(dev, rOFDM0_XBAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[2] = rtl8192_QueryBBReg(dev, rOFDM0_XCAGCCore1, bMaskByte0);
	priv->DefaultInitialGain[3] = rtl8192_QueryBBReg(dev, rOFDM0_XDAGCCore1, bMaskByte0);
	RT_TRACE(COMP_INIT, "Default initial gain (c50=0x%x, c58=0x%x, c60=0x%x, c68=0x%x) \n", 
			priv->DefaultInitialGain[0], priv->DefaultInitialGain[1], 
			priv->DefaultInitialGain[2], priv->DefaultInitialGain[3]);

	priv->framesync = rtl8192_QueryBBReg(dev, rOFDM0_RxDetector3, bMaskByte0);
	priv->framesyncC34 = rtl8192_QueryBBReg(dev, rOFDM0_RxDetector2, bMaskDWord);
	RT_TRACE(COMP_INIT, "Default framesync (0x%x) = 0x%x \n", 
				rOFDM0_RxDetector3, priv->framesync);
}



static void phy_InitBBRFRegisterDefinition(	struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
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
	priv->PHYRegDef[RF90_PATH_C].rfintfo = rFPGA0_XC_RFInterfaceOE;
	priv->PHYRegDef[RF90_PATH_D].rfintfo = rFPGA0_XD_RFInterfaceOE;

	priv->PHYRegDef[RF90_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE; 
	priv->PHYRegDef[RF90_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE; 
	priv->PHYRegDef[RF90_PATH_C].rfintfe = rFPGA0_XC_RFInterfaceOE;
	priv->PHYRegDef[RF90_PATH_D].rfintfe = rFPGA0_XD_RFInterfaceOE;

	priv->PHYRegDef[RF90_PATH_A].rf3wireOffset = rFPGA0_XA_LSSIParameter; 
	priv->PHYRegDef[RF90_PATH_B].rf3wireOffset = rFPGA0_XB_LSSIParameter;
	priv->PHYRegDef[RF90_PATH_C].rf3wireOffset = rFPGA0_XC_LSSIParameter;
	priv->PHYRegDef[RF90_PATH_D].rf3wireOffset = rFPGA0_XD_LSSIParameter;

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
	priv->PHYRegDef[RF90_PATH_C].rfHSSIPara1 = rFPGA0_XC_HSSIParameter1;  
	priv->PHYRegDef[RF90_PATH_D].rfHSSIPara1 = rFPGA0_XD_HSSIParameter1;  

	priv->PHYRegDef[RF90_PATH_A].rfHSSIPara2 = rFPGA0_XA_HSSIParameter2;  
	priv->PHYRegDef[RF90_PATH_B].rfHSSIPara2 = rFPGA0_XB_HSSIParameter2;  
	priv->PHYRegDef[RF90_PATH_C].rfHSSIPara2 = rFPGA0_XC_HSSIParameter2;  
	priv->PHYRegDef[RF90_PATH_D].rfHSSIPara2 = rFPGA0_XD_HSSIParameter2;  

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


bool PHY_SetRFPowerState(struct net_device* dev, RT_RF_POWER_STATE eRFPowerState)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool			bResult = FALSE;

	RT_TRACE(COMP_RF, "---------> PHY_SetRFPowerState(): eRFPowerState(%d)\n", eRFPowerState);

	if(eRFPowerState == priv->ieee80211->eRFPowerState)
	{
		RT_TRACE(COMP_RF, "<--------- PHY_SetRFPowerState(): discard the request for eRFPowerState(%d) is the same.\n", eRFPowerState);
		return bResult;
	}

	bResult = phy_SetRFPowerState8192SU(dev, eRFPowerState);
	
	RT_TRACE(COMP_RF, "<--------- PHY_SetRFPowerState(): bResult(%d)\n", bResult);

	return bResult;
}

static bool phy_SetRFPowerState8192SU(struct net_device* dev,RT_RF_POWER_STATE eRFPowerState)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool			bResult = TRUE;
	u8 		u1bTmp;
	
	if(priv->SetRFPowerStateInProgress == TRUE)
		return FALSE;
	
	priv->SetRFPowerStateInProgress = TRUE;
	
	switch(priv->rf_chip )
	{
		default:
		switch( eRFPowerState )
		{
			case eRfOn:
				write_nic_dword(dev, WFM5, FW_BB_RESET_ENABLE);
				write_nic_word(dev, CMDR, 0x37FC);
				write_nic_byte(dev, PHY_CCA, 0x3);
				write_nic_byte(dev, TXPAUSE, 0x00);
				write_nic_byte(dev, SPS1_CTRL, 0x64);
				break;
    	    
			case eRfSleep:
			case eRfOff:  
			  	if (priv->ieee80211->eRFPowerState == eRfSleep || priv->ieee80211->eRFPowerState == eRfOff)
						break;
				
#ifdef TO_DO_LIST
				for(QueueID = 0, i = 0; QueueID < MAX_TX_QUEUE; )
				{
						ring = &priv->tx_ring[QueueID];
						if(skb_queue_len(&ring->queue) == 0)
					{
						QueueID++;
						continue;
					}
					else
					{
						RT_TRACE(COMP_POWER, "eRf Off/Sleep: %d times TcbBusyQueue[%d] !=0 before doze!\n", (i+1), QueueID);
						udelay(10);
						i++;
					}
					
					if(i >= MAX_DOZE_WAITING_TIMES_9x)
					{
							RT_TRACE(COMP_POWER, "\n\n\n %s(): eRfOff: %d times TcbBusyQueue[%d] != 0 !!!\n\n\n", __FUNCTION__,MAX_DOZE_WAITING_TIMES_9x, QueueID);
						break;
					}
				}			
#endif
				write_nic_dword(dev, WFM5, FW_BB_RESET_DISABLE);				

				u1bTmp = read_nic_byte(dev, LDOV12D_CTRL);
				u1bTmp |= BIT0;
				write_nic_byte(dev, LDOV12D_CTRL, u1bTmp);

				write_nic_byte(dev, SPS1_CTRL, 0x0);
				write_nic_byte(dev, TXPAUSE, 0xFF);

				write_nic_word(dev, CMDR, 0x77FC);
				write_nic_byte(dev, PHY_CCA, 0x0);
				udelay(100);

				write_nic_word(dev, CMDR, 0x37FC);
				udelay(10);

				write_nic_word(dev, CMDR, 0x77FC);
				udelay(10);

				write_nic_word(dev, CMDR, 0x57FC);
				break;

			default:
				bResult = FALSE;
				break;
		} 
		break;
		
	}
		
	if(bResult)
	{
		priv->ieee80211->eRFPowerState = eRFPowerState;
		
		switch(priv->rf_chip )
		{
			case RF_8256:		
			switch(priv->ieee80211->eRFPowerState)
			{
				case eRfOff:
					if(priv->ieee80211->RfOffReason==RF_CHANGE_BY_IPS )
					{
						priv->ieee80211->LedControlHandler(dev,LED_CTL_NO_LINK); 
					}
					else
					{
						priv->ieee80211->LedControlHandler(dev, LED_CTL_POWER_OFF); 
					}
					break;
        		
				case eRfOn:
					if(priv->ieee80211->state == IEEE80211_LINKED)
					{
						priv->ieee80211->LedControlHandler(dev, LED_CTL_LINK); 
					}
					else
					{
						priv->ieee80211->LedControlHandler(dev, LED_CTL_NO_LINK); 
					}
					break;
        		
				default:
					break;
			}

				break;
		
			default:
				RT_TRACE(COMP_RF, "phy_SetRFPowerState8192SU(): Unknown RF type\n");
				break;
		}
	}

	priv->SetRFPowerStateInProgress = FALSE;

	return bResult;
}

 void
PHY_GetTxPowerLevel8192S(
	struct net_device* dev,
	 long*    		powerlevel
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
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

 void PHY_SetTxPowerLevel8192S(struct net_device* dev, u8	channel)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8	powerlevel = (u8)EEPROM_Default_TxPower, powerlevelOFDM24G = 0x10;
	s8 	ant_pwr_diff = 0;
	u32	u4RegValue;
	u8	index = (channel -1);
	u8	pwrdiff[2] = {0};
	u8	ht20pwr[2] = {0}, ht40pwr[2] = {0};
	u8	rfpath = 0, rfpathnum = 2;

	if(priv->bTXPowerDataReadFromEEPORM == FALSE)
		return;

	{
#ifdef EEPROM_OLD_FORMAT_SUPPORT
		powerlevel = priv->TxPowerLevelCCK[index];
		powerlevelOFDM24G = priv->TxPowerLevelOFDM24G[index];
#else	

		powerlevel = priv->RfTxPwrLevelCck[0][index];
		
		if (priv->rf_type == RF_1T2R || priv->rf_type == RF_1T1R)
		{
		powerlevelOFDM24G = priv->RfTxPwrLevelOfdm1T[0][index];
		
		
		rfpathnum = 1;
		ht20pwr[0] = ht40pwr[0] = priv->RfTxPwrLevelOfdm1T[0][index];
		}
		else if (priv->rf_type == RF_2T2R)
		{
		powerlevelOFDM24G = priv->RfTxPwrLevelOfdm2T[0][index];			
		ant_pwr_diff = 	priv->RfTxPwrLevelOfdm2T[1][index] - 
						priv->RfTxPwrLevelOfdm2T[0][index];
		

		ht20pwr[0] = ht40pwr[0] = priv->RfTxPwrLevelOfdm2T[0][index];
		ht20pwr[1] = ht40pwr[1] = priv->RfTxPwrLevelOfdm2T[1][index];
	}

	if (priv->EEPROMVersion == 2)	
	{		
		if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
		{
			for (rfpath = 0; rfpath < rfpathnum; rfpath++)
			{
				pwrdiff[rfpath] = priv->TxPwrHt20Diff[rfpath][index];

				if (pwrdiff[rfpath] < 8)	
				{
				#if 0
					if (rfpath == 0)
						powerlevelOFDM24G += pwrdiff[rfpath];	
				#endif
					ht20pwr[rfpath] += pwrdiff[rfpath];
				}
				else				
				{
				#if 0
					if (rfpath == 0)
						powerlevelOFDM24G -= (15-pwrdiff[rfpath]);	
				#endif
					ht20pwr[rfpath] -= (15-pwrdiff[rfpath]);
				}
			}

			if (priv->rf_type == RF_2T2R)
				ant_pwr_diff = ht20pwr[1] - ht20pwr[0];

		}

		if (priv->TxPwrbandEdgeFlag == 1/* && pHalData->ChannelPlan == 0*/)		
		{
			for (rfpath = 0; rfpath < rfpathnum; rfpath++)
			{
				pwrdiff[rfpath] = 0;
				if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
				{
					if (channel <= 3)
						pwrdiff[rfpath] = priv->TxPwrbandEdgeHt40[rfpath][0];	
					else if (channel >= 9)					
						pwrdiff[rfpath] = priv->TxPwrbandEdgeHt40[rfpath][1];
					else
						pwrdiff[rfpath] = 0;					

					ht40pwr[rfpath] -= pwrdiff[rfpath];
				}
				else if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
				{
					if (channel == 1)
						pwrdiff[rfpath] = priv->TxPwrbandEdgeHt20[rfpath][0];	
					else if (channel >= 11)
						pwrdiff[rfpath] = priv->TxPwrbandEdgeHt20[rfpath][1];	
					else
						pwrdiff[rfpath] = 0;					
					
					ht20pwr[rfpath] -= pwrdiff[rfpath];
				}
			#if 0
				if (rfpath == 0)
					powerlevelOFDM24G -= pwrdiff[rfpath];
			#endif
			}
			
			if (priv->rf_type == RF_2T2R)
			{
				if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
					ant_pwr_diff = ht40pwr[1] - ht40pwr[0];
				else
					ant_pwr_diff = ht20pwr[1] - ht20pwr[0];
			}
			if (priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
			{
				if (channel <= 1 || channel >= 11)
				{
				}
			}
			else
			{
				if (channel <= 3 || channel >= 9)
				{
				}
			}
		}
#if 0
		legacy_ant_pwr_diff= pHalData->TxPwrLegacyHtDiff[RF90_PATH_A][index];
#endif
		}
				
	if(ant_pwr_diff > 7)
		ant_pwr_diff = 7;
	if(ant_pwr_diff < -8)
		ant_pwr_diff = -8;
			
		
		ant_pwr_diff &= 0xf;	

		priv->AntennaTxPwDiff[2] = 0;
		priv->AntennaTxPwDiff[1] = 0;
		priv->AntennaTxPwDiff[0] = (u8)(ant_pwr_diff);		

		u4RegValue = (	priv->AntennaTxPwDiff[2]<<8 | 
						priv->AntennaTxPwDiff[1]<<4 | 
						priv->AntennaTxPwDiff[0]	);

		rtl8192_setBBreg(dev, rFPGA0_TxGainStage, (bXBTxAGC|bXCTxAGC|bXDTxAGC), u4RegValue);
#endif
	}
		
#ifdef TODO 
	if(	priv->ieee80211->iw_mode != IW_MODE_INFRA && priv->bWithCcxCellPwr &&
		channel == priv->ieee80211->current_network.channel)
	{
		u8	CckCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, priv->CcxCellPwr);
		u8	LegacyOfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_G, priv->CcxCellPwr);
		u8	OfdmCellPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, priv->CcxCellPwr);

		RT_TRACE(COMP_TXAGC,  
		("CCX Cell Limit: %d dbm => CCK Tx power index : %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n", 
		priv->CcxCellPwr, CckCellPwrIdx, LegacyOfdmCellPwrIdx, OfdmCellPwrIdx));
		RT_TRACE(COMP_TXAGC,  
		("EEPROM channel(%d) => CCK Tx power index: %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n",
		channel, powerlevel, powerlevelOFDM24G + priv->LegacyHTTxPowerDiff, powerlevelOFDM24G)); 

		if(powerlevel > CckCellPwrIdx)
			powerlevel = CckCellPwrIdx;
		if(powerlevelOFDM24G + priv->LegacyHTTxPowerDiff > LegacyOfdmCellPwrIdx)
		{
			if((OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff) > 0)
			{
				powerlevelOFDM24G = OfdmCellPwrIdx - priv->LegacyHTTxPowerDiff;
			}
			else
			{
				powerlevelOFDM24G = 0;
			}
		}

		RT_TRACE(COMP_TXAGC, 
		("Altered CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		powerlevel, powerlevelOFDM24G + priv->LegacyHTTxPowerDiff, powerlevelOFDM24G));
	}
#endif

	priv->CurrentCckTxPwrIdx = powerlevel;
	priv->CurrentOfdm24GTxPwrIdx = powerlevelOFDM24G;
	
	switch(priv->rf_chip)
	{
		case RF_8225:
		break;

		case RF_8256:
#if 0
			PHY_SetRF8256CCKTxPower(dev, powerlevel);
			PHY_SetRF8256OFDMTxPower(dev, powerlevelOFDM24G);
#endif
			break;

		case RF_6052:
			PHY_RF6052SetCckTxPower(dev, powerlevel);
			PHY_RF6052SetOFDMTxPower(dev, powerlevelOFDM24G);
			break;

		case RF_8258:
			break;
		default:
			break;
	}	

}

bool PHY_UpdateTxPowerDbm8192S(struct net_device* dev, long powerInDbm)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u8				idx;
	u8				rf_path;

	u8	CckTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_B, powerInDbm);
	u8	OfdmTxPwrIdx = phy_DbmToTxPwrIdx(dev, WIRELESS_MODE_N_24G, powerInDbm);

	if(OfdmTxPwrIdx - priv->LegacyHTTxPowerDiff > 0)
		OfdmTxPwrIdx -= priv->LegacyHTTxPowerDiff;
	else
		OfdmTxPwrIdx = 0;

	for(idx = 0; idx < 14; idx++)
	{
		priv->TxPowerLevelCCK[idx] = CckTxPwrIdx;
		priv->TxPowerLevelCCK_A[idx] = CckTxPwrIdx;
		priv->TxPowerLevelCCK_C[idx] = CckTxPwrIdx;
		priv->TxPowerLevelOFDM24G[idx] = OfdmTxPwrIdx;
		priv->TxPowerLevelOFDM24G_A[idx] = OfdmTxPwrIdx;
		priv->TxPowerLevelOFDM24G_C[idx] = OfdmTxPwrIdx;
		
		for (rf_path = 0; rf_path < 2; rf_path++)
		{
			priv->RfTxPwrLevelCck[rf_path][idx] = CckTxPwrIdx;
			priv->RfTxPwrLevelOfdm1T[rf_path][idx] =  \
			priv->RfTxPwrLevelOfdm2T[rf_path][idx] = OfdmTxPwrIdx;
		}
	}

	PHY_SetTxPowerLevel8192S(dev, priv->chan);

	return TRUE;	
}



extern void PHY_SetBeaconHwReg(	struct net_device* dev, u16 BeaconInterval)
{
	u32 NewBeaconNum;	

	NewBeaconNum = BeaconInterval *32 - 64;
	write_nic_dword(dev, WFM3+4, NewBeaconNum);
	write_nic_dword(dev, WFM3, 0xB026007C);
}

static u8 phy_DbmToTxPwrIdx(
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
static long phy_TxPwrIdxToDbm(
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
		break;
	}

	PwrOutDbm = TxPwrIdx / 2 + Offset; 

	return PwrOutDbm;
}


extern void PHY_ScanOperationBackup8192S(struct net_device* dev, u8 Operation)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

#if(RTL8192S_DISABLE_FW_DM == 0)

	if(priv->up)
	{
		switch(Operation)
		{
			case SCAN_OPT_BACKUP:
				priv->ieee80211->SetFwCmdHandler(dev, FW_CMD_PAUSE_DM_BY_SCAN);
				break;

			case SCAN_OPT_RESTORE:
				priv->ieee80211->SetFwCmdHandler(dev, FW_CMD_RESUME_DM_BY_SCAN);
				break;

			default:
				RT_TRACE(COMP_SCAN,"Unknown Scan Backup Operation. \n");
				break;
		}
	}
#endif
}


void PHY_InitialGain8192S(struct net_device* dev,u8 Operation	)
{


#if 0	
	if(!dev->bDriverStopped)
	{
		switch(Operation)
		{
			case IG_Backup:
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("IG_Backup, backup the initial gain.\n"));
				initial_gain = priv->DefaultInitialGain[0];
				BitMask = bMaskByte0;
				if(DM_DigTable.Dig_Algorithm == DIG_ALGO_BY_FALSE_ALARM)
					PHY_SetMacReg(dev, UFWP, bMaskByte1, 0x8);	
				pMgntInfo->InitGain_Backup.XAAGCCore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XAAGCCore1, BitMask);
				pMgntInfo->InitGain_Backup.XBAGCCore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XBAGCCore1, BitMask);
				pMgntInfo->InitGain_Backup.XCAGCCore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XCAGCCore1, BitMask);
				pMgntInfo->InitGain_Backup.XDAGCCore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XDAGCCore1, BitMask);
				BitMask  = bMaskByte2;
				pMgntInfo->InitGain_Backup.CCA		= (u8)rtl8192_QueryBBReg(dev, rCCK0_CCA, BitMask);

			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xc50 is %x\n",pMgntInfo->InitGain_Backup.XAAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xc58 is %x\n",pMgntInfo->InitGain_Backup.XBAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xc60 is %x\n",pMgntInfo->InitGain_Backup.XCAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xc68 is %x\n",pMgntInfo->InitGain_Backup.XDAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan InitialGainBackup 0xa0a is %x\n",pMgntInfo->InitGain_Backup.CCA));

			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Write scan initial gain = 0x%x \n", initial_gain));
				write_nic_byte(dev, rOFDM0_XAAGCCore1, initial_gain);
				write_nic_byte(dev, rOFDM0_XBAGCCore1, initial_gain);
				write_nic_byte(dev, rOFDM0_XCAGCCore1, initial_gain);
				write_nic_byte(dev, rOFDM0_XDAGCCore1, initial_gain);
				break;
			case IG_Restore:
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("IG_Restore, restore the initial gain.\n"));
				BitMask = 0x7f; 
				if(DM_DigTable.Dig_Algorithm == DIG_ALGO_BY_FALSE_ALARM)
					PHY_SetMacReg(dev, UFWP, bMaskByte1, 0x8);	
			
				rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, BitMask, (u32)pMgntInfo->InitGain_Backup.XAAGCCore1);
				rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, BitMask, (u32)pMgntInfo->InitGain_Backup.XBAGCCore1);
				rtl8192_setBBreg(dev, rOFDM0_XCAGCCore1, BitMask, (u32)pMgntInfo->InitGain_Backup.XCAGCCore1);
				rtl8192_setBBreg(dev, rOFDM0_XDAGCCore1, BitMask, (u32)pMgntInfo->InitGain_Backup.XDAGCCore1);
				BitMask  = (BIT22|BIT23);
				rtl8192_setBBreg(dev, rCCK0_CCA, BitMask, (u32)pMgntInfo->InitGain_Backup.CCA);

			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xc50 is %x\n",pMgntInfo->InitGain_Backup.XAAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xc58 is %x\n",pMgntInfo->InitGain_Backup.XBAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xc60 is %x\n",pMgntInfo->InitGain_Backup.XCAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xc68 is %x\n",pMgntInfo->InitGain_Backup.XDAGCCore1));
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Scan BBInitialGainRestore 0xa0a is %x\n",pMgntInfo->InitGain_Backup.CCA));

				if(DM_DigTable.Dig_Algorithm == DIG_ALGO_BY_FALSE_ALARM)
					PHY_SetMacReg(dev, UFWP, bMaskByte1, 0x1);	
				break;
			default:
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Unknown IG Operation. \n"));
				break;
		}
	}
#endif
}

void PHY_SetBWModeCallback8192S(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8	 			regBwOpMode;


	u8				regRRSR_RSC;

	RT_TRACE(COMP_SWBW, "==>SetBWModeCallback8190Pci()  Switch to %s bandwidth\n", \
					priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz");

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SetBWModeInProgress= FALSE;
		return;
	}
	
	if(!priv->up)
		return;

		
	regBwOpMode = read_nic_byte(dev, BW_OPMODE);
	regRRSR_RSC = read_nic_byte(dev, RRSR+2);
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:

			regBwOpMode |= BW_OPMODE_20MHZ;
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:

			regBwOpMode &= ~BW_OPMODE_20MHZ;
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			regRRSR_RSC = (regRRSR_RSC&0x90) |(priv->nCur40MhzPrimeSC<<5);
			write_nic_byte(dev, RRSR+2, regRRSR_RSC);
			break;

		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8190Pci():\
						unknown Bandwidth: %#X\n",priv->CurrentChannelBW);
			break;
	}
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x0);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x0);

			#if 0 
			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000204);
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00300000, 3);
			#endif
			
			if (priv->card_8192_version >= VERSION_8192S_BCUT)
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x58);
			

			break;

		case HT_CHANNEL_WIDTH_20_40:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x1);
			
			#if 0 
			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x35360000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x121c252e);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000409);
			#endif
			
			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));
			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);			
			
			if (priv->card_8192_version >= VERSION_8192S_BCUT)
				write_nic_byte(dev, rFPGA0_AnalogParameter2, 0x18);
			
			break;
			
		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8190Pci(): unknown Bandwidth: %#X\n"\
						,priv->CurrentChannelBW);
			break;
			
	}


	switch( priv->rf_chip )
	{
		case RF_8225:		
			break;	
			
		case RF_8256:
			break;
			
		case RF_8258:
			break;

		case RF_PSEUDO_11N:
			break;

		case RF_6052:
			PHY_RF6052SetBandwidth(dev, priv->CurrentChannelBW);
			break;
		default:
			printk("Unknown rf_chip: %d\n", priv->rf_chip);
			break;
	}

	priv->SetBWModeInProgress= FALSE;

	RT_TRACE(COMP_SWBW, "<==SetBWModeCallback8190Pci() \n" );
}


void rtl8192_SetBWMode(struct net_device *dev, HT_CHANNEL_WIDTH	Bandwidth, HT_EXTCHNL_OFFSET Offset)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	HT_CHANNEL_WIDTH tmpBW = priv->CurrentChannelBW; 

	

	


	if(priv->SetBWModeInProgress)
		return;

	priv->SetBWModeInProgress= TRUE;
	
	priv->CurrentChannelBW = Bandwidth;
	
	if(Offset==HT_EXTCHNL_OFFSET_LOWER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
	else if(Offset==HT_EXTCHNL_OFFSET_UPPER)
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
	else
		priv->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;

#if 0
	if(!priv->bDriverStopped)
	{
#ifdef USE_WORKITEM	
		PlatformScheduleWorkItem(&(priv->SetBWModeWorkItem));
#else
		PlatformSetTimer(dev, &(priv->SetBWModeTimer), 0);
#endif		
	}
#endif
	if((priv->up) )
	{
#ifdef RTL8192SE
	PHY_SetBWModeCallback8192S(dev);
#elif defined(RTL8192SU)		
	SetBWModeCallback8192SUsbWorkItem(dev);
#endif
	}
	else
	{
		RT_TRACE(COMP_SCAN, "PHY_SetBWMode8192S() SetBWModeInProgress FALSE driver sleep or unload\n");	
		priv->SetBWModeInProgress= FALSE;	
		priv->CurrentChannelBW = tmpBW;
	}
}

void PHY_SwChnlCallback8192S(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	u32		delay;
	
	RT_TRACE(COMP_CH, "==>SwChnlCallback8190Pci(), switch to channel %d\n", priv->chan);
	
	if(!priv->up)
		return;
	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=FALSE;
		return; 								
	}
	
	do{
		if(!priv->SwChnlInProgress)
			break;

		if(!phy_SwChnlStepByStep(dev, priv->chan, &priv->SwChnlStage, &priv->SwChnlStep, &delay))
		{
			if(delay>0)
			{
				mdelay(delay);
			}
			else
			continue;
		}
		else
		{
			priv->SwChnlInProgress=FALSE;
			break;
		}
	}while(true);
}

u8 rtl8192_phy_SwChnl(struct net_device* dev, u8 channel)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

        if(!priv->up)
		return false;
		
	if(priv->SwChnlInProgress)
		return false;

	if(priv->SetBWModeInProgress)
		return false;

	switch(priv->ieee80211->mode)
	{
	case WIRELESS_MODE_A:
	case WIRELESS_MODE_N_5G:
		if (channel<=14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_A but channel<=14");
			return false;
		}
		break;
		
	case WIRELESS_MODE_B:
		if (channel>14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_B but channel>14");
			return false;
		}
		break;
		
	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		if (channel>14){
			RT_TRACE(COMP_ERR, "WIRELESS_MODE_G but channel>14");
			return false;
		}
		break;

	default:
			;
		break;
	}
	
	priv->SwChnlInProgress = TRUE;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;

	priv->SwChnlStage=0;
	priv->SwChnlStep=0;

	if((priv->up))
	{
#ifdef RTL8192SE
	PHY_SwChnlCallback8192S(dev);
#elif defined(RTL8192SU)	
	SwChnlCallback8192SUsbWorkItem(dev);
#endif
#ifdef TO_DO_LIST
	if(bResult)
		{
			RT_TRACE(COMP_SCAN, "PHY_SwChnl8192S SwChnlInProgress TRUE schdule workitem done\n");
		}
		else
		{
			RT_TRACE(COMP_SCAN, "PHY_SwChnl8192S SwChnlInProgress FALSE schdule workitem error\n");		
			priv->SwChnlInProgress = false; 	
			priv->CurrentChannel = tmpchannel;			
		}
#endif
	}
	else
	{
		RT_TRACE(COMP_SCAN, "PHY_SwChnl8192S SwChnlInProgress FALSE driver sleep or unload\n");	
		priv->SwChnlInProgress = false;		
	}
        return true;	
}


void PHY_SwChnlPhy8192S(	
	struct net_device* dev,
	u8		channel
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	RT_TRACE(COMP_SCAN, "==>PHY_SwChnlPhy8192S(), switch to channel %d.\n", priv->chan);

#ifdef TO_DO_LIST
	if(RT_CANNOT_IO(dev))
		return;
#endif

	if(priv->SwChnlInProgress)
		return;
	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=FALSE;
		return;
	}
	
	priv->SwChnlInProgress = TRUE;
	if( channel == 0)
		channel = 1;
	
	priv->chan=channel;
	
	priv->SwChnlStage = 0;
	priv->SwChnlStep = 0;
	
	phy_FinishSwChnlNow(dev,channel);
	
	priv->SwChnlInProgress = FALSE;
}

static bool
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
		return FALSE;
	}
	if(CmdTableIdx >= CmdTableSz)
	{
		return FALSE;
	}

	pCmd = CmdTable + CmdTableIdx;
	pCmd->CmdID = CmdID;
	pCmd->Para1 = Para1;
	pCmd->Para2 = Para2;
	pCmd->msDelay = msDelay;

	return TRUE;
}

static bool
phy_SwChnlStepByStep(
	struct net_device* dev,
	u8		channel,
	u8		*stage,
	u8		*step,
	u32		*delay
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	SwChnlCmd				PreCommonCmd[MAX_PRECMD_CNT];
	u32					PreCommonCmdCnt;
	SwChnlCmd				PostCommonCmd[MAX_POSTCMD_CNT];
	u32					PostCommonCmdCnt;
	SwChnlCmd				RfDependCmd[MAX_RFDEPENDCMD_CNT];
	u32					RfDependCmdCnt;
	SwChnlCmd				*CurrentCmd = NULL;	
	u8					eRFPath;	
	
	RT_TRACE(COMP_CH, "===========>%s(), channel:%d, stage:%d, step:%d\n", __FUNCTION__, channel, *stage, *step);		
#ifdef ENABLE_DOT11D
	if (!IsLegalChannel(priv->ieee80211, channel))
	{
		RT_TRACE(COMP_ERR, "=============>set to illegal channel:%d\n", channel);
		return true; 
	}		
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
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;	
		
	case RF_8256:
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;
		
	case RF_6052:
		if (channel < 1 || channel > 14)
			RT_TRACE(COMP_ERR, "illegal channel for zebra:%d\n", channel);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, RF_CHNLBW, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_End, 0, 0, 0);		
		break;

	case RF_8258:
		break;
		
	default:
		return FALSE;
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
				return TRUE;
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
				PHY_SetTxPowerLevel8192S(dev,channel);
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
#ifdef  RTL8192SU
				priv->RfRegChnlVal[eRFPath] = ((priv->RfRegChnlVal[eRFPath] & 0xfffffc00) | CurrentCmd->Para2);
				rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, priv->RfRegChnlVal[eRFPath]);
#endif
			}
			break;
                default:
                        break;
		}
		
		break;
	}while(TRUE);

	(*delay)=CurrentCmd->msDelay;
	(*step)++;
	RT_TRACE(COMP_CH, "<===========%s(), channel:%d, stage:%d, step:%d\n", __FUNCTION__, channel, *stage, *step);		
	return FALSE;
}

static void
phy_FinishSwChnlNow(	
	struct net_device* dev,
	u8		channel
		)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	u32			delay;
  
	while(!phy_SwChnlStepByStep(dev,channel,&priv->SwChnlStage,&priv->SwChnlStep,&delay))
	{
		if(!priv->up)
			break;
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
u8 rtl8192_phy_CheckIsLegalRFPath(struct net_device* dev, u32 eRFPath)
{
	bool				rtValue = TRUE;

#if 0	
	if (priv->rf_type == RF_1T2R && eRFPath != RF90_PATH_A)
	{		
		rtValue = FALSE;
	}
	if (priv->rf_type == RF_1T2R && eRFPath != RF90_PATH_A)
	{

	}
#endif
	return	rtValue;

}	



void	
PHY_IQCalibrate(	struct net_device* dev)
{
	u32				i, reg;
	u32				old_value;
	long				X, Y, TX0[4];
	u32				TXA[4];
	

	for (i = 0; i < 10; i++)
	{
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05430);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000800e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x80800000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe40, bMaskDWord, 0x02140148);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe44, bMaskDWord, 0x681604a2);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe4c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe60, bMaskDWord, 0x0214014d);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe64, bMaskDWord, 0x281608ba);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe6c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000001);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000001);
		udelay(2000);
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05433);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000000e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x0);		

	
		reg = rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord);

		if (!(reg&(BIT27|BIT28|BIT30|BIT31)))
		{
			old_value = (rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord) & 0x3FF);

			X = (rtl8192_QueryBBReg(dev, 0xe94, bMaskDWord) & 0x03FF0000)>>16;
			TXA[RF90_PATH_A] = (X * old_value)/0x100;
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xFFFFFC00) | (u32)TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			udelay(5);

			Y = ( rtl8192_QueryBBReg(dev, 0xe9C, bMaskDWord) & 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xffc0ffff) |((u32) (TX0[RF90_PATH_C]&0x3F)<<16);			
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc94, bMaskDWord);		
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc94, bMaskDWord, reg);
			udelay(5);

			reg = rtl8192_QueryBBReg(dev, 0xc14, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xea4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			Y = (rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			udelay(5);
			old_value = (rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord) & 0x3FF);
			
			X = (rtl8192_QueryBBReg(dev, 0xeb4, bMaskDWord) & 0x03FF0000)>>16;
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			TXA[RF90_PATH_A] = (X * old_value) / 0x100;
			reg = (reg & 0xFFFFFC00) | TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			udelay(5);

			Y = (rtl8192_QueryBBReg(dev, 0xebc, bMaskDWord)& 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			reg = (reg & 0xffc0ffff) |( (TX0[RF90_PATH_C]&0x3F)<<16);
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc9c, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc9c, bMaskDWord, reg);
			udelay(5);

			reg = rtl8192_QueryBBReg(dev, 0xc1c, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xec4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			
			Y = (rtl8192_QueryBBReg(dev, 0xecc, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			udelay(5);
			
			RT_TRACE(COMP_INIT, "PHY_IQCalibrate OK\n");
			break;
		}

	}
	
	


}

extern void PHY_IQCalibrateBcut(struct net_device* dev)
{
	u32				i, reg;
	u32				old_value;
	long				X, Y, TX0[4];
	u32				TXA[4];	
	u32				calibrate_set[13] = {0};
	u32				load_value[13];
	u8				RfPiEnable=0;
	

	/*
	0xee0[31:0]=0x3fed92fb;   
	0xedc[31:0] =0x3fed92fb;   
	0xe70[31:0] =0x3fed92fb;   
	0xe74[31:0] =0x3fed92fb;   
	0xe78[31:0] =0x3fed92fb;   
	0xe7c[31:0]= 0x3fed92fb;   
	0xe80[31:0]= 0x3fed92fb;   
	0xe84[31:0]= 0x3fed92fb; 
	0xe88[31:0]= 0x3fed92fb;  
	0xe8c[31:0]= 0x3fed92fb;   
	0xed0[31:0]= 0x3fed92fb;   
	0xed4[31:0]= 0x3fed92fb;  
	0xed8[31:0]= 0x3fed92fb;
	*/
	calibrate_set [0] = 0xee0;
	calibrate_set [1] = 0xedc;
	calibrate_set [2] = 0xe70;
	calibrate_set [3] = 0xe74;
	calibrate_set [4] = 0xe78;
	calibrate_set [5] = 0xe7c;
	calibrate_set [6] = 0xe80;
	calibrate_set [7] = 0xe84;
	calibrate_set [8] = 0xe88;
	calibrate_set [9] = 0xe8c;
	calibrate_set [10] = 0xed0;
	calibrate_set [11] = 0xed4;
	calibrate_set [12] = 0xed8;
	for (i = 0; i < 13; i++)
	{
		load_value[i] = rtl8192_QueryBBReg(dev, calibrate_set[i], bMaskDWord);
		rtl8192_setBBreg(dev, calibrate_set[i], bMaskDWord, 0x3fed92fb);		
		
	}
	
	RfPiEnable = (u8)rtl8192_QueryBBReg(dev, rFPGA0_XA_HSSIParameter1, BIT8);

	for (i = 0; i < 10; i++)
	{
		RT_TRACE(COMP_INIT, "IQK -%d\n", i);	
		if (!RfPiEnable)	
		{
			rtl8192_setBBreg(dev, 0x820, bMaskDWord, 0x01000100);
			rtl8192_setBBreg(dev, 0x828, bMaskDWord, 0x01000100);
		}
	
		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05430);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000800e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x80800000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe40, bMaskDWord, 0x02140102);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe44, bMaskDWord, 0x681604c2);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe4c, bMaskDWord, 0x000028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe60, bMaskDWord, 0x02140102);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe64, bMaskDWord, 0x28160d05);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000000);
		udelay(5);

		udelay(2000);

		rtl8192_setBBreg(dev, 0xe6c, bMaskDWord, 0x020028d1);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xfb000000);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe48, bMaskDWord, 0xf8000000);

		udelay(2000);

		rtl8192_setBBreg(dev, 0xc04, bMaskDWord, 0x00a05433);
		udelay(5);
		rtl8192_setBBreg(dev, 0xc08, bMaskDWord, 0x000000e4);
		udelay(5);
		rtl8192_setBBreg(dev, 0xe28, bMaskDWord, 0x0);		

		if (!RfPiEnable)	
		{
			rtl8192_setBBreg(dev, 0x820, bMaskDWord, 0x01000000);
			rtl8192_setBBreg(dev, 0x828, bMaskDWord, 0x01000000);
		}

	
		reg = rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord);

		if (!(reg&(BIT27|BIT28|BIT30|BIT31)))
		{			
			old_value = (rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord) & 0x3FF);

			X = (rtl8192_QueryBBReg(dev, 0xe94, bMaskDWord) & 0x03FF0000)>>16;
			TXA[RF90_PATH_A] = (X * old_value)/0x100;
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xFFFFFC00) | (u32)TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			udelay(5);

			Y = ( rtl8192_QueryBBReg(dev, 0xe9C, bMaskDWord) & 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc80, bMaskDWord);
			reg = (reg & 0xffc0ffff) |((u32) (TX0[RF90_PATH_C]&0x3F)<<16);			
			rtl8192_setBBreg(dev, 0xc80, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc94, bMaskDWord);		
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc94, bMaskDWord, reg);
			udelay(5);

			reg = rtl8192_QueryBBReg(dev, 0xc14, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xea4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			Y = (rtl8192_QueryBBReg(dev, 0xeac, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc14, bMaskDWord, reg);
			udelay(5);
			old_value = (rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord) & 0x3FF);
			
			X = (rtl8192_QueryBBReg(dev, 0xeb4, bMaskDWord) & 0x03FF0000)>>16;
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			TXA[RF90_PATH_A] = (X * old_value) / 0x100;
			reg = (reg & 0xFFFFFC00) | TXA[RF90_PATH_A];
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			udelay(5);

			Y = (rtl8192_QueryBBReg(dev, 0xebc, bMaskDWord)& 0x03FF0000)>>16;
			TX0[RF90_PATH_C] = ((Y * old_value)/0x100);
			reg = rtl8192_QueryBBReg(dev, 0xc88, bMaskDWord);
			reg = (reg & 0xffc0ffff) |( (TX0[RF90_PATH_C]&0x3F)<<16);
			rtl8192_setBBreg(dev, 0xc88, bMaskDWord, reg);
			reg = rtl8192_QueryBBReg(dev, 0xc9c, bMaskDWord);
			reg = (reg & 0x0fffffff) |(((Y&0x3c0)>>6)<<28);
			rtl8192_setBBreg(dev, 0xc9c, bMaskDWord, reg);
			udelay(5);

			reg = rtl8192_QueryBBReg(dev, 0xc1c, bMaskDWord);
			X = (rtl8192_QueryBBReg(dev, 0xec4, bMaskDWord) & 0x03FF0000)>>16;			
			reg = (reg & 0xFFFFFC00) |X;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			
			Y = (rtl8192_QueryBBReg(dev, 0xecc, bMaskDWord) & 0x003F0000)>>16;
			reg = (reg & 0xFFFF03FF) |Y<<10;
			rtl8192_setBBreg(dev, 0xc1c, bMaskDWord, reg);
			udelay(5);
			
			RT_TRACE(COMP_INIT, "PHY_IQCalibrate OK\n");
			break;
		}

	}
	
	for (i = 0; i < 13; i++)
		rtl8192_setBBreg(dev, calibrate_set[i], bMaskDWord, load_value[i]);
	
	
	


}	


#ifdef RTL8192SU

void SwChnlCallback8192SUsb(struct net_device *dev)
{

	struct r8192_priv *priv = ieee80211_priv(dev);
	u32			delay;

	RT_TRACE(COMP_SCAN, "==>SwChnlCallback8190Pci(), switch to channel\
				%d\n", priv->chan);


	if(!priv->up)
		return;
	
	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SwChnlInProgress=FALSE;
		return; 								
	}
	
	do{
		if(!priv->SwChnlInProgress)
			break;

		if(!phy_SwChnlStepByStep(dev, priv->chan, &priv->SwChnlStage, &priv->SwChnlStep, &delay))
		{
			if(delay>0)
			{
				
			}
			else
			continue;
		}
		else
		{
			priv->SwChnlInProgress=FALSE;
		}
		break;
	}while(TRUE);
}


void SwChnlCallback8192SUsbWorkItem(struct net_device *dev )
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	
	RT_TRACE(COMP_TRACE, "==> SwChnlCallback8192SUsbWorkItem()\n");
#ifdef TO_DO_LIST	
	if(pAdapter->bInSetPower && RT_USB_CANNOT_IO(pAdapter))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("<== SwChnlCallback8192SUsbWorkItem() SwChnlInProgress FALSE driver sleep or unload\n"));
	
		pHalData->SwChnlInProgress = FALSE;		
		return;
	}
#endif
	phy_FinishSwChnlNow(dev, priv->chan);  
	priv->SwChnlInProgress = FALSE;
	
	RT_TRACE(COMP_TRACE, "<== SwChnlCallback8192SUsbWorkItem()\n");
}


void SetBWModeCallback8192SUsb(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8	 			regBwOpMode;

	u8				regRRSR_RSC;
	
	RT_TRACE(COMP_SCAN, "==>SetBWModeCallback8190Pci()  Switch to %s bandwidth\n", \
					priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz");

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SetBWModeInProgress= FALSE;
		return;
	}
		
	if(!priv->up)
		return;
		
		
	regBwOpMode = read_nic_byte(dev, BW_OPMODE);
	regRRSR_RSC = read_nic_byte(dev, RRSR+2);
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			
			regRRSR_RSC = (regRRSR_RSC&0x90) |(priv->nCur40MhzPrimeSC<<5);
			write_nic_byte(dev, RRSR+2, regRRSR_RSC);
			break;

		default:
			RT_TRACE(COMP_DBG, "SetChannelBandwidth8190Pci():\
						unknown Bandwidth: %#X\n",priv->CurrentChannelBW);
			break;
	}
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x0);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x0);
			#if 0 
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00300000, 3);

			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000204);
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00300000, 3);
			#endif
			
			if (priv->card_8192_version >= VERSION_8192S_BCUT)
				rtl8192_setBBreg(dev, rFPGA0_AnalogParameter2, 0xff, 0x58);

			break;
		case HT_CHANNEL_WIDTH_20_40:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));
			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);


			if (priv->card_8192_version >= VERSION_8192S_BCUT)
				rtl8192_setBBreg(dev, rFPGA0_AnalogParameter2, 0xff, 0x18);

			break;
		default:
			RT_TRACE(COMP_DBG, "SetChannelBandwidth8190Pci(): unknown Bandwidth: %#X\n"\
						,priv->CurrentChannelBW);
			break;
			
	}


#if 1
	switch( priv->rf_chip )
	{
		case RF_8225:		
			PHY_SetRF8225Bandwidth(dev, priv->CurrentChannelBW);
			break;	
			
		case RF_8256:
			break;
			
		case RF_6052:
			PHY_RF6052SetBandwidth(dev, priv->CurrentChannelBW);
			break;		
			
		case RF_8258:
			break;

		case RF_PSEUDO_11N:
			break;
			
		default:
			break;
	}
#endif
	priv->SetBWModeInProgress= FALSE;

	RT_TRACE(COMP_SCAN, "<==SetBWMode8190Pci()" );
}

void SetBWModeCallback8192SUsbWorkItem(struct net_device *dev)
{
	struct r8192_priv 		*priv = ieee80211_priv(dev);
	u8	 			regBwOpMode;

	u8			regRRSR_RSC;
	
	RT_TRACE(COMP_SCAN, "==>SetBWModeCallback8192SUsbWorkItem()  Switch to %s bandwidth\n", \
					priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz");

	if(priv->rf_chip == RF_PSEUDO_11N)
	{
		priv->SetBWModeInProgress= FALSE;
		return;
	}
		
	if(!priv->up)
		return;
		
		
	regBwOpMode = read_nic_byte(dev, BW_OPMODE);
	regRRSR_RSC = read_nic_byte(dev, RRSR+2);
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_20_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
			write_nic_byte(dev, BW_OPMODE, regBwOpMode);
			regRRSR_RSC = (regRRSR_RSC&0x90) |(priv->nCur40MhzPrimeSC<<5);
			write_nic_byte(dev, RRSR+2, regRRSR_RSC);

			break;

		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8192SUsbWorkItem():\
						unknown Bandwidth: %#X\n",priv->CurrentChannelBW);
			break;
	}
	
	switch(priv->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x0);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x0);
			
			#if 0 
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, bADClkPhase, 1);

			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x1a1b0000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x090e1317);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000204);
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x00100000, 1);
			#endif
			
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter2, 0xff, 0x58);

			break;
		case HT_CHANNEL_WIDTH_20_40:
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bRFMOD, 0x1);
			rtl8192_setBBreg(dev, rFPGA1_RFMOD, bRFMOD, 0x1);
			#if 0 
			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));	

			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, bADClkPhase, 0);

   			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);
			rtl8192_setBBreg(dev, rCCK0_TxFilter1, bMaskDWord, 0x35360000);
			rtl8192_setBBreg(dev, rCCK0_TxFilter2, bMaskDWord, 0x121c252e);
			rtl8192_setBBreg(dev, rCCK0_DebugPort, bMaskDWord, 0x00000409);
			#endif
			
			rtl8192_setBBreg(dev, rCCK0_System, bCCKSideBand, (priv->nCur40MhzPrimeSC>>1));
			rtl8192_setBBreg(dev, rOFDM1_LSTF, 0xC00, priv->nCur40MhzPrimeSC);

			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter2, 0xff, 0x18);

			break;


		default:
			RT_TRACE(COMP_DBG, "SetBWModeCallback8192SUsbWorkItem(): unknown Bandwidth: %#X\n"\
						,priv->CurrentChannelBW);
			break;
			
	}

	switch( priv->rf_chip )
	{
		case RF_8225:		
			PHY_SetRF8225Bandwidth(dev, priv->CurrentChannelBW);
			break;	
			
		case RF_8256:
			break;
			
		case RF_6052:
			PHY_RF6052SetBandwidth(dev, priv->CurrentChannelBW);
			break;	

		case RF_8258:
			break;

		case RF_PSEUDO_11N:
			break;
			
		default:
			break;
	}

	priv->SetBWModeInProgress= FALSE;

	RT_TRACE(COMP_SCAN, "<==SetBWModeCallback8192SUsbWorkItem()" );
}

#ifdef RTL8192SU
void InitialGain8192S(struct net_device *dev,	u8 Operation)
{
#ifdef TO_DO_LIST
	struct r8192_priv *priv = ieee80211_priv(dev);
#endif

}
#endif

void InitialGain819xUsb(struct net_device *dev,	u8 Operation)
{
	struct r8192_priv *priv = ieee80211_priv(dev);

	priv->InitialGainOperateType = Operation;

	if(priv->up)
	{
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20) 
		queue_delayed_work(priv->priv_wq,&priv->initialgain_operate_wq,0);
	#else
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		schedule_task(&priv->initialgain_operate_wq);	
		#else
		queue_work(priv->priv_wq,&priv->initialgain_operate_wq);	
		#endif
	#endif	
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
extern void InitialGainOperateWorkItemCallBack(struct work_struct *work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
       struct r8192_priv *priv = container_of(dwork,struct r8192_priv,initialgain_operate_wq);
       struct net_device *dev = priv->ieee80211->dev;
#else
extern void InitialGainOperateWorkItemCallBack(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
#endif
#define SCAN_RX_INITIAL_GAIN	0x17
#define POWER_DETECTION_TH	0x08
	u32	BitMask;
	u8	initial_gain;
	u8	Operation;
	
	Operation = priv->InitialGainOperateType;

	switch(Operation)
	{
		case IG_Backup:
			RT_TRACE(COMP_SCAN, "IG_Backup, backup the initial gain.\n");
			initial_gain = SCAN_RX_INITIAL_GAIN;
			BitMask = bMaskByte0;
			if(dm_digtable.dig_algorithm == DIG_ALGO_BY_FALSE_ALARM)
				rtl8192_setBBreg(dev, UFWP, bMaskByte1, 0x8);	
			priv->initgain_backup.xaagccore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XAAGCCore1, BitMask);
			priv->initgain_backup.xbagccore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XBAGCCore1, BitMask);
			priv->initgain_backup.xcagccore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XCAGCCore1, BitMask);
			priv->initgain_backup.xdagccore1 = (u8)rtl8192_QueryBBReg(dev, rOFDM0_XDAGCCore1, BitMask);
			BitMask  = bMaskByte2;
			priv->initgain_backup.cca		= (u8)rtl8192_QueryBBReg(dev, rCCK0_CCA, BitMask);

			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xc50 is %x\n",priv->initgain_backup.xaagccore1);
			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xc58 is %x\n",priv->initgain_backup.xbagccore1);
			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xc60 is %x\n",priv->initgain_backup.xcagccore1);
			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xc68 is %x\n",priv->initgain_backup.xdagccore1);
			RT_TRACE(COMP_SCAN, "Scan InitialGainBackup 0xa0a is %x\n",priv->initgain_backup.cca);

			RT_TRACE(COMP_SCAN, "Write scan initial gain = 0x%x \n", initial_gain);
			write_nic_byte(dev, rOFDM0_XAAGCCore1, initial_gain);
			write_nic_byte(dev, rOFDM0_XBAGCCore1, initial_gain);
			write_nic_byte(dev, rOFDM0_XCAGCCore1, initial_gain);
			write_nic_byte(dev, rOFDM0_XDAGCCore1, initial_gain);
			RT_TRACE(COMP_SCAN, "Write scan 0xa0a = 0x%x \n", POWER_DETECTION_TH);
			write_nic_byte(dev, 0xa0a, POWER_DETECTION_TH);
			break;
		case IG_Restore:
			RT_TRACE(COMP_SCAN, "IG_Restore, restore the initial gain.\n");
			BitMask = 0x7f; 
			if(dm_digtable.dig_algorithm == DIG_ALGO_BY_FALSE_ALARM)
				rtl8192_setBBreg(dev, UFWP, bMaskByte1, 0x8);	
			
			rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, BitMask, (u32)priv->initgain_backup.xaagccore1);
			rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, BitMask, (u32)priv->initgain_backup.xbagccore1);
			rtl8192_setBBreg(dev, rOFDM0_XCAGCCore1, BitMask, (u32)priv->initgain_backup.xcagccore1);
			rtl8192_setBBreg(dev, rOFDM0_XDAGCCore1, BitMask, (u32)priv->initgain_backup.xdagccore1);
			BitMask  = bMaskByte2;
			rtl8192_setBBreg(dev, rCCK0_CCA, BitMask, (u32)priv->initgain_backup.cca);

			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xc50 is %x\n",priv->initgain_backup.xaagccore1);
			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xc58 is %x\n",priv->initgain_backup.xbagccore1);
			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xc60 is %x\n",priv->initgain_backup.xcagccore1);
			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xc68 is %x\n",priv->initgain_backup.xdagccore1);
			RT_TRACE(COMP_SCAN, "Scan BBInitialGainRestore 0xa0a is %x\n",priv->initgain_backup.cca);

			PHY_SetTxPowerLevel8192S(dev,priv->ieee80211->current_network.channel); 

			if(dm_digtable.dig_algorithm == DIG_ALGO_BY_FALSE_ALARM)
				rtl8192_setBBreg(dev, UFWP, bMaskByte1, 0x1);	
			break;
		default:
			RT_TRACE(COMP_SCAN, "Unknown IG Operation. \n");
			break;
	}
}

#endif	

bool HalSetFwCmd8192S(struct net_device* dev, FW_CMD_IO_TYPE	FwCmdIO)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_HIGH_THROUGHPUT	pHTInfo = priv->ieee80211->pHTInfo;
	rt_firmware		*pFirmware = priv->pFirmware;
	u32	FwParam = FW_CMD_IO_PARA_QUERY(dev);
	u16	FwCmdMap = FW_CMD_IO_QUERY(dev);
	bool	bPostProcessing = false;

	
	if(priv->bInHctTest)
		return true;

	RT_TRACE(COMP_CMD, "-->HalSetFwCmd8192S(): Set FW Cmd(%x), SetFwCmdInProgress(%d)\n", (u32)FwCmdIO, priv->SetFwCmdInProgress);

	do{
		if(pFirmware->FirmwareVersion >= 0x35)
		{
			switch(FwCmdIO)
			{
				case FW_CMD_RA_REFRESH_N:
					FwCmdIO = FW_CMD_RA_REFRESH_N_COMB;
					break;
				case FW_CMD_RA_REFRESH_BG:
					FwCmdIO = FW_CMD_RA_REFRESH_BG_COMB;
					break;
				default:
					break;
			}
		}
		else
		{			
			if((FwCmdIO == FW_CMD_IQK_ENABLE) ||
				(FwCmdIO == FW_CMD_RA_REFRESH_N) ||
				(FwCmdIO == FW_CMD_RA_REFRESH_BG))
			{
				bPostProcessing = true;
				break;
			}
		}

		switch(FwCmdIO)
		{
		
			case FW_CMD_RA_INIT:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] RA init!!\n");
				FwCmdMap |= FW_RA_INIT_CTL;
				FW_CMD_IO_SET(dev, FwCmdMap);	
				FW_CMD_IO_CLR(dev, FW_RA_INIT_CTL);	 
				break;

			case FW_CMD_DIG_DISABLE:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG disable!!\n");
				FwCmdMap &= ~FW_DIG_ENABLE_CTL;
				FW_CMD_IO_SET(dev, FwCmdMap);			
				break;
				
			case FW_CMD_DIG_ENABLE:
			case FW_CMD_DIG_RESUME:	
				if(!(priv->DMFlag & HAL_DM_DIG_DISABLE))
				{
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG enable or resume!!\n");
					FwCmdMap |= (FW_DIG_ENABLE_CTL|FW_SS_CTL);
					FW_CMD_IO_SET(dev, FwCmdMap);	
				}
				break;

			case FW_CMD_DIG_HALT:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set DIG halt!!\n");
				FwCmdMap &= ~(FW_DIG_ENABLE_CTL|FW_SS_CTL);
				FW_CMD_IO_SET(dev, FwCmdMap);				
				break;

			case FW_CMD_TXPWR_TRACK_THERMAL:
				{
					u8	ThermalVal = 0;
					FwCmdMap |= FW_PWR_TRK_CTL;
					FwParam &= FW_PWR_TRK_PARAM_CLR; 
					ThermalVal = priv->ThermalValue;
					FwParam |= ((ThermalVal<<24) |(priv->ThermalMeter[0]<<16));
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set TxPwr tracking!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
					FW_CMD_PARA_SET(dev, FwParam);
					FW_CMD_IO_SET(dev, FwCmdMap);
					FW_CMD_IO_CLR(dev, FW_PWR_TRK_CTL);	 
				}
				break;
				
			case FW_CMD_RA_REFRESH_N_COMB:
				FwCmdMap |= FW_RA_N_CTL;
				FwCmdMap &= ~(FW_RA_BG_CTL |FW_RA_INIT_CTL);
				FwParam &= FW_RA_PARAM_CLR; 
				if(!(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL))
					FwParam |= ((pHTInfo->IOTRaFunc)&0xf);
				FwParam |= ((pHTInfo->IOTPeer & 0xf) <<4);
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set RA/IOT Comb in n mode!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
				FW_CMD_PARA_SET(dev, FwParam);
				FW_CMD_IO_SET(dev, FwCmdMap);	
				FW_CMD_IO_CLR(dev, FW_RA_N_CTL); 
				break;	

			case FW_CMD_RA_REFRESH_BG_COMB:				
				FwCmdMap |= FW_RA_BG_CTL;
				FwCmdMap &= ~(FW_RA_N_CTL|FW_RA_INIT_CTL); 
				FwParam &= FW_RA_PARAM_CLR; 
				if(!(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL))
					FwParam |= ((pHTInfo->IOTRaFunc)&0xf);
				FwParam |= ((pHTInfo->IOTPeer & 0xf) <<4);
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set RA/IOT Comb in BG mode!! FwCmdMap(%#x), FwParam(%#x)\n", FwCmdMap, FwParam);
				FW_CMD_PARA_SET(dev, FwParam);
				FW_CMD_IO_SET(dev, FwCmdMap);	
				FW_CMD_IO_CLR(dev, FW_RA_BG_CTL); 
				break;

			case FW_CMD_IQK_ENABLE:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] IQK enable.\n");
				FwCmdMap |= FW_IQK_CTL;
				FW_CMD_IO_SET(dev, FwCmdMap);	
				FW_CMD_IO_CLR(dev, FW_IQK_CTL); 
				break;

			case FW_CMD_RESUME_DM_BY_SCAN:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Resume DM after scan.\n");
				if(priv->DMFlag & (HAL_DM_DIG_DISABLE | HAL_DM_HIPWR_DISABLE))
				{
					break;
				}

				if( !(pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_HIGH_POWER))	
				{
					FwCmdMap |= (FW_DIG_ENABLE_CTL|FW_HIGH_PWR_ENABLE_CTL|FW_SS_CTL);
				}
				else
				{
					RT_TRACE(COMP_CMD, "[FW CMD]  [New Version] Resume DIG only!!\n");
					FwCmdMap |= (FW_DIG_ENABLE_CTL|FW_SS_CTL);
					FwCmdMap &= ~FW_HIGH_PWR_ENABLE_CTL;				
				}
				FW_CMD_IO_SET(dev, FwCmdMap);		
				bPostProcessing = true;
				break;

			case FW_CMD_PAUSE_DM_BY_SCAN:
				if(priv->DMFlag & (HAL_DM_DIG_DISABLE | HAL_DM_HIPWR_DISABLE))
				{
					break;
				}
				else
				{
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Pause DM before scan.\n");
					FwCmdMap &= ~(FW_DIG_ENABLE_CTL|FW_HIGH_PWR_ENABLE_CTL|FW_SS_CTL);
					FW_CMD_IO_SET(dev, FwCmdMap);		
					bPostProcessing = true;
				}
				break;

			case FW_CMD_HIGH_PWR_DISABLE:
				RT_TRACE(COMP_CMD, "[FW CMD] [New Version]  Set HighPwr disable!!\n");
				FwCmdMap &= ~FW_HIGH_PWR_ENABLE_CTL;
				FW_CMD_IO_SET(dev, FwCmdMap);		
				bPostProcessing = true;
				break;
				
			case FW_CMD_HIGH_PWR_ENABLE:	
				if(((pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_HIGH_POWER)==0) &&
					!(priv->DMFlag & HAL_DM_HIPWR_DISABLE))
				{
					RT_TRACE(COMP_CMD, "[FW CMD] [New Version] Set HighPwr enable!!\n");
					FwCmdMap |= (FW_HIGH_PWR_ENABLE_CTL|FW_SS_CTL);
					FW_CMD_IO_SET(dev, FwCmdMap);		
					bPostProcessing = true;
				}
				break;
				
			default:
				bPostProcessing = true; 
				break;
		}
	}while(FALSE);


	RT_TRACE(COMP_CMD, "[FW CMD] Current FwCmdMap(%#x)\n", priv->FwCmdIOMap);
	RT_TRACE(COMP_CMD, "[FW CMD] Current FwCmdIOParam(%#x)\n", priv->FwCmdIOParam);

	if(bPostProcessing && !priv->SetFwCmdInProgress)
	{
		if(!priv->up)
		{
			RT_TRACE(COMP_CMD, "SetFwCmdIOTimerCallback(): driver is going to unload\n");
			return false;
		}
		priv->SetFwCmdInProgress = true;
		priv->CurrentFwCmdIO = FwCmdIO; 
	}
	else
	{
		return false;
	}

	phy_SetFwCmdIOCallback(dev);

	return true;
}

void ChkFwCmdIoDone(struct net_device* dev)
{
	u16 PollingCnt = 1000;		
	u32 tmpValue;
	
	do
	{
#ifdef TO_DO_LIST	
		if(RT_USB_CANNOT_IO(Adapter))
		{
			RT_TRACE(COMP_CMD, "ChkFwCmdIoDone(): USB can NOT IO!!\n");
			return;
		}
#endif		
		udelay(10); 
		tmpValue = read_nic_dword(dev, WFM5);
		if(tmpValue == 0)
		{
			RT_TRACE(COMP_CMD, "[FW CMD] Set FW Cmd success!!\n");
			break;
		}			
		else
		{
			RT_TRACE(COMP_CMD, "[FW CMD] Polling FW Cmd PollingCnt(%d)!!\n", PollingCnt);
		}
	}while( --PollingCnt );

	if(PollingCnt == 0)
	{
		RT_TRACE(COMP_ERR, "[FW CMD] Set FW Cmd fail!!\n");
	}
}
void phy_SetFwCmdIOCallback(struct net_device* dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	PRT_HIGH_THROUGHPUT	pHTInfo = priv->ieee80211->pHTInfo;
	rt_firmware		*pFirmware = priv->pFirmware;
	u32	input, CurrentAID = 0;;

		
	if(!priv->up)
	{
		RT_TRACE(COMP_CMD, "SetFwCmdIOTimerCallback(): driver is going to unload\n");
		return;
	}
	
	RT_TRACE(COMP_CMD, "--->SetFwCmdIOTimerCallback(): Cmd(%#x), SetFwCmdInProgress(%d)\n", priv->CurrentFwCmdIO, priv->SetFwCmdInProgress);

	if(pFirmware->FirmwareVersion >= 0x34)
	{
		switch(priv->CurrentFwCmdIO)
		{
			case FW_CMD_RA_REFRESH_N:
				priv->CurrentFwCmdIO = FW_CMD_RA_REFRESH_N_COMB;
			break;
			case FW_CMD_RA_REFRESH_BG:
				priv->CurrentFwCmdIO = FW_CMD_RA_REFRESH_BG_COMB;
			break;
			default:
			break;
		}
	}

	switch(priv->CurrentFwCmdIO)
	{
		case FW_CMD_RA_RESET:
			RT_TRACE(COMP_CMD,"[FW CMD] Set RA Reset!!\n");
			write_nic_dword(dev, WFM5, FW_RA_RESET);	
			ChkFwCmdIoDone(dev);	
			break;
			
		case FW_CMD_RA_ACTIVE:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA Active!!\n");
			write_nic_dword(dev, WFM5, FW_RA_ACTIVE); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_RA_REFRESH_N:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA n refresh!!\n");
			if(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL)
				input = FW_RA_REFRESH;
			else
				input = FW_RA_REFRESH | (pHTInfo->IOTRaFunc << 8);
			write_nic_dword(dev, WFM5, input); 
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_RA_ENABLE_RSSI_MASK); 
			ChkFwCmdIoDone(dev);	
			break;			
			
		case FW_CMD_RA_REFRESH_BG:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA BG refresh!!\n");
			write_nic_dword(dev, WFM5, FW_RA_REFRESH); 
			ChkFwCmdIoDone(dev);
			write_nic_dword(dev, WFM5, FW_RA_DISABLE_RSSI_MASK); 			
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_RA_REFRESH_N_COMB:
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA n Combo refresh!!\n");
			if(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL)
				input = FW_RA_IOT_N_COMB;
			else
				input = FW_RA_IOT_N_COMB | (((pHTInfo->IOTRaFunc)&0x0f) << 8);
			input = input |((pHTInfo->IOTPeer & 0xf) <<12);
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA/IOT Comb in n mode!! input(%#x)\n", input);
			write_nic_dword(dev, WFM5, input); 			
			ChkFwCmdIoDone(dev);	
			break;		

		case FW_CMD_RA_REFRESH_BG_COMB:				
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA B/G Combo refresh!!\n");
			if(pHTInfo->IOTRaFunc & HT_IOT_RAFUNC_DISABLE_ALL)
				input = FW_RA_IOT_BG_COMB;
			else
				input = FW_RA_IOT_BG_COMB | (((pHTInfo->IOTRaFunc)&0x0f) << 8);
			input = input |((pHTInfo->IOTPeer & 0xf) <<12);
			RT_TRACE(COMP_CMD, "[FW CMD] Set RA/IOT Comb in B/G mode!! input(%#x)\n", input);
			write_nic_dword(dev, WFM5, input); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_IQK_ENABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] IQK Enable!!\n");
			write_nic_dword(dev, WFM5, FW_IQK_ENABLE); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_TXPWR_TRACK_ENABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] TxPwr tracking Enable!!\n");
			write_nic_dword(dev, WFM5, FW_TXPWR_TRACK_ENABLE); 
			ChkFwCmdIoDone(dev);
			break;

		case FW_CMD_TXPWR_TRACK_DISABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] TxPwr tracking Disable!!\n");
			write_nic_dword(dev, WFM5, FW_TXPWR_TRACK_DISABLE); 
			ChkFwCmdIoDone(dev);	
			break;

		case FW_CMD_PAUSE_DM_BY_SCAN:
			RT_TRACE(COMP_CMD,"[FW CMD] Pause DM by Scan!!\n");
			rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x40);
			break;
		
		case FW_CMD_RESUME_DM_BY_SCAN:			
			RT_TRACE(COMP_CMD, "[FW CMD] Resume DM by Scan!!\n");
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x83);
			PHY_SetTxPowerLevel8192S(dev, priv->chan);
			break;
	
		case FW_CMD_HIGH_PWR_DISABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] High Pwr Disable!!\n");
			if(priv->DMFlag & HAL_DM_HIPWR_DISABLE)
				break;
			rtl8192_setBBreg(dev, rOFDM0_XAAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rOFDM0_XBAGCCore1, bMaskByte0, 0x17);
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x40);
			break;
			
		case FW_CMD_HIGH_PWR_ENABLE:
			RT_TRACE(COMP_CMD, "[FW CMD] High Pwr Enable!!\n");
			if(priv->DMFlag & HAL_DM_HIPWR_DISABLE)
				break;
			rtl8192_setBBreg(dev, rCCK0_CCA, bMaskByte2, 0x83);	
			break;

		case FW_CMD_LPS_ENTER:
			RT_TRACE(COMP_CMD, "[FW CMD] Enter LPS mode!!\n");
			CurrentAID = priv->ieee80211->assoc_id;
			write_nic_dword(dev, WFM5, (FW_LPS_ENTER| ((CurrentAID|0xc000)<<8))    );
			ChkFwCmdIoDone(dev);	
			pHTInfo->IOTAction |=  HT_IOT_ACT_DISABLE_EDCA_TURBO;
			break;

		case FW_CMD_LPS_LEAVE:
			RT_TRACE(COMP_CMD, "[FW CMD] Leave LPS mode!!\n");
			write_nic_dword(dev, WFM5, FW_LPS_LEAVE );
			ChkFwCmdIoDone(dev);	
			pHTInfo->IOTAction &=  (~HT_IOT_ACT_DISABLE_EDCA_TURBO);
			break;
		
		default:			
			break;
	}
	
	priv->SetFwCmdInProgress = false;
	RT_TRACE(COMP_CMD, "<---SetFwCmdIOWorkItemCallback()\n");

}

void
MlmeDisassociateRequest(
	struct net_device* dev,
	u8* 		asSta,
	u8		asRsn
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8 i;

	RemovePeerTS(priv->ieee80211, asSta);
	
	SendDisassociation( priv->ieee80211, 0, asRsn );
	
	if(memcpy(priv->ieee80211->current_network.bssid,asSta,6) == 0)
	{
		priv->ieee80211->state = IEEE80211_NOLINK;
		for(i=0;i<6;i++)  priv->ieee80211->current_network.bssid[i] = 0x22;
		priv->OpMode = RT_OP_MODE_NO_LINK;
		{
			RT_OP_MODE	OpMode = priv->OpMode;
			LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
			u8 	btMsr 	= MSR_NOLINK;

			btMsr &= 0xfc;

			switch(OpMode)
			{
			case RT_OP_MODE_INFRASTRUCTURE:
				btMsr |= MSR_LINK_MANAGED;
				break;

			case RT_OP_MODE_IBSS:
				btMsr |= MSR_LINK_ADHOC;
				break;

			case RT_OP_MODE_AP:
				btMsr |= MSR_LINK_MASTER;
				break;

			default:
				btMsr |= MSR_LINK_NONE;
				break;
			}

			write_nic_byte(dev, MSR, btMsr);

			priv->ieee80211->LedControlHandler(dev, LedAction);
		}
		ieee80211_disassociate(priv->ieee80211);

		write_nic_word(dev, BSSIDR, ((u16*)priv->ieee80211->current_network.bssid)[0]);
		write_nic_dword(dev, BSSIDR+2, ((u32*)(priv->ieee80211->current_network.bssid+2))[0]);
		
	}

}


void
MgntDisconnectIBSS(
	struct net_device* dev
) 
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u8			i;
	bool	bFilterOutNonAssociatedBSSID = false;


	priv->ieee80211->state = IEEE80211_NOLINK;

	for(i=0;i<6;i++)  priv->ieee80211->current_network.bssid[i]= 0x55;
	
	priv->OpMode = RT_OP_MODE_NO_LINK;

	write_nic_word(dev, BSSIDR, ((u16*)priv->ieee80211->current_network.bssid)[0]);
	write_nic_dword(dev, BSSIDR+2, ((u32*)(priv->ieee80211->current_network.bssid+2))[0]);	
	{
			RT_OP_MODE	OpMode = priv->OpMode;
			LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
			u8	btMsr = MSR_NOLINK;

			btMsr &= 0xfc;

			switch(OpMode)
			{
			case RT_OP_MODE_INFRASTRUCTURE:
				btMsr |= MSR_LINK_MANAGED;
				break;

			case RT_OP_MODE_IBSS:
				btMsr |= MSR_LINK_ADHOC;
				break;

			case RT_OP_MODE_AP:
				btMsr |= MSR_LINK_MASTER;
				break;

			default:
				btMsr |= MSR_LINK_NONE;
				break;
			}

			write_nic_byte(dev, MSR, btMsr);

			priv->ieee80211->LedControlHandler(dev, LedAction);
	}
	ieee80211_stop_send_beacons(priv->ieee80211);

#ifdef TO_DI_LIST 
	if(pMgntInfo->bEnableSwBeaconTimer)
	{
		PlatformCancelTimer( Adapter, &pMgntInfo->SwBeaconTimer);
	}
#endif

	bFilterOutNonAssociatedBSSID = false;
	{
			u32 Type;
			Type = bFilterOutNonAssociatedBSSID;

			if (Type == true)
				priv->ReceiveConfig |= (RCR_CBSSID);
			else
				priv->ReceiveConfig &= (~RCR_CBSSID);

			write_nic_dword(dev, RCR,priv->ReceiveConfig);
			
	}
	notify_wx_assoc_event(priv->ieee80211);

}

void
MgntDisconnectAP(
	struct net_device* dev,
	u8 asRsn
) 
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool bFilterOutNonAssociatedBSSID = false;


#ifdef TO_DO
	if(   pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch ||
		(pMgntInfo->bAPSuportCCKM && pMgntInfo->bCCX8021xenable) )	
	{
		SecClearAllKeys(dev);
		RT_TRACE(COMP_SEC, DBG_LOUD,("======>CCKM clear key..."))
	}
#endif

	bFilterOutNonAssociatedBSSID = false;
	{
			u32 Type;
			Type = bFilterOutNonAssociatedBSSID;

			
			if (Type == true)
				priv->ReceiveConfig |= (RCR_CBSSID);
			else 
				priv->ReceiveConfig &= (~RCR_CBSSID);

			write_nic_dword(dev, RCR,priv->ReceiveConfig);	
	}

	MlmeDisassociateRequest( dev, priv->ieee80211->current_network.bssid, asRsn );

	priv->ieee80211->state = IEEE80211_NOLINK;
}

bool
MgntDisconnect(
	struct net_device* dev,
	u8 asRsn
) 
{
	struct r8192_priv *priv = ieee80211_priv(dev);

#ifdef TO_DO
	if(pMgntInfo->mPss != eAwake)
	{
		PlatformSetTimer( dev, &(pMgntInfo->AwakeTimer), 0 );
	}
#endif
#ifdef TO_DO
	if(pMgntInfo->mActingAsAp)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnect() ===> AP_DisassociateAllStation\n"));
		AP_DisassociateAllStation(dev, unspec_reason);
		return TRUE;
	}	
#endif

	if( priv->ieee80211->state == IEEE80211_LINKED )
	{
		if( priv->ieee80211->iw_mode == IW_MODE_ADHOC )
		{
			MgntDisconnectIBSS(dev);
		}
		if( priv->ieee80211->iw_mode == IW_MODE_INFRA )
		{
			MgntDisconnectAP(dev, asRsn);
		}

		notify_wx_assoc_event(priv->ieee80211);
	}

	return true;
}

bool
MgntActSet_RF_State(
	struct net_device* dev,
	RT_RF_POWER_STATE	StateToSet,
	RT_RF_CHANGE_SOURCE ChangeSource
	)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	bool 			bActionAllowed = false; 
	bool 			bConnectBySSID = false;
	RT_RF_POWER_STATE	rtState;
	u16					RFWaitCounter = 0;
	unsigned long flag;
	RT_TRACE(COMP_POWER, "===>MgntActSet_RF_State(): StateToSet(%d)\n",StateToSet);
	printk("===>MgntActSet_RF_State(): StateToSet(%d)\n",StateToSet);


	while(true)
	{
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		if(priv->RFChangeInProgress)
		{
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
			RT_TRACE(COMP_POWER, "MgntActSet_RF_State(): RF Change in progress! Wait to set..StateToSet(%d).\n", StateToSet);
			
			while(priv->RFChangeInProgress)
			{
				RFWaitCounter ++;
				RT_TRACE(COMP_POWER, "MgntActSet_RF_State(): Wait 1 ms (%d times)...\n", RFWaitCounter);
				udelay(1000); 

				if(RFWaitCounter > 100)
				{
					RT_TRACE(COMP_ERR, "MgntActSet_RF_State(): Wait too logn to set RF\n");
					return false;
				}
			}
		}
		else
		{
			priv->RFChangeInProgress = true;
			spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
			break;
		}
	}

	rtState = priv->ieee80211->eRFPowerState;

	switch(StateToSet) 
	{
	case eRfOn:

		priv->ieee80211->RfOffReason &= (~ChangeSource);

		if(! priv->ieee80211->RfOffReason)
		{
			priv->ieee80211->RfOffReason = 0;
			bActionAllowed = true;

			
			if(rtState == eRfOff && ChangeSource >=RF_CHANGE_BY_HW )
			{
				bConnectBySSID = true;
			}
		}
		else{
			RT_TRACE(COMP_POWER, "MgntActSet_RF_State - eRfon reject pMgntInfo->RfOffReason= 0x%x, ChangeSource=0x%X\n", priv->ieee80211->RfOffReason, ChangeSource);
			printk("MgntActSet_RF_State - eRfon reject pMgntInfo->RfOffReason= 0x%x, ChangeSource=0x%X\n", priv->ieee80211->RfOffReason, ChangeSource);
                }

		break;

	case eRfOff:

			if (priv->ieee80211->RfOffReason > RF_CHANGE_BY_IPS)
			{
				MgntDisconnect(dev, disas_lv_ss);
	
	
				
			}


		priv->ieee80211->RfOffReason |= ChangeSource;
		bActionAllowed = true;
		break;

	case eRfSleep:
		priv->ieee80211->RfOffReason |= ChangeSource;
		bActionAllowed = true;
		break;

	default:
		break;
	}

	if(bActionAllowed)
	{
		RT_TRACE(COMP_POWER, "MgntActSet_RF_State(): Action is allowed.... StateToSet(%d), RfOffReason(%#X)\n", StateToSet, priv->ieee80211->RfOffReason);
		PHY_SetRFPowerState(dev, StateToSet);	
		if(StateToSet == eRfOn) 
		{				
			if(bConnectBySSID)
			{				
			}
		}
		else if(StateToSet == eRfOff)
		{		
		}		
	}
	else
	{
		RT_TRACE(COMP_POWER, "MgntActSet_RF_State(): Action is rejected.... StateToSet(%d), ChangeSource(%#X), RfOffReason(%#X)\n", StateToSet, ChangeSource, priv->ieee80211->RfOffReason);
	}

	spin_lock_irqsave(&priv->rf_ps_lock,flag);
	priv->RFChangeInProgress = false;
	spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
	
	RT_TRACE(COMP_POWER, "<===MgntActSet_RF_State()\n");
	return bActionAllowed;
}

