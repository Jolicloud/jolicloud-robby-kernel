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
#define _RTL8192C_CMD_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <cmd_osdep.h>
#include <mlme_osdep.h>
#include <rtw_byteorder.h>
#include <circ_buf.h>
#include <rtw_ioctl_set.h>


#ifdef PLATFORM_LINUX
#ifdef CONFIG_SDIO_HCI
#include <linux/mmc/sdio_func.h>
#endif
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
#include <linux/usb.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
#include <linux/usb_ch9.h>
#else
#include <linux/usb/ch9.h>
#endif
#include <linux/circ_buf.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/atomic.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <linux/rtnetlink.h>
#endif


u32 read_macreg(_adapter *padapter, u32 addr, u32 sz)
{
	u32 val = 0;

	switch(sz)
	{
		case 1:
			val = rtw_read8(padapter, addr);
			break;
		case 2:
			val = rtw_read16(padapter, addr);
			break;
		case 4:
			val = rtw_read32(padapter, addr);
			break;
		default:
			val = 0xffffffff;
			break;
	}

	return val;
	
}

void write_macreg(_adapter *padapter, u32 addr, u32 val, u32 sz)
{
	switch(sz)
	{
		case 1:
			rtw_write8(padapter, addr, (u8)val);
			break;
		case 2:
			rtw_write16(padapter, addr, (u16)val);
			break;
		case 4:
			rtw_write32(padapter, addr, val);
			break;
		default:
			break;
	}

}

u32 read_bbreg(_adapter *padapter, u32 addr, u32 bitmask)
{
	return PHY_QueryBBReg(padapter, addr, bitmask);
}

void write_bbreg(_adapter *padapter, u32 addr, u32 bitmask, u32 val)
{
	PHY_SetBBReg(padapter, addr, bitmask, val);
}

u32 read_rfreg(_adapter *padapter, u8 rfpath, u32 addr, u32 bitmask)
{
	return PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)rfpath, addr, bitmask);
}

void write_rfreg(_adapter *padapter, u8 rfpath, u32 addr, u32 bitmask, u32 val)
{
	PHY_SetRFReg(padapter, (RF90_RADIO_PATH_E)rfpath, addr, bitmask, val);	
}

u8 rtl8192c_NULL_hdl(_adapter *padapter, u8 *pbuf)
{
	return H2C_SUCCESS;
}

u8 rtl8192c_setopmode_hdl(_adapter *padapter, u8 *pbuf)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct setopmode_parm *psetop = (struct setopmode_parm *)pbuf;

	rtw_write8(padapter, REG_BCN_MAX_ERR, 0xff);
	
	if(psetop->mode == Ndis802_11APMode)
	{
		pmlmeinfo->state = WIFI_FW_AP_STATE;
		rtw_write8(padapter, REG_BCN_CTRL, 0x12);
		Set_NETYPE0_MSR(padapter, _HW_STATE_AP_);	
		ResumeTxBeacon(padapter);
	}
	else if(psetop->mode == Ndis802_11Infrastructure)
	{
		rtw_write8(padapter, REG_BCN_CTRL, 0x18);
		Set_NETYPE0_MSR(padapter, _HW_STATE_STATION_);	
		StopTxBeacon(padapter);
	}
	else if(psetop->mode == Ndis802_11IBSS)
	{
		rtw_write8(padapter, REG_BCN_CTRL, 0x1a);
		Set_NETYPE0_MSR(padapter, _HW_STATE_ADHOC_);	
		ResumeTxBeacon(padapter);
	}
	else
	{
		rtw_write8(padapter, REG_BCN_CTRL, 0x18);
		Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);	
		StopTxBeacon(padapter);
	}

	return H2C_SUCCESS;
	
}

u8 rtl8192c_createbss_hdl(_adapter *padapter, u8 *pbuf)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX	*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
	struct joinbss_parm *pparm = (struct joinbss_parm *)pbuf;

	
	if(pparm->network.InfrastructureMode == Ndis802_11APMode)
	{
#ifdef CONFIG_AP_MODE
	
		if(pmlmeinfo->state == WIFI_FW_AP_STATE)
		{		
			//todo:
			return H2C_SUCCESS;		
		}		
#endif
	}


	//below is for ad-hoc master
	if(pparm->network.InfrastructureMode == Ndis802_11IBSS)
	{
		rtw_joinbss_reset(padapter);

		pmlmeext->linked_to = 0;
	
		pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
		pmlmeext->cur_ch_offset= HAL_PRIME_CHNL_OFFSET_DONT_CARE;	
		pmlmeinfo->ERP_enable = 0;
		pmlmeinfo->WMM_enable = 0;
		pmlmeinfo->HT_enable = 0;
		pmlmeinfo->HT_caps_enable = 0;
		pmlmeinfo->HT_info_enable = 0;
		pmlmeinfo->agg_enable_bitmap = 0;
		pmlmeinfo->candidate_tid_bitmap = 0;

		//disable dynamic functions, such as high power, DIG
		Save_DM_Func_Flag(padapter);
		Switch_DM_Func(padapter, DYNAMIC_FUNC_DISABLE, _FALSE);

		//config the initial gain under linking, need to write the BB registers
		write_bbreg(padapter, rOFDM0_XAAGCCore1, bMaskByte0, 0x30);
		write_bbreg(padapter, rOFDM0_XBAGCCore1, bMaskByte0, 0x30);

		//cancel link timer 
		_cancel_timer_ex(&pmlmeext->link_timer);

                //clear CAM
		flush_all_cam_entry(padapter);	


		_rtw_memcpy(pnetwork, pbuf, FIELD_OFFSET(WLAN_BSSID_EX, IELength)); 
		pnetwork->IELength = ((WLAN_BSSID_EX *)pbuf)->IELength;
	
		if(pnetwork->IELength>MAX_IE_SZ)//Check pbuf->IELength
			return H2C_PARAMETERS_ERROR;	
		
		_rtw_memcpy(pnetwork->IEs, ((WLAN_BSSID_EX *)pbuf)->IEs, pnetwork->IELength); 
	
	
		start_create_ibss(padapter);
		
	}	

	return H2C_SUCCESS;

}

u8 rtl8192c_join_cmd_hdl(_adapter *padapter, u8 *pbuf)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX	*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
	struct joinbss_parm *pparm = (struct joinbss_parm *)pbuf;
	
	//check already connecting to AP or not
	if (pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS)
	{
		if (pmlmeinfo->state & WIFI_FW_STATION_STATE)
		{
			issue_deauth(padapter, pnetwork->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
		}

		pmlmeinfo->state = WIFI_FW_NULL_STATE;
		
		//clear CAM
		flush_all_cam_entry(padapter);		
		
		_cancel_timer_ex(&pmlmeext->link_timer);
		
		//set MSR to nolink		
		Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);	

		//Set RCR to not to receive data frame when NO LINK state
		//rtw_write32(padapter, REG_RCR, rtw_read32(padapter, REG_RCR) & ~RCR_ADF);
		// reject all data frame
		rtw_write16(padapter, REG_RXFLTMAP2,0x00);
		
		//reset TSF
		rtw_write8(padapter, REG_DUAL_TSF_RST, (BIT(0)|BIT(1)));

		//disable update TSF
		if(IS_NORMAL_CHIP(pHalData->VersionID))
		{
			rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(4));	
		}
		else
		{
			//rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)&(~(BIT(4)|BIT(5))));	
			rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(4)|BIT(5));				
		}

	}

	#ifdef CONFIG_ANTENNA_DIVERSITY
	//switch antenna to Optimum_antenna
	printk("rtl8192c_join_cmd_hdl cur_ant(%d),opt_ant(%d)\n",pHalData->CurAntenna,pparm->network.PhyInfo.Optimum_antenna);
	if(pHalData->CurAntenna !=  pparm->network.PhyInfo.Optimum_antenna)		
	{						
		//PHY_SetRFPath(adapter,pnetwork->network.PhyInfo.Optimum_antenna);
		antenna_select_cmd(padapter, pparm->network.PhyInfo.Optimum_antenna, 0);
		printk("#### Change to Optimum_antenna(%s)\n",(2==pparm->network.PhyInfo.Optimum_antenna)?"A":"B");
	}
	#endif

	rtw_joinbss_reset(padapter);

	pmlmeext->linked_to = 0;
	
	pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
	pmlmeext->cur_ch_offset= HAL_PRIME_CHNL_OFFSET_DONT_CARE;	
	pmlmeinfo->ERP_enable = 0;
	pmlmeinfo->WMM_enable = 0;
	pmlmeinfo->HT_enable = 0;
	pmlmeinfo->HT_caps_enable = 0;
	pmlmeinfo->HT_info_enable = 0;
	pmlmeinfo->agg_enable_bitmap = 0;
	pmlmeinfo->candidate_tid_bitmap = 0;
	
	//pmlmeinfo->assoc_AP_vendor = maxAP;
	
	if (padapter->registrypriv.wifi_spec) {
		// for WiFi test, follow WMM test plan spec
		rtw_write32(padapter, REG_EDCA_VO_PARAM, 0x002F431C);
		rtw_write32(padapter, REG_EDCA_VI_PARAM, 0x005E541C);
		rtw_write32(padapter, REG_EDCA_BE_PARAM, 0x0000A525);
		rtw_write32(padapter, REG_EDCA_BK_PARAM, 0x0000A549);
	
                // for WiFi test, mixed mode with intel STA under bg mode throughput issue
	        if (padapter->mlmepriv.htpriv.ht_option == 0)
		     rtw_write32(padapter, REG_EDCA_BE_PARAM, 0x00004320);

	} else {
	        rtw_write32(padapter, REG_EDCA_VO_PARAM, 0x002F3217);
	        rtw_write32(padapter, REG_EDCA_VI_PARAM, 0x005E4317);
	        rtw_write32(padapter, REG_EDCA_BE_PARAM, 0x00105320);
	        rtw_write32(padapter, REG_EDCA_BK_PARAM, 0x0000A444);
	}
	
	//disable dynamic functions, such as high power, DIG
	//Switch_DM_Func(padapter, DYNAMIC_FUNC_DISABLE, _FALSE);

	//config the initial gain under linking, need to write the BB registers
	write_bbreg(padapter, rOFDM0_XAAGCCore1, 0x7f, 0x32);
	write_bbreg(padapter, rOFDM0_XBAGCCore1, 0x7f, 0x32);

	//set MSR to nolink		
	Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);		
			
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		//config RCR to receive different BSSID & not to receive data frame during linking				
		u32 v = rtw_read32(padapter, REG_RCR);
		v &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN );//| RCR_ADF
		rtw_write32(padapter, REG_RCR, v);
		rtw_write16(padapter, REG_RXFLTMAP2,0x00);
	}	
	else
	{
		//config RCR to receive different BSSID & not to receive data frame during linking	
		rtw_write32(padapter, REG_RCR, rtw_read32(padapter, REG_RCR) & 0xfffff7bf);
	}

	
	//cancel link timer 
	_cancel_timer_ex(&pmlmeext->link_timer);


	_rtw_memcpy(pnetwork, pbuf, FIELD_OFFSET(WLAN_BSSID_EX, IELength)); 
	pnetwork->IELength = ((WLAN_BSSID_EX *)pbuf)->IELength;
	
	if(pnetwork->IELength>MAX_IE_SZ)//Check pbuf->IELength
		return H2C_PARAMETERS_ERROR;	
		
	_rtw_memcpy(pnetwork->IEs, ((WLAN_BSSID_EX *)pbuf)->IEs, pnetwork->IELength); 
	
	start_clnt_join(padapter);
	
	//only for cisco's AP
	if(pmlmeinfo->assoc_AP_vendor == ciscoAP)				
	{	
		int ie_len;
		struct registry_priv	 *pregpriv = &padapter->registrypriv;
		u8 *p = rtw_get_ie((pmlmeinfo->network.IEs + sizeof(NDIS_802_11_FIXED_IEs)), _HT_ADD_INFO_IE_, &ie_len, (pmlmeinfo->network.IELength - sizeof(NDIS_802_11_FIXED_IEs)));
		if( p && ie_len)
		{
			struct HT_info_element *pht_info = (struct HT_info_element *)(p+2);
					
			if ((pregpriv->cbw40_enable) &&	 (pht_info->infos[0] & BIT(2)))
			{
				//switch to the 40M Hz mode according to the AP
				pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_40;
				switch (pht_info->infos[0] & 0x3)
				{
					case 1:
						pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_LOWER;
						break;
			
					case 3:
						pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_UPPER;
						break;
				
					default:
						pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
						break;
				}		
						
				DBG_871X("set ch/bw for cisco's ap before connected\n");
						
				set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
						
			}
					
		}
		
	}
	
	pmlmeinfo->assoc_AP_vendor = maxAP;
	
	return H2C_SUCCESS;
	
}

u8 rtl8192c_disconnect_hdl(_adapter *padapter, unsigned char *pbuf)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct setauth_parm		*pparm = (struct setauth_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*pnetwork = (WLAN_BSSID_EX*)(&(pmlmeinfo->network));
	struct	mlme_priv 	*pmlmepriv = &padapter->mlmepriv;	
	
	if (is_client_associated_to_ap(padapter))
	{
		issue_deauth(padapter, pnetwork->MacAddress, WLAN_REASON_DEAUTH_LEAVING);
	}

	//set_opmode_cmd(padapter, infra_client_with_mlme);

	pmlmeinfo->state = WIFI_FW_NULL_STATE;
	
	//switch to the 20M Hz mode after disconnect
	pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_20;
	pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
	
		
	//set MSR to no link state
	Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);		
		
	//Set RCR to not to receive data frame when NO LINK state
	//rtw_write32(padapter, REG_RCR, rtw_read32(padapter, REG_RCR) & ~RCR_ADF);
	rtw_write16(padapter, REG_RXFLTMAP2,0x00);
	
	//reset TSF
	rtw_write8(padapter, REG_DUAL_TSF_RST, (BIT(0)|BIT(1)));

	//disable update TSF
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(4));	
	}
	else
	{
		rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(4)|BIT(5));
	}
	
	if(((pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE) || ((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE))
	{
		//Stop BCN		
		rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)&(~(EN_BCN_FUNCTION | EN_TXBCN_RPT)));
	}

	pmlmeinfo->state = WIFI_FW_NULL_STATE;

	pmlmepriv->sitesurveyctrl.traffic_busy = _FALSE;		
	set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);

	flush_all_cam_entry(padapter);
		
	_cancel_timer_ex(&pmlmeext->link_timer);
	pmlmeext->linked_to = 0;

	
	return 	H2C_SUCCESS;
}

u8 rtl8192c_sitesurvey_cmd_hdl(_adapter *padapter, u8 *pbuf)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct sitesurvey_parm	*pparm = (struct sitesurvey_parm *)pbuf;
		
	if (pmlmeext->sitesurvey_res.state == _FALSE)
	{
		//for first time rtw_sitesurvey_cmd
		pmlmeext->sitesurvey_res.state = _TRUE;
		pmlmeext->sitesurvey_res.bss_cnt = 0;
		pmlmeext->sitesurvey_res.channel_idx = 0;
		
		if (le32_to_cpu(pparm->ss_ssidlen))
		{
			_rtw_memcpy(pmlmeext->sitesurvey_res.ss_ssid, pparm->ss_ssid, le32_to_cpu(pparm->ss_ssidlen));
		}	
		else
		{
			_rtw_memset(pmlmeext->sitesurvey_res.ss_ssid, 0, (IW_ESSID_MAX_SIZE + 1));
		}	
		
		pmlmeext->sitesurvey_res.ss_ssidlen = le32_to_cpu(pparm->ss_ssidlen);
	
		pmlmeext->sitesurvey_res.active_mode = le32_to_cpu(pparm->passive_mode);		

		//disable dynamic functions, such as high power, DIG
		Save_DM_Func_Flag(padapter);
		Switch_DM_Func(padapter, DYNAMIC_FUNC_DISABLE, _FALSE);

		//config the initial gain under scaning, need to write the BB registers
		write_bbreg(padapter, rOFDM0_XAAGCCore1, 0x7f, 0x20);
		write_bbreg(padapter, rOFDM0_XBAGCCore1, 0x7f, 0x20);
		
		
		//set MSR to no link state
		Set_NETYPE0_MSR(padapter, _HW_STATE_NOLINK_);		
		
		
		if(IS_NORMAL_CHIP(pHalData->VersionID))
		{
			//config RCR to receive different BSSID & not to receive data frame
			//pHalData->ReceiveConfig &= (~(RCR_CBSSID_DATA | RCR_CBSSID_BCN));			
			u32 v = rtw_read32(padapter, REG_RCR);
			v &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN );//| RCR_ADF
			rtw_write32(padapter, REG_RCR, v);
			rtw_write16(padapter, REG_RXFLTMAP2,0x00);

			//disable update TSF
			rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(4));
		}	
		else
		{
			//config RCR to receive different BSSID & not to receive data frame			
			rtw_write32(padapter, REG_RCR, rtw_read32(padapter, REG_RCR) & 0xfffff7bf);

			//disable update TSF
			rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(4)|BIT(5));
		}

		//issue null data if associating to the AP
		if (is_client_associated_to_ap(padapter) == _TRUE)
		{
			issue_nulldata(padapter, 1);
		}
		
	}
		
	site_survey(padapter);

	return H2C_SUCCESS;
	
}

u8 rtl8192c_setauth_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct setauth_parm		*pparm = (struct setauth_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	if (pparm->mode < 4)
	{
		pmlmeinfo->auth_algo = pparm->mode;
	}

	return 	H2C_SUCCESS;
}

#define CAM_CFG_VALID BIT15
u8 rtl8192c_setkey_hdl(_adapter *padapter, u8 *pbuf)
{
	unsigned short				ctrl;
	struct setkey_parm		*pparm = (struct setkey_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	unsigned char					null_sta[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	pmlmeinfo->key_index = pparm->keyid;
	
	//write cam
	ctrl = CAM_CFG_VALID | ((pparm->algorithm) << 2) | pparm->keyid;	
	
	write_cam(padapter, pparm->keyid, ctrl, null_sta, pparm->key);
	
	return H2C_SUCCESS;
}

u8 rtl8192c_set_stakey_hdl(_adapter *padapter, u8 *pbuf)
{
	unsigned short ctrl=0;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct set_stakey_parm	*pparm = (struct set_stakey_parm *)pbuf;
	
	if((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
	{
		unsigned char cam_id;//cam_entry
		struct sta_info *psta;
		struct sta_priv *pstapriv = &padapter->stapriv;
		
		psta = rtw_get_stainfo(pstapriv, pparm->addr);
		if(psta)
		{			
			ctrl = (BIT(15) | ((pparm->algorithm) << 2));

			DBG_8192C("r871x_set_stakey_hdl(): enc_algorithm=%d\n", pparm->algorithm);

			if((psta->mac_id<1) || (psta->mac_id>(NUM_STA-4)))
			{
				DBG_8192C("r871x_set_stakey_hdl():set_stakey failed, mac_id(aid)=%d\n", psta->mac_id);
				return H2C_REJECTED;
			}	
				 
			cam_id = (psta->mac_id + 3);//0~3 for default key, cmd_id=aid/macid + 3

			DBG_8192C("Write CAM, mac_addr=%x:%x:%x:%x:%x:%x, cam_entry=%d\n", pparm->addr[0], 
						pparm->addr[1], pparm->addr[2], pparm->addr[3], pparm->addr[4],
						pparm->addr[5], cam_id);

			write_cam(padapter, cam_id, ctrl, pparm->addr, pparm->key);
	
			return H2C_SUCCESS_RSP;
		
		}
		else
		{
			DBG_8192C("r871x_set_stakey_hdl(): sta has been free\n");
			return H2C_REJECTED;
		}
		
	}

	//below for sta mode
	
	ctrl = BIT(15) | ((pparm->algorithm) << 2);	

	write_cam(padapter, 4, ctrl, pparm->addr, pparm->key);//CAM_ID(CAM_ENTRY)=4	

	pmlmeinfo->enc_algo = pparm->algorithm;
	
	return H2C_SUCCESS;
}

u8 rtl8192c_add_ba_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct addBaReq_parm 	*pparm = (struct addBaReq_parm *)pbuf;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	struct sta_info *psta = rtw_get_stainfo(&padapter->stapriv, pparm->addr);
	
	if(!psta)
		return 	H2C_SUCCESS;
		

	if (((pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS) && (pmlmeinfo->HT_enable)) ||
		((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE))
	{		
		//pmlmeinfo->candidate_tid_bitmap |= (0x1 << pparm->tid);		
		//psta->htpriv.candidate_tid_bitmap |= BIT(pparm->tid);
		issue_action_BA(padapter, pparm->addr, WLAN_ACTION_ADDBA_REQ, (u16)pparm->tid);		
		//_set_timer(&pmlmeext->ADDBA_timer, ADDBA_TO);
		_set_timer(&psta->addba_retry_timer, ADDBA_TO);
	}
	else
	{		
		psta->htpriv.candidate_tid_bitmap &= ~BIT(pparm->tid);		
	}
	
	return 	H2C_SUCCESS;
	
}

u8 set_tx_beacon_cmd(_adapter* padapter)
{
	struct cmd_obj*		ph2c;
	struct Tx_Beacon_param 	*ptxBeacon_parm;	
	struct cmd_priv	*pcmdpriv = &(padapter->cmdpriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u8	res = _SUCCESS;
	
_func_enter_;	
	
	if ((ph2c = (struct cmd_obj*)_rtw_malloc(sizeof(struct cmd_obj))) == NULL)
	{
		res= _FAIL;
		goto exit;
	}
	
	if ((ptxBeacon_parm = (struct Tx_Beacon_param *)_rtw_malloc(sizeof(struct Tx_Beacon_param))) == NULL)
	{
		_rtw_mfree((unsigned char *)ph2c, sizeof(struct	cmd_obj));
		res= _FAIL;
		goto exit;
	}

	_rtw_memcpy(&(ptxBeacon_parm->network), &(pmlmeinfo->network), sizeof(WLAN_BSSID_EX));
	init_h2fwcmd_w_parm_no_rsp(ph2c, ptxBeacon_parm, GEN_CMD_CODE(_TX_Beacon));

	rtw_enqueue_cmd_ex(pcmdpriv, ph2c);

	
exit:
	
_func_exit_;

	return res;
}


thread_return rtw_cmd_thread(thread_context context)
{
	u8 ret;
	struct cmd_obj *pcmd;
	u8 *pcmdbuf, *prspbuf;
	u8 (*cmd_hdl)(_adapter *padapter, u8* pbuf);
	void (*pcmd_callback)(_adapter *dev, struct cmd_obj *pcmd);
       _adapter *padapter = (_adapter *)context;
	struct cmd_priv *pcmdpriv = &(padapter->cmdpriv);
	
_func_enter_;

	thread_enter(padapter);

	pcmdbuf = pcmdpriv->cmd_buf;
	prspbuf = pcmdpriv->rsp_buf;

	RT_TRACE(_module_rtl871x_cmd_c_,_drv_info_,("start r871x rtw_cmd_thread !!!!\n"));

	while(1)
	{
		if ((_rtw_down_sema(&(pcmdpriv->cmd_queue_sema))) == _FAIL)
			break;

		if (rtw_register_cmd_alive(padapter) != _SUCCESS)
		{
			continue;
		}
		
_next:

                if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{			
			printk("###> rtw_cmd_thread break.................\n");
			RT_TRACE(_module_rtl871x_cmd_c_, _drv_info_, ("rtw_cmd_thread:bDriverStopped(%d) OR bSurpriseRemoved(%d)", padapter->bDriverStopped, padapter->bSurpriseRemoved));		
			break;
		}
	
		if(!(pcmd = rtw_dequeue_cmd(&(pcmdpriv->cmd_queue)))) {
			rtw_unregister_cmd_alive(padapter);
			continue;
		}

		pcmdpriv->cmd_issued_cnt++;

		pcmd->cmdsz = _RND4((pcmd->cmdsz));//_RND4

		_rtw_memcpy(pcmdbuf, pcmd->parmbuf, pcmd->cmdsz);

		if(pcmd->cmdcode <= (sizeof(wlancmds) /sizeof(struct cmd_hdl)))
		{
			cmd_hdl = wlancmds[pcmd->cmdcode].h2cfuns;

			if (cmd_hdl)
			{			
				ret = cmd_hdl(padapter, pcmdbuf);
				pcmd->res = ret;
			}

			//invoke cmd->callback function		
			pcmd_callback = rtw_cmd_callback[pcmd->cmdcode].callback;
			if(pcmd_callback == NULL)
			{
				RT_TRACE(_module_rtl871x_cmd_c_,_drv_info_,("mlme_cmd_hdl(): pcmd_callback=0x%p, cmdcode=0x%x\n", pcmd_callback, pcmd->cmdcode));
				rtw_free_cmd_obj(pcmd);
			}	
			else
			{	
				//todo: !!! fill rsp_buf to pcmd->rsp if (pcmd->rsp!=NULL)
				
				pcmd_callback(padapter, pcmd);//need conider that free cmd_obj in rtw_cmd_callback
			}

			pcmdpriv->cmd_seq++;
			
		}
		
		cmd_hdl = NULL;

		flush_signals_thread();
		
		goto _next;
				
	}
	

	// free all cmd_obj resources
	do{

		pcmd = rtw_dequeue_cmd(&(pcmdpriv->cmd_queue));
		if(pcmd==NULL)
			break;

		rtw_free_cmd_obj(pcmd);
		
	}while(1);


	_rtw_up_sema(&pcmdpriv->terminate_cmdthread_sema);

_func_exit_;	

	thread_exit();

}

u8 rtl8192c_mlme_evt_hdl(_adapter *padapter, unsigned char *pbuf)
{
	u8 evt_code, evt_seq;
	u16 evt_sz;
	uint 	*peventbuf;
	void (*event_callback)(_adapter *dev, u8 *pbuf);
	struct evt_priv *pevt_priv = &(padapter->evtpriv);

	peventbuf = (uint*)pbuf;
	evt_sz = (u16)(*peventbuf&0xffff);
	evt_seq = (u8)((*peventbuf>>24)&0x7f);
	evt_code = (u8)((*peventbuf>>16)&0xff);
	
		
	// checking event sequence...		
	if ((evt_seq & 0x7f) != pevt_priv->event_seq)
	{
		RT_TRACE(_module_rtl871x_cmd_c_,_drv_info_,("Evetn Seq Error! %d vs %d\n", (evt_seq & 0x7f), pevt_priv->event_seq));
	
		pevt_priv->event_seq = (evt_seq+1)&0x7f;

		goto _abort_event_;
	}

	// checking if event code is valid
	if (evt_code >= MAX_C2HEVT)
	{
		RT_TRACE(_module_rtl871x_cmd_c_,_drv_err_,("\nEvent Code(%d) mismatch!\n", evt_code));
		goto _abort_event_;
	}

	// checking if event size match the event parm size	
	if ((rtw_wlanevents[evt_code].parmsize != 0) && 
			(rtw_wlanevents[evt_code].parmsize != evt_sz))
	{
			
		RT_TRACE(_module_rtl871x_cmd_c_,_drv_err_,("\nEvent(%d) Parm Size mismatch (%d vs %d)!\n", 
			evt_code, rtw_wlanevents[evt_code].parmsize, evt_sz));
		goto _abort_event_;	
			
	}

	pevt_priv->event_seq = (pevt_priv->event_seq+1)&0x7f;//update evt_seq

	peventbuf += 2;
				
	if(peventbuf)
	{
		event_callback = rtw_wlanevents[evt_code].event_callback;
		event_callback(padapter, (u8*)peventbuf);

		pevt_priv->evt_done_cnt++;
	}


_abort_event_:


	return H2C_SUCCESS;
		
}

void dynamic_chk_wk_hdl(_adapter *padapter, u8 *pbuf, int sz)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);


	/*
	 * Commented by Jeff 2010/12/25
	 * Long-time site survey will occpuy the medium causing no beacon
	 * or probe respons is received form current AP.
	 * If under site surveying, we don't decrese the "linked_to" counter
	 * to prevent "linked_status_chk" 's over killing.
	*/
	if(check_fwstate(pmlmepriv, WIFI_SITE_MONITOR) == _FALSE)
	{
		if(pmlmeext->linked_to > 0)
		{
			pmlmeext->linked_to--;	
			if(pmlmeext->linked_to==0)
			    linked_status_chk(padapter);		
		}
	}

	if(pHalData->hal_ops.hal_dm_watchdog)
		pHalData->hal_ops.hal_dm_watchdog(padapter);

	//check_hw_pbc(padapter, pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);	
}

u8 rtl8192c_drvextra_cmd_hdl(_adapter *padapter, unsigned char *pbuf)
{
	struct drvextra_cmd_parm *pdrvextra_cmd;

	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	pdrvextra_cmd = (struct drvextra_cmd_parm*)pbuf;
	
	switch(pdrvextra_cmd->ec_id)
	{
		case DYNAMIC_CHK_WK_CID:
			dynamic_chk_wk_hdl(padapter, pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);
			break;
		case DM_CTRL_WK_CID:
			//HalDmWatchDog(padapter);
			break;
		case PBC_POLLING_WK_CID:
			//check_hw_pbc(padapter, pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);			
			break;
		case BEFORE_ASSOC_PS_CTRL_WK_CID:
			before_assoc_ps_ctrl_wk_hdl(padapter, pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);	
			break;
#ifdef CONFIG_LPS
		case LPS_CTRL_WK_CID:
			lps_ctrl_wk_hdl(padapter, pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);
			break;
#endif
#ifdef CONFIG_ANTENNA_DIVERSITY
		case ANT_SELECT_WK_CID:
			antenna_select_wk_hdl(padapter, pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);
			break;
#endif
		default:
			break;

	}


	if(pdrvextra_cmd->pbuf && pdrvextra_cmd->sz>0)
	{
		_rtw_mfree(pdrvextra_cmd->pbuf, pdrvextra_cmd->sz);
	}


	return H2C_SUCCESS;

}
#if 0
static BOOLEAN
CheckWriteMSG(
	IN	PADAPTER		Adapter,
	IN	u8		BoxNum
)
{
	u8	valHMETFR;
	BOOLEAN	Result = _FALSE;
	
	valHMETFR = rtw_read8(Adapter, REG_HMETFR);

	//DbgPrint("CheckWriteH2C(): Reg[0x%2x] = %x\n",REG_HMETFR, valHMETFR);

	if(((valHMETFR>>BoxNum)&BIT0) == 1)
		Result = _TRUE;
	
	return Result;

}

static BOOLEAN CheckFwReadLastMSG(
	IN	PADAPTER		Adapter,
	IN	u8		BoxNum
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	valHMETFR, valMCUTST_1;
	BOOLEAN	 Result = _FALSE;
	
	valHMETFR = rtw_read8(Adapter, REG_HMETFR);
	valMCUTST_1 = rtw_read8(Adapter, (REG_MCUTST_1+BoxNum));

	//DbgPrint("REG[%x] = %x, REG[%x] = %x\n", 
	//	REG_HMETFR, valHMETFR, REG_MCUTST_1+BoxNum, valMCUTST_1 );

	// Do not seperate to 91C and 88C, we use the same setting. Suggested by SD4 Filen. 2009.12.03.
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		if(((valHMETFR>>BoxNum)&BIT0) == 0)
			Result = _TRUE;
	}
	else
	{
		if((((valHMETFR>>BoxNum)&BIT0) == 0) && (valMCUTST_1 == 0))
		{
			Result = _TRUE;
		}
	}
	
	return Result;
}
#endif

#define RTL92C_MAX_H2C_BOX_NUMS	4
#define RTL92C_MAX_CMD_LEN	5
#define MESSAGE_BOX_SIZE		4
#define EX_MESSAGE_BOX_SIZE	2


static u8 _is_fw_read_cmd_down(_adapter* padapter, u8 isvern, u8 msgbox_num)
{
	u8 read_down = _FALSE;
	int  retry_cnts = 100;
	
	u8 valid;

//	DBG_8192C(" _is_fw_read_cmd_down ,isnormal_chip(%x),reg_1cc(%x),msg_box(%d)...\n",isvern,rtw_read8(padapter,REG_HMETFR),msgbox_num);
	
	do{
		valid = rtw_read8(padapter,REG_HMETFR) & BIT(msgbox_num);	
		if(isvern){
			if(0 == valid ){
				read_down = _TRUE;
			}			
		}
		else{
			if((0 == valid) && (0 == rtw_read8(padapter, REG_MCUTST_1+msgbox_num))){
				read_down = _TRUE;	
			}
		}
	}while( (!read_down) && (retry_cnts--));

	return read_down;
	
}

/*****************************************
* H2C Msg format :
*| 31 - 8		|7		| 6 - 0	|	
*| h2c_msg 	|Ext_bit	|CMD_ID	|
*
******************************************/
void FillH2CCmd(_adapter* padapter, u8 ElementID, u32 CmdLen, u8* pCmdBuffer)
{	
#if 1
	u8 bcmd_down = _FALSE;
	int 	retry_cnts = 100;
	u8	h2c_box_num;
	u32	msgbox_addr;
	u32  msgbox_ex_addr;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8 isnchip =IS_NORMAL_CHIP(pHalData->VersionID);
	u32	h2c_cmd = 0;
	u16	h2c_cmd_ex = 0;

	_func_enter_;	
	
	if(!pCmdBuffer){
		return ;
	}
	if(CmdLen > RTL92C_MAX_CMD_LEN){
		return ;
	}
	if(padapter->bFWReady == _FALSE)		
	{
		return;
	}
	//pay attention to if  race condition happened in  H2C cmd setting.
	do{
		h2c_box_num = pHalData->LastHMEBoxNum;

		if(!_is_fw_read_cmd_down(padapter, isnchip, h2c_box_num)){
			DBG_8192C(" fw read cmd failed...\n");
			break;
		}		
				
		if(CmdLen<=3)
		{
			_rtw_memcpy((u8*)(&h2c_cmd)+1, pCmdBuffer, CmdLen );
		}
		else{
			_rtw_memcpy((u8*)(&h2c_cmd_ex), pCmdBuffer, EX_MESSAGE_BOX_SIZE);
			_rtw_memcpy((u8*)(&h2c_cmd)+1, pCmdBuffer+2,( CmdLen-EX_MESSAGE_BOX_SIZE));
			*(u8*)(&h2c_cmd) |= BIT(7);
		}

		*(u8*)(&h2c_cmd) |= ElementID;
			
		if(h2c_cmd & BIT(7)){
			msgbox_ex_addr = REG_HMEBOX_EXT_0 + (h2c_box_num *EX_MESSAGE_BOX_SIZE);
			h2c_cmd_ex = cpu_to_le16( h2c_cmd_ex );
			rtw_write16(padapter, msgbox_ex_addr, h2c_cmd_ex);
		}
		msgbox_addr =REG_HMEBOX_0 + (h2c_box_num *MESSAGE_BOX_SIZE);
		h2c_cmd = cpu_to_le32( h2c_cmd );
		rtw_write32(padapter,msgbox_addr, h2c_cmd);
		
		if(!isnchip){//for Test chip
			if(! (rtw_read8(padapter, REG_HMETFR) & BIT(h2c_box_num))){
				DBG_8192C("Chip test  - check fw write failed, write again..\n");
				continue;
			}			
			// Fill H2C protection register.
			rtw_write8(padapter,REG_MCUTST_1+h2c_box_num, 0xFF);			
		}
		bcmd_down = _TRUE;

	//	DBG_8192C("MSG_BOX:%d,CmdLen(%d), reg:0x%x =>h2c_cmd:0x%x, reg:0x%x =>h2c_cmd_ex:0x%x ..\n"
	//	 	,pHalData->LastHMEBoxNum ,CmdLen,msgbox_addr,h2c_cmd,msgbox_ex_addr,h2c_cmd_ex);
		
		 pHalData->LastHMEBoxNum = (h2c_box_num+1) % RTL92C_MAX_H2C_BOX_NUMS ;
		 
	}while((!bcmd_down) && (retry_cnts--));
/*
	if(bcmd_down)
		DBG_8192C("H2C Cmd exe down. \n"	);	
	else
		DBG_8192C("H2C Cmd exe failed. \n"	);
*/	
	_func_exit_;

	
#else
	u8	BoxNum;
	u16	BOXReg, BOXExtReg;
	u8	BoxContent[4], BoxExtContent[2];
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8 	BufIndex=0;
	u8	bWriteSucess = _FALSE;
	u8	IsFwRead = _FALSE;
	u8	WaitH2cLimmit = 100;

	u32	h2c_cmd = 0;
	u16	h2c_cmd_ex = 0;

_func_enter_;	

	//printk("FillH2CCmd : ElementID=%d \n",ElementID);

	while(!bWriteSucess)
	{
		// 2. Find the last BOX number which has been writen.
		BoxNum = pHalData->LastHMEBoxNum;
		switch(BoxNum)
		{
			case 0:
				BOXReg = REG_HMEBOX_0;
				BOXExtReg = REG_HMEBOX_EXT_0;
				break;
			case 1:
				BOXReg = REG_HMEBOX_1;
				BOXExtReg = REG_HMEBOX_EXT_1;
				break;
			case 2:
				BOXReg = REG_HMEBOX_2;
				BOXExtReg = REG_HMEBOX_EXT_2;
				break;
			case 3:
				BOXReg = REG_HMEBOX_3;
				BOXExtReg = REG_HMEBOX_EXT_3;
				break;
			default:
				break;
		}

		// 3. Check if the box content is empty.
		IsFwRead = CheckFwReadLastMSG(padapter, BoxNum);
		while(!IsFwRead)
		{
			//wait until Fw read
			WaitH2cLimmit--;
			if(WaitH2cLimmit == 0)
			{
				DBG_8192C("FillH2CCmd92C(): Wating too long for FW read clear HMEBox(%d)!!!\n", BoxNum);
				break;
			}
			rtw_msleep_os(10); //us
			IsFwRead = CheckFwReadLastMSG(padapter, BoxNum);
			//U1btmp = PlatformEFIORead1Byte(Adapter, 0x1BF);
			//RT_TRACE(COMP_CMD, DBG_LOUD, ("FillH2CCmd92C(): Wating for FW read clear HMEBox(%d)!!! 0x1BF = %2x\n", BoxNum, U1btmp));
		}

		// If Fw has not read the last H2C cmd, break and give up this H2C.
		if(!IsFwRead)
		{
			DBG_8192C("FillH2CCmd92C():  Write H2C register BOX[%d] fail!!!!! Fw do not read. \n", BoxNum);
			break;
		}

		// 4. Fill the H2C cmd into box		
		_rtw_memset(BoxContent, 0, sizeof(BoxContent));
		_rtw_memset(BoxExtContent, 0, sizeof(BoxExtContent));
		
		BoxContent[0] = ElementID; // Fill element ID

		//DBG_8192C("FillH2CCmd92C():Write ElementID BOXReg(%4x) = %2x \n", BOXReg, ElementID);

		switch(CmdLen)
		{
			case 1:
				{
					BoxContent[0] &= ~(BIT7);
					_rtw_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 1);
					rtw_write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					break;
				}
			case 2:
				{	
					BoxContent[0] &= ~(BIT7);
					_rtw_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 2);
					rtw_write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					break;
				}
			case 3:
				{
					BoxContent[0] &= ~(BIT7);
					_rtw_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 3);
					rtw_write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					break;
				}
			case 4:
				{
					BoxContent[0] |= (BIT7);
					_rtw_memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
					_rtw_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 2);
					rtw_write16(padapter, BOXExtReg, *((u16*)BoxExtContent));
					rtw_write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					h2c_cmd_ex = *((u32*)BoxExtContent);
					break;
				}
			case 5:
				{
					BoxContent[0] |= (BIT7);
					_rtw_memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
					_rtw_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 3);
					rtw_write16(padapter, BOXExtReg, *((u16*)BoxExtContent));
					rtw_write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					h2c_cmd_ex = *((u32*)BoxExtContent);
					break;
				}
			default:
					break;
					
		}


		DBG_8192C("MSG_BOX:%d,CmdLen(%d), reg:0x%x =>h2c_cmd:0x%x, reg:0x%x =>h2c_cmd_ex:0x%x ..\n"
		 	,pHalData->LastHMEBoxNum ,CmdLen,BOXReg,h2c_cmd,BOXExtReg,h2c_cmd_ex);

		//DBG_8192C("FillH2CCmd(): BoxExtContent=0x%x\n", *(u16*)BoxExtContent);		
		//DBG_8192C("FillH2CCmd(): BoxContent=0x%x\n", *(u32*)BoxContent);
			
		if(IS_NORMAL_CHIP(pHalData->VersionID))
		{
			// 5. Normal chip does not need to check if the H2C cmd has be written successfully.
			bWriteSucess = _TRUE;
		}
		else
		{	
			// 5. Check if the H2C cmd has be written successfully.
			bWriteSucess = CheckWriteMSG(padapter, BoxNum);
			if(!bWriteSucess) //If not then write again.
				continue;
			
			//6. Fill H2C protection register.

			rtw_write8(padapter, REG_MCUTST_1+BoxNum, 0xFF);
			//RT_TRACE(COMP_CMD, DBG_LOUD, ("FillH2CCmd92C():Write Reg(%4x) = 0xFF \n", REG_MCUTST_1+BoxNum));
		}

		// Record the next BoxNum
		pHalData->LastHMEBoxNum = BoxNum+1;
		if(pHalData->LastHMEBoxNum == 4) // loop to 0
			pHalData->LastHMEBoxNum = 0;
		
		//DBG_8192C("FillH2CCmd92C():pHalData->LastHMEBoxNum  = %d\n", pHalData->LastHMEBoxNum);
		
	}

_func_exit_;

#endif

}

u8 rtl8192c_h2c_msg_hdl(_adapter *padapter, unsigned char *pbuf)
{	
	u8 ElementID, CmdLen;
	u8 *pCmdBuffer;
	struct cmd_msg_parm  *pcmdmsg;
	
	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	pcmdmsg = (struct cmd_msg_parm*)pbuf;
	ElementID = pcmdmsg->eid;
	CmdLen = pcmdmsg->sz;
	pCmdBuffer = pcmdmsg->buf;

	FillH2CCmd(padapter, ElementID, CmdLen, pCmdBuffer);

	return H2C_SUCCESS;
}
#ifdef CONFIG_AUTOSUSPEND
#ifdef SUPPORT_HW_RFOFF_DETECTED
u8 set_FWSelectSuspend_cmd(_adapter *padapter ,u8 bfwpoll, u16 period)
{
	u8	res=_SUCCESS;
	struct H2C_SS_RFOFF_PARAM param;
	printk("==>%s \n",__FUNCTION__);
	param.gpio_period = period;//Polling GPIO_11 period time
	param.ROFOn = (_TRUE == bfwpoll)?1:0;
	FillH2CCmd(padapter, SELECTIVE_SUSPEND_ROF_CMD, sizeof(param), (u8*)(&param));		
	return res;
}
#endif
#endif

u8 set_rssi_cmd(_adapter*padapter, u8 *param)
{	
	u8	res=_SUCCESS;	
_func_enter_;	

	*((u32*) param ) = cpu_to_le32( *((u32*) param ) );
	FillH2CCmd(padapter, RSSI_SETTING_EID, 3, param);
_func_exit_;
	return res;
}

u8 set_raid_cmd(_adapter*padapter, u32 mask, u8 arg)
{	
	u8	buf[5];
	u8	res=_SUCCESS;
	
_func_enter_;	
	
	_rtw_memset(buf, 0, 5);
	mask = cpu_to_le32( mask );
	_rtw_memcpy(buf, &mask, 4);
	buf[4]  = arg;

	FillH2CCmd(padapter, MACID_CONFIG_EID, 5, buf);
	
_func_exit_;

	return res;

}

#ifdef CONFIG_ANTENNA_DIVERSITY
void antenna_select_wk_hdl(_adapter *padapter, u8 *pbuf, int antenna)
{
	printk("==> %s  , Ant_(%s)\n",__FUNCTION__,(antenna==2)?"A":"B");
	PHY_SetRFPath(padapter,antenna);
}
u8 antenna_select_cmd(_adapter*padapter, u8 antenna, u8 enqueue)
{
	struct cmd_obj		*ph2c;
	struct drvextra_cmd_parm	*pdrvextra_cmd_parm;	
	struct cmd_priv	*pcmdpriv = &padapter->cmdpriv;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	
	u8	res = _SUCCESS;
_func_enter_;

	if(IS_92C_SERIAL(pHalData->VersionID) ||(pHalData->AntDivCfg==0))	
		return res;

	if(enqueue)
	{
		ph2c = (struct cmd_obj*)_rtw_malloc(sizeof(struct cmd_obj));	
		if(ph2c==NULL){
			res= _FAIL;
			goto exit;
		}
		
		pdrvextra_cmd_parm = (struct drvextra_cmd_parm*)_rtw_malloc(sizeof(struct drvextra_cmd_parm)); 
		if(pdrvextra_cmd_parm==NULL){
			_rtw_mfree((unsigned char *)ph2c, sizeof(struct cmd_obj));
			res= _FAIL;
			goto exit;
		}

		pdrvextra_cmd_parm->ec_id = ANT_SELECT_WK_CID;
		pdrvextra_cmd_parm->sz = antenna;
		pdrvextra_cmd_parm->pbuf = NULL;
		printk("==> %s  , enqueue CMD \n",__FUNCTION__);	
		init_h2fwcmd_w_parm_no_rsp(ph2c, pdrvextra_cmd_parm, GEN_CMD_CODE(_Set_Drv_Extra));

		rtw_enqueue_cmd_ex(pcmdpriv, ph2c);
	}
	else
	{
		antenna_select_wk_hdl(padapter, NULL,antenna );
	}
	
exit:
	
_func_exit_;

	return res;

}
#endif

#ifdef CONFIG_LPS
void lps_ctrl_wk_hdl(_adapter *padapter, u8 *pbuf, int sz)
{
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
_func_enter_;		
	if((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE)
		|| (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE))
	{
		return;
	}

	switch(sz)
	{
		case LPS_CTRL_SCAN:
			printk("LPS_CTRL_SCAN \n");
			LPS_Leave(padapter);

			while( !FWLPS_RF_ON(padapter) )
			{
				printk("FW still in PS mode \n");
				rtw_usleep_os(100);
			}
			break;
		case LPS_CTRL_JOINBSS:
			printk("LPS_CTRL_JOINBSS \n");
			LPS_Leave(padapter);
			break;
		case LPS_CTRL_CONNECT:
			printk("LPS_CTRL_CONNECT \n");
			// Reset LPS Setting
			padapter->pwrctrlpriv.LpsIdleCount = 0;

			set_FwJoinBssReport_cmd(padapter, 1);
			break;
		case LPS_CTRL_DISCONNECT:
			printk("LPS_CTRL_DISCONNECT \n");
			LPS_Leave(padapter);

			set_FwJoinBssReport_cmd(padapter, 0);
			break;
		case LPS_CTRL_SPECIAL_PACKET:
			printk("LPS_CTRL_SPECIAL_PACKET \n");
			pwrpriv->DelayLPSLastTimeStamp = rtw_get_current_time();
			LPS_Leave(padapter);
			break;

		default:
			break;
	}

_func_exit_;
}

u8 lps_ctrl_wk_cmd(_adapter*padapter, u8 lps_ctrl_type, u8 enqueue)
{
	struct cmd_obj	*ph2c;
	struct drvextra_cmd_parm	*pdrvextra_cmd_parm;	
	struct cmd_priv	*pcmdpriv = &padapter->cmdpriv;
	struct pwrctrl_priv *pwrctrlpriv = &padapter->pwrctrlpriv;
	u8	res = _SUCCESS;
	
_func_enter_;
	if(!pwrctrlpriv->bLeisurePs)
		return res;

	if(enqueue)
	{
		ph2c = (struct cmd_obj*)_rtw_malloc(sizeof(struct cmd_obj));	
		if(ph2c==NULL){
			res= _FAIL;
			goto exit;
		}
		
		pdrvextra_cmd_parm = (struct drvextra_cmd_parm*)_rtw_malloc(sizeof(struct drvextra_cmd_parm)); 
		if(pdrvextra_cmd_parm==NULL){
			_rtw_mfree((unsigned char *)ph2c, sizeof(struct cmd_obj));
			res= _FAIL;
			goto exit;
		}

		pdrvextra_cmd_parm->ec_id = LPS_CTRL_WK_CID;
		pdrvextra_cmd_parm->sz = lps_ctrl_type;
		pdrvextra_cmd_parm->pbuf = NULL;

		init_h2fwcmd_w_parm_no_rsp(ph2c, pdrvextra_cmd_parm, GEN_CMD_CODE(_Set_Drv_Extra));

		rtw_enqueue_cmd_ex(pcmdpriv, ph2c);
	}
	else
	{
		lps_ctrl_wk_hdl(padapter, NULL, lps_ctrl_type);
	}
	
exit:
	
_func_exit_;

	return res;

}

void set_FwPwrMode_cmd(_adapter*padapter, u8 Mode)
{
	SETPWRMODE_PARM H2CSetPwrMode;
	
_func_enter_;
	DBG_871X("%s\n", __FUNCTION__);

	H2CSetPwrMode.Mode = Mode;
	H2CSetPwrMode.SmartPS = 1;
	H2CSetPwrMode.AwakeInterval = 1;//pPSC->RegMaxLPSAwakeIntvl;

	FillH2CCmd(padapter, SET_PWRMODE_EID, sizeof(H2CSetPwrMode), (u8 *)&H2CSetPwrMode);
	
_func_exit_;
}

void ConstructBeacon(_adapter *padapter, u8 *pframe, u32 *pLength)
{
	struct ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	u32					rate_len, pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


	DBG_871X("%s\n", __FUNCTION__);

	pwlanhdr = (struct ieee80211_hdr *)pframe;	

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	
	_rtw_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(cur_network), ETH_ALEN);

	SetSeqNum(pwlanhdr, 0/*pmlmeext->mgnt_seq*/);
	//pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_BEACON);
	
	pframe += sizeof(struct ieee80211_hdr_3addr);	
	pktlen = sizeof (struct ieee80211_hdr_3addr);
	
	//timestamp will be inserted by hardware
	pframe += 8;
	pktlen += 8;

	// beacon interval: 2 bytes
	_rtw_memcpy(pframe, (unsigned char *)(rtw_get_beacon_interval_from_ie(cur_network->IEs)), 2); 
	pframe += 2;
	pktlen += 2;

	// capability info: 2 bytes
	_rtw_memcpy(pframe, (unsigned char *)(rtw_get_capability_from_ie(cur_network->IEs)), 2);
	pframe += 2;
	pktlen += 2;


	if( (pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
	{
		DBG_871X("ie len=%d\n", cur_network->IELength);
		pktlen += cur_network->IELength - sizeof(NDIS_802_11_FIXED_IEs);
		_rtw_memcpy(pframe, cur_network->IEs+sizeof(NDIS_802_11_FIXED_IEs), pktlen);
		
		goto _ConstructBeacon;
	}

	//below for ad-hoc mode

	// SSID
	pframe = rtw_set_ie(pframe, _SSID_IE_, cur_network->Ssid.SsidLength, cur_network->Ssid.Ssid, &pktlen);

	// supported rates...
	rate_len = rtw_get_rateset_len(cur_network->SupportedRates);
	pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_, ((rate_len > 8)? 8: rate_len), cur_network->SupportedRates, &pktlen);

	// DS parameter set
	pframe = rtw_set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&(cur_network->Configuration.DSConfig), &pktlen);

	if( (pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE)
	{
		u32 ATIMWindow;
		// IBSS Parameter Set...
		//ATIMWindow = cur->Configuration.ATIMWindow;
		ATIMWindow = 0;
		pframe = rtw_set_ie(pframe, _IBSS_PARA_IE_, 2, (unsigned char *)(&ATIMWindow), &pktlen);
	}	


	//todo: ERP IE
	
	
	// EXTERNDED SUPPORTED RATE
	if (rate_len > 8)
	{
		pframe = rtw_set_ie(pframe, _EXT_SUPPORTEDRATES_IE_, (rate_len - 8), (cur_network->SupportedRates + 8), &pktlen);
	}


	//todo:HT for adhoc

_ConstructBeacon:

	if ((pktlen + TXDESC_SIZE) > 512)
	{
		DBG_871X("beacon frame too large\n");
		return;
	}

	*pLength = pktlen;

	DBG_871X("%s bcn_sz=%d\n", __FUNCTION__, pktlen);

}

void ConstructPSPoll(_adapter *padapter, u8 *pframe, u32 *pLength)
{
	struct ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	u32					pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	DBG_871X("%s\n", __FUNCTION__);

	pwlanhdr = (struct ieee80211_hdr *)pframe;

	// Frame control.
	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	SetPwrMgt(fctrl);
	SetFrameSubType(pframe, WIFI_PSPOLL);

	// AID.
	SetDuration(pframe, (pmlmeinfo->aid| 0xc000));

	// BSSID.
	_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	// TA.
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);

	*pLength = 16;
}

void ConstructNullFunctionData(_adapter *padapter, u8 *pframe, u32 *pLength, u8 *StaAddr, BOOLEAN bForcePowerSave)
{
	struct ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	u32					pktlen;
	struct mlme_priv		*pmlmepriv = &padapter->mlmepriv;
	struct wlan_network	*cur_network = &pmlmepriv->cur_network;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	DBG_871X("%s:%d\n", __FUNCTION__, bForcePowerSave);

	pwlanhdr = (struct ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	if (bForcePowerSave)
	{
		SetPwrMgt(fctrl);
	}

	switch(cur_network->network.InfrastructureMode)
	{			
		case Ndis802_11Infrastructure:
			SetToDs(fctrl);
			_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
			_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
			_rtw_memcpy(pwlanhdr->addr3, StaAddr, ETH_ALEN);
			break;
		case Ndis802_11APMode:
			SetFrDs(fctrl);
			_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
			_rtw_memcpy(pwlanhdr->addr2, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
			_rtw_memcpy(pwlanhdr->addr3, myid(&(padapter->eeprompriv)), ETH_ALEN);
			break;
		case Ndis802_11IBSS:
		default:
			_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
			_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
			_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
			break;
	}

	SetSeqNum(pwlanhdr, 0);

	SetFrameSubType(pframe, WIFI_DATA_NULL);

	pframe += sizeof(struct ieee80211_hdr_3addr);
	pktlen = sizeof(struct ieee80211_hdr_3addr);

	*pLength = pktlen;
}

void ConstructProbeRsp(_adapter *padapter, u8 *pframe, u32 *pLength, u8 *StaAddr, BOOLEAN bHideSSID)
{
	struct ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;	
	u8					*mac, *bssid;
	u32					pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	
	
	DBG_871X("%s\n", __FUNCTION__);
	
	pwlanhdr = (struct ieee80211_hdr *)pframe;	
	
	mac = myid(&(padapter->eeprompriv));
	bssid = cur_network->MacAddress;
	
	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, bssid, ETH_ALEN);

	SetSeqNum(pwlanhdr, 0);
	SetFrameSubType(fctrl, WIFI_PROBERSP);
	
	pktlen = sizeof(struct ieee80211_hdr_3addr);
	pframe += pktlen;

	if(cur_network->IELength>MAX_IE_SZ)
		return;

	_rtw_memcpy(pframe, cur_network->IEs, cur_network->IELength);
	pframe += cur_network->IELength;
	pktlen += cur_network->IELength;
	
	*pLength = pktlen;
}

//
// Description: In normal chip, we should send some packet to Hw which will be used by Fw
//			in FW LPS mode. The function is to fill the Tx descriptor of this packets, then 
//			Fw can tell Hw to send these packet derectly.
// Added by tynli. 2009.10.15.
//
VOID
FillFakeTxDescriptor92C(
	IN PADAPTER		Adapter,
	IN u8*			pDesc,
	IN u32			BufferLen,
	IN BOOLEAN		IsPsPoll
)
{
	struct tx_desc	*ptxdesc = (struct tx_desc *)pDesc;

	// Clear all status
	_rtw_memset(pDesc, 0, 32);

	//offset 0
	ptxdesc->txdw0 |= cpu_to_le32( OWN | FSG | LSG); //own, bFirstSeg, bLastSeg;

	ptxdesc->txdw0 |= cpu_to_le32(((TXDESC_SIZE+OFFSET_SZ)<<OFFSET_SHT)&0x00ff0000); //32 bytes for TX Desc

	ptxdesc->txdw0 |= cpu_to_le32(BufferLen&0x0000ffff); // Buffer size + command header

	//offset 4
	ptxdesc->txdw1 |= cpu_to_le32((QSLT_MGNT<<QSEL_SHT)&0x00001f00); // Fixed queue of Mgnt queue

	//Set NAVUSEHDR to prevent Ps-poll AId filed to be changed to error vlaue by Hw.
	if(IsPsPoll)
	{
		ptxdesc->txdw1 |= cpu_to_le32(NAVUSEHDR);
	}
	else
	{
		ptxdesc->txdw4 |= cpu_to_le32(BIT(7)); // Hw set sequence number
		ptxdesc->txdw3 |= cpu_to_le32((8 <<28)); //set bit3 to 1. Suugested by TimChen. 2009.12.29.
	}

	//offset 16
	ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate
	
	// USB interface drop packet if the checksum of descriptor isn't correct.
	// Using this checksum can let hardware recovery from packet bulk out error (e.g. Cancel URC, Bulk out error.).
	cal_txdesc_chksum(ptxdesc);
	
	//RT_PRINT_DATA(COMP_CMD, DBG_TRACE, "TxFillCmdDesc8192C(): H2C Tx Cmd Content ----->\n", pDesc, TX_DESC_SIZE);
}


//
// Description: Fill the reserved packets that FW will use to RSVD page. 
//			Now we just send 4 types packet to rsvd page.
//			(1)Beacon, (2)Ps-poll, (3)Null data, (4)ProbeRsp.
//	Input: 
//	    bDLFinished - FALSE: At the first time we will send all the packets as a large packet to Hw,
//				 		so we need to set the packet length to total lengh.
//			      TRUE: At the second time, we should send the first packet (default:beacon)
//						to Hw again and set the lengh in descriptor to the real beacon lengh.
// 2009.10.15 by tynli.
void SetFwRsvdPagePkt(PADAPTER Adapter, BOOLEAN bDLFinished)
{
	struct xmit_frame	*pmgntframe;
	struct pkt_attrib	*pattrib;
	struct xmit_priv	*pxmitpriv = &(Adapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(Adapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u32	BeaconLength, ProbeRspLength, PSPollLength, NullFunctionDataLength;
	u8	*ReservedPagePacket;	
	u8	PageNum=0, U1bTmp, TxDescLen=0;
	u16	BufIndex=0;
	u32	TotalPacketLen;
	RSVDPAGE_LOC	RsvdPageLoc;
	BOOLEAN	bDLOK = _FALSE;

	DBG_871X("%s\n", __FUNCTION__);

	ReservedPagePacket = (u8*)_rtw_malloc(1000);
	if(ReservedPagePacket == NULL){
		DBG_871X("%s(): alloc ReservedPagePacket fail !!!\n", __FUNCTION__);
		return;
	}

	_rtw_memset(ReservedPagePacket, 0, 1000);

	if(DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
	{
		BufIndex = TXDESC_OFFSET;
	}
	else
	{
		BufIndex = 0;
	}

	TxDescLen = TXDESC_SIZE;

	//(1) beacon
	ConstructBeacon(Adapter,&ReservedPagePacket[BufIndex],&BeaconLength);

	//printk("SetFwRsvdPagePkt(): HW_VAR_SET_TX_CMD: BCN\n", &ReservedPagePacket[BufIndex], (BeaconLength+BufIndex));

//--------------------------------------------------------------------

	// When we count the first page size, we need to reserve description size for the RSVD 
	// packet, it will be filled in front of the packet in TXPKTBUF.
	U1bTmp = (u8)PageNum_128(BeaconLength+TxDescLen);
	PageNum += U1bTmp;

	if(DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
		BufIndex = (PageNum*128) + TxDescLen+8; //Shift index for 8 bytes because the dummy bytes in the first descipstor.
	else
		BufIndex = (PageNum*128);
		
	//(2) ps-poll
	ConstructPSPoll(Adapter, &ReservedPagePacket[BufIndex],&PSPollLength);
	
	FillFakeTxDescriptor92C(Adapter, &ReservedPagePacket[BufIndex-TxDescLen], PSPollLength, _TRUE);

	//printk("SetFwRsvdPagePkt(): HW_VAR_SET_TX_CMD: PS-POLL\n", &ReservedPagePacket[BufIndex-TxDescLen], (PSPollLength+TxDescLen));

	RsvdPageLoc.LocPsPoll = PageNum;

//------------------------------------------------------------------
			
	U1bTmp = (u8)PageNum_128(PSPollLength+TxDescLen);
	PageNum += U1bTmp;

	if(DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
		BufIndex = (PageNum*128) + TxDescLen+8;
	else
		BufIndex = (PageNum*128);

	//(3) null data
	ConstructNullFunctionData(
		Adapter, 
		&ReservedPagePacket[BufIndex],
		&NullFunctionDataLength,
		get_my_bssid(&(pmlmeinfo->network)),
		_FALSE);
	
	FillFakeTxDescriptor92C(Adapter, &ReservedPagePacket[BufIndex-TxDescLen], NullFunctionDataLength, _FALSE);

	RsvdPageLoc.LocNullData = PageNum;
	
	//printk("SetFwRsvdPagePkt(): HW_VAR_SET_TX_CMD: NULL DATA \n", &ReservedPagePacket[BufIndex-TxDescLen], (NullFunctionDataLength+TxDescLen));
//------------------------------------------------------------------

	U1bTmp = (u8)PageNum_128(NullFunctionDataLength+TxDescLen);
	PageNum += U1bTmp;
	
	if(DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
		BufIndex = (PageNum*128) + TxDescLen+8;
	else
		BufIndex = (PageNum*128);
	
	//(4) probe response
	ConstructProbeRsp(
		Adapter, 
		&ReservedPagePacket[BufIndex],
		&ProbeRspLength,
		get_my_bssid(&(pmlmeinfo->network)),
		_FALSE);
	
	FillFakeTxDescriptor92C(Adapter, &ReservedPagePacket[BufIndex-TxDescLen], ProbeRspLength, _FALSE);

	RsvdPageLoc.LocProbeRsp = PageNum;

	//printk("SetFwRsvdPagePkt(): HW_VAR_SET_TX_CMD: PROBE RSP \n", &ReservedPagePacket[BufIndex-TxDescLen], (ProbeRspLength-TxDescLen));

//------------------------------------------------------------------

	U1bTmp = (u8)PageNum_128(ProbeRspLength+TxDescLen);

	PageNum += U1bTmp;

	TotalPacketLen = (PageNum*128);

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(Adapter, pattrib);
	pattrib->qsel = 0x10;
	pattrib->pktlen = pattrib->last_txcmdsz = TotalPacketLen - TxDescLen;
	_rtw_memcpy(pmgntframe->buf_addr, ReservedPagePacket, TotalPacketLen);

	dump_mgntframe(Adapter, pmgntframe);
	bDLOK = _TRUE;

	if(bDLOK)
	{
		DBG_871X("Set RSVD page location to Fw Len(%d).\n",sizeof(RsvdPageLoc));		
		FillH2CCmd(Adapter, RSVD_PAGE_EID, sizeof(RsvdPageLoc), (u8 *)&RsvdPageLoc);
	}

	_rtw_mfree(ReservedPagePacket, 1000);

}

void set_FwJoinBssReport_cmd(_adapter* padapter, u8 mstatus)
{
	JOINBSSRPT_PARM	JoinBssRptParm;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
_func_enter_;

	DBG_871X("%s\n", __FUNCTION__);

	if(mstatus == 1)
	{
		// We should set AID, correct TSF, HW seq enable before set JoinBssReport to Fw in 88/92C.
		// Suggested by filen. Added by tynli.
		rtw_write16(padapter, REG_BCN_PSR_RPT, (0xC000|pmlmeinfo->aid));
		// Do not set TSF again here or vWiFi beacon DMA INT will not work.
		//correct_TSF(padapter, pmlmeext);
		// Hw sequende enable by dedault. 2010.06.23. by tynli.
		//rtw_write16(padapter, REG_NQOS_SEQ, ((pmlmeext->mgnt_seq+100)&0xFFF));
		//rtw_write8(padapter, REG_HWSEQ_CTRL, 0xFF);

		if(IS_NORMAL_CHIP(pHalData->VersionID))
		{
			BOOLEAN bRecover = _FALSE;

			//set REG_CR bit 8
			//U1bTmp = rtw_read8(padapter, REG_CR+1);
			rtw_write8(padapter,  REG_CR+1, 0x03);

			// Disable Hw protection for a time which revserd for Hw sending beacon.
			// Fix download reserved page packet fail that access collision with the protection time.
			// 2010.05.11. Added by tynli.
			//SetBcnCtrlReg(padapter, 0, BIT3);
			//SetBcnCtrlReg(padapter, BIT4, 0);
			rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)&(~BIT(3)));
			rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(4));

			// Set FWHW_TXQ_CTRL 0x422[6]=0 to tell Hw the packet is not a real beacon frame.
			if(pHalData->RegFwHwTxQCtrl&BIT6)
				bRecover = _TRUE;

			// To tell Hw the packet is not a real beacon frame.
			//U1bTmp = rtw_read8(padapter, REG_FWHW_TXQ_CTRL+2);
			rtw_write8(padapter, REG_FWHW_TXQ_CTRL+2, (pHalData->RegFwHwTxQCtrl&(~BIT6)));
			pHalData->RegFwHwTxQCtrl &= (~BIT6);
			SetFwRsvdPagePkt(padapter, 0);

			// 2010.05.11. Added by tynli.
			//SetBcnCtrlReg(padapter, BIT3, 0);
			//SetBcnCtrlReg(padapter, 0, BIT4);
			rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(3));
			rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)&(~BIT(4)));

			// To make sure that if there exists an adapter which would like to send beacon.
			// If exists, the origianl value of 0x422[6] will be 1, we should check this to
			// prevent from setting 0x422[6] to 0 after download reserved page, or it will cause 
			// the beacon cannot be sent by HW.
			// 2010.06.23. Added by tynli.
			if(bRecover)
			{
				rtw_write8(padapter, REG_FWHW_TXQ_CTRL+2, (pHalData->RegFwHwTxQCtrl|BIT6));
				pHalData->RegFwHwTxQCtrl |= BIT6;
			}

			// Clear CR[8] or beacon packet will not be send to TxBuf anymore.
			rtw_write8(padapter, REG_CR+1, 0x02);
		}
	}

	JoinBssRptParm.OpMode = mstatus;
	printk("%s H2C len(%d)\n",__FUNCTION__,sizeof(JoinBssRptParm));
	FillH2CCmd(padapter, JOINBSS_RPT_EID, sizeof(JoinBssRptParm), (u8 *)&JoinBssRptParm);
	
_func_exit_;
}
#endif

void rtw_dummy_event_callback(_adapter *adapter , u8 *pbuf)
{

}
static void fwdbg_event_callback(_adapter *adapter , u8 *pbuf)
{

}

