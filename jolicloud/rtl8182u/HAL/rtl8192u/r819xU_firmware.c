/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
 * Linux device driver for RTL8192U 
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
#include "r8192U_hw.h"
#include "r819xU_firmware_img.h"
#include "r819xU_firmware.h"
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
#include <linux/firmware.h>
#endif

#ifdef RTK_DMP_PLATFORM
#include <linux/usb_setting.h> 
#endif

void firmware_init_param(struct net_device *dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	rt_firmware		*pfirmware = priv->pFirmware;

	pfirmware->cmdpacket_frag_thresold = GET_COMMAND_PACKET_FRAG_THRESHOLD(MAX_TRANSMIT_BUFFER_SIZE);
}

bool fw_download_code(struct net_device *dev, u8 *code_virtual_address, u32 buffer_len)
{
	struct r8192_priv   *priv = ieee80211_priv(dev);
	bool 		    rt_status = true;
	u16		    frag_threshold;
	u16		    frag_length, frag_offset = 0;
	int		    i;
	
	rt_firmware	    *pfirmware = priv->pFirmware;
	struct sk_buff	    *skb;
	unsigned char	    *seg_ptr;
	cb_desc		    *tcb_desc;	
	u8                  bLastIniPkt;

	firmware_init_param(dev);
	frag_threshold = pfirmware->cmdpacket_frag_thresold;
	do {
		if((buffer_len - frag_offset) > frag_threshold) {
			frag_length = frag_threshold ;
			bLastIniPkt = 0;

		} else {
			frag_length = buffer_len - frag_offset;
			bLastIniPkt = 1;

		}

		#ifdef RTL8192U
#ifdef USB_USE_ALIGNMENT
                u32 Tmpaddr;
                int alignment;
                skb  = dev_alloc_skb(USB_HWDESC_HEADER_LEN + frag_length + 4+ USB_512B_ALIGNMENT_SIZE);
#else
		skb  = dev_alloc_skb(USB_HWDESC_HEADER_LEN + frag_length + 4); 
#endif
		if (skb == NULL) {
			RT_TRACE(COMP_ERR, "dev_alloc_skb() failed! \n");
			rt_status = false;
			return rt_status;
		}

		#else
		skb  = dev_alloc_skb(frag_length + 4); 
		#endif
		memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
		tcb_desc = (cb_desc*)(skb->cb + MAX_DEV_ADDR_SIZE);
		tcb_desc->queue_index = TXCMD_QUEUE;
		tcb_desc->bCmdOrInit = DESC_PACKET_TYPE_INIT;
		tcb_desc->bLastIniPkt = bLastIniPkt;

#ifdef USB_USE_ALIGNMENT
                Tmpaddr = (u32)skb->data;
                alignment = Tmpaddr & 0x1ff;
                skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));     
#endif

		#ifdef RTL8192U
		skb_reserve(skb, USB_HWDESC_HEADER_LEN);
		#endif

		seg_ptr = skb->data;
		for(i=0 ; i < frag_length; i+=4) {
			*seg_ptr++ = ((i+0)<frag_length)?code_virtual_address[i+3]:0;
			*seg_ptr++ = ((i+1)<frag_length)?code_virtual_address[i+2]:0;
			*seg_ptr++ = ((i+2)<frag_length)?code_virtual_address[i+1]:0;
			*seg_ptr++ = ((i+3)<frag_length)?code_virtual_address[i+0]:0;
		}
		tcb_desc->txbuf_size= (u16)i;
		skb_put(skb, i);

		if(!priv->ieee80211->check_nic_enough_desc(dev,tcb_desc->queue_index)||
			(!skb_queue_empty(&priv->ieee80211->skb_waitQ[tcb_desc->queue_index]))||\
			(priv->ieee80211->queue_stop) ) {
			RT_TRACE(COMP_FIRMWARE,"=====================================================> tx full!\n");
			skb_queue_tail(&priv->ieee80211->skb_waitQ[tcb_desc->queue_index], skb);
		} else {
			priv->ieee80211->softmac_hard_start_xmit(skb,dev);
		}
		
		code_virtual_address += frag_length;
		frag_offset += frag_length;

	}while(frag_offset < buffer_len);
	
	return rt_status;

#if 0
cmdsend_downloadcode_fail:	
	rt_status = false;
	RT_TRACE(COMP_ERR, "CmdSendDownloadCode fail !!\n");
	return rt_status;
#endif
}

bool
fwSendNullPacket(
	struct net_device *dev,
	u32			Length
)
{
	bool	rtStatus = true;
	struct r8192_priv   *priv = ieee80211_priv(dev);
	struct sk_buff	    *skb;
	cb_desc		    *tcb_desc;	
	unsigned char	    *ptr_buf;
	bool	bLastInitPacket = false;
	

#ifdef USB_USE_ALIGNMENT
	u32 Tmpaddr;
	int alignment;
        skb  = dev_alloc_skb(Length+ 4+ USB_512B_ALIGNMENT_SIZE);
#else
	skb  = dev_alloc_skb(Length+ 4);
#endif
	memcpy((unsigned char *)(skb->cb),&dev,sizeof(dev));
	tcb_desc = (cb_desc*)(skb->cb + MAX_DEV_ADDR_SIZE);
	tcb_desc->queue_index = TXCMD_QUEUE;
	tcb_desc->bCmdOrInit = DESC_PACKET_TYPE_INIT;
	tcb_desc->bLastIniPkt = bLastInitPacket;

#ifdef USB_USE_ALIGNMENT
	Tmpaddr = (u32)skb->data;
	alignment = Tmpaddr & 0x1ff;
	skb_reserve(skb,(USB_512B_ALIGNMENT_SIZE - alignment));     
#endif

	ptr_buf = skb_put(skb, Length);
	memset(ptr_buf,0,Length);
	tcb_desc->txbuf_size= (u16)Length;

	if(!priv->ieee80211->check_nic_enough_desc(dev,tcb_desc->queue_index)||
			(!skb_queue_empty(&priv->ieee80211->skb_waitQ[tcb_desc->queue_index]))||\
			(priv->ieee80211->queue_stop) ) {
			RT_TRACE(COMP_FIRMWARE,"===================NULL packet==================================> tx full!\n");
			skb_queue_tail(&priv->ieee80211->skb_waitQ[tcb_desc->queue_index], skb);
		} else {
			priv->ieee80211->softmac_hard_start_xmit(skb,dev);
		}
	
	return rtStatus;
}

#if 0
bool fwsend_download_code(struct net_device *dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	rt_firmware		*pfirmware = (rt_firmware*)(&priv->firmware);

	bool			rt_status = true;
	u16			length = 0;
	u16			offset = 0;
	u16			frag_threhold;
	bool			last_init_packet = false;
	u32			check_txcmdwait_queueemptytime = 100000;
	u16			cmd_buf_len;
	u8			*ptr_cmd_buf;

	pfirmware->firmware_seg_index = 1;

	if(pfirmware->firmware_seg_index == pfirmware->firmware_seg_maxnum) {
		last_init_packet = 1;
	}

	cmd_buf_len = pfirmware->firmware_seg_container[pfirmware->firmware_seg_index-1].seg_size;
	ptr_cmd_buf = pfirmware->firmware_seg_container[pfirmware->firmware_seg_index-1].seg_ptr;
	rtl819xU_tx_cmd(dev, ptr_cmd_buf, cmd_buf_len, last_init_packet, DESC_PACKET_TYPE_INIT);

	rt_status = true;
	return rt_status;
}
#endif

bool CPUcheck_maincodeok_turnonCPU(struct net_device *dev)
{
	struct r8192_priv  *priv = ieee80211_priv(dev);
	bool rt_status = true;
	int		check_putcodeOK_time = 20000, check_bootOk_time = 20000;
	u32  CPU_status = 0;

	do {
		CPU_status = read_nic_dword(dev, CPU_GEN);

		if((CPU_status&CPU_GEN_PUT_CODE_OK) || (priv->usb_error==true))
			break;

	}while(check_putcodeOK_time--);

	if(!(CPU_status&CPU_GEN_PUT_CODE_OK)) {
		RT_TRACE(COMP_ERR, "Download Firmware: Put code fail!\n");
		goto CPUCheckMainCodeOKAndTurnOnCPU_Fail;
	} else {
		RT_TRACE(COMP_FIRMWARE, "Download Firmware: Put code ok!\n");
	}

	CPU_status = read_nic_dword(dev, CPU_GEN);
	write_nic_byte(dev, CPU_GEN, (u8)((CPU_status|CPU_GEN_PWR_STB_CPU)&0xff));
	mdelay(1000);

	do {
		CPU_status = read_nic_dword(dev, CPU_GEN);

		if((CPU_status&CPU_GEN_BOOT_RDY)||(priv->usb_error == true))
			break;
	}while(check_bootOk_time--);

	if(!(CPU_status&CPU_GEN_BOOT_RDY)) {
		goto CPUCheckMainCodeOKAndTurnOnCPU_Fail;
	} else {
		RT_TRACE(COMP_FIRMWARE, "Download Firmware: Boot ready!\n");
	}

	return rt_status;

CPUCheckMainCodeOKAndTurnOnCPU_Fail:	
	RT_TRACE(COMP_ERR, "ERR in %s()\n", __FUNCTION__);
	rt_status = FALSE;
	return rt_status;
}

bool CPUcheck_firmware_ready(struct net_device *dev)
{
	struct r8192_priv  *priv = ieee80211_priv(dev);
	bool		rt_status = true;
	int		check_time = 200000;
	u32		CPU_status = 0;

	do {
		CPU_status = read_nic_dword(dev, CPU_GEN);

		if((CPU_status&CPU_GEN_FIRM_RDY)||(priv->usb_error == true))
			break;
		
	}while(check_time--);

	if(!(CPU_status&CPU_GEN_FIRM_RDY))
		goto CPUCheckFirmwareReady_Fail;
	else
		RT_TRACE(COMP_FIRMWARE, "Download Firmware: Firmware ready!\n");

	return rt_status;
	
CPUCheckFirmwareReady_Fail:
	RT_TRACE(COMP_ERR, "ERR in %s()\n", __FUNCTION__);
	rt_status = false;
	return rt_status;

}

bool init_firmware(struct net_device *dev)
{
	struct r8192_priv 	*priv = ieee80211_priv(dev);
	bool			rt_status = TRUE;

	u8			*firmware_img_buf[3] = { &rtl8190_fwboot_array[0], 
						   	 &rtl8190_fwmain_array[0],
						   	 &rtl8190_fwdata_array[0]};

	u32			firmware_img_len[3] = { sizeof(rtl8190_fwboot_array), 
						   	sizeof(rtl8190_fwmain_array),
						   	sizeof(rtl8190_fwdata_array)};
	u32			file_length = 0;
	u8			*mapped_file = NULL;
	u32			init_step = 0;
	opt_rst_type_e	rst_opt = OPT_SYSTEM_RESET;
	firmware_init_step_e 	starting_state = FW_INIT_STEP0_BOOT;

	rt_firmware		*pfirmware = priv->pFirmware;
	const struct firmware 	*fw_entry;
	const char *fw_name[3] = { "RTL8192U/boot.img",
                           "RTL8192U/main.img",
			   "RTL8192U/data.img"};
	int rc;

	RT_TRACE(COMP_FIRMWARE, " PlatformInitFirmware()==>\n");

	if (pfirmware->firmware_status == FW_STATUS_0_INIT ) { 
		rst_opt = OPT_SYSTEM_RESET;
		starting_state = FW_INIT_STEP0_BOOT;

	}else if(pfirmware->firmware_status == FW_STATUS_5_READY) {
		rst_opt = OPT_FIRMWARE_RESET;
		starting_state = FW_INIT_STEP2_DATA;
	}else {
		 RT_TRACE(COMP_FIRMWARE, "PlatformInitFirmware: undefined firmware state\n");
	}
	
#if defined(USE_FW_SOURCE_HEADER_FILE) || defined(RTK_DMP_PLATFORM) || defined(WIFI_TEST)
	printk("Using FW_SOURCE_HEADER_FILE\n");
	priv->firmware_source = FW_SOURCE_HEADER_FILE;
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	priv->firmware_source = FW_SOURCE_HEADER_FILE;
#else
	priv->firmware_source = FW_SOURCE_IMG_FILE;
#endif
#endif
	for(init_step = starting_state; init_step <= FW_INIT_STEP2_DATA; init_step++) {
		if(rst_opt == OPT_SYSTEM_RESET) {
			switch(priv->firmware_source) {
				case FW_SOURCE_IMG_FILE:
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
					if(pfirmware->firmware_buf_size[init_step] == 0) {
						rc = request_firmware(&fw_entry, fw_name[init_step],&priv->udev->dev);
						if(rc < 0 ) {
							RT_TRACE(COMP_ERR, "request firmware fail!\n");
							goto download_firmware_fail;
						} 

						if(fw_entry->size > sizeof(pfirmware->firmware_buf[init_step])) {
							 RT_TRACE(COMP_FIRMWARE, "img file size exceed the container buffer fail!, entry_size = %d, buf_size = %d\n",fw_entry->size,sizeof(pfirmware->firmware_buf[init_step]));

							goto download_firmware_fail;
						}

						if(init_step != FW_INIT_STEP1_MAIN) {
							memcpy(pfirmware->firmware_buf[init_step],fw_entry->data,fw_entry->size);
							pfirmware->firmware_buf_size[init_step] = fw_entry->size;
						} else {
#ifdef RTL8190P
							memcpy(pfirmware->firmware_buf[init_step],fw_entry->data,fw_entry->size);
							pfirmware->firmware_buf_size[init_step] = fw_entry->size;
#else
							memset(pfirmware->firmware_buf[init_step],0,128);
							memcpy(&pfirmware->firmware_buf[init_step][128],fw_entry->data,fw_entry->size);
							pfirmware->firmware_buf_size[init_step] = fw_entry->size+128;
#endif
						}

						if(rst_opt == OPT_SYSTEM_RESET) {
							release_firmware(fw_entry);
						}
					}
					mapped_file = pfirmware->firmware_buf[init_step];
					file_length = pfirmware->firmware_buf_size[init_step];
#endif

					break;	

				case FW_SOURCE_HEADER_FILE:
					mapped_file =  firmware_img_buf[init_step];
					file_length  = firmware_img_len[init_step];
					if(init_step == FW_INIT_STEP2_DATA) {
						memcpy(pfirmware->firmware_buf[init_step], mapped_file, file_length);
						pfirmware->firmware_buf_size[init_step] = file_length;
					}
					break;

				default:
					break;
			}


		}else if(rst_opt == OPT_FIRMWARE_RESET ) {
			mapped_file = pfirmware->firmware_buf[init_step];
			file_length = pfirmware->firmware_buf_size[init_step];
		}

		rt_status = fw_download_code(dev,mapped_file,file_length);

		if(rt_status != TRUE) {
			goto download_firmware_fail;
		}

		switch(init_step) {
			case FW_INIT_STEP0_BOOT:
				pfirmware->firmware_status = FW_STATUS_1_MOVE_BOOT_CODE;
#ifdef RTL8190P
				rt_status = fwSendNullPacket(dev, RTL8190_CPU_START_OFFSET);
				if(rt_status != true)
				{
					RT_TRACE(COMP_INIT, "fwSendNullPacket() fail ! \n");
					goto  download_firmware_fail;						
				}
#endif
				break;

			case FW_INIT_STEP1_MAIN:
				pfirmware->firmware_status = FW_STATUS_2_MOVE_MAIN_CODE;

				rt_status = CPUcheck_maincodeok_turnonCPU(dev);
				if(rt_status != TRUE) {	
					RT_TRACE(COMP_ERR, "CPUcheck_maincodeok_turnonCPU fail!\n");
					goto download_firmware_fail;
				}

				pfirmware->firmware_status = FW_STATUS_3_TURNON_CPU;
				break;

			case FW_INIT_STEP2_DATA:
				pfirmware->firmware_status = FW_STATUS_4_MOVE_DATA_CODE;
				mdelay(1);

				rt_status = CPUcheck_firmware_ready(dev);
				if(rt_status != TRUE) {				
					RT_TRACE(COMP_ERR, "CPUcheck_firmware_ready fail(%d)!\n",rt_status);
					goto download_firmware_fail;
				}

				pfirmware->firmware_status = FW_STATUS_5_READY;
				break;
		}
	}

	RT_TRACE(COMP_FIRMWARE, "Firmware Download Success\n");

	return rt_status;	

download_firmware_fail:	
	RT_TRACE(COMP_ERR, "ERR in %s()\n", __FUNCTION__);
	rt_status = FALSE;
	return rt_status;

}

#if 0
void CmdAppendZeroAndEndianTransform(
	u1Byte	*pDst,
	u1Byte	*pSrc,
	u2Byte   	*pLength)
{

	u2Byte	ulAppendBytes = 0, i;
	u2Byte	ulLength = *pLength;



#if 0
	for( i=0 ; i<(*pLength) ; i+=4)
	{
		if((i+3) < (*pLength))	pDst[i+0] = pSrc[i+3];
		if((i+2) < (*pLength))	pDst[i+1] = pSrc[i+2];
		if((i+1) < (*pLength))	pDst[i+2] = pSrc[i+1];
		if((i+0) < (*pLength))	pDst[i+3] = pSrc[i+0];
	}
#else
	pDst += USB_HWDESC_HEADER_LEN;
	ulLength -= USB_HWDESC_HEADER_LEN;

	for( i=0 ; i<ulLength ; i+=4) {
		if((i+3) < ulLength)	pDst[i+0] = pSrc[i+3];
		if((i+2) < ulLength)	pDst[i+1] = pSrc[i+2];
		if((i+1) < ulLength)	pDst[i+2] = pSrc[i+1];
		if((i+0) < ulLength)	pDst[i+3] = pSrc[i+0];

	}
#endif	
	
	if(  ((*pLength) % 4)  >0)
	{
		ulAppendBytes = 4-((*pLength) % 4);

		for(i=0 ; i<ulAppendBytes; i++)
			pDst[  4*((*pLength)/4)  + i ] = 0x0;

		*pLength += ulAppendBytes;	
	}
}
#endif

#if 0	
RT_STATUS
CmdSendPacket(
	PADAPTER				Adapter,
	PRT_TCB					pTcb,
	PRT_TX_LOCAL_BUFFER 			pBuf,
	u4Byte					BufferLen,
	u4Byte					PacketType,
	BOOLEAN					bLastInitPacket
	)
{
	s2Byte		i;
	u1Byte		QueueID;
	u2Byte		firstDesc,curDesc = 0;
	u2Byte		FragIndex=0, FragBufferIndex=0;

	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	
	CmdInitTCB(Adapter, pTcb, pBuf, BufferLen);

	
	if(CmdCheckFragment(Adapter, pTcb, pBuf))
		CmdFragmentTCB(Adapter, pTcb);
	else
		pTcb->FragLength[0] = (u2Byte)pTcb->BufferList[0].Length;

	QueueID=pTcb->SpecifiedQueueID;
#if DEV_BUS_TYPE!=USB_INTERFACE	
	firstDesc=curDesc=Adapter->NextTxDescToFill[QueueID];
#endif
	
#if DEV_BUS_TYPE!=USB_INTERFACE
	if(VacancyTxDescNum(Adapter, QueueID) > pTcb->BufferCount)
#else
	if(PlatformIsTxQueueAvailable(Adapter, QueueID, pTcb->BufferCount) &&
		RTIsListEmpty(&Adapter->TcbWaitQueue[QueueID]))
#endif
	{
		pTcb->nDescUsed=0;

		for(i=0 ; i<pTcb->BufferCount ; i++)
		{
			Adapter->HalFunc.TxFillCmdDescHandler(
				Adapter,	
				pTcb,
				QueueID,							
				curDesc,							
				FragBufferIndex==0,						
				FragBufferIndex==(pTcb->FragBufCount[FragIndex]-1),		
				pTcb->BufferList[i].VirtualAddress,				
				pTcb->BufferList[i].PhysicalAddressLow,				
				pTcb->BufferList[i].Length,					
				i!=0,								
				(i==(pTcb->BufferCount-1)) && bLastInitPacket,			
				PacketType,							
				pTcb->FragLength[FragIndex]					
				);

			if(FragBufferIndex==(pTcb->FragBufCount[FragIndex]-1))
			{ 
				pTcb->nFragSent++;
			}

			FragBufferIndex++;
			if(FragBufferIndex==pTcb->FragBufCount[FragIndex])
			{
				FragIndex++;
				FragBufferIndex=0;
			}
			
#if DEV_BUS_TYPE!=USB_INTERFACE	
			curDesc=(curDesc+1)%Adapter->NumTxDesc[QueueID];
#endif
			pTcb->nDescUsed++;
		}

#if DEV_BUS_TYPE!=USB_INTERFACE
		RTInsertTailList(&Adapter->TcbBusyQueue[QueueID], &pTcb->List);
		IncrementTxDescToFill(Adapter, QueueID, pTcb->nDescUsed);
		Adapter->HalFunc.SetTxDescOWNHandler(Adapter, QueueID, firstDesc);
		Adapter->HalFunc.TxPollingHandler(Adapter, TXCMD_QUEUE);
#endif		
	}
	else
#if DEV_BUS_TYPE!=USB_INTERFACE	
		goto CmdSendPacket_Fail;
#else
	{
		pTcb->bLastInitPacket = bLastInitPacket;
		RTInsertTailList(&Adapter->TcbWaitQueue[pTcb->SpecifiedQueueID], &pTcb->List);
	}
#endif

	return rtStatus;

#if DEV_BUS_TYPE!=USB_INTERFACE	
CmdSendPacket_Fail:	
	rtStatus = RT_STATUS_FAILURE;
	return rtStatus;
#endif	
	
}
#endif




#if 0
RT_STATUS
FWSendNullPacket(
	IN	PADAPTER		Adapter,
	IN	u4Byte			Length
)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;


	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	BOOLEAN					bLastInitPacket = FALSE;
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	
#if DEV_BUS_TYPE==USB_INTERFACE
	Length += USB_HWDESC_HEADER_LEN;
#endif

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		PlatformZeroMemory(pBuf->Buffer.VirtualAddress, Length);
		rtStatus = CmdSendPacket(Adapter, pTcb, pBuf, Length, DESC_PACKET_TYPE_INIT, bLastInitPacket);	
		if(rtStatus == RT_STATUS_FAILURE)
			goto CmdSendNullPacket_Fail;
	}else
		goto CmdSendNullPacket_Fail;

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
	return rtStatus;

	
CmdSendNullPacket_Fail:	
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
	rtStatus = RT_STATUS_FAILURE;
	RT_ASSERT(rtStatus == RT_STATUS_SUCCESS, ("CmdSendDownloadCode fail !!\n"));
	return rtStatus;
}
#endif


