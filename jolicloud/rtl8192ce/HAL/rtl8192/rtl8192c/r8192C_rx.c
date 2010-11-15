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

#define 	rx_hal_is_cck_rate(_pdesc)\
			(_pdesc->RxMCS == DESC92S_RATE1M ||\
			 _pdesc->RxMCS == DESC92S_RATE2M ||\
			 _pdesc->RxMCS == DESC92S_RATE5_5M ||\
			 _pdesc->RxMCS == DESC92S_RATE11M)
			 
u8 rtl8192ce_HwRateToMRate(bool bIsHT,	u8 rate)
{
	u8	ret_rate = 0x02;

	if(!bIsHT){
		switch(rate)
		{
		
			case DESC92S_RATE1M:		ret_rate = MGN_1M;		break;
			case DESC92S_RATE2M:		ret_rate = MGN_2M;		break;
			case DESC92S_RATE5_5M:	ret_rate = MGN_5_5M;		break;
			case DESC92S_RATE11M:	ret_rate = MGN_11M;		break;
			case DESC92S_RATE6M:		ret_rate = MGN_6M;		break;
			case DESC92S_RATE9M:		ret_rate = MGN_9M;		break;
			case DESC92S_RATE12M:	ret_rate = MGN_12M;		break;
			case DESC92S_RATE18M:	ret_rate = MGN_18M;		break;
			case DESC92S_RATE24M:	ret_rate = MGN_24M;		break;
			case DESC92S_RATE36M:	ret_rate = MGN_36M;		break;
			case DESC92S_RATE48M:	ret_rate = MGN_48M;		break;
			case DESC92S_RATE54M:	ret_rate = MGN_54M;		break;
		
			default:							
				ret_rate = 0xff;
				RTPRINT(FRX,INIT_IQK,("HwRateToMRate92S(): Non supported Rate [%x], bIsHT = %d!!!\n", rate, bIsHT));
			break;
		}
	}else{
		switch(rate)
		{
		

			case DESC92S_RATEMCS0:	ret_rate = MGN_MCS0;		break;
			case DESC92S_RATEMCS1:	ret_rate = MGN_MCS1;		break;
			case DESC92S_RATEMCS2:	ret_rate = MGN_MCS2;		break;
			case DESC92S_RATEMCS3:	ret_rate = MGN_MCS3;		break;
			case DESC92S_RATEMCS4:	ret_rate = MGN_MCS4;		break;
			case DESC92S_RATEMCS5:	ret_rate = MGN_MCS5;		break;
			case DESC92S_RATEMCS6:	ret_rate = MGN_MCS6;		break;
			case DESC92S_RATEMCS7:	ret_rate = MGN_MCS7;		break;
			case DESC92S_RATEMCS8:	ret_rate = MGN_MCS8;		break;
			case DESC92S_RATEMCS9:	ret_rate = MGN_MCS9;		break;
			case DESC92S_RATEMCS10:	ret_rate = MGN_MCS10;		break;
			case DESC92S_RATEMCS11:	ret_rate = MGN_MCS11;		break;
			case DESC92S_RATEMCS12:	ret_rate = MGN_MCS12;		break;
			case DESC92S_RATEMCS13:	ret_rate = MGN_MCS13;		break;
			case DESC92S_RATEMCS14:	ret_rate = MGN_MCS14;		break;
			case DESC92S_RATEMCS15:	ret_rate = MGN_MCS15;		break;
			case DESC92S_RATEMCS32:	ret_rate = (0x80|0x20);		break;


			default:							
				ret_rate = 0xff;
				RTPRINT(FRX, INIT_IQK, ("HwRateToMRate92S(): Non supported Rate [%x], bIsHT = %d!!!\n",rate, bIsHT));
			break;
		}

	}	
	return ret_rate;
}


void rtl8192ce_UpdateReceivedRateHistogramStatistics(
	struct net_device *dev,
	struct rtllib_rx_stats* pstats
	)
{
    	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
    	u32 rcvType=1;   
   	u32 rateIndex = 0;
    	u32 preamble_guardinterval;  

	if(pstats->bCRC)
		rcvType = 2;
	else if(pstats->bICV)
		rcvType = 3;
    
   	 if(pstats->bShortPreamble)
		preamble_guardinterval = 1;
	else
		preamble_guardinterval	= 0;

	priv->stats.received_preamble_GI[preamble_guardinterval][rateIndex]++;
	priv->stats.received_rate_histogram[0][rateIndex]++; 
	priv->stats.received_rate_histogram[rcvType][rateIndex]++;
	
}



void rtl8192ce_UpdateRxPktTimeStamp (struct net_device *dev, struct rtllib_rx_stats *stats)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);

	if(stats->bIsAMPDU && !stats->bFirstMPDU) {
		stats->mac_time[0] = priv->LastRxDescTSFLow;
		stats->mac_time[1] = priv->LastRxDescTSFHigh;
	} else {
		priv->LastRxDescTSFLow = stats->mac_time[0];
		priv->LastRxDescTSFHigh = stats->mac_time[1];
	}
}

long
rtl8192ce_signal_scale_mapping(struct r8192_priv * priv,
	long currsig
	)
{
	long retsig;

#if defined RTL8192SE || defined RTL8192CE
	if(priv->CustomerID == RT_CID_819x_Lenovo)
	{
		return currsig;
	}
	else if(priv->CustomerID == RT_CID_819x_Netcore)
	{	
		if(currsig >= 31 && currsig <= 100)
		{
			retsig = 100;
		}	
		else if(currsig >= 21 && currsig <= 30)
		{
			retsig = 90 + ((currsig - 20) / 1);
		}
		else if(currsig >= 11 && currsig <= 20)
		{
			retsig = 80 + ((currsig - 10) / 1);
		}
		else if(currsig >= 7 && currsig <= 10)
		{
			retsig = 69 + (currsig - 7);
		}
		else if(currsig == 6)
		{
			retsig = 54;
		}
		else if(currsig == 5)
		{
			retsig = 45;
		}
		else if(currsig == 4)
		{
			retsig = 36;
		}
		else if(currsig == 3)
		{
			retsig = 27;
		}
		else if(currsig == 2)
		{
			retsig = 18;
		}
		else if(currsig == 1)
		{
			retsig = 9;
		}
		else
		{
			retsig = currsig;
		}
		return retsig;
	}
#endif

	if(currsig >= 61 && currsig <= 100)
	{
		retsig = 90 + ((currsig - 60) / 4);
	}
	else if(currsig >= 41 && currsig <= 60)
	{
		retsig = 78 + ((currsig - 40) / 2);
	}
	else if(currsig >= 31 && currsig <= 40)
	{
		retsig = 66 + (currsig - 30);
	}
	else if(currsig >= 21 && currsig <= 30)
	{
		retsig = 54 + (currsig - 20);
	}
	else if(currsig >= 5 && currsig <= 20)
	{
		retsig = 42 + (((currsig - 5) * 2) / 3);
	}
	else if(currsig == 4)
	{
		retsig = 36;
	}
	else if(currsig == 3)
	{
		retsig = 27;
	}
	else if(currsig == 2)
	{
		retsig = 18;
	}
	else if(currsig == 1)
	{
		retsig = 9;
	}
	else
	{
		retsig = currsig;
	}
	
	return retsig;
}


			 
void rtl8192ce_query_rxphystatus(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pstats,
	prx_desc  pdesc,	
	prx_fwinfo   pDrvInfo,
	struct rtllib_rx_stats * precord_stats,
	bool bpacket_match_bssid,
	bool bpacket_toself,
	bool bPacketBeacon,
	bool bToSelfBA
	)
{
	bool is_cck_rate;
	phy_sts_cck_8192s_t* cck_buf;
	s8 rx_pwr_all=0, rx_pwr[4]; 
	u8 rf_rx_num=0, EVM, PWDB_ALL;
	u8 i, max_spatial_stream;
	u32 rssi, total_rssi = 0;

	is_cck_rate = rx_hal_is_cck_rate(pdesc);

	memset(precord_stats, 0, sizeof(struct rtllib_rx_stats));
	pstats->bPacketMatchBSSID = precord_stats->bPacketMatchBSSID = bpacket_match_bssid;
	pstats->bPacketToSelf = precord_stats->bPacketToSelf = bpacket_toself;
	pstats->bIsCCK = precord_stats->bIsCCK = is_cck_rate;
	pstats->bPacketBeacon = precord_stats->bPacketBeacon = bPacketBeacon;
	pstats->bToSelfBA = precord_stats->bToSelfBA = bToSelfBA;
	pstats->bIsCCK = precord_stats->bIsCCK = is_cck_rate;

	pstats->RxMIMOSignalQuality[0] = -1;
	pstats->RxMIMOSignalQuality[1] = -1;
	precord_stats->RxMIMOSignalQuality[0] = -1;
	precord_stats->RxMIMOSignalQuality[1] = -1;

	if (is_cck_rate){
		u8 report, cck_highpwr;
			
		cck_buf = (phy_sts_cck_8192s_t*)pDrvInfo;

		if(!priv->bInPowerSaveMode)
		cck_highpwr = (u8)rtl8192_QueryBBReg(priv->rtllib->dev, rFPGA0_XA_HSSIParameter2, BIT9);
		else
			cck_highpwr = false;
		if (!cck_highpwr){
			report = cck_buf->cck_agc_rpt & 0xc0;
			report = report >> 6;
			switch(report){
				case 0x3:
					rx_pwr_all = -46 - (cck_buf->cck_agc_rpt&0x3e);
					break;
				case 0x2:
					rx_pwr_all = -26 - (cck_buf->cck_agc_rpt&0x3e);
					break;
				case 0x1:
					rx_pwr_all = -12 - (cck_buf->cck_agc_rpt&0x3e);
					break;
				case 0x0:
					rx_pwr_all = 16 - (cck_buf->cck_agc_rpt&0x3e);
					break;
			}
		}
		else{
			report = pDrvInfo->cfosho[0] & 0x60;
			report = report >> 5;
			switch(report){
				case 0x3:
					rx_pwr_all = -46 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x2:
					rx_pwr_all = -26 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x1:
					rx_pwr_all = -12 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x0:
					rx_pwr_all = 16 - ((cck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
			}
		}

		PWDB_ALL= rtl819x_query_rxpwrpercentage(rx_pwr_all);
		pstats->RxPWDBAll = precord_stats->RxPWDBAll = PWDB_ALL;
		pstats->RecvSignalPower = rx_pwr_all;

		if (bpacket_match_bssid){
			u8 sq;
			if (pstats->RxPWDBAll > 40)
				sq = 100;
			else{
				sq = cck_buf->sq_rpt;
				if (sq > 64)
					sq = 0;
				else if(sq < 20)
					sq = 100;
				else
					sq = ((64-sq)*100)/44;
			}
			pstats->SignalQuality = precord_stats->SignalQuality = sq;
			pstats->RxMIMOSignalQuality[0] = precord_stats->RxMIMOSignalQuality[0] = sq;
			pstats->RxMIMOSignalQuality[1] = precord_stats->RxMIMOSignalQuality[1] = -1;
		}
	}
	else{
		priv->brfpath_rxenable[0] = priv->brfpath_rxenable[1] = true;

		for (i=RF90_PATH_A; i<RF90_PATH_MAX; i++){
			if (priv->brfpath_rxenable[i])
				rf_rx_num ++;

			rx_pwr[i] = ((pDrvInfo->gain_trsw[i]&0x3f)*2) - 110;
			rssi = rtl819x_query_rxpwrpercentage(rx_pwr[i]);
			total_rssi += rssi;

			priv->stats.rxSNRdB[i] = (long)(pDrvInfo->rxsnr[i]/2);

			if (bpacket_match_bssid){
				pstats->RxMIMOSignalStrength[i] = (u8)rssi;
				precord_stats->RxMIMOSignalStrength [i] = (u8)rssi;
			}
		}

		rx_pwr_all = ((pDrvInfo->pwdb_all >> 1) & 0x7f) - 110;
		PWDB_ALL = rtl819x_query_rxpwrpercentage(rx_pwr_all);

		pstats->RxPWDBAll = precord_stats->RxPWDBAll = PWDB_ALL;
		pstats->RxPower = precord_stats->RxPower = rx_pwr_all;
		pstats->RecvSignalPower = precord_stats->RecvSignalPower = rx_pwr_all;

		if (pdesc->RxHT && pdesc->RxMCS >= DESC92S_RATEMCS8 && pdesc->RxMCS <= DESC92S_RATEMCS15)
			max_spatial_stream = 2;
		else
			max_spatial_stream = 1;

		for(i=0; i<max_spatial_stream; i++){
			EVM = rtl819x_evm_dbtopercentage(pDrvInfo->rxevm[i]);

			if (bpacket_match_bssid)
			{
				if (i==0)
					pstats->SignalQuality = 
					precord_stats->SignalQuality = (u8)(EVM&0xff);
				pstats->RxMIMOSignalQuality[i] =
				precord_stats->RxMIMOSignalQuality[i] = (u8)(EVM&0xff);
			}
		}
#if 0
		if (pdesc->BW)
			priv->stats.received_bwtype[1+pDrvInfo->rxsc]++;
		else
			priv->stats.received_bwtype[0]++;
#endif
	}

	if (is_cck_rate)
		pstats->SignalStrength = 
		precord_stats->SignalStrength = (u8)(rtl8192ce_signal_scale_mapping(priv,PWDB_ALL));
	else
		if (rf_rx_num != 0)
			pstats->SignalStrength = 
			precord_stats->SignalStrength = (u8)(rtl8192ce_signal_scale_mapping(priv,total_rssi/=rf_rx_num));

}

void rtl8192ce_update_rxsignalstatistics(
	struct r8192_priv * priv,
	struct rtllib_rx_stats * pstats
	)
{
	int	weighting = 0;

	if(priv->stats.recv_signal_power == 0)
		priv->stats.recv_signal_power = pstats->RecvSignalPower;

	if(pstats->RecvSignalPower > priv->stats.recv_signal_power)
		weighting = 5;
	else if(pstats->RecvSignalPower < priv->stats.recv_signal_power)
		weighting = (-5);
	priv->stats.recv_signal_power = (priv->stats.recv_signal_power * 5 + pstats->RecvSignalPower + weighting) / 6;

}	

void Process_UI_RSSI_8192ce(struct r8192_priv * priv,struct rtllib_rx_stats * pstats)
{
	u8			rfPath;
	u32			last_rssi, tmpVal;

	if(pstats->bPacketToSelf || pstats->bPacketBeacon)
	{
		priv->stats.RssiCalculateCnt++;	
		if(priv->stats.ui_rssi.TotalNum++ >= PHY_RSSI_SLID_WIN_MAX)
		{
			priv->stats.ui_rssi.TotalNum = PHY_RSSI_SLID_WIN_MAX;
			last_rssi = priv->stats.ui_rssi.elements[priv->stats.ui_rssi.index];
			priv->stats.ui_rssi.TotalVal -= last_rssi;
		}
		priv->stats.ui_rssi.TotalVal += pstats->SignalStrength;
	
		priv->stats.ui_rssi.elements[priv->stats.ui_rssi.index++] = pstats->SignalStrength;
		if(priv->stats.ui_rssi.index >= PHY_RSSI_SLID_WIN_MAX)
			priv->stats.ui_rssi.index = 0;
	
		tmpVal = priv->stats.ui_rssi.TotalVal/priv->stats.ui_rssi.TotalNum;
		priv->stats.signal_strength = rtl819x_translate_todbm(priv, (u8)tmpVal);

	}

	if(!pstats->bIsCCK && pstats->bPacketToSelf)
	{
		for (rfPath = RF90_PATH_A; rfPath < priv->NumTotalRFPath; rfPath++)
		{
			if (!rtl8192_phy_CheckIsLegalRFPath(priv->rtllib->dev, rfPath))
				continue;
			RT_TRACE(COMP_DBG, "pstats->RxMIMOSignalStrength[%d]  = %d \n", rfPath, pstats->RxMIMOSignalStrength[rfPath] );

			if(priv->stats.rx_rssi_percentage[rfPath] == 0)	
			{
				priv->stats.rx_rssi_percentage[rfPath] = pstats->RxMIMOSignalStrength[rfPath];
			}
			
			if(pstats->RxMIMOSignalStrength[rfPath]  > priv->stats.rx_rssi_percentage[rfPath])
			{
				priv->stats.rx_rssi_percentage[rfPath] = 
					( (priv->stats.rx_rssi_percentage[rfPath]*(Rx_Smooth_Factor-1)) + 
					(pstats->RxMIMOSignalStrength[rfPath])) /(Rx_Smooth_Factor);
				priv->stats.rx_rssi_percentage[rfPath] = priv->stats.rx_rssi_percentage[rfPath] + 1;
			}
			else
			{
				priv->stats.rx_rssi_percentage[rfPath] = 
					( (priv->stats.rx_rssi_percentage[rfPath]*(Rx_Smooth_Factor-1)) + 
					(pstats->RxMIMOSignalStrength[rfPath])) /(Rx_Smooth_Factor);
			}
			RT_TRACE(COMP_DBG, "priv->stats.rx_rssi_percentage[%d]  = %d \n",rfPath, priv->stats.rx_rssi_percentage[rfPath] );
		}
	}
	
}	

void Process_PWDB_8192ce(struct r8192_priv * priv,struct rtllib_rx_stats * pstats,struct rtllib_network* pnet, struct sta_info *pEntry)
{
	long	UndecoratedSmoothedPWDB=0;

#ifdef _RTL8192_EXT_PATCH_
	if(priv->rtllib->iw_mode == IW_MODE_MESH){
		if(pnet){
			if(priv->mshobj->ext_patch_get_peermp_rssi_param)
				UndecoratedSmoothedPWDB = priv->mshobj->ext_patch_get_peermp_rssi_param(pnet);
		}else
			UndecoratedSmoothedPWDB = priv->undecorated_smoothed_pwdb;
	}
	else 
#endif
	if(priv->rtllib->iw_mode == IW_MODE_ADHOC){
		if(pEntry){
			UndecoratedSmoothedPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;
		}
	}else
		UndecoratedSmoothedPWDB = priv->undecorated_smoothed_pwdb;

	if(pstats->bPacketToSelf || pstats->bPacketBeacon){
		
		priv->RSSI_sum += pstats->RxPWDBAll;
		priv->RSSI_cnt++;
		
		if(UndecoratedSmoothedPWDB < 0){	
			UndecoratedSmoothedPWDB = pstats->RxPWDBAll;
		}
		
		if(pstats->RxPWDBAll > (u32)UndecoratedSmoothedPWDB){
			UndecoratedSmoothedPWDB = 	
					( ((UndecoratedSmoothedPWDB)*(Rx_Smooth_Factor-1)) + 
					(pstats->RxPWDBAll)) /(Rx_Smooth_Factor);
			UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB + 1;
		}else{
			UndecoratedSmoothedPWDB = 	
					( ((UndecoratedSmoothedPWDB)*(Rx_Smooth_Factor-1)) + 
					(pstats->RxPWDBAll)) /(Rx_Smooth_Factor);
		}

#ifdef _RTL8192_EXT_PATCH_
		if(priv->rtllib->iw_mode == IW_MODE_MESH){
			if(pnet){
				if(priv->mshobj->ext_patch_set_peermp_rssi_param)
					priv->mshobj->ext_patch_set_peermp_rssi_param(pnet,UndecoratedSmoothedPWDB);
			}else
				priv->undecorated_smoothed_pwdb = UndecoratedSmoothedPWDB;
		}else 
#endif
		if(priv->rtllib->iw_mode == IW_MODE_ADHOC){
			if(pEntry){
				pEntry->rssi_stat.UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB;
			}
		}else{
			priv->undecorated_smoothed_pwdb = UndecoratedSmoothedPWDB;
		}
		rtl8192ce_update_rxsignalstatistics(priv, pstats);
	}
}

void Process_UiLinkQuality8192ce(struct r8192_priv * priv,struct rtllib_rx_stats * pstats)
{
	u32	last_evm=0, nSpatialStream, tmpVal;

	if(pstats->SignalQuality != 0)	
	{
		if (pstats->bPacketToSelf || pstats->bPacketBeacon)
		{
			if(priv->stats.ui_link_quality.TotalNum++ >= PHY_LINKQUALITY_SLID_WIN_MAX)
			{
				priv->stats.ui_link_quality.TotalNum = PHY_LINKQUALITY_SLID_WIN_MAX;
				last_evm = priv->stats.ui_link_quality.elements[priv->stats.ui_link_quality.index];
				priv->stats.ui_link_quality.TotalVal -= last_evm;
			}
			priv->stats.ui_link_quality.TotalVal += pstats->SignalQuality;

			priv->stats.ui_link_quality.elements[priv->stats.ui_link_quality.index++] = pstats->SignalQuality;
			if(priv->stats.ui_link_quality.index >= PHY_LINKQUALITY_SLID_WIN_MAX)
				priv->stats.ui_link_quality.index = 0;


			tmpVal = priv->stats.ui_link_quality.TotalVal/priv->stats.ui_link_quality.TotalNum;
			priv->stats.signal_quality = tmpVal;
			priv->stats.last_signal_strength_inpercent = tmpVal;
		
			for(nSpatialStream = 0; nSpatialStream<2 ; nSpatialStream++)	
			{
				if(pstats->RxMIMOSignalQuality[nSpatialStream] != -1)
				{
					if(priv->stats.rx_evm_percentage[nSpatialStream] == 0)	
					{
						priv->stats.rx_evm_percentage[nSpatialStream] = pstats->RxMIMOSignalQuality[nSpatialStream];
					}
					priv->stats.rx_evm_percentage[nSpatialStream] = 
					( (priv->stats.rx_evm_percentage[nSpatialStream]*(Rx_Smooth_Factor-1)) + 
					(pstats->RxMIMOSignalQuality[nSpatialStream]* 1)) /(Rx_Smooth_Factor);
				}
			}
		}
	}
	else
		;
	
}	


void rtl8192ce_process_phyinfo(struct r8192_priv * priv, u8* buffer,struct rtllib_rx_stats * pcurrent_stats,struct rtllib_network * pnet, struct sta_info *pEntry)
{
	if(!pcurrent_stats->bPacketMatchBSSID)
		return;

	Process_UI_RSSI_8192ce(priv, pcurrent_stats);

	Process_PWDB_8192ce(priv, pcurrent_stats, pnet, pEntry);

	Process_UiLinkQuality8192ce(priv, pcurrent_stats);	
}

void rtl8192ce_TranslateRxSignalStuff(struct net_device *dev, 
        struct sk_buff *skb,
        struct rtllib_rx_stats * pstats,
        prx_desc pdesc,	
        prx_fwinfo pdrvinfo)
{
	

    	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
    	bool	bPacketMatchBSSID, bPacketToSelf;
    	bool bPacketBeacon=false;
    	struct rtllib_hdr_3addr *hdr;
	struct rtllib_network* pnet=NULL;
	u8	*psaddr;
	struct sta_info *pEntry = NULL;
    	bool bToSelfBA=false;
    	static struct rtllib_rx_stats  previous_stats;
    	u16 fc,type;

    	u8* 	tmp_buf;
    	u8*	praddr;

	
   	 tmp_buf = skb->data + pstats->RxDrvInfoSize + pstats->RxBufShift;

    	hdr = (struct rtllib_hdr_3addr *)tmp_buf;
    	fc = le16_to_cpu(hdr->frame_ctl);
    	type = WLAN_FC_GET_TYPE(fc);	
    	praddr = hdr->addr1;


	psaddr = hdr->addr2;
	if(priv->rtllib->iw_mode == IW_MODE_ADHOC){
		pEntry = GetStaInfo(priv->rtllib, psaddr);
	}	

#ifdef _RTL8192_EXT_PATCH_
		if((priv->rtllib->iw_mode == IW_MODE_MESH) && (priv->mshobj->ext_patch_get_mpinfo))
			pnet = priv->mshobj->ext_patch_get_mpinfo(dev,psaddr);	


		/* Check if the received packet is acceptabe. */
	bPacketMatchBSSID = ((RTLLIB_FTYPE_CTL != type) && (!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV));
		if(pnet){
			bPacketMatchBSSID = bPacketMatchBSSID;
		}else{
			bPacketMatchBSSID = bPacketMatchBSSID &&
					(!compare_ether_addr(priv->rtllib->current_network.bssid, 
					(fc & RTLLIB_FCTL_TODS)? hdr->addr1 : 
					(fc & RTLLIB_FCTL_FROMDS )? hdr->addr2 : hdr->addr3));
		}
#else	
    		bPacketMatchBSSID = ((RTLLIB_FTYPE_CTL != type) &&
            		                (!compare_ether_addr(priv->rtllib->current_network.bssid,	
		                        (fc & RTLLIB_FCTL_TODS)? hdr->addr1 : 
		       			(fc & RTLLIB_FCTL_FROMDS )? hdr->addr2 : hdr->addr3)) &&
            				(!pstats->bHwError) && (!pstats->bCRC)&& (!pstats->bICV));		
#endif
		bPacketToSelf =  bPacketMatchBSSID & (!compare_ether_addr(praddr, priv->rtllib->dev->dev_addr));

		if(WLAN_FC_GET_FRAMETYPE(fc)== RTLLIB_STYPE_BEACON){
       	 		bPacketBeacon = true;
    		}

    	if(WLAN_FC_GET_FRAMETYPE(fc) == RTLLIB_STYPE_BLOCKACK){
       			if ((!compare_ether_addr(praddr,dev->dev_addr)))
            			bToSelfBA = true;
    		}

    		if(bPacketMatchBSSID)
    		{
        		priv->stats.numpacket_matchbssid++;
    		}
    		if(bPacketToSelf){
        		priv->stats.numpacket_toself++;
    		}
			
		    		rtl8192ce_query_rxphystatus(priv, pstats, pdesc, pdrvinfo, &previous_stats, 
    		bPacketMatchBSSID,bPacketToSelf ,bPacketBeacon, bToSelfBA);

		rtl8192ce_process_phyinfo(priv, tmp_buf,pstats,pnet, pEntry);
	
}


bool rtl8192ce_rx_query_status_desc(struct net_device* dev, struct rtllib_rx_stats*  stats, rx_desc *pdesc, struct sk_buff* skb)
{
	struct r8192_priv* priv = rtllib_priv(dev);
	rx_fwinfo*		pDrvInfo;
	
	u32	PHYStatus		= GET_RX_DESC_PHYST(pdesc);
	stats->Length 		= (u16)GET_RX_DESC_PKT_LEN(pdesc);
	stats->RxDrvInfoSize 	= (u8)GET_RX_DESC_DRV_INFO_SIZE(pdesc)*RX_DRV_INFO_SIZE_UNIT; 
	stats->RxBufShift 		= (u8)(GET_RX_DESC_SHIFT(pdesc)&0x03);      
	stats->bICV 			= (u16)GET_RX_DESC_ICV(pdesc);
	stats->bCRC 			= (u16)GET_RX_DESC_CRC32(pdesc);
	stats->bHwError 		= (stats->bCRC|stats->bICV);
	stats->Decrypted 		= !GET_RX_DESC_SWDEC(pdesc); 
	stats->rate 		= (u8)GET_RX_DESC_RXMCS(pdesc);
	stats->bShortPreamble	= (u16)GET_RX_DESC_SPLCP(pdesc);
	stats->bIsAMPDU 		= (bool)(GET_RX_DESC_PAGGR(pdesc) == 1);
	stats->bIsAMPDU 		= (bool)((GET_RX_DESC_PAGGR(pdesc) == 1 )
								&& (GET_RX_DESC_FAGGR(pdesc) == 1));
	stats->TimeStampLow 	= GET_RX_DESC_TSFL(pdesc);
	stats->RxIs40MHzPacket= (bool)GET_RX_DESC_BW(pdesc);

	if(IS_UNDER_11N_AES_MODE(priv->rtllib))
	{
		if(stats->bICV && !stats->bCRC)
		{
			stats->bICV = false;
			stats->bHwError = false;
		}
	}

	{
		if(stats->Length > 0x2000 || stats->Length < 24)
		{
		
			RTPRINT(FRX, INIT_IQK, ("Err RX data pkt len = %x\n", stats->Length));
			stats->bHwError |= 1;
		}
	}

	rtl8192ce_UpdateReceivedRateHistogramStatistics(dev, stats);

	if(!stats->bHwError)
		stats->rate = rtl8192ce_HwRateToMRate((bool)GET_RX_DESC_RXHT(pdesc), (u8)GET_RX_DESC_RXMCS(pdesc));
	else
		stats->rate = MGN_1M;


#ifdef MERGE_TO_DO
#endif
	rtl8192ce_UpdateRxPktTimeStamp(dev, stats);	

	if((stats->RxBufShift + stats->RxDrvInfoSize) > 0)
		stats->bShift = 1;	

	if (PHYStatus == true)
	{
		pDrvInfo = (rx_fwinfo*)(skb->data + stats->RxBufShift);
			
		rtl8192ce_TranslateRxSignalStuff(dev, skb, stats, pdesc, pDrvInfo);

	}
	return true;
	
}



