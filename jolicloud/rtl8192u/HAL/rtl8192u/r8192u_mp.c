/* 
   This file contains mp handlers.

   This is part of rtl8180 OpenSource driver.
   Copyright (C) Andrea Merello 2004-2005  <andreamrl@tiscali.it> 
   Released under the terms of GPL (General Public Licence)
   
   Parts of this driver are based on the GPL part 
   of the official realtek driver.
   
   Parts of this driver are based on the rtl8180 driver skeleton 
   from Patric Schenke & Andres Salomon.

   Parts of this driver are based on the Intel Pro Wireless 2100 GPL driver.
   
   We want to tanks the Authors of those projects and the Ndiswrapper 
   project Authors.
*/

#ifdef CONFIG_MP_TOOL

#define R8192U_MP_C

#include "r8192U.h"
#include "r8190_rtl8256.h"

#ifdef RTL8192SU
#include "r8192S_rtl8225.h" 
#include "r8192S_hw.h"
#include "r8192S_phy.h" 
#include "r8192S_phyreg.h"
#include "r8192S_Efuse.h"
#else
#include "r8192U_hw.h"
#include "r819xU_phy.h"
#include "r819xU_phyreg.h"
#endif


int r8192u_mp_ioctl_handle(struct net_device *dev,
						   struct iw_request_info *info, 
						   union iwreq_data *wrqu, char *extra)
{
	struct mp_ioctl_handler *phandler;
	struct mp_ioctl_param	*poidparam;
	u8 bset;
	int ret;
	u16 len;
	u8 *pparmbuf = NULL;	
	struct iw_point *p = &wrqu->data;

	printk("+r8192u_mp_ioctl_hdl\n");
		
	if( (!p->length) || (!p->pointer)){
		ret = -EINVAL;
		goto _r8192u_mp_ioctl_hdl_exit;
	}

	pparmbuf = NULL;
	bset = (u8)(p->flags&0xFFFF);
	len = p->length;
	pparmbuf = (u8*)kmalloc(len, GFP_ATOMIC);
	if (pparmbuf == NULL)
	{
		ret = -ENOMEM;
		goto _r8192u_mp_ioctl_hdl_exit;
	}
		
	if(bset)
	{
		if (copy_from_user(pparmbuf, p->pointer, len)) {
			kfree(pparmbuf);
			ret = -EFAULT;
			goto _r8192u_mp_ioctl_hdl_exit;
		}		
	
	}
	else
	{
		if (copy_from_user(pparmbuf, p->pointer, len)) {
			kfree(pparmbuf);
			ret = -EFAULT;
			goto _r8192u_mp_ioctl_hdl_exit;
		}	
			
	}

	poidparam = (struct mp_ioctl_param *)pparmbuf;	

	printk("r8192u_mp_ioctl_hdl: subcode [%d], len[%d], buffer_len[%d]\n", poidparam->subcode, poidparam->len, len);

	if(poidparam->subcode >= MAX_MP_IOCTL_SUBCODE)
	{
		DMESGW("no matching drvext subcodes\r\n");
		ret = -EINVAL;
		goto _r8192u_mp_ioctl_hdl_exit;
	}


	phandler = mp_ioctl_hdl + poidparam->subcode;

	if((phandler->paramsize!=0)&&(poidparam->len != phandler->paramsize))
	{
		printk("no matching drvext param size %d vs %d\r\n",poidparam->len , phandler->paramsize);
		ret = -EINVAL;		
		goto _r8192u_mp_ioctl_hdl_exit;
	}
	
	ret = phandler->handler(dev, bset, poidparam->data);
	
	if(bset==0x00)
	{
		if(copy_to_user(p->pointer, pparmbuf, len))
		{
			ret = -EFAULT;
		}		
	}


_r8192u_mp_ioctl_hdl_exit:
	
	if(pparmbuf)
		kfree(pparmbuf);
	
	return ret; 

}

/*-----------------------------------------------------------------------------
 * Function:	MPT_ProSetCrystalCap()
 *
 * Overview:		Set CrystalCap
 *				OID_RT_PRO_SET_CRYSTALCAP
 *
 * Input:			PVOID	Context
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	08/22/2007	Cosa	Create Version 0.
 *
 *---------------------------------------------------------------------------*/
int mp_ioctl_set_crystalcap_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct r8192_priv	*priv = ieee80211_priv(dev);

	rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, bXtalCap, priv->EEPROMCrystalCap);

	return TRUE;
}


int mp_ioctl_start_hdl(struct net_device *dev, unsigned char set, void*param)
{
	int ret = 0;
	int bstart;
	struct r8192_priv	*priv = ieee80211_priv(dev);
	struct mp_priv		*pmp_priv = &priv->mppriv;


	struct bandwidth_param a_bandwidth_param;
	struct rfchannel_param a_rfchannel_param;
	struct datarate_param a_datarate_param;
	struct txpower_param a_txpower_param;
	struct txagc_offset_param a_txagc_offset_param;
	struct antenna_param a_antenna_param;

	printk("+mp_ioctl_start_hdl\n");
	pmp_priv->bmp_mode = TRUE;

	if(param==NULL)
		return -1;
	
	bstart = *(int*)param;
	if(set)
	{
		if(bstart == MP_START_MODE)
		{
			u8 Index, btMsr = 0;
			u32 ReceiveConfig = 0;
			union iwreq_data wrqu;
			struct ieee80211_device *ieee = priv->ieee80211;

		
			pmp_priv->test_start = 1;


			ieee->iw_mode = IW_MODE_ADHOC;
			for(Index=0; Index<6; Index++)
			{
				ieee->current_network.bssid[Index] = Index;
			}

			strcpy(ieee->current_network.ssid,MP_DEFAULT_TX_ESSID);
			ieee->current_network.ssid_len = strlen(MP_DEFAULT_TX_ESSID);
			ieee->ssid_set = 1;
			
			ieee->state = IEEE80211_LINKED;
			wrqu.ap_addr.sa_family = ARPHRD_ETHER;
			memcpy(wrqu.ap_addr.sa_data, ieee->current_network.bssid, ETH_ALEN);
			wireless_send_event(ieee->dev, SIOCGIWAP, &wrqu, NULL);

			printk("Mark interface on.\n");
			netif_carrier_on(dev);

			ieee->fts = 3000;
			priv->rts = 3000;

			btMsr = MSR_LINK_ENEDCA;
			btMsr |= MSR_LINK_NONE << MSR_LINK_SHIFT;
			write_nic_byte(dev, MSR, btMsr);

			ReceiveConfig = 0xB014F70F;
#if 0
			ReceiveConfig = RCR_ONLYERLPKT | RCR_ENCS1 |  
						RCR_AMF | RCR_ADF |  RCR_ACF | 
						RCR_RXFTH | 
					 	RCR_AICV | RCR_ACRC32 |
						RCR_MXDMA | 
						RCR_AB | RCR_AM | RCR_APM | RCR_AAP;
#endif

			ReceiveConfig |= RCR_ACRC32;



			priv->stats.rxok = 0;
			priv->stats.rxstaterr= 0;
			priv->stats.rxurberr= 0;

			priv->stats.txbkokint = 0;
			priv->stats.txbkerr = 0;
			priv->stats.txbkdrop = 0;
			priv->stats.txbeokint = 0;
			priv->stats.txbeerr = 0;
			priv->stats.txbedrop = 0;
			priv->stats.txviokint = 0;
			priv->stats.txvierr = 0;
			priv->stats.txvidrop = 0;
			priv->stats.txvookint = 0;
			priv->stats.txvoerr = 0;
			priv->stats.txvodrop = 0;
			priv->stats.txbeaconokint = 0;
			priv->stats.txbeaconerr = 0;
			priv->stats.txbeacondrop = 0;
			priv->stats.txmanageokint = 0;
			priv->stats.txmanageerr = 0;
			priv->stats.txmanagedrop = 0;
			
			pmp_priv->bStartContTx = FALSE;
			pmp_priv->bSingleTone = FALSE;
			pmp_priv->current_preamble = Long_GI;
			pmp_priv->antenna_tx_path = RF90_PATH_A;

			pmp_priv->bPacketRx = FALSE;

			


			
			write_nic_dword(dev, RCR, (read_nic_dword(dev,RCR)&0xfffffff0));


			pmp_priv->bandwidth = BAND_20MHZ_MODE;
			a_bandwidth_param.bandwidth = pmp_priv->bandwidth;
			mp_ioctl_set_bandwidth_hdl(dev,set,&a_bandwidth_param);

			pmp_priv->current_modulation = WIRELESS_MODE_N_24G;
	
			pmp_priv->current_channel = 1;
			a_rfchannel_param.ch = pmp_priv->current_channel;
			a_rfchannel_param.modem = pmp_priv->current_modulation;
			mp_ioctl_set_ch_modulation_hdl(dev,set,&a_rfchannel_param);

			pmp_priv->rate_index = MPT_RATE_MCS0;
			a_datarate_param.rate_index = pmp_priv->rate_index;
			mp_ioctl_set_datarate_hdl(dev,set,&a_datarate_param);

			if (pmp_priv->current_channel <= MPT_RATE_11M)	
				a_txpower_param.pwr_index = priv->TxPowerLevelCCK[pmp_priv->current_channel-1];
			else if ((pmp_priv->current_modulation|0x16)!=0)/* B/G/2.4G N */
				a_txpower_param.pwr_index = priv->TxPowerLevelOFDM24G[pmp_priv->current_channel-1];
			else if ((pmp_priv->current_modulation|0x21)!=0)/* A/5G N */
				a_txpower_param.pwr_index = priv->TxPowerLevelOFDM5G[pmp_priv->current_channel-1];

			mp_ioctl_set_txpower_hdl(dev,set,&a_txpower_param);

			a_txagc_offset_param.txagc_offset = 0x0;
			mp_ioctl_set_txagc_offset_hdl(dev,set,&a_txagc_offset_param);

			mp_ioctl_set_crystalcap_hdl(dev,set, 0);


			a_antenna_param.antenna_path = (ANTENNA_A<<16|ANTENNA_AB);
			mp_ioctl_set_antenna_bb_hdl(dev,set,&a_antenna_param);

		}
	}
	else
	{
		
	}
	
	return ret;

}

void mp_dump8256CCKTxPower(struct net_device *dev)
{
	u32	TxAGC=0;
	TxAGC = rtl8192_QueryBBReg(dev, rTxAGC_CCK_Mcs32, bTxAGCRateCCK);

	printk("rtl8192_QueryBBReg rTxAGC_CCK_Mcs32 bTxAGCRateCCK %X\n", TxAGC);
}

void mp_dump8256OFDMTxPower(struct net_device *dev)
{
	u32				TxAGC=0;

	TxAGC= rtl8192_QueryBBReg(dev, rTxAGC_Rate18_06, bTxAGCRate18_06);
	printk("rtl8192_QueryBBReg rTxAGC_Rate18_06 bTxAGCRate18_06 %X\n", TxAGC);

	TxAGC= rtl8192_QueryBBReg(dev, rTxAGC_Rate54_24, bTxAGCRate54_24);
	printk("rtl8192_QueryBBReg rTxAGC_Rate54_24 bTxAGCRate54_24 %X\n", TxAGC);

	TxAGC= rtl8192_QueryBBReg(dev, rTxAGC_Mcs03_Mcs00, bTxAGCRateMCS3_MCS0);
	printk("rtl8192_QueryBBReg rTxAGC_Mcs03_Mcs00 bTxAGCRateMCS3_MCS0 %X\n", TxAGC);

	TxAGC= rtl8192_QueryBBReg(dev, rTxAGC_Mcs07_Mcs04, bTxAGCRateMCS7_MCS4);
	printk("rtl8192_QueryBBReg rTxAGC_Mcs07_Mcs04 bTxAGCRateMCS7_MCS4 %X\n", TxAGC);

	TxAGC= rtl8192_QueryBBReg(dev, rTxAGC_Mcs11_Mcs08, bTxAGCRateMCS11_MCS8);
	printk("rtl8192_QueryBBReg rTxAGC_Mcs11_Mcs08 bTxAGCRateMCS11_MCS8 %X\n", TxAGC);

	TxAGC= rtl8192_QueryBBReg(dev, rTxAGC_Mcs15_Mcs12, bTxAGCRateMCS15_MCS12);
	printk("rtl8192_QueryBBReg rTxAGC_Mcs15_Mcs12 bTxAGCRateMCS15_MCS12 %X\n", TxAGC);
}


void mp_ioctl_dump_txpower_config(struct net_device *dev)
{
	mp_dump8256CCKTxPower(dev);
	mp_dump8256OFDMTxPower(dev);
}

#if 0
int mp_ioctl_dump_ch_modulation_config(struct net_device *dev)
{
	RF90_RADIO_PATH_E eRFPath;
	u32 val_u32;

	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
	
		val_u32= rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x14, bMask12Bits);
		printk("rtl8192_phy_QueryRFReg");
	}
		
	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		u32 val_u32;
		rtl8192_phy_QueryRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x2c, bMask12Bits);

	}

	
	priv->ieee80211->SetWirelessMode(dev, prfchannel->modem);
	priv->rf_set_chan(dev, prfchannel->ch);
	priv->ieee80211->current_network.channel = prfchannel->ch;


		pmp_priv->current_channel=prfchannel->ch;
		dm_cck_txpower_adjust(dev,(pmp_priv->current_channel == 14));

/* modulation mode */
		pmp_priv->current_modulation = prfchannel->modem;

		switch( pmp_priv->current_modulation )
		{
			case WIRELESS_MODE_A:
				CWminIndex = 4;
				CWmaxIndex = 10;
				break;
			case WIRELESS_MODE_B:
				CWminIndex = 5;
				CWmaxIndex = 10;
				break;
			case WIRELESS_MODE_G:
				CWminIndex = 4;
				CWmaxIndex = 10;
				break;
			case WIRELESS_MODE_N_24G:
			case WIRELESS_MODE_N_5G:
				SlotTimeTimer = 20;
				CWminIndex = 4;
				CWmaxIndex = 10;
				break;
			default:
				break;
		}

		{
			u8	u1bAIFS = GET_ASIFSTIME(pmp_priv->current_modulation) + (2 * SlotTimeTimer);

			write_nic_byte(dev, SLOT_TIME, SlotTimeTimer);			
			write_nic_byte(dev, EDCAPARA_VO, u1bAIFS);
			write_nic_byte(dev, EDCAPARA_VI, u1bAIFS);
			write_nic_byte(dev, EDCAPARA_BE, u1bAIFS);
			write_nic_byte(dev, EDCAPARA_BK, u1bAIFS);
		}

/*QosSetLegacyACParam*/
		{

			u8		u1bAIFS;
			u32		u4bAcParam;
			u8 u1bAIFSN;
			u8 u1bACM;
			u8 u1bECWmin;
			u8 u1bECWmax;
			u32 u4blongData;
			u16 u2bTXOPLimit;
			u8	ulbAcmCtrl;

			u4blongData = 0;
			u1bAIFSN = 2; 
			u1bACM = 0;
			u1bECWmin = CWminIndex;
			u1bECWmax = CWmaxIndex;
			u2bTXOPLimit = 0;

			u1bAIFS = u1bAIFSN * SlotTimeTimer + GET_ASIFSTIME(pmp_priv->current_modulation); 
			u4bAcParam = (	(((u32)(u2bTXOPLimit)) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
							(((u32)(u1bECWmax)) << AC_PARAM_ECW_MAX_OFFSET)	|  
							(((u32)(u1bECWmin)) << AC_PARAM_ECW_MIN_OFFSET)	|  
							(((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET)	);

			write_nic_word(dev, EDCAPARA_BK, u4bAcParam);
			write_nic_word(dev, EDCAPARA_BE, u4bAcParam);
			write_nic_word(dev, EDCAPARA_VI, u4bAcParam);
			write_nic_word(dev, EDCAPARA_VO, u4bAcParam);

			ulbAcmCtrl = (read_nic_byte( dev, AcmHwCtrl ) | 1);
			ulbAcmCtrl &= (~AcmHw_BeqEn);
			write_nic_byte(dev, AcmHwCtrl, ulbAcmCtrl );

			ulbAcmCtrl = (read_nic_byte( dev, AcmHwCtrl ) | 1);
			ulbAcmCtrl &= (~AcmHw_ViqEn);
			write_nic_byte(dev, AcmHwCtrl, ulbAcmCtrl );

			ulbAcmCtrl = (read_nic_byte( dev, AcmHwCtrl ) | 1);
			ulbAcmCtrl &= (~AcmHw_BeqEn);
			write_nic_byte(dev, AcmHwCtrl, ulbAcmCtrl );
		}


		bResult = 0;

	}
	else
	{
		bResult = -1;
	}

	return bResult;

}	/* mpt_ProSetChannel */
#endif

int mp_ioctl_stop_hdl(struct net_device *dev, unsigned char set, void*param)
{
	int ret = 0;
	int bstop;
	struct r8192_priv	*priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;

	DMESG("+mp_ioctl_stop_hdl\n");

	bstop = *(int*)param;
	if(param==NULL)
		return -1;

	if(set)
	{
		if(bstop == MP_STOP_MODE)
		{
			u8 btMsr = 0;
			struct ieee80211_device *ieee = priv->ieee80211;
		
			pmp_priv->test_start= 0;

			ieee->fts = DEFAULT_FRAG_THRESHOLD;
			priv->rts = 2346;

			ieee->state = IEEE80211_NOLINK;
			btMsr = MSR_LINK_ENEDCA;
			btMsr |= MSR_LINK_NONE << MSR_LINK_SHIFT;
			write_nic_byte(dev, MSR, btMsr);

			write_nic_dword(dev, RCR, priv->ReceiveConfig);


			ieee->iw_mode = IW_MODE_INFRA;

			ieee->ssid_set = 0;



			priv->stats.rxok = 0;
			priv->stats.rxstaterr= 0;
			priv->stats.rxurberr= 0;

			priv->stats.txbkokint = 0;
			priv->stats.txbkerr = 0;
			priv->stats.txbkdrop = 0;
			priv->stats.txbeokint = 0;
			priv->stats.txbeerr = 0;
			priv->stats.txbedrop = 0;
			priv->stats.txviokint = 0;
			priv->stats.txvierr = 0;
			priv->stats.txvidrop = 0;
			priv->stats.txvookint = 0;
			priv->stats.txvoerr = 0;
			priv->stats.txvodrop = 0;
			priv->stats.txbeaconokint = 0;
			priv->stats.txbeaconerr = 0;
			priv->stats.txbeacondrop = 0;
			priv->stats.txmanageokint = 0;
			priv->stats.txmanageerr = 0;
			priv->stats.txmanagedrop = 0;

			pmp_priv->bStartContTx = FALSE;
			pmp_priv->bSingleTone = FALSE;
			pmp_priv->bmp_mode = FALSE;
		}
	}
	else
	{
		
	}

	return ret;

}

/*-----------------------------------------------------------------------------
 * Function:	mpt_StartCckContTx()
 *
 * Overview:	Start CCK Continuous Tx.
 *
 * Input:		PADAPTER	pAdapter
 *				BOOLEAN		bScrambleOn
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
void mp_start_continuous_tx_cck(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;

	u32			cckrate;

	printk("mp_start_continuous_tx_cck\n");

	if(!rtl8192_QueryBBReg(dev, rFPGA0_RFMOD, bCCKEn))
		rtl8192_setBBreg(dev, rFPGA0_RFMOD, bCCKEn, bEnable);

	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);

	switch(pmp_priv->rate_index)
	{
		case MPT_RATE_1M:
		case MPT_RATE_2M:
		case MPT_RATE_55M:
		case MPT_RATE_11M:
			cckrate = pmp_priv->rate_index - 1;
			break;
		default:
			cckrate = MPT_RATE_1M - 1;
			break;
	}
	rtl8192_setBBreg(dev, rCCK0_System, bCCKTxRate, cckrate);

	rtl8192_setBBreg(dev, rCCK0_System, bCCKBBMode, 0x2);    
	rtl8192_setBBreg(dev, rCCK0_System, bCCKScramble, 0x1);  

	pmp_priv->bCckContTx = TRUE;
	pmp_priv->bOfdmContTx = FALSE;
}	/* mpt_StartCckContTx */

void mp_stop_continuous_tx_cck(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;


	printk("mp_stop_continuous_tx_cck\n");
	pmp_priv->bCckContTx = FALSE;
	pmp_priv->bOfdmContTx = FALSE;

	rtl8192_setBBreg(dev, rCCK0_System, bCCKBBMode, 0x0);    
	rtl8192_setBBreg(dev, rCCK0_System, bCCKScramble, 0x1);  

	rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x0);
	rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x1);
}	/* mpt_StopCckCoNtTx */


void mp_start_continuous_tx_ofdm(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;


	printk("+mp_start_continuous_tx_ofdm\n");

	if(!rtl8192_QueryBBReg(dev, rFPGA0_RFMOD, bOFDMEn))
		rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, bEnable);

	rtl8192_setBBreg(dev, rCCK0_System, bCCKBBMode, bDisable);

	rtl8192_setBBreg(dev, rCCK0_System, bCCKScramble, bEnable);

	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bEnable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);

	rtl8192_setBBreg(dev, 0x820, 0x400, 1);
	rtl8192_setBBreg(dev, 0x830, 0x400, 1);

	pmp_priv->bCckContTx = FALSE;
	pmp_priv->bOfdmContTx = TRUE;

}

void mp_stop_continuous_tx_ofdm(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;


	printk("mp_stop_continuous_tx_ofdm\n");


	pmp_priv->bCckContTx = FALSE;
	pmp_priv->bOfdmContTx = FALSE;

	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
	mdelay(10);

	rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x0);
	rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x1);
	rtl8192_setBBreg(dev, 0x820, 0x400, 0);
	rtl8192_setBBreg(dev, 0x830, 0x400, 0);
}

int mp_ioctl_continuous_tx_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;

	int ret = 0;

	
	
	printk("mp_ioctl_continuous_tx_hdl for rate(%d)\n", pmp_priv->rate_index);
	if(pmp_priv->bStartContTx == FALSE)
	{ 
		if( pmp_priv->rate_index>= MPT_RATE_1M && pmp_priv->rate_index <= MPT_RATE_11M )
		{
			mp_start_continuous_tx_cck(dev);
			pmp_priv->bStartContTx = TRUE;
			
		}
		else if(pmp_priv->rate_index >= MPT_RATE_6M && pmp_priv->rate_index <= MPT_RATE_MCS15 )
		{
			mp_start_continuous_tx_ofdm(dev);
			pmp_priv->bStartContTx = TRUE;
		}
		else
		{
			printk("MPT_ProSetUpContTx(): Unknown wireless rate index: %d\n", pmp_priv->rate_index);
			pmp_priv->bStartContTx = FALSE;
			pmp_priv->bCckContTx = FALSE;
			pmp_priv->bOfdmContTx = FALSE;
		}
	}
	else
	{ 
		u8	bCckContTx = pmp_priv->bCckContTx;
		u8	bOfdmContTx = pmp_priv->bOfdmContTx;

		if(bCckContTx == TRUE && bOfdmContTx == FALSE)
		{ 
			mp_stop_continuous_tx_cck(dev);
			pmp_priv->bStartContTx = FALSE;
		}
		else if(bCckContTx == FALSE && bOfdmContTx == TRUE)
		{ 
			mp_stop_continuous_tx_ofdm(dev);
			pmp_priv->bStartContTx = FALSE;
		}
		else if(bCckContTx == FALSE && bOfdmContTx == FALSE)
		{ 
		}
		else
		{ 
			printk("MPT_ProSetUpContTx(): Unexpected case! pAdapter->bCckContTx: %d , pAdapter->bOfdmContTx: %d\n",
			pmp_priv->bCckContTx, pmp_priv->bOfdmContTx);
		}
	}


	printk("-mp_ioctl_continuous_tx_hdl()\n");
	return ret;

}

int mp_ioctl_packet_rx_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;

	priv->stats.rxoktotal = 0;
	priv->stats.rxstaterr = 0;
	priv->stats.rxurberr = 0;

	if (pmp_priv->bPacketRx == FALSE)
	{
		u32 write_rcr = 0;

		printk("mp_ioctl_packet_rx_hdl start\n");
		pmp_priv->rcr =read_nic_dword(dev,RCR);
		write_rcr = (pmp_priv->rcr & (~(RCR_AAP|RCR_APM|RCR_AM|RCR_AB)));
		write_nic_dword(dev, RCR, write_rcr);
		pmp_priv->bPacketRx = TRUE;
	}
	else
	{
		printk("mp_ioctl_packet_rx_hdl stop\n");

		write_nic_dword(dev,RCR,pmp_priv->rcr);
		pmp_priv->bPacketRx = FALSE;
	}

	return 0;
}

int mp_ioctl_get_packets_rx_hdl(struct net_device *dev, unsigned char set, void*param)
{
	int ret = 0;
	struct packets_rx_param *ppacketsrx = (struct packets_rx_param *)param;
	struct r8192_priv *priv = ieee80211_priv(dev);

	if(ppacketsrx == NULL)
		return -1;

	if(set)
	{
		ret = -1;
	}
	else
	{
		ppacketsrx->rxok =priv->stats.rxoktotal;
		ppacketsrx->rxerr = priv->stats.rxstaterr+priv->stats.rxurberr;
	}

	printk("mp_ioctl_get_packets_rx_hdl\n");

	return ret;
}


int mp_ioctl_single_carrier_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;
	int ret = 0;

	if(pmp_priv->bSingleCarrier)
	{
		if(!rtl8192_QueryBBReg(dev, rFPGA0_RFMOD, bOFDMEn))
			rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, bEnable);

		rtl8192_setBBreg(dev, rCCK0_System, bCCKBBMode, bDisable);

		rtl8192_setBBreg(dev, rCCK0_System, bCCKScramble, bEnable);

		rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
		rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bEnable);
		rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
	}
	else
	{ 
	    rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMContinueTx, bDisable);
	    rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleCarrier, bDisable);
	    rtl8192_setBBreg(dev, rOFDM1_LSTF, bOFDMSingleTone, bDisable);
	    mdelay(10);
	    rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x0);
	    rtl8192_setBBreg(dev, rPMAC_Reset, bBBResetB, 0x1);
	}

	return ret;
}

int mp_ioctl_set_txagc_offset_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct txagc_offset_param * ptxagc = (struct txagc_offset_param *)param;

	int	ret = TRUE;
	u32 ulTxAGCOffset = ptxagc->txagc_offset;
	u32 u4RegValue, u32_bitmask;
	u32	TxAGCOffset_B, TxAGCOffset_C, TxAGCOffset_D;

	TxAGCOffset_B = (ulTxAGCOffset&0x000000ff);
	TxAGCOffset_C = ((ulTxAGCOffset&0x0000ff00)>>8);
	TxAGCOffset_D = ((ulTxAGCOffset&0x00ff0000)>>16);

	if( TxAGCOffset_B > TxAGC_Offset_neg1 ||
		TxAGCOffset_C > TxAGC_Offset_neg1 ||
		TxAGCOffset_D > TxAGC_Offset_neg1 )
	{
		printk("mp_ioctl_set_txagc_offset_hdl(): TxAGCOffset:%d is invalid\n",ulTxAGCOffset);
		return FALSE;
	}

	priv->AntennaTxPwDiff[0] = TxAGCOffset_B;
	priv->AntennaTxPwDiff[1] = TxAGCOffset_C;
	priv->AntennaTxPwDiff[2] = TxAGCOffset_D;
	


	u32_bitmask = (bXBTxAGC|bXCTxAGC|bXDTxAGC);

	switch(priv->rf_chip)
	{
	case RF_8256:
		u4RegValue = (TxAGCOffset_D<<8 | TxAGCOffset_C<<4 | TxAGCOffset_B);
		rtl8192_setBBreg(dev, rFPGA0_TxGainStage, u32_bitmask, u4RegValue);
		break;
	default:
		rtl8192_setBBreg(dev, rFPGA0_TxGainStage, u32_bitmask, TxAGC_Offset_0);
		break;
	}

	printk("\n-mp_ioctl_set_txagc_offset_hdl() is finished \n");

	return ret;

}	/*-mp_ioctl_set_txagc_offset_hdl */


int mp_ioctl_set_antenna_bb_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;
	struct antenna_param * panten = (struct antenna_param * )param;
	R_ANTENNA_SELECT_OFDM	*p_ofdm_tx;	/* OFDM Tx register */
	R_ANTENNA_SELECT_CCK	*p_cck_txrx;

	int ret = 0;
	u32	ulAntenna = panten->antenna_path;
	u32 ulAntennaTx;
	u32 ulAntennaRx;
	
	u8	r_rx_antenna_ofdm=0, r_ant_select_cck_val=0;
	u8	chgTx=0, chgRx=0;
	u32	r_ant_sel_cck_val=0, r_ant_select_ofdm_val=0, r_ofdm_tx_en_val=0;
	u32	ulbandwidth = pmp_priv->bandwidth;
	u32	rfBand_A=0, rfBand_B=0, rfBand_C=0, rfBand_D=0;

	pmp_priv->antenna_tx_path = ((ulAntenna&0xffff0000)>>16);
	pmp_priv->antenna_rx_path = (ulAntenna&0x0000ffff);

	ulAntennaTx = pmp_priv->antenna_tx_path;
	ulAntennaRx = pmp_priv->antenna_rx_path;

	p_ofdm_tx = (R_ANTENNA_SELECT_OFDM *)&r_ant_select_ofdm_val;
	p_cck_txrx = (R_ANTENNA_SELECT_CCK *)&r_ant_select_cck_val;

	p_ofdm_tx->r_ant_non_ht 		= 0x5;
	p_ofdm_tx->r_ant_ht1			= 0x1;
	p_ofdm_tx->r_ant_ht2			= 0x4;
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
		break;
#if 0 
	case ANTENNA_B:
		p_ofdm_tx->r_tx_antenna		= 0x2;
		r_ofdm_tx_en_val			= 0x2;
		p_ofdm_tx->r_ant_l 			= 0x2;	
		p_ofdm_tx->r_ant_ht_s1 		= 0x2;	
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x2;
		p_cck_txrx->r_ccktx_enable	= 0x4;
		chgTx = 1;
		break;
	case ANTENNA_C:
		p_ofdm_tx->r_tx_antenna		= 0x4;
		r_ofdm_tx_en_val			= 0x4;
		p_ofdm_tx->r_ant_l 			= 0x4;	
		p_ofdm_tx->r_ant_ht_s1 		= 0x4;	
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x4;
		p_cck_txrx->r_ccktx_enable	= 0x2;
		chgTx = 1;
		break;
	case ANTENNA_D:
		p_ofdm_tx->r_tx_antenna		= 0x8;
		r_ofdm_tx_en_val			= 0x8;
		p_ofdm_tx->r_ant_l 			= 0x8;
		p_ofdm_tx->r_ant_ht_s1 		= 0x8;	
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x8;
		p_cck_txrx->r_ccktx_enable	= 0x1;
		chgTx = 1;
		break;
	case ANTENNA_AC:
		p_ofdm_tx->r_tx_antenna		= 0x5;
		r_ofdm_tx_en_val			= 0x5;
		p_ofdm_tx->r_ant_l 			= 0x5;
		p_ofdm_tx->r_ant_ht_s1 		= 0x5;
		p_ofdm_tx->r_ant_non_ht_s1 	= 0x5;
		p_cck_txrx->r_ccktx_enable	= 0xA;
		chgTx = 1;
		break;
#endif		

	default:
		break;
	}
	
	switch(ulAntennaRx)
	{
	case ANTENNA_A:
		rfBand_A = 0x500;	
		r_rx_antenna_ofdm 			= 0x1;	
		p_cck_txrx->r_cckrx_enable 	= 0x0;	
		p_cck_txrx->r_cckrx_enable_2	= 0x0;	
		chgRx = 1;
		break;
	case ANTENNA_B:
		rfBand_B = 0x500;	
		r_rx_antenna_ofdm 			= 0x2;	
		p_cck_txrx->r_cckrx_enable 	= 0x1;	
		p_cck_txrx->r_cckrx_enable_2	= 0x1;	
		chgRx = 1;
		break;
#if 0
	case ANTENNA_C:
		rfBand_C = 0x500;	
		r_rx_antenna_ofdm 			= 0x4;	
		p_cck_txrx->r_cckrx_enable 	= 0x2;	
		p_cck_txrx->r_cckrx_enable_2	= 0x2;	
		chgRx = 1;
		break;
	case ANTENNA_D:
		rfBand_D = 0x500;	
		r_rx_antenna_ofdm 			= 0x8;	
		p_cck_txrx->r_cckrx_enable 	= 0x3;	
		p_cck_txrx->r_cckrx_enable_2	= 0x3;	
		chgRx = 1;
		break;
	case ANTENNA_AC:
		rfBand_A = 0x500;	
		r_rx_antenna_ofdm 			= 0x5;	
		p_cck_txrx->r_cckrx_enable 	= 0x0;	
		p_cck_txrx->r_cckrx_enable_2= 0x2;		
		chgRx = 1;
		break;
	case ANTENNA_BD:
		rfBand_B = 0x500;	
		r_rx_antenna_ofdm 			= 0xA;	
		p_cck_txrx->r_cckrx_enable 	= 0x1;	
		p_cck_txrx->r_cckrx_enable_2= 0x3;		
		chgRx = 1;
		break;
#endif
	case ANTENNA_AB:
		rfBand_A = 0x500;	
		r_rx_antenna_ofdm 			= 0x3;	
		p_cck_txrx->r_cckrx_enable 	= 0x0;	
		p_cck_txrx->r_cckrx_enable_2= 0x1;		
		chgRx = 1;
		break;
#if 0
	case ANTENNA_CD:
		rfBand_C = 0x500;	
		r_rx_antenna_ofdm 			= 0xC;	
		p_cck_txrx->r_cckrx_enable 	= 0x2;	
		p_cck_txrx->r_cckrx_enable_2= 0x3;		
		chgRx = 1;
		break;
	case ANTENNA_ABCD:
		rfBand_A = 0x500;	
		r_rx_antenna_ofdm 			= 0xF;	
		p_cck_txrx->r_cckrx_enable 	= 0x0;	
		p_cck_txrx->r_cckrx_enable_2= 0x2;		
		chgRx = 1;
		break;
#endif
	}
	
	if (ulbandwidth == BAND_20MHZ_MODE)
	{
		if(!rfBand_A)
			rfBand_A = 0x100;
		if(!rfBand_B)
			rfBand_B = 0x100;
		if(!rfBand_C)
			rfBand_C = 0x100;
		if(!rfBand_D)
			rfBand_D = 0x100;
	}
	else
	{
		rfBand_A = 0x300;
		rfBand_B = 0x300;
		rfBand_C = 0x300;
		rfBand_D = 0x300;
	}

	printk("\n MPT_ProSwitchAntenna():rfReg0x0b = 0x%x, 0x%x, 0x%x, 0x%x \n",
		rfBand_A, rfBand_B, rfBand_C, rfBand_D);

	rtl8192_phy_SetRFReg(dev, RF90_PATH_A, 0x0b, bMask12Bits, rfBand_A);
	udelay(100);
	rtl8192_phy_SetRFReg(dev, RF90_PATH_B, 0x0b, bMask12Bits, rfBand_B);
	udelay(100); 
	rtl8192_phy_SetRFReg(dev, RF90_PATH_C, 0x0b, bMask12Bits, rfBand_C);
	udelay(100);
	rtl8192_phy_SetRFReg(dev, RF90_PATH_D, 0x0b, bMask12Bits, rfBand_D);
	udelay(100);
	
	if(chgTx && chgRx)
	{
		r_ant_sel_cck_val = r_ant_select_cck_val;
		rtl8192_setBBreg(dev, rFPGA1_TxInfo, 0x0fffffff, r_ant_select_ofdm_val);		
		rtl8192_setBBreg(dev, rFPGA0_TxInfo, 0x0000000f, r_ofdm_tx_en_val);		
		rtl8192_setBBreg(dev, rOFDM0_TRxPathEnable, 0x0000000f, r_rx_antenna_ofdm);	
		rtl8192_setBBreg(dev, rOFDM1_TRxPathEnable, 0x0000000f, r_rx_antenna_ofdm);	
		rtl8192_setBBreg(dev, rCCK0_AFESetting, bMaskByte3, r_ant_sel_cck_val);		
	}

	return ret;
}	/* mpt_ProSetAntennaBB */


int mp_ioctl_single_tone_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;

	u32		ulAntennaTx = pmp_priv->antenna_tx_path;
	RF90_RADIO_PATH_E	rfPath;

	int ret = 0;

	printk("mp_ioctl_single_tone_hdl for rate(%d)\n", pmp_priv->rate_index);

	switch(ulAntennaTx)
	{
		case ANTENNA_A:
			rfPath = RF90_PATH_A;
			break;
		case ANTENNA_C:
			rfPath = RF90_PATH_C;
			break;
		default:
			rfPath = RF90_PATH_A;
			break;
	}
	
	if(pmp_priv->bSingleTone==FALSE)
	{	
		printk("Start setting RF reg for single tone.\n");
		rtl8192_phy_SetRFReg(dev, rfPath, 0x00, bMask12Bits, 0xb0);
		udelay(100);
		rtl8192_phy_SetRFReg(dev, rfPath, 0x01, bMask12Bits, 0x41f);
		udelay(100);
		rtl8192_phy_SetRFReg(dev, rfPath, 0x0c, bMask12Bits, 0xc40);
		udelay(100);
		rtl8192_phy_SetRFReg(dev, rfPath, 0x08, bMask12Bits, 0xe01);
		udelay(100);
		rtl8192_phy_SetRFReg(dev, rfPath, 0x09, bMask12Bits, 0x5f0);
		udelay(100);
		pmp_priv->bSingleTone =  TRUE;
	}
	else
	{
		printk("Stop setting RF reg for single tone.\n");
		rtl8192_phy_SetRFReg(dev, rfPath, 0x01, bMask12Bits, 0xee0);
		udelay(100);
		rtl8192_phy_SetRFReg(dev, rfPath, 0x0c, bMask12Bits, 0x240);
		udelay(100);
		rtl8192_phy_SetRFReg(dev, rfPath, 0x08, bMask12Bits, 0xe1c);
		udelay(100);
		rtl8192_phy_SetRFReg(dev, rfPath, 0x09, bMask12Bits, 0x7f0);
		udelay(100);
		rtl8192_phy_SetRFReg(dev, rfPath, 0x00, bMask12Bits, 0xbf);
		udelay(100);
		pmp_priv->bSingleTone = FALSE;
	}

	return ret;
}


/*-----------------------------------------------------------------------------
 * Function:	mp_is_legal_channel()
 *
 * Overview:	Verify if specified channel is valid for current wireless mode.
 *				NOTE --> 
 *				The difference between IsLegalChannel() and 
 *				MPT_IsLegalChannel() is IsLegalChannel() is based on current 
 *				channel plan.
 *
 * Input:		PADAPTER	pAdapter
 *				u4Byte		ulChannel
 *
 * Output:		NONE
 *
 * Return:		TRUE if it is a legal channel, FALSE O.W..
 *
 * Revised History:
 *	When		Who		Remark
 *	05/16/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
u8 mp_is_legal_channel(u32 ulWirelessMode,u32 ulChannel)
{
	u8 bLegalChannel = TRUE;

	if( IS_WIRELESS_MODE_B(ulWirelessMode) == TRUE || 
		IS_WIRELESS_MODE_G(ulWirelessMode) == TRUE)
	{
		if(ulChannel > 14 || ulChannel < 1)
		{
			bLegalChannel = FALSE;
		}
	}
	else if(IS_WIRELESS_MODE_A(ulWirelessMode) == TRUE)
	{
		if(ulChannel <= 14)
		{
			bLegalChannel = FALSE;
		}
	}
	/* 2007/05/10 MH Add N 2.4/5 G channel check. */
	else if (IS_WIRELESS_MODE_N_24G(ulWirelessMode) == TRUE)
	{
		if (ulChannel < 1 || ulChannel > 14)
		{
			bLegalChannel = FALSE;
		}
	}
	else if (IS_WIRELESS_MODE_N_5G(ulWirelessMode) == TRUE)
	{

	}

	return bLegalChannel;

}	/* MPT_IsLegalChannel */


/*-----------------------------------------------------------------------------
 * Function:	mp_ioctl_set_ch_modulation_hdl
 *
 * Overview:	Switch channel for OID_RT_PRO_SET_CHANNEL_DIRECT_CALL.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *		mpt_ProSetChannel()
 *		mpt_ProSetModulation()
 *
 *---------------------------------------------------------------------------*/
int mp_ioctl_set_ch_modulation_hdl(struct net_device *dev, unsigned char set, void *param)
{
	struct r8192_priv		* priv		= ieee80211_priv(dev);
	struct mp_priv 			* pmp_priv	= &priv->mppriv;
	struct rfchannel_param	* prfchannel= (struct rfchannel_param*)param;
	struct txpower_param	txpwr;


	u8 SlotTimeTimer;
	u8 CWminIndex;
	u8 CWmaxIndex;


	u32	rfReg0x14, rfReg0x2c;
	s8	bResult = 0;
	u8	eRFPath;



	if(prfchannel==NULL)
		return -1;

	printk("set ch=0x%x, modem=0x%x\n", prfchannel->ch, prfchannel->modem);

	if(set)
	{
/* channel */

		switch(pmp_priv->current_modulation)
		{
			case WIRELESS_MODE_A:
			case WIRELESS_MODE_N_5G:
				if (prfchannel->ch<=14)
				{
					return -1;
				}
				break;
			case WIRELESS_MODE_B:
			case WIRELESS_MODE_G:
			case WIRELESS_MODE_N_24G:
				if (prfchannel->ch>14)
				{
					return -1;
				}	
				break;
		}

		rfReg0x14 = 0x5ab;
		if (pmp_priv->bandwidth == BAND_20MHZ_MODE)
		{
			if(pmp_priv->switch_to_channel == 1 || pmp_priv->switch_to_channel == 11)
			{
				if(pmp_priv->rate_index >= MPT_RATE_6M)	
					rfReg0x14 = 0x59b;
			}
		}
		else
		{
			if(pmp_priv->switch_to_channel == 3 || pmp_priv->switch_to_channel == 9)
				rfReg0x14 = 0x59b;
		}

		for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x14, bMask12Bits, rfReg0x14);
			udelay(100);
		}
		
		if(pmp_priv->bandwidth == BAND_20MHZ_MODE)
		{
			rfReg0x2c = 0x3d7;
			if(pmp_priv->rate_index<= MPT_RATE_11M)	
			{
				if(pmp_priv->switch_to_channel == 1 || pmp_priv->switch_to_channel == 11)
					rfReg0x2c = 0x3f7;
			}
		}
		else
		{
			rfReg0x2c = 0x3ff;
		}

		for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x2c, bMask12Bits, rfReg0x2c);
			udelay(100);
		}


		priv->ieee80211->SetWirelessMode(dev, prfchannel->modem);
		priv->rf_set_chan(dev, prfchannel->ch);
		priv->ieee80211->current_network.channel = prfchannel->ch;


		pmp_priv->current_channel=prfchannel->ch;
		dm_cck_txpower_adjust(dev,(pmp_priv->current_channel == 14));

		if(pmp_priv->current_modulation == WIRELESS_MODE_B)
			txpwr.pwr_index = priv->TxPowerLevelCCK[pmp_priv->current_channel-1];
		else
			txpwr.pwr_index = priv->TxPowerLevelOFDM24G[pmp_priv->current_channel-1];
		mp_ioctl_set_txpower_hdl(dev,set,&txpwr);

/* modulation mode */
		pmp_priv->current_modulation = prfchannel->modem;

		switch( pmp_priv->current_modulation )
		{
			case WIRELESS_MODE_A:
				CWminIndex = 4;
				CWmaxIndex = 10;
				break;
			case WIRELESS_MODE_B:
				CWminIndex = 5;
				CWmaxIndex = 10;
				break;
			case WIRELESS_MODE_G:
				CWminIndex = 4;
				CWmaxIndex = 10;
				break;
			case WIRELESS_MODE_N_24G:
			case WIRELESS_MODE_N_5G:
				SlotTimeTimer = 20;
				CWminIndex = 4;
				CWmaxIndex = 10;
				break;
			default:
				break;
		}

		{
			u8	u1bAIFS = GET_ASIFSTIME(pmp_priv->current_modulation) + (2 * SlotTimeTimer);

			write_nic_byte(dev, SLOT_TIME, SlotTimeTimer);			
			write_nic_byte(dev, EDCAPARA_VO, u1bAIFS);
			write_nic_byte(dev, EDCAPARA_VI, u1bAIFS);
			write_nic_byte(dev, EDCAPARA_BE, u1bAIFS);
			write_nic_byte(dev, EDCAPARA_BK, u1bAIFS);
		}

/*QosSetLegacyACParam*/
		{

			u8	u1bAIFS;
			u32	u4bAcParam;
			u8	u1bAIFSN;
			u8	u1bACM;
			u8	u1bECWmin;
			u8	u1bECWmax;
			u32	u4blongData;
			u16	u2bTXOPLimit;
			u8	ulbAcmCtrl;

			u4blongData = 0;
			u1bAIFSN = 2; 
			u1bACM = 0;
			u1bECWmin = CWminIndex;
			u1bECWmax = CWmaxIndex;
			u2bTXOPLimit = 0;

			u1bAIFS = u1bAIFSN * SlotTimeTimer + GET_ASIFSTIME(pmp_priv->current_modulation); 
			u4bAcParam = (	(((u32)(u2bTXOPLimit)) << AC_PARAM_TXOP_LIMIT_OFFSET)	| 
							(((u32)(u1bECWmax)) << AC_PARAM_ECW_MAX_OFFSET)	|  
							(((u32)(u1bECWmin)) << AC_PARAM_ECW_MIN_OFFSET)	|  
							(((u32)u1bAIFS) << AC_PARAM_AIFS_OFFSET)	);

			write_nic_word(dev, EDCAPARA_BE, u4bAcParam);
			write_nic_word(dev, EDCAPARA_BK, u4bAcParam);
			write_nic_word(dev, EDCAPARA_VI, u4bAcParam);
			write_nic_word(dev, EDCAPARA_VO, u4bAcParam);

			ulbAcmCtrl = (read_nic_byte( dev, AcmHwCtrl ) | 1);
			ulbAcmCtrl &= (~AcmHw_BeqEn);
			write_nic_byte(dev, AcmHwCtrl, ulbAcmCtrl );

			ulbAcmCtrl = (read_nic_byte( dev, AcmHwCtrl ) | 1);
			ulbAcmCtrl &= (~AcmHw_ViqEn);
			write_nic_byte(dev, AcmHwCtrl, ulbAcmCtrl );

			ulbAcmCtrl = (read_nic_byte( dev, AcmHwCtrl ) | 1);
			ulbAcmCtrl &= (~AcmHw_BeqEn);
			write_nic_byte(dev, AcmHwCtrl, ulbAcmCtrl );
		}


		bResult = 0;

	}
	else
	{
		bResult = -1;
	}

	return bResult;

}	/* mpt_ProSetChannel */


void mp_set8256CCKTxPower(struct net_device *dev, u8 TxPower)
{
	u16				TxAGC=0;


#if 0
#else

	TxAGC = TxPower;
	rtl8192_setBBreg(dev, rTxAGC_CCK_Mcs32, bTxAGCRateCCK, TxAGC);
#endif

}


void mp_set8256OFDMTxPower(struct net_device *dev,u8 TxPower)
{
	u32				TxAGC=0;

#if 0

	TxAGC|= ((TxPower<<24)|(TxPower<<16)|(TxPower<<8)|TxPower);
	write_nic_dword(dev, MCS_TXAGC, TxAGC);
	write_nic_dword(dev, MCS_TXAGC+4, TxAGC);
#else


	TxAGC|= ((TxPower<<24)|(TxPower<<16)|(TxPower<<8)|TxPower);
	
	rtl8192_setBBreg(dev, rTxAGC_Rate18_06, bTxAGCRate18_06, TxAGC);
	rtl8192_setBBreg(dev, rTxAGC_Rate54_24, bTxAGCRate54_24, TxAGC);
	rtl8192_setBBreg(dev, rTxAGC_Mcs03_Mcs00, bTxAGCRateMCS3_MCS0, TxAGC);
	rtl8192_setBBreg(dev, rTxAGC_Mcs07_Mcs04, bTxAGCRateMCS7_MCS4, TxAGC);
	rtl8192_setBBreg(dev, rTxAGC_Mcs11_Mcs08, bTxAGCRateMCS11_MCS8, TxAGC);
	rtl8192_setBBreg(dev, rTxAGC_Mcs15_Mcs12, bTxAGCRateMCS15_MCS12, TxAGC);
#endif
}


int mp_ioctl_set_txpower_hdl(struct net_device *dev, unsigned char set, void *param)
{
	u8 pi;
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct txpower_param*ptxpwr = (struct txpower_param*)param;
	struct mp_priv * pmp_priv = &priv->mppriv;
	u8	TxPowerLevel_CCK, TxPowerLevel_HTOFDM;


	if(ptxpwr==NULL)
		return -1;

	pi = (u8)ptxpwr->pwr_index;

	if(pi > 0x36)
		return -1;

	printk("+mp_ioctl_set_txpower, index=0x%x\n", pi);

	if(set)
	{
		if(pi > 0x36)
		{
			printk("mpt_ProSetTxPower(): TxPWR:%d is invalid\n", pi);
			return FALSE;
		}
		
		priv->TxPowerLevelCCK[pmp_priv->current_channel-1] = pi;
		priv->TxPowerLevelOFDM24G[pmp_priv->current_channel-1] = pi;

		TxPowerLevel_CCK = pi;
		TxPowerLevel_HTOFDM = pi;

		mp_set8256CCKTxPower(dev, TxPowerLevel_CCK);
		mp_set8256OFDMTxPower(dev, TxPowerLevel_HTOFDM);
	}
	else
	{
		return -1;
	}

	return 0;
}


u8 rate_index_mapping(u32 rate_index)
{
	switch(rate_index)
	{
		/* CCK rate. */
		case	MPT_RATE_1M:	return 2;
		case	MPT_RATE_2M:	return 4;
		case	MPT_RATE_55M:	return 11;	break;
		case	MPT_RATE_11M:	return 22;	break;
	
		/* OFDM rate. */
		case	MPT_RATE_6M:	return 12;	break;
		case	MPT_RATE_9M:	return 18;	break;
		case	MPT_RATE_12M:	return 24;	break;
		case	MPT_RATE_18M:	return 36;	break;
		case	MPT_RATE_24M:	return 48;	break;			
		case	MPT_RATE_36M:	return 72;	break;
		case	MPT_RATE_48M:	return 96;	break;
		case	MPT_RATE_54M:	return 108; break;
	
		/* HT rate. */
		case	MPT_RATE_MCS0:	return 0x80;	break;
		case	MPT_RATE_MCS1:	return 0x81;	break;
		case	MPT_RATE_MCS2:	return 0x82;	break;
		case	MPT_RATE_MCS3:	return 0x83;	break;
		case	MPT_RATE_MCS4:	return 0x84;	break;
		case	MPT_RATE_MCS5:	return 0x85;	break;
		case	MPT_RATE_MCS6:	return 0x86;	break;
		case	MPT_RATE_MCS7:	return 0x87;	break;
		case	MPT_RATE_MCS8:	return 0x88;	break;
		case	MPT_RATE_MCS9:	return 0x89;	break;
		case	MPT_RATE_MCS10: return 0x8A;	break;
		case	MPT_RATE_MCS11: return 0x8B;	break;
		case	MPT_RATE_MCS12: return 0x8C;	break;
		case	MPT_RATE_MCS13: return 0x8D;	break;
		case	MPT_RATE_MCS14: return 0x8E;	break;
		case	MPT_RATE_MCS15: return 0x8F;	break;
		case MPT_RATE_LAST:
		break;
	
		default:
			return 0xFF;
		break;
	}

	return 0xFF;
}

int mp_ioctl_set_preamble_hdl(struct net_device *dev, unsigned char set, void *param)
{
	struct preamble_param * ppreamble = (struct preamble_param*)param;
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;

	pmp_priv->current_preamble = ppreamble->preamble;

	return 0;

}	/* mpt_ProSetPreamble */




int mp_ioctl_set_datarate_hdl(struct net_device *dev, unsigned char set, void *param)
{
	unsigned char idx;
	struct datarate_param *pdatarate = (struct datarate_param*)param;
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct mp_priv * pmp_priv = &priv->mppriv;

	static u8	InCCK2M=0;
	u8			DataRate = 0xFF;
	u32			rfReg0x14, rfReg0x15, rfReg0x2c, rfReg0x0f;
	u8			eRFPath;


	struct preamble_param set_preamble;

	set_preamble.preamble = Long_GI;


	idx = (unsigned char)pdatarate->rate_index;

	printk("+mp_ioctl_set_datarate_hdl ridx:%d\n",idx);

	if(set)
	{
		pmp_priv->rate_index = pdatarate->rate_index;
		
		DataRate = rate_index_mapping(pmp_priv->rate_index);
		
		rfReg0x14 = 0x5ab;
		if (pmp_priv->bandwidth == BAND_20MHZ_MODE)
		{
			if(pmp_priv->current_channel== 1 || pmp_priv->current_channel== 11)
			{
				if(pmp_priv->rate_index >= MPT_RATE_6M)	
					rfReg0x14 = 0x59b;
			}
		}
		else
		{
			if(pmp_priv->current_channel == 3 || pmp_priv->current_channel == 9)
				rfReg0x14 = 0x59b;
		}
		for(eRFPath = 0; eRFPath < priv->NumTotalRFPath; eRFPath++)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x14, bMask12Bits, rfReg0x14);
			udelay(100);
		}
		
		if(pmp_priv->bandwidth == BAND_20MHZ_MODE)
		{
			rfReg0x2c = 0x3d7;
			if(pmp_priv->rate_index<= MPT_RATE_11M)	
			{
				if(pmp_priv->current_channel == 1 || pmp_priv->current_channel == 11)
					rfReg0x2c = 0x3f7;
			}
		}
		else
		{
			rfReg0x2c = 0x3ff;
		}
		for(eRFPath = 0; eRFPath < priv->NumTotalRFPath; eRFPath++)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x2c, bMask12Bits, rfReg0x2c);
			udelay(100);
		}
		rfReg0x15 = 0xf80;
		if(pmp_priv->rate_index == MPT_RATE_2M)
		{
			if(!InCCK2M)
			{
				if(pmp_priv->bandwidth == BAND_20MHZ_MODE)
					rfReg0x15 = 0xfc0;
				for(eRFPath = 0; eRFPath < priv->NumTotalRFPath; eRFPath++)
				{
					rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x15, bMask12Bits, rfReg0x15);
					udelay(100);
				}
				InCCK2M = 1;
			}
		}
		else
		{
			if(InCCK2M)
			{
				for(eRFPath = 0; eRFPath < priv->NumTotalRFPath; eRFPath++)
				{
					rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x15, bMask12Bits, rfReg0x15);
					udelay(100);
				}
				InCCK2M = 0;
			}
		}
		rfReg0x0f = 0xff0;
		if(pmp_priv->rate_index <= MPT_RATE_11M)
		{
			rfReg0x0f = 0xff0;
		}
		else
		{
			rfReg0x0f = 0x990;
		}

		for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
		{
			rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x0f, bMask12Bits, rfReg0x0f);
			udelay(100);
		}


		return 0;
	}
	else
	{
		return -1;
	}

	return 0;
}




int mp_ioctl_set_bandwidth_hdl(struct net_device *dev, unsigned char set, void *param)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	struct ieee80211_device* ieee = priv->ieee80211;
	PRT_HIGH_THROUGHPUT pHTInfo = ieee->pHTInfo;
	struct mp_priv * pmp_priv = &priv->mppriv;

	struct bandwidth_param * pbandwidth = (struct bandwidth_param *)param;

	u32	ulbandwidth = pbandwidth->bandwidth;

	u8	eRFPath;
	u32	ulRateIdx = pmp_priv->rate_index;
	
	u8	CurrChannel = pmp_priv->current_channel;
	u32	ulAntennaRx = pmp_priv->antenna_rx_path;
	u32	rfBand_A=0, rfBand_B=0, rfBand_C=0, rfBand_D=0;
	u32	rfReg0x0e, rfReg0x14, rfReg0x15, rfReg0x2c;

	/* 2007/06/07 MH Call normal driver API and set 40MHZ mode. */			
	if (ulbandwidth == BAND_20MHZ_MODE)
	{
		/* 20 MHZ sub-carrier mode --> dont care. */
		pHTInfo->bCurBW40MHz = FALSE;
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_NO_EXT;
		ieee->SetBWModeHandler(dev, 
								HT_CHANNEL_WIDTH_20,
								pHTInfo->CurSTAExtChnlOffset);
	}
	/* Sub-Carrier mode is defined in MAC data sheet chapter 12.3. */
	else if (ulbandwidth == BAND_40MHZ_DUPLICATE_MODE)
	{
		/* 40MHX sub-carrier mode --> duplicate. */
		pHTInfo->bCurBW40MHz = TRUE;				
		pHTInfo->bCurTxBW40MHz = TRUE;								
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_NO_DEF;			
		
		ieee->SetBWModeHandler(dev, 
								HT_CHANNEL_WIDTH_20_40,
								pHTInfo->CurSTAExtChnlOffset);
	}
	else if (ulbandwidth == BAND_40MHZ_LOWER_MODE)
	{
		/* 40MHX sub-carrier mode --> lower mode  */
		pHTInfo->bCurBW40MHz = TRUE;				
		pHTInfo->bCurTxBW40MHz = TRUE;								
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_LOWER;
		
		/* Extention channel is lower. Current channel must > 3. */
		/*if (pMgntInfo->dot11CurrentChannelNumber < 3)				
			DbgPrint("Illegal Current_Chl=%d\r\n", pMgntInfo->dot11CurrentChannelNumber);
		else
			pAdapter->HalFunc.SwChnlByTimerHandler(pAdapter, pMgntInfo->dot11CurrentChannelNumber-2);*/
		
		ieee->SetBWModeHandler(dev, 
										   HT_CHANNEL_WIDTH_20_40,
										   pHTInfo->CurSTAExtChnlOffset);
	}
	else if (ulbandwidth == BAND_40MHZ_UPPER_MODE)
	{
		/* 40MHX sub-carrier mode --> upper mode  */
		pHTInfo->bCurBW40MHz = TRUE;				
		pHTInfo->bCurTxBW40MHz = TRUE;
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_UPPER;
		
		/* Extention channel is upper. Current channel must < 12. */
		/*if (pMgntInfo->dot11CurrentChannelNumber > 12)
			DbgPrint("Illegal Current_Chl=%d", pMgntInfo->dot11CurrentChannelNumber);
		else
			pAdapter->HalFunc.SwChnlByTimerHandler(pAdapter, pMgntInfo->dot11CurrentChannelNumber+2);*/
		
		ieee->SetBWModeHandler(dev, 
										   HT_CHANNEL_WIDTH_20_40,
										   pHTInfo->CurSTAExtChnlOffset);
	}
	else if (ulbandwidth == BAND_40MHZ_DONTCARE_MODE)
	{
		/* 40MHX sub-carrier mode --> dont care mode  */
		pHTInfo->bCurBW40MHz = TRUE;
		pHTInfo->bCurTxBW40MHz = TRUE;
		pHTInfo->CurSTAExtChnlOffset = HT_EXTCHNL_OFFSET_LOWER;
		
		ieee->SetBWModeHandler(dev, 
										   HT_CHANNEL_WIDTH_20_40,
										   pHTInfo->CurSTAExtChnlOffset);
	}
	
	switch(ulAntennaRx)
	{
	case ANTENNA_A:
	case ANTENNA_AC:
	case ANTENNA_AB:
	case ANTENNA_ABCD:
		rfBand_A = 0x500;	
		break;
	case ANTENNA_B:
	case ANTENNA_BD:
		rfBand_B = 0x500;	
		break;
	case ANTENNA_C:
	case ANTENNA_CD:
		rfBand_C = 0x500;	
		break;
	case ANTENNA_D:
		rfBand_D = 0x500;	
		break;
	}
	if (ulbandwidth == BAND_20MHZ_MODE)
	{
		if(!rfBand_A)
			rfBand_A = 0x100;
		if(!rfBand_B)
			rfBand_B = 0x100;
		if(!rfBand_C)
			rfBand_C = 0x100;
		if(!rfBand_D)
			rfBand_D = 0x100;
	}
	else
	{
		rfBand_A = 0x300;
		rfBand_B = 0x300;
		rfBand_C = 0x300;
		rfBand_D = 0x300;
	}


	rtl8192_phy_SetRFReg(dev, RF90_PATH_A, 0x0b, bMask12Bits, rfBand_A);
	udelay(100);
	rtl8192_phy_SetRFReg(dev, RF90_PATH_B, 0x0b, bMask12Bits, rfBand_B);
	udelay(100);
	rtl8192_phy_SetRFReg(dev, RF90_PATH_C, 0x0b, bMask12Bits, rfBand_C);
	udelay(100);
	rtl8192_phy_SetRFReg(dev, RF90_PATH_D, 0x0b, bMask12Bits, rfBand_D);
	udelay(100);
	
	if (ulbandwidth == BAND_20MHZ_MODE)
		rfReg0x0e = 0x21;
	else
		rfReg0x0e = 0x1e1;


	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x0e, bMask12Bits, rfReg0x0e);
		udelay(100);
	}
	rfReg0x14 = 0x5ab;
	if (ulbandwidth == BAND_20MHZ_MODE)
	{
		if(CurrChannel == 1 || CurrChannel == 11)
		{
			if(ulRateIdx >= MPT_RATE_6M)	
				rfReg0x14 = 0x59b;
		}
	}
	else
	{
		if(CurrChannel == 3 || CurrChannel == 9)
			rfReg0x14 = 0x59b;
	}


	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x14, bMask12Bits, rfReg0x14);
		udelay(100);
	}
	rfReg0x15 = 0xf80;
	if(ulRateIdx == MPT_RATE_2M)
	{
		if(ulbandwidth == BAND_20MHZ_MODE)
			rfReg0x15 = 0xfc0;
	}

	for(eRFPath = 0; eRFPath <priv->NumTotalRFPath; eRFPath++)
	{
		rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x15, bMask12Bits, rfReg0x15);
		udelay(100);
	}

	if(ulbandwidth == BAND_20MHZ_MODE)
	{
		rfReg0x2c = 0x3d7;
		if(ulRateIdx <= MPT_RATE_11M)	
		{
			if(CurrChannel == 1 || CurrChannel == 11)
				rfReg0x2c = 0x3f7;
		}
	}
	else
	{
		rfReg0x2c = 0x3ff;
	}


	for(eRFPath = 0; eRFPath <RTL819X_TOTAL_RF_PATH; eRFPath++)
	{
		rtl8192_phy_SetRFReg(dev, (RF90_RADIO_PATH_E)eRFPath, 0x2c, bMask12Bits, rfReg0x2c);
		udelay(100);
	}



	return 0;
}


int mp_ioctl_read_reg_hdl(struct net_device *dev, unsigned char set, void*param)
{
	int ret = 0;
	struct rwreg_param*prwreg = (struct rwreg_param*)param;


	if(prwreg==NULL)
		return -1;

	if(set)
	{
		ret = -1;
	}
	else
	{
		switch(prwreg->width)
		{
			case 1:
				prwreg->value=read_nic_byte(dev, prwreg->offset);
				break;
			case 2:
				prwreg->value=read_nic_word(dev, prwreg->offset);
				break;
			case 4:
				prwreg->value=read_nic_dword(dev, prwreg->offset);
				break;
			default:
				ret = -1;
			   	break;
		}
	}

	if(ret!=-1)
	{
		if(prwreg->width==1)
		{
			printk("+mp_ioctl_read_reg_hdl addr:0x%08x byte:%d value:0x%02x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
		else if(prwreg->width==2)
		{
			printk("+mp_ioctl_read_reg_hdl addr:0x%08x byte:%d value:0x%04x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
		else if(prwreg->width==4)
		{
			printk("+mp_ioctl_read_reg_hdl addr:0x%08x byte:%d value:0x%08x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
	}

	return ret;
}


int mp_ioctl_write_reg_hdl(struct net_device *dev, unsigned char set, void *param)
{
	int ret;
	struct rwreg_param*prwreg = (struct rwreg_param*)param;
	
	
	if(prwreg==NULL)		return -1;

	ret = 0;
	if(set)
	{
		switch(prwreg->width)
		{
			case 1:
				printk("+mp_ioctl_write_reg_hdl - write8 \n");
				write_nic_byte(dev, prwreg->offset, (unsigned char)prwreg->value);	  	
				break;						
			case 2:
				printk("+mp_ioctl_write_reg_hdl - write16 \n");
		    		write_nic_word(dev, prwreg->offset, (unsigned short)prwreg->value);	
		  	  	break;												
	     	case 4:
				printk("+mp_ioctl_write_reg_hdl - write32 \n");
		   		write_nic_dword(dev, prwreg->offset, (unsigned int)prwreg->value);	  	
		  	   	break;
	     	default:
            	ret = -1;
			   	break;
		}
	}
	else
	{
		ret = -1;
	}

	if(ret!=-1){
		if(prwreg->width==1){
			printk("+mp_ioctl_write_reg_hdl addr:0x%08x byte:%d value:0x%02x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
		else if(prwreg->width==2){
			printk("+mp_ioctl_write_reg_hdl addr:0x%08x byte:%d value:0x%04x\n",prwreg->offset,prwreg->width,prwreg->value);
		}
		else if(prwreg->width==4){
			printk("+mp_ioctl_write_reg_hdl addr:0x%08x byte:%d value:0x%08x\n",prwreg->offset,prwreg->width,prwreg->value);
	       }
	}

	return ret;
}


int mp_ioctl_read_bbreg_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct bbreg_param *pbbreg = (struct bbreg_param*)param;
	
	if(pbbreg==NULL)
		return -1;

	if(set)
		return -1;	

#if 0
	write_nic_dword(dev, PhyAddr, pbbreg->offset);
	pbbreg->value = read_nic_byte(dev, PhyDataR);
#else
	
	pbbreg->value = rtl8192_QueryBBReg(dev, pbbreg->offset, bMaskDWord);
	
#endif

	printk("+mp_ioctl_read_bbreg_hdl offset:0x%04x, val=%08x \n", pbbreg->offset, pbbreg->value);

	return 0;
	
}

int mp_ioctl_write_bbreg_hdl(struct net_device *dev, unsigned char set, void *param)
{
	int ret = 0;

	struct bbreg_param*pbbreg = (struct bbreg_param*)param;

	if(pbbreg==NULL)
		return -1;

	if(!set)
		return -1;
	
#if 0
	write_nic_dword(dev, PhyAddr, pbbreg->value);
#else

	rtl8192_setBBreg(dev, pbbreg->offset, bMaskDWord, pbbreg->value);

#endif

	printk("+mp_ioctl_write_bbreg_hdl offset:0x%04x, val=%08x \n", pbbreg->offset, pbbreg->value);

	return ret;
	
}


int mp_ioctl_read_rfreg_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct rwreg_param* prwreg = (struct rwreg_param*)param;

	if(prwreg==NULL)
		return -1;
	if(set)
		return -1;

	printk("+mp_ioctl_read_rfreg_hdl offset:0x%02x \n",prwreg->offset);

	rtl8192_phy_QueryRFReg(	dev,
							(prwreg->width >> 4),
							(prwreg->offset & 0x0fff),
							bMaskDWord );

	return 0;
}

int mp_ioctl_write_rfreg_hdl(struct net_device *dev, unsigned char set, void *param)
{
	struct rwreg_param*prwreg = (struct rwreg_param*)param;

	printk("+mp_ioctl_write_rfreg_hdl offset:0x%02x value:0x%02x\n",prwreg->offset,prwreg->value);

	if(prwreg==NULL)
		return -1;
	if(set)
		return -1;

	rtl8192_phy_SetRFReg(	dev,
							(prwreg->offset >> 12),
							(prwreg->offset & 0x0fff),
							bMaskDWord, 
							(prwreg->value & 0x0fff) );

	return 0;
}


int mp_ioctl_read16_eeprom_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct eeprom_rw_param *peeprom16 = (struct eeprom_rw_param *)param;

	printk("mp_ioctl_read16_eeprom_hdl\n");
	if(peeprom16==NULL)
		return -1;

	if(set)
		return -1;

	return 0;
}

int mp_ioctl_write16_eeprom_hdl(struct net_device *dev, unsigned char set, void*param)
{
	struct eeprom_rw_param *peeprom16 = (struct eeprom_rw_param *)param;

	if(peeprom16==NULL)
		return -1;

	if(!set)
		return -1;


	return 0;
}

#endif

