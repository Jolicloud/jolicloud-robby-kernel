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
#define _RTL8192CU_RECV_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <mlme_osdep.h>
#include <ip.h>
#include <if_ether.h>
#include <ethernet.h>

#ifdef CONFIG_USB_HCI
#include <usb_ops.h>
#endif

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#include <wifi.h>
#include <circ_buf.h>


int	rtw_init_recv_priv(struct recv_priv *precvpriv, _adapter *padapter)
{
	int i;
	struct recv_buf *precvbuf;
	//struct recv_reorder_ctrl *preorder_ctrl;
	int	res=_SUCCESS;


	_rtw_init_sema(&precvpriv->recv_sema, 0);//will be removed
	_rtw_init_sema(&precvpriv->terminate_recvthread_sema, 0);//will be removed

	//init recv_buf
	_rtw_init_queue(&precvpriv->free_recv_buf_queue);


	precvpriv->pallocated_recv_buf = _rtw_malloc(NR_RECVBUFF *sizeof(struct recv_buf) + 4);
	if(precvpriv->pallocated_recv_buf==NULL){
		res= _FAIL;
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("alloc recv_buf fail!\n"));
		goto exit;
	}
	_rtw_memset(precvpriv->pallocated_recv_buf, 0, NR_RECVBUFF *sizeof(struct recv_buf) + 4);

	precvpriv->precv_buf = precvpriv->pallocated_recv_buf + 4 -
							((uint) (precvpriv->pallocated_recv_buf) &(4-1));


	precvbuf = (struct recv_buf*)precvpriv->precv_buf;

	for(i=0; i < NR_RECVBUFF ; i++)
	{
		_rtw_init_listhead(&precvbuf->list);

		_rtw_spinlock_init(&precvbuf->recvbuf_lock);

		res = rtw_os_recvbuf_resource_alloc(padapter, precvbuf);
		if(res==_FAIL)
			break;

		precvbuf->ref_cnt = 0;
		precvbuf->adapter =padapter;


		rtw_list_insert_tail(&precvbuf->list, &(precvpriv->free_recv_buf_queue.queue));

		precvbuf++;

	}
#ifdef CONFIG_SDIO_HCI

	precvpriv->recvbuf_drop= (struct recv_buf*)_rtw_malloc(sizeof(struct recv_buf));
#ifdef PLATFORM_LINUX
	((struct recv_buf *)precvpriv->recvbuf_drop)->pallocated_buf = _rtw_malloc(MAX_RECVBUF_SZ+4);
	if(((struct recv_buf *)precvpriv->recvbuf_drop)->pallocated_buf == NULL){
		res = _FAIL;
	}

	((struct recv_buf *)precvpriv->recvbuf_drop)->pbuf=((struct recv_buf *)precvpriv->recvbuf_drop)->pallocated_buf + 4 -  ((uint) (((struct recv_buf *)precvpriv->recvbuf_drop)->pallocated_buf) &(4-1));


	((struct recv_buf *)precvpriv->recvbuf_drop)->pdata = ((struct recv_buf *)precvpriv->recvbuf_drop)->phead = ((struct recv_buf *)precvpriv->recvbuf_drop)->ptail =((struct recv_buf *)precvpriv->recvbuf_drop)->pbuf;

	((struct recv_buf *)precvpriv->recvbuf_drop)->pend = ((struct recv_buf *)precvpriv->recvbuf_drop)->pdata + MAX_RECVBUF_SZ;



	((struct recv_buf *)precvpriv->recvbuf_drop)->len = 0;

#else
	rtw_os_recvbuf_resource_alloc(padapter, precvpriv->recvbuf_drop);
#endif
#endif

	precvpriv->free_recv_buf_queue_cnt = NR_RECVBUFF;


#ifdef PLATFORM_LINUX

	tasklet_init(&precvpriv->recv_tasklet,
	     (void(*)(unsigned long))rtl8192cu_recv_tasklet,
	     (unsigned long)padapter);


	skb_queue_head_init(&precvpriv->rx_skb_queue);

#ifdef CONFIG_PREALLOC_RECV_SKB
	{
		int i;
		u32 tmpaddr=0;
		int alignment=0;
		struct sk_buff *pskb=NULL;

		skb_queue_head_init(&precvpriv->free_recv_skb_queue);

		for(i=0; i<NR_PREALLOC_RECV_SKB; i++)
		{

	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)) // http://www.mail-archive.com/netdev@vger.kernel.org/msg17214.html
			pskb = dev_alloc_skb(MAX_RECVBUF_SZ + RECVBUFF_ALIGN_SZ);
	#else
			pskb = netdev_alloc_skb(padapter->pnetdev, MAX_RECVBUF_SZ + RECVBUFF_ALIGN_SZ);
	#endif

			if(pskb)
			{
				pskb->dev = padapter->pnetdev;

				tmpaddr = (u32)pskb->data;
				alignment = tmpaddr & (RECVBUFF_ALIGN_SZ-1);
				skb_reserve(pskb, (RECVBUFF_ALIGN_SZ - alignment));

				skb_queue_tail(&precvpriv->free_recv_skb_queue, pskb);
			}

			pskb=NULL;

		}

	}
#endif

#endif

exit:

	return res;

}

void rtw_free_recv_priv (struct recv_priv *precvpriv)
{
	int i;
	struct recv_buf *precvbuf;
	_adapter *padapter = precvpriv->adapter;

	precvbuf = (struct recv_buf *)precvpriv->precv_buf;

	for(i=0; i < NR_RECVBUFF ; i++)
	{
		rtw_os_recvbuf_resource_free(padapter, precvbuf);
		precvbuf++;
	}

	if(precvpriv->pallocated_recv_buf)
		_rtw_mfree(precvpriv->pallocated_recv_buf, NR_RECVBUFF *sizeof(struct recv_buf) + 4);


#ifdef PLATFORM_LINUX

	if (skb_queue_len(&precvpriv->rx_skb_queue)) {
		printk(KERN_WARNING "rx_skb_queue not empty\n");
	}

	skb_queue_purge(&precvpriv->rx_skb_queue);

#ifdef CONFIG_PREALLOC_RECV_SKB

	if (skb_queue_len(&precvpriv->free_recv_skb_queue)) {
		printk(KERN_WARNING "free_recv_skb_queue not empty, %d\n", skb_queue_len(&precvpriv->free_recv_skb_queue));
	}

	skb_queue_purge(&precvpriv->free_recv_skb_queue);

#endif

#endif

}

int rtw_init_recvbuf(_adapter *padapter, struct recv_buf *precvbuf)
{
	int res=_SUCCESS;
#ifdef CONFIG_USB_HCI
	precvbuf->transfer_len = 0;

	precvbuf->len = 0;

	precvbuf->ref_cnt = 0;

#endif //#ifdef CONFIG_USB_HCI
	if(precvbuf->pbuf)
	{
		precvbuf->pdata = precvbuf->phead = precvbuf->ptail = precvbuf->pbuf;
		precvbuf->pend = precvbuf->pdata + MAX_RECVBUF_SZ;
	}

	return res;

}

void rtl8192cu_update_recvframe_attrib_from_recvstat(union recv_frame *precvframe, struct recv_stat *prxstat)
{
	u8 physt, qos, shift, icverr, htc,crcerr;
	u32 *pphy_info;
	u16 drvinfo_sz=0;
	struct rx_pkt_attrib *pattrib = &precvframe->u.hdr.attrib;		
	_adapter *padapter = precvframe->u.hdr.adapter;

	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	
	u8 bPacketMatchBSSID=_FALSE;
	u8 bPacketToSelf = _FALSE;
	u8 bPacketBeacon = _FALSE;
	//Offset 0
	drvinfo_sz = (le32_to_cpu(prxstat->rxdw0)&0x000f0000)>>16;
	drvinfo_sz = drvinfo_sz<<3;

	pattrib->bdecrypted = ((le32_to_cpu(prxstat->rxdw0) & BIT(27)) >> 27)? 0:1;

	physt = ((le32_to_cpu(prxstat->rxdw0) & BIT(26)) >> 26)? 1:0;
	pattrib->physt = physt;

	shift = (le32_to_cpu(prxstat->rxdw0)&0x03000000)>>24;

	qos = ((le32_to_cpu(prxstat->rxdw0) & BIT(23)) >> 23)? 1:0;

	icverr = ((le32_to_cpu(prxstat->rxdw0) & BIT(15)) >> 15)? 1:0;

	crcerr =  ((le32_to_cpu(prxstat->rxdw0) & BIT(14)) >> 14 )?1:0;


	//Offset 4

	//Offset 8

	//Offset 12
#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_RX
	if ( le32_to_cpu(prxstat->rxdw3) & BIT(13)) {
		pattrib->tcpchk_valid = 1; // valid
		if ( le32_to_cpu(prxstat->rxdw3) & BIT(11) ) {
			pattrib->tcp_chkrpt = 1; // correct
			//printk("tcp csum ok\n");
		} else
			pattrib->tcp_chkrpt = 0; // incorrect

		if ( le32_to_cpu(prxstat->rxdw3) & BIT(12) )
			pattrib->ip_chkrpt = 1; // correct
		else
			pattrib->ip_chkrpt = 0; // incorrect

	} else {
		pattrib->tcpchk_valid = 0; // invalid
	}

#endif

	pattrib->mcs_rate=(u8)((le32_to_cpu(prxstat->rxdw3))&0x3f);
	pattrib->rxht=(u8)((le32_to_cpu(prxstat->rxdw3) >>6)&0x1);

	htc = (u8)((le32_to_cpu(prxstat->rxdw3) >>10)&0x1);

	//Offset 16
	//Offset 20


#if 0 //dump rxdesc for debug
	if(pHalData->bDumpRxPkt){
		printk("### rxdw0=0x%08x #### \n", le32_to_cpu(prxstat->rxdw0));	
		printk("pkt_len=0x%04x\n",(le32_to_cpu(prxstat->rxdw0)&0x3FFF));
		printk("drvinfo_sz=%d\n", drvinfo_sz);
		printk("physt=%d\n", physt);
		printk("shift=%d\n", shift);
		printk("qos=%d\n", qos);
		printk("icverr=%d\n", icverr);
		printk("htc=%d\n", htc);
		printk("bdecrypted=%d\n", pattrib->bdecrypted);
		printk("mcs_rate=%d\n", pattrib->mcs_rate);
		printk("rxht=%d\n", pattrib->rxht);
	}
#endif
	
	//phy_info
	if(drvinfo_sz && physt)
	{
		bPacketMatchBSSID = ((!IsFrameTypeCtrl(precvframe->u.hdr.rx_data)) && 
							!icverr && !crcerr && _rtw_memcmp(get_hdr_bssid(precvframe->u.hdr.rx_data), 
							get_my_bssid(&padapter->mlmeextpriv.mlmext_info.network), ETH_ALEN));

			

		bPacketToSelf = bPacketMatchBSSID &&  (_rtw_memcmp(get_da(precvframe->u.hdr.rx_data), myid(&padapter->eeprompriv), ETH_ALEN));

		bPacketBeacon =bPacketMatchBSSID && (GetFrameSubType(precvframe->u.hdr.rx_data) ==  WIFI_BEACON);
				
		pphy_info=(u32 *)prxstat+1;

		//printk("pphy_info, of0=0x%08x\n", *pphy_info);
		//printk("pphy_info, of1=0x%08x\n", *(pphy_info+1));
		//printk("pphy_info, of2=0x%08x\n", *(pphy_info+2));
		//printk("pphy_info, of3=0x%08x\n", *(pphy_info+3));
		//printk("pphy_info, of4=0x%08x\n", *(pphy_info+4));
		//printk("pphy_info, of5=0x%08x\n", *(pphy_info+5));
		//printk("pphy_info, of6=0x%08x\n", *(pphy_info+6));
		//printk("pphy_info, of7=0x%08x\n", *(pphy_info+7));
		
		
		rtl8192c_query_rx_phy_status(precvframe, prxstat);

#ifdef CONFIG_ANTENNA_DIVERSITY
		// If we switch to the antenna for testing, the signal strength 
		// of the packets in this time shall not be counted into total receiving power. 
		// This prevents error counting Rx signal strength and affecting other dynamic mechanism.

		// Select the packets to do RSSI checking for antenna switching.
		if(bPacketToSelf || bPacketBeacon)
		{	
			SwAntDivRSSICheck(padapter, precvframe->u.hdr.attrib.RxPWDBAll);	
		}
		
#endif
		if(bPacketToSelf || bPacketBeacon)	
			rtl8192c_process_phy_info(padapter,precvframe);		

#if 0 //dump phy_status for debug

		printk("signal_qual=%d\n", pattrib->signal_qual);
		printk("signal_strength=%d\n", pattrib->signal_strength);
#endif

	}


}

