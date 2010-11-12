/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 *
 * Based on the r8180 driver, which is:
 * Copyright 2004-2005 Andrea Merello <andreamrl@tiscali.it>, et al.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
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
#include "../rtl_core.h"
#include "../rtl_wx.h"

#ifdef _RTL8192_EXT_PATCH_
#include "../../../mshclass/msh_class.h"
#include "../rtl_mesh.h"
#endif

#include <asm/div64.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) && defined(USE_FW_SOURCE_IMG_FILE)
#include <linux/firmware.h>
#endif
extern int WDCAPARA_ADD[];
extern int hwwep; 



#define		FPGA_UNKNOWN			0
#define		FPGA_2MAC				1
#define		FPGA_PHY				2
#define		ASIC					3
#define		BOARD_TYPE				ASIC

#if BOARD_TYPE == FPGA_2MAC
#define		HAL_FW_ENABLE			0
#define		HAL_MAC_ENABLE			0
#define		HAL_BB_ENABLE			0
#define		HAL_RF_ENABLE			0
#else 
#define 		HAL_FW_ENABLE			1
#define		HAL_MAC_ENABLE			1
#define		HAL_BB_ENABLE			1
#define		HAL_RF_ENABLE			1

#define		FPGA_RF_UNKOWN		0
#define		FPGA_RF_8225			1
#define 		FPGA_RF_0222D			2
#define		FPGA_RF 				FPGA_RF_0222D
#endif

static u8 LegacyRateSet[12] = {0x02 , 0x04 , 0x0b , 
						0x16 , 0x0c , 0x12 , 
						0x18 , 0x24 , 0x30 , 
						0x48 , 0x60 , 0x6c};

bool
GetHalDefVar8192CE(
	struct net_device *dev,
	HAL_DEF_VARIABLE		eVariable,
	void*				pValue
	)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	bool		bResult = true;

	switch(eVariable)
	{
	case HAL_DEF_WOWLAN:
#if 0
		{	
			PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(priv);
			if(priv->bUnloadDriverwhenS3S4)
			{	
				*((bool*)pValue) = false;
			}
			else
			{
				if(pPSC->WoWLANMode) 
					*((bool*)pValue) = true; 
				else 
					*((bool*)pValue) = false;
			}
			
			RT_TRACE(COMP_INIT, "GetHalDefVar8192CE(): HAL_DEF_WOWLAN: bUnloadDriverwhenS3S4 = %d, WoWLANMode = %d\n",
				priv->bUnloadDriverwhenS3S4, pPSC->WoWLANMode);
		}
#else
		bResult = false;
#endif
		break;
		
	case HAL_DEF_THERMAL_VALUE:
		*((u8*)pValue) = priv->ThermalValue;
		break;
		
	case HAL_DEF_PCI_SUUPORT_L1_BACKDOOR:		
		*((bool*)pValue) = priv->bSupportBackDoor;		
		break;
		
	default:
		break;
	}

	return bResult;
}

void 
rtl8192ce_GetHwReg(struct net_device *dev,u8 variable,u8* val)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	
	switch(variable)	
	{
	case HW_VAR_ETHER_ADDR:
		*((u32*)(val)) = read_nic_dword(dev, REG_MACID);
		*((u16*)(val+4)) = read_nic_word(dev, REG_MACID+4);
		break;

	case HW_VAR_BASIC_RATE:
		*((u16*)(val)) = priv->basic_rate;
		break;

	case HW_VAR_BSSID:		
		*((u32*)(val)) = read_nic_dword(dev, REG_BSSID);
		*((u16*)(val+4)) = read_nic_word(dev, REG_BSSID+4);
		break;

	case HW_VAR_MEDIA_STATUS:
		val[0] = read_nic_byte(dev, REG_CR+2)&0x3;
		break;

	case HW_VAR_SLOT_TIME:
		*((u8*)(val)) = priv->slot_time;
		break;

	case HW_VAR_BEACON_INTERVAL:
		*((u16*)(val)) = read_nic_word(dev, REG_BCN_INTERVAL);
		break;

	case HW_VAR_ATIM_WINDOW:
		*((u16*)(val)) =  read_nic_word(dev, REG_ATIMWND);
		break;

	case HW_VAR_CONTENTION_WINDOW:
		*((u16*)(val)) = 0x0;
		break;

	case HW_VAR_RETRY_COUNT:
		*((u16*)(val)) = 0x0;
		break;

	case HW_VAR_RF_STATE:
		*((RT_RF_POWER_STATE *)(val)) = priv->rtllib->eRFPowerState;
		break;

	case HW_VAR_RF_OFF_BY_HW:
		*((bool *)(val)) = priv->bHwRadioOff;
		break;

	case HW_VAR_BUS_SPEED:
		*((u32*)(val)) = 55000000; 
		break;
			
	case HW_VAR_RCR:
		*((u32*)(val)) = priv->ReceiveConfig;
		break;

	case HW_VAR_EFUSE_USAGE: 
		*((u32*)(val)) = (priv->EfuseUsedPercentage<<16)|(priv->EfuseUsedBytes);	
		break;

	case HW_VAR_EFUSE_BYTES: 
		*((u16*)(val)) = priv->EfuseUsedBytes;	
		break;

	case HW_VAR_AUTOLOAD_STATUS: 
		*((bool*)(val)) = priv->AutoloadFailFlag;
		break;

	case HW_VAR_CCX_CHNL_LOAD:
		{
			RT_TRACE(COMP_INIT,"FIX ME!!!How to do  get HW_VAR_CCX_CHNL_LOAD in 92C??\n");
			break;			
		}
		break;

	case HW_VAR_CCX_NOISE_HISTOGRAM:
		{
			RT_TRACE(COMP_INIT,"FIX ME!!!How to do  get HW_VAR_CCX_NOISE_HISTOGRAM in 92C??\n");
			break;
		}
		break;

#ifdef MERGE_TO_DO
	case HW_VAR_INIT_TX_RATE: 
		{
			u8 RateIdx = read_nic_byte(dev, REG_TX_RATE_REG);
			if(RateIdx < 76)
				*((u8*)(val)) = (RateIdx<12)?(LegacyRateSet[RateIdx]):((RateIdx-12)|0x80);
			else
				*((u8*)(val)) = 0;
		}
		break;
#endif

		
	case HW_VAR_BCN_VALID:
		{
			*((u8*)(val)) = read_nic_byte(dev, REG_TDECTRL+2);
		}
		break;

	case HW_VAR_FWLPS_RF_ON:
		{
			u8	valSFE, valAPSD;
			valSFE = read_nic_byte(dev, REG_SYS_FUNC_EN);
			valSFE &= BIT0;
			valAPSD = read_nic_byte(dev, REG_APSD_CTRL);
			valAPSD = (valAPSD&BIT6)>>6;
			if(valSFE && !valAPSD)
				*((bool *)(val)) = true;
			else
				*((bool *)(val)) = false;
		}
		break;
		
	case HW_VAR_INIT_TX_RATE:
		{
			u8 RateIdx = read_nic_byte(dev, REG_INIDATA_RATE_SEL);
			RateIdx &= 0x3f;
				
			if(RateIdx == 28)
				*((u8*)(val)) = 0x8f;
			else if(RateIdx < 28)
				*((u8*)(val)) = (RateIdx<12)?(LegacyRateSet[RateIdx]):((RateIdx-12)|0x80);
			else
				*((u8*)(val)) = 0;
		}
		break;
		
	case HW_VAR_INT_MIGRATION:
		{
			*((bool*)(val)) = priv->bInterruptMigration;
		}
		break;

	case HW_VAR_INT_AC:
		{
			*((bool*)(val)) = priv->bDisableTxInt;
		}
		break;	
		
	default:
		break;
	}
}

void
rtl8192ce_SetHwReg(struct net_device *dev,u8 variable,u8* val)
{
	struct r8192_priv* priv = rtllib_priv(dev);


	switch(variable)
	{
		case HW_VAR_ETHER_ADDR:
		{
			u8	idx = 0;
			for(idx=0; idx <6 ; idx++)
			{
				write_nic_byte(dev, (REG_MACID+idx), val[idx]);
			}
		}
		break;

		case HW_VAR_MULTICAST_REG:
		break;

		case HW_VAR_BASIC_RATE:
		{
			u16			BrateCfg = 0;
			u8			RateIndex = 0;

			
			rtl8192_config_rate(dev, &BrateCfg);

#if 0
			if(	(priv->bt_coexist.BluetoothCoexist) &&
				(priv->bt_coexist.BT_CoexistType == BT_CSR)	)
			{
					priv->basic_rate = BrateCfg = BrateCfg & 0x151;
					RTPRINT(FBT, BT_TRACE, ("BT temp disable cck 2/5.5/11M, (0x%x = 0x%x)\n", REG_RRSR, BrateCfg & 0x151));
			}
			else
#endif
			{
				priv->basic_rate = BrateCfg = BrateCfg & 0x15f;
			}

  	                if(priv->rtllib->pHTInfo->IOTPeer == HT_IOT_PEER_CISCO && ((BrateCfg &0x150)==0))
			{
				BrateCfg |=0x010;
			}

			BrateCfg |= 0x01; 
			
#ifdef CONFIG_BT_30
			if(priv->BtInfo.BtOperationOn)
				BrateCfg |= 0x15f;
#endif
			
			write_nic_byte(dev, REG_RRSR, BrateCfg&0xff);
			write_nic_byte(dev, REG_RRSR+1, (BrateCfg>>8)&0xff);

			while(BrateCfg > 0x1)
			{
				BrateCfg = (BrateCfg>> 1);
				RateIndex++;
			}
			write_nic_byte(dev, REG_INIRTS_RATE_SEL, RateIndex);
		}
		break;

		case HW_VAR_BSSID:
		{
			u8	idx =0;
			for(idx = 0 ; idx < 6; idx++)
			{
				write_nic_byte(dev, (REG_BSSID+idx), val[idx]);
			}
		}
		break;
	
		case HW_VAR_MEDIA_STATUS:
		{
			RT_OP_MODE	OpMode = (RT_OP_MODE)(*((u8 *)(val)));
			LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
			u8		btMsr = read_nic_byte(dev, MSR);

			btMsr &= 0xfc;

#ifdef _RTL8192_EXT_PATCH_
			if(OpMode == RT_OP_MODE_NO_LINK || ((OpMode == RT_OP_MODE_INFRASTRUCTURE) && (priv->rtllib->iw_mode != IW_MODE_MESH)))
#else
			if(OpMode == RT_OP_MODE_NO_LINK || OpMode == RT_OP_MODE_INFRASTRUCTURE)
#endif
			{
				StopTxBeacon(dev);
				EnableBcnSubFunc(dev);
			}
#ifdef _RTL8192_EXT_PATCH_
			else if(OpMode == RT_OP_MODE_IBSS || OpMode == RT_OP_MODE_AP 
				|| ((OpMode == RT_OP_MODE_INFRASTRUCTURE) && (priv->rtllib->iw_mode == IW_MODE_MESH) && (!priv->rtllib->only_mesh)))
#else
			else if(OpMode == RT_OP_MODE_IBSS || OpMode == RT_OP_MODE_AP)
#endif
			{
				ResumeTxBeacon(dev);
				DisableBcnSubFunc(dev);
			}
			else
			{
				RT_TRACE(COMP_MLME, "Set HW_VAR_MEDIA_STATUS: No such media status(%x).\n", OpMode);
			}
			
			switch(OpMode)
			{
			case RT_OP_MODE_INFRASTRUCTURE:
				btMsr |= MSR_INFRA;
				LedAction = LED_CTL_LINK;
				break;

			case RT_OP_MODE_IBSS:
				btMsr |= MSR_ADHOC;
				break;

			case RT_OP_MODE_AP:
				btMsr |= MSR_AP;
				LedAction = LED_CTL_LINK;
				break;

			default:
				btMsr |= MSR_NOLINK;
				break;
			}

			write_nic_byte(dev, REG_CR+2, btMsr);

#if 0 
			if(OpMode == RT_OP_MODE_INFRASTRUCTURE)
			{
				 write_nic_byte(dev, REG_BCN_MAX_ERR, 0xFF);
			}
			else
			{
				 write_nic_byte(dev, REG_BCN_MAX_ERR, 0x10);
			}
#endif	


#if 0
			if(OpMode == RT_OP_MODE_IBSS || OpMode == RT_OP_MODE_AP)
			{
				u8	U1bTmp;
				U1bTmp = read_nic_byte(dev, REG_FWHW_TXQ_CTRL+2);
				write_nic_byte(dev, REG_FWHW_TXQ_CTRL+2, (U1bTmp|BIT6));
			}
#endif			
			priv->rtllib->LedControlHandler(dev, LedAction);

			if((btMsr&0xfc) == MSR_AP)
				write_nic_byte(dev, REG_BCNTCFG+1, 0x00);
			else
				write_nic_byte(dev, REG_BCNTCFG+1, 0x66);

		}
		break;


		case HW_VAR_BEACON_INTERVAL:
		
		break;
			
		case HW_VAR_CONTENTION_WINDOW:
		break;
			
		case HW_VAR_RETRY_COUNT:		
		break;

		case HW_VAR_SIFS:		
			write_nic_byte(dev, REG_SIFS_CTX+1, val[0]);
			write_nic_byte(dev, REG_SIFS_TRX+1, val[1]);
		
			write_nic_byte(dev,REG_SPEC_SIFS+1, val[0]);
			write_nic_byte(dev,REG_MAC_SPEC_SIFS+1, val[0]);

			if(priv->rtllib->mode & (WIRELESS_MODE_G | WIRELESS_MODE_B))
				write_nic_word(dev, REG_RESP_SIFS_OFDM, 0x0e0e);
			else
				write_nic_word(dev, REG_RESP_SIFS_OFDM,  *((u16*)val));
			
		break;

		case HW_VAR_DIFS:
		break;

		case HW_VAR_EIFS:
		break;

		case HW_VAR_SLOT_TIME:
		{
			AC_CODING	eACI;

			priv->slot_time = val[0];		
			write_nic_byte(dev, REG_SLOT, val[0]);


			if(priv->rtllib->current_network.qos_data.supported !=0)
			{
				for(eACI = 0; eACI < AC_MAX; eACI++)
				{
					priv->rtllib->SetHwRegHandler(dev, HW_VAR_AC_PARAM, (u8*)(&eACI));
				}
			}
			else
			{
				u8	u1bAIFS = aSifsTime + (2 * priv->slot_time);
				
				write_nic_byte(dev, REG_EDCA_VO_PARAM, u1bAIFS);
				write_nic_byte(dev, REG_EDCA_VI_PARAM, u1bAIFS);
				write_nic_byte(dev, REG_EDCA_BE_PARAM, u1bAIFS);
				write_nic_byte(dev, REG_EDCA_BK_PARAM, u1bAIFS);
			}
		}
		break;

		case HW_VAR_ACK_PREAMBLE:	
		{
			u8	regTmp;
			priv->short_preamble = (bool)(*(u8*)val );
			regTmp = (priv->nCur40MhzPrimeSC)<<5;		
			if(priv->short_preamble)
				regTmp |= 0x80;

			write_nic_byte(dev, REG_RRSR+2, regTmp);
		}
		break;	

		case HW_VAR_CW_CONFIG:
		break;

		case HW_VAR_CW_VALUES:
	
		break;

		case HW_VAR_RATE_FALLBACK_CONTROL:
		break;

		case HW_VAR_COMMAND:
			write_nic_word(dev, REG_CR, *((u8*)val)); 
		break;

		case HW_VAR_WPA_CONFIG:		
			write_nic_byte(dev, REG_SECCFG, *((u8*)val));
		break;

		case HW_VAR_AMPDU_MIN_SPACE:
		{
			u8	MinSpacingToSet;
			u8	SecMinSpace;

			MinSpacingToSet = *((u8*)val);
			if(MinSpacingToSet <= 7)
			{
				if((priv->rtllib->current_network.capability & WLAN_CAPABILITY_PRIVACY) == 0)  
					SecMinSpace = 0;
				else if (priv->rtllib->rtllib_ap_sec_type && 
						(priv->rtllib->rtllib_ap_sec_type(priv->rtllib) 
							 & (SEC_ALG_WEP|SEC_ALG_TKIP))) 
					SecMinSpace = 7;
				else
					SecMinSpace = 0;

				if(MinSpacingToSet < SecMinSpace)
					MinSpacingToSet = SecMinSpace;
				priv->rtllib->MinSpaceCfg = ((priv->rtllib->MinSpaceCfg&0xf8) |MinSpacingToSet);
				*val = MinSpacingToSet;
				RT_TRACE(COMP_MLME, "Set HW_VAR_AMPDU_MIN_SPACE: %#x\n", priv->rtllib->MinSpaceCfg);
				write_nic_byte(dev, REG_AMPDU_MIN_SPACE, priv->rtllib->MinSpaceCfg);	
			}
		}
		break;	

		case HW_VAR_SHORTGI_DENSITY:
		{
			u8	DensityToSet;

			DensityToSet = *((u8*)val);		
			priv->rtllib->MinSpaceCfg|= (DensityToSet<<3);		
			RT_TRACE(COMP_MLME, "Set HW_VAR_SHORTGI_DENSITY: %#x\n", priv->rtllib->MinSpaceCfg);
			write_nic_byte(dev, REG_AMPDU_MIN_SPACE, priv->rtllib->MinSpaceCfg);
		}		
		break;
		
		case HW_VAR_AMPDU_FACTOR:
		{
			u8	FactorToSet;
			u32	RegToSet = 0xb972a841;
			u8*	pTmpByte = NULL;
			u8	index = 0;

			if(	(priv->bt_coexist.BluetoothCoexist) &&
				(priv->bt_coexist.BT_CoexistType == BT_CSR) )
				RegToSet = 0x97427431;
			else
				RegToSet = 0xb972a841;
			
#if 1
			FactorToSet = *((u8*)val);
			if(FactorToSet <= 3)
			{
				FactorToSet = (1<<(FactorToSet + 2));
				if(FactorToSet>0xf)
					FactorToSet = 0xf;

				for(index=0; index<4; index++)
				{
					pTmpByte = (u8*)(&RegToSet) + index;

					if((*pTmpByte & 0xf0) > (FactorToSet<<4))
						*pTmpByte = (*pTmpByte & 0x0f) | (FactorToSet<<4);
				
					if((*pTmpByte & 0x0f) > FactorToSet)
						*pTmpByte = (*pTmpByte & 0xf0) | (FactorToSet);
				}

				{
					write_nic_dword(dev, REG_AGGLEN_LMT, RegToSet);
					RT_TRACE(COMP_MLME, "Set HW_VAR_AMPDU_FACTOR: %#x\n", FactorToSet);
				}
			}
#endif
		}
		break;

		case HW_VAR_AC_PARAM:
		{
			u8	pAcParam = *((u8*)val);
#ifdef MERGE_TO_DO
			u32	eACI = GET_WMM_AC_PARAM_ACI(pAcParam); 
#else
			u32	eACI = pAcParam;
#endif
			u8		u1bAIFS;
			u32		u4bAcParam;
			u8 mode = priv->rtllib->mode;
			struct rtllib_qos_parameters *qos_parameters = &priv->rtllib->current_network.qos_data.parameters;


			u1bAIFS = qos_parameters->aifs[pAcParam] * ((mode&(IEEE_G|IEEE_N_24G)) ?9:20) + aSifsTime; 

			DM_InitEdcaTurbo(dev);

			u4bAcParam = (	(((u32)(qos_parameters->tx_op_limit[pAcParam])) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
							(((u32)(qos_parameters->cw_max[pAcParam])) << AC_PARAM_ECW_MAX_OFFSET)	|  
							(((u32)(qos_parameters->cw_min[pAcParam])) << AC_PARAM_ECW_MIN_OFFSET)	|  
							(((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET)	);

			printk("%s():HW_VAR_AC_PARAM eACI:%x:%x\n", __func__,eACI, u4bAcParam);
			switch(eACI)
			{
			case AC1_BK:
				write_nic_dword(dev, REG_EDCA_BK_PARAM, u4bAcParam);
				break;

			case AC0_BE:
				write_nic_dword(dev, REG_EDCA_BE_PARAM, u4bAcParam);
				break;

			case AC2_VI:
				write_nic_dword(dev, REG_EDCA_VI_PARAM, u4bAcParam);
				break;

			case AC3_VO:
				write_nic_dword(dev, REG_EDCA_VO_PARAM, u4bAcParam);
				break;

			default:
				RT_ASSERT(false, ("SetHwReg8185(): invalid ACI: %d !\n", eACI));
				break;
			}
			if(priv->AcmMethod != eAcmWay2_SW)
				priv->rtllib->SetHwRegHandler(dev, HW_VAR_ACM_CTRL, (u8*)(&pAcParam));			
		}
		break;


	case HW_VAR_ACM_CTRL:
		{
			struct rtllib_qos_parameters *qos_parameters = &priv->rtllib->current_network.qos_data.parameters;
			u8	pAcParam = *((u8*)val);
#ifdef MERGE_TO_DO
			u32	eACI = GET_WMM_AC_PARAM_ACI(pAciAifsn); 
#else
			u32	eACI = pAcParam;
#endif
			PACI_AIFSN	pAciAifsn = (PACI_AIFSN)&(qos_parameters->aifs[0]);
			u8		ACM = pAciAifsn->f.ACM;
			u8		AcmCtrl = read_nic_byte( dev, REG_ACMHWCTRL);

			printk("===========>%s():HW_VAR_ACM_CTRL:%x\n", __func__,eACI);
			AcmCtrl = AcmCtrl | ((priv->AcmMethod == 2)?0x0:0x1);

			if( ACM )
			{ 
				switch(eACI)
				{
				case AC0_BE:
					AcmCtrl |= AcmHw_BeqEn;
					break;

				case AC2_VI:
					AcmCtrl |= AcmHw_ViqEn;
					break;

				case AC3_VO:
					AcmCtrl |= AcmHw_VoqEn;
					break;

				default:
					RT_TRACE( COMP_QOS, "SetHwReg8185(): [HW_VAR_ACM_CTRL] ACM set failed: eACI is %d\n", eACI );
					break;
				}
			}
			else
			{ 
				switch(eACI)
				{
				case AC0_BE:
					AcmCtrl &= (~AcmHw_BeqEn);
					break;

				case AC2_VI:
					AcmCtrl &= (~AcmHw_ViqEn);
					break;

				case AC3_VO:
					AcmCtrl &= (~AcmHw_BeqEn);
					break;

				default:
					break;
				}
			}

			RT_TRACE( COMP_QOS, "SetHwReg8190pci(): [HW_VAR_ACM_CTRL] Write 0x%X\n", AcmCtrl );
			write_nic_byte(dev, REG_ACMHWCTRL, AcmCtrl );
		}
		break;

		case HW_VAR_DIS_Req_Qsize:	

		break;
	
		case HW_VAR_RCR:
		{
			write_nic_dword(dev, REG_RCR,((u32*)(val))[0]);
			priv->ReceiveConfig = ((u32*)(val))[0];
		}
		break;

		case HW_VAR_RATR_0:
		{
			u32 ratr_value;
			ratr_value = ((u32*)(val))[0];
			if(priv->rf_type == RF_1T2R)	
			{
				ratr_value &=~ (RATE_ALL_OFDM_2SS);
			}
			priv->CurrentRATR0 = ratr_value;
		}
		break;

		case HW_VAR_CPU_RST:
		break;
		
		case HW_VAR_RF_STATE:
		{
			RT_RF_POWER_STATE eStateToSet = (RT_RF_POWER_STATE)(*((u8 *)val));
			PHY_SetRFPowerState(dev, eStateToSet);
		}
		break;	
		
		case HW_VAR_CECHK_BSSID:
		{
			u32	RegRCR, Type;

			Type = ((u8*)(val))[0];
			RTPRINT(FIOCTL, IOCTL_STATE, ("HW_VAR_CHECK_BSSID, set to %d\n", Type));
			priv->rtllib->GetHwRegHandler(dev, HW_VAR_RCR, (u8*)(&RegRCR));

#ifdef CONFIG_BT_30
			if(priv->BtInfo.BtOperationOn)
				Type = false;
#endif
						
			if (Type == true)
			{
				if(IS_NORMAL_CHIP(priv->card_8192_version))
				{
					RegRCR |= (RCR_CBSSID_DATA | RCR_CBSSID_BCN);
					
					priv->rtllib->SetHwRegHandler(dev, HW_VAR_RCR, (u8*)(&RegRCR));
					SetBcnCtrlReg(dev, 0, BIT4);
				}
				else
				{
					RegRCR |= (RCR_CBSSID);

					priv->rtllib->SetHwRegHandler(dev, HW_VAR_RCR, (u8*)(&RegRCR));
					SetBcnCtrlReg(dev, 0, (BIT4|BIT5));
				}
			}
			else if (Type == false)
			{
				if(IS_NORMAL_CHIP(priv->card_8192_version))
				{
					RegRCR &= (~(RCR_CBSSID_DATA | RCR_CBSSID_BCN));

					SetBcnCtrlReg(dev, BIT4, 0);
					priv->rtllib->SetHwRegHandler(dev, HW_VAR_RCR, (u8*)(&RegRCR));
				}
				else
				{
					RegRCR &= (~RCR_CBSSID);

					SetBcnCtrlReg(dev, (BIT4|BIT5), 0);
					priv->rtllib->SetHwRegHandler(dev, HW_VAR_RCR, (u8*)(&RegRCR));
				}
			}
		}
		break;

		case HW_VAR_USER_CONTROL_TURBO_MODE:
		{
#ifdef MERGE_TO_DO
			bool bForcedDisable = ((u8*)(val))[0];
			priv->bForcedDisableTurboEDCA = bForcedDisable;
#endif
		}
		break;

		case HW_VAR_RETRY_LIMIT:
		{
			u8 RetryLimit = ((u8*)(val))[0];
			
			priv->ShortRetryLimit = RetryLimit;
			priv->LongRetryLimit = RetryLimit;
			
			write_nic_word(dev, 	REG_RL,
							RetryLimit << RETRY_LIMIT_SHORT_SHIFT | \
							RetryLimit << RETRY_LIMIT_LONG_SHIFT);
		}					
		break;

		case HW_VAR_EFUSE_USAGE: 
			priv->EfuseUsedPercentage = *((u8*)val);	
		break;

		case HW_VAR_EFUSE_BYTES: 
			priv->EfuseUsedBytes = *((u16*)val);			
		break;

		case HW_VAR_AUTOLOAD_STATUS:		
			priv->AutoloadFailFlag = *((bool*)val);
		break;

		case HW_VAR_RF_2R_DISABLE:
		{

			RT_TRACE(COMP_INIT,"FIX ME!!!How to do  set HW_VAR_RF_2R_DISABLE in 92C??\n");
			break;
		}
		break;

		case HW_VAR_H2C_FW_PWRMODE:
			RT_TRACE(COMP_INIT,"FIX ME!!!How to do  set HW_VAR_H2C_FW_PWRMODE in 92C??\n");
			break;
		break;

		case HW_VAR_H2C_FW_JOINBSSRPT:
			RT_TRACE(COMP_INIT,"FIX ME!!!How to do  set HW_VAR_H2C_FW_JOINBSSRPT in 92C??\n");
			break;
		break;

		case HW_VAR_CCX_CLM_NHM:
		{ 
		}
		break;

		case HW_VAR_CCX_CHNL_LOAD:
		{
		}
		break;

		case HW_VAR_CCX_NOISE_HISTOGRAM:
		{
		}
		break;
		
		case HW_VAR_SET_RPWM:
		{	
			u8	RpwmVal;
			
			RpwmVal = read_nic_byte(dev, REG_PCIE_HRPWM);
			udelay(1);
			if(RpwmVal & BIT7) 
			{
				write_nic_byte(dev, REG_PCIE_HRPWM, (*(u8*)val));
			}
			else 
			{
				write_nic_byte(dev, REG_PCIE_HRPWM, ((*(u8*)val)|BIT7));
			}
		}
		break;
		
		case HW_VAR_HANDLE_FW_C2H: 
		break;

		case HW_VAR_DL_FW_RSVD_PAGE:
		{
		}
		break;

		case HW_VAR_AID:
		{
			u16	U2bTmp;
			U2bTmp = read_nic_word(dev, REG_BCN_PSR_RPT);
			U2bTmp &= 0xC000;
			write_nic_word(dev, REG_BCN_PSR_RPT, (U2bTmp|priv->rtllib->assoc_id));
		}

		case HW_VAR_HW_SEQ_ENABLE:
		{
			write_nic_word(dev, REG_NQOS_SEQ, ((priv->rtllib->seq_ctrl[0]+100)&0xFFF));
			write_nic_byte(dev, REG_HWSEQ_CTRL, 0xFF);
		}
		break;

	case HW_VAR_CORRECT_TSF:
#if 0
		{
			#define sTU 1024
			
			u64	U8bTmp;
			bool	bTypeIbss = ((u8*)(val))[0];

			priv->rtllib->bdTstamp = priv->rtllib->current_network.time_stamp[1];
			priv->rtllib->bdTstamp <<= 32;
			priv->rtllib->bdTstamp |= priv->rtllib->current_network.time_stamp[0];

			U8bTmp = priv->rtllib->bdTstamp - do_div(priv->rtllib->bdTstamp, (priv->rtllib->current_network.beacon_interval*1024)) -1024; 
			printk("---------------bdTstamp>%#x:%#x, beacon_inter:%d\n", (u32)priv->rtllib->bdTstamp, (u32)(priv->rtllib->bdTstamp>>32), priv->rtllib->current_network.beacon_interval);
			printk("---------------old>%#x:%#x\n", read_nic_dword(dev, REG_TSFTR),read_nic_dword(dev, REG_TSFTR+4));
			printk("---------------new>%#x:%#x\n", (u32)U8bTmp, (u32)(U8bTmp>>32));

			if(bTypeIbss == true)
				StopTxBeacon(dev);
			
			SetBcnCtrlReg(dev, 0, BIT3);

			write_nic_dword(dev, REG_TSFTR, U8bTmp);
			write_nic_dword(dev, REG_TSFTR+4, (u32)(U8bTmp>>32));

			SetBcnCtrlReg(dev, BIT3, 0);

			if(bTypeIbss == true)
				ResumeTxBeacon(dev);
		}
#endif
		break;
		
	case HW_VAR_BCN_VALID:
		{
			u8	BcnValid = ((u8*)(val))[0];
			write_nic_byte(dev, REG_TDECTRL+2, BcnValid);
		}
		break;

	case HW_VAR_IO_CMD:
		HalSetIO8192C(dev,(*(PIO_TYPE)val));
		break;

	case HW_VAR_DUAL_TSF_RST: 
		write_nic_byte(dev, REG_DUAL_TSF_RST, (BIT0|BIT1));
		break;

	case HW_VAR_WF_MASK:
		{
#ifdef MERGE_TO_DO
			
			u8		Index = (*(u8*)val);			
			PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(priv);
			PRT_PM_WOL_PATTERN_INFO pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);

			if(pPmWoLPatternInfo[Index].CrcRemainder == 0xffff) 
			{
				WKFMCAMDeleteOneEntry(dev, Index);
			}
			else
			{
				WKFMCAMAddOneEntry(dev, Index, 0);
			}
#endif
		}
		break;

	
	case HW_VAR_WF_IS_MAC_ADDR:
		{	
			u8		u1btmp;
			bool		bUWF;
			
			bUWF = (bool)((u8*)(val))[0];

			if(bUWF)
			{	
				u1btmp = read_nic_byte(dev, REG_WOW_CTRL);
				write_nic_byte(dev, REG_WOW_CTRL, (u1btmp|WOW_UWF));
			}
			else
			{	
				u1btmp = read_nic_byte(dev, REG_WOW_CTRL);
				u1btmp &= ~(WOW_UWF);
				write_nic_byte(dev, REG_WOW_CTRL, u1btmp);
			}
		}
		break;

	case HW_VAR_SWITCH_EPHY_WoWLAN:
		{
			bool	bSleep = (bool)((u8*)(val))[0];

			PHY_SwitchEphyParameter(dev,  bSleep); 
		}
		break;
		
	case HW_VAR_INT_MIGRATION:
		{	
			
			bool	bIntMigration = *(bool*)(val);			
			
			if(bIntMigration)
			{	
				write_nic_io_dword(dev, REG_INT_MIG, 0xffff0fa0);
				priv->bInterruptMigration = bIntMigration;
			}
			else
			{	
				write_nic_io_dword(dev, REG_INT_MIG, 0);		
				priv->bInterruptMigration = bIntMigration;
			}
		}
		break;

	case HW_VAR_INT_AC:
		{
			
			bool	 bDisableACInt = *((bool*)val);	
						
			if(bDisableACInt) 
			{		
				priv->rtllib->UpdateInterruptMaskHandler( dev, 0, RT_AC_INT_MASKS );
				priv->bDisableTxInt = bDisableACInt;
			}
			else
			{
				priv->rtllib->UpdateInterruptMaskHandler( dev, RT_AC_INT_MASKS, 0 );
				priv->bDisableTxInt = bDisableACInt;
			}
		}
		break;
		
	default:
		break;
	}
	
}

bool
rtl8192ce_NicIFSetMacAddress(
	struct net_device* dev,
	u8*          	addrbuf
)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);

	priv->rtllib->SetHwRegHandler( dev, HW_VAR_ETHER_ADDR, addrbuf);
#ifdef MERGE_TO_DO
	memcpy(priv->CurrentAddress, addrbuf, 6);
#endif
	return true;
}

u8 getChnlGroup(u8 chnl)
{
	u8	group=0;

	if (chnl < 3)			
		group = 0;
	else if (chnl < 9)		
		group = 1;
	else					
		group = 2;
	
	return group;
}

void rtl8192ce_ReadBluetoothCoexistInfoFromHWPG(
	struct net_device*	dev,
	bool				AutoLoadFail,
	u8*				hwinfo
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8			tempval;

#if 0
	pHalData->EEPROMBluetoothCoexist = 1;
	pHalData->EEPROMBluetoothType = BT_CSR;
	pHalData->EEPROMBluetoothAntNum = Ant_x2;
	pHalData->EEPROMBluetoothAntIsolation = 1;
	pHalData->EEPROMBluetoothRadioShared = BT_Radio_Shared;
#else
	if(!AutoLoadFail)
	{
		priv->EEPROMBluetoothCoexist = ((hwinfo[RF_OPTION1]&0xe0)>>5);	
		tempval = hwinfo[RF_OPTION4];
		priv->EEPROMBluetoothType = ((tempval&0xe)>>1);				
		priv->EEPROMBluetoothAntNum = (tempval&0x1);					
		priv->EEPROMBluetoothAntIsolation = ((tempval&0x10)>>4);			
		priv->EEPROMBluetoothRadioShared = ((tempval&0x20)>>5);			
	}
	else
	{
		priv->EEPROMBluetoothCoexist = 0;
		priv->EEPROMBluetoothType = BT_2Wire;
		priv->EEPROMBluetoothAntNum = Ant_x2;
		priv->EEPROMBluetoothAntIsolation = 0;
		priv->EEPROMBluetoothRadioShared = BT_Radio_Shared;
	}
#endif

	printk("=========>%s(): EEPROMBluetoothCoexist:%d, EEPROMBluetoothType:%d\n", __func__, priv->EEPROMBluetoothCoexist,priv->EEPROMBluetoothType);

	rtl8192ce_BT_VAR_INIT(dev);
}

void rtl8192ce_ReadTxPowerInfoFromHWPG(
	struct net_device*	dev,
	bool				AutoLoadFail,
	u8*				hwinfo
	)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8				rf_path, index, tempval;
	u16				i;

	for(rf_path=0; rf_path<2; rf_path++)
	{
		for(i=0; i<3; i++)
		{
			if(!AutoLoadFail)
			{
				priv->EEPROMChnlAreaTxPwrCCK[rf_path][i] = hwinfo[EEPROM_TxPowerCCK+rf_path*3+i];	
				priv->EEPROMChnlAreaTxPwrHT40_1S[rf_path][i] = 	hwinfo[EEPROM_TxPowerHT40_1S+rf_path*3+i];
			}
			else
			{
				priv->EEPROMChnlAreaTxPwrCCK[rf_path][i] = EEPROM_Default_TxPowerLevel;
				priv->EEPROMChnlAreaTxPwrHT40_1S[rf_path][i] = EEPROM_Default_TxPowerLevel;
			}
		}
	}
	for(i=0; i<3; i++)
	{
		if(!AutoLoadFail)
			tempval = hwinfo[EEPROM_TxPowerHT40_2SDiff+i];
		else
			tempval = EEPROM_Default_HT40_2SDiff;
		priv->EEPROMChnlAreaTxPwrHT40_2SDiff[RF90_PATH_A][i] = (tempval&0xf);
		priv->EEPROMChnlAreaTxPwrHT40_2SDiff[RF90_PATH_B][i] = ((tempval&0xf0)>>4);
	}
	for(rf_path=0; rf_path<2; rf_path++)
		for (i=0; i<3; i++)
			RTPRINT(FINIT, INIT_EEPROM, ("RF(%d) EEPROM CCK Area(%d) = 0x%x\n", 
				rf_path, i, priv->EEPROMChnlAreaTxPwrCCK[rf_path][i]));
        for(rf_path=0; rf_path<2; rf_path++)
            for (i=0; i<3; i++)
			RTPRINT(FINIT, INIT_EEPROM, ("RF(%d) EEPROM HT40 1S Area(%d) = 0x%x\n", 
				rf_path, i, priv->EEPROMChnlAreaTxPwrHT40_1S[rf_path][i]));
        for(rf_path=0; rf_path<2; rf_path++)
            for (i=0; i<3; i++)
			RTPRINT(FINIT, INIT_EEPROM, ("RF(%d) EEPROM HT40 2S Diff Area(%d) = 0x%x\n", 
				rf_path, i, priv->EEPROMChnlAreaTxPwrHT40_2SDiff[rf_path][i]));
	for(rf_path=0; rf_path<2; rf_path++)
	{
		for(i=0; i<14; i++)	
		{
			index = getChnlGroup((u8)i);

			priv->TxPwrLevelCck[rf_path][i]  = 
			priv->EEPROMChnlAreaTxPwrCCK[rf_path][index];
			priv->TxPwrLevelHT40_1S[rf_path][i]  = 
			priv->EEPROMChnlAreaTxPwrHT40_1S[rf_path][index];

			if((priv->EEPROMChnlAreaTxPwrHT40_1S[rf_path][index] - 
				priv->EEPROMChnlAreaTxPwrHT40_2SDiff[rf_path][index]) > 0)
			{
				priv->TxPwrLevelHT40_2S[rf_path][i]  = 
					priv->EEPROMChnlAreaTxPwrHT40_1S[rf_path][index] - 
					priv->EEPROMChnlAreaTxPwrHT40_2SDiff[rf_path][index];
			}
			else
			{
				priv->TxPwrLevelHT40_2S[rf_path][i]  = 0;
			}
		}

		for(i=0; i<14; i++)
		{
			RTPRINT(FINIT, INIT_TxPower, 
				("RF(%d)-Ch(%d) [CCK / HT40_1S / HT40_2S] = [0x%x / 0x%x / 0x%x]\n", 
				rf_path, i, priv->TxPwrLevelCck[rf_path][i], 
				priv->TxPwrLevelHT40_1S[rf_path][i], 
				priv->TxPwrLevelHT40_2S[rf_path][i]));
		}
	}
	
	for(i=0; i<3; i++)
	{
		if(!AutoLoadFail)
		{
			priv->EEPROMPwrLimitHT40[i] = hwinfo[EEPROM_TxPWRGroup+i];
			priv->EEPROMPwrLimitHT20[i] = hwinfo[EEPROM_TxPWRGroup+3+i];
		}
		else
		{
			priv->EEPROMPwrLimitHT40[i] = 0;
			priv->EEPROMPwrLimitHT20[i] = 0;
		}
	}

	for(rf_path=0; rf_path<2; rf_path++)
	{
		for(i=0; i<14; i++)
		{
			index = getChnlGroup((u8)i);
			
			if(rf_path == RF90_PATH_A)
			{
				priv->PwrGroupHT20[rf_path][i] = (priv->EEPROMPwrLimitHT20[index]&0xf);
				priv->PwrGroupHT40[rf_path][i] = (priv->EEPROMPwrLimitHT40[index]&0xf);
			}
			else if(rf_path == RF90_PATH_B)
			{
				priv->PwrGroupHT20[rf_path][i] = ((priv->EEPROMPwrLimitHT20[index]&0xf0)>>4);
				priv->PwrGroupHT40[rf_path][i] = ((priv->EEPROMPwrLimitHT40[index]&0xf0)>>4);
			}
			RTPRINT(FINIT, INIT_TxPower, ("RF-%d PwrGroupHT20[%d] = 0x%x\n", rf_path, i, priv->PwrGroupHT20[rf_path][i]));
			RTPRINT(FINIT, INIT_TxPower, ("RF-%d PwrGroupHT40[%d] = 0x%x\n", rf_path, i, priv->PwrGroupHT40[rf_path][i]));
		}
	}


	for(i=0; i<14; i++)	
	{
		index = getChnlGroup((u8)i);
		
		if(!AutoLoadFail)
			tempval = hwinfo[EEPROM_TxPowerHT20Diff+index];
		else
			tempval = EEPROM_Default_HT20_Diff;		
		priv->TxPwrHt20Diff[RF90_PATH_A][i] = (tempval&0xF);
		priv->TxPwrHt20Diff[RF90_PATH_B][i] = ((tempval>>4)&0xF);

		if(priv->TxPwrHt20Diff[RF90_PATH_A][i] & BIT3)	
			priv->TxPwrHt20Diff[RF90_PATH_A][i] |= 0xF0;		

		if(priv->TxPwrHt20Diff[RF90_PATH_B][i] & BIT3)	
			priv->TxPwrHt20Diff[RF90_PATH_B][i] |= 0xF0;		

		index = getChnlGroup((u8)i);

		if(!AutoLoadFail)
			tempval = hwinfo[EEPROM_TxPowerOFDMDiff+index];
		else
			tempval = EEPROM_Default_LegacyHTTxPowerDiff;
		priv->TxPwrLegacyHtDiff[RF90_PATH_A][i] = (tempval&0xF);
		priv->TxPwrLegacyHtDiff[RF90_PATH_B][i] = ((tempval>>4)&0xF);
	}
	priv->LegacyHTTxPowerDiff = priv->TxPwrLegacyHtDiff[RF90_PATH_A][7];
	
	for(i=0; i<14; i++)
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Ht20 to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrHt20Diff[RF90_PATH_A][i]));
	for(i=0; i<14; i++)
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Legacy to Ht40 Diff[%d] = 0x%x\n", i, priv->TxPwrLegacyHtDiff[RF90_PATH_A][i]));
	for(i=0; i<14; i++)
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Ht20 to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrHt20Diff[RF90_PATH_B][i]));
	for(i=0; i<14; i++)
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Legacy to HT40 Diff[%d] = 0x%x\n", i, priv->TxPwrLegacyHtDiff[RF90_PATH_B][i]));

	if(!AutoLoadFail)
	{
		priv->EEPROMRegulatory = (hwinfo[RF_OPTION1]&0x7);	
	}
	else
	{
		priv->EEPROMRegulatory = 0;
	}
	RTPRINT(FINIT, INIT_TxPower, ("EEPROMRegulatory = 0x%x\n", priv->EEPROMRegulatory));
	if(!AutoLoadFail)
	{
		priv->EEPROMTSSI[RF90_PATH_A] = hwinfo[EEPROM_TSSI_A];
		priv->EEPROMTSSI[RF90_PATH_B] = hwinfo[EEPROM_TSSI_B];
	}
	else
	{
		priv->EEPROMTSSI[RF90_PATH_A] = EEPROM_Default_TSSI;
		priv->EEPROMTSSI[RF90_PATH_B] = EEPROM_Default_TSSI;
	}
	RTPRINT(FINIT, INIT_TxPower, ("TSSI_A = 0x%x, TSSI_B = 0x%x\n", 
			priv->EEPROMTSSI[RF90_PATH_A], 
			priv->EEPROMTSSI[RF90_PATH_B]));
		
	if(!AutoLoadFail)
		tempval = hwinfo[EEPROM_THERMAL_METER];
	else
		tempval = EEPROM_Default_ThermalMeter;
	priv->EEPROMThermalMeter = (tempval&0x1f);	

	if(priv->EEPROMThermalMeter == 0x1f || AutoLoadFail)
		priv->bAPKThermalMeterIgnore = true;
	
	
	priv->ThermalMeter[0] = priv->EEPROMThermalMeter;	
	RTPRINT(FINIT, INIT_TxPower, ("ThermalMeter = 0x%x\n", priv->EEPROMThermalMeter));	
}

static void rtl8192ce_config_hw_for_load_fail(struct net_device* dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	u16			i;
	u8 			sMacAddr[6] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x43};
	u8*			hwinfo = 0;

	RT_TRACE(COMP_INIT, "====> ConfigAdapterInfo8192CForAutoLoadFail\n");

	
	priv->eeprom_vid = 0;
	priv->eeprom_did = 0;		
	priv->eeprom_ChannelPlan = 0;
	priv->eeprom_CustomerID = 0;

	RT_TRACE(COMP_INIT, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_INIT, "EEPROM DID = 0x%4x\n", priv->eeprom_did);	
	RT_TRACE(COMP_INIT, "EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);
	RT_TRACE(COMP_INIT, "EEPROM ChannelPlan = 0x%4x\n", priv->eeprom_ChannelPlan);
	for(i = 0; i < 6; i++)
		dev->dev_addr[i] = sMacAddr[i];			
	
	rtl8192ce_NicIFSetMacAddress(dev, dev->dev_addr);

	RT_TRACE(COMP_INIT,  
	"ReadAdapterInfo8192SUsb(), Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
	dev->dev_addr[0], dev->dev_addr[1], 
	dev->dev_addr[2], dev->dev_addr[3], 
	dev->dev_addr[4], dev->dev_addr[5]); 	


	rtl8192ce_ReadTxPowerInfoFromHWPG(dev, priv->AutoloadFailFlag, hwinfo);
	
	rtl8192ce_ReadBluetoothCoexistInfoFromHWPG(dev, priv->AutoloadFailFlag, hwinfo);

	priv->eeprom_ChannelPlan = 0;
	priv->eeprom_version = 1;		
	priv->bTXPowerDataReadFromEEPORM = false;

	priv->eeprom_CustomerID = 0;
	RT_TRACE(COMP_INIT, "EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);
			
	priv->EEPROMBoardType = EEPROM_Default_BoardType;	
	RT_TRACE(COMP_INIT, "BoardType = %#x\n", priv->EEPROMBoardType);


	
	RT_TRACE(COMP_INIT, "<==== ConfigAdapterInfo8192SForAutoLoadFail\n");
}

void
rtl8192ce_HalCustomizedBehavior(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	
	switch(priv->CustomerID)
	{
	
		case RT_CID_DEFAULT:
			break;
			
		case RT_CID_TOSHIBA:
			break;

		case RT_CID_CCX:
			break;

		case RT_CID_819x_Lenovo:
			priv->LedStrategy = SW_LED_MODE7;
			RT_TRACE(COMP_INIT,"RT_CID_819x_Lenovo \n");			
			break;

		case RT_CID_819x_HP:
			break;

		case RT_CID_819x_Acer:
			break;

		case RT_CID_WHQL:
			break;
	
		default:
			RT_TRACE(COMP_INIT,"Unkown hardware Type \n");
			break;
	}
	RT_TRACE(COMP_INIT, "HalCustomizedBehavior8192C(): RT Customized ID: 0x%02X\n", priv->CustomerID);

	priv->LedStrategy = SW_LED_MODE7;	

	if(priv->eeprom_smid >= 0x6000 && priv->eeprom_smid < 0x7fff)
		if(priv->RegWirelessMode != WIRELESS_MODE_B)
			priv->RegWirelessMode = WIRELESS_MODE_G;
		
}

static void rtl8192ce_ReadAdapterInfo(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);	
	u16			i,usValue;
	u16			EEPROMId;
	u8			hwinfo[HWSET_MAX_SIZE];

	RT_TRACE(COMP_INIT, "====> ReadAdapterInfo8192CE\n");

	if (priv->epromtype == EEPROM_93C46)
	{	

		RT_TRACE(COMP_INIT, "EEPROM\n");
		for(i = 0; i < HWSET_MAX_SIZE; i += 2)
		{
			usValue = eprom_read(dev, (u16) (i>>1));
			*((u16*)(&hwinfo[i])) = usValue;					
		}	
	}
	else if (priv->epromtype == EEPROM_BOOT_EFUSE)
	{	
		RT_TRACE(COMP_INIT, "EFUSE\n");

		EFUSE_ShadowMapUpdate(dev);

		memcpy((void*) hwinfo, (void*)&priv->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE);		
	}

	RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("MAP \n"), hwinfo, HWSET_MAX_SIZE);	

	EEPROMId = *((u16 *)&hwinfo[0]);
	if( EEPROMId != RTL8190_EEPROM_ID )
	{
		RT_TRACE(COMP_INIT, "EEPROM ID(%#x) is invalid!!\n", EEPROMId); 
		priv->AutoloadFailFlag=true;
	}
	else
	{
		RT_TRACE(COMP_INIT, "Autoload OK\n"); 
		priv->AutoloadFailFlag=false;
	}	

	if (priv->AutoloadFailFlag == true)
	{
		rtl8192ce_config_hw_for_load_fail(dev);
		return;
	}

	   
	priv->eeprom_vid		= *(u16 *)&hwinfo[EEPROM_VID];
	priv->eeprom_did		= *(u16 *)&hwinfo[EEPROM_DID];
	priv->eeprom_svid		= *(u16 *)&hwinfo[EEPROM_SVID];
	priv->eeprom_smid		= *(u16 *)&hwinfo[EEPROM_SMID];

	RT_TRACE(COMP_INIT, "EEPROMId = 0x%4x\n", EEPROMId);
	RT_TRACE(COMP_INIT, "EEPROM VID = 0x%4x\n", priv->eeprom_vid);
	RT_TRACE(COMP_INIT, "EEPROM DID = 0x%4x\n", priv->eeprom_did);
	RT_TRACE(COMP_INIT, "EEPROM SVID = 0x%4x\n", priv->eeprom_svid);
	RT_TRACE(COMP_INIT, "EEPROM SMID = 0x%4x\n", priv->eeprom_smid);


	for(i = 0; i < 6; i += 2)
	{
		usValue = *(u16 *)&hwinfo[EEPROM_MAC_ADDR+i];
		*((u16*)(&dev->dev_addr[i])) = usValue;
	}

	rtl8192ce_NicIFSetMacAddress(dev, dev->dev_addr);

	RT_TRACE(COMP_INIT,  
	"ReadAdapterInfo8192S(), Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
	dev->dev_addr[0], dev->dev_addr[1], 
	dev->dev_addr[2], dev->dev_addr[3], 
	dev->dev_addr[4], dev->dev_addr[5]); 	

	rtl8192ce_ReadTxPowerInfoFromHWPG(dev, priv->AutoloadFailFlag, hwinfo);
	
	rtl8192ce_ReadBluetoothCoexistInfoFromHWPG(dev, priv->AutoloadFailFlag, hwinfo);
	
	priv->eeprom_ChannelPlan = *(u8 *)&hwinfo[EEPROM_ChannelPlan];
	priv->eeprom_version  = *(u16 *)&hwinfo[EEPROM_Version];
	priv->bTXPowerDataReadFromEEPORM = true;
	RT_TRACE(COMP_INIT, "EEPROM ChannelPlan = 0x%4x\n", priv->eeprom_ChannelPlan);
	
#if 0
	tempval = 0;
	if (tempval == 0)	
		priv->rf_type = RF_2T2R;
	else if (tempval == 1)	 
		priv->rf_type = RF_1T2R;
	else if (tempval == 2)	 
		priv->rf_type = RF_1T2R;
	else if (tempval == 3)	 
		priv->rf_type = RF_1T1R;

	priv->rf_chip = RF_6052;
#endif

	priv->eeprom_CustomerID = *(u8 *)&hwinfo[EEPROM_CustomID]; 	
	RT_TRACE(COMP_INIT, "EEPROM Customer ID: 0x%2x\n", priv->eeprom_CustomerID);
	
	if(priv->CustomerID == RT_CID_DEFAULT)
	{
		switch(priv->eeprom_CustomerID)
		{	
			case EEPROM_CID_DEFAULT:
				if(priv->eeprom_did==0x8176)
				{
					if((priv->eeprom_svid== 0x10EC && priv->eeprom_smid== 0x6151) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6152) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6154) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6155) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6177) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6178) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6179) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6180) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7151) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7152) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7154) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7155) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7177) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7178) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7179) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7180) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8151) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8152) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8154) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8155) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8181) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8182) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8184) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8185) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9151) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9152) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9154) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9155) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9181) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9182) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9184) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9185) )
							priv->CustomerID = RT_CID_TOSHIBA;
					else if(priv->eeprom_svid == 0x1025)
							priv->CustomerID = RT_CID_819x_Acer;		
					else if((priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6191) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6192) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6193) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7191) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7192) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7193) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8191) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8192) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8193) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9191) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9192) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9193) )
							priv->CustomerID = RT_CID_819x_SAMSUNG;
					else if((priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8195) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9195) )
							priv->CustomerID = RT_CID_819x_Lenovo;
					else if((priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8197) ||
							(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9196) )
							priv->CustomerID = RT_CID_819x_CLEVO;
					else if((priv->eeprom_svid == 0x1028 && priv->eeprom_smid == 0x8194) ||
							(priv->eeprom_svid == 0x1028 && priv->eeprom_smid == 0x8198) ||
							(priv->eeprom_svid == 0x1028 && priv->eeprom_smid == 0x9197) ||
							(priv->eeprom_svid == 0x1028 && priv->eeprom_smid == 0x9198))
							priv->CustomerID = RT_CID_819x_DELL;
					else
						priv->CustomerID = RT_CID_DEFAULT;
				}
				else if(priv->eeprom_did==0x8178)
				{
					if((priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6181) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6182) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6184) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x6185) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7181) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7182) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7184) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x7185) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8181) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8182) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8184) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x8185) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9181) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9182) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9184) ||
						(priv->eeprom_svid == 0x10EC && priv->eeprom_smid == 0x9185) )
							priv->CustomerID = RT_CID_TOSHIBA;
					else if(priv->eeprom_svid == 0x1025)
						priv->CustomerID = RT_CID_819x_Acer;
					else
						priv->CustomerID = RT_CID_DEFAULT;
				}
				else
				{
					priv->CustomerID = RT_CID_DEFAULT;
				}
				break;
				
			case EEPROM_CID_TOSHIBA:       
				priv->CustomerID = RT_CID_TOSHIBA;
				break;

			case EEPROM_CID_QMI:
				priv->CustomerID = RT_CID_819x_QMI;
				break;
				
			case EEPROM_CID_WHQL:
#ifdef MERGE_TO_DO	
				priv->bInHctTest = true;
				priv->bSupportTurboMode = false;
				priv->bAutoTurboBy8186 = false;
				priv->PowerSaveControl.bInactivePs = false;
				priv->PowerSaveControl.bIPSModeBackup = false;
				priv->PowerSaveControl.bLeisurePs = false;
				priv->keepAliveLevel = 0;	
				priv->bUnloadDriverwhenS3S4 = false;
#endif
				break;
					
			default:
				priv->CustomerID = RT_CID_DEFAULT;
				break;
					
		}
	}

	RT_TRACE(COMP_INIT, "MGNT Customer ID: 0x%2x\n", priv->CustomerID);
	
	priv->AntDivCfg = (hwinfo[RF_OPTION1]&0x18)>>3;
	printk("SWAS: bHwAntDiv = %x\n", priv->AntDivCfg);
	

	
	RT_TRACE(COMP_INIT, "<==== ReadAdapterInfo8192SE\n");
}

static void rtl8192ce_read_eeprom_info(struct net_device* dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8				tmpU1b;	    

	RT_TRACE(COMP_INIT, "====> ReadAdapterInfo8192C\n");

	PHY_RFShadowRefresh(dev);

	priv->card_8192_version =  ReadChipVersion(dev);
	RT_TRACE(COMP_INIT, "VersionID = 0x%4x\n", priv->card_8192_version);
	
	priv->rf_chip = RF_6052;
	
	tmpU1b = read_nic_byte(dev, REG_9346CR);	

	if (tmpU1b & BIT4)
	{
		RT_TRACE(COMP_INIT, "Boot from EEPROM\n");
		priv->epromtype = EEPROM_93C46;
	}
	else 
	{
		RT_TRACE(COMP_INIT, "Boot from EFUSE\n");
		priv->epromtype = EEPROM_BOOT_EFUSE;
	}

	if (tmpU1b & BIT5)
	{
		RT_TRACE(COMP_INIT, "Autoload OK\n"); 
		priv->AutoloadFailFlag=false;		
		rtl8192ce_ReadAdapterInfo(dev);
	}	
	else
	{ 	
		RT_TRACE(COMP_INIT, "AutoLoad Fail reported from CR9346!!\n"); 
		priv->AutoloadFailFlag=true;		
		rtl8192ce_config_hw_for_load_fail(dev);		

		if (priv->epromtype == EEPROM_BOOT_EFUSE)
		{
			EFUSE_ShadowMapUpdate(dev);
		}
	}	

#ifdef MERGE_TO_DO	
	if(priv->bInHctTest)
	{
		priv->PowerSaveControl.bInactivePs = false;
		priv->PowerSaveControl.bIPSModeBackup = false;
		priv->PowerSaveControl.bLeisurePs = false;
		priv->keepAliveLevel = 0;
	}
#endif

	if(priv->CustomerID == RT_CID_DEFAULT)
	{ 
		switch(priv->eeprom_CustomerID)
		{
			case EEPROM_CID_DEFAULT:
				priv->CustomerID = RT_CID_DEFAULT;
				break;
			case EEPROM_CID_TOSHIBA:       
				priv->CustomerID = RT_CID_TOSHIBA;
				break;

			case EEPROM_CID_CCX:
				priv->CustomerID = RT_CID_CCX;
				break;
				
			default:
				break;
		}
	}
	
#ifdef MERGE_TO_DO
	if((priv->RegChannelPlan >= RT_CHANNEL_DOMAIN_MAX) || (priv->eeprom_ChannelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK))
	{
		priv->ChannelPlan = HalMapChannelPlan8192C(dev, (priv->eeprom_ChannelPlan & (~(EEPROM_CHANNEL_PLAN_BY_HW_MASK))));
		priv->bChnlPlanFromHW = (priv->eeprom_ChannelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK) ? true : false; 
	}
	else
	{
		priv->ChannelPlan = (RT_CHANNEL_DOMAIN)priv->RegChannelPlan;
	}

	switch(priv->ChannelPlan)
	{
		case RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN:
		{
			PRT_DOT11D_INFO	pDot11dInfo = GET_DOT11D_INFO(priv->rtllib);
	
			pDot11dInfo->bEnabled = true;
		}
		RT_TRACE(COMP_INIT, "ReadAdapterInfo8187(): Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n");
		break;
	}
#endif

#ifdef ENABLE_DOT11D
	priv->ChannelPlan = COUNTRY_CODE_WORLD_WIDE_13;

	if(priv->ChannelPlan == COUNTRY_CODE_GLOBAL_DOMAIN) {
		GET_DOT11D_INFO(priv->rtllib)->bEnabled = 1;
		RT_TRACE(COMP_INIT, "%s: Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n", __FUNCTION__);
	}
#endif
	
	RT_TRACE(COMP_INIT, "ChannelPlan = %d\n" , priv->ChannelPlan);

	rtl8192ce_HalCustomizedBehavior(dev);	

	RT_TRACE(COMP_INIT, "<==== ReadAdapterInfo8192C\n");

	return ;
}

void rtl8192ce_get_eeprom_size(struct net_device* dev)
{
	u8	size;
	u32	curRCR;
	
	RT_TRACE(COMP_TRACE, "====> GetEEPROMSize\n");
	
	curRCR = read_nic_word(dev, REG_9346CR);
	size = (curRCR & Cmd9346CR_9356SEL) ? 6 : 4; 
#if 0	
	if(size == 8)
		pHalData->EEType = EEPROM_93C56;
	else
		pHalData->EEType = EEPROM_93C46;
	RT_TRACE(COMP_INIT, DBG_LOUD, ("EEPROM type is %s\n",size==8 ? "93C56" : "93C46"));

	RT_TRACE(COMP_TRACE, DBG_TRACE, ("<==== GetEEPROMSize\n"));
#endif

	rtl8192ce_read_eeprom_info(dev);

	return ;

}


void rtl8192ce_HwConfigure(struct net_device *dev)
{

	struct r8192_priv *priv = rtllib_priv(dev);	

	u8	regBwOpMode = 0;
	u32	regRATR = 0, regRRSR = 0;


	switch(priv->rtllib->mode)
	{
	case WIRELESS_MODE_B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK;
		regRRSR = RATE_ALL_CCK;
		break;
	case WIRELESS_MODE_A:
		regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_G:
	case WIRELESS_MODE_G | WIRELESS_MODE_B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_AUTO:
	case WIRELESS_MODE_N_24G:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_N_5G:
		regBwOpMode = BW_OPMODE_5G;
		regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	}

	write_nic_byte(dev, REG_INIRTS_RATE_SEL, 0x8);
	
	write_nic_byte(dev, REG_BWOPMODE, regBwOpMode);
#if 0	
	priv->rtllib->SetHwRegHandler(dev, HW_VAR_RATR_0, (u8*)(&regRATR));
	
	regTmp = read_nic_byte(dev, 0x313);
	regRRSR = ((regTmp) << 24) | (regRRSR & 0x00ffffff);
	write_nic_dword(dev, REG_RRSR, regRRSR);
#endif	

	write_nic_dword(dev, REG_RRSR, regRRSR);

	write_nic_byte(dev,REG_SLOT, 0x09);

	write_nic_byte(dev,REG_AMPDU_MIN_SPACE, 0x0);

	write_nic_word(dev,REG_FWHW_TXQ_CTRL, 0x1F80);

	write_nic_word(dev,REG_RL, 0x0707);

	write_nic_dword(dev, REG_BAR_MODE_CTRL, 0x02012802);

	write_nic_byte(dev,REG_HWSEQ_CTRL, 0x0);

	write_nic_dword(dev, REG_DARFRC, 0x01000000);
	write_nic_dword(dev, REG_DARFRC+4, 0x07060504);
	write_nic_dword(dev, REG_RARFRC, 0x01000000);
	write_nic_dword(dev, REG_RARFRC+4, 0x07060504);

	if(	(priv->bt_coexist.BluetoothCoexist) &&
		(priv->bt_coexist.BT_CoexistType == BT_CSR) )
	{
		write_nic_dword(dev, REG_AGGLEN_LMT, 0x97427431);
		RTPRINT(FBT, BT_TRACE,("BT write 0x%x = 0x97427431\n", REG_AGGLEN_LMT));
	}
	else
	{
		write_nic_dword(dev, REG_AGGLEN_LMT, 0xb972a841);
	}

	write_nic_byte(dev, REG_ATIMWND, 0x2);

#if 0 
	if(IS_NORMAL_CHIP(priv->card_8192_version) )
	{
		write_nic_byte(dev, REG_BCN_MAX_ERR, 0x0a);
	}
	else
#endif
	{
		write_nic_byte(dev, REG_BCN_MAX_ERR, 0xff);
	}
	


	if(IS_NORMAL_CHIP(priv->card_8192_version))
		priv->RegBcnCtrlVal = 0x1f;
	else
		priv->RegBcnCtrlVal = 0x2d;
	write_nic_byte(dev, REG_BCN_CTRL, (u8)priv->RegBcnCtrlVal);

	write_nic_byte(dev, REG_TBTT_PROHIBIT+1,0xff); 
	
	write_nic_byte(dev, REG_TBTT_PROHIBIT+1,0xff); 



	write_nic_byte(dev, REG_PIFS, 0x1C);
	write_nic_byte(dev, REG_AGGR_BREAK_TIME, 0x16);
	
	write_nic_word(dev, REG_NAV_PROT_LEN, 0x0020);

	if(	(priv->bt_coexist.BluetoothCoexist) &&
		(priv->bt_coexist.BT_CoexistType == BT_CSR) )
	{
		write_nic_word(dev, REG_NAV_PROT_LEN, 0x0020);
		RTPRINT(FBT, BT_TRACE,("BT write 0x%x = 0x0020\n", REG_NAV_PROT_LEN));
		write_nic_word(dev, REG_PROT_MODE_CTRL, 0x0402);
		RTPRINT(FBT, BT_TRACE,("BT write 0x%x = 0x0402\n", REG_PROT_MODE_CTRL));
	}	
	else
	{
		write_nic_word(dev, REG_NAV_PROT_LEN, 0x0020);
	}

	if(	(priv->bt_coexist.BluetoothCoexist) &&
		(priv->bt_coexist.BT_CoexistType == BT_CSR) )
	{
		write_nic_dword(dev, REG_FAST_EDCA_CTRL, 0x03086666);
		RTPRINT(FBT, BT_TRACE,("BT write 0x%x = 0x03086666\n", REG_FAST_EDCA_CTRL));
	}
	else
	{
		if(1)
		{
			write_nic_dword(dev, REG_FAST_EDCA_CTRL, 0x086666);
		}
		else
		{
		write_nic_word(dev, REG_FAST_EDCA_CTRL, 0x0);
	}
	}
	
#if (BOARD_TYPE == FPGA_2MAC)
	write_nic_byte(dev, REG_ACKTO, 0x40);

	write_nic_word(dev,REG_SPEC_SIFS, 0x1010);
	write_nic_word(dev,REG_MAC_SPEC_SIFS, 0x1010);

	write_nic_word(dev,REG_SIFS_CTX, 0x1010);	

	write_nic_word(dev,REG_SIFS_TRX, 0x1010);

#else
	write_nic_byte(dev, REG_ACKTO, 0x40);

	write_nic_word(dev,REG_SPEC_SIFS, 0x100a);
	write_nic_word(dev,REG_MAC_SPEC_SIFS, 0x100a);

	write_nic_word(dev,REG_SIFS_CTX, 0x100a);	

	write_nic_word(dev,REG_SIFS_TRX, 0x100a);
#endif

	write_nic_dword(dev, REG_MAR, 0xffffffff);
	write_nic_dword(dev, REG_MAR+4, 0xffffffff);




		
				
#if 0
	Adapter->MgntInfo.MinSpaceCfg = 0x90;	
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_MIN_SPACE, (pu1Byte)(&Adapter->MgntInfo.MinSpaceCfg));	
        switch(pHalData->RF_Type)
	{
		case RF_1T2R:
		case RF_1T1R:
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter: RF_Type%s\n", (pHalData->RF_Type==RF_1T1R? "(1T1R)":"(1T2R)")));
			Adapter->MgntInfo.MinSpaceCfg = (MAX_MSS_DENSITY_1T<<3);						
			break;
		case RF_2T2R:
		case RF_2T2R_GREEN:
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter:RF_Type(2T2R)\n"));
			Adapter->MgntInfo.MinSpaceCfg = (MAX_MSS_DENSITY_2T<<3);			
			break;
	}
	write_nic_byte(dev, AMPDU_MIN_SPACE, priv->MinSpaceCfg);
#endif 
}

void 
rtl8192ce_rf_recovery(struct net_device*dev)
{
	u8 offset = 0x25;
	for(;offset<0x29;offset++)
	{
		PHY_RFShadowCompareFlagSet( dev, (RF90_RADIO_PATH_E)0, offset,true);
		if(PHY_RFShadowCompare(dev, (RF90_RADIO_PATH_E)0, offset))
		{
			PHY_RFShadowRecorverFlagSet(dev, (RF90_RADIO_PATH_E)0, offset, true);
			PHY_RFShadowRecorver( dev, (RF90_RADIO_PATH_E)0, offset);
		}
	}
}

bool
_LLTWrite(
	struct net_device *dev,
	u32		address,
	u32		data
	)
{
	bool	status = true;
	long 		count = 0;
	u32 		value = _LLT_INIT_ADDR(address) | _LLT_INIT_DATA(data) | _LLT_OP(_LLT_WRITE_ACCESS);

	write_nic_dword(dev, REG_LLT_INIT, value);

	do{

		value = read_nic_dword(dev, REG_LLT_INIT	);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			break;
		}

		if(count > POLLING_LLT_THRESHOLD){
			RT_TRACE(COMP_INIT,"Failed to polling write LLT done at address %d!\n", address);
			status = false;
				break;
		}
	}while(++count);

	return status;

}
#define LLT_CONFIG	5
bool
LLT_table_init(struct net_device* dev)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);
	unsigned short i;

	u8	txpktbuf_bndy;
	u8	maxPage;
	bool	status;

#if LLT_CONFIG == 1	
	maxPage = 255;
	txpktbuf_bndy = 252;
#elif LLT_CONFIG == 2 
	maxPage = 127;
	txpktbuf_bndy = 124;
#elif LLT_CONFIG == 3 
	maxPage = 255;
	txpktbuf_bndy = 174;
#elif LLT_CONFIG == 4 
	maxPage = 255;
	txpktbuf_bndy = 246;
#elif LLT_CONFIG == 5 
	maxPage = 255;
	txpktbuf_bndy = 246;
#endif

#if (SUPERMAC_92D_ENABLE == 1)
	if(priv->card_8192 == NIC_8192DE){
		maxPage = 255;
		txpktbuf_bndy = 252;
	}
#endif

#if LLT_CONFIG == 1
	write_nic_byte(dev,REG_RQPN_NPQ, 0x1c);
	write_nic_dword(dev,REG_RQPN, 0x80a71c1c);
#elif LLT_CONFIG == 2
	write_nic_dword(dev,REG_RQPN, 0x845B1010);
#elif LLT_CONFIG == 3
	write_nic_dword(dev,REG_RQPN, 0x84838484);
#elif LLT_CONFIG == 4
	write_nic_dword(dev,REG_RQPN, 0x80bd1c1c);
#elif LLT_CONFIG == 5 

	if(IS_NORMAL_CHIP(priv->card_8192_version))
		write_nic_word(dev, REG_RQPN_NPQ, 0x0000);

	write_nic_dword(dev,REG_RQPN, 0x80b01c29);
#endif

	write_nic_dword(dev,REG_TRXFF_BNDY, (0x27FF0000 |txpktbuf_bndy));

	write_nic_byte(dev,REG_TDECTRL+1, txpktbuf_bndy);

	write_nic_byte(dev,REG_TXPKTBUF_BCNQ_BDNY, txpktbuf_bndy);
	write_nic_byte(dev,REG_TXPKTBUF_MGQ_BDNY, txpktbuf_bndy);

	write_nic_byte(dev,0x45D, txpktbuf_bndy);
	
	write_nic_byte(dev,REG_PBP, 0x11);		

	write_nic_byte(dev,REG_RX_DRVINFO_SZ, 0x4);	

	for(i = 0 ; i < (txpktbuf_bndy - 1) ; i++){
		status = _LLTWrite(dev, i , i + 1);
		if(true != status){
			return status;
		}
	}

	status = _LLTWrite(dev, (txpktbuf_bndy - 1), 0xFF); 
	if(true != status){
		return status;
	}

	for(i = txpktbuf_bndy ; i < maxPage ; i++){
		status = _LLTWrite(dev, i, (i + 1)); 
		if(true != status){
			return status;
		}
	}

	status = _LLTWrite(dev, maxPage, txpktbuf_bndy);
	if(true != status)
	{
		return status;
	}

	return true;
}

#ifdef CONFIG_64BIT_DMA
bool rtl8192ce_PlatformEnableDMA64(struct net_device* dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	bool			bResult = true;
	u8			value;

	pci_read_config_byte(priv->pdev,0x719, &value);

	value |= (BIT5);

	pci_write_config_byte(priv->pdev,0x719, value);
	
	return bResult;
}
#endif

#define MORE_DELAY_VS_WIN

static  bool 	rtl8192ce_InitMAC(struct net_device* dev)
{
	unsigned char		bytetmp;
	unsigned short	  	wordtmp;
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u16				retry = 0;

	RT_TRACE(COMP_INIT, "=======>InitMAC()\n")

#if defined CONFIG_ASPM_OR_D3
	
#endif

	write_nic_byte(dev,REG_RSV_CTRL, 0x00);

	if(	(priv->bt_coexist.BluetoothCoexist) &&
		(priv->bt_coexist.BT_CoexistType == BT_CSR)	)
	{
		u32	value32;
		value32 = read_nic_dword(dev, REG_APS_FSMCO);
		value32 |= (SOP_ABG|SOP_AMB|XOP_BTCK);
		write_nic_dword(dev, REG_APS_FSMCO, value32);
	}
	
	write_nic_byte(dev,REG_SPS0_CTRL, 0x2b);
	
	write_nic_byte(dev,REG_AFE_XTAL_CTRL, 0x0F);
	
	
#if 1 

	if(	(priv->bt_coexist.BluetoothCoexist) &&
		(priv->bt_coexist.BT_CoexistType == BT_CSR)	)
	{
		u32 u4bTmp = read_nic_dword(dev, REG_AFE_XTAL_CTRL);
	
		u4bTmp &= (~0x00024800);
		write_nic_dword(dev, REG_AFE_XTAL_CTRL, u4bTmp);
	}

	bytetmp = read_nic_byte(dev,REG_APS_FSMCO+1) | BIT0;
	udelay(2);
#ifdef MORE_DELAY_VS_WIN
	udelay(200);
#endif
	write_nic_byte(dev,REG_APS_FSMCO+1, bytetmp);
	udelay(2);
#ifdef MORE_DELAY_VS_WIN
	udelay(200);
#endif

	bytetmp = read_nic_byte(dev,REG_APS_FSMCO+1);
	udelay(2);
#ifdef MORE_DELAY_VS_WIN
	mdelay(1);
#endif
	retry = 0;
	printk("################>%s()Reg0xEC:%x:%x\n", __func__,read_nic_dword(dev, 0xEC),bytetmp);
	while((bytetmp & BIT0) && retry < 1000){
		retry++;
		udelay(50);
		bytetmp = read_nic_byte(dev,REG_APS_FSMCO+1);
		printk("################>%s()Reg0xEC:%x:%x\n", __func__,read_nic_dword(dev, 0xEC),bytetmp);
		udelay(50);
	}

	write_nic_word(dev, REG_APS_FSMCO, 0x1012); 

	write_nic_byte(dev, REG_SYS_ISO_CTRL+1, 0x82);
	udelay(2);		

#endif

	if(	(priv->bt_coexist.BluetoothCoexist) &&
		(priv->bt_coexist.BT_CoexistType == BT_CSR)	) 
	{
	bytetmp = read_nic_byte(dev, REG_AFE_XTAL_CTRL+2) & 0xfd;
	write_nic_byte(dev, REG_AFE_XTAL_CTRL+2, bytetmp);
	}

	write_nic_word(dev,REG_CR, 0x2ff);

	

	
	if(LLT_table_init(dev) == false)
	{
		return false;
	}

	write_nic_dword(dev,REG_HISR, 0xffffffff);
	write_nic_byte(dev,REG_HISRE, 0xff);
	


#if (SUPERMAC_92D_ENABLE == 1)
	if(priv->card_8192 == NIC_8192DE){	
		write_nic_byte(dev,0x2c, 0xf4); 
	}
#endif

	write_nic_word(dev,REG_TRXFF_BNDY+2, 0x27ff);

	
	if(IS_NORMAL_CHIP(priv->card_8192_version))
	{
		wordtmp = read_nic_word(dev,REG_TRXDMA_CTRL);
		wordtmp &= 0xf;
		wordtmp |= 0xF771;
		write_nic_word(dev,REG_TRXDMA_CTRL, wordtmp);
	}
	else
	{
		write_nic_word(dev,REG_TRXDMA_CTRL, 0x3501);
	}


	write_nic_byte(dev,REG_FWHW_TXQ_CTRL+1, 0x1F);


	write_nic_dword(dev,REG_RCR, priv->ReceiveConfig);

	write_nic_dword(dev,REG_TCR, priv->TransmitConfig);


	write_nic_byte(dev,0x4d0, 0x0);

#if 1
	write_nic_dword(dev, REG_BCNQ_DESA, ((u64)priv->tx_ring[BEACON_QUEUE].dma) & DMA_BIT_MASK(32));
	
	write_nic_dword(dev, REG_MGQ_DESA, (u64)priv->tx_ring[MGNT_QUEUE].dma & DMA_BIT_MASK(32));  
	write_nic_dword(dev, REG_VOQ_DESA, (u64)priv->tx_ring[VO_QUEUE].dma & DMA_BIT_MASK(32));
	write_nic_dword(dev, REG_VIQ_DESA, (u64)priv->tx_ring[VI_QUEUE].dma & DMA_BIT_MASK(32));
	write_nic_dword(dev, REG_BEQ_DESA, (u64)priv->tx_ring[BE_QUEUE].dma & DMA_BIT_MASK(32));
	write_nic_dword(dev, REG_BKQ_DESA, (u64)priv->tx_ring[BK_QUEUE].dma & DMA_BIT_MASK(32));
	write_nic_dword(dev,REG_HQ_DESA,   (u64)priv->tx_ring[HIGH_QUEUE].dma & DMA_BIT_MASK(32));
	write_nic_dword(dev, REG_RX_DESA,  (u64)priv->rx_ring_dma[RX_MPDU_QUEUE] & DMA_BIT_MASK(32));
#else
	rtl8192_rx_enable(dev);
	rtl8192_tx_enable(dev);
#endif

#ifdef CONFIG_64BIT_DMA
	write_nic_dword(dev, REG_BCNQ_DESA+4, 	((u64)priv->tx_ring[BEACON_QUEUE].dma)>>32);
	write_nic_dword(dev, REG_MGQ_DESA+4, 	((u64)priv->tx_ring[MGNT_QUEUE].dma)>>32);  
	write_nic_dword(dev, REG_VOQ_DESA+4, 	((u64)priv->tx_ring[VO_QUEUE].dma)>>32);
	write_nic_dword(dev, REG_VIQ_DESA+4, 	((u64)priv->tx_ring[VI_QUEUE].dma)>>32);
	write_nic_dword(dev, REG_BEQ_DESA+4, 	((u64)priv->tx_ring[BE_QUEUE].dma)>>32);
	write_nic_dword(dev, REG_BKQ_DESA+4,	((u64)priv->tx_ring[BK_QUEUE].dma)>>32);
	write_nic_dword(dev, REG_HQ_DESA+4,	((u64)priv->tx_ring[HIGH_QUEUE].dma)>>32);
	write_nic_dword(dev, REG_RX_DESA+4, 	((u64)priv->rx_ring_dma[RX_MPDU_QUEUE])>>32);


	if (((u64)priv->rx_ring_dma[RX_MPDU_QUEUE])>>32 != 0)
	{
		RT_TRACE(COMP_INIT, "Enable DMA64 bit\n");
		printk("Enable DMA64 bit\n");

		if (((u64)priv->tx_ring[MGNT_QUEUE].dma)>>32)
		{
			RT_TRACE(COMP_INIT, "MGNT_QUEUE HA=0\n");
		}
		
		rtl8192ce_PlatformEnableDMA64(dev);
	}
	else
	{
		RT_TRACE(COMP_INIT, "Enable DMA32 bit\n");
		printk("Enable DMA32 bit\n");
	}
#endif


	if(IS_92C_SERIAL(priv->card_8192_version))
		write_nic_byte(dev,REG_PCIE_CTRL_REG+3, 0x77);
	else
		write_nic_byte(dev,REG_PCIE_CTRL_REG+3, 0x22);


	write_nic_dword(dev, REG_INT_MIG, 0);	
	priv->bInterruptMigration = false;

	bytetmp = read_nic_byte(dev, REG_APSD_CTRL);
	write_nic_byte(dev, REG_APSD_CTRL, bytetmp & ~BIT6);
	do{
		retry++;
		bytetmp = read_nic_byte(dev, REG_APSD_CTRL);
	}while((retry<200) && (bytetmp&BIT7)); 

	rtl8192ce_gen_RefreshLedState(dev);

	write_nic_dword(dev, REG_MCUTST_1, 0x0);

	
	RT_TRACE(COMP_INIT, "<=======InitMAC()\n")
		
	return true;
	
}


void
EnableAspmBackDoor92CE(struct net_device* dev)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);
	
	
		write_nic_byte(dev, 0x34b, 0x93);
	write_nic_word(dev, 0x350, 0x870c);
	write_nic_byte(dev, 0x352, 0x1);

	if(priv->bSupportBackDoor)
		write_nic_byte(dev, 0x349, 0x1b);
	else
		write_nic_byte(dev, 0x349, 0x03);
	
	write_nic_word(dev, 0x350, 0x2718);
	write_nic_byte(dev, 0x352, 0x1);
}

#define   byte(x,n)  ( (x >> (8 * n)) & 0xff  )
static u8		TmpFwBuf_heap[32000];
bool rtl8192ce_LoadFw(struct net_device* dev)
{
	bool	rtStatus = true;
	struct r8192_priv 		*priv = rtllib_priv(dev);

	char*	R88CFwImageFileName = {"RTL8192CE/rtl8192cfw.bin"};
	char*	R92CFwImageFileName = {"RTL8192CE/rtl8192cfw.bin"};

	char*	FwImageFileName[1];
	u8*	TmpFwBuf = TmpFwBuf_heap;
	
	u32		TmpFwLen = 0;
	u32		CurPtr;
	u32		WriteAddr;
	u32		Temp;
	PRT_8192C_FIRMWARE_HDR		pFwHdr = NULL;

	if(priv->card_8192_version &BIT0)
		FwImageFileName[0] = R92CFwImageFileName;
	else
		FwImageFileName[0] = R88CFwImageFileName;

#ifndef USE_FW_SOURCE_IMG_FILE
	priv->firmware_source = FW_SOURCE_HEADER_FILE;
#else
	priv->firmware_source = FW_SOURCE_IMG_FILE; 
#endif

	RT_TRACE(COMP_INIT, "===> FwLoad()\n");

	do{
		CurPtr = 0;
					
		switch(priv->firmware_source)
		{
		case FW_SOURCE_IMG_FILE:
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) && defined(USE_FW_SOURCE_IMG_FILE)
			{
				const struct firmware 	*fw_entry = NULL;
				int 			rc = 0;

				rc = request_firmware(&fw_entry, FwImageFileName[0],&priv->pdev->dev);
				if(rc < 0 ) {
					RT_TRACE(COMP_ERR, "request firmware fail!\n");
					release_firmware(fw_entry);
					goto DownloadFirmware_Fail;
				}else{
					printk("request firmware success\n");
				} 

				memcpy(TmpFwBuf,fw_entry->data,fw_entry->size);
				TmpFwLen = fw_entry->size;
				release_firmware(fw_entry);
			}
#endif		
			break;
		case FW_SOURCE_HEADER_FILE:
			{
			
				if(ImgArrayLength > FW_8192C_SIZE){
					rtStatus = RT_STATUS_FAILURE;
					RT_TRACE(COMP_INIT, "Firmware size exceed 0x%X. Check it.\n", FW_8192C_SIZE );
					goto DownloadFirmware_Fail;
				}
			
				memcpy(TmpFwBuf, Rtl819XFwImageArray, ImgArrayLength);
				TmpFwLen = ImgArrayLength;
			
				RT_TRACE(COMP_INIT, "Fw download from header.\n");
			}
			break;
		}

		pFwHdr = (PRT_8192C_FIRMWARE_HDR)TmpFwBuf;
		priv->firmware_version =  byte(pFwHdr->Version ,0); 
		priv->FirmwareSubVersion = byte(pFwHdr->Subversion, 0); 
		RT_TRACE(COMP_INIT, " FirmwareVersion(%#x), Signature(%#x)\n", 
			priv->firmware_version, pFwHdr->Signature);

		if(IS_FW_HEADER_EXIST(pFwHdr))
		{
			printk("Shift 32 bytes for FW header!!\n");
			TmpFwBuf = TmpFwBuf + 32;
		}
		
		write_nic_byte(dev, 0x41, 0x40);
		mdelay(1);

		write_nic_byte(dev, 0x03, read_nic_byte(dev, 0x03)|0x04);
		mdelay(1);

		CurPtr = CurPtr + 0x0;

		WriteAddr = 0x1000;
		write_nic_byte(dev, 0x80, read_nic_byte(dev, 0x80)|0x01);
		mdelay(1);
		write_nic_byte(dev, 0x82, read_nic_byte(dev, 0x82)&0xf7);
		mdelay(1);
		while(CurPtr < TmpFwLen)
		{
			if(CurPtr+4 > TmpFwLen)
			{
				while(CurPtr < TmpFwLen)
				{
					Temp = *(TmpFwBuf+CurPtr);
					write_nic_byte(dev, WriteAddr, (u8)Temp);
					WriteAddr++;
					CurPtr++;
				}
			}
			else
			{
				udelay(1);
				Temp = *((u32*)(TmpFwBuf+CurPtr));
				write_nic_dword(dev, WriteAddr, Temp);

				WriteAddr+=4;
				CurPtr+=4;
			}
		}
		write_nic_byte(dev, REG_TCR+3, 0x7f);
		Temp = read_nic_byte(dev, 0x80);
		Temp &= 0xfe;
		Temp |= 0x02;
		write_nic_byte(dev, 0x80, (u8)Temp);
		mdelay(1);
		write_nic_byte(dev, 0x81, 0x00);
		
	}while(0);

	RT_TRACE(COMP_INIT, "<=== FwLoad()\n");
	return rtStatus;

DownloadFirmware_Fail:
	RT_TRACE(COMP_INIT, " <=== FwLoad(): DownloadFirmware_Fail\n");
	return false;
}

bool
rtl8192ce_GetNmodeSupportBySWSec(struct net_device* dev)
{
	
	return false;	
}

bool
rtl8192ce_GetNmodeSupportBySecCfg(struct net_device* dev)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);
	
	return true;

	if(priv->rtllib->pHTInfo->IOTPeer == HT_IOT_PEER_REALTEK_92SE)
		return true;	

	if(priv->rtllib->rtllib_ap_sec_type && 
	   (priv->rtllib->rtllib_ap_sec_type(priv->rtllib) & SEC_ALG_TKIP))
	{
		return false;
	}
	else if(priv->rtllib->rtllib_ap_sec_type && 
	   (priv->rtllib->rtllib_ap_sec_type(priv->rtllib) & SEC_ALG_WEP))
	{
		if(priv->bWEPinNmodeFromReg)
			return true;
		else
		return  false;
	}
	else
	{
		return true;	
}

}

bool rtl8192ce_GetHalfNmodeSupportByAPs(struct net_device* dev)
{
	
	return false;	
}

void
rtl8192ce_EnableHWSecurityConfig(struct net_device* dev)
{
	u8 	SECR_value = 0x0;
	struct r8192_priv 		*priv = rtllib_priv(dev);

	RT_TRACE(COMP_INIT, "EnableHWSecurityConfig8192SE(8190)-------- \n");
	
#ifdef MERGE_TO_DO	
	RT_TRACE(COMP_INIT, "\tPairwiseEncAlgorithm = %d\tGroupEncAlgorithm = %d\n", 
	Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm, Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm);

	/*if(
	  (Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == WEP40_Encryption && Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == WEP40_Encryption) ||
	  (Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == WEP104_Encryption && Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == WEP104_Encryption)
	  )*/
	if(Adapter->MgntInfo.SecurityInfo.TxUseDK)
	{
		SECR_value |= SCR_TxUseDK;
	}
	if(Adapter->MgntInfo.SecurityInfo.RxUseDK)
	{
		SECR_value |= SCR_RxUseDK;
	}
	if(Adapter->MgntInfo.SecurityInfo.UseDefaultKey)
	{
		SECR_value |= SCR_TxUseDK;
		SECR_value |= SCR_RxUseDK;
	}	
	if(Adapter->MgntInfo.SecurityInfo.SKByA2)
	{
		SECR_value |= SCR_SKByA2;
	}
	if(Adapter->MgntInfo.SecurityInfo.NoSKMC)
	{
		SECR_value |= SCR_NoSKMC;
	}
	
	if(Adapter->MgntInfo.SecurityInfo.SWTxEncryptFlag == false)
	  SECR_value |= SCR_TxEncEnable;
	if(Adapter->MgntInfo.SecurityInfo.SWRxDecryptFlag == false)
	  SECR_value |= SCR_RxDecEnable;
#else
	SECR_value = SCR_TxEncEnable | SCR_RxDecEnable;
	if (((KEY_TYPE_WEP40 == priv->rtllib->pairwise_key_type) || (KEY_TYPE_WEP104 == priv->rtllib->pairwise_key_type)) && (priv->rtllib->auth_mode != 2))
	{
		SECR_value |= SCR_RxUseDK;
		SECR_value |= SCR_TxUseDK;
	}
	else if ((priv->rtllib->iw_mode == IW_MODE_ADHOC) && (priv->rtllib->pairwise_key_type & (KEY_TYPE_CCMP | KEY_TYPE_TKIP)))
	{
		SECR_value |= SCR_RxUseDK;
		SECR_value |= SCR_TxUseDK;
	}

	priv->rtllib->hwsec_active = 1;
	if ((priv->rtllib->pHTInfo->IOTAction&HT_IOT_ACT_PURE_N_MODE) || !hwwep)
	{
		priv->rtllib->hwsec_active = 0;
		SECR_value &= ~SCR_RxDecEnable;
	}
#endif

#if 1
	if(rtl8192ce_GetNmodeSupportBySWSec(dev))
	{	
		SECR_value &= ~SCR_RxDecEnable;
		SECR_value &= ~SCR_TxEncEnable;
	}			
#endif	

	if(IS_NORMAL_CHIP( priv->card_8192_version))
		SECR_value |= (SCR_RXBCUSEDK|SCR_TXBCUSEDK);
	
	write_nic_byte(dev, REG_CR+1, 0x02);

	RT_TRACE(COMP_SEC,"The SECR-value %x \n",SECR_value)
	priv->rtllib->SetHwRegHandler(dev, HW_VAR_WPA_CONFIG, &SECR_value);
	
}



bool rtl8192ce_adapter_start(struct net_device* dev)
{ 	
	struct r8192_priv *priv = rtllib_priv(dev);
	bool 		rtStatus 	= true;
	bool		isNormal, is92C;
	u32				i;
	u8				tmpU1b = 0;
	
	priv->being_init_adapter = true;
	RT_TRACE(COMP_INIT, "=======>InitializeAdapter8192CE()\n");

	
#ifdef CONFIG_ASPM_OR_D3
	RT_DISABLE_ASPM(dev);
#endif

	rtl8192_pci_resetdescring(dev);

	 rtStatus = rtl8192ce_InitMAC(dev);
	if(rtStatus != true)
	{
		RT_TRACE(COMP_INIT, "Init MAC failed\n");
		return rtStatus;
	}


#if HAL_FW_ENABLE
	rtStatus = FirmwareDownload92C(dev);
	if(rtStatus != true)
	{
		RT_TRACE(COMP_INIT, "FwLoad failed\n");
		rtStatus = true;
	}
	else
	{
		RT_TRACE(COMP_INIT, "FwLoad SUCCESSFULLY!!!\n");
	}

	priv->LastHMEBoxNum = 0;
#endif
	
#if BOARD_TYPE==FPGA_2MAC
	priv->rf_chip = RF_PSEUDO_11N;
	priv->rf_type = RF_2T2R;	
#elif BOARD_TYPE==FPGA_PHY
	#if FPGA_RF==FPGA_RF_8225
		priv->rf_chip = RF_8225;
		priv->rf_type = RF_2T2R;
	#elif FPGA_RF==FPGA_RF_0222D
		priv->rf_chip = RF_6052;
		priv->RF_Type = RF_2T2R;
	#endif
#endif

#if (HAL_MAC_ENABLE == 1)
	RT_TRACE(COMP_INIT, "MAC Config Start!\n");
	rtStatus = PHY_MACConfig8192C(dev);
	if (rtStatus != true)
	{
		RT_TRACE(COMP_INIT, "MAC Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "MAC Config Finished!\n");
#endif	

#if (HAL_BB_ENABLE == 1)
	RT_TRACE(COMP_INIT, "BB Config Start!\n");
	rtStatus = PHY_BBConfig8192C(dev);
	if (rtStatus!= true)
	{
		RT_TRACE(COMP_INIT, "BB Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "BB Config Finished!\n");
#endif	


	priv->Rf_Mode = RF_OP_By_SW_3wire;
#if (HAL_RF_ENABLE == 1)		
	RT_TRACE(COMP_INIT, "RF Config started!\n");
	rtStatus = PHY_RFConfig8192C(dev);
	if(rtStatus != true)
	{
		RT_TRACE(COMP_INIT, "RF Config failed\n");
		return rtStatus;
	}
	RT_TRACE(COMP_INIT, "RF Config Finished!\n");	

	priv->RfRegChnlVal[0] = PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)0, RF_CHNLBW, bRFRegOffsetMask);
	priv->RfRegChnlVal[1] = PHY_QueryRFReg(dev, (RF90_RADIO_PATH_E)1, RF_CHNLBW, bRFRegOffsetMask);
#endif	


	/*---- Set CCK and OFDM Block "ON"----*/
	PHY_SetBBReg(dev, rFPGA0_RFMOD, bCCKEn, 0x1);
	PHY_SetBBReg(dev, rFPGA0_RFMOD, bOFDMEn, 0x1);
	
#if (MP_DRIVER == 0)
	PHY_SetBBReg(dev, rFPGA0_AnalogParameter2, BIT10, 1);
#endif
	
	rtl8192ce_HwConfigure(dev);

	if(priv->ResetProgress == RESET_TYPE_NORESET)
		rtl8192_SetWirelessMode(dev, priv->rtllib->mode);

	CamResetAllEntry(dev);
	rtl8192ce_EnableHWSecurityConfig(dev);

	/* Write correct tx power index */
	PHY_SetTxPowerLevel8192C(dev, priv->chan);

#if 1
	priv->rtllib->eRFPowerState = eRfOn;

	tmpU1b = read_nic_byte(dev, REG_MAC_PINMUX_CFG)&(~BIT3);
	write_nic_byte(dev, REG_MAC_PINMUX_CFG, tmpU1b);
	tmpU1b = read_nic_byte(dev, REG_GPIO_IO_SEL);
	RTPRINT(FPWR, PWRHW, ("GPIO_IN=%02x\n", tmpU1b));
	priv->rtllib->RfOffReason |= (tmpU1b & BIT3) ? 0 : RF_CHANGE_BY_HW;
	priv->rtllib->RfOffReason |= (priv->RegRfOff) ? RF_CHANGE_BY_SW : 0;
	
	
	if(priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)
	{ 
		RT_TRACE((COMP_INIT|COMP_RF), "InitializeAdapter8190(): Turn off RF for RfOffReason(%d) ----------\n", priv->rtllib->RfOffReason);
		MgntActSet_RF_State(dev, eRfOff, priv->rtllib->RfOffReason,true);
	}
	else
	{
		printk("InitializeAdapter8190(): ????\n");
		priv->rtllib->eRFPowerState = eRfOn;
		priv->rtllib->RfOffReason = 0; 
		
		priv->rtllib->LedControlHandler(dev, LED_CTL_POWER_ON);


	}		
#endif



	rtl8192ce_NicIFSetMacAddress(dev, dev->dev_addr);

	priv->SilentResetRxSoltNum = 4;	
	priv->SilentResetRxSlotIndex = 0;
	for( i=0; i < MAX_SILENT_RESET_RX_SLOT_NUM; i++ )
	{
		priv->SilentResetRxStuckEvent[i] = 0;
	}

#ifdef CONFIG_ASPM_OR_D3
	EnableAspmBackDoor92CE(dev);
#endif
	
	if(0) 
	{
		write_nic_byte(dev, 0x1d, 0x30);
		write_nic_byte(dev, 0x1e, 0x1d);

		write_nic_byte(dev, 0x42, 0x0c);
		write_nic_byte(dev, 0x4c, 0x00);
		write_nic_byte(dev, 0xef, 0x18);
	}

	rtl8192ce_BT_HW_INIT(dev);


#ifdef CONFIG_ASPM_OR_D3
	RT_ENABLE_ASPM(dev);
#endif
	rtl8192_irq_enable(dev);

	priv->being_init_adapter = false;

#if (MP_DRIVER == 1)
#else
	if(priv->rtllib->eRFPowerState == eRfOn)
	{
		PHY_IQCalibrate(dev);
		dm_CheckTXPowerTracking(dev);
		PHY_LCCalibrate(dev);
	}
#endif

#ifdef MERGE_TO_DO
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_WOWLAN , &bSupportRemoteWakeUp);
	if(bSupportRemoteWakeUp) 
	{
		u1Byte	u1bTmp;
		u1Byte	i;
		u4Byte	u4bTmp;
#if 0
		u4bTmp = PlatformEFIORead4Byte(Adapter, REG_PCIE_CTRL_REG);
		u4bTmp &= ~(BIT17);
		 PlatformEFIOWrite4Byte(Adapter, REG_PCIE_CTRL_REG, u4bTmp);
#endif

		u1bTmp = PlatformEFIORead1Byte(Adapter, REG_RXPKT_NUM+2);
		u1bTmp &= ~(BIT2);
		PlatformEFIOWrite1Byte(Adapter, REG_RXPKT_NUM+2, u1bTmp);

		if(pPSC->WoWLANMode == eWakeOnMagicPacketOnly)
		{
			PlatformEFIOWrite1Byte(Adapter, REG_WOW_CTRL, WOW_MAGIC);
		}
		else if (pPSC->WoWLANMode == eWakeOnPatternMatchOnly)
		{
			PlatformEFIOWrite1Byte(Adapter, REG_WOW_CTRL, WOW_WOMEN);
		}
		else if (pPSC->WoWLANMode == eWakeOnBothTypePacket)
		{
			PlatformEFIOWrite1Byte(Adapter, REG_WOW_CTRL, WOW_MAGIC|WOW_WOMEN); 
		}
		
		PlatformClearPciPMEStatus(Adapter);

		if(ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT))
		{
			ResetWoLPara(Adapter);
		}
		else
		{
			if(pPSC->WoWLANMode > eWakeOnMagicPacketOnly)
			{
				for(i=0; i<(MAX_SUPPORT_WOL_PATTERN_NUM-2); i++)
				{
					Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_WF_MASK, (pu1Byte)(&i)); 
					Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_WF_CRC, (pu1Byte)(&i)); 
				}
			}
		}
	}
#endif

	isNormal = IS_NORMAL_CHIP(priv->card_8192_version);
	is92C = IS_92C_SERIAL(priv->card_8192_version);

	tmpU1b = EFUSE_Read1Byte(dev, 0x1FA); 

	if(!(tmpU1b & BIT0))
	{
		PHY_SetRFReg(dev, RF90_PATH_A, 0x15, 0x0F, 0x05);
		RT_TRACE(COMP_INIT, "PA BIAS path A\n");
	}	

	if(!(tmpU1b & BIT1) && isNormal && is92C)
	{
		PHY_SetRFReg(dev, RF90_PATH_B, 0x15, 0x0F, 0x05);
		RT_TRACE(COMP_INIT, "PA BIAS path B\n");	
	}

	if(!(tmpU1b & BIT4))
	{
		tmpU1b = read_nic_byte(dev, 0x16);
		tmpU1b &= 0x0F; 
		write_nic_byte(dev, 0x16, tmpU1b | 0x90);
		RT_TRACE(COMP_INIT, "under 1.5V\n");	
	}
	
	RT_TRACE(COMP_INIT, "<=======InitializeAdapter8192CE()\n");

	return rtStatus;
	
}

void rtl8192ce_gen_RefreshLedState(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PLED_8190 pLed0 = &(priv->SwLed0);	

	if(priv->bfirst_init)
	{
		printk("gen_RefreshLedState first init\n");
		return;
	}
	
	RT_TRACE(COMP_LED, "gen_RefreshLedState:() pMgntInfo->RfOffReason=%x\n", priv->rtllib->RfOffReason);

	if(priv->rtllib->RfOffReason == RF_CHANGE_BY_IPS )
	{
			switch(priv->LedStrategy)
			{
				case SW_LED_MODE7:
					SwLedOn(dev, pLed0);
					break;

				case SW_LED_MODE8:
					priv->rtllib->LedControlHandler(dev, LED_CTL_NO_LINK);
					break;					

				default:	
					SwLedOn(dev, pLed0);
					break;					
			}
	}
		else if(priv->rtllib->RfOffReason == RF_CHANGE_BY_INIT)
		{
			switch(priv->LedStrategy)
			{
				case SW_LED_MODE7:
					SwLedOn(dev, pLed0);
					break;

				default:
					SwLedOn(dev, pLed0);
					break;
					
			}		
	}
	else		
	{
		SwLedOff(dev, pLed0);		
	}

}	


void rtl8192ce_net_update(struct net_device *dev)
{

	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_network *net = &priv->rtllib->current_network;
	u8 	OpMode ;
	u8	RetryLimit = 0x30;
	u16 cap = net->capability;

	if (priv->rtllib->iw_mode == IW_MODE_ADHOC){
		RetryLimit = HAL_RETRY_LIMIT_AP_ADHOC;
	} else {
		RetryLimit = (priv->CustomerID == RT_CID_CCX) ? 7 : HAL_RETRY_LIMIT_INFRA;
	}	

	if (priv->rtllib->iw_mode == IW_MODE_ADHOC){
		if (priv->rtllib->state == RTLLIB_LINKED)
		        OpMode = RT_OP_MODE_IBSS;
		else
			OpMode = RT_OP_MODE_NO_LINK;
	} 
	else if (priv->rtllib->iw_mode == IW_MODE_INFRA) {
		if (priv->rtllib->state == RTLLIB_LINKED)
		        OpMode = RT_OP_MODE_INFRASTRUCTURE;
		else
			OpMode = RT_OP_MODE_NO_LINK;
	}
#ifdef _RTL8192_EXT_PATCH_
	else if (priv->rtllib->iw_mode == IW_MODE_MESH) {
                if (priv->rtllib->only_mesh) {
                        if (priv->rtllib->mesh_state == RTLLIB_MESH_LINKED) {
                                OpMode = RT_OP_MODE_AP;
                        } else {
                                OpMode = RT_OP_MODE_NO_LINK;
                        }
                } else {
                        if (priv->rtllib->mesh_state == RTLLIB_MESH_LINKED) {
                                OpMode = RT_OP_MODE_IBSS;
                        } else {
                                OpMode = RT_OP_MODE_NO_LINK;
	                }
	
			/* when use hardware beacon, must setup the "nettype" as "AP" mode;
			   setup as "Infra" mode, hardware will stop to send beacon  */
                        if (priv->rtllib->state == RTLLIB_LINKED)
#ifdef _ENABLE_SW_BEACON
		                OpMode = RT_OP_MODE_INFRASTRUCTURE;
#else
				OpMode = RT_OP_MODE_AP;	 
#endif
	        }
        }
#endif
	
	priv->rtllib->SetHwRegHandler( dev, HW_VAR_BASIC_RATE, (u8*)(net->rates));
	priv->dot11CurrentPreambleMode = PREAMBLE_AUTO;
	
	priv->rtllib->SetHwRegHandler( dev, HW_VAR_BSSID, net->bssid);

	priv->rtllib->SetHwRegHandler(dev, HW_VAR_MEDIA_STATUS, &OpMode);
	priv->rtllib->SetHwRegHandler(dev, HW_VAR_RETRY_LIMIT, (u8*)(&RetryLimit));
		
	if (priv->rtllib->iw_mode == IW_MODE_ADHOC){
		priv->rtllib->SetHwRegHandler( dev, HW_VAR_BEACON_INTERVAL, (u8*)(&net->beacon_interval));
	}
			
	rtl8192_update_cap(dev, cap);
}

#ifdef _RTL8192_EXT_PATCH_
void rtl8192ce_mesh_net_update(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_network *net = &priv->rtllib->current_mesh_network;
	u8 	OpMode ;
	u8	RetryLimit = 0x7;
	u16 cap = net->capability;

	/* At the peer mesh mode, the peer MP shall recognize the short preamble */
	priv->short_preamble = 1;

	if (priv->rtllib->iw_mode == IW_MODE_MESH) {
		if (priv->rtllib->only_mesh) {
			if (priv->rtllib->mesh_state == RTLLIB_MESH_LINKED) {
				OpMode = RT_OP_MODE_AP;
			} else {
				OpMode = RT_OP_MODE_NO_LINK;
			}
		} else {
			if (priv->rtllib->mesh_state == RTLLIB_MESH_LINKED) {				
				OpMode = RT_OP_MODE_IBSS;
			} else {
				OpMode = RT_OP_MODE_NO_LINK;
			}

			if (priv->rtllib->state == RTLLIB_LINKED)
				OpMode = RT_OP_MODE_INFRASTRUCTURE;
		}
	}

	priv->rtllib->SetHwRegHandler( dev, HW_VAR_BASIC_RATE, (u8*)(net->rates));
	priv->dot11CurrentPreambleMode = PREAMBLE_AUTO;
	

	priv->rtllib->SetHwRegHandler(dev, HW_VAR_MEDIA_STATUS, &OpMode);
	priv->rtllib->SetHwRegHandler(dev, HW_VAR_RETRY_LIMIT, (u8*)(&RetryLimit));
		
	
	priv->rtllib->SetHwRegHandler( dev, HW_VAR_BEACON_INTERVAL, (u8*)(&net->beacon_interval));
			
	rtl8192_update_cap(dev, cap);
}
#endif

u8 rtl8192ce_GetFreeRATRIndex (struct net_device* dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	u8 bitmap = priv->RATRTableBitmap;
	u8 ratr_index = 0;
	for(	;ratr_index<4; ratr_index++)
	{
		if((bitmap & BIT0) == 0)
			{
				priv->RATRTableBitmap |= BIT0<<ratr_index;
				return ratr_index;
			}
		bitmap = bitmap >>1;
	}
	return ratr_index;
}

void rtl8192ce_UpdateHalRATRTable(struct net_device* dev,u8* pMcsRate,struct sta_info* pEntry)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	struct rtllib_device* rtllib = priv->rtllib;
	u32 ratr_value = 0;
	u8 ratr_index = 0;
	u8 bNMode = 0;
	u16 shortGI_rate = 0;
	u32 tmp_ratr_value = 0;
	u8 MimoPs;
	WIRELESS_MODE WirelessMode;
	u8 bCurTxBW40MHz, bCurShortGI40MHz, bCurShortGI20MHz;

	if(rtllib->iw_mode == IW_MODE_ADHOC){	

		if(pEntry == NULL){
			printk("Doesn't have match Entry\n");
			return;
		}	

		if(pEntry->ratr_index != 8)
			ratr_index = pEntry->ratr_index;
		else
		        ratr_index = rtl8192ce_GetFreeRATRIndex(dev);
		
		if(ratr_index == 4){
			RT_TRACE(COMP_RATE, "Ratrtable are full");
			return;
		}
		MimoPs = pEntry->htinfo.MimoPs;

		if(rtllib->mode < pEntry->wireless_mode)
			WirelessMode = rtllib->mode;
		else	
			WirelessMode = pEntry->wireless_mode;

		bCurTxBW40MHz = pEntry->htinfo.bCurTxBW40MHz;
		bCurShortGI40MHz = pEntry->htinfo.bCurShortGI40MHz;
		bCurShortGI20MHz = pEntry->htinfo.bCurShortGI20MHz;		
		pEntry->ratr_index = ratr_index;
	}
	else  
	{
		ratr_index = 0;
		WirelessMode = rtllib->mode;
		MimoPs = rtllib->pHTInfo->PeerMimoPs;
		bCurTxBW40MHz = rtllib->pHTInfo->bCurTxBW40MHz;
		bCurShortGI40MHz = rtllib->pHTInfo->bCurShortGI40MHz;
		bCurShortGI20MHz = rtllib->pHTInfo->bCurShortGI20MHz;					
	}

	rtl8192_config_rate(dev, (u16*)(&ratr_value));
	ratr_value |= (*(u16*)(pMcsRate)) << 12;
	switch (WirelessMode)
	{
		case IEEE_A:
			ratr_value &= 0x00000FF0;
			break;
		case IEEE_B:
			if(ratr_value & 0x0000000c)		
				ratr_value &= 0x0000000d;
			else
				ratr_value &= 0x0000000f;
			break;
		case IEEE_G:
		case IEEE_G|IEEE_B:
			ratr_value &= 0x00000FF5;
			break;
		case IEEE_N_24G:
		case IEEE_N_5G:
			bNMode = 1;
			if (MimoPs == 0) 
				ratr_value &= 0x0007F005;
			else{
				u32 ratr_mask;

				if (priv->rf_type == RF_1T2R ||
				    priv->rf_type  == RF_1T1R || 
				    (rtllib->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_TX_2SS)){
					ratr_mask = 0x000ff005;
				}else{
					ratr_mask = 0x0f0ff005;
					}

				if ((bCurTxBW40MHz) && 
				   !(rtllib->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_TX_40_MHZ))
					ratr_mask |= 0x00000010;
				else
					printk("=====================>%s():%x,%x\n", __func__, bCurTxBW40MHz, rtllib->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_TX_40_MHZ);

				ratr_value &= ratr_mask;
			}
			break;
		default:
			if(priv->rf_type == RF_1T2R)	
			{
				ratr_value &= 0x000ff0ff;
			}
			else
			{
				ratr_value &= 0x0f0ff0ff;
			}
			
			printk("====>%s(), mode is not correct:%x\n", __FUNCTION__,WirelessMode);
			break; 
	}
	
	if( (priv->bt_coexist.BluetoothCoexist) &&
		(priv->bt_coexist.BT_CoexistType == BT_CSR) &&
		(priv->bt_coexist.BT_CUR_State) &&
		(priv->bt_coexist.BT_Ant_isolation) &&
		((priv->bt_coexist.BT_Service==BT_SCO)||
		(priv->bt_coexist.BT_Service==BT_Busy)) )
		ratr_value &= 0x0fffcfc0;
	else
	ratr_value &= 0x0FFFFFFF;	
	
	if (bNMode && ((bCurTxBW40MHz && bCurShortGI40MHz) ||(!bCurTxBW40MHz && bCurShortGI20MHz)))
	{
		ratr_value |= 0x10000000;  
		tmp_ratr_value = (ratr_value>>12);
		for(shortGI_rate=15; shortGI_rate>0; shortGI_rate--)
		{
			if((1<<shortGI_rate) & tmp_ratr_value)
				break;
		}	
		shortGI_rate = (shortGI_rate<<12)|(shortGI_rate<<8)|\
			       (shortGI_rate<<4)|(shortGI_rate);
	}

	write_nic_dword(dev, REG_ARFR0+ratr_index*4, ratr_value);
	
	
	printk("===========%s: %x\n", __FUNCTION__, read_nic_dword(dev, REG_ARFR0));
}

void
rtl8192ce_UpdateBeaconInterruptMask(
	struct net_device*		dev,
	bool				start
	)
{
	struct r8192_priv* 		priv = rtllib_priv(dev);
	u32 AddMSR = 0;
	u32 RemoveMSR = 0;

	if( start )
	{
		AddMSR = RT_IBSS_INT_MASKS;
	}

	if( !start )
	{
		RemoveMSR = RT_IBSS_INT_MASKS;
	}

	priv->rtllib->UpdateInterruptMaskHandler(priv->rtllib->dev, AddMSR, RemoveMSR);
}

void
rtl8192ce_UpdateInterruptMask(
	struct net_device*		dev,
	u32				AddMSR,
	u32				RemoveMSR
	)
{
	struct r8192_priv* 		priv = rtllib_priv(dev);

	if( AddMSR )
	{
		priv->irq_mask[0] |= AddMSR;
	}

	if( RemoveMSR )
	{
		priv->irq_mask[0] &= (~RemoveMSR);
	}

	rtl8192ce_DisableInterrupt(dev);
	rtl8192ce_EnableInterrupt(dev);
}

void
rtl8192ce_UpdateHalRAMask(
	struct net_device*		dev,
	bool					bMulticast,
	u8					macId,
	u8  					MimoPs,
	u8  					WirelessMode,
	u8  		 			bCurTxBW40MHz,
	u8					rssi_level)
{
	struct r8192_priv* 		priv = rtllib_priv(dev);
	struct rtllib_device* 	rtllib = priv->rtllib;
	u32					ratr_bitmap;
	u8					ratr_index = 8;
	u32					value[2];
	bool					bShortGI = false;
	u8* 					pMcsRate = rtllib->dot11HTOperationalRateSet;

	if(rtllib->mode == WIRELESS_MODE_B)
		macId = 1;

	if(macId == 0)
	{
		MimoPs = rtllib->pHTInfo->PeerMimoPs;
		WirelessMode = rtllib->mode;
		bCurTxBW40MHz = rtllib->pHTInfo->bCurTxBW40MHz;
	}	
#ifdef MERGE_TO_DO	
	else if (macId == 1)
	{
		WirelessMode = WIRELESS_MODE_B;
	}
	else
	{
		if(pEntry == NULL)		
		{
			RT_ASSERT(false,("Doesn't have match Entry"));
			return;
		}

		MimoPs = pEntry->htinfo.MimoPs;

		if(rtllib->mode < pEntry->wireless_mode)
			WirelessMode = rtllib->mode;
		else	
			WirelessMode = pEntry->wireless_mode;
		
		bCurTxBW40MHz = pEntry->htinfo.bBw40MHz;
		
	}
#endif

	rtl8192_config_rate(dev, (u16*)(&ratr_bitmap));
	ratr_bitmap |= (*((u16*)(pMcsRate))) << 12;
	
	switch (WirelessMode)
	{
		case WIRELESS_MODE_B:
		{
			ratr_index = RATR_INX_WIRELESS_B;
			if(ratr_bitmap & 0x0000000c)		
			ratr_bitmap &= 0x0000000d;
			else
				ratr_bitmap &= 0x0000000f;
		}
		break;

		case WIRELESS_MODE_G:
		case WIRELESS_MODE_G | WIRELESS_MODE_B:
		{
			ratr_index = RATR_INX_WIRELESS_GB;
			
			if(rssi_level == 1)
				ratr_bitmap &= 0x00000f00;
			else if(rssi_level == 2)
				ratr_bitmap &= 0x00000ff0;
			else
				ratr_bitmap &= 0x00000ff5;
		}
		break;
			
		case WIRELESS_MODE_A:
		{
			ratr_index = RATR_INX_WIRELESS_A;
			ratr_bitmap &= 0x00000ff0;
		}
		break;
			
		case WIRELESS_MODE_N_24G:
		case WIRELESS_MODE_N_5G:
		{
			ratr_index = RATR_INX_WIRELESS_NGB;

			if(MimoPs == MIMO_PS_STATIC)
			{
				if(rssi_level == 1)
					ratr_bitmap &= 0x00070000;
				else if(rssi_level == 2)
					ratr_bitmap &= 0x0007f000;
				else
					ratr_bitmap &= 0x0007f005;
			}
			else
			{
				if (	priv->rf_type == RF_1T2R || priv->rf_type == RF_1T1R	)
				{
					if (bCurTxBW40MHz)
					{
						if(rssi_level == 1)
							ratr_bitmap &= 0x000f0000;
						else if(rssi_level == 2)
							ratr_bitmap &= 0x000ff000;
						else
							ratr_bitmap &= 0x000ff015;
					}
					else
					{
						if(rssi_level == 1)
							ratr_bitmap &= 0x000f0000;
						else if(rssi_level == 2)
							ratr_bitmap &= 0x000ff000;
						else
							ratr_bitmap &= 0x000ff005;
					}	
				}
				else
				{
					if (bCurTxBW40MHz)
					{
						if(rssi_level == 1)
							ratr_bitmap &= 0x0f0f0000;
						else if(rssi_level == 2)
							ratr_bitmap &= 0x0f0ff000;
						else
							ratr_bitmap &= 0x0f0ff015;
					}
					else
					{
						if(rssi_level == 1)
							ratr_bitmap &= 0x0f0f0000;
						else if(rssi_level == 2)
							ratr_bitmap &= 0x0f0ff000;
						else
							ratr_bitmap &= 0x0f0ff005;
					}
				}
			}
			if( (rtllib->pHTInfo->bCurTxBW40MHz && rtllib->pHTInfo->bCurShortGI40MHz) ||
				(!rtllib->pHTInfo->bCurTxBW40MHz && rtllib->pHTInfo->bCurShortGI20MHz)	)
			{
				if(macId == 0)
					bShortGI = true;
				else if(macId == 1)
					bShortGI = false;
#ifdef MERGE_TO_DO
				else
				{
					if(pEntry != NULL)
					{
						if(pHTInfo->bCurTxBW40MHz)
							bShortGI = pEntry->HTInfo.bShortGI40M;								
						else
							bShortGI = pEntry->HTInfo.bShortGI20M;		
					}
					else

					{
						bShortGI = true;							
					}
				}
#endif
			}
		}
		break;

		default:
			ratr_index = RATR_INX_WIRELESS_NGB;
			
			if(priv->rf_type == RF_1T2R)
				ratr_bitmap &= 0x000ff0ff;
			else
				ratr_bitmap &= 0x0f0ff0ff;
			break;
	}


	if(macId == 0)
	{
		if(rtllib->pHTInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)
			ratr_bitmap &= 0xfffffff0; 
		if(rtllib->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_SHORT_GI)
			bShortGI = false;
	}		
#ifdef  MERGE_TO_DO
	else if (pEntry != NULL){
		pEntry->ratr_index = ratr_index;
	}
#endif

	
	value[0] = (ratr_bitmap&0x0fffffff) | (ratr_index<<28);
	value[1] = macId | (bShortGI?0x20:0x00) | 0x80;

	printk("===========%s: Rate_index:%x, %x:%x\n", __FUNCTION__, ratr_index, value[0], value[1]);
	FillH2CCmd92C(dev, H2C_RA_MASK, 5, (u8*)(&value));


}

void rtl8192ce_update_msr(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	LED_CTL_MODE	LedAction = LED_CTL_NO_LINK;
	u8		btMsr = read_nic_byte(dev, MSR);

	btMsr &= 0xfc;

	switch (priv->rtllib->iw_mode) {
	case IW_MODE_INFRA:
		if (priv->rtllib->state == RTLLIB_LINKED){
			printk("##########%s():HW_VAR_MEDIA_STATUS:LINKED WITH AP\n", __func__);
			btMsr |= MSR_INFRA;
			LedAction = LED_CTL_LINK;
		}else{
			printk("###########%s():HW_VAR_MEDIA_STATUS:NOLINK ...\n", __func__);
			btMsr |= MSR_NOLINK;
		}
		break;
	case IW_MODE_ADHOC:
		if (priv->rtllib->state == RTLLIB_LINKED){
			printk("##########%s():HW_VAR_MEDIA_STATUS:LINKED WITH ADHOC\n", __func__);
			btMsr |= MSR_ADHOC;
		}else{
			printk("###########%s():HW_VAR_MEDIA_STATUS:NOLINK ...\n", __func__);
			btMsr |= MSR_NOLINK;
		}
		break;
	case IW_MODE_MASTER:
		if (priv->rtllib->state == RTLLIB_LINKED){
			btMsr |= MSR_AP;
			LedAction = LED_CTL_LINK;
		}else{
			printk("###########%s():HW_VAR_MEDIA_STATUS:NOLINK ...\n", __func__);
			btMsr |= MSR_NOLINK;
		}
		break;
#ifdef _RTL8192_EXT_PATCH_
	case IW_MODE_MESH:
		printk("%s: iw_mode=%d state=%d only_mesh=%d mesh_state=%d\n", __FUNCTION__,priv->rtllib->iw_mode,priv->rtllib->state, priv->rtllib->only_mesh, priv->rtllib->mesh_state); 
		if (priv->rtllib->only_mesh) {
			if (priv->rtllib->mesh_state == RTLLIB_MESH_LINKED) {
				btMsr |= MSR_AP;
				LedAction = LED_CTL_LINK;
			} else {
				btMsr |= MSR_NOLINK;
			}
		} else {
			if (priv->rtllib->mesh_state == RTLLIB_MESH_LINKED) {
				btMsr |= MSR_ADHOC;
			} else {
				btMsr |= MSR_NOLINK;
			}
			if (priv->rtllib->state == RTLLIB_LINKED)
				btMsr |= MSR_INFRA;
		}
		break;
#endif	
				
	default:
		break;
	}


	write_nic_byte(dev, MSR, btMsr);
	if(priv->rtllib->LedControlHandler)
		priv->rtllib->LedControlHandler(dev, LedAction);
}

void rtl8192ce_link_change(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device* rtllib = priv->rtllib;
	u8 OpMode ;
	u8 bFilterOutNonAssociatedBSSID;
	u8 bTypeIbss;

	if (IS_NIC_DOWN(priv))
                return;

	if (rtllib->state == RTLLIB_LINKED) {
		
		if(priv->DM_Type == DM_Type_ByFW) {
			if(rtllib->pHTInfo->IOTAction & HT_IOT_ACT_DISABLE_HIGH_POWER)
				rtllib->SetFwCmdHandler(dev, FW_CMD_HIGH_PWR_DISABLE);
			else
				rtllib->SetFwCmdHandler(dev, FW_CMD_HIGH_PWR_ENABLE);	
		}

		rtl8192ce_net_update(dev);

#if 0
		if( pMgntInfo->pStaQos->CurrentQosMode > QOS_DISABLE )
		{
			QosOnAssocRsp( Adapter, asocpdu );
		}
		else
		{ 
			QosSetLegacyACParam(Adapter);
		}

		if(pMgntInfo->pHTInfo->bCurrentHTSupport)
		{
			HTOnAssocRsp(Adapter, asocpdu);
		}
		else
		{
			PlatformZeroMemory(pMgntInfo->dot11HTOperationalRateSet, 16);
			HTSetConnectBwMode(
							Adapter, 
							HT_CHANNEL_WIDTH_20, 
							HT_EXTCHNL_OFFSET_NO_EXT);
		}
#endif
	
		if(priv->rtllib->bUseRAMask && priv->rtllib->mode != WIRELESS_MODE_B)
		{
			priv->rtllib->UpdateHalRAMaskHandler(
									dev,
									false,
									0,
									priv->rtllib->pHTInfo->PeerMimoPs,
									priv->rtllib->mode,
									priv->rtllib->pHTInfo->bCurTxBW40MHz,
									0);
			priv->rssi_level = 0;
		}else{
			rtl8192ce_UpdateHalRATRTable(dev,rtllib->dot11HTOperationalRateSet,NULL);
		}

		if(rtllib->IntelPromiscuousModeInfo.bPromiscuousOn){
                      	bFilterOutNonAssociatedBSSID = false;
#ifdef _RTL8192_EXT_PATCH_
		}else if(rtllib->iw_mode == IW_MODE_MESH){
			bFilterOutNonAssociatedBSSID = false;
#endif			
		}else{
			bFilterOutNonAssociatedBSSID = true;
		}		
		priv->rtllib->SetHwRegHandler(dev, HW_VAR_CECHK_BSSID, (u8*)(&bFilterOutNonAssociatedBSSID));

		if(priv->rtllib->iw_mode == IW_MODE_INFRA){
			bTypeIbss = false;
			priv->rtllib->SetHwRegHandler(dev, HW_VAR_CORRECT_TSF, (u8*)(&bTypeIbss));
#ifdef _RTL8192_EXT_PATCH_
		} else if (priv->rtllib->iw_mode == IW_MODE_MESH){
			bTypeIbss = true;
#endif
		} else {
			bTypeIbss = true;
			priv->rtllib->SetHwRegHandler(dev, HW_VAR_CORRECT_TSF, (u8*)(&bTypeIbss));
		}		

	}
#ifdef _RTL8192_EXT_PATCH_
	else if ((rtllib->mesh_state == RTLLIB_MESH_LINKED) && rtllib->only_mesh) {
		rtl8192ce_mesh_net_update(dev);
		if(rtllib->bUseRAMask) {
			rtllib->UpdateHalRAMaskHandler(
											dev,
											false,
											0,
											rtllib->pHTInfo->PeerMimoPs,
											rtllib->mode,
											rtllib->pHTInfo->bCurTxBW40MHz,
											0);
			priv->rssi_level = 0;
		} else {
			rtl8192ce_UpdateHalRATRTable(dev, rtllib->dot11HTOperationalRateSet, NULL);
		}

		bFilterOutNonAssociatedBSSID = false;
		rtllib->SetHwRegHandler(dev, HW_VAR_CECHK_BSSID, (u8*)(&bFilterOutNonAssociatedBSSID));
	}	
#endif
	else 
	{		
		OpMode = RT_OP_MODE_NO_LINK;
		priv->rtllib->SetHwRegHandler(dev, HW_VAR_MEDIA_STATUS, &OpMode);

		priv->rtllib->SetHwRegHandler(dev, HW_VAR_DUAL_TSF_RST, 0);
		
		bFilterOutNonAssociatedBSSID = false;
		priv->rtllib->SetHwRegHandler(dev, HW_VAR_CECHK_BSSID, (u8*)(&bFilterOutNonAssociatedBSSID));
	}

}

void rtl8192ce_AllowAllDestAddr(struct net_device* dev,
                        bool bAllowAllDA, bool WriteIntoReg)
{
        struct r8192_priv* priv = rtllib_priv(dev);

        if( bAllowAllDA )
        {
                priv->ReceiveConfig |= RCR_AAP;         
        }
        else
        {
                priv->ReceiveConfig &= ~RCR_AAP;                
        }

        if( WriteIntoReg )
        {
                write_nic_dword( dev, REG_RCR, priv->ReceiveConfig );
        }
}

#define DBG_TRACE 5

void
rtl8192ce_PowerOffAdapter(struct net_device *dev)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	u8	u1bTmp;
	u32	u4bTmp;
	
	RT_TRACE(COMP_INIT, "=======>PowerOffAdapter8192CE()\n");
	




#ifdef CONFIG_ASPM_OR_D3
	RT_ENABLE_ASPM(dev);
#endif

	write_nic_byte(dev, REG_TXPAUSE, 0xFF);
	
	PHY_SetRFReg(dev, RF90_PATH_A, 0x00, bRFRegOffsetMask, 0x00);
	write_nic_byte(dev, REG_RF_CTRL, 0x00);

	write_nic_byte(dev, REG_APSD_CTRL, 0x40);

	write_nic_byte(dev, REG_SYS_FUNC_EN, 0xE2);

	write_nic_byte(dev, REG_SYS_FUNC_EN, 0xE0);

	if(read_nic_byte(dev, REG_MCUFWDL)&BIT7) 
	{
	FirmwareSelfReset(dev);
	}
	
	write_nic_byte(dev, REG_SYS_FUNC_EN+1, 0x51);

	write_nic_byte(dev, REG_MCUFWDL, 0x00);
	

	write_nic_dword(dev, REG_GPIO_PIN_CTRL, 0x00000000);

	u1bTmp = read_nic_byte(dev, REG_GPIO_PIN_CTRL);

	if((priv->bt_coexist.BluetoothCoexist) &&
		(priv->bt_coexist.BT_CoexistType == BT_CSR))
	{ 
		write_nic_dword(dev, REG_GPIO_PIN_CTRL, 0x00F30000| (u1bTmp <<8));
	}
	else
	{ 
		write_nic_dword(dev, REG_GPIO_PIN_CTRL, 0x00FF0000| (u1bTmp <<8));
	}


	write_nic_word(dev, REG_GPIO_IO_SEL, 0x0790);

	write_nic_word(dev, REG_LEDCFG0, 0x8080);	


	write_nic_byte(dev, REG_AFE_PLL_CTRL, 0x80);

	write_nic_byte(dev, REG_SPS0_CTRL, 0x23);

	if(	(priv->bt_coexist.BluetoothCoexist) &&
		(priv->bt_coexist.BT_CoexistType == BT_CSR)	)
	{ 
		u4bTmp = read_nic_dword(dev, REG_AFE_XTAL_CTRL);

		u4bTmp |= 0x03800000;
		write_nic_dword(dev, REG_AFE_XTAL_CTRL, u4bTmp);

		u4bTmp |= 0x00020000;
		write_nic_dword(dev, REG_AFE_XTAL_CTRL, u4bTmp);

		u4bTmp |= 0x00004000;
		write_nic_dword(dev, REG_AFE_XTAL_CTRL, u4bTmp);

		u4bTmp |= 0x00000800;
		write_nic_dword(dev, REG_AFE_XTAL_CTRL, u4bTmp);
	}
	else
	{
		write_nic_byte(dev, REG_AFE_XTAL_CTRL, 0x0e);
	}
	
	write_nic_byte(dev, REG_RSV_CTRL, 0x0e);
	

	write_nic_byte(dev, REG_APS_FSMCO+1, 0x10); 




	

	RT_TRACE(COMP_INIT, "<=======PowerOffAdapter8192CE()\n");
	
}	


void rtl8192ce_halt_adapter(struct net_device *dev, bool bReset)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	PRT_POWER_SAVE_CONTROL	pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
        int 	i;
	u8	u1bTmp = 0;
	u8	OpMode;
	bool	bSupportRemoteWakeUp = false;

	RT_TRACE(COMP_INIT, "==> rtl8192ce_halt_adapter()\n");
	
	if(!IS_92C_SERIAL(priv->card_8192_version) && (priv->AntDivCfg!=0))
	{
		;
	}
	
	priv->rtllib->state = RTLLIB_NOLINK;
	OpMode = RT_OP_MODE_NO_LINK;
	priv->rtllib->SetHwRegHandler(dev, HW_VAR_MEDIA_STATUS, &OpMode);

	if(priv->bDriverIsGoingToUnload  || priv->rtllib->RfOffReason > RF_CHANGE_BY_PS)	
		priv->rtllib->LedControlHandler(dev, LED_CTL_POWER_OFF);

	RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
	
#ifdef MERGE_TO_DO
	priv->rtllib->GetHalDefVarHandler(dev, HAL_DEF_WOWLAN, &bSupportRemoteWakeUp);
#endif

#if 1
	if(!bSupportRemoteWakeUp)
	{
	rtl8192ce_PowerOffAdapter(dev);
	} else {
		bool bSleep = true;

		u1bTmp = read_nic_byte(dev, REG_RXPKT_NUM+2);
		write_nic_byte(dev, REG_RXPKT_NUM+2, u1bTmp|BIT2);	

		priv->rtllib->SetHwRegHandler(dev, HW_VAR_SWITCH_EPHY_WoWLAN, (u8*)&bSleep);

		u1bTmp = read_nic_byte(dev, REG_SPS0_CTRL);
		write_nic_byte(dev, REG_SPS0_CTRL, (u1bTmp|BIT1));

		write_nic_byte(dev, REG_APS_FSMCO+1, 0x0);

#ifdef MERGE_TO_DO
		PlatformClearPciPMEStatus(Adapter);
#endif

		if(IS_NORMAL_CHIP(priv->card_8192_version))
		{
			write_nic_byte(dev, REG_SYS_CLKR, (read_nic_byte(dev, REG_SYS_CLKR)|BIT3));

			write_nic_byte(dev, REG_RSV_CTRL, 0x20);
			write_nic_byte(dev, REG_RSV_CTRL, 0x60);
		}
	}


#else
	RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
	udelay(100);
	
	udelay(20);
	if(!bReset)
	{
		mdelay(200);


		u1bTmp = read_nic_byte(dev, REG_APSD_CTRL);
		write_nic_byte(dev, REG_APSD_CTRL, u1bTmp|BIT6);
		do{
			retry++;
			u1bTmp = read_nic_byte(dev, REG_APSD_CTRL);
		}while((retry<200) && (u1bTmp&BIT7));

		u2bTmp = read_nic_word(dev, REG_SYS_FUNC_EN);
		write_nic_word(dev, REG_SYS_FUNC_EN, u2bTmp & 0x73fc);

		u1bTmp = read_nic_byte(dev, REG_MCUFWDL);
		write_nic_byte(dev, REG_MCUFWDL, u1bTmp&0xfc);

		u1bTmp = read_nic_byte(dev, REG_APS_FSMCO+1);
		write_nic_byte(dev, REG_APS_FSMCO+1, u1bTmp & 0xfe);

#if 0 
		u1bTmp = read_nic_byte(dev, REG_APS_FSMCO+1);
		write_nic_byte(dev, REG_APS_FSMCO+1, u1bTmp | 0x18);
#endif

	}	
#endif

	RT_TRACE(COMP_INIT, "<== HaltAdapter8192CE()\n");
	
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->rtllib->skb_waitQ [i]);
        }
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->rtllib->skb_aggQ [i]);
        }
#ifdef _RTL8192_EXT_PATCH_	
	for(i = 0; i < MAX_QUEUE_SIZE; i++) {
                skb_queue_purge(&priv->rtllib->skb_meshaggQ [i]);
        }
#endif	

	skb_queue_purge(&priv->skb_queue);
	RT_TRACE(COMP_INIT, "<== HaltAdapter8192CE()\n");
	return;
}

void
rtl8192ce_SetBeaconRelatedRegisters(
	struct net_device 	*dev
)
{

	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_network *net = NULL;
	u16 BcnInterval, AtimWindow;
	
	if((priv->rtllib->iw_mode == IW_MODE_ADHOC) || (priv->rtllib->iw_mode == IW_MODE_MASTER))
		net = &priv->rtllib->current_network;
#ifdef _RTL8192_EXT_PATCH_
	else if (priv->rtllib->iw_mode == IW_MODE_MESH) {
		net = &priv->rtllib->current_mesh_network;
	}
#endif
	BcnInterval = net->beacon_interval;
	AtimWindow = 2;	

	 RT_TRACE(COMP_INIT,"============>SetBeaconRelatedRegisters8192CE\n");

	 rtl8192_irq_disable(dev);
	 
	write_nic_word(dev, REG_ATIMWND, AtimWindow);
	
	write_nic_word(dev, REG_BCN_INTERVAL, BcnInterval);



	if(IS_NORMAL_CHIP(priv->card_8192_version))
	{	
		write_nic_word(dev, REG_BCNTCFG, 0x660f);
	}else{
		write_nic_word(dev, REG_BCNTCFG, 0x66ff);
	}


	
	write_nic_byte(dev,REG_RXTSF_OFFSET_CCK, 0x18);	
	write_nic_byte(dev,REG_RXTSF_OFFSET_OFDM, 0x18);

	write_nic_byte(dev,0x606, 0x30);

	
	
	rtl8192_irq_enable(dev);
}

void
rtl8192ce_SetTxAntenna(
	struct net_device  *dev,
	u8          SelectedAntenna
	)
{
	if(IS_HARDWARE_TYPE_8192CE(dev)){
		SetAntennaConfig92C( dev, SelectedAntenna);
	}
}

u8          
rtl8192ce_GetRFType(
	struct net_device  *dev
)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	return (priv->rf_type);
}

void
rtl8192ce_InitializeVariables(struct net_device  *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);

	strcpy(priv->nick, "rtl8192CE");

	rtl8192ce_BT_REG_INIT(dev);

#ifdef _ENABLE_SW_BEACON
	priv->rtllib->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE  |
		IEEE_SOFTMAC_BEACONS;
#else	
	priv->rtllib->softmac_features  = IEEE_SOFTMAC_SCAN | 
		IEEE_SOFTMAC_ASSOCIATE | IEEE_SOFTMAC_PROBERQ | 
		IEEE_SOFTMAC_PROBERS | IEEE_SOFTMAC_TX_QUEUE /* |
		IEEE_SOFTMAC_BEACONS*/;
#endif

	priv->bRPDownloadFinished = true;
	
	priv->rtllib->tx_headroom = 0;

	priv->ShortRetryLimit = 0x30;
	priv->LongRetryLimit = 0x30;
	
	priv->EarlyRxThreshold = 7;
	priv->pwrGroupCnt = 0;

	priv->bIgnoreSilentReset = false;  
	priv->enable_gpio0 = 0;

	priv->TransmitConfig = CFENDFORM | BIT12 | BIT13;

	priv->ReceiveConfig = (\
					RCR_APPFCS	
					| RCR_AMF | RCR_ADF| RCR_APP_MIC| RCR_APP_ICV
					| RCR_AICV | RCR_ACRC32	
					| RCR_AB | RCR_AM			
     					| RCR_APM 					
     					| RCR_APP_PHYST_RXFF		
     					| RCR_HTC_LOC_CTRL
					);

	priv->irq_mask[0] = (u32)(IMR_ROK | IMR_VODOK | IMR_VIDOK | IMR_BEDOK | IMR_BKDOK |		\
	 				IMR_MGNTDOK | IMR_HIGHDOK | 					\
					IMR_BDOK | /*IMR_TIMEOUT0 |*/ IMR_RDU | IMR_RXFOVW	|			\
					0/*IMR_BcnInt | IMR_TBDOK | IMR_TBDER | IMR_TXFOVW | IMR_TBDOK | IMR_TBDER*/);

		
	
#if 0
	priv->irq_mask[1] 	= (u32)\
	( /*IMR_WLANOFF | IMR_OCPINT |*/IMR_CPWM|IMR_C2HCMD /*| IMR_RXERR|IMR_TXERR*/);
#endif

	priv->bDisableTxInt = false;


	priv->MidHighPwrTHR_L1 = 0x3B;
	priv->MidHighPwrTHR_L2 = 0x40;
	priv->PwrDomainProtect = false;

	priv->rtllib->bForcedShowRxRate = true;
}

void rtl8192ce_EnableInterrupt(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	
	priv->irq_enabled = 1;
	
	write_nic_dword(dev, REG_HIMR, priv->irq_mask[0]&0xFFFFFFFF);	
		
}

void rtl8192ce_DisableInterrupt(struct net_device *dev)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);	

	write_nic_dword(dev, REG_HIMR, IMR8190_DISABLED);	
	
	
	priv->irq_enabled = 0;
}

void rtl8192ce_ClearInterrupt(struct net_device *dev)
{
	u32 tmp = 0;

	tmp = read_nic_dword(dev, REG_HISR);	
	write_nic_dword(dev, REG_HISR, tmp);

	tmp = 0;
	tmp = read_nic_dword(dev, REG_HISRE);	
	write_nic_dword(dev, REG_HISRE, tmp);	
}

void rtl8192ce_enable_rx(struct net_device *dev)
{
    struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
    write_nic_dword(dev, REG_RX_DESA,priv->rx_ring_dma[RX_MPDU_QUEUE]);
}

u32 TX_DESC_BASE[] = {REG_BKQ_DESA, REG_BEQ_DESA, REG_VIQ_DESA, 
					  REG_VOQ_DESA, REG_BCNQ_DESA, REG_CMDQ_DESA_NODEF, 
					  REG_MGQ_DESA, REG_HQ_DESA, REG_HDAQ_DESA_NODEF};
void rtl8192ce_enable_tx(struct net_device *dev)
{
    struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
    u32 i;
	
    for (i = 0; i < MAX_TX_QUEUE_COUNT; i++)
		if((i != 8) && (i != 5))
        		write_nic_dword(dev, TX_DESC_BASE[i], priv->tx_ring[i].dma);
}


void rtl8192ce_beacon_disable(struct net_device *dev) 
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u32 reg;

	reg = read_nic_dword(priv->rtllib->dev,REG_HIMR);

	reg &= ~(IMR_BcnInt | IMR_BcnInt | IMR_TBDOK | IMR_TBDER);
	write_nic_dword(priv->rtllib->dev, REG_HIMR, reg);	
}

void rtl8192ce_interrupt_recognized(struct net_device *dev, u32 *p_inta, u32 *p_intb)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	
	*p_inta = read_nic_dword(dev, ISR) & priv->irq_mask[0];
	write_nic_dword(dev,ISR,*p_inta); 

}

bool rtl8192ce_HalTxCheckStuck(struct net_device *dev)
{
	u16		RegTxCounter = read_nic_word(dev, 0x366);
	struct 	r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	bool		bStuck = false;

	
	if(priv->TxCounter==RegTxCounter)
		bStuck = true;

	priv->TxCounter = RegTxCounter;

	return bStuck;
}

bool
rtl8192ce_HalRxCheckStuck(struct net_device *dev)
{
	struct 	r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	u16		RegRxCounter = (u16)(priv->InterruptLog.nIMR_ROK&0xffff);
	bool		bStuck = false;
	u32		SlotIndex = 0, TotalRxStuckCount = 0;
	u8		i;
	

	SlotIndex = (priv->SilentResetRxSlotIndex++)%priv->SilentResetRxSoltNum;
	
	if(priv->RxCounter==RegRxCounter)
	{
		
		priv->SilentResetRxStuckEvent[SlotIndex] = 1;

		for( i = 0; i < priv->SilentResetRxSoltNum ; i++ )
		{
			TotalRxStuckCount += priv->SilentResetRxStuckEvent[i];
		}

		if(TotalRxStuckCount  == priv->SilentResetRxSoltNum){
			bStuck = true;
		}
	}
	else
	{
		priv->SilentResetRxStuckEvent[SlotIndex] = 0;
	}
	
	priv->RxCounter = RegRxCounter;

	return bStuck;
}

static void
dbus_wirelessRadioOff_helper(bool radioOff)
{
	static char *dbus_path = "/usr/bin/dbus-send";
	char *dbus_argv[] = {
		dbus_path,
		"--system",
		"--type=method_call",
		"--dest=org.freedesktop.NetworkManager",
		"/org/freedesktop/NetworkManager",
		"org.freedesktop.DBus.Properties.Set",
		"string:org.freedesktop.NetworkManager",
		"string:WirelessEnabled",
		NULL,
		NULL
	};
	static char *envp[] = {"HOME=/", "TERM=linux", "PATH=/usr/bin:/bin", NULL};
	
	if ( radioOff )
		dbus_argv[8] = "variant:boolean:false";
	else
		dbus_argv[8] = "variant:boolean:true";

	call_usermodehelper(dbus_path, dbus_argv, envp, 1);

	dbus_argv[7] = "string:WirelessHardwareOverride";
	if ( radioOff )
		dbus_argv[8] = "variant:boolean:true";
	else
		dbus_argv[8] = "variant:boolean:false";

	call_usermodehelper(dbus_path, dbus_argv, envp, 1);
}

/*-----------------------------------------------------------------------------
 * Function:	GPIOChangeRFWorkItemCallBack()
 *
 * Overview:	Trun on/off RF according to GPIO1. In register 0x108 bit 1.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/10/2008	MHC		Copy from 818xB code to be template. And we only monitor
 *						0x108 register bit 1 for HW RF-ON/OFF.
 *
 *---------------------------------------------------------------------------*/
void
rtl8192ce_GPIOChangeRFWorkItemCallBack(struct net_device *dev)
{
	struct r8192_priv *priv = rtllib_priv(dev);
	u8					u1Tmp = 0;
	RT_RF_POWER_STATE	eRfPowerStateToSet, CurRfState;
	bool					bActuallySet = false;
	PRT_POWER_SAVE_CONTROL		pPSC = (PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl));
	unsigned long flag = 0;

	if((priv->up_first_time == 1) || (priv->being_init_adapter))
	{
		;
		return;
	}


#ifdef MERGE_TO_DO
	if(priv->bDriverStopped  || RT_IS_FUNC_DISABLED(pAdapter, DF_IO_BIT))
	{
		RT_TRACE(COMP_INIT | COMP_POWER | COMP_RF, "GPIOChangeRFWorkItemCallBack(): Callback function breaks out!!\n");
		return;
	}
#endif

	if(priv->ResetProgress == RESET_TYPE_SILENT)
	{
		RT_TRACE((COMP_INIT | COMP_POWER | COMP_RF),  
			"GPIOChangeRFWorkItemCallBack(): Silent Reseting!!!!!!!\n");
		return;
	}

#ifdef MERGE_TO_DO
	if(priv->bIntoIPS)
	{
		priv->bIntoIPS = false;
		return;
	}
#endif

	if (pPSC->bSwRfProcessing) 
	{
		RT_TRACE(COMP_SCAN, "GPIOChangeRFWorkItemCallBack(): Rf is in switching state.\n");
		return;
	}

#ifdef MERGE_TO_DO
	if(priv->MgntInfo.bSetPnpPwrInProgress || 
		priv->bDriverIsGoingToPnpSetPowerSleep)
	{	
		RT_TRACE(COMP_INIT | COMP_POWER | COMP_RF,  
			"GPIOChangeRFWorkItemCallBack(): Return!!! PNP is set in pregress or driver is going to sleep.\n");
		return;
	}		
#endif

	RT_TRACE(COMP_RF, "GPIOChangeRFWorkItemCallBack() ---------> \n");


/*	PlatformAcquireSpinLock(pAdapter,RT_RF_STATE_SPINLOCK);
	if(pMgntInfo->bPollingCPIOChangeRFInProgress)
	{
		PlatformReleaseSpinLock(pAdapter, RT_RF_STATE_SPINLOCK);
		RT_TRACE(COMP_RF,DBG_LOUD,
			("%s(): PollingGPIOChangeRFInProgress\n",__FUNCTION__));
		return;			
	}
	else
	{
		PlatformReleaseSpinLock(pAdapter, RT_RF_STATE_SPINLOCK);
	}*/

	spin_lock_irqsave(&priv->rf_ps_lock,flag);
	if(priv->RFChangeInProgress)
	{
		spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
		RT_TRACE(COMP_RF, "GPIOChangeRFWorkItemCallBack(): RF Change in progress! \n");
		return;
	}
	else
	{
		priv->RFChangeInProgress = true;
		spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
	}

	priv->rtllib->GetHwRegHandler(dev, HW_VAR_RF_STATE, (u8*)&CurRfState);

#ifdef CONFIG_ASPM_OR_D3
	if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM))
	{
		RT_DISABLE_ASPM(dev);
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
	}
#ifdef MERGE_TO_DO
	else if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3))
	{
		RT_LEAVE_D3(dev, false);
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
	}
#endif
#endif

	write_nic_byte(dev, REG_MAC_PINMUX_CFG, read_nic_byte(dev, REG_MAC_PINMUX_CFG)&~(BIT3));

	u1Tmp = read_nic_byte(dev, REG_GPIO_IO_SEL);

	eRfPowerStateToSet = (u1Tmp & BIT3) ? eRfOn : eRfOff;

	if(priv->bResetInProgress)	
	{
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		priv->RFChangeInProgress = false;
		spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
		return;
	}
	if( (priv->bHwRadioOff == true) && ((eRfPowerStateToSet == eRfOn)&&(priv->sw_radio_on == true)))
	{
		RT_TRACE(COMP_RF, "GPIOChangeRF  - HW Radio ON, RF ON\n");
		printk("GPIOChangeRF  - HW Radio ON, RF ON\n");
		eRfPowerStateToSet = eRfOn;
		bActuallySet = true;
	}
	else if ( (priv->bHwRadioOff == false) && ((eRfPowerStateToSet == eRfOff) || (priv->sw_radio_on == false)))
	{
		RT_TRACE(COMP_RF, "GPIOChangeRF  - HW Radio OFF\n");
		printk("GPIOChangeRF  - HW Radio OFF, RF OFF\n");
		eRfPowerStateToSet = eRfOff;
		bActuallySet = true;
	}

	if(bActuallySet)
	{
		priv->bHwRfOffAction = 1;
#ifdef CONFIG_ASPM_OR_D3
		if(eRfPowerStateToSet == eRfOn)
		{
			if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM))
			{
				RT_DISABLE_ASPM(dev);
				RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
			}
#ifdef MERGE_TO_DO
			else if((pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3) && RT_IN_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3))
			{
				RT_LEAVE_D3(dev, false);
				RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
			}
#endif
		}
#endif
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		priv->RFChangeInProgress = false;
		spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
		MgntActSet_RF_State(dev, eRfPowerStateToSet, RF_CHANGE_BY_HW,true);

		dbus_wirelessRadioOff_helper( priv->bHwRadioOff );

#ifdef CONFIG_ASPM_OR_D3
		if(eRfPowerStateToSet == eRfOff)
		{
			if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM)
			{
				RT_ENABLE_ASPM(dev);
				RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
			}
#ifdef MERGE_TO_DO
			else if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3)
			{
				RT_ENTER_D3(dev, false);
				RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
			}
#endif
		}	
#endif

		rtl8192ce_gen_RefreshLedState(dev);

	}	
	else if(eRfPowerStateToSet == eRfOff || CurRfState == eRfOff)
	{
		if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_HALT_NIC)
		{ 
			PHY_SetRtl8192seRfHalt(dev);
			RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
		}

#ifdef CONFIG_ASPM_OR_D3
		if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_ASPM)
		{
			RT_ENABLE_ASPM(dev);
			RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_ASPM);
		}
#ifdef MERGE_TO_DO
		else if(pPSC->RegRfPsLevel & RT_RF_OFF_LEVL_PCI_D3)
		{
			RT_ENTER_D3(dev, false);
			RT_SET_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_PCI_D3);
		}
#endif
#endif
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		priv->RFChangeInProgress = false;
		spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
	}
	else
	{
		spin_lock_irqsave(&priv->rf_ps_lock,flag);
		priv->RFChangeInProgress = false;
		spin_unlock_irqrestore(&priv->rf_ps_lock,flag);
	}
	RT_TRACE(COMP_RF, "GPIOChangeRFWorkItemCallBack() <--------- \n");
}/* GPIOChangeRFWorkItemCallBack HW_RadioGpioChk92SE */


void
ActUpdateChannelAccessSetting(
	struct net_device 			*dev,
	WIRELESS_MODE			WirelessMode,
	PCHANNEL_ACCESS_SETTING	ChnlAccessSetting
	)
{		
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	WIRELESS_MODE Tmp_WirelessMode = WirelessMode;

	switch( Tmp_WirelessMode )
	{
	case WIRELESS_MODE_A:
		ChnlAccessSetting->SlotTimeTimer = 9;
		ChnlAccessSetting->CWminIndex = 4;
		ChnlAccessSetting->CWmaxIndex = 10;
		break;
	case WIRELESS_MODE_B:
		ChnlAccessSetting->SlotTimeTimer = 20;
		ChnlAccessSetting->CWminIndex = 5;
		ChnlAccessSetting->CWmaxIndex = 10;
		break;
	case WIRELESS_MODE_G:
	case WIRELESS_MODE_G | WIRELESS_MODE_B:
		ChnlAccessSetting->SlotTimeTimer = 20;
		ChnlAccessSetting->CWminIndex = 4;
		ChnlAccessSetting->CWmaxIndex = 10;
		break;
	case WIRELESS_MODE_N_24G:
	case WIRELESS_MODE_N_5G:
		ChnlAccessSetting->SlotTimeTimer = 9;
		ChnlAccessSetting->CWminIndex = 4;
		ChnlAccessSetting->CWmaxIndex = 10;
		break;
	default:
		RT_ASSERT(false, ("ActUpdateChannelAccessSetting(): Wireless mode is not defined!\n"));
		break;
	}

	priv->rtllib->SetHwRegHandler( dev, HW_VAR_SLOT_TIME, (u8*)&ChnlAccessSetting->SlotTimeTimer );	

	{
		u16	SIFS_Timer;

		if(WirelessMode == WIRELESS_MODE_G)
			SIFS_Timer = 0x0a0a;
		else
			SIFS_Timer = 0x1010;

		priv->rtllib->SetHwRegHandler( dev, HW_VAR_SIFS,  (u8*)&SIFS_Timer);
	}		

}

void StopTxBeacon(struct net_device* dev)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);
	u8 			tmp1Byte = 0;
	
	 if(IS_NORMAL_CHIP(priv->card_8192_version))
	 {
		tmp1Byte = read_nic_byte(dev, REG_FWHW_TXQ_CTRL+2);
		write_nic_byte(dev, REG_FWHW_TXQ_CTRL+2, tmp1Byte & (~BIT6));
		write_nic_byte(dev, REG_TBTT_PROHIBIT+1, 0x64);
		tmp1Byte = read_nic_byte(dev, REG_TBTT_PROHIBIT+2);
		tmp1Byte &= ~(BIT0);
		write_nic_byte(dev, REG_TBTT_PROHIBIT+2, tmp1Byte);

	 }
	 else
	 {
		tmp1Byte = read_nic_byte(dev, REG_TXPAUSE);
		write_nic_byte(dev, REG_TXPAUSE, tmp1Byte | BIT6);
	 }
}

void ResumeTxBeacon(struct net_device* dev)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);
	u8 			tmp1Byte = 0;
	
	 if(IS_NORMAL_CHIP(priv->card_8192_version))
	 {
		tmp1Byte = read_nic_byte(dev, REG_FWHW_TXQ_CTRL+2);
		write_nic_byte(dev, REG_FWHW_TXQ_CTRL+2, tmp1Byte | BIT6);
		write_nic_byte(dev, REG_TBTT_PROHIBIT+1, 0xff);
		tmp1Byte = read_nic_byte(dev, REG_TBTT_PROHIBIT+2);
		tmp1Byte |= BIT0;
		write_nic_byte(dev, REG_TBTT_PROHIBIT+2, tmp1Byte);

	 }
	 else
	 {
		tmp1Byte = read_nic_byte(dev, REG_TXPAUSE);
		write_nic_byte(dev, REG_TXPAUSE, tmp1Byte & (~BIT6));
	 }
}

void 
EnableBcnSubFunc(struct net_device* dev)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);

	if(IS_NORMAL_CHIP(priv->card_8192_version))
		SetBcnCtrlReg(dev, 0, BIT1);
	else
		SetBcnCtrlReg(dev, 0, BIT4);
}

void 
DisableBcnSubFunc(struct net_device* dev)
{
	struct r8192_priv 		*priv = rtllib_priv(dev);

	if(IS_NORMAL_CHIP(priv->card_8192_version))
		SetBcnCtrlReg(dev, BIT1, 0);
	else
		SetBcnCtrlReg(dev, BIT4, 0);
}


void rtl8192ce_BT_REG_INIT(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);

	priv->bRegBT_Iso = 2;
	
	priv->bRegBT_Sco = 3;

	priv->bRegBT_Sco = 0;
}

void rtl8192ce_BT_VAR_INIT(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);

	priv->bt_coexist.BluetoothCoexist = priv->EEPROMBluetoothCoexist;
	priv->bt_coexist.BT_Ant_Num = priv->EEPROMBluetoothAntNum;
	priv->bt_coexist.BT_CoexistType = priv->EEPROMBluetoothType;

	if(priv->bRegBT_Iso==2)
		priv->bt_coexist.BT_Ant_isolation = priv->EEPROMBluetoothAntIsolation;
	else
		priv->bt_coexist.BT_Ant_isolation = priv->bRegBT_Iso;
	
	priv->bt_coexist.BT_RadioSharedType = priv->EEPROMBluetoothRadioShared;

	RTPRINT(FBT, BT_TRACE,("BT Coexistance = 0x%x\n", priv->bt_coexist.BluetoothCoexist));
	if(priv->bt_coexist.BluetoothCoexist)
	{
		if(priv->bt_coexist.BT_Ant_Num == Ant_x2)
		{
			RTPRINT(FBT, BT_TRACE,("BlueTooth BT_Ant_Num = Antx2\n"));
		}
		else if(priv->bt_coexist.BT_Ant_Num == Ant_x1)
		{
			RTPRINT(FBT, BT_TRACE,("BlueTooth BT_Ant_Num = Antx1\n"));
		}
		switch(priv->bt_coexist.BT_CoexistType)
		{
			case BT_2Wire:
				RTPRINT(FBT, BT_TRACE,("BlueTooth BT_CoexistType = BT_2Wire\n"));
				break;
			case BT_ISSC_3Wire:
				RTPRINT(FBT, BT_TRACE,("BlueTooth BT_CoexistType = BT_ISSC_3Wire\n"));
				break;
			case BT_Accel:
				RTPRINT(FBT, BT_TRACE,("BlueTooth BT_CoexistType = BT_Accel\n"));
				break;
			case BT_CSR:
				RTPRINT(FBT, BT_TRACE,("BlueTooth BT_CoexistType = BT_CSR\n"));
				break;
			case BT_RTL8756:
				RTPRINT(FBT, BT_TRACE,("BlueTooth BT_CoexistType = BT_RTL8756\n"));
				break;
			default:
				RTPRINT(FBT, BT_TRACE,("BlueTooth BT_CoexistType = Unknown\n"));
				break;
		}
		RTPRINT(FBT, BT_TRACE,("BlueTooth BT_Ant_isolation = %d\n", priv->bt_coexist.BT_Ant_isolation));

		if(priv->bRegBT_Sco==1)
			priv->bt_coexist.BT_Service = BT_OtherAction;
		else if(priv->bRegBT_Sco==2)
			priv->bt_coexist.BT_Service = BT_SCO;
		else if(priv->bRegBT_Sco==4)
			priv->bt_coexist.BT_Service = BT_Busy;
		else if(priv->bRegBT_Sco==5)
			priv->bt_coexist.BT_Service = BT_OtherBusy;		
		else
			priv->bt_coexist.BT_Service = BT_Idle;

		priv->bt_coexist.BtEdcaUL = 0;
		priv->bt_coexist.BtEdcaDL = 0;
		priv->bt_coexist.BtRssiState = 0xff;
		
		priv->rf_type = RF_1T1R;
		RTPRINT(FBT, BT_TRACE, ("BT temp set RF = 1T1R\n"));
		
		priv->rf_type = RF_1T1R;
		RTPRINT(FBT, BT_TRACE, ("BT temp set RF = 1T1R\n"));
			
		RTPRINT(FBT, BT_TRACE,("BT Service = BT_Idle\n"));
		RTPRINT(FBT, BT_TRACE,("BT_RadioSharedType = 0x%x\n", priv->bt_coexist.BT_RadioSharedType));
	}
}

void rtl8192ce_BT_HW_INIT(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8 u1Tmp;

	if(priv->bt_coexist.BluetoothCoexist &&
		priv->bt_coexist.BT_CoexistType == BT_CSR)
	{
		if(priv->bt_coexist.BT_Ant_isolation)
		{
			write_nic_byte(dev, REG_GPIO_MUXCFG, 0xa0);
			RTPRINT(FBT, BT_TRACE,("BT write 0x%x = 0x%x\n", REG_GPIO_MUXCFG, 0xa0));
		}

		u1Tmp = read_nic_byte(dev, 0x4fd) & BIT0;
		u1Tmp = u1Tmp | 
				((priv->bt_coexist.BT_Ant_isolation==1)?0:BIT1) | 
				((priv->bt_coexist.BT_Service==BT_SCO)?0:BIT2);
		write_nic_byte(dev, 0x4fd, u1Tmp);
		RTPRINT(FBT, BT_TRACE,("BT write 0x%x = 0x%x for non-isolation\n", 0x4fd, u1Tmp));
		
		
		write_nic_dword(dev, REG_BT_COEX_TABLE+4, 0xaaaa9aaa);
		RTPRINT(FBT, BT_TRACE,("BT write 0x%x = 0x%x\n", REG_BT_COEX_TABLE+4, 0xaaaa9aaa));
		
		write_nic_dword(dev, REG_BT_COEX_TABLE+8, 0xffbd0040);
		RTPRINT(FBT, BT_TRACE,("BT write 0x%x = 0x%x\n", REG_BT_COEX_TABLE+8, 0xffbd0040));

		write_nic_dword(dev, REG_BT_COEX_TABLE+0xc, 0x40000010);
		RTPRINT(FBT, BT_TRACE,("BT write 0x%x = 0x%x\n", REG_BT_COEX_TABLE+0xc, 0x40000010));

		u1Tmp = read_nic_byte(dev, rOFDM0_TRxPathEnable);
		u1Tmp &= ~(BIT1);
		write_nic_byte(dev, rOFDM0_TRxPathEnable, u1Tmp);
		RTPRINT(FBT, BT_TRACE,("BT write 0xC04 = 0x%x\n", u1Tmp));
			
		u1Tmp = read_nic_byte(dev, rOFDM1_TRxPathEnable);
		u1Tmp &= ~(BIT1);
		write_nic_byte(dev, rOFDM1_TRxPathEnable, u1Tmp);
		RTPRINT(FBT, BT_TRACE,("BT write 0xD04 = 0x%x\n", u1Tmp));
	}
}

u8 rtl8192ce_BT_FW_PRIVE_NOTIFY(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);
	u8 tempval=0;

	if(priv->bt_coexist.BluetoothCoexist)
	{
		if(priv->bt_coexist.BT_CoexistType == BT_CSR)
			tempval = 1;
		if(priv->bt_coexist.BT_Ant_Num == Ant_x2)
			tempval |= (2<<4);
		else if(priv->bt_coexist.BT_Ant_Num == Ant_x1)
			tempval |= (1<<4);
		RTPRINT(FBT, BT_TRACE,("FW Prive : bt_type = 0x%x\n", tempval));
	}
	return tempval;
}

void rtl8192ce_BT_SWITCH(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);

	RTPRINT(FBT, BT_TRACE,("FW Notify => BT : %s\n", (priv->bt_coexist.BT_CUR_State==1)? "ON":"OFF"));

	if(priv->bt_coexist.BluetoothCoexist)
	{
		if(priv->bt_coexist.BT_CoexistType == BT_CSR)
		{
			if(priv->rf_type != RF_1T1R)
			{
				if(priv->bt_coexist.BT_CUR_State == 1)	
				{
#if 0
					u1Tmp = PlatformEFIORead1Byte(Adapter, GPIO_OUT);
					u1Tmp &= ~BIT7;
					u1Tmp |= BIT6;
					PlatformEFIOWrite1Byte(Adapter, GPIO_OUT, u1Tmp);
					RTPRINT(FBT, BT_TRACE, ("BT write 0x%x = 0x%x\n", GPIO_OUT, u1Tmp));
					u1Tmp = PlatformEFIORead1Byte(Adapter, 0xC04);
					u1Tmp &= ~(BIT1);
					PlatformEFIOWrite1Byte(Adapter, 0xC04, u1Tmp);
					RTPRINT(FBT, BT_TRACE, ("BT write 0xC04 = 0x%x\n", u1Tmp));
					
					u1Tmp = PlatformEFIORead1Byte(Adapter, 0xD04);
					u1Tmp &= ~(BIT1);
					PlatformEFIOWrite1Byte(Adapter, 0xD04, u1Tmp);
					RTPRINT(FBT, BT_TRACE, ("BT write 0xD04 = 0x%x\n", u1Tmp));
					PHY_SetBBReg(Adapter, rFPGA0_XB_HSSIParameter2, 0x700000, 1);	
					RTPRINT(FBT, BT_TRACE, ("BT write 0x%x[22:20] = 1 (Ant-B standby mode)\n", rFPGA0_XB_HSSIParameter2));
#endif
				}
				else
				{
#if 0
					u1Tmp = PlatformEFIORead1Byte(Adapter, GPIO_OUT);
					u1Tmp &= ~BIT6;
					u1Tmp |= BIT7;
					PlatformEFIOWrite1Byte(Adapter, GPIO_OUT, u1Tmp);
					RTPRINT(FBT, BT_TRACE, ("BT write 0x%x = 0x%x\n", GPIO_OUT, u1Tmp));
					u1Tmp = PlatformEFIORead1Byte(Adapter, 0xC04);
					u1Tmp |= (BIT1);
					PlatformEFIOWrite1Byte(Adapter, 0xC04, u1Tmp);
					RTPRINT(FBT, BT_TRACE, ("BT write 0xC04 = 0x%x\n", u1Tmp));
					
					u1Tmp = PlatformEFIORead1Byte(Adapter, 0xD04);
					u1Tmp |= (BIT1);
					PlatformEFIOWrite1Byte(Adapter, 0xD04, u1Tmp);
					RTPRINT(FBT, BT_TRACE, ("BT write 0xD04 = 0x%x\n", u1Tmp));
					PHY_SetBBReg(Adapter, rFPGA0_XB_HSSIParameter2, 0x700000, 3);	
					RTPRINT(FBT, BT_TRACE, ("BT write 0x%x[22:20] = 3 (Ant-B Rx mode)\n", rFPGA0_XB_HSSIParameter2));
#endif
				}
			}
		}
	}
}

void rtl8192ce_BT_ServiceChange(struct net_device* dev)
{
	struct r8192_priv 	*priv = rtllib_priv(dev);

	RTPRINT(FBT, BT_TRACE,("FW BT_ServiceChange()\n"));

	if(priv->bt_coexist.BluetoothCoexist)
	{
		if(priv->bt_coexist.BT_CoexistType == BT_CSR)
		{
			switch(priv->bt_coexist.BT_Service)
			{
				case BT_SCO:
					RTPRINT(FBT, BT_TRACE,("BT Service : BT_SCO\n"));
					break;
				case BT_A2DP:
					RTPRINT(FBT, BT_TRACE,("BT Service : BT_A2DP\n"));
					break;
				case BT_HID:
					RTPRINT(FBT, BT_TRACE,("BT Service : BT_HID\n"));
					break;
				case BT_HID_Idle:
					RTPRINT(FBT, BT_TRACE,("BT Service : BT_HID_Idle\n"));
					break;
				case BT_Scan:
					RTPRINT(FBT, BT_TRACE,("BT Service : BT_Scan\n"));
					break;
				case BT_Idle:
					RTPRINT(FBT, BT_TRACE,("BT Service : BT_Idle\n"));
					break;

				case BT_OtherAction:
					RTPRINT(FBT, BT_TRACE,("BT Service : BT_OtherAction\n"));
					break;					
				case BT_OtherBusy:
					RTPRINT(FBT, BT_TRACE,("BT Service : BT_OtherBusy\n"));
					break;
			}
		}
	}
}



