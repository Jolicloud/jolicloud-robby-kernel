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
#define _RTL871X_MLME_C_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


#ifdef PLATFORM_LINUX
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
#endif


#include <recv_osdep.h>
#include <xmit_osdep.h>
#include <hal_init.h>
#include <mlme_osdep.h>
#include <sta_info.h>
#include <wifi.h>
#include <wlan_bssdef.h>

sint	_rtw_init_mlme_priv (_adapter* padapter)
{
	sint	i;
	u8	*pbuf;
	struct wlan_network	*pnetwork;
	struct	mlme_priv *pmlmepriv = &padapter->mlmepriv;
	sint	res=_SUCCESS;

_func_enter_;

	_rtw_memset((u8 *)pmlmepriv, 0, sizeof(struct mlme_priv));
	pmlmepriv->nic_hdl = (u8 *)padapter;

	pmlmepriv->pscanned = NULL;
	pmlmepriv->fw_state = 0;
	pmlmepriv->cur_network.network.InfrastructureMode = Ndis802_11AutoUnknown;
	pmlmepriv->passive_mode=1;// 1: active, 0: pasive. Maybe someday we should rename this varable to "active_mode" (Jeff)

	_rtw_spinlock_init(&(pmlmepriv->lock));	
	_rtw_init_queue(&(pmlmepriv->free_bss_pool));
	_rtw_init_queue(&(pmlmepriv->scanned_queue));

	set_scanned_network_val(pmlmepriv, 0);
	
	_rtw_memset(&pmlmepriv->assoc_ssid,0,sizeof(NDIS_802_11_SSID));

	pbuf = _rtw_malloc(MAX_BSS_CNT * (sizeof(struct wlan_network)));
	
	if (pbuf == NULL){
		res=_FAIL;
		goto exit;
	}
	pmlmepriv->free_bss_buf = pbuf;
		
	pnetwork = (struct wlan_network *)pbuf;
	
	for(i = 0; i < MAX_BSS_CNT; i++)
	{		
		_rtw_init_listhead(&(pnetwork->list));

		rtw_list_insert_tail(&(pnetwork->list), &(pmlmepriv->free_bss_pool.queue));

		pnetwork++;
	}

	//pbuf = _rtw_malloc(sizeof(struct sitesurvey_ctrl));

	pmlmepriv->sitesurveyctrl.last_rx_pkts=0;
	pmlmepriv->sitesurveyctrl.last_tx_pkts=0;
	pmlmepriv->sitesurveyctrl.traffic_busy=_FALSE;
	
	//allocate DMA-able/Non-Page memory for cmd_buf and rsp_buf

	rtw_init_mlme_timer(padapter);

exit:

_func_exit_;

	return res;
}	

static void mfree_mlme_priv_lock (struct mlme_priv *pmlmepriv)
{
	_rtw_spinlock_free(&pmlmepriv->lock);
	_rtw_spinlock_free(&(pmlmepriv->free_bss_pool.lock));
	_rtw_spinlock_free(&(pmlmepriv->scanned_queue.lock));
}

static void _free_mlme_priv (struct mlme_priv *pmlmepriv)
{
_func_enter_;

	if(pmlmepriv){
		mfree_mlme_priv_lock (pmlmepriv);

		if (pmlmepriv->free_bss_buf)
			_rtw_mfree(pmlmepriv->free_bss_buf, MAX_BSS_CNT * sizeof(struct wlan_network));
	}
_func_exit_;	
}

sint	_rtw_enqueue_network(_queue *queue, struct wlan_network *pnetwork)
{
	_irqL irqL;

_func_enter_;	

	if (pnetwork == NULL)
		goto exit;
	
	_enter_critical(&queue->lock, &irqL);

	rtw_list_insert_tail(&pnetwork->list, &queue->queue);

	_exit_critical(&queue->lock, &irqL);

exit:	

_func_exit_;		

	return _SUCCESS;
}

struct	wlan_network *_rtw_dequeue_network(_queue *queue)
{
	_irqL irqL;

	struct wlan_network *pnetwork;

_func_enter_;	

	_enter_critical(&queue->lock, &irqL);

	if (_rtw_queue_empty(queue) == _TRUE)

		pnetwork = NULL;
	
	else
	{
		pnetwork = LIST_CONTAINOR(get_next(&queue->queue), struct wlan_network, list);
		
		list_delete(&(pnetwork->list));
	}
	
	_exit_critical(&queue->lock, &irqL);

_func_exit_;		

	return pnetwork;
}

struct	wlan_network *_rtw_alloc_network(struct	mlme_priv *pmlmepriv )//(_queue *free_queue)
{
	_irqL	irqL;
	struct	wlan_network	*pnetwork;	
	_queue *free_queue = &pmlmepriv->free_bss_pool;
	_list* plist = NULL;
	
_func_enter_;	

	_enter_critical(&free_queue->lock, &irqL);
	
	if (_rtw_queue_empty(free_queue) == _TRUE) {
		pnetwork=NULL;
		goto exit;
	}
	plist = get_next(&(free_queue->queue));
	
	pnetwork = LIST_CONTAINOR(plist , struct wlan_network, list);
	
	list_delete(&pnetwork->list);
	
	RT_TRACE(_module_rtl871x_mlme_c_, _drv_info_, ("_rtw_alloc_network: ptr=%p\n", plist));
	
	pnetwork->last_scanned = rtw_get_current_time();
	pmlmepriv->num_of_scanned ++;
	
	_exit_critical(&free_queue->lock, &irqL);
exit:	

_func_exit_;		

	return pnetwork;	
}

void _rtw_free_network(struct	mlme_priv *pmlmepriv ,struct wlan_network *pnetwork,u8 isfreeall)
{
	u32 curr_time, delta_time;
	_irqL irqL;	
	_queue *free_queue = &(pmlmepriv->free_bss_pool);
	
_func_enter_;		

	if (pnetwork == NULL)
		goto exit;

	if (pnetwork->fixed == _TRUE)
		goto exit;

	curr_time = rtw_get_current_time();	
	if(!isfreeall)
	{
#ifdef PLATFORM_WINDOWS

	delta_time = (curr_time -pnetwork->last_scanned)/10;

	if(delta_time  < SCANQUEUE_LIFETIME*1000000)// unit:usec
	{
		goto exit;
	}
	
#endif

#ifdef PLATFORM_LINUX

	delta_time = (curr_time -pnetwork->last_scanned)/HZ;	

	if(delta_time < SCANQUEUE_LIFETIME)// unit:sec
	{		
		goto exit;
	}
	
#endif
	}

	_enter_critical(&free_queue->lock, &irqL);
	
	list_delete(&(pnetwork->list));

	rtw_list_insert_tail(&(pnetwork->list),&(free_queue->queue));
		
	pmlmepriv->num_of_scanned --;
	

	//DBG_871X("_rtw_free_network:SSID=%s\n", pnetwork->network.Ssid.Ssid);
	
	_exit_critical(&free_queue->lock, &irqL);
	
exit:		
	
_func_exit_;			

}

static void _free_network_nolock(struct	mlme_priv *pmlmepriv, struct wlan_network *pnetwork)
{

	_queue *free_queue = &(pmlmepriv->free_bss_pool);

_func_enter_;		

	if (pnetwork == NULL)
		goto exit;

	if (pnetwork->fixed == _TRUE)
		goto exit;

	//_enter_critical(&free_queue->lock, &irqL);
	
	list_delete(&(pnetwork->list));

	rtw_list_insert_tail(&(pnetwork->list), get_list_head(free_queue));
		
	pmlmepriv->num_of_scanned --;
	
	//_exit_critical(&free_queue->lock, &irqL);
	
exit:		

_func_exit_;			

}


/*
	return the wlan_network with the matching addr

	Shall be calle under atomic context... to avoid possible racing condition...
*/
struct wlan_network *_rtw_find_network(_queue *scanned_queue, u8 *addr)
{

	_irqL irqL;
	_list	*phead, *plist;
	struct	wlan_network *pnetwork = NULL;
	u8 zero_addr[ETH_ALEN] = {0,0,0,0,0,0};
	
_func_enter_;	

	if(_rtw_memcmp(zero_addr, addr, ETH_ALEN)){
		pnetwork=NULL;
		goto exit;
		}
	
	_enter_critical(&scanned_queue->lock, &irqL);
	
	phead = get_list_head(scanned_queue);
	plist = get_next(phead);
	 
	while (plist != phead)
       {
                pnetwork = LIST_CONTAINOR(plist, struct wlan_network ,list);
                plist = get_next(plist);
		if (_rtw_memcmp(addr, pnetwork->network.MacAddress, ETH_ALEN) == _TRUE)
                        break;
        }

	_exit_critical(&scanned_queue->lock, &irqL);
	
exit:		
	
_func_exit_;		

	return pnetwork;
	
}


void _rtw_free_network_queue(_adapter *padapter,u8 isfreeall)
{
	_irqL irqL;
	_list *phead, *plist;
	struct wlan_network *pnetwork;
	struct mlme_priv* pmlmepriv = &padapter->mlmepriv;
	_queue *scanned_queue = &pmlmepriv->scanned_queue;
	_queue	*free_queue = &pmlmepriv->free_bss_pool;
	u8 *mybssid = get_bssid(pmlmepriv);

_func_enter_;	
	

	_enter_critical(&scanned_queue->lock, &irqL);

	phead = get_list_head(scanned_queue);
	plist = get_next(phead);

	while (rtw_end_of_queue_search(phead, plist) == _FALSE)
	{

		pnetwork = LIST_CONTAINOR(plist, struct wlan_network, list);

		plist = get_next(plist);

		_rtw_free_network(pmlmepriv,pnetwork,isfreeall);
		
	}

	_exit_critical(&scanned_queue->lock, &irqL);
	
_func_exit_;		

}




sint rtw_if_up(_adapter *padapter)	{

	sint res;
_func_enter_;		

	if( padapter->bDriverStopped || padapter->bSurpriseRemoved ||
		(check_fwstate(&padapter->mlmepriv, _FW_LINKED)== _FALSE)){		
		RT_TRACE(_module_rtl871x_mlme_c_, _drv_info_, ("rtw_if_up:bDriverStopped(%d) OR bSurpriseRemoved(%d)", padapter->bDriverStopped, padapter->bSurpriseRemoved));	
		res=_FALSE;
	}
	else
		res=  _TRUE;
	
_func_exit_;
	return res;
}


void rtw_generate_random_ibss(u8* pibss)
{
	u32	curtime = rtw_get_current_time();

_func_enter_;
	pibss[0] = 0x02;  //in ad-hoc mode bit1 must set to 1
	pibss[1] = 0x11;
	pibss[2] = 0x87;
	pibss[3] = (u8)(curtime & 0xff) ;//p[0];
	pibss[4] = (u8)((curtime>>8) & 0xff) ;//p[1];
	pibss[5] = (u8)((curtime>>16) & 0xff) ;//p[2];
_func_exit_;
	return;
}
u8 *rtw_get_capability_from_ie(u8 *ie)
{
	return (ie + 8 + 2);
}


u16 rtw_get_capability(WLAN_BSSID_EX *bss)
{
	u16	val;
_func_enter_;	
	_rtw_memcpy((u8 *)&val, rtw_get_capability_from_ie(bss->IEs), 2); 
_func_exit_;		
	return le16_to_cpu(val);
}

u8 *rtw_get_timestampe_from_ie(u8 *ie)
{
	return (ie + 0);	
}

u8 *rtw_get_beacon_interval_from_ie(u8 *ie)
{
	return (ie + 8);	
}


int	rtw_init_mlme_priv (_adapter *padapter)//(struct	mlme_priv *pmlmepriv)
{
	int	res;
_func_enter_;	
	res = _rtw_init_mlme_priv(padapter);// (pmlmepriv);
_func_exit_;	
	return res;
}

void rtw_free_mlme_priv (struct mlme_priv *pmlmepriv)
{
_func_enter_;
	RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_free_mlme_priv\n"));
	_free_mlme_priv (pmlmepriv);
_func_exit_;	
}

static int	enqueue_network(_queue *queue, struct wlan_network *pnetwork)
{
	int	res;
_func_enter_;		
	res = _rtw_enqueue_network(queue, pnetwork);
_func_exit_;		
	return res;
}



static struct	wlan_network *dequeue_network(_queue *queue)
{
	struct wlan_network *pnetwork;
_func_enter_;		
	pnetwork = _rtw_dequeue_network(queue);
_func_exit_;		
	return pnetwork;
}


static struct	wlan_network *alloc_network(struct	mlme_priv *pmlmepriv )//(_queue	*free_queue)
{
	struct	wlan_network	*pnetwork;
_func_enter_;			
	pnetwork = _rtw_alloc_network(pmlmepriv);
_func_exit_;			
	return pnetwork;
}

void rtw_free_network(struct mlme_priv *pmlmepriv, struct	wlan_network *pnetwork,u8 is_freeall )//(struct	wlan_network *pnetwork, _queue	*free_queue)
{
_func_enter_;		
	RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_free_network==> ssid = %s \n\n" , pnetwork->network.Ssid.Ssid));
	_rtw_free_network(pmlmepriv, pnetwork,is_freeall);
_func_exit_;		
}


static void free_network_nolock(struct mlme_priv *pmlmepriv, struct wlan_network *pnetwork )
{
_func_enter_;		
	//RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_free_network==> ssid = %s \n\n" , pnetwork->network.Ssid.Ssid));
	_free_network_nolock(pmlmepriv, pnetwork);
_func_exit_;		
}


void rtw_free_network_queue(_adapter* dev,u8 isfreeall)
{
_func_enter_;		
	_rtw_free_network_queue(dev,isfreeall);
_func_exit_;			
}

/*
	return the wlan_network with the matching addr

	Shall be calle under atomic context... to avoid possible racing condition...
*/
static struct	wlan_network *find_network(_queue *scanned_queue, u8 *addr)
{
	struct	wlan_network *pnetwork = _rtw_find_network(scanned_queue, addr);

	return pnetwork;
}

int rtw_is_same_ibss(_adapter *adapter, struct wlan_network *pnetwork)
{
	int ret=_TRUE;
	struct security_priv *psecuritypriv = &adapter->securitypriv;

	if ( (psecuritypriv->dot11PrivacyAlgrthm != _NO_PRIVACY_ ) &&
		    ( pnetwork->network.Privacy == 0 ) )
	{
		ret=_FALSE;
	}
	else if((psecuritypriv->dot11PrivacyAlgrthm == _NO_PRIVACY_ ) &&
		 ( pnetwork->network.Privacy == 1 ) )
	{
		ret=_FALSE;
	}
	else
	{
		ret=_TRUE;
	}
	
	return ret;
	
}

static int is_same_network(WLAN_BSSID_EX *src, WLAN_BSSID_EX *dst)
{
	 u16 s_cap, d_cap;
	 
_func_enter_;	

#ifdef PLATFORM_OS_XP
	 if ( ((uint)dst) <= 0x7fffffff || 
	 	((uint)src) <= 0x7fffffff ||
	 	((uint)&s_cap) <= 0x7fffffff ||
	 	((uint)&d_cap) <= 0x7fffffff)
	{
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n@@@@ error address of dst\n"));
			
		KeBugCheckEx(0x87110000, (ULONG_PTR)dst, (ULONG_PTR)src,(ULONG_PTR)&s_cap, (ULONG_PTR)&d_cap);

		return _FALSE;
	}
#endif

	_rtw_memcpy((u8 *)&s_cap, rtw_get_capability_from_ie(src->IEs), 2);
	_rtw_memcpy((u8 *)&d_cap, rtw_get_capability_from_ie(dst->IEs), 2);
	
	s_cap = le16_to_cpu(s_cap);
	d_cap = le16_to_cpu(d_cap);
	
_func_exit_;			

	return ((src->Ssid.SsidLength == dst->Ssid.SsidLength) &&
			(src->Configuration.DSConfig == dst->Configuration.DSConfig) &&
			( (_rtw_memcmp(src->MacAddress, dst->MacAddress, ETH_ALEN)) == _TRUE) &&
			( (_rtw_memcmp(src->Ssid.Ssid, dst->Ssid.Ssid, src->Ssid.SsidLength)) == _TRUE) &&
			((s_cap & WLAN_CAPABILITY_IBSS) == 
			(d_cap & WLAN_CAPABILITY_IBSS)) &&
			((s_cap & WLAN_CAPABILITY_BSS) == 
			(d_cap & WLAN_CAPABILITY_BSS)));
	
}

struct	wlan_network	* rtw_get_oldest_wlan_network(_queue *scanned_queue)
{
	_list	*plist, *phead;

	
	struct	wlan_network	*pwlan = NULL;
	struct	wlan_network	*oldest = NULL;
_func_enter_;		
	phead = get_list_head(scanned_queue);
	
	plist = get_next(phead);

	while(1)
	{
		
		if (rtw_end_of_queue_search(phead,plist)== _TRUE)
			break;
		
		pwlan= LIST_CONTAINOR(plist, struct wlan_network, list);

		if(pwlan->fixed!=_TRUE)
		{		
			if (oldest == NULL ||time_after(oldest->last_scanned, pwlan->last_scanned))
				oldest = pwlan;
		}
		
		plist = get_next(plist);
	}
_func_exit_;		
	return oldest;
	
}

static void update_network(WLAN_BSSID_EX *dst, WLAN_BSSID_EX *src,_adapter * padapter)
{
	u32 last_evm = 0, tmpVal;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

_func_enter_;		

#ifdef CONFIG_ANTENNA_DIVERSITY
		if((0 != pHalData->AntDivCfg) && (!IS_92C_SERIAL(pHalData->VersionID)) )
		{
			
			//printk("update_network=> orgRSSI(%d)(%d),newRSSI(%d)(%d)\n",dst->Rssi,query_rx_pwr_percentage(dst->Rssi),
			//	src->Rssi,query_rx_pwr_percentage(src->Rssi));
			//select optimum_antenna for before linked =>For antenna diversity
			if(dst->Rssi >=  src->Rssi )//keep org parameter
			{
				src->Rssi = dst->Rssi;
				src->PhyInfo.Optimum_antenna = dst->PhyInfo.Optimum_antenna;						
			}			
		}
	
#endif
	_rtw_memcpy((u8 *)dst, (u8 *)src, get_WLAN_BSSID_EX_sz(src));
#ifdef CONFIG_ANTENNA_DIVERSITY
//	printk("## update_network=> RSSI(%d),Ant_(%s)\n",dst->Rssi,(2==dst->PhyInfo.Optimum_antenna)?"A":"B");
#endif
#if 0
//	printk("update_network: rssi=0x%lx dst->Rssi=%d ,dst->Rssi=0x%lx , src->Rssi=0x%lx",(dst->Rssi+src->Rssi)/2,dst->Rssi,dst->Rssi,src->Rssi);
	if (check_fwstate(&padapter->mlmepriv, _FW_LINKED) && is_same_network(&(padapter->mlmepriv.cur_network.network), src))
	{
	
	//	printk("b:ssid=%s update_network: src->rssi=0x%d padapter->recvpriv.ui_rssi=%d\n",src->Ssid.Ssid,src->Rssi,padapter->recvpriv.signal);
		if(padapter->recvpriv.signal_qual_data.total_num++ >= PHY_LINKQUALITY_SLID_WIN_MAX)
	        {
	              padapter->recvpriv.signal_qual_data.total_num = PHY_LINKQUALITY_SLID_WIN_MAX;
	              last_evm = padapter->recvpriv.signal_qual_data.elements[padapter->recvpriv.signal_qual_data.index];
	              padapter->recvpriv.signal_qual_data.total_val -= last_evm;
	        }
               	padapter->recvpriv.signal_qual_data.total_val += query_rx_pwr_percentage(src->Rssi);

              	padapter->recvpriv.signal_qual_data.elements[padapter->recvpriv.signal_qual_data.index++] = query_rx_pwr_percentage(src->Rssi);
                if(padapter->recvpriv.signal_qual_data.index >= PHY_LINKQUALITY_SLID_WIN_MAX)
                       padapter->recvpriv.signal_qual_data.index = 0;

  //          printk("Total SQ=%d  pattrib->signal_qual= %d\n", padapter->recvpriv.signal_qual_data.total_val, src->Rssi);

                // <1> Showed on UI for user,in percentage.
              	tmpVal = padapter->recvpriv.signal_qual_data.total_val/padapter->recvpriv.signal_qual_data.total_num;
                padapter->recvpriv.signal=(u8)tmpVal;//Link quality

		src->Rssi= translate_percentage_to_dbm(padapter->recvpriv.signal) ;
	}
	else{
//	printk("ELSE:ssid=%s update_network: src->rssi=0x%d dst->rssi=%d\n",src->Ssid.Ssid,src->Rssi,dst->Rssi);
		src->Rssi=(src->Rssi +dst->Rssi)/2;//dBM
	}	

//	printk("a:update_network: src->rssi=0x%d padapter->recvpriv.ui_rssi=%d\n",src->Rssi,padapter->recvpriv.signal);

#endif
	
_func_exit_;		
}

static void update_current_network(_adapter *adapter, WLAN_BSSID_EX *pnetwork)
{
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);	
	
_func_enter_;		

#ifdef PLATFORM_OS_XP
	if ((unsigned long)(&(pmlmepriv->cur_network.network)) < 0x7ffffff)
	{		
		KeBugCheckEx(0x87111c1c, (ULONG_PTR)(&(pmlmepriv->cur_network.network)), 0, 0,0);
	}
#endif

	if(is_same_network(&(pmlmepriv->cur_network.network), pnetwork))
	{
		//RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,"Same Network\n");	

		//if(pmlmepriv->cur_network.network.IELength<= pnetwork->IELength)
		{
			update_network(&(pmlmepriv->cur_network.network), pnetwork,adapter);
			rtw_update_protection(adapter, (pmlmepriv->cur_network.network.IEs) + sizeof (NDIS_802_11_FIXED_IEs), 
									pmlmepriv->cur_network.network.IELength);
		}	
	}	
	
_func_exit_;			

}


/*

Caller must hold pmlmepriv->lock first.


*/
void rtw_update_scanned_network(_adapter *adapter, WLAN_BSSID_EX *target)
{
	_list	*plist, *phead;
	
	ULONG	bssid_ex_sz;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(adapter);
	_queue	*queue	= &(pmlmepriv->scanned_queue);	
	struct	wlan_network	*pnetwork = NULL;
	struct  wlan_network	*oldest = NULL;
_func_enter_;			

	
	phead = get_list_head(queue);	
	plist = get_next(phead);	
	
	while(1)
	{
		
		if (rtw_end_of_queue_search(phead,plist)== _TRUE)
			break;
		
		pnetwork	= LIST_CONTAINOR(plist, struct wlan_network, list);

		if ((unsigned long)(pnetwork) < 0x7ffffff)
		{
#ifdef PLATFORM_OS_XP	
				KeBugCheckEx(0x87111c1c, (ULONG_PTR)pnetwork, 0, 0,0);
#endif
		}

		if (is_same_network(&(pnetwork->network), target))
			break;
		
		if ((oldest == ((struct wlan_network *)0)) ||
		time_after(oldest->last_scanned, pnetwork->last_scanned))
			oldest = pnetwork;
		
		plist = get_next(plist);
		
	}
	
	
	/* If we didn't find a match, then get a new network slot to initialize
	 * with this beacon's information */
	if (rtw_end_of_queue_search(phead,plist)== _TRUE) {
		
		if (_rtw_queue_empty(&(pmlmepriv->free_bss_pool)) == _TRUE) {
			/* If there are no more slots, expire the oldest */
			//list_del_init(&oldest->list);
			pnetwork = oldest;
			
//			printk("update_network: rssi=0x%lx ,pnetwork->network.Rssi=0x%lx , target->Rssi=0x%lx",(pnetwork->network.Rssi+target->Rssi)/2,pnetwork->network.Rssi,target->Rssi);

			//target->Rssi=(pnetwork->network.Rssi+target->Rssi)/2;
#ifdef CONFIG_ANTENNA_DIVERSITY
			target->PhyInfo.Optimum_antenna = pHalData->CurAntenna;//optimum_antenna=>For antenna diversity
#endif
			_rtw_memcpy(&(pnetwork->network), target,  get_WLAN_BSSID_EX_sz(target));
			pnetwork->last_scanned = rtw_get_current_time();

		} else {
			/* Otherwise just pull from the free list */
			
			pnetwork = alloc_network(pmlmepriv); // will update scan_time


			if(pnetwork==NULL){ 
				RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n\n\nsomething wrong here\n\n\n"));
				goto exit;
			}


			bssid_ex_sz = get_WLAN_BSSID_EX_sz(target);
			target->Length = bssid_ex_sz;
#ifdef CONFIG_ANTENNA_DIVERSITY
			target->PhyInfo.Optimum_antenna = pHalData->CurAntenna;
#endif		
			_rtw_memcpy(&(pnetwork->network), target, bssid_ex_sz );

			rtw_list_insert_tail(&(pnetwork->list),&(queue->queue)); 
			
		}	 
	} 
	else {
		/* we have an entry and we are going to update it. But this entry may
		 * be already expired. In this case we do the same as we found a new 
		 * net and call the new_net handler
		 */

		//target.Reserved[0]==1, means that scaned network is a bcn frame.
		if((pnetwork->network.IELength>target->IELength) && (target->Reserved[0]==1))
			goto exit;
	
		update_network(&(pnetwork->network),target,adapter);

		pnetwork->last_scanned = rtw_get_current_time();

	}

exit:	

_func_exit_;			

}


void rtw_add_network(_adapter *adapter, WLAN_BSSID_EX *pnetwork)
{
	_irqL irqL;
	struct	mlme_priv	*pmlmepriv = &(((_adapter *)adapter)->mlmepriv);
	_queue	*queue	= &(pmlmepriv->scanned_queue);

_func_enter_;		

	_enter_critical_bh(&queue->lock, &irqL);
	
	update_current_network(adapter, pnetwork);
	
	rtw_update_scanned_network(adapter, pnetwork);

	_exit_critical_bh(&queue->lock, &irqL);
	
_func_exit_;		
}

//select the desired network based on the capability of the (i)bss.
// check items: (1) security
//			   (2) network_type
//			   (3) WMM
//			   (4) HT
//                     (5) others
static int is_desired_network(_adapter *adapter, struct wlan_network *pnetwork)
{
	struct security_priv *psecuritypriv = &adapter->securitypriv;
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;
	struct registry_priv	 *pregpriv = &adapter->registrypriv;
	u32 desired_encmode;
	u32 privacy;

	u8 wps_ie[512];
	uint wps_ielen;

	int bselected = _TRUE;
	
	desired_encmode = psecuritypriv->ndisencryptstatus;
	privacy = pnetwork->network.Privacy;

	if(psecuritypriv->wps_phase == _TRUE)
	{
		if(rtw_get_wps_ie(pnetwork->network.IEs, pnetwork->network.IELength, wps_ie, &wps_ielen)==_TRUE)
		{
			//rtw_disassoc_cmd(adapter);			
			//rtw_indicate_disconnect(adapter);
			//rtw_free_assoc_resources(adapter);
			return _TRUE;
		}
		else
		{	
			return _FALSE;
		}	
	}
	if (pregpriv->wifi_spec == 1) //for  correct flow of 8021X  to do....
	{
		if ((desired_encmode == Ndis802_11EncryptionDisabled) && (privacy != 0))	
				bselected = _FALSE;
	}
	

 	if ((desired_encmode != Ndis802_11EncryptionDisabled) && (privacy == 0))
		bselected = _FALSE;

	if(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE)
	{
		if(pnetwork->network.InfrastructureMode != pmlmepriv->cur_network.network.InfrastructureMode)
			bselected = _FALSE;
	}	
		

	return bselected;
}

/* TODO: Perry : For Power Management */
void rtw_atimdone_event_callback(_adapter	*adapter , u8 *pbuf)
{

_func_enter_;		
	RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("receive atimdone_evet\n"));	
_func_exit_;			
	return;	
}


void rtw_survey_event_callback(_adapter	*adapter, u8 *pbuf)
{
	_irqL  irqL;
	u32 len;
	WLAN_BSSID_EX *pnetwork;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);

_func_enter_;		

	pnetwork = (WLAN_BSSID_EX *)pbuf;

	RT_TRACE(_module_rtl871x_mlme_c_,_drv_info_,("rtw_survey_event_callback, ssid=%s\n",  pnetwork->Ssid.Ssid));

#ifdef CONFIG_RTL8712
        //endian_convert
 	pnetwork->Length = le32_to_cpu(pnetwork->Length);
  	pnetwork->Ssid.SsidLength = le32_to_cpu(pnetwork->Ssid.SsidLength);	
	pnetwork->Privacy =le32_to_cpu( pnetwork->Privacy);
	pnetwork->Rssi = le32_to_cpu(pnetwork->Rssi);
	pnetwork->NetworkTypeInUse =le32_to_cpu(pnetwork->NetworkTypeInUse);	
	pnetwork->Configuration.ATIMWindow = le32_to_cpu(pnetwork->Configuration.ATIMWindow);
	pnetwork->Configuration.BeaconPeriod = le32_to_cpu(pnetwork->Configuration.BeaconPeriod);
	pnetwork->Configuration.DSConfig =le32_to_cpu(pnetwork->Configuration.DSConfig);
	pnetwork->Configuration.FHConfig.DwellTime=le32_to_cpu(pnetwork->Configuration.FHConfig.DwellTime);
	pnetwork->Configuration.FHConfig.HopPattern=le32_to_cpu(pnetwork->Configuration.FHConfig.HopPattern);
	pnetwork->Configuration.FHConfig.HopSet=le32_to_cpu(pnetwork->Configuration.FHConfig.HopSet);
	pnetwork->Configuration.FHConfig.Length=le32_to_cpu(pnetwork->Configuration.FHConfig.Length);	
	pnetwork->Configuration.Length = le32_to_cpu(pnetwork->Configuration.Length);
	pnetwork->InfrastructureMode = le32_to_cpu(pnetwork->InfrastructureMode);
	pnetwork->IELength = le32_to_cpu(pnetwork->IELength);
#endif	

	len = get_WLAN_BSSID_EX_sz(pnetwork);
	if(len > (sizeof(WLAN_BSSID_EX)))
	{
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n ****rtw_survey_event_callback: return a wrong bss ***\n"));
		goto exit;
	}


#ifdef CONFIG_DRVEXT_MODULE
	update_random_seed((void *)(adapter), pnetwork->IEs);	
#endif



	_enter_critical_bh(&pmlmepriv->lock, &irqL);

	// update IBSS_network 's timestamp
	if ((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)) == _TRUE)
	{
		//RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,"rtw_survey_event_callback : WIFI_ADHOC_MASTER_STATE \n\n");
		if(_rtw_memcmp(&(pmlmepriv->cur_network.network.MacAddress), pnetwork->MacAddress, ETH_ALEN))
		{
			struct wlan_network* ibss_wlan = NULL;
			
			_rtw_memcpy(pmlmepriv->cur_network.network.IEs, pnetwork->IEs, 8);

			ibss_wlan = find_network(&pmlmepriv->scanned_queue,  pnetwork->MacAddress);
			if(!ibss_wlan)
			{
				_rtw_memcpy(ibss_wlan->network.IEs , pnetwork->IEs, 8);			
				goto exit;
			}
		}
	}

	// lock pmlmepriv->lock when you accessing network_q
	if ((check_fwstate(pmlmepriv, _FW_UNDER_LINKING)) == _FALSE)
	{		
		if(pnetwork->Ssid.Ssid[0]!=0)
		{
			rtw_add_network(adapter, pnetwork);
		}	
		else
		{
#ifndef CONFIG_PLATFORM_MT53XX
			pnetwork->Ssid.SsidLength = 8;
			_rtw_memcpy(pnetwork->Ssid.Ssid, "<hidden>", 8);
			rtw_add_network(adapter, pnetwork);			
#endif
		}
	}

exit:	
		
	_exit_critical_bh(&pmlmepriv->lock, &irqL);

_func_exit_;		

	return;	
}



void rtw_surveydone_event_callback(_adapter	*adapter, u8 *pbuf)
{
	_irqL  irqL;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	
#ifdef CONFIG_MLME_EXT	

	mlmeext_surveydone_event_callback(adapter);

#endif

_func_enter_;			

	_enter_critical_bh(&pmlmepriv->lock, &irqL);
	
	RT_TRACE(_module_rtl871x_mlme_c_,_drv_info_,("rtw_surveydone_event_callback: fw_state:%x\n\n", pmlmepriv->fw_state));
	
	if  (pmlmepriv->fw_state & _FW_UNDER_SURVEY)
	{
		u8 timer_cancelled;
		
		_cancel_timer(&pmlmepriv->scan_to_timer, &timer_cancelled);
		
		pmlmepriv->fw_state ^= _FW_UNDER_SURVEY;
	}
	else {
	
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("nic status =%x, survey done event comes too late!\n", pmlmepriv->fw_state));	
	}	
	
	if(pmlmepriv->to_join == _TRUE)
	{
		if((check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)==_TRUE) )
		{
			if(check_fwstate(pmlmepriv, _FW_LINKED)==_FALSE)
			{
				pmlmepriv->fw_state |= _FW_UNDER_LINKING;	
				
		   		if(rtw_select_and_join_from_scanned_queue(pmlmepriv)==_SUCCESS)
		   		{
		       			_set_timer(&pmlmepriv->assoc_timer, MAX_JOIN_TIMEOUT );	 
                  		}
		   		else	
		  		{
					WLAN_BSSID_EX    *pdev_network = &(adapter->registrypriv.dev_network); 			
					u8 *pibss = adapter->registrypriv.dev_network.MacAddress;

					pmlmepriv->fw_state ^= _FW_UNDER_SURVEY;//because don't set assoc_timer

					RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("switching to adhoc master\n"));
				
					_rtw_memset(&pdev_network->Ssid, 0, sizeof(NDIS_802_11_SSID));
					_rtw_memcpy(&pdev_network->Ssid, &pmlmepriv->assoc_ssid, sizeof(NDIS_802_11_SSID));
	
					rtw_update_registrypriv_dev_network(adapter);
					rtw_generate_random_ibss(pibss);

                       			pmlmepriv->fw_state = WIFI_ADHOC_MASTER_STATE;
			
					if(rtw_createbss_cmd(adapter)!=_SUCCESS)
					{
	                     		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("Error=>rtw_createbss_cmd status FAIL\n"));						
					}	

			     		pmlmepriv->to_join = _FALSE;
		   		}
		 	}
		}
		else
		{
			if(rtw_select_and_join_from_scanned_queue(pmlmepriv)==_SUCCESS)
			{
                             pmlmepriv->fw_state |= _FW_UNDER_LINKING;		
			     pmlmepriv->to_join = _FALSE;
	     		     _set_timer(&pmlmepriv->assoc_timer, MAX_JOIN_TIMEOUT);	 
			} 
			else
			{
				RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("try_to_join, but select scanning queue fail\n"));
			}			

		}
		
	}

	_exit_critical_bh(&pmlmepriv->lock, &irqL);	

_func_exit_;	

}

static void free_scanqueue(struct	mlme_priv *pmlmepriv)
{
	_irqL irqL;
	_queue *free_queue = &pmlmepriv->free_bss_pool;
	_queue *scan_queue = &pmlmepriv->scanned_queue;
	_list	*plist, *phead, *ptemp;
	
_func_enter_;		
	
	RT_TRACE(_module_rtl871x_mlme_c_, _drv_notice_, ("+free_scanqueue\n"));

	_enter_critical(&free_queue->lock, &irqL);

	phead = get_list_head(scan_queue);
	plist = get_next(phead);

	while (plist != phead)
       {
		ptemp = get_next(plist);
		list_delete(plist);
		rtw_list_insert_tail(plist, &free_queue->queue);
		plist =ptemp;
		pmlmepriv->num_of_scanned --;
        }
	
	_exit_critical(&free_queue->lock, &irqL);
	
_func_exit_;
}
	
/*
*rtw_free_assoc_resources: the caller has to lock pmlmepriv->lock
*/
void rtw_free_assoc_resources(_adapter *adapter )
{
	_irqL irqL;
	struct wlan_network* pwlan = NULL;
     	struct	mlme_priv *pmlmepriv = &adapter->mlmepriv;
	struct 	mlme_ext_info *pmlmeinfo = &adapter->mlmeextpriv.mlmext_info;
   	struct	sta_priv *pstapriv = &adapter->stapriv;
	struct wlan_network *tgt_network = &pmlmepriv->cur_network;
	
_func_enter_;			

	RT_TRACE(_module_rtl871x_mlme_c_, _drv_notice_, ("+rtw_free_assoc_resources\n"));

	pwlan = find_network(&pmlmepriv->scanned_queue, tgt_network->network.MacAddress);
	
	RT_TRACE(_module_rtl871x_mlme_c_, _drv_info_, ("tgt_network->network.MacAddress=%02x:%02x:%02x:%02x:%02x:%02x ssid=%s\n",
		tgt_network->network.MacAddress[0],tgt_network->network.MacAddress[1],
		tgt_network->network.MacAddress[2],tgt_network->network.MacAddress[3],
		tgt_network->network.MacAddress[4],tgt_network->network.MacAddress[5], 
		tgt_network->network.Ssid.Ssid));

	if(check_fwstate( pmlmepriv, WIFI_STATION_STATE|WIFI_AP_STATE))
	{
		struct sta_info* psta;
		
		psta = rtw_get_stainfo(&adapter->stapriv, tgt_network->network.MacAddress);

		_enter_critical_bh(&(pstapriv->sta_hash_lock), &irqL);
		rtw_free_stainfo(adapter,  psta);
		_exit_critical_bh(&(pstapriv->sta_hash_lock), &irqL);
		
	}

	if(check_fwstate( pmlmepriv, WIFI_ADHOC_STATE|WIFI_ADHOC_MASTER_STATE|WIFI_AP_STATE))
	{
		rtw_free_all_stainfo(adapter);
	}

	if(pwlan)		
	{
		pwlan->fixed = _FALSE;
	}	
	else
	{
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_free_assoc_resources : pwlan== NULL \n\n"));
	}


	if((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) && (adapter->stapriv.asoc_sta_count== 1))
		/*||check_fwstate(pmlmepriv, WIFI_STATION_STATE)*/)
	{
		free_network_nolock(pmlmepriv, pwlan); 
	}	
	pmlmeinfo->key_mask = 0;

_func_exit_;	
	
}

/*
*rtw_indicate_connect: the caller has to lock pmlmepriv->lock
*/
void rtw_indicate_connect(_adapter *padapter)
{
	struct	mlme_priv *pmlmepriv = &padapter->mlmepriv;
	
_func_enter_;	
	RT_TRACE(_module_rtl871x_mlme_c_, _drv_err_, ("+rtw_indicate_connect\n"));
 
	pmlmepriv->to_join = _FALSE;
#ifdef CONFIG_ANTENNA_DIVERSITY	
	SwAntDivRestAfterLink(padapter);
#endif	
	set_fwstate(pmlmepriv, _FW_LINKED);

	padapter->ledpriv.LedControlHandler(padapter, LED_CTL_LINK);

	rtw_os_indicate_connect(padapter);

	RT_TRACE(_module_rtl871x_mlme_c_, _drv_err_, ("-rtw_indicate_connect: fw_state=0x%08x\n", get_fwstate(pmlmepriv)));
 
_func_exit_;
}


/*
*rtw_indicate_connect: the caller has to lock pmlmepriv->lock
*/
void rtw_indicate_disconnect( _adapter *padapter )
{
	struct	mlme_priv *pmlmepriv = &padapter->mlmepriv;	

_func_enter_;	
	
	RT_TRACE(_module_rtl871x_mlme_c_, _drv_err_, ("+rtw_indicate_disconnect\n"));

	if((pmlmepriv->fw_state & _FW_LINKED))
	{
	        pmlmepriv->fw_state ^= _FW_LINKED;

	padapter->ledpriv.LedControlHandler(padapter, LED_CTL_NO_LINK);

	rtw_os_indicate_disconnect(padapter);
	
#ifdef CONFIG_LPS
	lps_ctrl_wk_cmd(padapter, LPS_CTRL_DISCONNECT, 1);
#endif
 	
	}
 	
_func_exit_;	

}

//Notes:
//pnetwork : returns from rtw_joinbss_event_callback
//ptarget_wlan: found from scanned_queue
//if join_res > 0, for (fw_state==WIFI_STATION_STATE), we check if  "ptarget_sta" & "ptarget_wlan" exist.	
//if join_res > 0, for (fw_state==WIFI_ADHOC_STATE), we only check if "ptarget_wlan" exist.
//if join_res > 0, update "cur_network->network" from "pnetwork->network" if (ptarget_wlan !=NULL).
//
void rtw_joinbss_event_callback(_adapter *adapter, u8 *pbuf)
{
	_irqL irqL,irqL2;
	int	res;
	static u8 retry=0;
	u8 timer_cancelled;
	struct sta_info *ptarget_sta= NULL, *pcur_sta = NULL;
   	struct	sta_priv *pstapriv = &adapter->stapriv;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	struct wlan_network 	*pnetwork	= (struct wlan_network *)pbuf;
	struct wlan_network 	*cur_network = &(pmlmepriv->cur_network);
	struct wlan_network	*pcur_wlan = NULL, *ptarget_wlan = NULL;
	unsigned int 		the_same_macaddr = _FALSE;	
#ifdef CONFIG_DRVEXT_MODULE
	int enable_wpa = 0, enable_wsc = 0;
	struct drvext_priv *pdrvext = &adapter->drvextpriv;
#endif

_func_enter_;	

#ifdef CONFIG_RTL8712
       //endian_convert
	pnetwork->join_res = le32_to_cpu(pnetwork->join_res);
	pnetwork->network_type = le32_to_cpu(pnetwork->network_type);
	pnetwork->network.Length = le32_to_cpu(pnetwork->network.Length);
	pnetwork->network.Ssid.SsidLength = le32_to_cpu(pnetwork->network.Ssid.SsidLength);
	pnetwork->network.Privacy =le32_to_cpu( pnetwork->network.Privacy);
	pnetwork->network.Rssi = le32_to_cpu(pnetwork->network.Rssi);
	pnetwork->network.NetworkTypeInUse =le32_to_cpu(pnetwork->network.NetworkTypeInUse) ;	
	pnetwork->network.Configuration.ATIMWindow = le32_to_cpu(pnetwork->network.Configuration.ATIMWindow);
	pnetwork->network.Configuration.BeaconPeriod = le32_to_cpu(pnetwork->network.Configuration.BeaconPeriod);
	pnetwork->network.Configuration.DSConfig = le32_to_cpu(pnetwork->network.Configuration.DSConfig);
	pnetwork->network.Configuration.FHConfig.DwellTime=le32_to_cpu(pnetwork->network.Configuration.FHConfig.DwellTime);
	pnetwork->network.Configuration.FHConfig.HopPattern=le32_to_cpu(pnetwork->network.Configuration.FHConfig.HopPattern);
	pnetwork->network.Configuration.FHConfig.HopSet=le32_to_cpu(pnetwork->network.Configuration.FHConfig.HopSet);
	pnetwork->network.Configuration.FHConfig.Length=le32_to_cpu(pnetwork->network.Configuration.FHConfig.Length);	
	pnetwork->network.Configuration.Length = le32_to_cpu(pnetwork->network.Configuration.Length);
	pnetwork->network.InfrastructureMode = le32_to_cpu(pnetwork->network.InfrastructureMode);
	pnetwork->network.IELength = le32_to_cpu(pnetwork->network.IELength );
#endif

	RT_TRACE(_module_rtl871x_mlme_c_,_drv_info_,("joinbss event call back received with res=%d\n", pnetwork->join_res));

	rtw_get_encrypt_decrypt_from_registrypriv(adapter);
	

	if (pmlmepriv->assoc_ssid.SsidLength == 0)
	{
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("@@@@@   joinbss event call back  for Any SSid\n"));		
	}
	else
	{
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("@@@@@   rtw_joinbss_event_callback for SSid:%s\n", pmlmepriv->assoc_ssid.Ssid));
	}
	
	the_same_macaddr = _rtw_memcmp(pnetwork->network.MacAddress, cur_network->network.MacAddress, ETH_ALEN);

	pnetwork->network.Length = get_WLAN_BSSID_EX_sz(&pnetwork->network);
	if(pnetwork->network.Length > sizeof(WLAN_BSSID_EX))
	{
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n\n ***joinbss_evt_callback return a wrong bss ***\n\n"));
		goto ignore_joinbss_callback;
	}
		
	_enter_critical_bh(&pmlmepriv->lock, &irqL);
	
	RT_TRACE(_module_rtl871x_mlme_c_,_drv_info_,("\n rtw_joinbss_event_callback !! _enter_critical \n"));

	if(pnetwork->join_res > 0)
	{
		if ((pmlmepriv->fw_state) & _FW_UNDER_LINKING) 
		{
			//s1. find ptarget_wlan
			if((pmlmepriv->fw_state) & _FW_LINKED)
			{
				if(the_same_macaddr == _TRUE)
				{
					ptarget_wlan = find_network(&pmlmepriv->scanned_queue, cur_network->network.MacAddress);					
				}
				else
				{
					pcur_wlan = find_network(&pmlmepriv->scanned_queue, cur_network->network.MacAddress);
					pcur_wlan->fixed = _FALSE;					

					pcur_sta = rtw_get_stainfo(pstapriv, cur_network->network.MacAddress);
					_enter_critical_bh(&(pstapriv->sta_hash_lock), &irqL2);
					rtw_free_stainfo(adapter,  pcur_sta);
					_exit_critical_bh(&(pstapriv->sta_hash_lock), &irqL2);

					ptarget_wlan = find_network(&pmlmepriv->scanned_queue, pnetwork->network.MacAddress);
					if(ptarget_wlan)	ptarget_wlan->fixed = _TRUE;						
				}

			}
			else
			{
				ptarget_wlan = find_network(&pmlmepriv->scanned_queue, pnetwork->network.MacAddress);
				if(ptarget_wlan)	ptarget_wlan->fixed = _TRUE;				
			}
		
			if(ptarget_wlan == NULL)
			{			
				RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("Can't find ptarget_wlan when joinbss_event callback\n"));
				goto ignore_joinbss_callback;
			}
					
			//s2. find ptarget_sta & update ptarget_sta
			if(check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE)
			{ 
				if(the_same_macaddr == _TRUE)
				{
					ptarget_sta = rtw_get_stainfo(pstapriv, pnetwork->network.MacAddress);
					if(ptarget_sta==NULL)
					{
						ptarget_sta = rtw_alloc_stainfo(pstapriv, pnetwork->network.MacAddress);
					}	
				}
				else
				{
					ptarget_sta = rtw_alloc_stainfo(pstapriv, pnetwork->network.MacAddress);
				}

				if(ptarget_sta) //update ptarget_sta
				{
					ptarget_sta->aid  = pnetwork->join_res;					
					ptarget_sta->qos_option = 1;//? 
					ptarget_sta->mac_id=0;

					if(adapter->securitypriv.dot11AuthAlgrthm== dot11AuthAlgrthm_8021X)
					{						
						adapter->securitypriv.binstallGrpkey=_FALSE;
						adapter->securitypriv.busetkipkey=_FALSE;						
						adapter->securitypriv.bgrpkey_handshake=_FALSE;

						ptarget_sta->ieee8021x_blocked=_TRUE;
						ptarget_sta->dot118021XPrivacy=adapter->securitypriv.dot11PrivacyAlgrthm;
						
						_rtw_memset((u8 *)&ptarget_sta->dot118021x_UncstKey, 0, sizeof (union Keytype));
						
						_rtw_memset((u8 *)&ptarget_sta->dot11tkiprxmickey, 0, sizeof (union Keytype));
						_rtw_memset((u8 *)&ptarget_sta->dot11tkiptxmickey, 0, sizeof (union Keytype));
						
						_rtw_memset((u8 *)&ptarget_sta->dot11txpn, 0, sizeof (union pn48));	
						_rtw_memset((u8 *)&ptarget_sta->dot11rxpn, 0, sizeof (union pn48));	
					}		
					
				}
				else
				{
					RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("Can't allocate stainfo when joinbss_event callback\n"));
					goto ignore_joinbss_callback;
				}
				
			}
		
			//s3. update cur_network & indicate connect
			if(ptarget_wlan)
			{			

				RT_TRACE(_module_rtl871x_mlme_c_,_drv_info_,("\nfw_state:%x, BSSID:%x:%x:%x:%x:%x:%x (fw_state=%d)\n",pmlmepriv->fw_state, 
						pnetwork->network.MacAddress[0], pnetwork->network.MacAddress[1],
						pnetwork->network.MacAddress[2], pnetwork->network.MacAddress[3],
						pnetwork->network.MacAddress[4], pnetwork->network.MacAddress[5],
						pmlmepriv->fw_state));

			
				_rtw_memcpy(&cur_network->network, &pnetwork->network, pnetwork->network.Length);
				cur_network->aid = pnetwork->join_res;
				
				//update fw_state //will clr _FW_UNDER_LINKING here indirectly
				switch(pnetwork->network.InfrastructureMode)
				{
					case Ndis802_11Infrastructure:						
							pmlmepriv->fw_state = WIFI_STATION_STATE;
							break;
					case Ndis802_11IBSS:		
							pmlmepriv->fw_state = WIFI_ADHOC_STATE;
							break;
					default:
							pmlmepriv->fw_state = WIFI_NULL_STATE;
							RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("Invalid network_mode\n"));
							break;
				}

				rtw_update_protection(adapter, (cur_network->network.IEs) + sizeof (NDIS_802_11_FIXED_IEs), 
									(cur_network->network.IELength));
			
#ifdef CONFIG_80211N_HT			
				//TODO: update HT_Capability
				rtw_update_ht_cap(adapter, cur_network->network.IEs, cur_network->network.IELength);
#endif				

				//indicate connect
				if(check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE)
				{
				
#ifdef CONFIG_DRVEXT_MODULE

						if (pdrvext->enable_wpa) 
						{
							//Added by Albert 2008/10/16 for TKIP countermeasure.					       
						 	pdrvext->wpa_tkip_mic_error_occur_time = 0;
						 	pdrvext->wpa_tkip_countermeasure_enable = 0;
							 _rtw_memset(pdrvext->wpa_tkip_countermeasure_blocked_bssid, 0x00, ETH_ALEN );
						
							res = drvext_l2_connect_callback(adapter);
							
							 if (res == L2_CONNECTED)
							 {
							 	rtw_indicate_connect(adapter);
						 	 }	
							 else if (res == L2_DISCONNECTED)
							 {
						 		goto select_and_join_new_bss;
							 }
							 else if (res == L2_PENDING)
							 {
								//DEBUG_ERR(("Going for WPA module\n"));
								pdrvext->wpasm.rx_replay_counter_set = 0;
								enable_wpa = 1;								
							}
							else if (res == L2_WSC_PENDING)
							{
								//DEBUG_ERR(("Going for WSC module\n"));						
								enable_wsc = 1;								
						 	}	


						}
						else
#endif							
						{
							rtw_indicate_connect(adapter);
						}
		
				}
				else
				{
					//adhoc mode will rtw_indicate_connect when rtw_stassoc_event_callback
					RT_TRACE(_module_rtl871x_mlme_c_,_drv_info_,("adhoc mode, fw_state:%x", pmlmepriv->fw_state));
				}

				
			}
		
#ifdef CONFIG_DRVEXT_MODULE
			if(enable_wpa) 
			{
				_set_timer(&pmlmepriv->assoc_timer, MAX_JOIN_TIMEOUT);   				
				//DEBUG_ERR(("@@ Set Assoc Timer [%x] for WPA@@\n"));
			}
			else
#endif				
			{
				RT_TRACE(_module_rtl871x_mlme_c_,_drv_info_,("Cancle assoc_timer \n"));		
				_cancel_timer(&pmlmepriv->assoc_timer, &timer_cancelled);
			}
	

		}
		else
		{
			RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_joinbss_event_callback err: fw_state:%x", pmlmepriv->fw_state));	
			goto ignore_joinbss_callback;
		}
				
	}
	else //if join_res < 0 (join fails), then try again
	{

#ifdef CONFIG_DRVEXT_MODULE

select_and_join_new_bss:

		//drvext_assoc_fail_indicate(adapter);
#endif	

		res = rtw_select_and_join_from_scanned_queue(pmlmepriv);	
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_select_and_join_from_scanned_queue again! res:%d\n",res));
		if (res != _SUCCESS || retry>2)
		{
			RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("Set Assoc_Timer = 1; can't find match ssid in scanned_q \n"));
			
			_set_timer(&pmlmepriv->assoc_timer, 1);					

			//rtw_free_assoc_resources(adapter);

			if((check_fwstate(pmlmepriv, _FW_UNDER_LINKING)) == _TRUE)
			{		
                		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("fail! clear _FW_UNDER_LINKING ^^^fw_state=%x\n", pmlmepriv->fw_state));
				pmlmepriv->fw_state ^= _FW_UNDER_LINKING;
			}						

			retry = 0;
			
		}
		else
		{
			//extend time of assoc_timer
			_set_timer(&pmlmepriv->assoc_timer, MAX_JOIN_TIMEOUT);	

			retry++;
		}
		
	}

				
ignore_joinbss_callback:

	_exit_critical_bh(&pmlmepriv->lock, &irqL);

	if(pnetwork->join_res > 0)
	{
		retry = 0;
		mlmeext_joinbss_event_callback(adapter);

#ifdef CONFIG_LPS
		lps_ctrl_wk_cmd(adapter, LPS_CTRL_CONNECT, 0);
#endif
	}

#ifdef CONFIG_DRVEXT_MODULE_WSC

	if (enable_wsc)
	{
		// here we start registration protocol
		start_reg_protocol(adapter);
	}
	
#endif	

_func_exit_;	

}

void rtw_stassoc_event_callback(_adapter *adapter, u8 *pbuf)
{
	_irqL irqL;	
	struct sta_info *psta;
	struct mlme_priv *pmlmepriv = &(adapter->mlmepriv);
	struct stassoc_event *pstassoc	= (struct stassoc_event*)pbuf;

_func_enter_;	
	
	// to do: 
	if(rtw_access_ctrl(&adapter->acl_list, pstassoc->macaddr) == _FALSE)
		return;

	psta = rtw_get_stainfo(&adapter->stapriv, pstassoc->macaddr);	
	if( psta != NULL)
	{
		//the sta have been in sta_info_queue => do nothing 
		
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("Error: rtw_stassoc_event_callback: sta has been in sta_hash_queue \n"));
		
		goto exit; //(between drv has received this event before and  fw have not yet to set key to CAM_ENTRY)
	}

	psta = rtw_alloc_stainfo(&adapter->stapriv, pstassoc->macaddr);	
	if (psta == NULL) {
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("Can't alloc sta_info when rtw_stassoc_event_callback\n"));
		goto exit;
	}	
	
	//to do : init sta_info variable
	psta->qos_option = 0;	
	psta->mac_id = le32_to_cpu((uint)pstassoc->cam_id);
	//psta->aid = (uint)pstassoc->cam_id;
	
	if(adapter->securitypriv.dot11AuthAlgrthm==dot11AuthAlgrthm_8021X)
		psta->dot118021XPrivacy = adapter->securitypriv.dot11PrivacyAlgrthm;

	psta->ieee8021x_blocked = _FALSE;		
	
	_enter_critical(&pmlmepriv->lock, &irqL);

	if ( (check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)==_TRUE ) || 
		(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)==_TRUE ) )
	{
		if(adapter->stapriv.asoc_sta_count== 2)
		{ 
			// a sta + bc/mc_stainfo (not Ibss_stainfo)
			rtw_indicate_connect(adapter);
		}
	}

	_exit_critical(&pmlmepriv->lock, &irqL);


	mlmeext_sta_add_event_callback(adapter, psta);
	
#ifdef CONFIG_RTL8711
	//submit SetStaKey_cmd to tell fw, fw will allocate an CAM entry for this sta	
	rtw_setstakey_cmd(adapter, (unsigned char*)psta, _FALSE);
#endif
		
exit:
	
_func_exit_;	

}

void rtw_stadel_event_callback(_adapter *adapter, u8 *pbuf)
{
	_irqL irqL,irqL2;	
	struct sta_info *psta;
	struct wlan_network* pwlan = NULL;
	WLAN_BSSID_EX    *pdev_network=NULL;
	u8* pibss = NULL;
	struct	mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	struct 	stadel_event *pstadel	= (struct stadel_event*)pbuf;
   	struct	sta_priv *pstapriv = &adapter->stapriv;
	struct wlan_network *tgt_network = &(pmlmepriv->cur_network);
	
_func_enter_;	

	mlmeext_sta_del_event_callback(adapter);

	_enter_critical_bh(&pmlmepriv->lock, &irqL2);

	if(pmlmepriv->fw_state & WIFI_STATION_STATE)
	{
		rtw_free_assoc_resources(adapter);
		rtw_indicate_disconnect(adapter);
	}

	if ( (pmlmepriv->fw_state& WIFI_ADHOC_MASTER_STATE) || 
	      (pmlmepriv->fw_state& WIFI_ADHOC_STATE))
	{
		psta = rtw_get_stainfo(&adapter->stapriv, pstadel->macaddr);
		
		_enter_critical_bh(&(pstapriv->sta_hash_lock), &irqL);
		
		rtw_free_stainfo(adapter,  psta);
		
		_exit_critical_bh(&(pstapriv->sta_hash_lock), &irqL);
		
		if(adapter->stapriv.asoc_sta_count== 1) //a sta + bc/mc_stainfo (not Ibss_stainfo)
		{ 
			//rtw_indicate_disconnect(adapter);//removed@20091105
			
			//free old ibss network
			//pwlan = find_network(&pmlmepriv->scanned_queue, pstadel->macaddr);
			pwlan = find_network(&pmlmepriv->scanned_queue, tgt_network->network.MacAddress);
			if(pwlan)	
			{
				pwlan->fixed = _FALSE;
				free_network_nolock(pmlmepriv, pwlan); 
			}

			//re-create ibss
			pdev_network = &(adapter->registrypriv.dev_network);			
			pibss = adapter->registrypriv.dev_network.MacAddress;

			_rtw_memcpy(pdev_network, &tgt_network->network, get_WLAN_BSSID_EX_sz(&tgt_network->network));
			
			_rtw_memset(&pdev_network->Ssid, 0, sizeof(NDIS_802_11_SSID));
			_rtw_memcpy(&pdev_network->Ssid, &pmlmepriv->assoc_ssid, sizeof(NDIS_802_11_SSID));
	
			rtw_update_registrypriv_dev_network(adapter);			

			rtw_generate_random_ibss(pibss);
			
			if((pmlmepriv->fw_state & WIFI_ADHOC_STATE))
			{
				pmlmepriv->fw_state |= WIFI_ADHOC_MASTER_STATE;
				pmlmepriv->fw_state ^= WIFI_ADHOC_STATE;
			}

			if(rtw_createbss_cmd(adapter)!=_SUCCESS)
			{
				RT_TRACE(_module_rtl871x_ioctl_set_c_,_drv_err_,("***Error=>rtw_stadel_event_callback: rtw_createbss_cmd status FAIL*** \n "));										
			}

			
		}
		
	}
	
	_exit_critical_bh(&pmlmepriv->lock, &irqL2);
	
_func_exit_;	

}


void rtw_cpwm_event_callback(_adapter *adapter, u8 *pbuf)
{
	struct reportpwrstate_parm *preportpwrstate = (struct reportpwrstate_parm *)pbuf;

_func_enter_;

	RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_cpwm_event_callback !!!\n"));
#ifdef CONFIG_PWRCTRL
	preportpwrstate->state |= (u8)(adapter->pwrctrlpriv.cpwm_tog + 0x80);
	cpwm_int_hdl(adapter, preportpwrstate);
#endif

_func_exit_;

}


void _rtw_sitesurvey_ctrl_handler (_adapter *adapter)
{
	struct	mlme_priv *pmlmepriv = &adapter->mlmepriv;
	struct	sitesurvey_ctrl	*psitesurveyctrl=&pmlmepriv->sitesurveyctrl;
	struct	registry_priv	*pregistrypriv=&adapter->registrypriv;
	u64 current_tx_pkts;
	u64 current_rx_pkts;
	
_func_enter_;		
	
	current_tx_pkts=(adapter->xmitpriv.tx_pkts)-(psitesurveyctrl->last_tx_pkts);
	current_rx_pkts=(adapter->recvpriv.rx_pkts)-(psitesurveyctrl->last_rx_pkts);

	psitesurveyctrl->last_tx_pkts=adapter->xmitpriv.tx_pkts;
	psitesurveyctrl->last_rx_pkts=adapter->recvpriv.rx_pkts;

	if( (current_tx_pkts>pregistrypriv->busy_thresh)||(current_rx_pkts>pregistrypriv->busy_thresh)) 
	{	
		//printk("%s traffic_busy = true,Curr_tx(%lld),Curr_rx(%lld)\n",__FUNCTION__,current_tx_pkts,current_rx_pkts);
		psitesurveyctrl->traffic_busy= _TRUE;
	}
	else 
	{		
		psitesurveyctrl->traffic_busy= _FALSE;
	}
	
_func_exit_;	

}

void _rtw_join_timeout_handler (_adapter *adapter)
{
	_irqL irqL;
	struct	mlme_priv *pmlmepriv = &adapter->mlmepriv;

#if 0
	if (adapter->bDriverStopped == _TRUE){
		_rtw_up_sema(&pmlmepriv->assoc_terminate);
		return;
	}
#endif	

_func_enter_;		

	RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n^^^rtw_join_timeout_handler ^^^fw_state=%x\n", pmlmepriv->fw_state));
	RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("+rtw_join_timeout_handler, fw_state=%x\n", pmlmepriv->fw_state));	
	
	if(adapter->bDriverStopped ||adapter->bSurpriseRemoved)
		return;

	
	_enter_critical(&pmlmepriv->lock, &irqL);


	if((check_fwstate(pmlmepriv, _FW_UNDER_LINKING)) == _TRUE)
	{
		
                RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("clear _FW_UNDER_LINKING ^^^fw_state=%x\n", pmlmepriv->fw_state));
		pmlmepriv->fw_state ^= _FW_UNDER_LINKING;                

	}

	if((check_fwstate(pmlmepriv, _FW_LINKED)) == _TRUE)
	{
		rtw_os_indicate_disconnect(adapter);
		pmlmepriv->fw_state ^= _FW_LINKED;
	}

	free_scanqueue(pmlmepriv);//???
	adapter->ledpriv.LedControlHandler(adapter, LED_CTL_NO_LINK);
 
	_exit_critical(&pmlmepriv->lock, &irqL);
	
_func_exit_;

}

void rtw_scan_timeout_handler (_adapter *adapter)
{	
	_irqL irqL;
	struct	mlme_priv *pmlmepriv = &adapter->mlmepriv;
	
	RT_TRACE(_module_rtl871x_mlme_c_, _drv_err_, ("\n^^^rtw_scan_timeout_handler ^^^fw_state=%x\n", pmlmepriv->fw_state));

	_enter_critical(&pmlmepriv->lock, &irqL);
	
	if (pmlmepriv->fw_state & _FW_UNDER_SURVEY)
	{		
		pmlmepriv->fw_state ^= _FW_UNDER_SURVEY;
	}
	else 
	{	
		RT_TRACE(_module_rtl871x_mlme_c_, _drv_err_, ("fw status =%x, rtw_scan_timeout_handler: survey done event comes too late!\n", pmlmepriv->fw_state));	
	}	

	_exit_critical(&pmlmepriv->lock, &irqL);
	
}


void dynamic_check_timer_handlder(_adapter *adapter)
{
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;

	if(adapter->hw_init_completed == _FALSE)
		return;

	if ((adapter->bDriverStopped == _TRUE)||(adapter->bSurpriseRemoved== _TRUE))
		return;

	if(adapter->net_closed == _TRUE)
		return;

	dynamic_chk_wk_cmd(adapter);

	//dm_ctrl_wk_cmd(adapter);

	//pbc_polling_wk_cmd(adapter);

	if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)
		_rtw_sitesurvey_ctrl_handler(adapter);


	if(pmlmepriv->scan_interval >0)
		pmlmepriv->scan_interval--;

#ifdef CONFIG_NATIVEAP_MLME
	if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
	{
		expire_timeout_chk(adapter);
	}	
#endif
	
}


/*
Calling context:
The caller of the sub-routine will be in critical section...

The caller must hold the following spinlock

pmlmepriv->lock


*/
int rtw_select_and_join_from_scanned_queue(struct mlme_priv *pmlmepriv )
{	
	_list	*phead;
	unsigned char *dst_ssid, *src_ssid;
	_adapter *adapter;	
	HAL_DATA_TYPE	*pHalData;
	_queue	*queue	= &(pmlmepriv->scanned_queue);
	struct	wlan_network	*pnetwork = NULL;
	struct	wlan_network	*pnetwork_max_rssi = NULL;
	
	phead = get_list_head(queue);		
	adapter = (_adapter *)pmlmepriv->nic_hdl;
	pHalData = GET_HAL_DATA(adapter);
	
       pmlmepriv->pscanned = get_next( phead );
_func_enter_;	

	while (1)
	{			
		if ((rtw_end_of_queue_search(phead, pmlmepriv->pscanned)) == _TRUE)
		{
			if((pmlmepriv->assoc_by_rssi==_TRUE)  && (pnetwork_max_rssi!=NULL))
			{
				pnetwork = pnetwork_max_rssi;
				goto ask_for_joinbss;
			}				
		
			RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("(1)rtw_select_and_join_from_scanned_queue return _FAIL\n"));
			return _FAIL;	
		}

		pnetwork = LIST_CONTAINOR(pmlmepriv->pscanned, struct wlan_network, list);
		if(pnetwork==NULL){
			RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("(2)rtw_select_and_join_from_scanned_queue return _FAIL:(pnetwork==NULL)\n"));
			return _FAIL;	
		}

		
		pmlmepriv->pscanned = get_next(pmlmepriv->pscanned);

		if(pmlmepriv->assoc_by_bssid==_TRUE)
		{
			dst_ssid = pnetwork->network.MacAddress;
			src_ssid = pmlmepriv->assoc_bssid;
			
			if(_rtw_memcmp(dst_ssid, src_ssid, ETH_ALEN)==_TRUE)
			{
				//remove the condition @ 20081125
				//if((pmlmepriv->cur_network.network.InfrastructureMode==Ndis802_11AutoUnknown)||
				//	pmlmepriv->cur_network.network.InfrastructureMode == pnetwork->network.InfrastructureMode)
				//		goto ask_for_joinbss;
				
				if (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)
				{
					if(is_same_network(&pmlmepriv->cur_network.network, &pnetwork->network))
					{
						//DBG_871X("select_and_join(1): _FW_LINKED and is same network, it needn't join again\n");

						rtw_indicate_connect(adapter);//rtw_indicate_connect again
							
						return 2;
					}
					else
					{
						rtw_disassoc_cmd(adapter);
			
						rtw_indicate_disconnect(adapter);

						rtw_free_assoc_resources(adapter);

						goto ask_for_joinbss;
						
					}
				}
				else
				{
					goto ask_for_joinbss;
				}
							
			}
			
		}		
		else if (pmlmepriv->assoc_ssid.SsidLength == 0)
		{			
			goto ask_for_joinbss;//anyway, join first selected(dequeued) pnetwork if ssid_len=0				
		}
	
		
		dst_ssid = pnetwork->network.Ssid.Ssid;
		src_ssid = pmlmepriv->assoc_ssid.Ssid;
/*
#ifdef CONFIG_ANTENNA_DIVERSITY
			printk("#### dst_ssid=(%s) Opt_Ant_(%s) , cur_Ant(%s)\n", dst_ssid,
				(2==pnetwork->network.PhyInfo.Optimum_antenna)?"A":"B",
				(2==pHalData->CurAntenna)?"A":"B");
#endif
*/
		if (((_rtw_memcmp(dst_ssid, src_ssid, pmlmepriv->assoc_ssid.SsidLength)) == _TRUE)&&
			(pnetwork->network.Ssid.SsidLength==pmlmepriv->assoc_ssid.SsidLength))
		{
			RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("dst_ssid=%s, src_ssid=%s \n", dst_ssid, src_ssid));
#ifdef CONFIG_ANTENNA_DIVERSITY
			printk("#### dst_ssid=(%s) Opt_Ant_(%s) , cur_Ant(%s)\n", dst_ssid,
				(2==pnetwork->network.PhyInfo.Optimum_antenna)?"A":"B",
				(2==pHalData->CurAntenna)?"A":"B");
#endif
			//remove the condition @ 20081125
			//if((pmlmepriv->cur_network.network.InfrastructureMode==Ndis802_11AutoUnknown)||
			//	pmlmepriv->cur_network.network.InfrastructureMode == pnetwork->network.InfrastructureMode)
			//{
			//	_rtw_memcpy(pmlmepriv->assoc_bssid, pnetwork->network.MacAddress, ETH_ALEN);
			//	goto ask_for_joinbss;
			//}

			if(pmlmepriv->assoc_by_rssi==_TRUE)//if the ssid is the same, select the bss which has the max rssi
			{
				if(pnetwork_max_rssi)
				{
					if(pnetwork->network.Rssi > pnetwork_max_rssi->network.Rssi)
						pnetwork_max_rssi = pnetwork;					
				}
				else
				{
					pnetwork_max_rssi = pnetwork;
				}				
				
			}
			else if(is_desired_network(adapter, pnetwork) == _TRUE)
			{
				if (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)
				{
#if 0				
					if(is_same_network(&pmlmepriv->cur_network.network, &pnetwork->network))
					{
						DBG_871X("select_and_join(2): _FW_LINKED and is same network, it needn't join again\n");
						
						rtw_indicate_connect(adapter);//rtw_indicate_connect again
						
						return 2;
					}
					else
#endif						
					{
						rtw_disassoc_cmd(adapter);
			
						//rtw_indicate_disconnect(adapter);//

						rtw_free_assoc_resources(adapter);

						goto ask_for_joinbss;						
					}
				}
				else
				{				
					goto ask_for_joinbss;
				}				

			}
		
			
		}
 	
 	}

_func_exit_;	

     return _FAIL;

ask_for_joinbss:
	
_func_exit_;

	return rtw_joinbss_cmd(adapter, pnetwork);

}



sint rtw_set_auth(_adapter * adapter,struct security_priv *psecuritypriv)
{
	struct	cmd_obj* pcmd;
	struct 	setauth_parm *psetauthparm;
	struct	cmd_priv	*pcmdpriv=&(adapter->cmdpriv);
	sint		res=_SUCCESS;
	
_func_enter_;	

	pcmd = (struct	cmd_obj*)_rtw_malloc(sizeof(struct	cmd_obj));
	if(pcmd==NULL){
		res= _FAIL;  //try again
		goto exit;
	}
	
	psetauthparm=(struct setauth_parm*)_rtw_malloc(sizeof(struct setauth_parm));
	if(psetauthparm==NULL){
		_rtw_mfree((unsigned char *)pcmd, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	_rtw_memset(psetauthparm, 0, sizeof(struct setauth_parm));
	psetauthparm->mode=(unsigned char)psecuritypriv->dot11AuthAlgrthm;
	
	pcmd->cmdcode = _SetAuth_CMD_;
	pcmd->parmbuf = (unsigned char *)psetauthparm;   
	pcmd->cmdsz =  (sizeof(struct setauth_parm));  
	pcmd->rsp = NULL;
	pcmd->rspsz = 0;


	_rtw_init_listhead(&pcmd->list);

	RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("after enqueue set_auth_cmd, auth_mode=%x\n", psecuritypriv->dot11AuthAlgrthm));

	rtw_enqueue_cmd(pcmdpriv, pcmd);

exit:

_func_exit_;

	return _SUCCESS;

}


sint rtw_set_key(_adapter * adapter,struct security_priv *psecuritypriv,sint keyid)
{
	u8 keylen;
	struct	cmd_obj* pcmd;
	struct 	setkey_parm *psetkeyparm;
	struct	cmd_priv	*pcmdpriv=&(adapter->cmdpriv);
	struct 	mlme_ext_info *pmlmeinfo = &adapter->mlmeextpriv.mlmext_info;
	sint res=_SUCCESS;
	
_func_enter_;
	
	pcmd = (struct	cmd_obj*)_rtw_malloc(sizeof(struct	cmd_obj));
	if(pcmd==NULL){
		res= _FAIL;  //try again
		goto exit;
	}
	psetkeyparm=(struct setkey_parm*)_rtw_malloc(sizeof(struct setkey_parm));
	if(psetkeyparm==NULL){
		_rtw_mfree((unsigned char *)pcmd, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	_rtw_memset(psetkeyparm, 0, sizeof(struct setkey_parm));

	if(psecuritypriv->dot11AuthAlgrthm ==dot11AuthAlgrthm_8021X){		
		psetkeyparm->algorithm=(unsigned char)psecuritypriv->dot118021XGrpPrivacy;	
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n rtw_set_key: psetkeyparm->algorithm=(unsigned char)psecuritypriv->dot118021XGrpPrivacy=%d \n", psetkeyparm->algorithm));
	}	
	else{
		psetkeyparm->algorithm=(u8)psecuritypriv->dot11PrivacyAlgrthm;
		RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n rtw_set_key: psetkeyparm->algorithm=(u8)psecuritypriv->dot11PrivacyAlgrthm=%d \n", psetkeyparm->algorithm));

	}
	psetkeyparm->keyid=(u8)keyid;//0~3
	pmlmeinfo->key_mask |= BIT(psetkeyparm->keyid);
#ifdef CONFIG_AUTOSUSPEND
	if( _TRUE  == adapter->pwrctrlpriv.bInternalAutoSuspend)
	{
		adapter->pwrctrlpriv.wepkeymask = pmlmeinfo->key_mask;
		printk("....AutoSuspend pwrctrlpriv.wepkeymask(%x)\n",adapter->pwrctrlpriv.wepkeymask);
	}
#endif
	printk("==> rtw_set_key algorithm(%x),keyid(%x),key_mask(%x)\n",psetkeyparm->algorithm,psetkeyparm->keyid,pmlmeinfo->key_mask);
	RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n rtw_set_key: psetkeyparm->algorithm=%d psetkeyparm->keyid=(u8)keyid=%d \n",psetkeyparm->algorithm, keyid));

	switch(psetkeyparm->algorithm){
		
		case _WEP40_:
			keylen=5;
			_rtw_memcpy(&(psetkeyparm->key[0]), &(psecuritypriv->dot11DefKey[keyid].skey[0]), keylen);
			break;
		case _WEP104_:
			keylen=13;
			_rtw_memcpy(&(psetkeyparm->key[0]), &(psecuritypriv->dot11DefKey[keyid].skey[0]), keylen);
			break;
		case _TKIP_:
			keylen=16;			
			_rtw_memcpy(&psetkeyparm->key, &psecuritypriv->dot118021XGrpKey[keyid-1], keylen);
			psetkeyparm->grpkey=1;
			break;
		case _AES_:
			keylen=16;			
			_rtw_memcpy(&psetkeyparm->key, &psecuritypriv->dot118021XGrpKey[keyid-1], keylen);
			psetkeyparm->grpkey=1;
			break;
		default:
			RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n rtw_set_key:psecuritypriv->dot11PrivacyAlgrthm = %x (must be 1 or 2 or 4 or 5)\n",psecuritypriv->dot11PrivacyAlgrthm));
			res= _FAIL;
			goto exit;
	}

	
	pcmd->cmdcode = _SetKey_CMD_;
	pcmd->parmbuf = (u8 *)psetkeyparm;   
	pcmd->cmdsz =  (sizeof(struct setkey_parm));  
	pcmd->rsp = NULL;
	pcmd->rspsz = 0;


	_rtw_init_listhead(&pcmd->list);

	//_rtw_init_sema(&(pcmd->cmd_sem), 0);

	rtw_enqueue_cmd(pcmdpriv, pcmd);

exit:
_func_exit_;
	return _SUCCESS;

}


//adjust IEs for rtw_joinbss_cmd in WMM
int rtw_restruct_wmm_ie(_adapter *adapter, u8 *in_ie, u8 *out_ie, uint in_len, uint initial_out_len)
{
	unsigned	int ielength=0;
	unsigned int i, j;

	i = 12; //after the fixed IE
	while(i<in_len)
	{
		ielength = initial_out_len;		
		
		if(in_ie[i] == 0xDD && in_ie[i+2] == 0x00 && in_ie[i+3] == 0x50  && in_ie[i+4] == 0xF2 && in_ie[i+5] == 0x02 && i+5 < in_len) //WMM element ID and OUI
		{

			//Append WMM IE to the last index of out_ie
			/*
			for(j=i; j< i+(in_ie[i+1]+2); j++)
			{
				out_ie[ielength] = in_ie[j];				
				ielength++;
			}
			out_ie[initial_out_len+8] = 0x00; //force the QoS Info Field to be zero
	                */
                       
                        for ( j = i; j < i + 9; j++ )
                        {
                            out_ie[ ielength] = in_ie[ j ];
                            ielength++;
                        } 
                        out_ie[ initial_out_len + 1 ] = 0x07;
                        out_ie[ initial_out_len + 6 ] = 0x00;
                        out_ie[ initial_out_len + 8 ] = 0x00;
	
			break;
		}

		i+=(in_ie[i+1]+2); // to the next IE element
	}
	
	return ielength;
	
}


//
// Ported from 8185: IsInPreAuthKeyList(). (Renamed from SecIsInPreAuthKeyList(), 2006-10-13.)
// Added by Annie, 2006-05-07.
//
// Search by BSSID,
// Return Value:
//		-1 		:if there is no pre-auth key in the  table
//		>=0		:if there is pre-auth key, and   return the entry id
//
//

static int SecIsInPMKIDList(_adapter *Adapter, u8 *bssid)
{
	struct security_priv *psecuritypriv=&Adapter->securitypriv;
	int i=0;

	do
	{
		if( ( psecuritypriv->PMKIDList[i].bUsed ) && 
                    (  _rtw_memcmp( psecuritypriv->PMKIDList[i].Bssid, bssid, ETH_ALEN ) == _TRUE ) )
		{
			break;
		}
		else
		{	
			i++;
			//continue;
		}
		
	}while(i<NUM_PMKID_CACHE);

	if( i == NUM_PMKID_CACHE )
	{ 
		i = -1;// Could not find.
	}
	else
	{ 
		// There is one Pre-Authentication Key for the specific BSSID.
	}

	return (i);
	
}

sint rtw_restruct_sec_ie(_adapter *adapter,u8 *in_ie, u8 *out_ie, uint in_len)
{
	u8 authmode, securitytype, match;
	u8 sec_ie[255], uncst_oui[4], bkup_ie[255];
	u8 wpa_oui[4]={0x0, 0x50, 0xf2, 0x01};
	uint 	ielength, cnt, remove_cnt;
	int iEntry;

	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;
	struct security_priv *psecuritypriv=&adapter->securitypriv;
	uint 	ndisauthmode=psecuritypriv->ndisauthtype;
	uint ndissecuritytype = psecuritypriv->ndisencryptstatus;
	
_func_enter_;

	RT_TRACE(_module_rtl871x_mlme_c_, _drv_notice_,
		 ("+rtw_restruct_sec_ie: ndisauthmode=%d ndissecuritytype=%d\n",
		  ndisauthmode, ndissecuritytype));
	  
	authmode = 0xFF;//init

	if((ndisauthmode==Ndis802_11AuthModeWPA)||(ndisauthmode==Ndis802_11AuthModeWPAPSK))
	{
		authmode=_WPA_IE_ID_;
		uncst_oui[0]=0x0;
		uncst_oui[1]=0x50;
		uncst_oui[2]=0xf2;
	}
	if((ndisauthmode==Ndis802_11AuthModeWPA2)||(ndisauthmode==Ndis802_11AuthModeWPA2PSK))
	{	
		authmode=_WPA2_IE_ID_;
		uncst_oui[0]=0x0;
		uncst_oui[1]=0x0f;
		uncst_oui[2]=0xac;
	}
	
	switch(ndissecuritytype)
	{
		case Ndis802_11Encryption1Enabled:
		case Ndis802_11Encryption1KeyAbsent:
				securitytype=_WEP40_;
				uncst_oui[3]=0x1;
				break;
		case Ndis802_11Encryption2Enabled:
		case Ndis802_11Encryption2KeyAbsent:
				securitytype=_TKIP_;
				uncst_oui[3]=0x2;
				break;
		case Ndis802_11Encryption3Enabled:
		case Ndis802_11Encryption3KeyAbsent: 	
				securitytype=_AES_;
				uncst_oui[3]=0x4;
				break;
		default:
				securitytype=_NO_PRIVACY_;
				break;				
	}
		
	//Search required WPA or WPA2 IE and copy to sec_ie[ ]
	cnt=12;
	match=_FALSE;
	while(cnt<in_len)
	{
		if(in_ie[cnt]==authmode)
		{
			if((authmode==_WPA_IE_ID_)&&(_rtw_memcmp(&in_ie[cnt+2], &wpa_oui[0], 4)==_TRUE))
			{
				_rtw_memcpy(&sec_ie[0], &in_ie[cnt], in_ie[cnt+1]+2);
				match=_TRUE;
				RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_restruct_sec_ie: Get WPA IE from %d in_len=%d \n",cnt,in_len));
				break;
			}
			if(authmode==_WPA2_IE_ID_)
			{
				_rtw_memcpy(&sec_ie[0], &in_ie[cnt], in_ie[cnt+1]+2);
				match=_TRUE;
				RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_restruct_sec_ie: Get WPA2 IE from %d in_len=%d \n",cnt,in_len));
				break;
			}	
			if(((authmode==_WPA_IE_ID_)&&(_rtw_memcmp(&in_ie[cnt+2], &wpa_oui[0], 4)==_TRUE))||(authmode==_WPA2_IE_ID_))
			{
				_rtw_memcpy(&bkup_ie[0], &in_ie[cnt], in_ie[cnt+1]+2);
				RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("rtw_restruct_sec_ie: cnt=%d in_len=%d \n",cnt,in_len));
			}
		}
	
		cnt += in_ie[cnt+1] + 2; //get next
	}
	
	//restruct WPA IE or WPA2 IE in sec_ie[ ]
	if(match==_TRUE)
	{
		if(sec_ie[0]==_WPA_IE_ID_)
		{
			// parsing SSN IE to select required encryption algorithm, and set the bc/mc encryption algorithm
			while(_TRUE)
			{
				if(_rtw_memcmp(&sec_ie[2], &wpa_oui[0], 4) !=_TRUE)//check wpa_oui tag
				{  	 
					RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n SSN IE but doesn't has wpa_oui tag! \n"));
					match=_FALSE;
					break;
				}
				
				if((sec_ie[6]!=0x01) ||(sec_ie[7]!= 0x0))
				{ 	
					//IE Ver error
					RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n SSN IE :IE version error (%.2x %.2x != 01 00 )! \n",sec_ie[6],sec_ie[7]));
					match=_FALSE;
					break;
				}
				
				if(_rtw_memcmp(&sec_ie[8], &wpa_oui[0], 3) ==_TRUE)
				{ 
					//get bc/mc encryption type (group key tyep)
					switch(sec_ie[11])
					{
						case 0x0: //none
							psecuritypriv->dot118021XGrpPrivacy=_NO_PRIVACY_;
							break;
						case 0x1: //WEP_40
							psecuritypriv->dot118021XGrpPrivacy=_WEP40_;
							break;
						case 0x2: //TKIP
							psecuritypriv->dot118021XGrpPrivacy=_TKIP_;
							break;
						case 0x3: //AESCCMP
						case 0x4: 
							psecuritypriv->dot118021XGrpPrivacy=_AES_;
							break;
						case 0x5: //WEP_104	
							psecuritypriv->dot118021XGrpPrivacy=_WEP104_;
							break;
					}
					
				}
				else
				{
					RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n SSN IE :Multicast error (%.2x %.2x %.2x %.2x != 00 50 F2 xx )! \n",
									sec_ie[8],sec_ie[9],sec_ie[10],sec_ie[11]));
					match =_FALSE;
					break;
				}
				
				if(sec_ie[12]==0x01)
				{ 
					//check the unicast encryption type
					if(_rtw_memcmp(&sec_ie[14], &uncst_oui[0], 4) !=_TRUE)
					{
						RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n SSN IE :Unicast error (%.2x %.2x %.2x %.2x != 00 50 F2 %.2x )! \n",
										sec_ie[14],sec_ie[15],sec_ie[16],sec_ie[17],uncst_oui[3]));
						match =_FALSE;
						
						break;
						
					} //else the uncst_oui is match
				}
				else//mixed mode, unicast_enc_type > 1
				{
					//select the uncst_oui and remove the other uncst_oui
					cnt=sec_ie[12];
					remove_cnt=(cnt-1)*4;
					sec_ie[12]=0x01;
					_rtw_memcpy(&sec_ie[14], &uncst_oui[0], 4);
					
					//remove the other unicast suit
					_rtw_memcpy(&sec_ie[18], &sec_ie[18+remove_cnt],(sec_ie[1]-18+2-remove_cnt));
					sec_ie[1]=sec_ie[1]-remove_cnt;
				}
				
				break;				
			}			
		}

		if(authmode==_WPA2_IE_ID_)
		{
			// parsing RSN IE to select required encryption algorithm, and set the bc/mc encryption algorithm
			while(_TRUE)
			{
				if((sec_ie[2]!=0x01)||(sec_ie[3]!= 0x0))
				{ 
					//IE Ver error
					RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n RSN IE :IE version error (%.2x %.2x != 01 00 )! \n",sec_ie[2],sec_ie[3]));
					match=_FALSE;
					break;
				}
				
				if(_rtw_memcmp(&sec_ie[4], &uncst_oui[0], 3) ==_TRUE)
				{ 
					//get bc/mc encryption type
					switch(sec_ie[7])
					{
						case 0x1: //WEP_40
							psecuritypriv->dot118021XGrpPrivacy=_WEP40_;
							break;
						case 0x2: //TKIP
							psecuritypriv->dot118021XGrpPrivacy=_TKIP_;
							break;
						case 0x4: //AESWRAP
							psecuritypriv->dot118021XGrpPrivacy=_AES_;
							break;
						case 0x5: //WEP_104	
							psecuritypriv->dot118021XGrpPrivacy=_WEP104_;
							break;
						default: //none
							psecuritypriv->dot118021XGrpPrivacy=_NO_PRIVACY_;
							break;	
					}
				}
				else
				{
					RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n RSN IE :Multicast error (%.2x %.2x %.2x %.2x != 00 50 F2 xx )! \n",
								sec_ie[4],sec_ie[5],sec_ie[6],sec_ie[7]));
					match=_FALSE;
					break;
				}
				
				if(sec_ie[8]==0x01)
				{ 
					//check the unicast encryption type
					if(_rtw_memcmp(&sec_ie[10], &uncst_oui[0],4) !=_TRUE)
					{
						RT_TRACE(_module_rtl871x_mlme_c_,_drv_err_,("\n SSN IE :Unicast error (%.2x %.2x %.2x %.2x != 00 50 F2 xx )! \n",
									sec_ie[10],sec_ie[11],sec_ie[12],sec_ie[13]));

						match =_FALSE;						
						break;
						
					} //else the uncst_oui is match
				}
				else //mixed mode, unicast_enc_type > 1
				{ 
					//select the uncst_oui and remove the other uncst_oui
					cnt=sec_ie[8];
					remove_cnt=(cnt-1)*4;
					sec_ie[8]=0x01;
					_rtw_memcpy( &sec_ie[10] , &uncst_oui[0],4 );
					
					//remove the other unicast suit
					_rtw_memcpy(&sec_ie[14],&sec_ie[14+remove_cnt],(sec_ie[1]-14+2-remove_cnt));
					sec_ie[1]=sec_ie[1]-remove_cnt;
				}

				break;				
			}			
		}

	}

	//copy fixed ie only
	_rtw_memcpy(out_ie, in_ie,12);
	ielength=12;
		
	if(psecuritypriv->wps_phase == _TRUE)
	{
		//DBG_871X("wps_phase == _TRUE\n");

		_rtw_memcpy(out_ie+ielength, psecuritypriv->wps_ie, psecuritypriv->wps_ie_len);
		
		ielength += psecuritypriv->wps_ie_len;
		psecuritypriv->wps_phase = _FALSE;
	
	}
	else if((authmode==_WPA_IE_ID_)||(authmode==_WPA2_IE_ID_))
	{		
		//copy RSN or SSN		
		if(match ==_TRUE)
		{		
			_rtw_memcpy(&out_ie[ielength], &sec_ie[0], sec_ie[1]+2);
			ielength+=sec_ie[1]+2;
			
			if(authmode==_WPA2_IE_ID_)
			{
				//the Pre-Authentication bit should be zero, john
				out_ie[ielength-1]= 0;
				out_ie[ielength-2]= 0;
			}

			rtw_report_sec_ie(adapter, authmode, sec_ie);
	
#ifdef CONFIG_DRVEXT_MODULE
			drvext_report_sec_ie(&adapter->drvextpriv, authmode, sec_ie);	
#endif
			
		}
		
	}
	else
	{
	
	}
	
	iEntry = SecIsInPMKIDList(adapter, pmlmepriv->assoc_bssid);
	if(iEntry<0)
	{
		return ielength;
	}
	else
	{
		if(authmode == _WPA2_IE_ID_)
		{
			out_ie[ielength]=1;
			ielength++;
			out_ie[ielength]=0;	//PMKID count = 0x0100
			ielength++;
			_rtw_memcpy(	&out_ie[ielength], &psecuritypriv->PMKIDList[iEntry].PMKID, 16);
		
			ielength+=16;
			out_ie[13]+=18;//PMKID length = 2+16
		}
	}
	
	//rtw_report_sec_ie(adapter, authmode, sec_ie);

_func_exit_;
	
	return ielength;	
}

void rtw_init_registrypriv_dev_network(	_adapter* adapter)
{
	struct registry_priv* pregistrypriv = &adapter->registrypriv;
	struct eeprom_priv* peepriv = &adapter->eeprompriv;
	WLAN_BSSID_EX    *pdev_network = &pregistrypriv->dev_network;
	u8 *myhwaddr = myid(peepriv);
	
_func_enter_;

	_rtw_memcpy(pdev_network->MacAddress, myhwaddr, ETH_ALEN);

	_rtw_memcpy(&pdev_network->Ssid, &pregistrypriv->ssid, sizeof(NDIS_802_11_SSID));
	
	pdev_network->Configuration.Length=sizeof(NDIS_802_11_CONFIGURATION);
	pdev_network->Configuration.BeaconPeriod = 100;	
	pdev_network->Configuration.FHConfig.Length = 0;
	pdev_network->Configuration.FHConfig.HopPattern = 0;
	pdev_network->Configuration.FHConfig.HopSet = 0;
	pdev_network->Configuration.FHConfig.DwellTime = 0;
	
	
_func_exit_;	
	
}

void rtw_update_registrypriv_dev_network(_adapter* adapter) 
{
	int sz=0;
	struct registry_priv* pregistrypriv = &adapter->registrypriv;	
	WLAN_BSSID_EX    *pdev_network = &pregistrypriv->dev_network;
	struct	security_priv*	psecuritypriv = &adapter->securitypriv;
	struct	wlan_network	*cur_network = &adapter->mlmepriv.cur_network;
	struct	xmit_priv	*pxmitpriv = &adapter->xmitpriv;

_func_enter_;

#if 0
	pxmitpriv->vcs_setting = pregistrypriv->vrtl_carrier_sense;
	pxmitpriv->vcs = pregistrypriv->vcs_type;
	pxmitpriv->vcs_type = pregistrypriv->vcs_type;
	pxmitpriv->rts_thresh = pregistrypriv->rts_thresh;
	pxmitpriv->frag_len = pregistrypriv->frag_thresh;
	
	adapter->qospriv.qos_option = pregistrypriv->wmm_enable;
#endif	

	pdev_network->Privacy = (psecuritypriv->dot11PrivacyAlgrthm > 0 ? 1 : 0) ; // adhoc no 802.1x

	pdev_network->Rssi = 0;

	switch(pregistrypriv->wireless_mode)
	{
		case WIRELESS_11B:
			pdev_network->NetworkTypeInUse = (Ndis802_11DS);
			break;	
		case WIRELESS_11G:
		case WIRELESS_11BG:
		case WIRELESS_11N:	
		case WIRELESS_11GN:
		case WIRELESS_11BGN:	
			pdev_network->NetworkTypeInUse = (Ndis802_11OFDM24);
			break;
		case WIRELESS_11A:
			pdev_network->NetworkTypeInUse = (Ndis802_11OFDM5);
			break;
		default :
			// TODO
			break;
	}
	
	pdev_network->Configuration.DSConfig = (pregistrypriv->channel);
	RT_TRACE(_module_rtl871x_mlme_c_,_drv_info_,("pregistrypriv->channel=%d, pdev_network->Configuration.DSConfig=0x%x\n", pregistrypriv->channel, pdev_network->Configuration.DSConfig));	

	if(cur_network->network.InfrastructureMode == Ndis802_11IBSS)
			pdev_network->Configuration.ATIMWindow = (0);

	pdev_network->InfrastructureMode = (cur_network->network.InfrastructureMode);

	// 1. Supported rates
	// 2. IE

	//rtw_set_supported_rate(pdev_network->SupportedRates, pregistrypriv->wireless_mode) ; // will be called in rtw_generate_ie
	sz = rtw_generate_ie(pregistrypriv);

	pdev_network->IELength = sz;

	pdev_network->Length =get_WLAN_BSSID_EX_sz((WLAN_BSSID_EX  *)pdev_network);

	//notes: translate IELength & Length after assign the Length to cmdsz in rtw_createbss_cmd();
	//pdev_network->IELength = cpu_to_le32(sz);
		
_func_exit_;	

}

void rtw_get_encrypt_decrypt_from_registrypriv(	_adapter* adapter)
{
	u16	wpaconfig=0;
	struct registry_priv* pregistrypriv = &adapter->registrypriv;
	struct security_priv* psecuritypriv= &adapter->securitypriv;
_func_enter_;


_func_exit_;	
	
}

//the fucntion is at passive_level 
void rtw_joinbss_reset(_adapter *padapter)
{
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;

#ifdef CONFIG_80211N_HT	
	struct ht_priv		*phtpriv = &pmlmepriv->htpriv;
#endif

	//todo: if you want to do something io/reg/hw setting before join_bss, please add code here
	



#ifdef CONFIG_80211N_HT

	phtpriv->ampdu_enable = _FALSE;//reset to disabled	

	if(phtpriv->ht_option)
	{
	
#ifdef CONFIG_USB_HCI	
		//validate  usb rx aggregation
		//rtw_write8(padapter, 0x102500D9, 48);//TH = 48 pages, 6k
		rtw_write8(padapter, REG_RXDMA_AGG_PG_TH, 48);
		
#endif

	}
	else
	{

#ifdef CONFIG_USB_HCI	
	//invalidate  usb rx aggregation
		//rtw_write8(padapter, 0x102500D9, 1);// TH=1 => means that invalidate usb rx aggregation
		rtw_write8(padapter, REG_RXDMA_AGG_PG_TH, 1);
#endif

	}
	
#endif	

}


#ifdef CONFIG_80211N_HT

//the fucntion is >= passive_level 
unsigned int rtw_restructure_ht_ie(_adapter *padapter, u8 *in_ie, u8 *out_ie, uint in_len, uint *pout_len)
{
	u32 ielen, out_len;
	unsigned char *p, *pframe;
	struct ieee80211_ht_cap ht_capie;
	unsigned char WMM_IE[] = {0x00, 0x50, 0xf2, 0x02, 0x00, 0x01, 0x00};
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	struct qos_priv   	*pqospriv= &pmlmepriv->qospriv;
	struct ht_priv		*phtpriv = &pmlmepriv->htpriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	phtpriv->ht_option = 0;

	p = rtw_get_ie(in_ie+12, _HT_CAPABILITY_IE_, &ielen, in_len-12);

	if(p && ielen>0)
	{
		if(pqospriv->qos_option == 0)
		{
			out_len = *pout_len;
			pframe = rtw_set_ie(out_ie+out_len, _VENDOR_SPECIFIC_IE_, 
								_WMM_IE_Length_, WMM_IE, pout_len);

			pqospriv->qos_option = 1;
		}

		out_len = *pout_len;

		_rtw_memset(&ht_capie, 0, sizeof(struct ieee80211_ht_cap));

		ht_capie.cap_info = IEEE80211_HT_CAP_SUP_WIDTH |IEEE80211_HT_CAP_SGI_20 |
							IEEE80211_HT_CAP_SGI_40 | IEEE80211_HT_CAP_TX_STBC |
							IEEE80211_HT_CAP_MAX_AMSDU | IEEE80211_HT_CAP_DSSSCCK40;
		
		ht_capie.ampdu_params_info = (IEEE80211_HT_CAP_AMPDU_FACTOR&0x03) |
										(IEEE80211_HT_CAP_AMPDU_DENSITY&0x00) ; 

		
		pframe = rtw_set_ie(out_ie+out_len, _HT_CAPABILITY_IE_, 
							sizeof(struct ieee80211_ht_cap), (unsigned char*)&ht_capie, pout_len);


		//_rtw_memcpy(out_ie+out_len, p, ielen+2);//gtest
		//*pout_len = *pout_len + (ielen+2);

							
		phtpriv->ht_option = 1;
		

		//spec case only for cisco's ap because cisco's ap issue assoc rsp using mcs rate @40MHz or @20MHz	
		pmlmeinfo->assoc_AP_vendor = check_assoc_AP(in_ie, in_len);
		if(pmlmeinfo->assoc_AP_vendor == ciscoAP)
		{		
			p = rtw_get_ie(in_ie+12, _HT_ADD_INFO_IE_, &ielen, in_len-12);
			if(p && (ielen==sizeof(struct ieee80211_ht_addt_info)))
			{
				out_len = *pout_len;		
				pframe = rtw_set_ie(out_ie+out_len, _HT_ADD_INFO_IE_, ielen, p+2 , pout_len);
			}	
	}
	
	}
	
	return (phtpriv->ht_option);
	
}

//the fucntion is > passive_level (in critical_section)
void rtw_update_ht_cap(_adapter *padapter, u8 *pie, uint ie_len)
{	
	u8 *p, max_ampdu_sz;
	int i, len;		
	struct sta_info *bmc_sta, *psta;	
	struct ieee80211_ht_cap *pht_capie;
	struct ieee80211_ht_addt_info *pht_addtinfo;
	struct recv_reorder_ctrl *preorder_ctrl;
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	struct ht_priv		*phtpriv = &pmlmepriv->htpriv;
	struct recv_priv *precvpriv = &padapter->recvpriv;
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
	struct wlan_network *pcur_network = &(pmlmepriv->cur_network);
	

	if(!phtpriv->ht_option)
		return;


	//printk("+rtw_update_ht_cap()\n");

	//maybe needs check if ap supports rx ampdu.
	if((phtpriv->ampdu_enable==_FALSE) &&(pregistrypriv->ampdu_enable==1))
	{
		phtpriv->ampdu_enable = _TRUE;
	}

	
	//check Max Rx A-MPDU Size 
	len = 0;
	p = rtw_get_ie(pie+sizeof (NDIS_802_11_FIXED_IEs), _HT_CAPABILITY_IE_, &len, ie_len-sizeof (NDIS_802_11_FIXED_IEs));
	if(p && len>0)	
	{
		pht_capie = (struct ieee80211_ht_cap *)(p+2);
		max_ampdu_sz = (pht_capie->ampdu_params_info & IEEE80211_HT_CAP_AMPDU_FACTOR);
		max_ampdu_sz = 1 << (max_ampdu_sz+3); // max_ampdu_sz (kbytes);
		
		//printk("rtw_update_ht_cap(): max_ampdu_sz=%d\n", max_ampdu_sz);
		phtpriv->rx_ampdu_maxlen = max_ampdu_sz;
		
	}

	//for A-MPDU Rx reordering buffer control for bmc_sta & sta_info
	//if A-MPDU Rx is enabled, reseting  rx_ordering_ctrl wstart_b(indicate_seq) to default value=0xffff
	//todo: check if AP can send A-MPDU packets
	bmc_sta = rtw_get_bcmc_stainfo(padapter);
	if(bmc_sta)
	{
		for(i=0; i < 16 ; i++)
		{
			//preorder_ctrl = &precvpriv->recvreorder_ctrl[i];
			preorder_ctrl = &bmc_sta->recvreorder_ctrl[i];
			preorder_ctrl->enable = _FALSE;
			preorder_ctrl->indicate_seq = 0xffff;
			preorder_ctrl->wend_b= 0xffff;
			preorder_ctrl->wsize_b = 64;//max_ampdu_sz;//ex. 32(kbytes) -> wsize_b=32
		}
	}

	psta = rtw_get_stainfo(&padapter->stapriv, pcur_network->network.MacAddress);
	if(psta)
	{
		for(i=0; i < 16 ; i++)
		{
			//preorder_ctrl = &precvpriv->recvreorder_ctrl[i];
			preorder_ctrl = &psta->recvreorder_ctrl[i];
			preorder_ctrl->enable = _FALSE;
			preorder_ctrl->indicate_seq = 0xffff;
			preorder_ctrl->wend_b= 0xffff;
			preorder_ctrl->wsize_b = 64;//max_ampdu_sz;//ex. 32(kbytes) -> wsize_b=32
		}
	}

	len=0;
	p = rtw_get_ie(pie+sizeof (NDIS_802_11_FIXED_IEs), _HT_ADD_INFO_IE_, &len, ie_len-sizeof (NDIS_802_11_FIXED_IEs));
	if(p && len>0)	
	{
		pht_addtinfo = (struct ieee80211_ht_addt_info *)(p+2);		
	}

}

void rtw_issue_addbareq_cmd(_adapter *padapter, struct xmit_frame *pxmitframe)
{
	u8 issued;
	int priority;
	struct sta_info *psta=NULL;
	struct ht_priv	*phtpriv;
	struct pkt_attrib *pattrib =&pxmitframe->attrib;
 	struct sta_priv *pstapriv = &padapter->stapriv;	
	s32 bmcst = IS_MCAST(pattrib->ra);

	if(bmcst)
		return;
	
	priority = pattrib->priority;

	if (pattrib->psta)
		psta = pattrib->psta;
	else
		psta = rtw_get_stainfo(&padapter->stapriv, pattrib->ra);
	
	if(psta==NULL)
		return;
	
	phtpriv = &psta->htpriv;

	if((phtpriv->ht_option==1) && (phtpriv->ampdu_enable==_TRUE)) 
	{
		issued = (phtpriv->agg_enable_bitmap>>priority)&0x1;
		issued |= (phtpriv->candidate_tid_bitmap>>priority)&0x1;

		if(0==issued)
		{
			DBG_871X("issue_addbareq_cmd, p=%d\n", priority);
			psta->htpriv.candidate_tid_bitmap |= BIT((u8)priority);
			rtw_addbareq_cmd(padapter,(u8)priority, pattrib->ra);
		}	
	}
	
}

#endif

