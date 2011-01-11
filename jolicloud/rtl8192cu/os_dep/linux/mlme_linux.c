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


#define _MLME_OSDEP_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <mlme_osdep.h>

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/smp_lock.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/circ_buf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <net/iw_handler.h>

/*
void sitesurvey_ctrl_handler(void *FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;

	_rtw_sitesurvey_ctrl_handler(adapter);

	_set_timer(&adapter->mlmepriv.sitesurveyctrl.sitesurvey_ctrl_timer, 3000);
}
*/

void rtw_join_timeout_handler (void *FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;
	_rtw_join_timeout_handler(adapter);
}


void _rtw_scan_timeout_handler (void *FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;
	rtw_scan_timeout_handler(adapter);
}


void _dynamic_check_timer_handlder (void *FunctionContext)
{
	_adapter *adapter = (_adapter *)FunctionContext;
		 
	dynamic_check_timer_handlder(adapter);	
	_set_timer(&adapter->mlmepriv.dynamic_chk_timer, 2000);
}


void rtw_init_mlme_timer(_adapter *padapter)
{
	struct	mlme_priv *pmlmepriv = &padapter->mlmepriv;

	_init_timer(&(pmlmepriv->assoc_timer), padapter->pnetdev, rtw_join_timeout_handler, (pmlmepriv->nic_hdl));
	//_init_timer(&(pmlmepriv->sitesurveyctrl.sitesurvey_ctrl_timer), padapter->pnetdev, sitesurvey_ctrl_handler, (u8 *)(pmlmepriv->nic_hdl));
	_init_timer(&(pmlmepriv->scan_to_timer), padapter->pnetdev, _rtw_scan_timeout_handler, (pmlmepriv->nic_hdl));

	_init_timer(&(pmlmepriv->dynamic_chk_timer), padapter->pnetdev, _dynamic_check_timer_handlder, (u8 *)(pmlmepriv->nic_hdl));

}

extern void rtw_indicate_wx_assoc_event(_adapter *padapter);
extern void rtw_indicate_wx_disassoc_event(_adapter *padapter);

void rtw_os_indicate_connect(_adapter *adapter)
{

_func_enter_;	

        rtw_indicate_wx_assoc_event(adapter);
	netif_carrier_on(adapter->pnetdev);

_func_exit_;	

}

static RT_PMKID_LIST   backupPMKIDList[ NUM_PMKID_CACHE ];
void rtw_os_indicate_disconnect( _adapter *adapter )
{
   //RT_PMKID_LIST   backupPMKIDList[ NUM_PMKID_CACHE ];
   u8              backupPMKIDIndex = 0;
   u8              backupTKIPCountermeasure = 0x00;
      
_func_enter_;

   rtw_indicate_wx_disassoc_event(adapter);	
   netif_carrier_off(adapter->pnetdev);
	
   if(adapter->securitypriv.dot11AuthAlgrthm == dot11AuthAlgrthm_8021X)//802.1x
   {		 
        // Added by Albert 2009/02/18
        // We have to backup the PMK information for WiFi PMK Caching test item.
        //
        // Backup the btkip_countermeasure information.
        // When the countermeasure is trigger, the driver have to disconnect with AP for 60 seconds.
        
        _rtw_memset( &backupPMKIDList[ 0 ], 0x00, sizeof( RT_PMKID_LIST ) * NUM_PMKID_CACHE );

        _rtw_memcpy( &backupPMKIDList[ 0 ], &adapter->securitypriv.PMKIDList[ 0 ], sizeof( RT_PMKID_LIST ) * NUM_PMKID_CACHE );
        backupPMKIDIndex = adapter->securitypriv.PMKIDIndex;
        backupTKIPCountermeasure = adapter->securitypriv.btkip_countermeasure;

       _rtw_memset((unsigned char *)&adapter->securitypriv, 0, sizeof (struct security_priv));
       _init_timer(&(adapter->securitypriv.tkip_timer),adapter->pnetdev, rtw_use_tkipkey_handler, adapter);

       // Added by Albert 2009/02/18
       // Restore the PMK information to securitypriv structure for the following connection.
       _rtw_memcpy( &adapter->securitypriv.PMKIDList[ 0 ], &backupPMKIDList[ 0 ], sizeof( RT_PMKID_LIST ) * NUM_PMKID_CACHE );
       adapter->securitypriv.PMKIDIndex = backupPMKIDIndex;
       adapter->securitypriv.btkip_countermeasure = backupTKIPCountermeasure;

	adapter->securitypriv.ndisauthtype = Ndis802_11AuthModeOpen;
	adapter->securitypriv.ndisencryptstatus = Ndis802_11WEPDisabled;

   }
   else //reset values in securitypriv 
   {
   	//if(adapter->mlmepriv.fw_state & WIFI_STATION_STATE)
   	//{
	struct security_priv *psec_priv=&adapter->securitypriv;

	psec_priv->dot11AuthAlgrthm =dot11AuthAlgrthm_Open;  //open system
	psec_priv->dot11PrivacyAlgrthm = _NO_PRIVACY_;
	psec_priv->dot11PrivacyKeyIndex = 0;

	psec_priv->dot118021XGrpPrivacy = _NO_PRIVACY_;
	psec_priv->dot118021XGrpKeyid = 1;

	psec_priv->ndisauthtype = Ndis802_11AuthModeOpen;
	psec_priv->ndisencryptstatus = Ndis802_11WEPDisabled;	
	psec_priv->wps_phase = _FALSE;
   	//}
   }

_func_exit_;

}

void rtw_report_sec_ie(_adapter *adapter,u8 authmode,u8 *sec_ie)
{
		uint len;
		u8 *buff,*p,i;
		union iwreq_data wrqu;

_func_enter_;

	RT_TRACE(_module_mlme_osdep_c_,_drv_info_,("+rtw_report_sec_ie, authmode=%d\n", authmode));

	buff = NULL;
	if(authmode==_WPA_IE_ID_)
	{
		RT_TRACE(_module_mlme_osdep_c_,_drv_info_,("rtw_report_sec_ie, authmode=%d\n", authmode));
		
		buff = _rtw_malloc(IW_CUSTOM_MAX);
		
		_rtw_memset(buff,0,IW_CUSTOM_MAX);
		
		p=buff;
		
		p+=sprintf(p,"ASSOCINFO(ReqIEs=");

		len = sec_ie[1]+2;
		len =  (len < IW_CUSTOM_MAX) ? len:IW_CUSTOM_MAX;
			
		for(i=0;i<len;i++){
			p+=sprintf(p,"%02x",sec_ie[i]);
		}

		p+=sprintf(p,")");
		
		_rtw_memset(&wrqu,0,sizeof(wrqu));
		
		wrqu.data.length=p-buff;
		
		wrqu.data.length = (wrqu.data.length<IW_CUSTOM_MAX) ? wrqu.data.length:IW_CUSTOM_MAX;
		
		wireless_send_event(adapter->pnetdev,IWEVCUSTOM,&wrqu,buff);

		if(buff)
		    _rtw_mfree(buff, IW_CUSTOM_MAX);
		
	}

_func_exit_;

}

void _survey_timer_hdl (void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	
	survey_timer_hdl(padapter);
}

void _link_timer_hdl (void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	link_timer_hdl(padapter);
}

void _addba_timer_hdl(void *FunctionContext)
{
	struct sta_info *psta = (struct sta_info *)FunctionContext;
	addba_timer_hdl(psta);
}

void init_addba_retry_timer(_adapter *padapter, struct sta_info *psta)
{

	_init_timer(&psta->addba_retry_timer, padapter->pnetdev, _addba_timer_hdl, psta);
}

/*
void _reauth_timer_hdl(void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	reauth_timer_hdl(padapter);
}

void _reassoc_timer_hdl(void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	reassoc_timer_hdl(padapter);
}
*/

void init_mlme_ext_timer(_adapter *padapter)
{	
	struct	mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;

	_init_timer(&pmlmeext->survey_timer, padapter->pnetdev, _survey_timer_hdl, padapter);
	_init_timer(&pmlmeext->link_timer, padapter->pnetdev, _link_timer_hdl, padapter);
	//_init_timer(&pmlmeext->ADDBA_timer, padapter->pnetdev, _addba_timer_hdl, padapter);

	//_init_timer(&pmlmeext->reauth_timer, padapter->pnetdev, _reauth_timer_hdl, padapter);
	//_init_timer(&pmlmeext->reassoc_timer, padapter->pnetdev, _reassoc_timer_hdl, padapter);
}


#ifdef CONFIG_HOSTAPD_MLME

static void mgnt_xmit_cb(struct urb *urb)
{	
	struct sk_buff *skb = (struct sk_buff *)urb->context;

	//printk("%s\n", __FUNCTION__);


	dev_kfree_skb_any(skb);
	
}

static int mgnt_xmit_entry(struct sk_buff *skb, struct net_device *pnetdev)
{
	u16 fc;
	int rc, len, pipe;	
	unsigned int bmcst, tid, qsel;
	struct sk_buff *pxmit_skb;
	struct urb *urb;
	unsigned char *pxmitbuf;
	struct tx_desc *ptxdesc;
	struct ieee80211_hdr *tx_hdr;
	struct hostapd_priv *phostapdpriv = netdev_priv(pnetdev);
	_adapter *padapter = (_adapter *)phostapdpriv->padapter;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	struct dvobj_priv *pdvobj = &padapter->dvobjpriv;	

	
	//printk("%s\n", __FUNCTION__);

	
	len = skb->len;
	tx_hdr = (struct ieee80211_hdr *)(skb->data);
	fc = le16_to_cpu(tx_hdr->frame_ctl);
	bmcst = IS_MCAST(tx_hdr->addr1);

	if ((fc & IEEE80211_FCTL_FTYPE) != IEEE80211_FTYPE_MGMT)
		goto _exit;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)) // http://www.mail-archive.com/netdev@vger.kernel.org/msg17214.html
	pxmit_skb = dev_alloc_skb(len + TXDESC_SIZE);			
#else			
	pxmit_skb = netdev_alloc_skb(pnetdev, len + TXDESC_SIZE);
#endif		

	if(!pxmit_skb)
		goto _exit;

	pxmitbuf = pxmit_skb->data;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		goto _exit;
	}

	// ----- fill tx desc -----	
	ptxdesc = (struct tx_desc *)pxmitbuf;	
	_rtw_memset(ptxdesc, 0, sizeof(*ptxdesc));
		
	//offset 0	
	ptxdesc->txdw0 |= cpu_to_le32(len&0x0000ffff); 
	ptxdesc->txdw0 |= cpu_to_le32(((TXDESC_SIZE+OFFSET_SZ)<<OFFSET_SHT)&0x00ff0000);//default = 32 bytes for TX Desc
	ptxdesc->txdw0 |= cpu_to_le32(OWN | FSG | LSG);

	if(bmcst)	
	{
		ptxdesc->txdw0 |= cpu_to_le32(BIT(24));
	}	

	//offset 4	
	ptxdesc->txdw1 |= cpu_to_le32(0x00);//MAC_ID

	ptxdesc->txdw1 |= cpu_to_le32((0x12<<QSEL_SHT)&0x00001f00);

	ptxdesc->txdw1 |= cpu_to_le32((0x06<< 16) & 0x000f0000);//b mode

	//offset 8			

	//offset 12		
	ptxdesc->txdw3 |= cpu_to_le32((le16_to_cpu(tx_hdr->seq_ctl)<<16)&0xffff0000);

	//offset 16		
	ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate
		
	//offset 20

	cal_txdesc_chksum(ptxdesc);
	// ----- end of fill tx desc -----

	//
	skb_put(pxmit_skb, len + TXDESC_SIZE);
	pxmitbuf = pxmitbuf + TXDESC_SIZE;
	_rtw_memcpy(pxmitbuf, skb->data, len);

	//printk("mgnt_xmit, len=%x\n", pxmit_skb->len);


	// ----- prepare urb for submit -----
	
	//translate DMA FIFO addr to pipehandle		
	//pipe = ffaddr2pipehdl(pdvobj, MGT_QUEUE_INX);
	pipe = usb_sndbulkpipe(pdvobj->pusbdev, pHalData->Queue2EPNum[(u8)MGT_QUEUE_INX]&0x0f);
	
	usb_fill_bulk_urb(urb, pdvobj->pusbdev, pipe,
			  pxmit_skb->data, pxmit_skb->len, mgnt_xmit_cb, pxmit_skb);
	
	urb->transfer_flags |= URB_ZERO_PACKET;
	usb_anchor_urb(urb, &phostapdpriv->anchored);
	rc = usb_submit_urb(urb, GFP_ATOMIC);
	if (rc < 0) {
		usb_unanchor_urb(urb);
		kfree_skb(skb);
	}
	usb_free_urb(urb);

	
_exit:	
	
	dev_kfree_skb_any(skb);

	return 0;
	
}

static int mgnt_netdev_open(struct net_device *pnetdev)
{
	struct hostapd_priv *phostapdpriv = netdev_priv(pnetdev);

	printk("mgnt_netdev_open: MAC Address:" MACSTR "\n", MAC2STR(pnetdev->dev_addr));


	init_usb_anchor(&phostapdpriv->anchored);
	
 	if(!netif_queue_stopped(pnetdev))
      		netif_start_queue(pnetdev);
	else
		netif_wake_queue(pnetdev);


	netif_carrier_on(pnetdev);
		
	//rtw_write16(phostapdpriv->padapter, 0x0116, 0x0100);//only excluding beacon 
		
	return 0;	
}
static int mgnt_netdev_close(struct net_device *pnetdev)
{
	struct hostapd_priv *phostapdpriv = netdev_priv(pnetdev);

	printk("%s\n", __FUNCTION__);

	usb_kill_anchored_urbs(&phostapdpriv->anchored);

	netif_carrier_off(pnetdev);

	if (!netif_queue_stopped(pnetdev))
		netif_stop_queue(pnetdev);
	
	//rtw_write16(phostapdpriv->padapter, 0x0116, 0x3f3f);
	
	return 0;	
}

#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,29))
static const struct net_device_ops rtl871x_mgnt_netdev_ops = {
	.ndo_open = mgnt_netdev_open,
       .ndo_stop = mgnt_netdev_close,
       .ndo_start_xmit = mgnt_xmit_entry,
       //.ndo_set_mac_address = r871x_net_set_mac_address,
       //.ndo_get_stats = r871x_net_get_stats,
       //.ndo_do_ioctl = r871x_mp_ioctl,
};
#endif

int hostapd_mode_init(_adapter *padapter)
{
	unsigned char mac[ETH_ALEN];
	struct hostapd_priv *phostapdpriv;
	struct net_device *pnetdev;
	
	pnetdev = alloc_etherdev(sizeof(struct hostapd_priv));	
	if (!pnetdev)
	   return -ENOMEM;

	//SET_MODULE_OWNER(pnetdev);
       ether_setup(pnetdev);

	//pnetdev->type = ARPHRD_IEEE80211;
	
	phostapdpriv = netdev_priv(pnetdev);
	phostapdpriv->pmgnt_netdev = pnetdev;
	phostapdpriv->padapter= padapter;
	padapter->phostapdpriv = phostapdpriv;
	
	//pnetdev->init = NULL;
	
#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,29))

	printk("register rtl871x_mgnt_netdev_ops to netdev_ops\n");

	pnetdev->netdev_ops = &rtl871x_mgnt_netdev_ops;
	
#else

	pnetdev->open = mgnt_netdev_open;

	pnetdev->stop = mgnt_netdev_close;	
	
	pnetdev->hard_start_xmit = mgnt_xmit_entry;
	
	//pnetdev->set_mac_address = r871x_net_set_mac_address;
	
	//pnetdev->get_stats = r871x_net_get_stats;

	//pnetdev->do_ioctl = r871x_mp_ioctl;
	
#endif

	pnetdev->watchdog_timeo = HZ; /* 1 second timeout */	

	//pnetdev->wireless_handlers = NULL;

#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_TX
	pnetdev->features |= NETIF_F_IP_CSUM;
#endif	

	
	
	if(dev_alloc_name(pnetdev,"mgnt.wlan%d") < 0)
	{
		printk("hostapd_mode_init(): dev_alloc_name, fail! \n");		
	}


	//SET_NETDEV_DEV(pnetdev, pintfpriv->udev);


	mac[0]=0x00;
	mac[1]=0xe0;
	mac[2]=0x4c;
	mac[3]=0x87;
	mac[4]=0x11;
	mac[5]=0x12;
				
	_rtw_memcpy(pnetdev->dev_addr, mac, ETH_ALEN);
	

	netif_carrier_off(pnetdev);


	/* Tell the network stack we exist */
	if (register_netdev(pnetdev) != 0)
	{
		printk("hostapd_mode_init(): register_netdev fail!\n");
		
		if(pnetdev)
      		{	 
			free_netdev(pnetdev);
      		}
	}
	
	return 0;
	
}

void hostapd_mode_unload(_adapter *padapter)
{
	struct hostapd_priv *phostapdpriv = padapter->phostapdpriv;
	struct net_device *pnetdev = phostapdpriv->pmgnt_netdev;

	unregister_netdev(pnetdev);
	free_netdev(pnetdev);
	
}

#endif

