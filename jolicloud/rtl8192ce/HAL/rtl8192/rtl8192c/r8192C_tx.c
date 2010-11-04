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

u8 rtl8192ce_MRateToHwRate(struct net_device*dev, u8 rate)
{
	u8		ret = DESC92S_RATE1M;
	u16		max_sg_rate;
	static	u16	multibss_sg_old = 0x1234;
	u16		multibss_sg;
	struct r8192_priv *priv = rtllib_priv(dev);
		
	switch(rate)
	{
	case MGN_1M:		ret = DESC92S_RATE1M;		break;
	case MGN_2M:		ret = DESC92S_RATE2M;		break;
	case MGN_5_5M:	ret = DESC92S_RATE5_5M;	break;
	case MGN_11M:	ret = DESC92S_RATE11M;	break;
	case MGN_6M:		ret = DESC92S_RATE6M;		break;
	case MGN_9M:		ret = DESC92S_RATE9M;		break;
	case MGN_12M:	ret = DESC92S_RATE12M;	break;
	case MGN_18M:	ret = DESC92S_RATE18M;	break;
	case MGN_24M:	ret = DESC92S_RATE24M;	break;
	case MGN_36M:	ret = DESC92S_RATE36M;	break;
	case MGN_48M:	ret = DESC92S_RATE48M;	break;
	case MGN_54M:	ret = DESC92S_RATE54M;	break;

	case MGN_MCS0:	ret = DESC92S_RATEMCS0;	break;
	case MGN_MCS1:	ret = DESC92S_RATEMCS1;	break;
	case MGN_MCS2:	ret = DESC92S_RATEMCS2;	break;
	case MGN_MCS3:	ret = DESC92S_RATEMCS3;	break;
	case MGN_MCS4:	ret = DESC92S_RATEMCS4;	break;
	case MGN_MCS5:	ret = DESC92S_RATEMCS5;	break;
	case MGN_MCS6:	ret = DESC92S_RATEMCS6;	break;
	case MGN_MCS7:	ret = DESC92S_RATEMCS7;	break;
	case MGN_MCS8:	ret = DESC92S_RATEMCS8;	break;
	case MGN_MCS9:	ret = DESC92S_RATEMCS9;	break;
	case MGN_MCS10:	ret = DESC92S_RATEMCS10;	break;
	case MGN_MCS11:	ret = DESC92S_RATEMCS11;	break;
	case MGN_MCS12:	ret = DESC92S_RATEMCS12;	break;
	case MGN_MCS13:	ret = DESC92S_RATEMCS13;	break;
	case MGN_MCS14:	ret = DESC92S_RATEMCS14;	break;
	case MGN_MCS15:	ret = DESC92S_RATEMCS15;	break;	

	case MGN_MCS0_SG:	
	case MGN_MCS1_SG:
	case MGN_MCS2_SG:	
	case MGN_MCS3_SG:	
	case MGN_MCS4_SG:	
	case MGN_MCS5_SG:
	case MGN_MCS6_SG:	
	case MGN_MCS7_SG:	
	case MGN_MCS8_SG:	
	case MGN_MCS9_SG:
	case MGN_MCS10_SG:
	case MGN_MCS11_SG:	
	case MGN_MCS12_SG:	
	case MGN_MCS13_SG:	
	case MGN_MCS14_SG:
	case MGN_MCS15_SG:	

		ret = DESC92S_RATEMCS15_SG;
		max_sg_rate = rate&0xf;
		multibss_sg = max_sg_rate | (max_sg_rate<<4) | (max_sg_rate<<8) | (max_sg_rate<<12);
		if (multibss_sg_old != multibss_sg)
		{
#ifdef MERGE_TO_DO
			write_nic_dword(dev, SG_RATE, multibss_sg);
#endif
			multibss_sg_old = multibss_sg;
		}
	break;

	case (0x80|0x20): 	
		ret = DESC92S_RATEMCS32; 
	break;

	default:				
		if(priv->card_8192 == NIC_8192DE){
			if(read_nic_byte(dev, 0x2c) &0x02)
				ret = DESC92S_RATEMCS7;
			else
				ret = DESC92S_RATEMCS15;
			break;
				
		}
		ret = DESC92S_RATEMCS15;	
	break;

	}

	return ret;
}

u8 rtl8192ce_QueryIsShort(u8 TxHT, u8 TxRate, cb_desc *tcb_desc)
{
	u8   tmp_Short;

	tmp_Short = (TxHT==1)?((tcb_desc->bUseShortGI)?1:0):((tcb_desc->bUseShortPreamble)?1:0);
#if defined RTL8192SE || defined RTL8192CE
	if(TxHT==1 && TxRate != DESC92S_RATEMCS15)
#elif defined RTL8192E || defined RTL8190P
	if(TxHT==1 && TxRate != DESC90_RATEMCS15)
#endif
		tmp_Short = 0;

	return tmp_Short;
}

u8 rtl8192ce_MapHwQueueToFirmwareQueue(u8 QueueID, u8 priority)
{

	u8 QueueSelect = 0x0;	

	switch(QueueID)																					
	{
		case BE_QUEUE:
			QueueSelect = priority;			
			break;
			
		case BK_QUEUE:
			QueueSelect = priority;
			break;

		case VO_QUEUE:
			QueueSelect = priority;
			break;
			
		case VI_QUEUE:
			QueueSelect = priority;
			break;
			
		case MGNT_QUEUE:
			QueueSelect = QSLT_MGNT;
			break;
			
		case BEACON_QUEUE:			
			QueueSelect = QSLT_BEACON;			
			break;
			
			
		
			
		default:
			RT_ASSERT(false, ("TransmitTCB(): Impossible Queue Selection: %d \n", QueueID));
			break;
	}

	return QueueSelect;
}

void  rtl8192ce_tx_fill_desc(struct net_device* dev, tx_desc * pDesc_tx, cb_desc * cb_desc, struct sk_buff* skb)
{	
	struct r8192_priv* 	priv = rtllib_priv(dev);

	bool				bFirstSeg		= 1;
	bool				bLastSeg		= 1;

	PRT_HIGH_THROUGHPUT		pHTInfo = priv->rtllib->pHTInfo;
	struct rtllib_hdr_4addr * header = NULL;
	u16 				fc=0, stype=0,sc=0,seq=0;
	u8*	pDesc = (u8*)pDesc_tx;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(priv);

	dma_addr_t mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
	
	header = (struct rtllib_hdr_4addr *)(((u8*)skb->data));
	fc = header->frame_ctl;
	sc = le16_to_cpu(header->seq_ctl);
	stype = WLAN_FC_GET_STYPE(fc);
	seq = WLAN_GET_SEQ_SEQ(sc);
	
	RT_TRACE(COMP_SEND, "==>TxFillDescriptor8192CE\n");
	
	
	CLEAR_PCI_TX_DESC_CONTENT(pDesc, sizeof(tx_desc));
	RT_TRACE(COMP_SEND, "Clear TxDesc OK.\n");

	{
#if (EARLYMODE_ENABLE_FOR_92D== 1)
		if(priv->card_8192 == NIC_8192DE ){	
			{
				SET_TX_DESC_PKT_OFFSET(pDesc,1);
				SET_TX_DESC_OFFSET(pDesc, USB_HWDESC_HEADER_LEN+8);

				memcpy(tmpBuf, VirtualAddress, BufferLen);
				memcpy(VirtualAddress + 8 , tmpBuf ,BufferLen);
				
				memset(VirtualAddress, 0, 8);
				cb_desc->BufferList[nBufIndex].Length += 8;
				BufferLen += 8; 
				offset = 8;
				if(pTcb->EMPktNum)
				{
					InsertEMContent(pTcb,VirtualAddress);
					
				}
			}

		}
#else
		SET_TX_DESC_OFFSET(pDesc, USB_HWDESC_HEADER_LEN);
#endif
	

#if (MP_CCK_WORKAROUND == 1)
		if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40 &&
			rtllib_is_cck_rate(cb_desc->data_rate)){	
			cb_desc->data_rate = MGN_6M;
		}
#endif
		RT_TRACE(COMP_SEND,"pTcb->dataRate:%x. Fill :%X\n",cb_desc->data_rate,rtl8192ce_MRateToHwRate(dev, (u8)cb_desc->data_rate));
		SET_TX_DESC_TX_RATE(pDesc, rtl8192ce_MRateToHwRate(dev, (u8)cb_desc->data_rate));
		SET_TX_DESC_DATA_SHORTGI(pDesc, rtl8192ce_QueryIsShort(((cb_desc->data_rate & 0x80) ? 1 : 0), rtl8192ce_MRateToHwRate(dev, (u8)cb_desc->data_rate), cb_desc));
		SET_TX_DESC_AGG_BREAK(pDesc, ((cb_desc->bAMPDUEnable) ? 1 : 0));
		if(cb_desc->bAMPDUEnable)
		{
			SET_TX_DESC_MAX_AGG_NUM(pDesc, 0x14);
			
		}
		
		SET_TX_DESC_SEQ(pDesc, seq);
			
		SET_TX_DESC_RTS_ENABLE(pDesc, ((cb_desc->bRTSEnable && !cb_desc->bCTSEnable) ? 1 : 0));
		SET_TX_DESC_HW_RTS_ENABLE(pDesc,((cb_desc->bRTSEnable ||cb_desc->bCTSEnable) ? 1 : 0));
		SET_TX_DESC_CTS2SELF(pDesc, ((cb_desc->bCTSEnable) ? 1 : 0));
		SET_TX_DESC_RTS_STBC(pDesc, ((cb_desc->bRTSSTBC) ? 1 : 0));

		SET_TX_DESC_RTS_RATE(pDesc, rtl8192ce_MRateToHwRate(dev, (u8)cb_desc->rts_rate));
		SET_TX_DESC_RTS_BW(pDesc, 0);
		SET_TX_DESC_RTS_SC(pDesc, cb_desc->RTSSC);
		SET_TX_DESC_RTS_SHORT(pDesc, (((cb_desc->rts_rate & 0x80) == 0) ? (cb_desc->bRTSUseShortPreamble ? 1 : 0) : (cb_desc->bRTSUseShortGI? 1 : 0)));

		if(priv->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
		{
			if(cb_desc->bPacketBW)
			{
				if(cb_desc->bBTTxPacket)
				{
					SET_TX_DESC_DATA_BW(pDesc, 0);
					SET_TX_DESC_TX_SUB_CARRIER(pDesc, priv->nCur40MhzPrimeSC);
				}
				else
				{
					SET_TX_DESC_DATA_BW(pDesc, 1);
					SET_TX_DESC_TX_SUB_CARRIER(pDesc, 3);
				}
			}
			else
			{
				SET_TX_DESC_DATA_BW(pDesc, 0);
				SET_TX_DESC_TX_SUB_CARRIER(pDesc, priv->nCur40MhzPrimeSC);
			}
		}
		else
		{
			SET_TX_DESC_DATA_BW(pDesc, 0);
			SET_TX_DESC_TX_SUB_CARRIER(pDesc, 0);		
		}


		/*DWORD 0*/	
		SET_TX_DESC_LINIP(pDesc, 0);		
		SET_TX_DESC_PKT_SIZE(pDesc, (u16)skb->len);
		
		/*DWORD 1*/
		
#ifdef MERGE_TO_DO
		if( cb_desc->bEncrypt && !tempAdapter->MgntInfo.SecurityInfo.SWTxEncryptFlag)
		{
			EncAlg = SecGetEncryptionOverhead(
					tempAdapter,
					&EncryptionMPDUHeadOverhead, 
					&EncryptionMPDUTailOverhead, 
					NULL, 
					NULL,
					false,
					false);
			MPDUOverhead = EncryptionMPDUTailOverhead;
		}
		else
		{
			MPDUOverhead = 0;
		}
#endif
		SET_TX_DESC_AMPDU_DENSITY(pDesc,cb_desc->ampdu_density);

		if (cb_desc->bHwSec) {
       			static u8 tmp =0;
       			if (!tmp) {
            		 	tmp = 1;
	        	}
#ifdef _RTL8192_EXT_PATCH_
			if(cb_desc->mesh_pkt == 0)
#endif
			{
			switch (priv->rtllib->pairwise_key_type) {
				case KEY_TYPE_NA:
					SET_TX_DESC_SEC_TYPE(pDesc, 0x0);
					break;
				case KEY_TYPE_WEP40:
				case KEY_TYPE_WEP104:
				case KEY_TYPE_TKIP:
					SET_TX_DESC_SEC_TYPE(pDesc, 0x1);	
					break;
#ifdef MERGE_TO_DO
				case WAPI_Encryption:   
					SET_TX_DESC_SEC_TYPE(pDesc, 0x2);	
					break;
#endif
				case KEY_TYPE_CCMP:
					SET_TX_DESC_SEC_TYPE(pDesc, 0x3);	
					break;
				default:
					SET_TX_DESC_SEC_TYPE(pDesc, 0x0);	
					break;

			}
		}
#ifdef _RTL8192_EXT_PATCH_
			else if(cb_desc->mesh_pkt == 1)
			{
				switch (priv->rtllib->mesh_pairwise_key_type) {
					case KEY_TYPE_NA:
						SET_TX_DESC_SEC_TYPE(pDesc, 0x0);
						break;
					case KEY_TYPE_WEP40:
					case KEY_TYPE_WEP104:
					case KEY_TYPE_TKIP:
						SET_TX_DESC_SEC_TYPE(pDesc, 0x1);	
						break;
#ifdef MERGE_TO_DO
					case WAPI_Encryption:   
						SET_TX_DESC_SEC_TYPE(pDesc, 0x2);	
						break;
#endif
					case KEY_TYPE_CCMP:
						SET_TX_DESC_SEC_TYPE(pDesc, 0x3);	
						break;
					default:
						SET_TX_DESC_SEC_TYPE(pDesc, 0x0);	
						break;
				}
			}
#endif			
		}
		SET_TX_DESC_PKT_ID(pDesc, 0);
		
		
		SET_TX_DESC_QUEUE_SEL(pDesc,  rtl8192ce_MapHwQueueToFirmwareQueue(cb_desc->queue_index, cb_desc->priority));

		SET_TX_DESC_DATA_RATE_FB_LIMIT(pDesc, 0x1F); 
		SET_TX_DESC_RTS_RATE_FB_LIMIT(pDesc, 0xF); 
		SET_TX_DESC_DISABLE_FB(pDesc, cb_desc->bTxDisableRateFallBack?1:0);		
		SET_TX_DESC_USE_RATE(pDesc, cb_desc->bTxUseDriverAssingedRate?1:0);		

		if (cb_desc->bTxUseDriverAssingedRate && (cb_desc->data_rate & 0x80))		
			RF_ChangeTxPath(dev, cb_desc->data_rate);

#if 1
		if(IsQoSDataFrame(skb->data))		
		{	
			SET_TX_DESC_QOS(pDesc, 1);
			if(pHTInfo->bRDGEnable)
			{
				RT_TRACE(COMP_SEND,"Enable RDG function.\n");
				SET_TX_DESC_RDG_ENABLE(pDesc,1);
				SET_TX_DESC_HTC(pDesc,1);
			}
		}
#endif


#ifdef MERGE_TO_DO
		if(MgntActQuery_ApType(tempAdapter) == RT_AP_TYPE_VWIFI_AP)
		{
			if((EF1Byte(pFrame[0]) & 0xfc) == Type_Probe_Rsp)
			{
				SET_TX_DESC_DATA_RETRY_LIMIT(pDesc, 0);
				SET_TX_DESC_RETRY_LIMIT_ENABLE(pDesc, 1);
			}
			else if(GET_80211_HDR_TYPE(pFrame) == 0) 
			{
				if(pMgntInfo->dot11CurrentWirelessMode & (WIRELESS_MODE_B | WIRELESS_MODE_G))
				{
					SET_TX_DESC_DATA_RETRY_LIMIT(pDesc, 3);
					SET_TX_DESC_RETRY_LIMIT_ENABLE(pDesc, 1);
				}
			}

			if(cb_desc->bAMPDUEnable)
			{
				u8	FactorToSet = (1<<(cb_desc->AMPDUFactor+ 2));
			
				if(FactorToSet>0xf)
					FactorToSet = 0xf;
		
				SET_TX_DESC_USE_MAX_LEN(pDesc, 1);
				SET_TX_DESC_MCS15_SGI_MAX_LEN(pDesc, (FactorToSet>0xb)?0xb:FactorToSet); 
				SET_TX_DESC_MCSG6_MAX_LEN(pDesc, (FactorToSet>0x9)?0x9:FactorToSet); 
				SET_TX_DESC_MCSG5_MAX_LEN(pDesc, (FactorToSet>0x7)?0x7:FactorToSet); 
				SET_TX_DESC_MCSG4_MAX_LEN(pDesc, (FactorToSet>0x2)?0x2:FactorToSet); 
				SET_TX_DESC_MCS7_SGI_MAX_LEN(pDesc, (FactorToSet>0xa)?0xa:FactorToSet); 
				SET_TX_DESC_MCSG3_MAX_LEN(pDesc, (FactorToSet>0x8)?0x8:FactorToSet); 
				SET_TX_DESC_MCSG2_MAX_LEN(pDesc, (FactorToSet>0x4)?0x4:FactorToSet); 
				SET_TX_DESC_MCSG1_MAX_LEN(pDesc, (FactorToSet>0x1)?0x1:FactorToSet); 
			}
		}
#endif
	}

	SET_TX_DESC_FIRST_SEG(pDesc, (bFirstSeg ? 1 : 0));
	SET_TX_DESC_LAST_SEG(pDesc, (bLastSeg ? 1 : 0));
	
	SET_TX_DESC_TX_BUFFER_SIZE(pDesc, (u16) skb->len);
	
	SET_TX_DESC_TX_BUFFER_ADDRESS(pDesc, cpu_to_le32(mapping));

	if(priv->rtllib->bUseRAMask)
	{
		SET_TX_DESC_RATE_ID(pDesc, cb_desc->RATRIndex);
		SET_TX_DESC_MACID(pDesc, cb_desc->macId);
	}
	else
	{
		SET_TX_DESC_RATE_ID(pDesc, 0xC + cb_desc->RATRIndex);
		SET_TX_DESC_MACID(pDesc, cb_desc->RATRIndex);
	}

#if 1
	if(IsQoSDataFrame(skb->data))		
	{	
		SET_TX_DESC_QOS(pDesc, 1);
	}
#endif



#if 0 
	if(QueueIndex == BEACON_QUEUE)
	{
		SET_TX_DESC_FIRST_SEG(pDesc, 1); 
		SET_TX_DESC_LAST_SEG(pDesc, 1); 
		
		SET_TX_DESC_OFFSET(pDesc, 0x20); 

		SET_TX_DESC_USE_RATE(pDesc, 1); 

	}
#endif
	if(!IsQoSDataFrame(skb->data) && pPSC->bLeisurePs && pPSC->bFwCtrlLPS)
	{
		SET_TX_DESC_HWSEQ_EN(pDesc, 1); 
		SET_TX_DESC_MORE_FRAG(pDesc, (bLastSeg ? 1 : 0));
		SET_TX_DESC_PKT_ID(pDesc, 8); 
	}
	
#ifdef DUMP_SKB_DATA
	if((stype == RTLLIB_STYPE_ASSOC_REQ) || (stype == RTLLIB_STYPE_AUTH))
	{
		if(stype == RTLLIB_STYPE_ASSOC_REQ) 
			printk("RTLLIB_STYPE_ASSOC_REQ\n");
		if(stype == RTLLIB_STYPE_AUTH)
			printk("RTLLIB_STYPE_AUTH\n");
		int i;
		printk("\n---------------------------\n");
		for (i = 0; i < skb->len; i++){
			printk("%2.2x ", ((u8*)skb->data)[i]);
			if(i%32 == 0 && i!=0)
				printk("\n");
		}
		printk("\n---------------------------\n");
	}
#endif

	RT_TRACE(COMP_SEND, "TxFillDescriptor8192CE<==\n");

}


void  rtl8192ce_tx_fill_cmd_desc(struct net_device* dev, tx_desc_cmd * entry, cb_desc * cb_desc, struct sk_buff* skb)
{	
	struct r8192_priv *priv = rtllib_priv(dev);
	dma_addr_t mapping = pci_map_single(priv->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
	memset((void*)entry, 0, 32); 	
#ifdef MERGE_TO_DO
	if(/*DescPacketType*/cb_desc->bCmdOrInit == DESC_PACKET_TYPE_INIT) 
	{ 


		CLEAR_PCI_TX_DESC_CONTENT(entry, sizeof(tx_desc));

		SET_TX_DESC_LINIP((u8*)entry, bLastInitPacket);
		
		SET_TX_DESC_FIRST_SEG((u8*)entry, 1); 
		SET_TX_DESC_LAST_SEG((u8*)entry, 1); 
		
		SET_TX_DESC_PKT_SIZE((u8*)entry, (u16)(BufferLen));
		SET_TX_DESC_TX_BUFFER_SIZE((u8*)entry, (u2Byte)(BufferLen));
		SET_TX_DESC_TX_BUFFER_ADDRESS((u8*)entry, PhyAddressLow);

		SET_TX_DESC_OWN((u8*)entry, 1);
		Adapter->TxStats.NumTxDescFill[QueueIndex] ++;	
	}
	else
#endif
	{ 
		
		bool	bFirstSeg = 0;
		PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(priv);

		struct rtllib_hdr_4addr * header = (struct rtllib_hdr_4addr *)(((u8*)skb->data));
		u16	sc = le16_to_cpu(header->seq_ctl);
		u16	seq = WLAN_GET_SEQ_SEQ(sc);


		

		CLEAR_PCI_TX_DESC_CONTENT(entry, sizeof(tx_desc));

		if(bFirstSeg)
		{
			SET_TX_DESC_OFFSET(entry, USB_HWDESC_HEADER_LEN);
		}

		SET_TX_DESC_TX_RATE(entry,  DESC92S_RATE1M);


		SET_TX_DESC_SEQ(entry, seq);
		


		/*DWORD 0*/ 
		SET_TX_DESC_LINIP(entry, 0);		
		
		
		SET_TX_DESC_QUEUE_SEL(entry,  rtl8192ce_MapHwQueueToFirmwareQueue(cb_desc->queue_index, cb_desc->priority));

	
		SET_TX_DESC_FIRST_SEG(entry, 1);
		SET_TX_DESC_LAST_SEG(entry, 1);
		
		SET_TX_DESC_TX_BUFFER_SIZE((u8*)entry, (u16)(skb->len));	
		
		SET_TX_DESC_TX_BUFFER_ADDRESS((u8*)entry, cpu_to_le32(mapping));
	
			SET_TX_DESC_RATE_ID(entry, cb_desc->RATRIndex);
			SET_TX_DESC_MACID(entry, cb_desc->macId);
	
	
		{
			SET_TX_DESC_OWN(entry, 1);
		}

		
		SET_TX_DESC_PKT_SIZE((u8*)entry, (u16)(skb->len));

		SET_TX_DESC_FIRST_SEG(entry, 1); 
		SET_TX_DESC_LAST_SEG(entry, 1); 

		SET_TX_DESC_OFFSET(entry, 0x20); 

		SET_TX_DESC_USE_RATE(entry, 1); 

		
		if(!IsQoSDataFrame(skb->data) && pPSC->bFwCtrlLPS)
		{
			SET_TX_DESC_HWSEQ_EN(entry, 1); 
			SET_TX_DESC_PKT_ID(entry, 8); 
		}

		RT_PRINT_DATA(COMP_CMD, DBG_LOUD, ("TxFillCmdDesc8192SE(): H2C Tx Cmd Content ----->\n"), entry, sizeof(tx_desc));
	}
}


