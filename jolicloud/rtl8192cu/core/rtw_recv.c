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
#define _RTL871X_RECV_C_
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


void _rtw_init_sta_recv_priv(struct sta_recv_priv *psta_recvpriv)
{


_func_enter_;

	_rtw_memset((u8 *)psta_recvpriv, 0, sizeof (struct sta_recv_priv));

	_rtw_spinlock_init(&psta_recvpriv->lock);

	//for(i=0; i<MAX_RX_NUMBLKS; i++)
	//	_rtw_init_queue(&psta_recvpriv->blk_strms[i]);

	_rtw_init_queue(&psta_recvpriv->defrag_q);

_func_exit_;

}

sint _rtw_init_recv_priv(struct recv_priv *precvpriv, _adapter *padapter)
{
	sint i;

	union recv_frame *precvframe;

	sint	res=_SUCCESS;

_func_enter_;

	 _rtw_memset((unsigned char *)precvpriv, 0, sizeof (struct  recv_priv));

	_rtw_spinlock_init(&precvpriv->lock);

	_rtw_init_queue(&precvpriv->free_recv_queue);
	_rtw_init_queue(&precvpriv->recv_pending_queue);

	precvpriv->adapter = padapter;

	precvpriv->free_recvframe_cnt = NR_RECVFRAME;

	rtw_os_recv_resource_init(precvpriv, padapter);

	precvpriv->pallocated_frame_buf = _rtw_malloc(NR_RECVFRAME * sizeof(union recv_frame) + RXFRAME_ALIGN_SZ);
	if(precvpriv->pallocated_frame_buf==NULL){
		res= _FAIL;
		goto exit;
	}
	_rtw_memset(precvpriv->pallocated_frame_buf, 0, NR_RECVFRAME * sizeof(union recv_frame) + RXFRAME_ALIGN_SZ);

	precvpriv->precv_frame_buf = precvpriv->pallocated_frame_buf + RXFRAME_ALIGN_SZ -
							((uint) (precvpriv->pallocated_frame_buf) &(RXFRAME_ALIGN_SZ-1));

	precvframe = (union recv_frame*) precvpriv->precv_frame_buf;


	for(i=0; i < NR_RECVFRAME ; i++)
	{
		_rtw_init_listhead(&(precvframe->u.list));

		rtw_list_insert_tail(&(precvframe->u.list), &(precvpriv->free_recv_queue.queue));

		res = rtw_os_recv_resource_alloc(padapter, precvframe);

		precvframe->u.hdr.adapter =padapter;
		precvframe++;

	}

#ifdef CONFIG_USB_HCI

	precvpriv->rx_pending_cnt=1;

	_rtw_init_sema(&precvpriv->allrxreturnevt, 0);

#endif


	res = rtw_init_recv_priv(precvpriv, padapter);


exit:

_func_exit_;

	return res;

}

static void mfree_recv_priv_lock(struct recv_priv *precvpriv)
{
	_rtw_spinlock_free(&precvpriv->lock);
	_rtw_free_sema(&precvpriv->recv_sema);
	_rtw_free_sema(&precvpriv->terminate_recvthread_sema);

	_rtw_spinlock_free(&precvpriv->free_recv_queue.lock);
	_rtw_spinlock_free(&precvpriv->recv_pending_queue.lock);

	_rtw_spinlock_free(&precvpriv->free_recv_buf_queue.lock);

}

void _rtw_free_recv_priv (struct recv_priv *precvpriv)
{
_func_enter_;

	mfree_recv_priv_lock(precvpriv);

	rtw_os_recv_resource_free(precvpriv);

	if(precvpriv->pallocated_frame_buf)
		_rtw_mfree(precvpriv->pallocated_frame_buf, NR_RECVFRAME * sizeof(union recv_frame) + RXFRAME_ALIGN_SZ);

	rtw_free_recv_priv(precvpriv);

_func_exit_;

}

union recv_frame *rtw_alloc_recvframe (_queue *pfree_recv_queue)
{
	//_irqL irqL;
	union recv_frame  *precvframe;
	_list	*plist, *phead;
	_adapter *padapter;
	struct recv_priv *precvpriv;
_func_enter_;

	//_enter_critical(&pfree_recv_queue->lock, &irqL);

	if(_rtw_queue_empty(pfree_recv_queue) == _TRUE)
	{
		precvframe = NULL;
	}
	else
	{
		phead = get_list_head(pfree_recv_queue);

		plist = get_next(phead);

		precvframe = LIST_CONTAINOR(plist, union recv_frame, u);

		list_delete(&precvframe->u.hdr.list);
		padapter=precvframe->u.hdr.adapter;
		if(padapter !=NULL){
			precvpriv=&padapter->recvpriv;
			if(pfree_recv_queue == &precvpriv->free_recv_queue)
				precvpriv->free_recvframe_cnt--;
		}
	}

	//_exit_critical(&pfree_recv_queue->lock, &irqL);

_func_exit_;

	return precvframe;

}


void rtw_init_recvframe(union recv_frame *precvframe, struct recv_priv *precvpriv)
{
	struct recv_buf *precvbuf = precvframe->u.hdr.precvbuf;

	/* Perry: This can be removed */
	_rtw_init_listhead(&precvframe->u.hdr.list);

	precvframe->u.hdr.len=0;


}


int rtw_free_recvframe(union recv_frame *precvframe, _queue *pfree_recv_queue)
{
	//_irqL irqL;
	_adapter *padapter=precvframe->u.hdr.adapter;
	struct recv_priv *precvpriv = &padapter->recvpriv;

_func_enter_;


#ifdef PLATFORM_WINDOWS
	rtw_os_read_port(padapter, precvframe->u.hdr.precvbuf);
#endif

#ifdef PLATFORM_LINUX

	if(precvframe->u.hdr.pkt)
	{
		dev_kfree_skb_any(precvframe->u.hdr.pkt);//free skb by driver
		precvframe->u.hdr.pkt = NULL;
	}

#ifdef CONFIG_SDIO_HCI
{
	_irqL irql;
	struct recv_buf *precvbuf=precvframe->u.hdr.precvbuf;
	if(precvbuf !=NULL){
		_enter_critical_ex(&precvbuf->recvbuf_lock, &irql);

		precvbuf->ref_cnt--;
		if(precvbuf->ref_cnt == 0 ){
			_enter_critical(&precvpriv->free_recv_buf_queue.lock, &irqL);
			list_delete(&(precvbuf->list));
			rtw_list_insert_tail(&(precvbuf->list), get_list_head(&precvpriv->free_recv_buf_queue));
			precvpriv->free_recv_buf_queue_cnt++;
			_exit_critical(&precvpriv->free_recv_buf_queue.lock, &irqL);
			RT_TRACE(_module_rtl871x_recv_c_,_drv_notice_,("rtw_os_read_port: precvbuf=0x%p enqueue:precvpriv->free_recv_buf_queue_cnt=%d\n",precvbuf,precvpriv->free_recv_buf_queue_cnt));
		}
		RT_TRACE(_module_rtl871x_recv_c_,_drv_notice_,("rtw_os_read_port: precvbuf=0x%p enqueue:precvpriv->free_recv_buf_queue_cnt=%d\n",precvbuf,precvpriv->free_recv_buf_queue_cnt));
		_exit_critical_ex(&precvbuf->recvbuf_lock, &irql);
	}
}
#endif
#endif

	//_enter_critical(&pfree_recv_queue->lock, &irqL);

	list_delete(&(precvframe->u.hdr.list));

	rtw_list_insert_tail(&(precvframe->u.hdr.list), get_list_head(pfree_recv_queue));

	if(padapter !=NULL){
		if(pfree_recv_queue == &precvpriv->free_recv_queue)
				precvpriv->free_recvframe_cnt++;
	}

      //_exit_critical(&pfree_recv_queue->lock, &irqL);

_func_exit_;

	return _SUCCESS;

}


static union recv_frame *dequeue_recvframe (_queue *queue)
{
	return rtw_alloc_recvframe(queue);
}


sint rtw_enqueue_recvframe(union recv_frame *precvframe, _queue *queue)
{
	_irqL irqL;
	_adapter *padapter=precvframe->u.hdr.adapter;
	struct recv_priv *precvpriv = &padapter->recvpriv;

_func_enter_;


	//_rtw_spinlock(&pfree_recv_queue->lock);
	_enter_critical(&queue->lock, &irqL);

	//_rtw_init_listhead(&(precvframe->u.hdr.list));
	list_delete(&(precvframe->u.hdr.list));


	rtw_list_insert_tail(&(precvframe->u.hdr.list), get_list_head(queue));

	if (padapter != NULL) {
		if (queue == &precvpriv->free_recv_queue)
			precvpriv->free_recvframe_cnt++;
	}

	//_rtw_spinunlock(&pfree_recv_queue->lock);
	_exit_critical(&queue->lock, &irqL);


_func_exit_;

	return _SUCCESS;
}

/*
sint	rtw_enqueue_recvframe(union recv_frame *precvframe, _queue *queue)
{
	return rtw_free_recvframe(precvframe, queue);
}
*/




/*
caller : defrag ; recvframe_chk_defrag in recv_thread  (passive)
pframequeue: defrag_queue : will be accessed in recv_thread  (passive)

using spinlock to protect

*/

void rtw_free_recvframe_queue(_queue *pframequeue,  _queue *pfree_recv_queue)
{
	union	recv_frame 	*precvframe;
	_list	*plist, *phead;

_func_enter_;
	_rtw_spinlock(&pframequeue->lock);

	phead = get_list_head(pframequeue);
	plist = get_next(phead);

	while(rtw_end_of_queue_search(phead, plist) == _FALSE)
	{
		precvframe = LIST_CONTAINOR(plist, union recv_frame, u);

		plist = get_next(plist);

		//list_delete(&precvframe->u.hdr.list); // will do this in rtw_free_recvframe()

		rtw_free_recvframe(precvframe, pfree_recv_queue);
	}

	_rtw_spinunlock(&pframequeue->lock);

_func_exit_;

}


static sint recvframe_chkmic(_adapter *adapter,  union recv_frame *precvframe){

	sint	i,res=_SUCCESS;
	u32	datalen;
	u8	miccode[8];
	u8	bmic_err=_FALSE;
	u8	*pframe, *payload,*pframemic;
	u8	*mickey;
	struct	sta_info		*stainfo;
	struct	rx_pkt_attrib	*prxattrib=&precvframe->u.hdr.attrib;
	struct 	security_priv	*psecuritypriv=&adapter->securitypriv;


_func_enter_;

	stainfo=rtw_get_stainfo(&adapter->stapriv ,&prxattrib->ta[0]);

	if(prxattrib->encrypt ==_TKIP_)
	{
		RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("\n recvframe_chkmic:prxattrib->encrypt ==_TKIP_\n"));
		RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("\n recvframe_chkmic:da=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
			prxattrib->ra[0],prxattrib->ra[1],prxattrib->ra[2],prxattrib->ra[3],prxattrib->ra[4],prxattrib->ra[5]));

		//calculate mic code
		if(stainfo!= NULL)
		{
			if(IS_MCAST(prxattrib->ra))
			{
				mickey=&psecuritypriv->dot118021XGrprxmickey.skey[0];
				RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("\n recvframe_chkmic: bcmc key \n"));
				if(psecuritypriv->binstallGrpkey==_FALSE)
				{
					res=_FAIL;
					RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("\n recvframe_chkmic:didn't install group key!!!!!!!!!!\n"));
					goto exit;
				}
			}
			else{
				mickey=&stainfo->dot11tkiprxmickey.skey[0];
				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("\n recvframe_chkmic: unicast key \n"));
			}

			datalen=precvframe->u.hdr.len-prxattrib->hdrlen-prxattrib->iv_len-prxattrib->icv_len-8;//icv_len included the mic code
			pframe=precvframe->u.hdr.rx_data;
			payload=pframe+prxattrib->hdrlen+prxattrib->iv_len;

			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("\n prxattrib->iv_len=%d prxattrib->icv_len=%d\n",prxattrib->iv_len,prxattrib->icv_len));

			//rtw_seccalctkipmic(&stainfo->dot11tkiprxmickey.skey[0],pframe,payload, datalen ,&miccode[0],(unsigned char)prxattrib->priority); //care the length of the data

			rtw_seccalctkipmic(mickey,pframe,payload, datalen ,&miccode[0],(unsigned char)prxattrib->priority); //care the length of the data

			pframemic=payload+datalen;

			bmic_err=_FALSE;

			for(i=0;i<8;i++){
				if(miccode[i] != *(pframemic+i)){
					RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("recvframe_chkmic:miccode[%d](%02x) != *(pframemic+%d)(%02x) ",i,miccode[i],i,*(pframemic+i)));
					bmic_err=_TRUE;
				}
			}

			if(bmic_err==_TRUE){

				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("\n *(pframemic-8)-*(pframemic-1)=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
					*(pframemic-8),*(pframemic-7),*(pframemic-6),*(pframemic-5),*(pframemic-4),*(pframemic-3),*(pframemic-2),*(pframemic-1)));
				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("\n *(pframemic-16)-*(pframemic-9)=0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
					*(pframemic-16),*(pframemic-15),*(pframemic-14),*(pframemic-13),*(pframemic-12),*(pframemic-11),*(pframemic-10),*(pframemic-9)));

				{
					uint i;
					RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("\n ======demp packet (len=%d)======\n",precvframe->u.hdr.len));
					for(i=0;i<precvframe->u.hdr.len;i=i+8){
						RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x",
							*(precvframe->u.hdr.rx_data+i),*(precvframe->u.hdr.rx_data+i+1),
							*(precvframe->u.hdr.rx_data+i+2),*(precvframe->u.hdr.rx_data+i+3),
							*(precvframe->u.hdr.rx_data+i+4),*(precvframe->u.hdr.rx_data+i+5),
							*(precvframe->u.hdr.rx_data+i+6),*(precvframe->u.hdr.rx_data+i+7)));
					}
					RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("\n ======demp packet end [len=%d]======\n",precvframe->u.hdr.len));
					RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("\n hrdlen=%d, \n",prxattrib->hdrlen));
				}

				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("ra=0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x psecuritypriv->binstallGrpkey=%d ",
					prxattrib->ra[0],prxattrib->ra[1],prxattrib->ra[2],
					prxattrib->ra[3],prxattrib->ra[4],prxattrib->ra[5],psecuritypriv->binstallGrpkey));

				if(prxattrib->bdecrypted ==_TRUE)
				{
					rtw_handle_tkip_mic_err(adapter,(u8)IS_MCAST(prxattrib->ra));
					RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,(" mic error :prxattrib->bdecrypted=%d ",prxattrib->bdecrypted));
				}
				else
				{
					RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,(" mic error :prxattrib->bdecrypted=%d ",prxattrib->bdecrypted));
					RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,(" mic error :prxattrib->bdecrypted=%d ",prxattrib->bdecrypted));
				}

				res=_FAIL;

			}
			else{
				//mic checked ok
				if((psecuritypriv->bcheck_grpkey ==_FALSE)&&(IS_MCAST(prxattrib->ra)==_TRUE)){
					psecuritypriv->bcheck_grpkey =_TRUE;
					RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("psecuritypriv->bcheck_grpkey =_TRUE"));
				}
			}

		}
		else
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("recvframe_chkmic: rtw_get_stainfo==NULL!!!\n"));
		}

		recvframe_pull_tail(precvframe, 8);

	}

exit:

_func_exit_;

	return res;

}

//decrypt and set the ivlen,icvlen of the recv_frame
static union recv_frame * decryptor(_adapter *padapter,union recv_frame *precv_frame)
{

	struct rx_pkt_attrib *prxattrib = &precv_frame->u.hdr.attrib;
	struct security_priv *psecuritypriv=&padapter->securitypriv;
	union recv_frame *return_packet=precv_frame;

_func_enter_;

	RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("prxstat->decrypted=%x prxattrib->encrypt = 0x%03x\n",prxattrib->bdecrypted,prxattrib->encrypt));

	if((prxattrib->encrypt>0) && ((prxattrib->bdecrypted==0) ||(psecuritypriv->sw_decrypt==_TRUE)))
	{
		psecuritypriv->hw_decrypted=_FALSE;

		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("prxstat->decrypted==0 psecuritypriv->hw_decrypted=_FALSE\n"));

		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("perfrom software decryption! \n"));

		//printk("perfrom software decryption!\n");
		RT_TRACE(_module_rtl871x_recv_c_,_drv_notice_,("###  software decryption!\n"));
		switch(prxattrib->encrypt){
		case _WEP40_:
		case _WEP104_:
			rtw_wep_decrypt(padapter, (u8 *)precv_frame);
			break;
		case _TKIP_:
			rtw_tkip_decrypt(padapter, (u8 *)precv_frame);
			break;
		case _AES_:
			rtw_aes_decrypt(padapter, (u8 * )precv_frame);
			break;
		default:
				break;
		}
	}
	else if(prxattrib->bdecrypted==1)
	{
#if 0
		if((prxstat->icv==1)&&(prxattrib->encrypt!=_AES_))
		{
			psecuritypriv->hw_decrypted=_FALSE;

			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("psecuritypriv->hw_decrypted=_FALSE"));

			rtw_free_recvframe(precv_frame, &padapter->recvpriv.free_recv_queue);

			return_packet=NULL;

		}
		else
#endif
		{
			psecuritypriv->hw_decrypted=_TRUE;
			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("### psecuritypriv->hw_decrypted=_TRUE\n"));

		}
	}

	//recvframe_chkmic(adapter, precv_frame);   //move to recvframme_defrag function

_func_exit_;

	return return_packet;

}
//###set the security information in the recv_frame
static union recv_frame * portctrl(_adapter *adapter,union recv_frame * precv_frame)
{
	u8   *psta_addr,*ptr;
	uint  auth_alg;
	struct recv_frame_hdr *pfhdr;
	struct sta_info * psta;
	struct	sta_priv *pstapriv ;
	union recv_frame * prtnframe;
	u16	ether_type=0;
	u16  eapol_type = 0x888e;//for Funia BD's WPA issue  
	struct rx_pkt_attrib *pattrib = & precv_frame->u.hdr.attrib;

_func_enter_;

	pstapriv = &adapter->stapriv;
	ptr=get_recvframe_data(precv_frame);
	pfhdr=&precv_frame->u.hdr;
	psta_addr=pfhdr->attrib.ta;
	psta=rtw_get_stainfo(pstapriv, psta_addr);

	auth_alg=adapter->securitypriv.dot11AuthAlgrthm;

	RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("########portctrl:adapter->securitypriv.dot11AuthAlgrthm= 0x%d\n",adapter->securitypriv.dot11AuthAlgrthm));

	if(auth_alg==2)
	{

	if ((psta!=NULL) && (psta->ieee8021x_blocked))
	{
		//blocked
		//only accept EAPOL frame
		RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("########portctrl:psta->ieee8021x_blocked==1\n"));

		prtnframe=precv_frame;

				//get ether_type
		ptr=ptr+pfhdr->attrib.hdrlen+pfhdr->attrib.iv_len+LLC_HEADER_SIZE;
		_rtw_memcpy(&ether_type,ptr, 2);
		ether_type= ntohs((unsigned short )ether_type);

		if (ether_type == eapol_type) {
			prtnframe=precv_frame;
		} else {
			//free this frame
			rtw_free_recvframe(precv_frame, &adapter->recvpriv.free_recv_queue);
			prtnframe=NULL;
		}
			
	}
	else
	{
		//allowed
		//check decryption status, and decrypt the frame if needed
		RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("########portctrl:psta->ieee8021x_blocked==0\n"));
		RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("portctrl:precv_frame->hdr.attrib.privacy=%x\n",precv_frame->u.hdr.attrib.privacy));

		//prxstat=(struct recv_stat *)(precv_frame->u.hdr.rx_head);
		if(pattrib->bdecrypted==0)
			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("portctrl:prxstat->decrypted=%x\n", pattrib->bdecrypted));

		prtnframe=precv_frame;
		//check is the EAPOL frame or not (Rekey)
		if(ether_type == eapol_type){

			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("########portctrl:ether_type == 0x888e\n"));
			//check Rekey

			prtnframe=precv_frame;
		}
		else{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("########portctrl:ether_type = 0x%.4x\n",ether_type));
		}
		
	}

	}
	else
	{
		prtnframe=precv_frame;
	}

_func_exit_;

		return prtnframe;

}

static sint recv_decache(union recv_frame *precv_frame, u8 bretry, struct stainfo_rxcache *prxcache)
{
	sint tid = precv_frame->u.hdr.attrib.priority;

	u16 seq_ctrl = ( (precv_frame->u.hdr.attrib.seq_num&0xffff) << 4) |
		(precv_frame->u.hdr.attrib.frag_num & 0xf);

_func_enter_;

	if(tid>15)
	{
		RT_TRACE(_module_rtl871x_recv_c_, _drv_notice_, ("recv_decache, (tid>15)! seq_ctrl=0x%x, tid=0x%x\n", seq_ctrl, tid));

		return _FAIL;
	}

	if(1)//if(bretry)
	{
		if(seq_ctrl == prxcache->tid_rxseq[tid])
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_notice_, ("recv_decache, seq_ctrl=0x%x, tid=0x%x, tid_rxseq=0x%x\n", seq_ctrl, tid, prxcache->tid_rxseq[tid]));

			return _FAIL;
		}
	}

	prxcache->tid_rxseq[tid] = seq_ctrl;

_func_exit_;

	return _SUCCESS;

}

static void process_null_data(_adapter *padapter, union recv_frame *precv_frame)
{
#ifdef CONFIG_AP_MODE
	unsigned char pwrbit;
	u8 *ptr = precv_frame->u.hdr.rx_data;
	struct rx_pkt_attrib *pattrib = &precv_frame->u.hdr.attrib;
	struct sta_priv *pstapriv = &padapter->stapriv;
	struct sta_info *psta=NULL;

	psta = rtw_get_stainfo(pstapriv, pattrib->src);

	pwrbit = GetPwrMgt(ptr);

	if(psta)
	{
		if(pwrbit)
		{
			psta->state |= WIFI_SLEEP_STATE;
			pstapriv->sta_dz_bitmap |= BIT(psta->aid-1);
		}
		else
		{
			if(psta->state & WIFI_SLEEP_STATE)
			{
				psta->state ^= WIFI_SLEEP_STATE;

				pstapriv->sta_dz_bitmap &= ~BIT(psta->aid-1);
				
				wakeup_sta_to_xmit(padapter, psta);

			}
		}

	}

#endif
}

static sint sta2sta_data_frame(
	_adapter *adapter,
	union recv_frame *precv_frame,
	struct sta_info**psta
)
{
	u8 *ptr = precv_frame->u.hdr.rx_data;
	sint ret = _SUCCESS;
	struct rx_pkt_attrib *pattrib = & precv_frame->u.hdr.attrib;
	struct	sta_priv 		*pstapriv = &adapter->stapriv;
	struct	security_priv	*psecuritypriv = &adapter->securitypriv;
	struct	mlme_priv	*pmlmepriv = &adapter->mlmepriv;
	u8 *mybssid  = get_bssid(pmlmepriv);
	u8 *myhwaddr = myid(&adapter->eeprompriv);
	u8 * sta_addr = NULL;

	sint bmcast = IS_MCAST(pattrib->dst);

_func_enter_;

	if ((check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE))
	{

		// filter packets that SA is myself or multicast or broadcast
		if (_rtw_memcmp(myhwaddr, pattrib->src, ETH_ALEN)){
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,(" SA==myself \n"));
			ret= _FAIL;
			goto exit;
		}

		if( (!_rtw_memcmp(myhwaddr, pattrib->dst, ETH_ALEN))	&& (!bmcast) ){
			ret= _FAIL;
			goto exit;
		}

		if( _rtw_memcmp(pattrib->bssid, "\x0\x0\x0\x0\x0\x0", ETH_ALEN) ||
		   _rtw_memcmp(mybssid, "\x0\x0\x0\x0\x0\x0", ETH_ALEN) ||
		   (!_rtw_memcmp(pattrib->bssid, mybssid, ETH_ALEN)) ) {
			ret= _FAIL;
			goto exit;
		}

		sta_addr = pattrib->src;

	}
	else if(check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE)
	{
		// For Station mode, sa and bssid should always be BSSID, and DA is my mac-address
		if(!_rtw_memcmp(pattrib->bssid, pattrib->src, ETH_ALEN) )
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("bssid != TA under STATION_MODE; drop pkt\n"));
			ret= _FAIL;
			goto exit;
		}

		sta_addr = pattrib->bssid;

	}
	else if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
	{
		if (bmcast)
		{
			// For AP mode, if DA == MCAST, then BSSID should be also MCAST
			if (!IS_MCAST(pattrib->bssid)){
					ret= _FAIL;
					goto exit;
			}
		}
		else // not mc-frame
		{
			// For AP mode, if DA is non-MCAST, then it must be BSSID, and bssid == BSSID
			if(!_rtw_memcmp(pattrib->bssid, pattrib->dst, ETH_ALEN)) {
				ret= _FAIL;
				goto exit;
			}

			sta_addr = pattrib->src;
		}

	}
	else if(check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE)
	{
		_rtw_memcpy(pattrib->dst, GetAddr1Ptr(ptr), ETH_ALEN);
		_rtw_memcpy(pattrib->src, GetAddr2Ptr(ptr), ETH_ALEN);
		_rtw_memcpy(pattrib->bssid, GetAddr3Ptr(ptr), ETH_ALEN);
		_rtw_memcpy(pattrib->ra, pattrib->dst, ETH_ALEN);
		_rtw_memcpy(pattrib->ta, pattrib->src, ETH_ALEN);

		sta_addr = mybssid;
	}
	else
	{
		ret  = _FAIL;
	}



	if(bmcast)
		*psta = rtw_get_bcmc_stainfo(adapter);
	else
		*psta = rtw_get_stainfo(pstapriv, sta_addr); // get ap_info

	if (*psta == NULL) {
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("can't get psta under sta2sta_data_frame ; drop pkt\n"));
#ifdef CONFIG_MP_INCLUDED
		if(check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE)
		adapter->mppriv.rx_pktloss++;
#endif
		ret= _FAIL;
		goto exit;
	}

exit:
_func_exit_;
	return ret;

}


static sint ap2sta_data_frame(
	_adapter *adapter,
	union recv_frame *precv_frame,
	struct sta_info**psta )
{
	u8 *ptr = precv_frame->u.hdr.rx_data;
	struct rx_pkt_attrib *pattrib = & precv_frame->u.hdr.attrib;
	sint ret = _SUCCESS;
	struct	sta_priv 		*pstapriv = &adapter->stapriv;
	struct	security_priv	*psecuritypriv = &adapter->securitypriv;
	struct	mlme_priv	*pmlmepriv = &adapter->mlmepriv;

	u8 *mybssid  = get_bssid(pmlmepriv);
	u8 *myhwaddr = myid(&adapter->eeprompriv);

	sint bmcast = IS_MCAST(pattrib->dst);

_func_enter_;

	if ((check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE)
#ifndef CONFIG_DRVEXT_MODULE
		&& (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)
#endif
		)
	{

		// if NULL-frame, drop packet
		if ((GetFrameSubType(ptr)) == WIFI_DATA_NULL)
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,(" NULL frame \n"));
			ret= _FAIL;
			goto exit;
		}

		//drop QoS-SubType Data, including QoS NULL, excluding QoS-Data
		if( (GetFrameSubType(ptr) & WIFI_QOS_DATA_TYPE )== WIFI_QOS_DATA_TYPE)
		{
			if(GetFrameSubType(ptr)&(BIT(4)|BIT(5)|BIT(6)))
			{
				ret= _FAIL;
				goto exit;
			}

		}

		// filter packets that SA is myself or multicast or broadcast
		if (_rtw_memcmp(myhwaddr, pattrib->src, ETH_ALEN)){
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,(" SA==myself \n"));
			ret= _FAIL;
			goto exit;
		}

		// da should be for me
		if((!_rtw_memcmp(myhwaddr, pattrib->dst, ETH_ALEN))&& (!bmcast))
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,(" ap2sta_data_frame:  compare DA fail; DA= %x:%x:%x:%x:%x:%x \n",
					pattrib->dst[0],
					pattrib->dst[1],
					pattrib->dst[2],
					pattrib->dst[3],
					pattrib->dst[4],
					pattrib->dst[5]));

			ret= _FAIL;
			goto exit;
		}


		// check BSSID
		if( _rtw_memcmp(pattrib->bssid, "\x0\x0\x0\x0\x0\x0", ETH_ALEN) ||
		     _rtw_memcmp(mybssid, "\x0\x0\x0\x0\x0\x0", ETH_ALEN) ||
		     (!_rtw_memcmp(pattrib->bssid, mybssid, ETH_ALEN)) )
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,(" ap2sta_data_frame:  compare BSSID fail ; BSSID=%x:%x:%x:%x:%x:%x\n",
				pattrib->bssid[0],
				pattrib->bssid[1],
				pattrib->bssid[2],
				pattrib->bssid[3],
				pattrib->bssid[4],
				pattrib->bssid[5]));

			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("mybssid= %x:%x:%x:%x:%x:%x\n",
				mybssid[0],
				mybssid[1],
				mybssid[2],
				mybssid[3],
				mybssid[4],
				mybssid[5]));

			ret= _FAIL;
			goto exit;
		}

		if(bmcast)
			*psta = rtw_get_bcmc_stainfo(adapter);
		else
			*psta = rtw_get_stainfo(pstapriv, pattrib->bssid); // get ap_info

		if (*psta == NULL) {
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("ap2sta: can't get psta under STATION_MODE ; drop pkt\n"));
			ret= _FAIL;
			goto exit;
		}

	}
	else if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE) &&
		     (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) )
	{
		_rtw_memcpy(pattrib->dst, GetAddr1Ptr(ptr), ETH_ALEN);
		_rtw_memcpy(pattrib->src, GetAddr2Ptr(ptr), ETH_ALEN);
		_rtw_memcpy(pattrib->bssid, GetAddr3Ptr(ptr), ETH_ALEN);
		_rtw_memcpy(pattrib->ra, pattrib->dst, ETH_ALEN);
		_rtw_memcpy(pattrib->ta, pattrib->src, ETH_ALEN);

		//
		_rtw_memcpy(pattrib->bssid,  mybssid, ETH_ALEN);


		*psta = rtw_get_stainfo(pstapriv, pattrib->bssid); // get sta_info
		if (*psta == NULL) {
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("can't get psta under MP_MODE ; drop pkt\n"));
			ret= _FAIL;
			goto exit;
		}


	}
	else
	{
		ret = _FAIL;
	}

exit:

_func_exit_;

	return ret;

}

static sint sta2ap_data_frame(
	_adapter *adapter,
	union recv_frame *precv_frame,
	struct sta_info**psta )
{
	u8 *ptr = precv_frame->u.hdr.rx_data;
	struct rx_pkt_attrib *pattrib = & precv_frame->u.hdr.attrib;
	struct	sta_priv 		*pstapriv = &adapter->stapriv;
	struct	security_priv	*psecuritypriv = &adapter->securitypriv;
	struct	mlme_priv	*pmlmepriv = &adapter->mlmepriv;
	unsigned char *mybssid  = get_bssid(pmlmepriv);	
	sint ret=_SUCCESS;

_func_enter_;

	if (check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
	{
			//For AP mode, RA=BSSID, TX=STA(SRC_ADDR), A3=DST_ADDR
			if(!_rtw_memcmp(pattrib->bssid, mybssid, ETH_ALEN))
			{
					ret= _FAIL;
					goto exit;
				}

			*psta = rtw_get_stainfo(pstapriv, pattrib->src);

			if (*psta == NULL)
			{
				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("can't get psta under AP_MODE; drop pkt\n"));
				ret= _FAIL;
				goto exit;
			}

			// if NULL-frame, drop packet
			if ((GetFrameSubType(ptr)) == WIFI_DATA_NULL)
			{
				RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,(" NULL frame \n"));

				process_null_data(adapter, precv_frame);

				ret= _FAIL;
				goto exit;
			}

			//drop QoS-SubType Data, including QoS NULL, excluding QoS-Data
			if( (GetFrameSubType(ptr) & WIFI_QOS_DATA_TYPE )== WIFI_QOS_DATA_TYPE)
			{
				if(GetFrameSubType(ptr)&(BIT(4)|BIT(5)|BIT(6)))
				{
					ret= _FAIL;
					goto exit;
				}
			}

	}

exit:

_func_exit_;

	return ret;

}

static sint validate_recv_ctrl_frame(_adapter *adapter, union recv_frame *precv_frame)
{
	RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("+validate_recv_ctrl_frame\n"));
	//DBG_871X("+validate_recv_ctrl_frame\n");

	return _FAIL;
}

static sint validate_recv_mgnt_frame(_adapter *adapter, union recv_frame *precv_frame)
{
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;

	RT_TRACE(_module_rtl871x_recv_c_, _drv_info_, ("+validate_recv_mgnt_frame\n"));

#if 0
	if(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
	{
#ifdef CONFIG_NATIVEAP_MLME		
	        mgt_dispatcher(adapter, precv_frame);
#else
		hostapd_mlme_rx(adapter, precv_frame);
#endif	
	}
	else
	{
		mgt_dispatcher(adapter, precv_frame);
	}
#endif

	mgt_dispatcher(adapter, precv_frame);

	return _SUCCESS;

}


static sint validate_recv_data_frame(_adapter *adapter, union recv_frame *precv_frame)
{
	int res;
	u8 bretry;
	u8 *psa, *pda, *pbssid;
	struct sta_info *psta = NULL;
	u8 *ptr = precv_frame->u.hdr.rx_data;
	struct rx_pkt_attrib	*pattrib = & precv_frame->u.hdr.attrib;
	struct sta_priv 	*pstapriv = &adapter->stapriv;
	struct security_priv	*psecuritypriv = &adapter->securitypriv;	
	sint ret = _SUCCESS;

_func_enter_;

	bretry = GetRetry(ptr);
	pda = get_da(ptr);
	psa = get_sa(ptr);
	pbssid = get_hdr_bssid(ptr);

	if(pbssid == NULL){
		ret= _FAIL;
		goto exit;
	}

	_rtw_memcpy(pattrib->dst, pda, ETH_ALEN);
	_rtw_memcpy(pattrib->src, psa, ETH_ALEN);

	_rtw_memcpy(pattrib->bssid, pbssid, ETH_ALEN);

	switch(pattrib->to_fr_ds)
	{
		case 0:
			_rtw_memcpy(pattrib->ra, pda, ETH_ALEN);
			_rtw_memcpy(pattrib->ta, psa, ETH_ALEN);
			res= sta2sta_data_frame(adapter, precv_frame, &psta);
			break;

		case 1:
			_rtw_memcpy(pattrib->ra, pda, ETH_ALEN);
			_rtw_memcpy(pattrib->ta, pbssid, ETH_ALEN);
			res= ap2sta_data_frame(adapter, precv_frame, &psta);
			break;

		case 2:
			_rtw_memcpy(pattrib->ra, pbssid, ETH_ALEN);
			_rtw_memcpy(pattrib->ta, psa, ETH_ALEN);
			res= sta2ap_data_frame(adapter, precv_frame, &psta);
			break;

		case 3:
			_rtw_memcpy(pattrib->ra, GetAddr1Ptr(ptr), ETH_ALEN);
			_rtw_memcpy(pattrib->ta, GetAddr2Ptr(ptr), ETH_ALEN);
			res=_FAIL;
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,(" case 3\n"));
			break;

		default:
			res=_FAIL;
			break;

	}

	if( (!MacAddr_isBcst(pattrib->dst)) && (!IS_MCAST(pattrib->dst))){
		adapter->recvpriv.NumRxUnicastOkInPeriod++;
	}

	if(res==_FAIL){
		//RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,(" after to_fr_ds_chk; res = fail \n"));
		ret= res;
		goto exit;
	}


	if(psta==NULL){
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,(" after to_fr_ds_chk; psta==NULL \n"));
		ret= _FAIL;
		goto exit;
	}
	
	//psta->rssi = prxcmd->rssi;
	//psta->signal_quality= prxcmd->sq;
	precv_frame->u.hdr.psta = psta;
		

	pattrib->amsdu=0;
	//parsing QC field
	if(pattrib->qos == 1)
	{
		pattrib->priority = GetPriority((ptr + 24));
		pattrib->ack_policy =GetAckpolicy((ptr + 24));
		pattrib->amsdu = GetAMsdu((ptr + 24));
		pattrib->hdrlen = pattrib->to_fr_ds==3 ? 32 : 26;
	}
	else
	{
		pattrib->priority=0;
		pattrib->hdrlen = pattrib->to_fr_ds==3 ? 30 : 24;
	}


	if(pattrib->order)//HT-CTRL 11n
	{
		pattrib->hdrlen += 4;
	}

	precv_frame->u.hdr.preorder_ctrl = &psta->recvreorder_ctrl[pattrib->priority];

	// decache, drop duplicate recv packets
	if(recv_decache(precv_frame, bretry, &psta->sta_recvpriv.rxcache) == _FAIL)
	{
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("decache : drop pkt\n"));
		ret= _FAIL;
		goto exit;
	}

	if(pattrib->privacy){

		RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("validate_recv_data_frame:pattrib->privacy=%x\n", pattrib->privacy));
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("\n ^^^^^^^^^^^IS_MCAST(pattrib->ra(0x%02x))=%d^^^^^^^^^^^^^^^6\n", pattrib->ra[0],IS_MCAST(pattrib->ra)));

		GET_ENCRY_ALGO(psecuritypriv, psta, pattrib->encrypt, IS_MCAST(pattrib->ra));

		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("\n pattrib->encrypt=%d\n",pattrib->encrypt));

		SET_ICE_IV_LEN(pattrib->iv_len, pattrib->icv_len, pattrib->encrypt);
	}
	else
	{
		pattrib->encrypt = 0;
		pattrib->iv_len = pattrib->icv_len = 0;
	}

exit:

_func_exit_;

	return ret;
}

static sint validate_recv_frame(_adapter *adapter, union recv_frame *precv_frame)
{
	//shall check frame subtype, to / from ds, da, bssid

	//then call check if rx seq/frag. duplicated.

	u8 type;
	u8 subtype;
	sint retval = _SUCCESS;
	
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(adapter);	

	struct rx_pkt_attrib *pattrib = & precv_frame->u.hdr.attrib;

	u8 *ptr = precv_frame->u.hdr.rx_data;
	u8  ver =(unsigned char) (*ptr)&0x3 ;
 
_func_enter_;




	//add version chk
	if(ver!=0){
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("validate_recv_data_frame fail! (ver!=0)\n"));
		retval= _FAIL;
		goto exit;
	}

	type =  GetFrameType(ptr);
	subtype = GetFrameSubType(ptr); //bit(7)~bit(2)

	pattrib->to_fr_ds = get_tofr_ds(ptr);

	pattrib->frag_num = GetFragNum(ptr);
	pattrib->seq_num = GetSequence(ptr);

	pattrib->pw_save = GetPwrMgt(ptr);
	pattrib->mfrag = GetMFrag(ptr);
	pattrib->mdata = GetMData(ptr);
	pattrib->privacy = GetPrivacy(ptr);
	pattrib->order = GetOrder(ptr);
#if 0

if(pHalData->bDumpRxPkt ==1){
	int i;
	DBG_871X("############################# \n");
	
	for(i=0; i<64;i=i+8)
		DBG_871X("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:\n", *(ptr+i),
		*(ptr+i+1), *(ptr+i+2) ,*(ptr+i+3) ,*(ptr+i+4),*(ptr+i+5), *(ptr+i+6), *(ptr+i+7));
	DBG_871X("############################# \n");
}
else if(pHalData->bDumpRxPkt ==2){
	if(type== WIFI_MGT_TYPE){
		int i;
		DBG_871X("############################# \n");
		
		for(i=0; i<64;i=i+8)
			DBG_871X("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:\n", *(ptr+i),
			*(ptr+i+1), *(ptr+i+2) ,*(ptr+i+3) ,*(ptr+i+4),*(ptr+i+5), *(ptr+i+6), *(ptr+i+7));
		DBG_871X("############################# \n");
	}
}
else if(pHalData->bDumpRxPkt ==3){
	if(type== WIFI_DATA_TYPE){
		int i;
		DBG_871X("############################# \n");
		
		for(i=0; i<64;i=i+8)
			DBG_871X("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:\n", *(ptr+i),
			*(ptr+i+1), *(ptr+i+2) ,*(ptr+i+3) ,*(ptr+i+4),*(ptr+i+5), *(ptr+i+6), *(ptr+i+7));
		DBG_871X("############################# \n");
	}
}

#endif
	switch (type)
	{
		case WIFI_MGT_TYPE: //mgnt
			retval = validate_recv_mgnt_frame(adapter, precv_frame);
			if (retval == _FAIL)
			{
				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("validate_recv_mgnt_frame fail\n"));
			}
			retval = _FAIL; // only data frame return _SUCCESS
			break;
		case WIFI_CTRL_TYPE: //ctrl
			retval = validate_recv_ctrl_frame(adapter, precv_frame);
			if (retval == _FAIL)
			{
				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("validate_recv_ctrl_frame fail\n"));
			}
			break;
		case WIFI_DATA_TYPE: //data			
			pattrib->qos = (subtype & BIT(7))? 1:0;
			retval = validate_recv_data_frame(adapter, precv_frame);
			if (retval == _FAIL)
			{
				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("validate_recv_data_frame fail\n"));
			}
			break;
		default:
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("validate_recv_data_frame fail! type=0x%x\n", type));
			retval = _FAIL;
			break;
	}

exit:

_func_exit_;

	return retval;
}


//remove the wlanhdr and add the eth_hdr
#if 1
static sint wlanhdr_to_ethhdr ( union recv_frame *precvframe)
{
	sint	rmv_len;
	u16	eth_type, len;
	u8	bsnaphdr;
	u8	*psnap_type;
	struct ieee80211_snap_hdr	*psnap;
	
	sint ret=_SUCCESS;
	_adapter			*adapter =precvframe->u.hdr.adapter;
	struct mlme_priv	*pmlmepriv = &adapter->mlmepriv;

	u8	*ptr = get_recvframe_data(precvframe) ; // point to frame_ctrl field
	struct rx_pkt_attrib *pattrib = & precvframe->u.hdr.attrib;

_func_enter_;

	if(pattrib->encrypt){
		recvframe_pull_tail(precvframe, pattrib->icv_len);	
	}

	psnap=(struct ieee80211_snap_hdr	*)(ptr+pattrib->hdrlen + pattrib->iv_len);
	psnap_type=ptr+pattrib->hdrlen + pattrib->iv_len+SNAP_SIZE;
	/* convert hdr + possible LLC headers into Ethernet header */
	//eth_type = (psnap_type[0] << 8) | psnap_type[1];
	if((_rtw_memcmp(psnap, rfc1042_header, SNAP_SIZE) &&
		(_rtw_memcmp(psnap_type, SNAP_ETH_TYPE_IPX, 2) == _FALSE) && 
		(_rtw_memcmp(psnap_type, SNAP_ETH_TYPE_APPLETALK_AARP, 2)==_FALSE) )||
		//eth_type != ETH_P_AARP && eth_type != ETH_P_IPX) ||
		 _rtw_memcmp(psnap, bridge_tunnel_header, SNAP_SIZE)){
		/* remove RFC1042 or Bridge-Tunnel encapsulation and replace EtherType */
		bsnaphdr = _TRUE;
	}
	else {
		/* Leave Ethernet header part of hdr and full payload */
		bsnaphdr = _FALSE;
	}

	rmv_len = pattrib->hdrlen + pattrib->iv_len +(bsnaphdr?SNAP_SIZE:0);
	len = precvframe->u.hdr.len - rmv_len;

	RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("\n===pattrib->hdrlen: %x,  pattrib->iv_len:%x ===\n\n", pattrib->hdrlen,  pattrib->iv_len));

	if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE))	   	
	{
		ptr += rmv_len ;	
		*ptr = 0x87;
		*(ptr+1) = 0x12;

		eth_type = 0x8712;
		// append rx status for mp test packets
		ptr = recvframe_pull(precvframe, (rmv_len-sizeof(struct ethhdr)+2)-24);
		_rtw_memcpy(ptr, get_rxmem(precvframe), 24);
		ptr+=24;
	}
	else {
		ptr = recvframe_pull(precvframe, (rmv_len-sizeof(struct ethhdr)+ (bsnaphdr?2:0)));
	}

	_rtw_memcpy(ptr, pattrib->dst, ETH_ALEN);
	_rtw_memcpy(ptr+ETH_ALEN, pattrib->src, ETH_ALEN);

	if(!bsnaphdr) {
		len = htons(len);
		_rtw_memcpy(ptr+12, &len, 2);
	}

_func_exit_;	
	return ret;

}

#else

sint wlanhdr_to_ethhdr ( union recv_frame *precvframe)
{
	sint rmv_len;
	u16 eth_type;
	u8	bsnaphdr;
	u8	*psnap_type;
	struct ieee80211_snap_hdr	*psnap;

	sint ret=_SUCCESS;
	_adapter	*adapter =precvframe->u.hdr.adapter;
	struct	mlme_priv	*pmlmepriv = &adapter->mlmepriv;

	u8* ptr = get_recvframe_data(precvframe) ; // point to frame_ctrl field
	struct rx_pkt_attrib *pattrib = & precvframe->u.hdr.attrib;
	struct _vlan *pvlan = NULL;

_func_enter_;

	psnap=(struct ieee80211_snap_hdr	*)(ptr+pattrib->hdrlen + pattrib->iv_len);
	psnap_type=ptr+pattrib->hdrlen + pattrib->iv_len+SNAP_SIZE;
	if (psnap->dsap==0xaa && psnap->ssap==0xaa && psnap->ctrl==0x03)
	{
		if (_rtw_memcmp(psnap->oui, oui_rfc1042, WLAN_IEEE_OUI_LEN))
			bsnaphdr=_TRUE;//wlan_pkt_format = WLAN_PKT_FORMAT_SNAP_RFC1042;	
		else if (_rtw_memcmp(psnap->oui, SNAP_HDR_APPLETALK_DDP, WLAN_IEEE_OUI_LEN) &&
			_rtw_memcmp(psnap_type, SNAP_ETH_TYPE_APPLETALK_DDP, 2) )
			bsnaphdr=_TRUE;	//wlan_pkt_format = WLAN_PKT_FORMAT_APPLETALK;
		else if (_rtw_memcmp( psnap->oui, oui_8021h, WLAN_IEEE_OUI_LEN))
			bsnaphdr=_TRUE;	//wlan_pkt_format = WLAN_PKT_FORMAT_SNAP_TUNNEL;
		else {
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("drop pkt due to invalid frame format!\n"));
			ret= _FAIL;
			goto exit;
		}

	} else
		bsnaphdr=_FALSE;//wlan_pkt_format = WLAN_PKT_FORMAT_OTHERS;

	rmv_len = pattrib->hdrlen + pattrib->iv_len +(bsnaphdr?SNAP_SIZE:0);
	RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("===pattrib->hdrlen: %x,  pattrib->iv_len:%x ===\n", pattrib->hdrlen,  pattrib->iv_len));

	if (check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE)
	{
		ptr += rmv_len ;
		*ptr = 0x87;
		*(ptr+1) = 0x12;

		//back to original pointer
		ptr -= rmv_len;
	}

	ptr += rmv_len ;

	_rtw_memcpy(&eth_type, ptr, 2);
	eth_type= ntohs((unsigned short )eth_type); //pattrib->ether_type
	ptr +=2;

	if(pattrib->encrypt){
		recvframe_pull_tail(precvframe, pattrib->icv_len);
	}

	if(eth_type == 0x8100) //vlan
	{
		pvlan = (struct _vlan *) ptr;

		//eth_type = get_vlan_encap_proto(pvlan);
		//eth_type = pvlan->h_vlan_encapsulated_proto;//?
		rmv_len += 4;
		ptr+=4;
	}

	if(eth_type==0x0800)//ip
	{
		//struct iphdr*  piphdr = (struct iphdr*) ptr;
		//__u8 tos = (unsigned char)(pattrib->priority & 0xff);

		//piphdr->tos = tos;

		//if (piphdr->protocol == 0x06)
		//{
		//	RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("@@@===recv tcp len:%d @@@===\n", precvframe->u.hdr.len));
		//}
	}
	else if(eth_type==0x8712)// append rx status for mp test packets
	{
		//ptr -= 16;
		//_rtw_memcpy(ptr, get_rxmem(precvframe), 16);
	}
	else
	{
#ifdef PLATFORM_OS_XP
		NDIS_PACKET_8021Q_INFO VlanPriInfo;
		UINT32 UserPriority = precvframe->u.hdr.attrib.priority;
		UINT32 VlanID = (pvlan!=NULL ? get_vlan_id(pvlan) : 0 );

		VlanPriInfo.Value =          // Get current value.
				NDIS_PER_PACKET_INFO_FROM_PACKET(precvframe->u.hdr.pkt, Ieee8021QInfo);

		VlanPriInfo.TagHeader.UserPriority = UserPriority;
		VlanPriInfo.TagHeader.VlanId =  VlanID ;

		VlanPriInfo.TagHeader.CanonicalFormatId = 0; // Should be zero.
		VlanPriInfo.TagHeader.Reserved = 0; // Should be zero.
		NDIS_PER_PACKET_INFO_FROM_PACKET(precvframe->u.hdr.pkt, Ieee8021QInfo) = VlanPriInfo.Value;
#endif
	}

	if(eth_type==0x8712)// append rx status for mp test packets
	{
		ptr = recvframe_pull(precvframe, (rmv_len-sizeof(struct ethhdr)+2)-24);
		_rtw_memcpy(ptr, get_rxmem(precvframe), 24);
		ptr+=24;
	}
	else
		ptr = recvframe_pull(precvframe, (rmv_len-sizeof(struct ethhdr)+2));

	_rtw_memcpy(ptr, pattrib->dst, ETH_ALEN);
	_rtw_memcpy(ptr+ETH_ALEN, pattrib->src, ETH_ALEN);

	eth_type = htons((unsigned short)eth_type) ;
	_rtw_memcpy(ptr+12, &eth_type, 2);

exit:

_func_exit_;

	return ret;
}
#endif

static void count_rx_stats(_adapter *padapter, union recv_frame *prframe)
{
	int sz;
	struct sta_info *psta = NULL;
	struct stainfo_stats *pstats = NULL;
	struct recv_priv *precvpriv = &padapter->recvpriv;

	sz = get_recvframe_len(prframe);
	precvpriv->rx_bytes += sz;

	psta = prframe->u.hdr.psta;

	if(psta)
	{
		pstats = &psta->sta_stats;

		pstats->rx_pkts++;
		pstats->rx_bytes += sz;
	}


}


//perform defrag
static union recv_frame * recvframe_defrag(_adapter *adapter,_queue *defrag_q)
{
	_list	 *plist, *phead;
	u8	*data,wlanhdr_offset;
	u8	curfragnum;
	struct recv_frame_hdr *pfhdr,*pnfhdr;
	union recv_frame* prframe, *pnextrframe;
	_queue	*pfree_recv_queue;

_func_enter_;

	curfragnum=0;
	pfree_recv_queue=&adapter->recvpriv.free_recv_queue;

	phead = get_list_head(defrag_q);
	plist = get_next(phead);
	prframe = LIST_CONTAINOR(plist, union recv_frame, u);
	pfhdr=&prframe->u.hdr;
	list_delete(&(prframe->u.list));

	if(curfragnum!=pfhdr->attrib.frag_num)
	{
		//the first fragment number must be 0
		//free the whole queue
		rtw_free_recvframe(prframe, pfree_recv_queue);
		rtw_free_recvframe_queue(defrag_q, pfree_recv_queue);

		return NULL;
	}

	curfragnum++;

	plist= get_list_head(defrag_q);

	plist = get_next(plist);

	data=get_recvframe_data(prframe);

	while(rtw_end_of_queue_search(phead, plist) == _FALSE)
	{
		pnextrframe = LIST_CONTAINOR(plist, union recv_frame , u);
		pnfhdr=&pnextrframe->u.hdr;


		//check the fragment sequence  (2nd ~n fragment frame)

		if(curfragnum!=pnfhdr->attrib.frag_num)
		{
			//the fragment number must be increasing  (after decache)
			//release the defrag_q & prframe
			rtw_free_recvframe(prframe, pfree_recv_queue);
			rtw_free_recvframe_queue(defrag_q, pfree_recv_queue);
			return NULL;
		}

		curfragnum++;

		//copy the 2nd~n fragment frame's payload to the first fragment
		//get the 2nd~last fragment frame's payload

		wlanhdr_offset = pnfhdr->attrib.hdrlen + pnfhdr->attrib.iv_len;

		recvframe_pull(pnextrframe, wlanhdr_offset);

		//append  to first fragment frame's tail (if privacy frame, pull the ICV)
		recvframe_pull_tail(prframe, pfhdr->attrib.icv_len);

		//memcpy
		_rtw_memcpy(pfhdr->rx_tail, pnfhdr->rx_data, pnfhdr->len);

		recvframe_put(prframe, pnfhdr->len);

		pfhdr->attrib.icv_len=pnfhdr->attrib.icv_len;
		plist = get_next(plist);

	};

	//free the defrag_q queue and return the prframe
	rtw_free_recvframe_queue(defrag_q, pfree_recv_queue);

	RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("Performance defrag!!!!!\n"));

_func_exit_;

	return prframe;
}


//check if need to defrag, if needed queue the frame to defrag_q
static union recv_frame * recvframe_chk_defrag(_adapter *padapter,union recv_frame* precv_frame)
{
	u8	ismfrag;
	u8	fragnum;
	u8	*psta_addr;
	struct recv_frame_hdr *pfhdr;
	struct sta_info * psta;
	struct	sta_priv *pstapriv ;
	_list	 *phead;
	union recv_frame* prtnframe=NULL;
	_queue *pfree_recv_queue, *pdefrag_q;

_func_enter_;

	pstapriv = &padapter->stapriv;

	pfhdr=&precv_frame->u.hdr;

	pfree_recv_queue=&padapter->recvpriv.free_recv_queue;

	//need to define struct of wlan header frame ctrl
	ismfrag= pfhdr->attrib.mfrag;
	fragnum=pfhdr->attrib.frag_num;

	psta_addr=pfhdr->attrib.ta;
	psta=rtw_get_stainfo(pstapriv, psta_addr);
	if (psta==NULL)
		pdefrag_q = NULL;
	else
		pdefrag_q=&psta->sta_recvpriv.defrag_q;

	if ((ismfrag==0) && (fragnum==0))
	{
		prtnframe = precv_frame;//isn't a fragment frame
	}

	if (ismfrag==1)
	{
		//0~(n-1) fragment frame
		//enqueue to defraf_g
		if(pdefrag_q != NULL)
		{
			if(fragnum==0)
			{
				//the first fragment
				if(_rtw_queue_empty(pdefrag_q) == _FALSE)
				{
					//free current defrag_q
					rtw_free_recvframe_queue(pdefrag_q, pfree_recv_queue);
				}
			}


			//Then enqueue the 0~(n-1) fragment into the defrag_q

			//_rtw_spinlock(&pdefrag_q->lock);
			phead = get_list_head(pdefrag_q);
			rtw_list_insert_tail(&pfhdr->list, phead);
			//_rtw_spinunlock(&pdefrag_q->lock);

			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("Enqueuq: ismfrag = %d, fragnum= %d\n", ismfrag,fragnum));

			prtnframe=NULL;

		}
		else
		{
			//can't find this ta's defrag_queue, so free this recv_frame
			rtw_free_recvframe(precv_frame, pfree_recv_queue);
			prtnframe=NULL;
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("Free because pdefrag_q ==NULL: ismfrag = %d, fragnum= %d\n", ismfrag, fragnum));
		}

	}

	if((ismfrag==0)&&(fragnum!=0))
	{
		//the last fragment frame
		//enqueue the last fragment
		if(pdefrag_q != NULL)
		{
			//_rtw_spinlock(&pdefrag_q->lock);
			phead = get_list_head(pdefrag_q);
			rtw_list_insert_tail(&pfhdr->list,phead);
			//_rtw_spinunlock(&pdefrag_q->lock);

			//call recvframe_defrag to defrag
			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("defrag: ismfrag = %d, fragnum= %d\n", ismfrag, fragnum));
			precv_frame = recvframe_defrag(padapter, pdefrag_q);
			prtnframe=precv_frame;

		}
		else
		{
			//can't find this ta's defrag_queue, so free this recv_frame
			rtw_free_recvframe(precv_frame, pfree_recv_queue);
			prtnframe=NULL;
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("Free because pdefrag_q ==NULL: ismfrag = %d, fragnum= %d\n", ismfrag,fragnum));
		}

	}


	if((prtnframe!=NULL)&&(prtnframe->u.hdr.attrib.privacy))
	{
		//after defrag we must check tkip mic code
		if(recvframe_chkmic(padapter,  prtnframe)==_FAIL)
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("recvframe_chkmic(padapter,  prtnframe)==_FAIL\n"));
			rtw_free_recvframe(prtnframe,pfree_recv_queue);
			prtnframe=NULL;
		}
	}

_func_exit_;

	return prtnframe;

}


static int amsdu_to_msdu(_adapter *padapter, union recv_frame *prframe)
{

#ifdef PLATFORM_LINUX	//for amsdu TP improvement,Creator: Thomas 
	int	a_len, padding_len;
	u16	eth_type, nSubframe_Length;	
	u8	nr_subframes, i;
	unsigned char *data_ptr, *pdata;
	struct rx_pkt_attrib *pattrib;
	_pkt *sub_skb,*subframes[MAX_SUBFRAME_COUNT];
	struct recv_priv *precvpriv = &padapter->recvpriv;
	_queue *pfree_recv_queue = &(precvpriv->free_recv_queue);
	int	ret = _SUCCESS;

	nr_subframes = 0;

	pattrib = &prframe->u.hdr.attrib;

	recvframe_pull(prframe, prframe->u.hdr.attrib.hdrlen);
	
	if(prframe->u.hdr.attrib.iv_len >0)
	{
		recvframe_pull(prframe, prframe->u.hdr.attrib.iv_len);
	}

	a_len = prframe->u.hdr.len;

	pdata = prframe->u.hdr.rx_data;

	while(a_len > ETH_HLEN) {
		
		/* Offset 12 denote 2 mac address */
		nSubframe_Length = *((u16*)(pdata + 12));
		//==m==>change the length order
		nSubframe_Length = (nSubframe_Length>>8) + (nSubframe_Length<<8);
		//ntohs(nSubframe_Length);

		if( a_len < (ETHERNET_HEADER_SIZE + nSubframe_Length) ) {
			printk("nRemain_Length is %d and nSubframe_Length is : %d\n",a_len,nSubframe_Length);
			goto exit;
		}

		/* move the data point to data content */
		pdata += ETH_HLEN;
		a_len -= ETH_HLEN;

		/* Allocate new skb for releasing to upper layer */
#ifdef CONFIG_SKB_COPY
		sub_skb = dev_alloc_skb(nSubframe_Length + 12);
		skb_reserve(sub_skb, 12);
		data_ptr = (u8 *)skb_put(sub_skb, nSubframe_Length);
		_rtw_memcpy(data_ptr, pdata, nSubframe_Length);
#else
		sub_skb = skb_clone(prframe->u.hdr.pkt, GFP_ATOMIC);
		sub_skb->data = pdata;
		sub_skb->len = nSubframe_Length;
		sub_skb->tail = sub_skb->data + nSubframe_Length;
#endif

		//sub_skb->dev = padapter->pnetdev;
		subframes[nr_subframes++] = sub_skb;
		if(nr_subframes >= MAX_SUBFRAME_COUNT) {
			printk("ParseSubframe(): Too many Subframes! Packets dropped!\n");
			break;
		}

		pdata += nSubframe_Length;
		a_len -= nSubframe_Length;
		if(a_len != 0) {
			padding_len = 4 - ((nSubframe_Length + ETH_HLEN) & (4-1));
			if(padding_len == 4) {
				padding_len = 0;
			}

			if(a_len < padding_len) {
				goto exit;
			}
			pdata += padding_len;
			a_len -= padding_len;
		}
	}

	for(i=0; i<nr_subframes; i++){
		sub_skb = subframes[i];
		/* convert hdr + possible LLC headers into Ethernet header */
		eth_type = (sub_skb->data[6] << 8) | sub_skb->data[7];
		if (sub_skb->len >= 8 &&
			((_rtw_memcmp(sub_skb->data, rfc1042_header, SNAP_SIZE) &&
			  eth_type != ETH_P_AARP && eth_type != ETH_P_IPX) ||
			 _rtw_memcmp(sub_skb->data, bridge_tunnel_header, SNAP_SIZE) )) {
			/* remove RFC1042 or Bridge-Tunnel encapsulation and replace EtherType */
			skb_pull(sub_skb, SNAP_SIZE);
			_rtw_memcpy(skb_push(sub_skb, ETH_ALEN), pattrib->src, ETH_ALEN);
			_rtw_memcpy(skb_push(sub_skb, ETH_ALEN), pattrib->dst, ETH_ALEN);
		} else {
			u16 len;
			/* Leave Ethernet header part of hdr and full payload */
			len = htons(sub_skb->len);
			_rtw_memcpy(skb_push(sub_skb, 2), &len, 2);
			_rtw_memcpy(skb_push(sub_skb, ETH_ALEN), pattrib->src, ETH_ALEN);
			_rtw_memcpy(skb_push(sub_skb, ETH_ALEN), pattrib->dst, ETH_ALEN);
		}

		/* Indicat the packets to upper layer */
		if (sub_skb) {
			//_rtw_memset(sub_skb->cb, 0, sizeof(sub_skb->cb));

			sub_skb->protocol = eth_type_trans(sub_skb, padapter->pnetdev);
			sub_skb->dev = padapter->pnetdev;

#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_RX
			if ( (pattrib->tcpchk_valid == 1) && (pattrib->tcp_chkrpt == 1) ) {
				sub_skb->ip_summed = CHECKSUM_UNNECESSARY;
			} else {
				sub_skb->ip_summed = CHECKSUM_NONE;
			}
#else /* !CONFIG_RTL8712_TCP_CSUM_OFFLOAD_RX */
			sub_skb->ip_summed = CHECKSUM_NONE;
#endif

			netif_rx(sub_skb);
		}
	}

exit:

	prframe->u.hdr.len=0;
	rtw_free_recvframe(prframe, pfree_recv_queue);//free this recv_frame
	
	return ret;
#else
	_irqL irql;
	unsigned char *ptr, *pdata, *pbuf, *psnap_type;
	union recv_frame *pnrframe, *pnrframe_new;
	int a_len, mv_len, padding_len;
	u16 eth_type, type_len;
	u8 bsnaphdr;
	struct ieee80211_snap_hdr	*psnap;
	struct _vlan *pvlan;
	struct recv_priv *precvpriv = &padapter->recvpriv;
	_queue *pfree_recv_queue = &(precvpriv->free_recv_queue);
	int ret = _SUCCESS;
#ifdef PLATFORM_WINDOWS
	struct recv_buf *precvbuf = prframe->u.hdr.precvbuf;
#endif
	a_len = prframe->u.hdr.len - prframe->u.hdr.attrib.hdrlen;

	recvframe_pull(prframe, prframe->u.hdr.attrib.hdrlen);

	if(prframe->u.hdr.attrib.iv_len >0)
	{
		recvframe_pull(prframe, prframe->u.hdr.attrib.iv_len);
	}

	pdata = prframe->u.hdr.rx_data;

	prframe->u.hdr.len=0;

	pnrframe = prframe;


	do{

		mv_len=0;
		pnrframe->u.hdr.rx_data = pnrframe->u.hdr.rx_tail = pdata;
		ptr = pdata;


		_rtw_memcpy(pnrframe->u.hdr.attrib.dst, ptr, ETH_ALEN);
		ptr+=ETH_ALEN;
		_rtw_memcpy(pnrframe->u.hdr.attrib.src, ptr, ETH_ALEN);
		ptr+=ETH_ALEN;

		_rtw_memcpy(&type_len, ptr, 2);
		type_len= ntohs((unsigned short )type_len);
		ptr +=2;
		mv_len += ETH_HLEN;

		recvframe_put(pnrframe, type_len+ETH_HLEN);//update tail;

		if(pnrframe->u.hdr.rx_data >= pnrframe->u.hdr.rx_tail || type_len<8)
		{
			//panic("pnrframe->u.hdr.rx_data >= pnrframe->u.hdr.rx_tail || type_len<8\n");

			rtw_free_recvframe(pnrframe, pfree_recv_queue);

			goto exit;
		}

		psnap=(struct ieee80211_snap_hdr *)(ptr);
		psnap_type=ptr+SNAP_SIZE;
		if (psnap->dsap==0xaa && psnap->ssap==0xaa && psnap->ctrl==0x03)
		{
			if ( _rtw_memcmp(psnap->oui, oui_rfc1042, WLAN_IEEE_OUI_LEN))
			{
				bsnaphdr=_TRUE;//wlan_pkt_format = WLAN_PKT_FORMAT_SNAP_RFC1042;
			}
			else if (_rtw_memcmp(psnap->oui, SNAP_HDR_APPLETALK_DDP, WLAN_IEEE_OUI_LEN) &&
					_rtw_memcmp(psnap_type, SNAP_ETH_TYPE_APPLETALK_DDP, 2) )
			{
				bsnaphdr=_TRUE;	//wlan_pkt_format = WLAN_PKT_FORMAT_APPLETALK;
			}
			else if (_rtw_memcmp( psnap->oui, oui_8021h, WLAN_IEEE_OUI_LEN))
			{
				bsnaphdr=_TRUE;	//wlan_pkt_format = WLAN_PKT_FORMAT_SNAP_TUNNEL;
			}
			else
			{
				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("drop pkt due to invalid frame format!\n"));

				//KeBugCheckEx(0x87123333, 0xe0, 0x4c, 0x87, 0xdd);

				//panic("0x87123333, 0xe0, 0x4c, 0x87, 0xdd\n");

				rtw_free_recvframe(pnrframe, pfree_recv_queue);

				goto exit;
			}

		}
		else
		{
			bsnaphdr=_FALSE;//wlan_pkt_format = WLAN_PKT_FORMAT_OTHERS;
		}

		ptr += (bsnaphdr?SNAP_SIZE:0);
		_rtw_memcpy(&eth_type, ptr, 2);
		eth_type= ntohs((unsigned short )eth_type); //pattrib->ether_type

		mv_len+= 2+(bsnaphdr?SNAP_SIZE:0);
		ptr += 2;//now move to iphdr;

		pvlan = NULL;
		if(eth_type == 0x8100) //vlan
		{
			pvlan = (struct _vlan *)ptr;
			ptr+=4;
			mv_len+=4;
		}

		if(eth_type==0x0800)//ip
		{
			struct iphdr*  piphdr = (struct iphdr*)ptr;


			if (piphdr->protocol == 0x06)
			{
				RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("@@@===recv tcp len:%d @@@===\n", pnrframe->u.hdr.len));
			}
		}
#ifdef PLATFORM_OS_XP
		else
		{
			NDIS_PACKET_8021Q_INFO VlanPriInfo;
			UINT32 UserPriority = pnrframe->u.hdr.attrib.priority;
			UINT32 VlanID = (pvlan!=NULL ? get_vlan_id(pvlan) : 0 );

			VlanPriInfo.Value =          // Get current value.
					NDIS_PER_PACKET_INFO_FROM_PACKET(pnrframe->u.hdr.pkt, Ieee8021QInfo);

			VlanPriInfo.TagHeader.UserPriority = UserPriority;
			VlanPriInfo.TagHeader.VlanId =  VlanID;

			VlanPriInfo.TagHeader.CanonicalFormatId = 0; // Should be zero.
			VlanPriInfo.TagHeader.Reserved = 0; // Should be zero.
			NDIS_PER_PACKET_INFO_FROM_PACKET(pnrframe->u.hdr.pkt, Ieee8021QInfo) = VlanPriInfo.Value;

		}
#endif

		pbuf = recvframe_pull(pnrframe, (mv_len-sizeof(struct ethhdr)));

		_rtw_memcpy(pbuf, pnrframe->u.hdr.attrib.dst, ETH_ALEN);
		_rtw_memcpy(pbuf+ETH_ALEN, pnrframe->u.hdr.attrib.src, ETH_ALEN);

		eth_type = htons((unsigned short)eth_type) ;
		_rtw_memcpy(pbuf+12, &eth_type, 2);

		padding_len = (4) - ((type_len + ETH_HLEN)&(4-1));

		a_len -= (type_len + ETH_HLEN + padding_len) ;


#if 0

	if(a_len > ETH_HLEN)
	{
		pnrframe_new = rtw_alloc_recvframe(pfree_recv_queue);
		if(pnrframe_new)
		{
			_pkt *pskb_copy;
			unsigned int copy_len  = pnrframe->u.hdr.len;

			_rtw_init_listhead(&pnrframe_new->u.hdr.list);

	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)) // http://www.mail-archive.com/netdev@vger.kernel.org/msg17214.html
			pskb_copy = dev_alloc_skb(copy_len+64);
	#else
			pskb_copy = netdev_alloc_skb(padapter->pnetdev, copy_len + 64);
	#endif
			if(pskb_copy==NULL)
			{
				printk("amsdu_to_msdu:can not all(ocate memory for skb copy\n");
			}

			pnrframe_new->u.hdr.pkt = pskb_copy;

			_rtw_memcpy(pskb_copy->data, pnrframe->u.hdr.rx_data, copy_len);

			pnrframe_new->u.hdr.rx_data = pnrframe->u.hdr.rx_data;
			pnrframe_new->u.hdr.rx_tail = pnrframe->u.hdr.rx_data + copy_len;


			if ((padapter->bDriverStopped ==_FALSE)&&( padapter->bSurpriseRemoved==_FALSE))
			{
				rtw_recv_indicatepkt(padapter, pnrframe_new);//indicate this recv_frame
			}
			else
			{
				rtw_free_recvframe(pnrframe_new, pfree_recv_queue);//free this recv_frame
			}

		}
		else
		{
			printk("amsdu_to_msdu:can not allocate memory for pnrframe_new\n");
		}

	}
	else
	{
		if ((padapter->bDriverStopped ==_FALSE)&&( padapter->bSurpriseRemoved==_FALSE))
		{
			rtw_recv_indicatepkt(padapter, pnrframe);//indicate this recv_frame
		}
		else
		{
			rtw_free_recvframe(pnrframe, pfree_recv_queue);//free this recv_frame
		}

		pnrframe = NULL;

	}

#else

		//padding_len = (4) - ((type_len + ETH_HLEN)&(4-1));

		//a_len -= (type_len + ETH_HLEN + padding_len) ;

		pnrframe_new = NULL;


		if(a_len > ETH_HLEN)
		{
			pnrframe_new = rtw_alloc_recvframe(pfree_recv_queue);

			if(pnrframe_new)
			{


				//pnrframe_new->u.hdr.precvbuf = precvbuf;//precvbuf is assigned before call rtw_init_recvframe()
				//rtw_init_recvframe(pnrframe_new, precvpriv);
				{
						_pkt *pskb = pnrframe->u.hdr.pkt;
						_rtw_init_listhead(&pnrframe_new->u.hdr.list);

						pnrframe_new->u.hdr.len=0;

#ifdef PLATFORM_LINUX
						if(pskb)
						{
							pnrframe_new->u.hdr.pkt = skb_clone(pskb, GFP_ATOMIC);
						}
#endif

				}

				pdata += (type_len + ETH_HLEN + padding_len);
				pnrframe_new->u.hdr.rx_head = pnrframe_new->u.hdr.rx_data = pnrframe_new->u.hdr.rx_tail = pdata;
				pnrframe_new->u.hdr.rx_end = pdata + a_len + padding_len;//

#ifdef PLATFORM_WINDOWS
				pnrframe_new->u.hdr.precvbuf=precvbuf;
				_enter_critical(&precvbuf->recvbuf_lock, &irql);
				precvbuf->ref_cnt++;
				_exit_critical(&precvbuf->recvbuf_lock, &irql);
#endif

			}
			else
			{
				//panic("pnrframe_new=%x\n", pnrframe_new);
			}
		}


		if ((padapter->bDriverStopped ==_FALSE)&&( padapter->bSurpriseRemoved==_FALSE) )
		{
			rtw_recv_indicatepkt(padapter, pnrframe);//indicate this recv_frame
		}
		else
		{
			rtw_free_recvframe(pnrframe, pfree_recv_queue);//free this recv_frame
		}


		pnrframe = NULL;
		if(pnrframe_new)
		{
			pnrframe = pnrframe_new;
		}


#endif

	}while(pnrframe);

exit:

	return ret;
#endif

}


static int check_indicate_seq(struct recv_reorder_ctrl *preorder_ctrl, u16 seq_num)
{
	u8	wsize = preorder_ctrl->wsize_b;
	u16	wend = (preorder_ctrl->indicate_seq + wsize -1) & 0xFFF;//% 4096;

	// Rx Reorder initialize condition.
	if (preorder_ctrl->indicate_seq == 0xFFFF)
	{
		preorder_ctrl->indicate_seq = seq_num;

		//DbgPrint("check_indicate_seq, 1st->indicate_seq=%d\n", precvpriv->indicate_seq);
	}

	//DbgPrint("enter->check_indicate_seq(): IndicateSeq: %d, NewSeq: %d\n", precvpriv->indicate_seq, seq_num);

	// Drop out the packet which SeqNum is smaller than WinStart
	if( SN_LESS(seq_num, preorder_ctrl->indicate_seq) )
	{
		//RT_TRACE(COMP_RX_REORDER, DBG_LOUD, ("CheckRxTsIndicateSeq(): Packet Drop! IndicateSeq: %d, NewSeq: %d\n", pTS->RxIndicateSeq, NewSeqNum));

		//DbgPrint("CheckRxTsIndicateSeq(): Packet Drop! IndicateSeq: %d, NewSeq: %d\n", precvpriv->indicate_seq, seq_num);

		return _FALSE;
	}

	//
	// Sliding window manipulation. Conditions includes:
	// 1. Incoming SeqNum is equal to WinStart =>Window shift 1
	// 2. Incoming SeqNum is larger than the WinEnd => Window shift N
	//
	if( SN_EQUAL(seq_num, preorder_ctrl->indicate_seq) )
	{
		preorder_ctrl->indicate_seq = (preorder_ctrl->indicate_seq + 1) & 0xFFF;
	}
	else if(SN_LESS(wend, seq_num))
	{
		//RT_TRACE(COMP_RX_REORDER, DBG_LOUD, ("CheckRxTsIndicateSeq(): Window Shift! IndicateSeq: %d, NewSeq: %d\n", pTS->RxIndicateSeq, NewSeqNum));
		//DbgPrint("CheckRxTsIndicateSeq(): Window Shift! IndicateSeq: %d, NewSeq: %d\n", precvpriv->indicate_seq, seq_num);

		// boundary situation, when seq_num cross 0xFFF
		if(seq_num >= (wsize - 1))
			preorder_ctrl->indicate_seq = seq_num + 1 -wsize;
		else
			preorder_ctrl->indicate_seq = 0xFFF - (wsize - (seq_num + 1)) + 1;
	}

	//DbgPrint("exit->check_indicate_seq(): IndicateSeq: %d, NewSeq: %d\n", precvpriv->indicate_seq, seq_num);

	return _TRUE;
}


static int enqueue_reorder_recvframe(struct recv_reorder_ctrl *preorder_ctrl, union recv_frame *prframe)
{
	struct rx_pkt_attrib *pattrib = &prframe->u.hdr.attrib;
	_queue *ppending_recvframe_queue = &preorder_ctrl->pending_recvframe_queue;
	_list	*phead, *plist;
	union recv_frame *pnextrframe;
	struct rx_pkt_attrib *pnextattrib;

	//DbgPrint("+enqueue_reorder_recvframe()\n");

	//_enter_critical_ex(&ppending_recvframe_queue->lock, &irql);
	//_rtw_spinlock_ex(&ppending_recvframe_queue->lock);


	phead = get_list_head(ppending_recvframe_queue);
	plist = get_next(phead);

	while(rtw_end_of_queue_search(phead, plist) == _FALSE)
	{
		pnextrframe = LIST_CONTAINOR(plist, union recv_frame, u);
		pnextattrib = &pnextrframe->u.hdr.attrib;

		if(SN_LESS(pnextattrib->seq_num, pattrib->seq_num))
		{
			plist = get_next(plist);
		}
		else if( SN_EQUAL(pnextattrib->seq_num, pattrib->seq_num))
		{
			//Duplicate entry is found!! Do not insert current entry.
			//RT_TRACE(COMP_RX_REORDER, DBG_TRACE, ("InsertRxReorderList(): Duplicate packet is dropped!! IndicateSeq: %d, NewSeq: %d\n", pTS->RxIndicateSeq, SeqNum));

			//_exit_critical_ex(&ppending_recvframe_queue->lock, &irql);

			return _FALSE;
		}
		else
		{
			break;
		}

		//DbgPrint("enqueue_reorder_recvframe():while\n");

	}


	//_enter_critical_ex(&ppending_recvframe_queue->lock, &irql);
	//_rtw_spinlock_ex(&ppending_recvframe_queue->lock);

	list_delete(&(prframe->u.hdr.list));

	rtw_list_insert_tail(&(prframe->u.hdr.list), plist);

	//_rtw_spinunlock_ex(&ppending_recvframe_queue->lock);
	//_exit_critical_ex(&ppending_recvframe_queue->lock, &irql);


	//RT_TRACE(COMP_RX_REORDER, DBG_TRACE, ("InsertRxReorderList(): Pkt insert into buffer!! IndicateSeq: %d, NewSeq: %d\n", pTS->RxIndicateSeq, SeqNum));
	return _TRUE;

}


static int recv_indicatepkts_in_order(_adapter *padapter, struct recv_reorder_ctrl *preorder_ctrl, int bforced)
{
//	_irqL irql;
	//u8 bcancelled;
	_list	*phead, *plist;
	union recv_frame *prframe;
	struct rx_pkt_attrib *pattrib;
	//u8 index = 0;
	int bPktInBuf = _FALSE;
	struct recv_priv *precvpriv = &padapter->recvpriv;
	_queue *ppending_recvframe_queue = &preorder_ctrl->pending_recvframe_queue;

	//DbgPrint("+recv_indicatepkts_in_order\n");

	//_enter_critical_ex(&ppending_recvframe_queue->lock, &irql);
	//_rtw_spinlock_ex(&ppending_recvframe_queue->lock);

	phead = 	get_list_head(ppending_recvframe_queue);
	plist = get_next(phead);

#if 0
	// Check if there is any other indication thread running.
	if(pTS->RxIndicateState == RXTS_INDICATE_PROCESSING)
		return;
#endif

	// Handling some condition for forced indicate case.
	if(bforced==_TRUE)
	{
		if(rtw_is_list_empty(phead))
		{
			// _exit_critical_ex(&ppending_recvframe_queue->lock, &irql);
			//_rtw_spinunlock_ex(&ppending_recvframe_queue->lock);
			return _TRUE;
		}
	
		 prframe = LIST_CONTAINOR(plist, union recv_frame, u);
	        pattrib = &prframe->u.hdr.attrib;	
		preorder_ctrl->indicate_seq = pattrib->seq_num;		
	}

	// Prepare indication list and indication.
	// Check if there is any packet need indicate.
	while(!rtw_is_list_empty(phead))
	{
		prframe = LIST_CONTAINOR(plist, union recv_frame, u);
		pattrib = &prframe->u.hdr.attrib;

		if(!SN_LESS(preorder_ctrl->indicate_seq, pattrib->seq_num))
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_notice_,
				 ("recv_indicatepkts_in_order: indicate=%d seq=%d amsdu=%d\n",
				  preorder_ctrl->indicate_seq, pattrib->seq_num, pattrib->amsdu));

#if 0
			// This protect buffer from overflow.
			if(index >= REORDER_WIN_SIZE)
			{
				RT_ASSERT(FALSE, ("IndicateRxReorderList(): Buffer overflow!! \n"));
				bPktInBuf = TRUE;
				break;
			}
#endif

			plist = get_next(plist);
			list_delete(&(prframe->u.hdr.list));

			if(SN_EQUAL(preorder_ctrl->indicate_seq, pattrib->seq_num))
			{
				preorder_ctrl->indicate_seq = (preorder_ctrl->indicate_seq + 1) & 0xFFF;
			}

#if 0
			index++;
			if(index==1)
			{
				//Cancel previous pending timer.
				//PlatformCancelTimer(Adapter, &pTS->RxPktPendingTimer);
				if(bforced!=_TRUE)
				{
					//printk("_cancel_timer(&preorder_ctrl->reordering_ctrl_timer, &bcancelled);\n");
					_cancel_timer(&preorder_ctrl->reordering_ctrl_timer, &bcancelled);
				}
			}
#endif

			//Set this as a lock to make sure that only one thread is indicating packet.
			//pTS->RxIndicateState = RXTS_INDICATE_PROCESSING;

			// Indicate packets
			//RT_ASSERT((index<=REORDER_WIN_SIZE), ("RxReorderIndicatePacket(): Rx Reorder buffer full!! \n"));


			//indicate this recv_frame
			//DbgPrint("recv_indicatepkts_in_order, indicate_seq=%d, seq_num=%d\n", precvpriv->indicate_seq, pattrib->seq_num);
			if(!pattrib->amsdu)
			{
				//printk("recv_indicatepkts_in_order, amsdu!=1, indicate_seq=%d, seq_num=%d\n", preorder_ctrl->indicate_seq, pattrib->seq_num);

				if ((padapter->bDriverStopped == _FALSE) &&
				    (padapter->bSurpriseRemoved == _FALSE))
				{
					rtw_recv_indicatepkt(padapter, prframe);		//indicate this recv_frame
				}
			}
			else if(pattrib->amsdu==1)
			{
				if(amsdu_to_msdu(padapter, prframe)!=_SUCCESS)
				{
					rtw_free_recvframe(prframe, &precvpriv->free_recv_queue);
				}
			}
			else
			{
				//error condition;
			}


			//Update local variables.
			bPktInBuf = _FALSE;

		}
		else
		{
			bPktInBuf = _TRUE;
			break;
		}

		//DbgPrint("recv_indicatepkts_in_order():while\n");

	}

	//_rtw_spinunlock_ex(&ppending_recvframe_queue->lock);
	//_exit_critical_ex(&ppending_recvframe_queue->lock, &irql);

/*
	//Release the indication lock and set to new indication step.
	if(bPktInBuf)
	{
		// Set new pending timer.
		//pTS->RxIndicateState = RXTS_INDICATE_REORDER;
		//PlatformSetTimer(Adapter, &pTS->RxPktPendingTimer, pHTInfo->RxReorderPendingTime);
		//printk("_set_timer(&preorder_ctrl->reordering_ctrl_timer, REORDER_WAIT_TIME)\n");
		_set_timer(&preorder_ctrl->reordering_ctrl_timer, REORDER_WAIT_TIME);
	}
	else
	{
		//pTS->RxIndicateState = RXTS_INDICATE_IDLE;
	}
*/
	//_exit_critical_ex(&ppending_recvframe_queue->lock, &irql);

	//return _TRUE;
	return bPktInBuf;

}


static int recv_indicatepkt_reorder(_adapter *padapter, union recv_frame *prframe)
{
	_irqL irql;
	int retval = _SUCCESS;
	struct recv_priv *precvpriv = &padapter->recvpriv;
	struct rx_pkt_attrib *pattrib = &prframe->u.hdr.attrib;
	struct recv_reorder_ctrl *preorder_ctrl = prframe->u.hdr.preorder_ctrl;
	_queue *ppending_recvframe_queue = &preorder_ctrl->pending_recvframe_queue;

	if(!pattrib->amsdu)
	{
		//s1.
		wlanhdr_to_ethhdr(prframe);

		if(pattrib->qos !=1 /*|| pattrib->priority!=0 || IS_MCAST(pattrib->ra)*/)
		{
			if ((padapter->bDriverStopped == _FALSE) &&
			    (padapter->bSurpriseRemoved == _FALSE))
			{
				RT_TRACE(_module_rtl871x_recv_c_, _drv_alert_, ("@@@@  recv_indicatepkt_reorder -recv_func rtw_recv_indicatepkt\n" ));

				rtw_recv_indicatepkt(padapter, prframe);
				return _SUCCESS;

			}
			
			return _FAIL;
		
		}

		if (preorder_ctrl->enable == _FALSE)
		{
			//indicate this recv_frame			
			preorder_ctrl->indicate_seq = pattrib->seq_num;
			
			rtw_recv_indicatepkt(padapter, prframe);		
			
			preorder_ctrl->indicate_seq = (preorder_ctrl->indicate_seq + 1)%4096;
			
			return _SUCCESS;	
		}			

#ifndef CONFIG_RECV_REORDERING_CTRL
		//indicate this recv_frame
		rtw_recv_indicatepkt(padapter, prframe);
		return _SUCCESS;
#endif

	}
	else if(pattrib->amsdu==1) //temp filter -> means didn't support A-MSDUs in a A-MPDU
	{
	        if (preorder_ctrl->enable == _FALSE)
		{
			preorder_ctrl->indicate_seq = pattrib->seq_num;

			retval = amsdu_to_msdu(padapter, prframe);

			preorder_ctrl->indicate_seq = (preorder_ctrl->indicate_seq + 1)%4096;

			return retval;
		}
	}
	else
	{

	}

	_enter_critical_bh(&ppending_recvframe_queue->lock, &irql);

	RT_TRACE(_module_rtl871x_recv_c_, _drv_notice_,
		 ("recv_indicatepkt_reorder: indicate=%d seq=%d\n",
		  preorder_ctrl->indicate_seq, pattrib->seq_num));

	//s2. check if winstart_b(indicate_seq) needs to been updated
	if(!check_indicate_seq(preorder_ctrl, pattrib->seq_num))
	{
		//pHTInfo->RxReorderDropCounter++;
		//ReturnRFDList(Adapter, pRfd);
		//RT_TRACE(COMP_RX_REORDER, DBG_TRACE, ("RxReorderIndicatePacket() ==> Packet Drop!!\n"));
		//_exit_critical_ex(&ppending_recvframe_queue->lock, &irql);
		//return _FAIL;
		goto _err_exit;
	}


	//s3. Insert all packet into Reorder Queue to maintain its ordering.
	if(!enqueue_reorder_recvframe(preorder_ctrl, prframe))
	{
		//DbgPrint("recv_indicatepkt_reorder, enqueue_reorder_recvframe fail!\n");
		//_exit_critical_ex(&ppending_recvframe_queue->lock, &irql);
		//return _FAIL;
		goto _err_exit;
	}


	//s4.
	// Indication process.
	// After Packet dropping and Sliding Window shifting as above, we can now just indicate the packets
	// with the SeqNum smaller than latest WinStart and buffer other packets.
	//
	// For Rx Reorder condition:
	// 1. All packets with SeqNum smaller than WinStart => Indicate
	// 2. All packets with SeqNum larger than or equal to WinStart => Buffer it.
	//

	//recv_indicatepkts_in_order(padapter, preorder_ctrl, _TRUE);
	if(recv_indicatepkts_in_order(padapter, preorder_ctrl, _FALSE)==_TRUE)
	{
		_set_timer(&preorder_ctrl->reordering_ctrl_timer, REORDER_WAIT_TIME);
		_exit_critical_bh(&ppending_recvframe_queue->lock, &irql);	
	}
	else
	{
		_exit_critical_bh(&ppending_recvframe_queue->lock, &irql);
		_cancel_timer_ex(&preorder_ctrl->reordering_ctrl_timer);
	}


	return _SUCCESS;

_err_exit:

        _exit_critical_bh(&ppending_recvframe_queue->lock, &irql);

	return _FAIL;
}


void rtw_reordering_ctrl_timeout_handler(void *pcontext)
{
	_irqL irql;
	struct recv_reorder_ctrl *preorder_ctrl = (struct recv_reorder_ctrl *)pcontext;
	_adapter *padapter = preorder_ctrl->padapter;
	_queue *ppending_recvframe_queue = &preorder_ctrl->pending_recvframe_queue;


	if(padapter->bDriverStopped ||padapter->bSurpriseRemoved)
	{
		return;
	}

	//printk("+rtw_reordering_ctrl_timeout_handler()=>\n");

	_enter_critical_bh(&ppending_recvframe_queue->lock, &irql);

	recv_indicatepkts_in_order(padapter, preorder_ctrl, _TRUE);

	_exit_critical_bh(&ppending_recvframe_queue->lock, &irql);

}


static int process_recv_indicatepkts(_adapter *padapter, union recv_frame *prframe)
{
	int retval = _SUCCESS;
	struct recv_priv *precvpriv = &padapter->recvpriv;
	struct rx_pkt_attrib *pattrib = &prframe->u.hdr.attrib;
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;

#ifdef CONFIG_80211N_HT

	struct ht_priv	*phtpriv = &pmlmepriv->htpriv;

	if(phtpriv->ht_option==1) //B/G/N Mode
	{
		//prframe->u.hdr.preorder_ctrl = &precvpriv->recvreorder_ctrl[pattrib->priority];

		if(recv_indicatepkt_reorder(padapter, prframe)!=_SUCCESS)// including perform A-MPDU Rx Ordering Buffer Control
		{
			if ((padapter->bDriverStopped == _FALSE) &&
			    (padapter->bSurpriseRemoved == _FALSE))
			{
				retval = _FAIL;
				return retval;
			}
		}
	}
	else //B/G mode
#endif
	{
		retval=wlanhdr_to_ethhdr (prframe);
		if(retval != _SUCCESS)
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("wlanhdr_to_ethhdr: drop pkt \n"));
			return retval;
		}

		if ((padapter->bDriverStopped ==_FALSE)&&( padapter->bSurpriseRemoved==_FALSE))
		{
			//indicate this recv_frame
			RT_TRACE(_module_rtl871x_recv_c_, _drv_notice_, ("@@@@ process_recv_indicatepkts- recv_func rtw_recv_indicatepkt\n" ));
			rtw_recv_indicatepkt(padapter, prframe);


		}
		else
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_notice_, ("@@@@ process_recv_indicatepkts- recv_func free_indicatepkt\n" ));

			RT_TRACE(_module_rtl871x_recv_c_, _drv_notice_, ("recv_func:bDriverStopped(%d) OR bSurpriseRemoved(%d)", padapter->bDriverStopped, padapter->bSurpriseRemoved));
			retval = _FAIL;
			return retval;
		}

	}

	return retval;

}
 

static int recv_func(_adapter *padapter, void *pcontext)
{
	struct rx_pkt_attrib *pattrib;
	union recv_frame *prframe, *orig_prframe;
	int retval = _SUCCESS;
	_queue *pfree_recv_queue = &padapter->recvpriv.free_recv_queue;
	struct recv_priv *precvpriv = &padapter->recvpriv;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);


	prframe = (union recv_frame *)pcontext;
	orig_prframe = prframe;

	pattrib = &prframe->u.hdr.attrib;

#ifdef CONFIG_MP_INCLUDED
	if ((check_fwstate(pmlmepriv, WIFI_MP_STATE) == _TRUE))//&&(padapter->mppriv.check_mp_pkt == 0))
	{
		if (pattrib->crc_err == 1)
			padapter->mppriv.rx_crcerrpktcount++;
		else
			padapter->mppriv.rx_pktcount++;

		if (check_fwstate(pmlmepriv, WIFI_MP_LPBK_STATE) == _FALSE) {
			RT_TRACE(_module_rtl871x_recv_c_, _drv_alert_, ("MP - Not in loopback mode , drop pkt \n"));
			rtw_free_recvframe(orig_prframe, pfree_recv_queue);//free this recv_frame
			goto _exit_recv_func;
		}
	}
#endif

	//check the frame crtl field and decache
	retval = validate_recv_frame(padapter, prframe);
	if (retval != _SUCCESS)
	{
		RT_TRACE(_module_rtl871x_recv_c_, _drv_info_, ("recv_func: validate_recv_frame fail! drop pkt\n"));
		rtw_free_recvframe(orig_prframe, pfree_recv_queue);//free this recv_frame
		goto _exit_recv_func;
	}
	// DATA FRAME
	padapter->ledpriv.LedControlHandler(padapter, LED_CTL_RX);

	//pHalData->hal_ops.process_phy_info(padapter, prframe);

	prframe = decryptor(padapter, prframe);
	if (prframe == NULL) {
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("decryptor: drop pkt\n"));
		retval = _FAIL;
		goto _exit_recv_func;
	}

	prframe=portctrl(padapter, prframe);
	if(prframe==NULL)	{
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("portctrl: drop pkt \n"));
		retval = _FAIL;
		goto _exit_recv_func;		
	}

	prframe = recvframe_chk_defrag(padapter, prframe);
	if (prframe == NULL) {
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("recvframe_chk_defrag: drop pkt\n"));
		goto _exit_recv_func;
	}

	count_rx_stats(padapter, prframe);

#ifdef CONFIG_80211N_HT

	retval = process_recv_indicatepkts(padapter, prframe);
	if (retval != _SUCCESS)
	{
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("recv_func: process_recv_indicatepkts fail! \n"));
		rtw_free_recvframe(orig_prframe, pfree_recv_queue);//free this recv_frame
		goto _exit_recv_func;
	}

#else

	if (!pattrib->amsdu)
	{
		retval = wlanhdr_to_ethhdr (prframe);
		if (retval != _SUCCESS)
		{
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("wlanhdr_to_ethhdr: drop pkt \n"));
			rtw_free_recvframe(orig_prframe, pfree_recv_queue);//free this recv_frame
			goto _exit_recv_func;
		}

		if ((padapter->bDriverStopped == _FALSE) && (padapter->bSurpriseRemoved == _FALSE))
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_alert_, ("@@@@ recv_func: recv_func rtw_recv_indicatepkt\n" ));
			//indicate this recv_frame
			rtw_recv_indicatepkt(padapter, prframe);
		}
		else
		{
			RT_TRACE(_module_rtl871x_recv_c_, _drv_alert_, ("@@@@  recv_func: rtw_free_recvframe\n" ));
			RT_TRACE(_module_rtl871x_recv_c_, _drv_debug_, ("recv_func:bDriverStopped(%d) OR bSurpriseRemoved(%d)", padapter->bDriverStopped, padapter->bSurpriseRemoved));
			retval = _FAIL;
			rtw_free_recvframe(orig_prframe, pfree_recv_queue); //free this recv_frame
		}

	}
	else if(pattrib->amsdu==1)
	{

		retval = amsdu_to_msdu(padapter, prframe);
		if(retval != _SUCCESS)
		{
			rtw_free_recvframe(orig_prframe, pfree_recv_queue);
			goto _exit_recv_func;
		}
	}
	else
	{

	}
#endif


_exit_recv_func:

	return retval;
}


s32 rtw_recv_entry(union recv_frame *precvframe)
{
	_adapter *padapter;
	struct recv_priv *precvpriv;
	struct	mlme_priv	*pmlmepriv ;
	struct dvobj_priv *pdev;
	struct recv_stat *prxstat;
	u8 *phead, *pdata, *ptail,*pend;

	_queue *pfree_recv_queue, *ppending_recv_queue;
	u8 blk_mode = _FALSE;
	s32 ret=_SUCCESS;
	struct intf_hdl * pintfhdl;

_func_enter_;

//	RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("+rtw_recv_entry\n"));

	padapter = precvframe->u.hdr.adapter;
	pintfhdl = &padapter->iopriv.intf;

	pdev=&padapter->dvobjpriv;	
	pmlmepriv = &padapter->mlmepriv;
	precvpriv = &padapter->recvpriv;
	pfree_recv_queue = &precvpriv->free_recv_queue;
	ppending_recv_queue = &precvpriv->recv_pending_queue;

	phead = precvframe->u.hdr.rx_head;
	pdata = precvframe->u.hdr.rx_data;
	ptail = precvframe->u.hdr.rx_tail;
	pend = precvframe->u.hdr.rx_end;
	prxstat = (struct recv_stat *)phead;

	//padapter->ledpriv.LedControlHandler(padapter, LED_CTL_RX);

#ifdef CONFIG_SDIO_HCI
	if (precvpriv->free_recvframe_cnt <= 1)
		goto _recv_entry_drop;
#endif

#ifdef CONFIG_RECV_THREAD_MODE
	if (_rtw_queue_empty(ppending_recv_queue) == _TRUE)
	{
		//enqueue_recvframe_usb(precvframe, ppending_recv_queue);//enqueue to recv_pending_queue
	 	rtw_enqueue_recvframe(precvframe, ppending_recv_queue);
		_rtw_up_sema(&precvpriv->recv_sema);
	}
	else
	{
		//enqueue_recvframe_usb(precvframe, ppending_recv_queue);//enqueue to recv_pending_queue
		rtw_enqueue_recvframe(precvframe, ppending_recv_queue);
	}
#else
	if ((ret = recv_func(padapter, precvframe)) == _FAIL)
	{
		RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("rtw_recv_entry: recv_func return fail!!!\n"));
		goto _recv_entry_drop;
	}
#endif

	precvpriv->rx_pkts++;

_func_exit_;

	return ret;

_recv_entry_drop:


	precvpriv->rx_drop++;

#ifdef CONFIG_MP_INCLUDED
	padapter->mppriv.rx_pktloss = precvpriv->rx_drop;
#endif

	//RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("_recv_entry_drop\n"));

_func_exit_;

	return ret;
}

