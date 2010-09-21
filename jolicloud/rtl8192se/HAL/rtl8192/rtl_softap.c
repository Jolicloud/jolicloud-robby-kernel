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
#ifdef ASL  

#include "rtl_softap.h"
#include "rtl_core.h"
#include "rtl_wx.h"
#include "../../rtllib/rtllib.h"
#define TKIP_KEY_LEN 32
int needsendBeacon=0;

void ap_send_to_hostapd(struct rtllib_device *ieee, struct sk_buff *skb)
{

	printk("\nRomit : Inside send_to_hostapd.");
	ieee->apdev->last_rx = jiffies;
	skb->dev = ieee->apdev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
        skb_reset_mac_header(skb);
#else
        skb->mac.raw = skb->data;
#endif


	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = __constant_htons(ETH_P_802_2);
	memset(skb->cb, 0, sizeof(skb->cb));
	if (netif_rx(skb) != 0)
	{
		printk("\nnetif_rx failed");
		dev_kfree_skb_any(skb);
	}
	printk("\nExiting send_to_hostapd");
	return;
}
/*For softap's WPA.added by Lawrence.*/
int ap_stations_cryptlist_is_empty(struct rtllib_device *ieee)
{
	int i;
	
	for(i=1;i<APDEV_MAX_ASSOC;i++)
		if(ieee->stations_crypt[i]->used)
			return 0;
	return 1;	
}

extern int ap_cryptlist_find_station(struct rtllib_device *ieee,const u8 *addr,u8 justfind)
{
        int i;
        int ret=-1;
        for (i = 0; i < APDEV_MAX_ASSOC; i++) {
		if(memcmp(ieee->stations_crypt[i]->mac_addr,addr, ETH_ALEN) == 0)
			return i;       /*Find!!*/ 
	}
	if (!justfind) {
		for (i = 0; i < APDEV_MAX_ASSOC; i++) {
			   if (ieee->stations_crypt[i]->used ==0)
			   	return i; 
		}
	}
        if(ret == APDEV_MAX_ASSOC)/*No empty space.*/
                ret = -1;
        return ret;/*Not find,then return a empty space.*/
}
static u8 ap_is_stainfo_exist(struct rtllib_device *rtllib, u8 *addr)
{
	int k=0;
	struct sta_info * psta = NULL;
	u8 sta_idx = APDEV_MAX_ASSOC;
	
	for (k=0; k<APDEV_MAX_ASSOC; k++) {
		psta = rtllib->apdev_assoc_list[k];
		if (NULL != psta) {
			if (memcmp(addr, psta->macaddr, ETH_ALEN) == 0) {
				sta_idx = k;
				break;
			}
		}
	}
	return sta_idx;
}
struct sta_info *ap_get_stainfo(struct rtllib_device *ieee, u8 *addr)
{
	int k=0;
	struct sta_info * psta = NULL;
	struct sta_info * psta_find = NULL;
	
	for (k=0; k<APDEV_MAX_ASSOC; k++) {
		psta = ieee->apdev_assoc_list[k];
		if (NULL != psta) {
			if (memcmp(addr, psta->macaddr, ETH_ALEN) == 0) {
				psta_find = psta;
				break;
			}
		}
	}
	return psta_find;
}
static u8 ap_get_free_stainfo_idx(struct rtllib_device *rtllib, u8 *addr)
{
	int k = 0;
	while((rtllib->apdev_assoc_list[k] != NULL) && (k < APDEV_MAX_ASSOC))
		k++;
	return k;
}

static void ap_del_stainfo(struct rtllib_device *rtllib, u8 *addr)
{
	int idx = 0;
	struct sta_info * psta = NULL;
	
	for(idx = 0; idx < APDEV_MAX_ASSOC; idx++)
	{
		psta = rtllib->apdev_assoc_list[idx];
		if(psta != NULL) 
		{
 			if(memcmp(psta->macaddr, addr, ETH_ALEN) == 0)
 			{
#ifdef ASL_DEBUG_2
				printk("\nRemoved STA %x:%x:%x:%x:%x:%x!!!", psta->macaddr[0],psta->macaddr[1],psta->macaddr[2],psta->macaddr[3],psta->macaddr[4],psta->macaddr[5]);
#endif
				skb_queue_purge(&(psta->PsQueue));
				kfree(psta);
				rtllib->apdev_assoc_list[idx] = NULL;
			}
		}
	}
}
static void ap_set_del_sta_list(struct rtllib_device *rtllib, u8 *del_addr)
{
	int idx = 0;
	u8* addr;
	for (idx = 0; idx < APDEV_MAX_ASSOC; idx++) {
		if (rtllib->to_be_del_cam_entry[idx].isused) {
			addr = rtllib->to_be_del_cam_entry[idx].addr;
			if (memcmp(addr, del_addr, ETH_ALEN) == 0) {
				printk("%s(): addr "MAC_FMT" is exist in to_be_del_entry\n",__func__,MAC_ARG(del_addr));
				return;
			}	
		}
	}
	if (idx == APDEV_MAX_ASSOC) {
		for (idx = 0; idx < APDEV_MAX_ASSOC; idx++) {
			if (rtllib->to_be_del_cam_entry[idx].isused == false) {
				memcpy(rtllib->to_be_del_cam_entry[idx].addr, del_addr,ETH_ALEN);
				rtllib->to_be_del_cam_entry[idx].isused = true;
			}
		}
	}
}
static void ap_del_stainfo_list(struct rtllib_device *rtllib)
{
	int idx = 0;
	struct sta_info * psta = NULL;
	
	for(idx = 0; idx < APDEV_MAX_ASSOC; idx++)
	{
		psta = rtllib->apdev_assoc_list[idx];
		if(psta != NULL) 
		{
			kfree(psta);
			rtllib->apdev_assoc_list[idx] = NULL;
		}
	}
}
#ifdef RATE_ADAPTIVE_FOR_AP
void ap_init_stainfo(struct rtllib_device *ieeedev,int index)
{
	int idx = index;
	ieeedev->apdev_assoc_list[idx]->StaDataRate = 0;
	ieeedev->apdev_assoc_list[idx]->StaSS = 0;
	ieeedev->apdev_assoc_list[idx]->RetryFrameCnt = 0;
	ieeedev->apdev_assoc_list[idx]->NoRetryFrameCnt = 0;
	ieeedev->apdev_assoc_list[idx]->LastRetryCnt = 0;
	ieeedev->apdev_assoc_list[idx]->LastNoRetryCnt = 0;
	ieeedev->apdev_assoc_list[idx]->AvgRetryRate = 0;
	ieeedev->apdev_assoc_list[idx]->LastRetryRate = 0;
	ieeedev->apdev_assoc_list[idx]->txRateIndex = 11;
	ieeedev->apdev_assoc_list[idx]->APDataRate = 0x2; 
	ieeedev->apdev_assoc_list[idx]->ForcedDataRate = 0x2; 
	
}
void ap_init_rateadaptivestate(struct net_device *dev,struct sta_info  *pEntry)
{
	prate_adaptive	pRA = (prate_adaptive)&pEntry->rate_adaptive;

	pRA->ratr_state = DM_RATR_STA_MAX;
	pRA->PreRATRState = DM_RATR_STA_MAX;
	pEntry->rssi_stat.UndecoratedSmoothedPWDB = -1;

}
void ap_reset_stainfo(struct rtllib_device *rtllib,int index)
{
}

u8 ap_frame_retry(u16 fc)
{
	return (fc&0x0800)?1:0;
}


void ap_update_stainfo(struct rtllib_device * ieee,struct rtllib_hdr_4addr *hdr,struct rtllib_rx_stats *rx_stats)
{
	u8		*MacAddr = hdr->addr2 ;
	struct sta_info *pEntry;
	u16 fc;
	u16 supportRate[] = {10,20,55,110,60,90,120,180,240,360,480,540};
	
	u8 * ptmp = (u8*)hdr;
	int i;
	fc = le16_to_cpu(hdr->frame_ctl);

	pEntry = ap_get_stainfo(ieee, MacAddr);
	if(pEntry == NULL)
		return;
	else {
		if((pEntry->LastRetryCnt==pEntry->RetryFrameCnt) &&(pEntry->LastNoRetryCnt==pEntry->NoRetryFrameCnt)) {
			pEntry->RetryFrameCnt=0;
			pEntry->NoRetryFrameCnt=0;
		}
	
		pEntry->StaDataRate = supportRate[rx_stats->rate];
		pEntry->StaSS = rx_stats->SignalStrength;
		if(rx_stats->rate <= 11)
                        pEntry->txRateIndex = 11-rx_stats->rate;
		
		

		if(ap_frame_retry(fc))
			pEntry->RetryFrameCnt++;
		else
			pEntry->NoRetryFrameCnt++;
		
	}
}
u16 ap_sta_rate_adaptation(
	struct rtllib_device *ieee, 
	struct sta_info 	*pEntry, 
	bool			isForce
	)
{
	int retryRate=0, avgRetryRate=0, retryThreshold, avgRetryThreshold;
	u8 i, meanRateIdx=0, txRateIdx=0;
	u16 CurrentRetryCnt, CurrentNoRetryCnt;
	u16 supportRate[12] = {540,480,360,240,180,120,90,60,110,55,20,10};


	CurrentRetryCnt=pEntry->RetryFrameCnt;
	CurrentNoRetryCnt=pEntry->NoRetryFrameCnt;
	if((CurrentRetryCnt + CurrentNoRetryCnt) !=0)
		retryRate=(CurrentRetryCnt*100/(CurrentRetryCnt+CurrentNoRetryCnt));


	if(pEntry->AvgRetryRate!=0)
 		avgRetryRate=(retryRate+pEntry->AvgRetryRate)/2;
	else
		avgRetryRate=retryRate;

	pEntry->LastRetryCnt=CurrentRetryCnt;
	pEntry->LastNoRetryCnt=CurrentNoRetryCnt;
	
	if(isForce)
	{
		pEntry->AvgRetryRate=avgRetryRate;
		pEntry->LastRetryRate=retryRate;
		return pEntry->ForcedDataRate & 0x7F;
	}



	retryThreshold=40;
	avgRetryThreshold=40;


	/* The test of forcedDataRate indicates that 
	*/
	if(pEntry->StaSS >40)
	{
		meanRateIdx=0; 
		retryThreshold=50;
		avgRetryThreshold=50;
	}
	else if(pEntry->StaSS >27)
	{	
		meanRateIdx=1; 
	}else if(pEntry->StaSS >18)
	{	
		meanRateIdx=2; 
	}
	else
		meanRateIdx=3; 


	/*for(i=0, top=DATALEN_BKT0; i<DATALEN_BKTS; i++, top <<=DATALEN_BKTPOWER)
	{
		txRateIdx=i;
		if(PacketLen <= top) break;
	}*/

	if(pEntry->StaSS >18)
	{
		txRateIdx=pEntry->txRateIndex;

		if(retryRate>=retryThreshold)
		{
			if((retryRate-pEntry->LastRetryRate)>=retryThreshold)
			{
				if(txRateIdx==5) 
					txRateIdx=8;
				else if(txRateIdx < 11)
					txRateIdx++;
			}
		}else
		{
			if(txRateIdx ==8) 
				txRateIdx=5;
			else if(txRateIdx > 0)
				txRateIdx--;
		}
	
		if(txRateIdx > 11) txRateIdx=11;
		
		if(txRateIdx < meanRateIdx)
			txRateIdx=meanRateIdx;
	}
	else
	{
		for(i=0; i < 12; i++)
		{
			txRateIdx=i;
			if(supportRate[i]==pEntry->StaDataRate)
				break;
		}	
	}

	pEntry->APDataRate=supportRate[txRateIdx];
	pEntry->txRateIndex=txRateIdx;
	pEntry->AvgRetryRate=avgRetryRate;
	pEntry->LastRetryRate=retryRate;


	return pEntry->APDataRate;
}

void ap_set_dataframe_txrate( struct rtllib_device *ieee, struct sta_info *pEntry )
{
		u16 rate;
		if (ieee->ForcedDataRate == 0) {
		
			if (!(is_multicast_ether_addr(pEntry->macaddr) ||
				is_broadcast_ether_addr(pEntry->macaddr) )) { 
				
				if (pEntry != NULL) {
					bool bStaForceRate =0;
					rate = ap_sta_rate_adaptation(
							ieee, 
							pEntry, 
							bStaForceRate); 
					


				}
			}
		   	else {
                    	rate = ieee->rate;
			}
		} 
		else {
			u16 CurrentRetryCnt, CurrentNoRetryCnt;
			int retryRate=0, avgRetryRate=0;

			rate = (u8)(ieee->ForcedDataRate);

			if(pEntry != NULL){
				CurrentRetryCnt=pEntry->RetryFrameCnt;
				CurrentNoRetryCnt=pEntry->NoRetryFrameCnt;
				retryRate=(CurrentRetryCnt*100/(CurrentRetryCnt+CurrentNoRetryCnt));
				avgRetryRate=(retryRate+pEntry->AvgRetryRate)/2;

				pEntry->LastRetryCnt=CurrentRetryCnt;
				pEntry->LastNoRetryCnt=CurrentNoRetryCnt;
				pEntry->AvgRetryRate=avgRetryRate;
			}
		}

		ieee->rate = rate;
}

#endif
void ap_reset_admit_TRstream_rx(struct rtllib_device *rtllib, u8 *Addr)
{
	u8 	dir;
	bool				search_dir[4] = {0, 0, 0, 0};
	struct list_head*		psearch_list; 
	PTS_COMMON_INFO	pRet = NULL;
	PRX_TS_RECORD pRxTS = NULL;

	if(rtllib->iw_mode != IW_MODE_MASTER) 
		return;

	search_dir[DIR_UP] 	= true;
	search_dir[DIR_BI_DIR]	= true;
	psearch_list = &rtllib->Rx_TS_Admit_List;

	for(dir = 0; dir <= DIR_BI_DIR; dir++)
	{
		if(search_dir[dir] ==false )
			continue;
		list_for_each_entry(pRet, psearch_list, List){
			if ((memcmp(pRet->Addr, Addr, 6) == 0) && (pRet->TSpec.f.TSInfo.field.ucDirection == dir))
			{
				pRxTS = (PRX_TS_RECORD)pRet;
				pRxTS->RxIndicateSeq = 0xffff;
				pRxTS->RxTimeoutIndicateSeq = 0xffff;
			}
					
		}	
	}

	return;
}
void ap_ps_return_allqueuedpackets(struct rtllib_device *rtllib, u8 bMulticastOnly)
{
	int	i = 0;
	struct sta_info *pEntry = NULL;

	skb_queue_purge(&rtllib->GroupPsQueue);

	if (!bMulticastOnly) {
		for(i = 0; i < APDEV_MAX_ASSOC; i++)
		{
			pEntry = rtllib->apdev_assoc_list[i];

			if(pEntry != NULL)
			{
				skb_queue_purge(&rtllib->apdev_assoc_list[i]->PsQueue);
			}
		}
	}
}

int apdev_up(struct net_device *apdev,bool is_silent_reset)
{
	struct apdev_priv * appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	PRT_POWER_SAVE_CONTROL pPSC = (PRT_POWER_SAVE_CONTROL)(&(ieee->PowerSaveControl));
	bool init_status;
	struct net_device *dev = ieee->dev; 
	RT_TRACE(COMP_INIT, "==========>%s()\n", __FUNCTION__);
	printk("==========>%s()\n", __FUNCTION__);

	appriv->priv->up_first_time = 0;
	if (appriv->priv->apdev_up) {
		RT_TRACE(COMP_ERR,"%s():apdev is up,return\n",__FUNCTION__);
		return -1;
	}
	ieee->iw_mode = IW_MODE_MASTER;
#ifdef RTL8192SE        
        appriv->priv->ReceiveConfig =
		RCR_APPFCS | RCR_APWRMGT | /*RCR_ADD3 |*/
		RCR_AMF | RCR_ADF | RCR_APP_MIC | RCR_APP_ICV |
		RCR_AICV | RCR_ACRC32    |                               
		RCR_AB          | RCR_AM                |                               
		RCR_APM         |                                                               
		/*RCR_AAP               |*/                                                     
		RCR_APP_PHYST_STAFF | RCR_APP_PHYST_RXFF |      
		(appriv->priv->EarlyRxThreshold<<RCR_FIFO_OFFSET)       ;
#elif defined RTL8192CE
	appriv->priv->ReceiveConfig = (\
		RCR_APPFCS	
		| RCR_AMF | RCR_ADF| RCR_APP_MIC| RCR_APP_ICV
		| RCR_AICV | RCR_ACRC32 
		| RCR_AB | RCR_AM			
		| RCR_APM					
		| RCR_APP_PHYST_RXFF		
		| RCR_HTC_LOC_CTRL
		);
#else
        appriv->priv->ReceiveConfig = RCR_ADD3  |
		RCR_AMF | RCR_ADF |             
		RCR_AICV |                      
		RCR_AB | RCR_AM | RCR_APM |     
		RCR_AAP | ((u32)7<<RCR_MXDMA_OFFSET) |
		((u32)7 << RCR_FIFO_OFFSET) | RCR_ONLYERLPKT;

#endif

	if(!appriv->priv->up) {
		RT_TRACE(COMP_INIT, "Bringing up iface");
		printk("Bringing up iface");
		appriv->priv->bfirst_init = true;
		init_status = appriv->priv->ops->initialize_adapter(dev);
		if(init_status != true) {
			RT_TRACE(COMP_ERR,"ERR!!! %s(): initialization is failed!\n",__FUNCTION__);
			return -1;
		}
		RT_TRACE(COMP_INIT, "start adapter finished\n");
		RT_CLEAR_PS_LEVEL(pPSC, RT_RF_OFF_LEVL_HALT_NIC);
		printk("==>%s():priv->up is 0\n",__FUNCTION__);
		appriv->rtllib->ieee_up=1;		
		appriv->priv->bfirst_init = false;
#if defined RTL8192SE || defined RTL8192CE
		if(ieee->eRFPowerState!=eRfOn)
			MgntActSet_RF_State(dev, eRfOn, ieee->RfOffReason, true);	
#endif
#ifdef ENABLE_GPIO_RADIO_CTL
		if(appriv->priv->polling_timer_on == 0){
			check_rfctrl_gpio_timer((unsigned long)dev);
		}
#endif
		dm_InitRateAdaptiveMask(dev);
		watch_dog_timer_callback((unsigned long) dev);
	
	} else {
		rtllib_stop_scan(ieee);
		msleep(1000);
	}
	appriv->priv->apdev_up = 1;
	if(!is_silent_reset){
		appriv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
		printk("%s():set chan %d\n",__FUNCTION__,INIT_DEFAULT_CHAN);
		appriv->rtllib->set_chan(dev, INIT_DEFAULT_CHAN);
	}
	
	if(!ieee->proto_started) {
#ifndef CONFIG_MP
		rtllib_softmac_start_protocol(ieee, 0);
#endif
	}
	if(!netif_queue_stopped(apdev))
	    netif_start_queue(apdev);
	else
	    netif_wake_queue(apdev);

	rtllib_reset_queue(ieee);
	RT_TRACE(COMP_INIT, "<==========%s()\n", __FUNCTION__);
	return 0;
}
static int apdev_open(struct net_device *apdev)
{
	struct apdev_priv * appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct r8192_priv *priv = (void*)ieee->priv;
	int ret;
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = apdev_up(apdev, false);
	SEM_UP_PRIV_WX(&priv->wx_sem);	
	
	return ret;
}
int apdev_down(struct net_device *apdev)
{
	struct apdev_priv * appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct r8192_priv *priv = (void*)ieee->priv;
	struct net_device *dev = ieee->dev;
	unsigned long flags = 0;
	u8 RFInProgressTimeOut = 0;
	if(priv->apdev_up == 0) {
		RT_TRACE(COMP_ERR,"%s():ERR!!! apdev is already down\n",__FUNCTION__)
		return -1;
	}

	RT_TRACE(COMP_DOWN, "==========>%s()\n", __FUNCTION__);
	
	if (netif_running(apdev)) {
		netif_stop_queue(apdev);
	}
	if(!priv->up)
	{
		if(priv->rtllib->LedControlHandler)
			priv->rtllib->LedControlHandler(dev, LED_CTL_POWER_OFF);
#ifdef CONFIG_ASPM_OR_D3
	RT_ENABLE_ASPM(dev);
#endif
		printk("===>%s():priv->up is 0\n",__FUNCTION__);
        	priv->bDriverIsGoingToUnload = true;	
		ieee->ieee_up = 0;  
		priv->bfirst_after_down = 1;
		priv->rtllib->wpa_ie_len = 0;
		if(priv->rtllib->wpa_ie)
			kfree(priv->rtllib->wpa_ie);
		priv->rtllib->wpa_ie = NULL;
		CamResetAllEntry(dev);	
		memset(priv->rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*32);
#ifdef ASL
		if(priv->rtllib->iw_mode == IW_MODE_MASTER)
			ap_ps_return_allqueuedpackets(priv->rtllib, false);
#endif
		rtl8192_irq_disable(dev);
		rtl8192_cancel_deferred_work(priv);
#ifndef RTL8190P
		cancel_delayed_work(&priv->rtllib->hw_wakeup_wq);
#endif
		deinit_hal_dm(dev);
		del_timer_sync(&priv->watch_dog_timer);	
		rtllib_softmac_stop_protocol(ieee, 0, true);

		SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
		while(priv->RFChangeInProgress)
		{
			SPIN_UNLOCK_PRIV_RFPS(&priv->rf_ps_lock);
			if(RFInProgressTimeOut > 100)
			{
				SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
				break;
			}
			printk("===>%s():RF is in progress, need to wait until rf chang is done.\n",__FUNCTION__);
			mdelay(1);
			RFInProgressTimeOut ++;
			SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
		}
		priv->RFChangeInProgress = true;
		SPIN_UNLOCK_PRIV_RFPS(&priv->rf_ps_lock);
		priv->ops->stop_adapter(dev, false);
		SPIN_LOCK_PRIV_RFPS(&priv->rf_ps_lock);
		priv->RFChangeInProgress = false;
		SPIN_UNLOCK_PRIV_RFPS(&priv->rf_ps_lock);
		udelay(100);
		memset(&priv->rtllib->current_network, 0 , offsetof(struct rtllib_network, list));
		priv->rtllib->current_network.channel = INIT_DEFAULT_CHAN;
#ifdef CONFIG_ASPM_OR_D3
		RT_ENABLE_ASPM(dev);
#endif
	} else {
		printk("===>%s():priv->up is 1\n",__FUNCTION__);
		rtllib_softmac_stop_protocol(ieee, 0, true);
		ieee->iw_mode = IW_MODE_INFRA;
	}
	priv->apdev_up = 0;  
	RT_TRACE(COMP_DOWN, "<==========%s()\n", __FUNCTION__);

	return 0;
}

static int apdev_close(struct net_device *apdev)
{
	struct apdev_priv * appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device *ieee = appriv->rtllib;
	struct r8192_priv *priv = (void *)ieee->priv;
	int ret;
	
	SEM_DOWN_PRIV_WX(&priv->wx_sem);	
	ret = apdev_down(apdev);
	SEM_UP_PRIV_WX(&priv->wx_sem);	
	
	return ret;
}
static int apdev_tx(struct sk_buff *skb, struct net_device *apdev)
{
	struct apdev_priv *appriv = netdev_priv_rsl(apdev);
	struct rtllib_device *rtllib = (struct rtllib_device *)appriv->rtllib;
	struct net_device *dev = rtllib->dev;
	struct rtl_packet *rtl;
	struct rtl_packet *ret_rtl;
	struct rtllib_hdr_4addr * hdr = NULL;
	u8 zero_ether_header[14] = {0};
	bool is_hostapd_pkt = false;
	bool is_ioctl = false;
	u16 fc = 0;
	u8 type = 0, stype = 0;
	u8 *seq;
	void *key;
	int idx=0;
#ifdef ASL_WME
	struct wme_ac_parameter *ac;
	int aci=0;
	int i;
#endif
	rtl = (struct rtl_packet *)skb->data;
	ret_rtl = NULL;
	seq = NULL;
	key = NULL;	
	if (memcmp(rtl->ether_head, zero_ether_header, 14) == 0)
		is_hostapd_pkt = true;
	if (rtl->is_ioctl == 1)
		is_ioctl = true;
	if (is_hostapd_pkt) {
		if (is_ioctl) {
#ifdef ASL_DEBUG_2
		printk("\nGot an RTL packet!");
		printk("\n-------------------------rtl->id = %x----------------------------------",rtl->id);
		printk("\n-------------------------rtl->type = %x----------------------------------",rtl->type);
		printk("\n-------------------------rtl->subtype = %x----------------------------------",rtl->subtype);
#endif
			switch (rtl->type) {
				case RTL_CUSTOM_TYPE_WRITE:
					switch (rtl->subtype) {
						case RTL_CUSTOM_SUBTYPE_SET_RSNIE:
#ifdef ASL_DEBUG
							printk("\nActivating RSN!!!!!!!!!!!!");
#endif
							if (MAX_WPA_IE_LEN >= rtl->length) 	{
								memcpy(rtllib->rsn_ie, rtl->data, rtl->length);
								rtllib->rsn_ie_len = rtl->length;
								rtllib->rsn_activated = 1;
								printk("\nRSN IE Len=%d rsn_activted=1 now..",rtl->length);
							} else {
								printk("\nRSN IE Len=%d MAX WPA IE Len=%d",rtl->length, MAX_WPA_IE_LEN);
							}
							break;
						case RTL_CUSTOM_SUBTYPE_RESET_RSNIE:
#ifdef ASL_DEBUG
							printk("\nDeactivating RSN!!!!!!!!!!!!!!");
#endif
							rtllib->rsn_activated = 0;
							rtllib->rsn_ie_len = 0;
							break;
#ifdef ASL_WME
						case RTL_CUSTOM_SUBTYPE_SET_WME:
							printk("=======>%s(): RTL_CUSTOM_SUBTYPE_SET_WME\n",__FUNCTION__);
							rtllib->wme_enabled = 1;
							rtllib->current_network.qos_data.active = 1;
							ac = (struct wme_ac_parameter *)(rtl->data);
							for (i=0;i<4;i++) {	
	                           				aci = ac->aci;
								rtllib->current_network.qos_data.parameters.flag[aci] = ac->acm;
								rtllib->current_network.qos_data.parameters.cw_min[aci] = ac->eCWmin;
								rtllib->current_network.qos_data.parameters.cw_max[aci] = ac->eCWmax;
								rtllib->current_network.qos_data.parameters.tx_op_limit[aci] = le16_to_cpu(ac->txopLimit);
								ac++;
							}
							queue_work_rsl(appriv->priv->priv_wq, &appriv->priv->qos_activate);
							RT_TRACE (COMP_QOS, "QoS parameters change call "
									"qos_activate\n");
							printk("\nUpdate wme params OK.");
							break;
#endif
						case RTL_CUSTOM_SUBTYPE_SET_ASSOC:
						{
#ifdef ASL_DEBUG
							printk("\nAppending associated client to the list");
#endif
#ifdef ASL_DEBUG_2
							printk("\nCurrent assoc list : \n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p", rtllib->apdev_assoc_list[0], rtllib->apdev_assoc_list[1], rtllib->apdev_assoc_list[2],rtllib->apdev_assoc_list[3],rtllib->apdev_assoc_list[4],rtllib->apdev_assoc_list[5],rtllib->apdev_assoc_list[6],rtllib->apdev_assoc_list[7],rtllib->apdev_assoc_list[8],rtllib->apdev_assoc_list[9]);
#endif
							struct sta_info *new_sta = (struct sta_info *)rtl->data;
							int idx_exist = ap_is_stainfo_exist(rtllib, new_sta->macaddr);

							if (idx_exist >= APDEV_MAX_ASSOC) {
								idx = ap_get_free_stainfo_idx(rtllib, new_sta->macaddr);
							} else {
								printk("there has one entry in associate list, break!!\n");
								break;
							}
							if (idx >= APDEV_MAX_ASSOC - 1) {
								printk("\nBuffer overflow - could not append!!!");
								return -1;
							} else {
#ifdef ASL_DEBUG_2
								printk("\nSizeof struct sta_info = %d",sizeof(struct sta_info));
								printk("\nrtl->length = %d",rtl->length);
								printk("\nBefore alloction, apdev_assoc_list[idx] = %p",rtllib->apdev_assoc_list[idx]);
#endif
								rtllib->apdev_assoc_list[idx] = (struct sta_info *)kzalloc(sizeof(struct sta_info), GFP_ATOMIC);
								
#ifdef ASL_DEBUG_2
								printk("\nAfter allocation, apdev_assoc_list[idx] = %p", rtllib->apdev_assoc_list[idx]);
#endif
								memcpy(rtllib->apdev_assoc_list[idx], rtl->data, rtl->length);
								rtllib->apdev_assoc_list[idx]->bPowerSave = 0;
								skb_queue_head_init(&(rtllib->apdev_assoc_list[idx]->PsQueue));
								rtllib->apdev_assoc_list[idx]->LastActiveTime = jiffies;
#if 1
								printk("\nAppended!!!");
								printk("\nindex is %d\n",idx);
								printk("\nAid = %d",rtllib->apdev_assoc_list[idx]->aid);
								printk("\nAuthentication = %d",rtllib->apdev_assoc_list[idx]->authentication);
								printk("\nEncryption = %d",rtllib->apdev_assoc_list[idx]->encryption);
								printk("\nCapability = %d",rtllib->apdev_assoc_list[idx]->capability);
								printk("\nwme_enable = %d",rtllib->apdev_assoc_list[idx]->wme_enable);
								printk("\nsta->htinfo.AMSDU_MaxSize = %d\n",rtllib->apdev_assoc_list[idx]->htinfo.AMSDU_MaxSize);
#endif
#ifdef RATE_ADAPTIVE_FOR_AP
								ap_init_stainfo(rtllib, idx);
								ap_init_rateadaptivestate(rtllib,rtllib->apdev_assoc_list[idx]);
#endif
								printk("\nCurrent assoc list : \n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p", rtllib->apdev_assoc_list[0], rtllib->apdev_assoc_list[1], rtllib->apdev_assoc_list[2],rtllib->apdev_assoc_list[3],rtllib->apdev_assoc_list[4],rtllib->apdev_assoc_list[5],rtllib->apdev_assoc_list[6],rtllib->apdev_assoc_list[7],rtllib->apdev_assoc_list[8],rtllib->apdev_assoc_list[9]);
								queue_delayed_work_rsl(rtllib->wq, &rtllib->update_assoc_sta_info_wq, 0);

								ap_reset_admit_TRstream_rx(rtllib, rtllib->apdev_assoc_list[idx]->macaddr);
							}
							break;
						}
						case RTL_CUSTOM_SUBTYPE_CLEAR_ASSOC:
							printk("\nRemoving associated client "MAC_FMT" from the list!!!", MAC_ARG(rtl->data));
#ifdef ASL_DEBUG_2
							printk("\nCurrent assoc list : \n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p", rtllib->apdev_assoc_list[0], rtllib->apdev_assoc_list[1], rtllib->apdev_assoc_list[2],rtllib->apdev_assoc_list[3],rtllib->apdev_assoc_list[4],rtllib->apdev_assoc_list[5],rtllib->apdev_assoc_list[6],rtllib->apdev_assoc_list[7],rtllib->apdev_assoc_list[8],rtllib->apdev_assoc_list[9]);
#endif
							ap_del_stainfo(rtllib, rtl->data);

							ap_set_del_sta_list(rtllib, rtl->data);
							printk("\nAssociating "MAC_FMT"", MAC_ARG(rtl->data));
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)  
							queue_delayed_work_rsl(rtllib->wq, &rtllib->clear_sta_hw_sec_wq, 0);
#else
							schedule_task(&rtllib->clear_sta_hw_sec_wq);
#endif
#ifdef ASL_DEBUG_2
							printk("\nCurrent assoc list : \n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p\n%p", rtllib->apdev_assoc_list[0], rtllib->apdev_assoc_list[1], rtllib->apdev_assoc_list[2],rtllib->apdev_assoc_list[3],rtllib->apdev_assoc_list[4],rtllib->apdev_assoc_list[5],rtllib->apdev_assoc_list[6],rtllib->apdev_assoc_list[7],rtllib->apdev_assoc_list[8],rtllib->apdev_assoc_list[9]);
#endif
							break;
						default:
							break;
					}
					break;
				case RTL_CUSTOM_TYPE_READ:
#if 0
					switch (rtl->subtype) {
						case RTL_CUSTOM_SUBTYPE_STASC:
							printk("\nGetting STA SC!!!!!!!!");
							memcpy(&idx, rtl->data, sizeof(int));
							if (rtllib->crypt[idx]) {
								ret_rtl = (struct rtl_packet *)kmalloc(sizeof(struct rtl_packet)+TKIP_KEY_LEN+6, GFP_ATOMIC);
								ret_rtl->id = 0xffff;
								ret_rtl->type = RTL_CUSTOM_TYPE_READ;
#define TKIP_KEY_LEN 32
								ret_rtl->subtype = RTL_CUSTOM_SUBTYPE_STASC;
								ret_rtl->length = TKIP_KEY_LEN + 6;
								key = (void *)kmalloc(TKIP_KEY_LEN, GFP_ATOMIC);
								seq = (u8 *)kmalloc(6, GFP_ATOMIC);
								rtllib->crypt[idx]->ops->get_key(key, TKIP_KEY_LEN, seq, priv);
								printk("\nkey = %s",key);
								printk("\nSeq = %d %d %d %d %d %d", seq[0],seq[1],seq[2],seq[3],seq[4],seq[5]);
							} else {
								printk("\nCrypt not loaded.");
							}
							break;
						default:
							break;
					}
#endif
					break;
				default:
					break;
			}
		} else {
			struct sk_buff* new_skb = NULL;
			u8* tag = NULL;
			hdr = (struct rtllib_hdr_4addr *)skb->data;
			fc = le16_to_cpu(hdr->frame_ctl);
			type = WLAN_FC_GET_TYPE(fc);
			stype = WLAN_FC_GET_STYPE(fc);
			if (RTLLIB_FTYPE_MGMT == type) {
#ifdef ASL_DEBUG_2
				printk("\nGot a packet to transmit.");
				int i=0;
				printk("\n----------------PKG DUMP------------\n");	
				for (i=0; i<skb->len;i++) {
					printk("%x ",*(u8*)((u32)skb->data+i));
				}
				printk("\n---------------------------------------\n");	
#endif
				new_skb = dev_alloc_skb(skb->len-15);
				if (!new_skb) {
					printk("can't malloc new skb\n");
					return -ENOMEM;
				}
				tag = (u8*)skb_put(new_skb, skb->len-15);
				memcpy(tag, skb->data+15, skb->len-15);
				softmac_mgmt_xmit(new_skb,rtllib);
			} 
		}
	dev_kfree_skb_any(skb);
	}else {
#ifdef ASL_DEBUG_2
		bool bIsMulticast = false;
		u8 dest[6] = {0};
		memcpy(dest,skb->data,6);
		bIsMulticast = is_broadcast_ether_addr(dest) ||is_multicast_ether_addr(dest);
		if(!bIsMulticast) {
	    		printk("=============>%s(): send normal data pkt\n",__FUNCTION__);
			dump_buf(skb->data,skb->len);
		}
#endif
		rtllib_xmit_inter(skb, dev);
	}
			
	return 0;
}
static void apdev_tx_timeout(struct net_device *apdev)
{
	struct apdev_priv *appriv = (struct apdev_priv *)netdev_priv_rsl(apdev);
	struct rtllib_device * ieee = appriv->rtllib;
	struct net_device *dev = ieee->dev;

	printk("%s():Timeout transmitting !\n",__func__);
	rtl8192_tx_timeout(dev);	
	return;
}
struct net_device_stats *apdev_stats(struct net_device *apdev)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0) 
        return &((struct apdev_priv *)netdev_priv(apdev))->stats;
#else
	return &((struct apdev_priv *)apdev->priv)->stats;
#endif	
}
int rtl8192_ap_wpa_set_encryption(struct net_device *dev,struct ieee_param *ipw, int param_len)
{
	struct r8192_priv *priv = (struct r8192_priv *)rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	u32 key[4];
	u8 *addr = ipw->sta_addr;
	u8 broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	u8 entry_idx = 0;
	
	if (param_len != (int) ((char *) ipw->u.crypt.key - (char *) ipw) + ipw->u.crypt.key_len) 
	{
		printk("%s:Len mismatch %d, %d\n", __FUNCTION__, param_len, ipw->u.crypt.key_len);
		return -EINVAL;
	}

	if (ipw->u.crypt.set_tx) 
	{
#ifdef ASL_DEBUG_2
	    	printk("=============>%s():set_tx is 1\n",__FUNCTION__);
#endif
		if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
			ieee->pairwise_key_type = KEY_TYPE_CCMP;
		else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
			ieee->pairwise_key_type = KEY_TYPE_TKIP;
		else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
		{
			if (ipw->u.crypt.key_len == 13)
				ieee->pairwise_key_type = KEY_TYPE_WEP104;
			else if (ipw->u.crypt.key_len == 5)
				ieee->pairwise_key_type = KEY_TYPE_WEP40;
		}
		else 
			ieee->pairwise_key_type = KEY_TYPE_NA;

		if (ieee->pairwise_key_type)
		{
			entry_idx = rtl8192_get_free_hwsec_cam_entry(ieee, addr);
			if(entry_idx >=  TOTAL_CAM_ENTRY)
			{
				printk("%s: Can not find free hw security cam entry\n", __FUNCTION__);
				return -EINVAL;
			}
			memcpy((u8*)key, ipw->u.crypt.key, 16);
			EnableHWSecurityConfig8192(dev);
			set_swcam(dev,entry_idx,ipw->u.crypt.idx,ieee->pairwise_key_type,addr, 0, key, 0);
			set_swcam(dev,ipw->u.crypt.idx,ipw->u.crypt.idx,ieee->pairwise_key_type, addr, 0, key, 0);
			setKey(dev, entry_idx, ipw->u.crypt.idx, ieee->pairwise_key_type, addr, 0, key);
			setKey(dev, ipw->u.crypt.idx, ipw->u.crypt.idx, ieee->pairwise_key_type, addr, 0, key);
			
		}
	}
	
	else 
	{
#ifdef ASL_DEBUG_2
	    	printk("===>set_tx is 0\n");
#endif
		memcpy((u8*)key, ipw->u.crypt.key, 16);
		if (strcmp(ipw->u.crypt.alg, "CCMP") == 0)
			ieee->group_key_type= KEY_TYPE_CCMP;
		else if (strcmp(ipw->u.crypt.alg, "TKIP") == 0)
			ieee->group_key_type = KEY_TYPE_TKIP;
		else if (strcmp(ipw->u.crypt.alg, "WEP") == 0)
		{
			if (ipw->u.crypt.key_len == 13)
				ieee->group_key_type = KEY_TYPE_WEP104;
			else if (ipw->u.crypt.key_len == 5)
				ieee->group_key_type = KEY_TYPE_WEP40;
		}
		else
			ieee->group_key_type = KEY_TYPE_NA;
		if (ieee->group_key_type)
		{
			set_swcam(	dev, 
					ipw->u.crypt.idx,
					ipw->u.crypt.idx, 
					ieee->group_key_type,	
					broadcast_addr,	
					0,		
					key,
					0);
			setKey(	dev, 
					ipw->u.crypt.idx,
					ipw->u.crypt.idx, 
					ieee->group_key_type,	
					broadcast_addr,	
					0,		
					key);		
		}

	}
	return 0;
}
int pid_hostapd = 0;
int apdev_dequeue(spinlock_t *plock, APDEV_QUEUE *q, u8 *item, int *itemsize)
{
	unsigned long flags;

	RT_ASSERT_RET_VALUE(plock, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(q, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(item, (-E_WAPI_QNULL));
	printk("===========%s()\n",__FUNCTION__);
	if(APDEV_IsEmptyQueue(q))
		return -E_APDEV_QEMPTY;

 	spin_lock_irqsave(plock, flags);
	
	memcpy(item, q->ItemArray[q->Head].Item, q->ItemArray[q->Head].ItemSize);
	*itemsize = q->ItemArray[q->Head].ItemSize;
	q->NumItem--;
	if((q->Head+1) == q->MaxItem)
		q->Head = 0;
	else
		q->Head++;

	spin_unlock_irqrestore(plock, flags);
	printk("itemsize is %d\n",*itemsize);	
	return E_APDEV_OK;
}
static int apdev_ioctl(struct net_device *apdev, struct ifreq *ifr, int cmd)
{
	int ret;
	struct apdev_priv *priv;
	struct iwreq *wrq = (struct iwreq *)ifr;
	struct rtllib_device *rtllib;
	struct rtl_packet *rtl;
	u8 *seq;
	u8* key;
	int idx=0;
	struct rtl_seqnum *seqn;
	struct r8192_priv *priv_8192;
	struct ieee_param *ipw = NULL;
	struct iw_point *p = &wrq->u.data;
	static u8 QueueData[APDEV_MAX_BUFF_LEN];
	priv = netdev_priv_rsl(apdev);
	rtllib = (struct rtllib_device *)priv->rtllib;
	priv_8192 = rtllib_priv(rtllib->dev);
	ret = -1;
	rtl = NULL;
#define DATAQUEUE_EMPTY	"Queue is empty"
	switch (cmd) {
		case RTL_IOCTL_SHOSTAP:
#ifdef ASL_DEBUG_2
			printk("\nSetting hostap to 1");
#endif
			printk("\nStart hostap!!!");
			rtllib->hostap = 1;
			printk("\nCur_chan=%d ssid=%s ",rtllib->current_network.channel,rtllib->current_network.ssid);
			printk("bssid:%x:%x:%x:%x:%x:%x ", rtllib->current_network.bssid[0], rtllib->current_network.bssid[1], rtllib->current_network.bssid[2], rtllib->current_network.bssid[3],rtllib->current_network.bssid[4],rtllib->current_network.bssid[5]);
			printk("rates-len=%d ext_rates_len=%d\n", rtllib->current_network.rates_len, rtllib->current_network.rates_ex_len);
			HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20, HT_EXTCHNL_OFFSET_NO_EXT);  
		/*resume Send Beacon.added by lawrence.*/
			if(needsendBeacon){
				printk("====>%s()\n",__FUNCTION__);
				rtllib_start_send_beacons(rtllib);	
			}
			memset(rtllib->to_be_del_cam_entry, 0, sizeof(struct del_item));
			rtllib->PowerSaveStationNum = 0;
			ap_ps_return_allqueuedpackets(rtllib,false);
		
			ret = 0;
		break;
	   	case RTL_IOCTL_CHOSTAP:
#ifdef ASL_DEBUG_2
			printk("\nClear hostap to 0");
#endif
			printk("\nClear hostap to 0");
		/*Close ap.Remove list.*/
			ap_del_stainfo_list(rtllib);

		/*stop send Beacon.added by lawrence.*/
			printk("\nstop send Beacon.");
			rtllib_stop_send_beacons(rtllib);
			memset(rtllib->swcamtable,0,sizeof(SW_CAM_TABLE)*TOTAL_CAM_ENTRY);
			rtllib->hostap = 0;
			needsendBeacon = 1;/*resume send beacon*/
			ret = 0;
		break;
	    	case RTL_IOCTL_GHOSTAP:
		printk("\nGetting hostap");
			ret = 0;
		break;
		case RTL_IOCTL_SAVE_PID:
		{
			u8 strPID[10];
			u32 len = 0;
			int i = 0;

			printk("%s(): RTL_IOCTL_SAVE_PID!\n",__FUNCTION__);
			if (rtllib->hostap != 1){
				ret = -1;
				break;    
			}
			if ( !wrq->u.data.pointer ){
				ret = -1;
				break;
			}
			len = wrq->u.data.length;
			memset(strPID, 0, sizeof(strPID));
			if(copy_from_user(strPID, (void *)wrq->u.data.pointer, len)){
				ret = -1;
				break;
			}
			pid_hostapd = 0;
			for(i = 0; i < len; i++) {
				pid_hostapd = pid_hostapd * 10 + (strPID[i] - 48);
			}		
			printk("%s(): strPID=%s pid_wapi=%d!\n",__FUNCTION__, strPID, pid_hostapd);
			ret = 0;
			break;	
		}
		case RTL_IOCTL_DEQUEUE:
		{
			int QueueDataLen = 0;

			printk("%s(): RTL_IOCTL_DEQUEUE!\n",__FUNCTION__);
			if((ret = apdev_dequeue(&rtllib->apdev_queue_lock, rtllib->apdev_queue, QueueData, &QueueDataLen)) != 0){
			    	printk("======>empty queue\n");
				if(copy_to_user((void *)(wrq->u.data.pointer), DATAQUEUE_EMPTY, sizeof(DATAQUEUE_EMPTY))==0)
					wrq->u.data.length = sizeof(DATAQUEUE_EMPTY);
			}else{
			    	printk("======>have item in queue\n");
				printk("===>len is %d\n",QueueDataLen);
				if(copy_to_user((void *)wrq->u.data.pointer, (void *)QueueData, QueueDataLen)==0)
					wrq->u.data.length = QueueDataLen;
			}
			ret = 0;	
			break;
		}
		case RTL_IOCTL_GET_RFTYPE:
		{
			printk("===============>RTL_IOCTL_GET_RFTYPE: priv->rf_type is %d\n",priv_8192->rf_type);
			if (copy_to_user((void *)(wrq->u.data.pointer), (void* )&(priv_8192->rf_type), 1) == 0)
				wrq->u.data.length = 1;
			ret = 0;
			break;
		}
/*	    case RTL_IOCTL_SRSN:
		printk("\nActivating RSN!!!!!!!!!!!!");
		rtl =(struct rtl_packet *)kmalloc(wrq->u.data.length);
		copy_from_user(rtl, wrq->u.data.pointer, wr->u.data.length);
		rtllib->rsn_ie = (u8 *)kmalloc(rtl->length,GFP_ATOMIC);
		memcpy(rtllib->rsn_ie, rtl->data, rtl->length);
		rtllib->rsn_ie_len = rtl->length;
		rtllib->rsn_activated = 1;
		break;
	    case RTL_IOCTL_CRSN:
		printk("\nDeactivating RSN!!!!!!!!!!!!!!");
		rtllib->rsn_activated = 0;
		rtllib->rsn_ie_len = 0;
		kfree(rtllib->rsn_ie);
		break;*/
	    	case RTL_IOCTL_GSEQNUM:
#ifdef ASL_DEBUG_2
		printk("\nGetting Seq Num");
#endif
		printk("\nGetting Seq Num");
		if (!wrq->u.data.pointer) {
#ifdef ASL_DEBUG_2
			printk("\nwrq->u.data.pointer does not exits!!!");
#endif
			return -EINVAL;
		}
		#if 0
		if (wrq->u.data.length > sizeof(struct rtl_seqnum)) {
			printk("wrq->u.data.length > sizeof(struct rtl_seqnum), ERR\n");
			return ENOMEM;
		}
		#endif
		seqn = (struct rtl_seqnum* )kzalloc(sizeof(struct rtl_seqnum), GFP_KERNEL);
		seq = (u8 *)kzalloc(6, GFP_KERNEL);
		key = (u8 *)kzalloc(TKIP_KEY_LEN, GFP_KERNEL);
		ret = copy_from_user(seqn, wrq->u.data.pointer, sizeof(struct rtl_seqnum));
		idx = seqn->idx;
		/*if (idx)
			idx--;
		else
		idx = rtllib->tx_keyidx;*/
#ifdef ASL_DEBUG_2
		printk("\nIndex : %d",idx);
#endif
		if(rtllib->stations_crypt[0]->crypt[idx]){
			rtllib->stations_crypt[0]->crypt[idx]->ops->get_key(key, TKIP_KEY_LEN, seq, rtllib->stations_crypt[0]->crypt[idx]->priv);
#ifdef ASL_DEBUG_2
			printk("\nGot seqnum : %x%x%x%x%x%x", seq[0],seq[1],seq[2],seq[3],seq[4],seq[5]);
			printk("\nGot key:");
			for(i=0;i<32;i++)
			  printk("%x",(u32)key[i]);
			printk("wrq->u.data.length is %d\n",wrq->u.data.length);
#endif
			memcpy(seqn->seqnum, seq, 6);
			ret = copy_to_user(wrq->u.data.pointer, seqn, wrq->u.data.length);
			kfree(seq);
			kfree(seqn);
			kfree(key);
#ifdef ASL_DEBUG_2
			printk("\nGet Seq num DONE!!!");
#endif
			return 0;
		} else {
			printk("\nieee->crypt for the index does not exists!!!");
			return -EINVAL;
		}

#if 0

		if (rtllib->crypt[idx]) {
			rtllib->crypt[idx]->ops->get_key(key, TKIP_KEY_LEN, seq, rtllib->crypt[idx]->priv);
#ifdef ASL_DEBUG_2
			printk("\nGot seqnum : %x%x%x%x%x%x", seq[0],seq[1],seq[2],seq[3],seq[4],seq[5]);
			printk("\nGot key:");
			for(i=0;i<32;i++)
			  printk("%x",(u8*)key[i]);
#endif
			memcpy(seqn->seqnum, seq, 6);
			copy_to_user(wrq->u.data.pointer, seqn, wrq->u.data.length);
			kfree(seq);
			kfree(seqn);
			kfree(key);
#ifdef ASL_DEBUG_2
			printk("\nGet Seq num DONE!!!");
#endif
			return 0;
		} else {
			printk("\nieee->crypt for the index does not exists!!!");
			return -EINVAL;
		}
#endif
		
		break;
	    	case RTL_IOCTL_SHIDDENSSID:
			printk("\nSetting hidden SSID!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			rtllib->bhidden_ssid = 1;
			ret = 0;
		break;
		case RTL_IOCTL_CHIDDENSSID:
			printk("\nResetting hidden SSID!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			rtllib->bhidden_ssid = 0;
			ret = 0;
		break;
	    	case RTL_IOCTL_SMODE:
	    	{
			int mode;
			ret = copy_from_user(&mode, wrq->u.data.pointer, wrq->u.data.length);
			rtl8192_SetWirelessMode(rtllib->dev, (u8)mode);
			printk("\n=====>%s=====set mode %d\n",__FUNCTION__,rtllib->mode);
			
			if(rtllib->iw_mode == IW_MODE_MASTER)
			{
				priv_8192->ShortRetryLimit=7;
				priv_8192->LongRetryLimit=7;
			}
			priv_8192->ops->rtl819x_hwconfig(rtllib->dev);
			ret = 0;
		break;
	    	}
		case RTL_IOCTL_START_BN:
		{
			int bstart = false;
			ret = copy_from_user(&bstart, wrq->u.data.pointer, wrq->u.data.length);
			if (rtllib->mode == WIRELESS_MODE_B) {
				rtllib->current_network.rates_ex_len = 0;
			}
			if (rtllib->mode == WIRELESS_MODE_A) {
				rtllib->current_network.rates_len = 0;
			}
			if (bstart == true) {
				rtllib_stop_send_beacons(rtllib);
				rtllib_start_send_beacons(rtllib);
			}
			break;
		}
		case RTL_IOCTL_SHTCONF:
	    	{
		       rtl_ht_conf_t ht_conf;
			ret = copy_from_user(&ht_conf, wrq->u.data.pointer, wrq->u.data.length);
			printk("\n=====>%s=====set HTCONF\n", __FUNCTION__);
			
			rtllib->pHTInfo->bEnableHT = ht_conf.bEnableHT;
			rtllib->pHTInfo->bRegBW40MHz = ht_conf.bRegBW40MHz;
			rtllib->pHTInfo->bRegShortGI20MHz= ht_conf.bRegShortGI20MHz;
			rtllib->pHTInfo->bRegShortGI40MHz = ht_conf.bRegShortGI40MHz;
			rtllib->pHTInfo->bRegSuppCCK = ht_conf.bRegSuppCCK; 
			
			rtllib->pHTInfo->bAMSDU_Support = ht_conf.bAMSDU_Support; 
			rtllib->pHTInfo->nAMSDU_MaxSize = ht_conf.nAMSDU_MaxSize; 
			rtllib->pHTInfo->bAMPDUEnable = ht_conf.bAMPDUEnable; 
			rtllib->pHTInfo->AMPDU_Factor = ht_conf.AMPDU_Factor; 
			rtllib->pHTInfo->MPDU_Density = ht_conf.MPDU_Density; 
			rtllib->pHTInfo->ForcedAMSDUMode = ht_conf.ForcedAMSDUMode; 
			rtllib->pHTInfo->ForcedAMSDUMaxSize = ht_conf.ForcedAMSDUMaxSize; 
			
			rtllib->pHTInfo->SelfMimoPs = ht_conf.SelfMimoPs; 
			rtllib->pHTInfo->bRegRxReorderEnable = ht_conf.bRegRxReorderEnable; 
			rtllib->pHTInfo->RxReorderWinSize = ht_conf.RxReorderWinSize; 
			rtllib->pHTInfo->RxReorderPendingTime = ht_conf.RxReorderPendingTime; 

			rtllib->pHTInfo->bRegRT2RTAggregation = ht_conf.bRegRT2RTAggregation;
			
			memcpy(rtllib->Regdot11HTOperationalRateSet, ht_conf.Regdot11HTOperationalRateSet, 16);
			rtllib->pHTInfo->CurrentOpMode = ht_conf.OptMode; 
			HTUseDefaultSetting(rtllib);		
			dm_init_edca_turbo(rtllib->dev);
			if(rtllib->pHTInfo->bRegBW40MHz)
				HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20_40, (rtllib->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
			else
				HTSetConnectBwMode(rtllib, HT_CHANNEL_WIDTH_20, (rtllib->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
		
			ret = 0;
		break;
	    	}
           
	    	case RTL_IOCTL_STXBW:
	    	{
			rtllib->pHTInfo->bCurTxBW40MHz = *((u8*)(wrq->u.data.pointer)); 
			printk("\n!!!!!Set bCurTxBW40MHz to %d", rtllib->pHTInfo->bCurTxBW40MHz);
			ret = 0;
		break;
	    	}
	    	case RTL_IOCTL_WPA:
			printk("%s(): set inctl WPA\n",__FUNCTION__);
		   	if(rtllib->iw_mode == IW_MODE_MASTER) {
				if (p->length < sizeof(struct ieee_param) || !p->pointer){
				    	printk("===========>length is small or point is null\n");
					ret = -EINVAL;
					goto out;
				}

				ipw = (struct ieee_param *)kmalloc(p->length, GFP_KERNEL);
				if (ipw == NULL){
				    	printk("======>ipw malloc failed\n");
					ret = -ENOMEM;
					goto out;
				}
				if (copy_from_user(ipw, p->pointer, p->length)) {
				    	printk("===========>copy from user failed\n");
					kfree(ipw);
					ipw = NULL;
					return  -EFAULT;
			    	}		
#ifdef ASL_DEBUG_2
				printk("\nGoing to call ieee80211_wpa_suplicant_ioctl!");
#endif
				if(ipw->cmd == IEEE_CMD_SET_ENCRYPTION)
					ret = rtl8192_ap_wpa_set_encryption(rtllib->dev, ipw, p->length);

#ifdef ASL_DEBUG_2
				int i;
				printk("@@ wrq->u pointer = ");
				for(i=0;i<wrq->u.data.length;i++){
					if(i%10==0) printk("\n");
						printk( "%2x-", *(u8*)(wrq->u.data.pointer+i));
				}
				printk("\n");
#endif

				ret = rtllib_wpa_supplicant_ioctl(rtllib, &wrq->u.data,0);
			}
			kfree(ipw);
        		ipw = NULL;	
		break;

	    	default:
			ret = -EOPNOTSUPP;
		break;
	}
out:
	return ret;
}
void ap_ps_on_pspoll(struct rtllib_device *ieee, u16 fc, u8 *pSaddr, struct sta_info *pEntry)
{
	struct sk_buff * skb;
		
	if(pEntry == NULL)
		return;
	
	printk("In %s(): fc=0x%x addr="MAC_FMT"\n", __FUNCTION__, fc, MAC_ARG(pSaddr));
	if(WLAN_FC_GET_PWRMGT(fc) == false)
		return;


	if (skb_queue_len(&(pEntry->PsQueue))) {
		pcb_desc tcb_desc = NULL;
		skb = skb_dequeue(&(pEntry->PsQueue));
		if (skb) {
			tcb_desc = (pcb_desc)skb->cb;
			if( skb_queue_len(&(pEntry->PsQueue)))
				tcb_desc->bMoreData = true;

			rtllib_xmit_inter(skb, ieee->dev);
		}
	}

}
struct sta_info *ap_ps_update_station_psstate(
	struct rtllib_device *ieee,
	u8 *MacAddr,
	u8 bPowerSave
	)
{
	struct sta_info *pEntry;

	if(ieee->iw_mode != IW_MODE_MASTER)
		return NULL;

	pEntry = ap_get_stainfo(ieee, MacAddr);
	if(pEntry == NULL)
	{
		return NULL;
	}

	if(bPowerSave && !pEntry->bPowerSave)
	{ 
		printk("In %s(): addr="MAC_FMT" bPS=%d\n", __FUNCTION__,MAC_ARG(MacAddr),bPowerSave);
		pEntry->bPowerSave = true;
		ieee->PowerSaveStationNum++;
	}
	else if(!bPowerSave && pEntry->bPowerSave)
	{ 
		struct sk_buff *skb = NULL;
		int i, len;

		printk("In %s(): addr="MAC_FMT" bPS=%d\n", __FUNCTION__,MAC_ARG(MacAddr),bPowerSave);
		pEntry->bPowerSave = false;
		ieee->PowerSaveStationNum--;

		len = skb_queue_len(&pEntry->PsQueue);
		if(len > 0)
		{
			for(i=0;i<len;i++)
			{
				skb = skb_dequeue(&pEntry->PsQueue);
				if(NULL != skb)		
					rtllib_xmit_inter(skb, ieee->dev);
			}
		}
	}

	return pEntry;
}
/* Return type : 1 - src and dst found
		-1 - src not found, dst found
		-2 - src found, dst not found
		-3 - src and dst both not found */
int ap_is_data_frame_authorized(
	struct rtllib_device *ieee, 
	struct net_device *dev, 
	u8* src, 	u8* dst) 
{
	int i,src_found=0,dst_found=0;
	struct sta_info *sta=NULL;
#ifdef ASL_DEBUG
	printk("%s - Input source address : %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, 
			src[0], src[1], src[2], src[3], src[4], src[5]);
	printk("%s - Input destination address : %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, 
			dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);
#endif
	for (i = 0; i < APDEV_MAX_ASSOC; i++) {
		sta = (struct sta_info *)ieee->apdev_assoc_list[i];
		if(sta == NULL)
			continue;
#ifdef ASL_DEBUG
		printk("\n%s - Found address : %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, 
				sta->macaddr[0], sta->macaddr[1], sta->macaddr[2], sta->macaddr[3], 
				sta->macaddr[4], sta->macaddr[5]);
#endif
		if(memcmp(src, sta->macaddr, ETH_ALEN) == 0)
			src_found = 1;
		if((memcmp(dst, sta->macaddr, ETH_ALEN) == 0) || (memcmp(dst, dev->dev_addr, ETH_ALEN) == 0))
			dst_found = 1;
		if((dst[0] == 0xff) && (dst[1] == 0xff) && (dst[2] == 0xff) && (dst[3] == 0xff) && \
				(dst[4] == 0xff) && (dst[5] == 0xff)) {
			dst_found = 1;
		}
	
	}
	if(src_found) {
		if(dst_found)
			return 1;
		else
			return -2;
	} else {
		if(dst_found)
			return -1;
		else
			return -3;
	}
}
u16 ap_ps_fill_tim(struct rtllib_device *ieee)
{
	if (ieee->PowerSaveStationNum == 0) {
		ieee->Tim.Octet = (u8 *)ieee->TimBuf;
		ieee->Tim.Octet[0] = (u8)ieee->mDtimCount;
		ieee->Tim.Octet[1] = (u8)ieee->mDtimPeriod;
		ieee->Tim.Octet[2] = 0;
		ieee->Tim.Octet[3] = 0;

		ieee->Tim.Length = 4;
	} else {
		int i;
		int MaxAID=-1,MinAID=-1;
		u8 ByteOffset,BitOffset,BeginByte,EndByte;

		memset(ieee->TimBuf, 0, 254);

		if (skb_queue_len(&(ieee->GroupPsQueue))) {
			ieee->TimBuf[3]=0x01;
			MinAID = 0;
		}
		
		for (i = 0;i < APDEV_MAX_ASSOC; i++) {
			if (NULL != ieee->apdev_assoc_list[i]		&&
				ieee->apdev_assoc_list[i]->bPowerSave	&&
				skb_queue_len(&(ieee->apdev_assoc_list[i]->PsQueue))) {
				ByteOffset = ieee->apdev_assoc_list[i]->aid >> 3;
				BitOffset = ieee->apdev_assoc_list[i]->aid % 8;

				ieee->TimBuf[3 + ByteOffset] |= (1<<BitOffset);

				if(MaxAID == -1 || ieee->apdev_assoc_list[i]->aid > MaxAID)
					MaxAID = ieee->apdev_assoc_list[i]->aid;

				if(MinAID == -1 || ieee->apdev_assoc_list[i]->aid < MinAID)
					MinAID = ieee->apdev_assoc_list[i]->aid;
			}
		}

		if (MinAID == -1 || MaxAID == -1) {
			BeginByte = 0;
			EndByte = 0;
		} else {
			BeginByte = ((MinAID>>4)<<1);
			EndByte = (MaxAID>>3);
		}
		ieee->Tim.Octet = (u8 *)(&ieee->TimBuf[BeginByte]);

		ieee->Tim.Octet[0] = (u8)ieee->mDtimCount;
		ieee->Tim.Octet[1] = (u8)ieee->mDtimPeriod;
 		ieee->Tim.Octet[2] = BeginByte | (skb_queue_len(&(ieee->GroupPsQueue)) ? 1 : 0);

		ieee->Tim.Length = 4 + (EndByte-BeginByte);
		if(MaxAID>0)
		{
			printk("Beg=%d End=%d MaxAID=%d MinAID=%d\n",BeginByte,EndByte,MaxAID,MinAID);
			printk("********Tim Length %d*************\n", ieee->Tim.Length);
			for(i=0;i<ieee->Tim.Length;i++)
				printk("%x-", ieee->Tim.Octet[i]);
			printk("\n");
		}
	}
	return ieee->Tim.Length;
}
#ifdef ASL_WME

/* Add WME Parameter Element to Beacon and Probe Response frames. */
u8 * ap_eid_wme(struct rtllib_device *ieee,u8 *eid)
{
		
	u8 *pos = eid;
	struct wme_parameter_element *wme =
		(struct wme_parameter_element *) (pos + 2);
	int e;

	eid[0] = MFIE_TYPE_GENERIC;
	wme->oui[0] = 0x00;
	wme->oui[1] = 0x50;
	wme->oui[2] = 0xf2;
	wme->oui_type = WME_OUI_TYPE;
	wme->oui_subtype = WME_OUI_SUBTYPE_PARAMETER_ELEMENT;
	wme->version = WME_VERSION;
	wme->acInfo = ieee->parameter_set_count & 0xf;

	/* fill in a parameter set record for each AC */
	for (e = 0; e < 4; e++) {
		struct wme_ac_parameter *ac = &wme->ac[e];
		struct rtllib_qos_parameters *acp =&ieee->current_network.qos_data.parameters;

		ac->aifsn = acp->aifs[e];
		ac->acm = acp->flag[e];
		ac->aci = e;
		ac->reserved = 0;
		ac->eCWmin = acp->cw_min[e];
		ac->eCWmax = acp->cw_max[e];
		ac->txopLimit = le16_to_cpu(acp->tx_op_limit[e]);
	}

	pos = (u8 *) (wme + 1);
	eid[1] = pos - eid - 2; /* element length */

	return pos;
}

#endif
void ap_start_aprequest(struct net_device* dev)
{
#if 1
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;

	if (IS_NIC_DOWN(priv)) {
		RT_TRACE(COMP_ERR,"%s(): Driver is not up!!!\n",__FUNCTION__);
		return;
	}
	

	RT_TRACE(COMP_RESET, "===> ++++++ AP_StartApRequest() ++++++\n");

	
	

	ieee->state = RTLLIB_LINKED;
	ieee->link_change(ieee->dev);
	priv->ops->rtl819x_hwconfig(ieee->dev);
	notify_wx_assoc_event(ieee);
	
	/*When hostap setup,begin send beacon.By lawrence.*/
	rtllib_start_send_beacons(ieee);
	if (ieee->data_hard_resume)
		ieee->data_hard_resume(ieee->dev);
#ifdef ASL
	netif_carrier_on(ieee->apdev);
#else
	netif_carrier_on(ieee->dev);
#endif
	queue_work_rsl(priv->priv_wq, &priv->qos_activate);

	if(ieee->pHTInfo->bRegBW40MHz)
		HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20_40, (ieee->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
	else
		HTSetConnectBwMode(ieee, HT_CHANNEL_WIDTH_20, (ieee->current_network.channel<=6)?HT_EXTCHNL_OFFSET_UPPER:HT_EXTCHNL_OFFSET_LOWER);  
	
		
	RT_TRACE(COMP_RESET,"<=== ++++++ AP_StartApRequest() ++++++\n");
#endif
}
void ap_cam_restore_allentry(struct net_device* dev)
{
	u32 i;
	struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
	for( i = 0 ; i<	TOTAL_CAM_ENTRY; i++) {
		if (ieee->swcamtable[i].bused ) {
			setKey(dev,
					i,
					ieee->swcamtable[i].key_index,
					ieee->swcamtable[i].key_type,
					ieee->swcamtable[i].macaddr,
					ieee->swcamtable[i].useDK,
					(u32*)(&ieee->swcamtable[i].key_buf[0]));
		}
	}
	
}
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,20)
void clear_sta_hw_sec(struct work_struct * work)
{
	struct delayed_work *dwork = container_of(work,struct delayed_work,work);
	struct rtllib_device *ieee = container_of(dwork, struct rtllib_device, clear_sta_hw_sec_wq);
#else
void clear_sta_hw_sec(struct net_device *dev)
{
        struct r8192_priv *priv = rtllib_priv(dev);
	struct rtllib_device *ieee = priv->rtllib;
#endif
	int index = 0;
	for (index = 0; index < APDEV_MAX_ASSOC; index++) {
		if (ieee->to_be_del_cam_entry[index].isused) {
			rtl8192_del_hwsec_cam_entry(ieee, ieee->to_be_del_cam_entry[index].addr, false);
			memset(ieee->to_be_del_cam_entry[index].addr, 0, ETH_ALEN);
			ieee->to_be_del_cam_entry[index].isused = false;
		}
	}
}
#ifdef HAVE_NET_DEVICE_OPS
static const struct net_device_ops apdev_netdev_ops = {
	.ndo_open = apdev_open,
	.ndo_stop = apdev_close,
	.ndo_get_stats = apdev_stats,
	.ndo_tx_timeout = apdev_tx_timeout,
	.ndo_do_ioctl = apdev_ioctl,
	.ndo_start_xmit = apdev_tx,
};
#endif
void apdev_init_queue(APDEV_QUEUE * q, int szMaxItem, int szMaxData)
{
	RT_ASSERT_RET(q);
	
	q->Head = 0;
	q->Tail = 0;
	q->NumItem = 0;
	q->MaxItem = szMaxItem;
	q->MaxData = szMaxData;
}
int apdev_en_queue(spinlock_t *plock, APDEV_QUEUE *q, u8 *item, int itemsize)
{
	unsigned long flags;

	RT_ASSERT_RET_VALUE(plock, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(q, (-E_WAPI_QNULL));
	RT_ASSERT_RET_VALUE(item, (-E_WAPI_QNULL));
	
	if(APDEV_IsFullQueue(q))
		return -E_APDEV_QFULL;
	if(itemsize > q->MaxData)
		return -E_APDEV_ITEM_TOO_LARGE;

 	spin_lock_irqsave(plock, flags);

	q->ItemArray[q->Tail].ItemSize = itemsize;
	memset(q->ItemArray[q->Tail].Item, 0, sizeof(q->ItemArray[q->Tail].Item));
	memcpy(q->ItemArray[q->Tail].Item, item, itemsize);
	q->NumItem++;
	if((q->Tail+1) == q->MaxItem)
		q->Tail = 0;
	else
		q->Tail++;

	spin_unlock_irqrestore(plock, flags);
	
	return E_APDEV_OK;
}
void notify_hostapd(void)
{ 
	struct task_struct *p;
        
	if(pid_hostapd != 0){

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
		p = find_task_by_pid(pid_hostapd);
#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
                p = find_task_by_vpid(pid_hostapd);
#else
		p = pid_task(find_vpid(pid_hostapd), PIDTYPE_PID);
#endif
#endif
		if(p){
			send_sig(SIGUSR1,p,0); 
		}else {
			pid_hostapd = 0;
		}
	}
}
int apdev_create_event_send(struct rtllib_device *ieee, u8 *Buff, u16 BufLen)
{
	APDEV_EVENT_T *pEvent;
	u8 *pbuf = NULL;
	int ret = 0;
	
	printk("==========> %s\n", __FUNCTION__);
	
	RT_ASSERT_RET_VALUE(ieee, -1);
	RT_ASSERT_RET_VALUE(MacAddr, -1);

	pbuf= (u8 *)kmalloc((sizeof(APDEV_EVENT_T) + BufLen), GFP_ATOMIC);
	if(NULL == pbuf)
	    return -1;

	pEvent = (APDEV_EVENT_T *)pbuf;	
	pEvent->BuffLength = BufLen;
	if(BufLen > 0){
		memcpy(pEvent->Buff, Buff, BufLen);
	}

	ret = apdev_en_queue(&ieee->apdev_queue_lock, ieee->apdev_queue, pbuf, (sizeof(APDEV_EVENT_T) + BufLen));
	notify_hostapd();

	if(pbuf)
		kfree(pbuf);

	printk("<========== %s\n", __FUNCTION__);
	return ret;
}

void apdev_init(struct net_device* apdev)
{
	struct apdev_priv *appriv;
	ether_setup(apdev);
#ifdef HAVE_NET_DEVICE_OPS
	apdev->netdev_ops = &apdev_netdev_ops;
#else
	apdev->open = apdev_open;
	apdev->stop = apdev_close;
	apdev->hard_start_xmit = apdev_tx;
	apdev->do_ioctl = apdev_ioctl;
	apdev->get_stats = apdev_stats;
	apdev->tx_timeout = apdev_tx_timeout;
#endif
	apdev->wireless_handlers = &apdev_wx_handlers_def;
	apdev->watchdog_timeo = HZ*3;
	apdev->type = ARPHRD_ETHER;
	apdev->destructor = free_netdev;

	memset(apdev->broadcast, 0xFF, ETH_ALEN);


	appriv = netdev_priv_rsl(apdev);
	memset(appriv, 0, sizeof(struct apdev_priv));
	return;
}
#endif
