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
#define _HCI_INTF_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>
#include <hal_init.h>
#include <rtl8712_efuse.h>
#include <rtw_version.h>

#ifndef CONFIG_USB_HCI

#error "CONFIG_USB_HCI shall be on!\n"

#endif

#include <usb_vendor_req.h>
#include <usb_ops.h>
#include <usb_osintf.h>
#include <usb_hal.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifdef CONFIG_80211N_HT
extern int rtw_ht_enable;
extern int rtw_cbw40_enable;
extern int rtw_ampdu_enable;//for enable tx_ampdu
#endif

extern char* rtw_initmac;

static struct usb_interface *pintf;

extern u32 rtw_start_drv_threads(_adapter *padapter);
extern void rtw_stop_drv_threads (_adapter *padapter);
extern u8 rtw_init_drv_sw(_adapter *padapter);
extern u8 rtw_free_drv_sw(_adapter *padapter);
extern struct net_device *rtw_init_netdev(void);
extern void cancel_all_timer(_adapter *padapter);
#ifdef CONFIG_IPS
extern int  ips_netdrv_open(_adapter *padapter);
extern void ips_dev_unload(_adapter *padapter);
#endif

extern int pm_netdev_open(struct net_device *pnetdev);
int rtw_suspend(struct usb_interface *intf, pm_message_t message);
int rtw_resume(struct usb_interface *intf);


extern u8 reset_drv_sw(_adapter *padapter);
static void rtw_dev_unload(_adapter *padapter);

static int rtw_drv_init(struct usb_interface *pusb_intf,const struct usb_device_id *pdid);
static void rtw_dev_remove(struct usb_interface *pusb_intf);

#define USB_VENDER_ID_REALTEK		0x0BDA
//2010-10-19 DID_USB_V38
static struct usb_device_id rtw_usb_id_tbl[] ={

	/*=== Realtek demoboard ===*/		
	{USB_DEVICE(0x0BDA, 0x8191)},//Default ID
	
	/****** 8188CU ********/
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8176)},//8188cu 1*1 dongole 
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8177)},//8188cu 1*1 dongole, (b/g mode only)
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8170)},//8188CE-VAU USB minCard
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x817A)},//8188cu Slim Solo
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x817B)},//8188cu Slim Combo	
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x817D)},//8188RU High-power USB Dongle
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8754)},//8188 Combo for BC4
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x817E)},//8188CE-VAU USB minCard (b/g mode only)
	
	/****** 8192CU ********/
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8177)},//8191cu 1*2
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x8178)},//8192cu 2*2
	{USB_DEVICE(USB_VENDER_ID_REALTEK, 0x817C)},//8192CE-VAU USB minCard

	/*=== Customer ID ===*/	
	/****** 8188CU ********/
	{USB_DEVICE(0x07B8, 0x8189)},//Funai - Abocom
	{USB_DEVICE(0x2019, 0xED17)},//PCI - Edimax        
	{USB_DEVICE(0x0DF6, 0x0052)}, //Sitecom - Edimax    
	{USB_DEVICE(0x7392, 0x7811)},//Edimax - Edimax  
	{USB_DEVICE(0x07B8, 0x8188)},//Abocom - Abocom 
	{USB_DEVICE(0x0EB0, 0x9071)},//NO Brand - Etop  
	{USB_DEVICE(0x06F8, 0xE033)},//Hercules - Edimax 
	{USB_DEVICE(0x103C, 0x1629)},//HP - Lite-On ,8188CUS Slim Combo
	//{USB_DEVICE(0x0DF6, 0x0052)},//Sitecom - Edimax V18
	//{USB_DEVICE(0x0EB0, 0x9071)},//NO Brand - Etop   
	//{USB_DEVICE(0x06F8, 0xE033)},//Hercules - Edimax  V18
	{USB_DEVICE(0x2001, 0x3308)},//D-Link - Alpha
	{USB_DEVICE(0x050D, 0x1102)},//Belkin - Edimax
	{USB_DEVICE(0x2019, 0xAB2A)},//Planex - Abocom
	{USB_DEVICE(0x20F4, 0x648B)},//TRENDnet - Cameo
	{USB_DEVICE(0x4855, 0x0090)},// 	- Feixun
	
	{USB_DEVICE(0x3358, 0x13D3)},// -Azwave 8188CE-VAU
	{USB_DEVICE(0x3359, 0x13D3)},//Russian customer -Azwave (8188CE-VAU  g mode)
	
	/****** 8192CU ********/	
	{USB_DEVICE(0x07b8, 0x8178)},//Funai -Abocom
	{USB_DEVICE(0x2001, 0x3307)},//D-Link-Cameo   
	{USB_DEVICE(0x2001, 0x330A)},//D-Link-Alpha   
	{USB_DEVICE(0x2001, 0x3309)},//D-Link-Alpha   	
	{USB_DEVICE(0x0586, 0x341F)},//Zyxel -Abocom
	{USB_DEVICE(0x7392, 0x7822)},//Edimax -Edimax	
	{USB_DEVICE(0x2019, 0xAB2B)},//Planex -Abocom
	{USB_DEVICE(0x07B8, 0x8178)},//Abocom -Abocom	
	{USB_DEVICE(0x07AA, 0x0056)},//ATKK-Gemtek	
	{USB_DEVICE(0x4855, 0x0091)},// 	-Feixun
	{}
};

static struct specific_device_id specific_device_id_tbl[] = {
		{.idVendor=USB_VENDER_ID_REALTEK, .idProduct=0x817E, .flags=SPEC_DEV_ID_DISABLE_HT},
		{.idVendor=USB_VENDER_ID_REALTEK, .idProduct=0x8177, .flags=SPEC_DEV_ID_DISABLE_HT},		
		{.idVendor=0x0b05, .idProduct=0x1791, .flags=SPEC_DEV_ID_DISABLE_HT},
		{.idVendor=0x13D3, .idProduct=0x3311, .flags=SPEC_DEV_ID_DISABLE_HT},
		{.idVendor=0x13D3, .idProduct=0x3359, .flags=SPEC_DEV_ID_DISABLE_HT},		
	{}
};

typedef struct _driver_priv{

	struct usb_driver rtw_usb_drv;
	int drv_registered;

}drv_priv, *pdrv_priv;

static drv_priv drvpriv = {
	.rtw_usb_drv.name = (char*)"rtw_usb_drv",
	.rtw_usb_drv.probe = rtw_drv_init,
	.rtw_usb_drv.disconnect = rtw_dev_remove,
	.rtw_usb_drv.id_table = rtw_usb_id_tbl,
	.rtw_usb_drv.suspend =  rtw_suspend,
	.rtw_usb_drv.resume = rtw_resume,
#ifdef CONFIG_AUTOSUSPEND	
	.rtw_usb_drv.supports_autosuspend = 1,	
#endif
};

MODULE_DEVICE_TABLE(usb, rtw_usb_id_tbl);

static void intf_chip_configure(_adapter *padapter)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);	

	if(pHalData->hal_ops.intf_chip_configure)
		pHalData->hal_ops.intf_chip_configure(padapter);
}

static void intf_read_chip_info(_adapter *padapter)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);

	pHalData->hal_ops.read_adapter_info(padapter);
}

static u32 usb_dvobj_init(_adapter *padapter)
{
	int i;
	u8 val8;
	u32 blocksz;
	int status = _SUCCESS;

	struct usb_device_descriptor 		*pdev_desc;

	struct usb_host_config			*phost_conf;
	struct usb_config_descriptor 		*pconf_desc;

	struct usb_host_interface		*phost_iface;
	struct usb_interface_descriptor		*piface_desc;

	struct usb_host_endpoint		*phost_endp;
	struct usb_endpoint_descriptor		*pendp_desc;

	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);

	struct dvobj_priv *pdvobjpriv = &padapter->dvobjpriv;
	struct usb_device *pusbd = pdvobjpriv->pusbdev;

	//PURB urb = NULL;

_func_enter_;

	pdvobjpriv->padapter = padapter;

	pHalData->RtNumInPipes = 0;
	pHalData->RtNumOutPipes = 0;

	//padapter->EepromAddressSize = 6;
	//pdvobjpriv->nr_endpoint = 6;

	pdev_desc = &pusbd->descriptor;

#if 0
	printk("\n8712_usb_device_descriptor:\n");
	printk("bLength=%x\n", pdev_desc->bLength);
	printk("bDescriptorType=%x\n", pdev_desc->bDescriptorType);
	printk("bcdUSB=%x\n", pdev_desc->bcdUSB);
	printk("bDeviceClass=%x\n", pdev_desc->bDeviceClass);
	printk("bDeviceSubClass=%x\n", pdev_desc->bDeviceSubClass);
	printk("bDeviceProtocol=%x\n", pdev_desc->bDeviceProtocol);
	printk("bMaxPacketSize0=%x\n", pdev_desc->bMaxPacketSize0);
	printk("idVendor=%x\n", pdev_desc->idVendor);
	printk("idProduct=%x\n", pdev_desc->idProduct);
	printk("bcdDevice=%x\n", pdev_desc->bcdDevice);
	printk("iManufacturer=%x\n", pdev_desc->iManufacturer);
	printk("iProduct=%x\n", pdev_desc->iProduct);
	printk("iSerialNumber=%x\n", pdev_desc->iSerialNumber);
	printk("bNumConfigurations=%x\n", pdev_desc->bNumConfigurations);
#endif

	phost_conf = pusbd->actconfig;
	pconf_desc = &phost_conf->desc;

#if 0
	printk("\n8712_usb_configuration_descriptor:\n");
	printk("bLength=%x\n", pconf_desc->bLength);
	printk("bDescriptorType=%x\n", pconf_desc->bDescriptorType);
	printk("wTotalLength=%x\n", pconf_desc->wTotalLength);
	printk("bNumInterfaces=%x\n", pconf_desc->bNumInterfaces);
	printk("bConfigurationValue=%x\n", pconf_desc->bConfigurationValue);
	printk("iConfiguration=%x\n", pconf_desc->iConfiguration);
	printk("bmAttributes=%x\n", pconf_desc->bmAttributes);
	printk("bMaxPower=%x\n", pconf_desc->bMaxPower);
#endif

	//printk("\n/****** num of altsetting = (%d) ******/\n", pintf->num_altsetting);

	phost_iface = &pintf->altsetting[0];
	piface_desc = &phost_iface->desc;

#if 0
	printk("\n8712_usb_interface_descriptor:\n");
	printk("bLength=%x\n", piface_desc->bLength);
	printk("bDescriptorType=%x\n", piface_desc->bDescriptorType);
	printk("bInterfaceNumber=%x\n", piface_desc->bInterfaceNumber);
	printk("bAlternateSetting=%x\n", piface_desc->bAlternateSetting);
	printk("bNumEndpoints=%x\n", piface_desc->bNumEndpoints);
	printk("bInterfaceClass=%x\n", piface_desc->bInterfaceClass);
	printk("bInterfaceSubClass=%x\n", piface_desc->bInterfaceSubClass);
	printk("bInterfaceProtocol=%x\n", piface_desc->bInterfaceProtocol);
	printk("iInterface=%x\n", piface_desc->iInterface);
#endif

	pdvobjpriv->nr_endpoint = piface_desc->bNumEndpoints;


	//printk("\ndump 8712_usb_endpoint_descriptor:\n");

	for (i = 0; i < pdvobjpriv->nr_endpoint; i++)
	{
		phost_endp = phost_iface->endpoint + i;
		if (phost_endp)
		{
			pendp_desc = &phost_endp->desc;

			printk("\n8712_usb_endpoint_descriptor(%d):\n", i);
			printk("bLength=%x\n",pendp_desc->bLength);
			printk("bDescriptorType=%x\n",pendp_desc->bDescriptorType);
			printk("bEndpointAddress=%x\n",pendp_desc->bEndpointAddress);

			if (pendp_desc->bEndpointAddress & USB_DIR_IN)
				pHalData->RtNumInPipes++;
			else
				pHalData->RtNumOutPipes++;

			//printk("bmAttributes=%x\n",pendp_desc->bmAttributes);
			//printk("wMaxPacketSize=%x\n",pendp_desc->wMaxPacketSize);
			printk("wMaxPacketSize=%x\n",le16_to_cpu(pendp_desc->wMaxPacketSize));
			printk("bInterval=%x\n",pendp_desc->bInterval);
			//printk("bRefresh=%x\n",pendp_desc->bRefresh);
			//printk("bSynchAddress=%x\n",pendp_desc->bSynchAddress);
		}
	}
	
	printk("nr_endpoint=%d, in_num=%d, out_num=%d\n\n", pdvobjpriv->nr_endpoint, pHalData->RtNumInPipes, pHalData->RtNumOutPipes);

	if (pusbd->speed == USB_SPEED_HIGH)
	{
		pdvobjpriv->ishighspeed = _TRUE;
		pHalData->UsbBulkOutSize = USB_HIGH_SPEED_BULK_SIZE;//512 bytes
		printk("8192cu: USB_SPEED_HIGH\n");
	}
	else
	{
		pdvobjpriv->ishighspeed = _FALSE;
		pHalData->UsbBulkOutSize = USB_FULL_SPEED_BULK_SIZE;//64 bytes
		printk("8192cu: NON USB_SPEED_HIGH\n");
	}


	//.2
	if ((init_io_priv(padapter)) == _FAIL)
	{
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,(" \n Can't init io_reqs\n"));
		status = _FAIL;
	}

	//.3 misc
	_rtw_init_sema(&(padapter->dvobjpriv.usb_suspend_sema), 0);	


	//.4 usb endpoint mapping
	intf_chip_configure(padapter);

_func_exit_;

	return status;
}

static void usb_dvobj_deinit(_adapter * padapter){

	struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;

	_func_enter_;


	_func_exit_;
}

static void decide_chip_type_by_usb_device_id(_adapter *padapter, const struct usb_device_id *pdid)
{

	padapter->chip_type = NULL_CHIP_TYPE;

	//TODO:
	padapter->chip_type = RTL8188C_8192C;

}


static void rtw_intf_stop(_adapter *padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtl871x_intf_stop\n"));

	//disabel_hw_interrupt
	if(padapter->bSurpriseRemoved == _FALSE)
	{
		//device still exists, so driver can do i/o operation
		//TODO:
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("SurpriseRemoved==_FALSE\n"));
	}

	//cancel in irp
	if(pHalData->hal_ops.inirp_deinit !=NULL)
	{
		pHalData->hal_ops.inirp_deinit(padapter);
	}

	//cancel out irp
	write_port_cancel(padapter);


	//todo:cancel other irps

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-rtl871x_intf_stop\n"));

}
#ifdef CONFIG_IPS
void ips_dev_unload(_adapter *padapter)
{
	struct net_device *pnetdev= (struct net_device*)padapter->pnetdev;
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
	u8 trycnt = 100;
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+ips_dev_unload\n"));

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+ips_dev_unload\n"));
	printk("%s...\n",__FUNCTION__);
//	if(padapter->bup == _TRUE)
	{
		printk("+ips_dev_unload\n");
		//padapter->bup = _FALSE;
		//padapter->bDriverStopped = _TRUE;

		//s3.
		rtw_write8(padapter,0x522,0xff);//pause tx
		//keep sn
		pxmitpriv->nqos_ssn = rtw_read16(padapter,0x4dc);
		//RX DMA stop
		rtw_write32(padapter,0x284,(rtw_read32(padapter,0x284)|BIT18));
		do{
			if(!(rtw_read32(padapter,0x284)&BIT17))
				break;				
		}while(trycnt--);
		if(trycnt ==0)
		{
			printk("Stop RX DMA failed \n");
		}
		//RQPN Load 0
		rtw_write16(padapter,0x214,0x0);
		rtw_write32(padapter,0x200,0x80000000);
		rtw_mdelay_os(6);
		
		rtw_intf_stop(padapter);//cancel read /write port

		//s5.
		if(padapter->bSurpriseRemoved == _FALSE)
		{
			printk("r871x_dev_unload()->rtl871x_hal_deinit()\n");
			rtw_hal_deinit(padapter);

			//padapter->bSurpriseRemoved = _TRUE;
		}

		//s6.
		if(padapter->dvobj_deinit)
		{
			padapter->dvobj_deinit(padapter);

		}
		else
		{
			RT_TRACE(_module_hci_intfs_c_,_drv_err_,("Initialize hcipriv.hci_priv_init error!!!\n"));
		}

	}
/*	else
	{
		printk("ips_dev_unload():padapter->bup == _FALSE\n" );
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("r871x_dev_unload():padapter->bup == _FALSE\n" ));
	}*/
	printk("-ips_dev_unload\n");

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-r871x_dev_unload\n"));
}
#endif
static void rtw_dev_unload(_adapter *padapter)
{
	struct net_device *pnetdev= (struct net_device*)padapter->pnetdev;

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtw_dev_unload\n"));

	if(padapter->bup == _TRUE)
	{
		printk("+rtw_dev_unload\n");
		//s1.
/*		if(pnetdev)
		{
			netif_carrier_off(pnetdev);
			netif_stop_queue(pnetdev);
		}

		//s2.
		//s2-1.  issue rtw_disassoc_cmd to fw
		rtw_disassoc_cmd(padapter);
		//s2-2.  indicate disconnect to os
		rtw_indicate_disconnect(padapter);
		//s2-3.
		rtw_free_assoc_resources(padapter);
		//s2-4.
		rtw_free_network_queue(padapter,_TRUE);*/

		padapter->bDriverStopped = _TRUE;

		//s3.
		rtw_write8(padapter,0x522,0xff);//pause tx/rx
		rtw_intf_stop(padapter);

		//s4.
		rtw_stop_drv_threads(padapter);


		//s5.
		if(padapter->bSurpriseRemoved == _FALSE)
		{
			//printk("r871x_dev_unload()->rtl871x_hal_deinit()\n");
			rtw_hal_deinit(padapter);

			padapter->bSurpriseRemoved = _TRUE;
		}

		//s6.
		usb_dvobj_deinit(padapter);
		

		padapter->bup = _FALSE;

	}
	else
	{
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("r871x_dev_unload():padapter->bup == _FALSE\n" ));
	}

	printk("-rtw_dev_unload\n");

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-rtw_dev_unload\n"));

}

static void disable_ht_for_spec_devid(const struct usb_device_id *pdid)
{
#ifdef CONFIG_80211N_HT
	u16 vid, pid;
	u32 flags;
	int i;
	int num = sizeof(specific_device_id_tbl)/sizeof(struct specific_device_id);

	for(i=0; i<num; i++)
	{
		vid = specific_device_id_tbl[i].idVendor;
		pid = specific_device_id_tbl[i].idProduct;
		flags = specific_device_id_tbl[i].flags;

		if((pdid->idVendor==vid) && (pdid->idProduct==pid) && (flags&SPEC_DEV_ID_DISABLE_HT))
		{
			 rtw_ht_enable = 0;
			 rtw_cbw40_enable = 0;
			 rtw_ampdu_enable = 0;
		}

	}
#endif
}


int rtw_suspend(struct usb_interface *pusb_intf, pm_message_t message)
{
	struct net_device *pnetdev=usb_get_intfdata(pusb_intf);
	_adapter *padapter = (_adapter*)netdev_priv(pnetdev);
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;
	struct usb_device *usb_dev = interface_to_usbdev(pusb_intf);
	
	_func_enter_;

	if((!padapter->bup) || (padapter->bDriverStopped))
	{
		printk("padapter->bup=%d bDriverStopped=%d\n",padapter->bup, padapter->bDriverStopped);		
		goto error_exit;
	}
	

	printk("###########  rtw_suspend  #################\n");

#ifdef CONFIG_AUTOSUSPEND
	/* device may already be autosuspend when a  system suspend occurs*/
	#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,33))
		if (message.event & PM_EVENT_AUTO)
	#else
		if (usb_dev->auto_pm > 0)
	#endif		
		{
						
			printk("********** in autosuspend *******************\n");
			//return -EBADF;
		}

#endif

	if(padapter)//system suspend
	{		
		LeaveAllPowerSaveMode(padapter);
		
		printk("==> rtw_suspend\n");	
		_enter_pwrlock(&pwrpriv->lock);
		padapter->net_closed = _TRUE;
		//s1.
		if(pnetdev)
		{
			netif_carrier_off(pnetdev);
			netif_stop_queue(pnetdev);
		}

		//s2.
		//s2-1.  issue rtw_disassoc_cmd to fw
		rtw_disassoc_cmd(padapter);
		//s2-2.  indicate disconnect to os
		rtw_indicate_disconnect(padapter);
		//s2-3.
		rtw_free_assoc_resources(padapter);
#ifdef CONFIG_AUTOSUSPEND
		if(!pwrpriv->bInternalAutoSuspend )
#endif
		//s2-4.
		rtw_free_network_queue(padapter,_TRUE);

		rtw_dev_unload(padapter);
#ifdef CONFIG_AUTOSUSPEND
		pwrpriv->current_rfpwrstate = rf_off;
		pwrpriv->bips_processing = _FALSE;
		
#endif		
		_exit_pwrlock(&pwrpriv->lock);
	}
	else
		goto error_exit;
	
	_func_exit_;
	return 0;
	
error_exit:
	printk("%s, failed \n",__FUNCTION__);
	return (-1);

}

int rtw_resume(struct usb_interface *pusb_intf)
{
	struct net_device *pnetdev=usb_get_intfdata(pusb_intf);
	_adapter *padapter = (_adapter*)netdev_priv(pnetdev);
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;
	struct usb_device *usb_dev = interface_to_usbdev(pusb_intf);
	
	_func_enter_;

	printk("###########  rtw_resume  #################\n");
	printk("bkeepfwalive(%x)\n",pwrpriv->bkeepfwalive);
#ifdef SUPPORT_HW_RFOFF_DETECTED
/*
	if((padapter) && (padapter->pwrctrlpriv.bHWPwrPindetect) &&(padapter->registrypriv.usbss_enable))
	{
		rt_rf_power_state rfpwrstate;
		rfpwrstate = RfOnOffDetect(padapter);
		if(rfpwrstate == rf_off)
		{
			printk("still in rf_off ,abort resume......\n");
			return (0);	
		}
	}
*/
#endif	
	if(padapter)//system resume
	{	
		printk("==> rtw_resume\n");
		_enter_pwrlock(&pwrpriv->lock);
		reset_drv_sw(padapter);
	
		if(pm_netdev_open(pnetdev) != 0)
			goto error_exit;

		netif_device_attach(pnetdev);	
		netif_carrier_on(pnetdev);
#ifdef CONFIG_AUTOSUSPEND
		if(pwrpriv->bInternalAutoSuspend )
		{
			HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
			pwrpriv->current_rfpwrstate = rf_on;	
			pwrpriv->bkeepfwalive = _FALSE;
			pwrpriv->bInternalAutoSuspend = _FALSE;
			pwrpriv->brfoffbyhw = _FALSE;
	#if ( RTL8192C_WEP_ISSUE==1)	
			if(!IS_92C_SERIAL(pHalData->VersionID))
	#endif
			{
				printk("enc_algorithm(%x),wepkeymask(%x)\n",padapter->securitypriv.dot11PrivacyAlgrthm,pwrpriv->wepkeymask);
				if((_WEP40_ == padapter->securitypriv.dot11PrivacyAlgrthm) ||(_WEP104_ == padapter->securitypriv.dot11PrivacyAlgrthm))
				{
					sint keyid;	
			
					for(keyid=0;keyid<4;keyid++){				
						if(pwrpriv->wepkeymask & BIT(keyid))
							rtw_set_key(padapter,&padapter->securitypriv, keyid);	
					}
				}
			}
		}
#endif
		_exit_pwrlock(&pwrpriv->lock);
	}
	else
	{
		goto error_exit;	
	}

	_func_exit_;
	
	return 0;
error_exit:
	printk("%s, Open net dev failed \n",__FUNCTION__);
	return (-1);
}


static u8 key_char2num(u8 ch)
{
    if((ch>='0')&&(ch<='9'))
        return ch - '0';
    else if ((ch>='a')&&(ch<='f'))
        return ch - 'a' + 10;
    else if ((ch>='A')&&(ch<='F'))
        return ch - 'A' + 10;
    else
	 return 0xff;
}

static u8 key_2char2num(u8 hch, u8 lch)
{
    return ((key_char2num(hch) << 4) | key_char2num(lch));
}

/*
 * drv_init() - a device potentially for us
 *
 * notes: drv_init() is called when the bus driver has located a card for us to support.
 *        We accept the new device by returning 0.
*/
static int rtw_drv_init(struct usb_interface *pusb_intf, const struct usb_device_id *pdid)
{
	int i;
	u8 mac[ETH_ALEN];
	uint status;
	_adapter *padapter = NULL;
	struct dvobj_priv *pdvobjpriv;
	struct net_device *pnetdev;
	//struct device *dev = &pusb_intf->dev;
	
	RT_TRACE(_module_hci_intfs_c_, _drv_err_, ("+rtw_drv_init\n"));
	//printk("+rtw_drv_init\n");

	//2009.8.13, by Thomas
	// In this probe function, O.S. will provide the usb interface pointer to driver.
	// We have to increase the reference count of the usb device structure by using the usb_get_dev function.
	usb_get_dev(interface_to_usbdev(pusb_intf));

	pintf = pusb_intf;	


#ifdef CONFIG_80211N_HT
	//step 0.
	disable_ht_for_spec_devid(pdid);
#endif

	//step 1. set USB interface data
	// init data
	pnetdev = rtw_init_netdev();
	if (!pnetdev) goto error;
	SET_NETDEV_DEV(pnetdev, &pusb_intf->dev);

	padapter = netdev_priv(pnetdev);
	pdvobjpriv = &padapter->dvobjpriv;
	pdvobjpriv->padapter = padapter;
	pdvobjpriv->pusbintf = pusb_intf ;
	pdvobjpriv->pusbdev = interface_to_usbdev(pusb_intf);

	// set data
	usb_set_intfdata(pusb_intf, pnetdev);

	//step 1-1., decide the chip_type via vid/pid
	decide_chip_type_by_usb_device_id(padapter, pdid);

#ifdef CONFIG_AUTOSUSPEND
	if( padapter->registrypriv.power_mgnt != PS_MODE_ACTIVE )
	{
		if(padapter->pwrctrlpriv.bSupportRemoteWakeup)
		{
			pusb_intf->needs_remote_wakeup = 1;		
			device_init_wakeup(&pusb_intf->dev, 1);
		}
		
		if(padapter->registrypriv.usbss_enable ){ 	/* autosuspend (2s delay) */
			pdvobjpriv->pusbdev->autosuspend_delay = 0 * HZ;//15 * HZ; idle-delay time		 	

			#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,35))
			usb_enable_autosuspend(padapter->dvobjpriv.pusbdev);
			#endif

			#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,22) && LINUX_VERSION_CODE<=KERNEL_VERSION(2,6,34))
			padapter->bDisableAutosuspend = padapter->dvobjpriv.pusbdev->autosuspend_disabled ;
			padapter->dvobjpriv.pusbdev->autosuspend_disabled = 0;//autosuspend disabled by the user
			#endif

			usb_autopm_get_interface(padapter->dvobjpriv.pusbintf );//init pm_usage_cnt ,let it start from 1

			#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,32))
			printk("%s...pm_usage_cnt(%d).....\n",__FUNCTION__,atomic_read(&(pdvobjpriv->pusbintf ->pm_usage_cnt)));
			#else
			printk("%s...pm_usage_cnt(%d).....\n",__FUNCTION__,pdvobjpriv->pusbintf ->pm_usage_cnt);
			#endif							
		}
	}	
#endif

	//step 2.	
	if(padapter->chip_type == RTL8188C_8192C)
	{
#ifdef CONFIG_RTL8192C
		rtl8192cu_set_hal_ops(padapter);
#endif

	}
	else if(padapter->chip_type == RTL8192D)
	{

#ifdef CONFIG_RTL8192D
		rtl8192cu_set_hal_ops(padapter);
#endif
		
	}
	else
	{
		status = _FAIL;
		goto error;
	}
		

	//step 3.
	//initialize the dvobj_priv
	status = usb_dvobj_init(padapter);	
	if (status != _SUCCESS) {
		RT_TRACE(_module_hci_intfs_c_, _drv_err_, ("initialize device object priv Failed!\n"));
		goto error;
	}


	//step 4.
	status = rtw_init_drv_sw(padapter);
	if(status ==_FAIL){
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("Initialize driver software resource Failed!\n"));
		goto error;
	}


	//step 5. read efuse/eeprom data and get mac_addr
	intf_read_chip_info(padapter);	

	if ( rtw_initmac )
	{	//	Users specify the mac address
		int jj,kk;

		for( jj = 0, kk = 0; jj < ETH_ALEN; jj++, kk += 3 )
		{
			mac[jj] = key_2char2num(rtw_initmac[kk], rtw_initmac[kk+ 1]);
		}
	}
	else
	{	//	Use the mac address stored in the Efuse
		_rtw_memcpy(mac, padapter->eeprompriv.mac_addr, ETH_ALEN);
	}

	if (((mac[0]==0xff) &&(mac[1]==0xff) && (mac[2]==0xff) &&
	     (mac[3]==0xff) && (mac[4]==0xff) &&(mac[5]==0xff)) ||
	    ((mac[0]==0x0) && (mac[1]==0x0) && (mac[2]==0x0) &&
	     (mac[3]==0x0) && (mac[4]==0x0) &&(mac[5]==0x0)))
	{
		mac[0] = 0x00;
		mac[1] = 0xe0;
		mac[2] = 0x4c;
		mac[3] = 0x87;
		mac[4] = 0x00;
		mac[5] = 0x00;
	}
	_rtw_memcpy(pnetdev->dev_addr, mac, ETH_ALEN);
	printk("MAC Address from efuse= %02x:%02x:%02x:%02x:%02x:%02x\n",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);


	//step 6.
	/* Tell the network stack we exist */
	if (register_netdev(pnetdev) != 0) {
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("register_netdev() failed\n"));
		goto error;
	}

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-drv_init - Adapter->bDriverStopped=%d, Adapter->bSurpriseRemoved=%d\n",padapter->bDriverStopped, padapter->bSurpriseRemoved));
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-871x_drv - drv_init, success!\n"));
	//printk("-871x_drv - drv_init, success!\n");


#ifdef RTK_DMP_PLATFORM
	rtw_proc_init_one(pnetdev);
#endif	

#ifdef CONFIG_HOSTAPD_MLME
	hostapd_mode_init(padapter);
#endif

#ifdef CONFIG_PLATFORM_RTD2880B
	printk("wlan link up\n");
	//rtd2885_wlan_netlink_sendMsg("linkup", "8712");
#endif

	return 0;

error:

	usb_put_dev(interface_to_usbdev(pusb_intf));//decrease the reference count of the usb device structure if driver fail on initialzation

	usb_set_intfdata(pusb_intf, NULL);

	usb_dvobj_deinit(padapter);
	
	if (pnetdev)
	{
		//unregister_netdev(pnetdev);
		free_netdev(pnetdev);
	}

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-871x_usb - drv_init, fail!\n"));
	//printk("-871x_usb - drv_init, fail!\n");

	return -ENODEV;
}
#ifdef CONFIG_IPS

int r871xu_ips_pwr_up(_adapter *padapter)
{	
	int result;
	printk("===>  r871xu_ips_pwr_up..............\n");
	reset_drv_sw(padapter);
	result = ips_netdrv_open(padapter);
 	printk("<===  r871xu_ips_pwr_up..............\n");
	return result;

}

void r871xu_ips_pwr_down(_adapter *padapter)
{
	printk("===> r871xu_ips_pwr_down...................\n");

	padapter->bCardDisableWOHSM = _TRUE;
	padapter->net_closed = _TRUE;

	padapter->ledpriv.LedControlHandler(padapter, LED_CTL_NO_LINK);
	
	ips_dev_unload(padapter);
	padapter->bCardDisableWOHSM = _FALSE;
	printk("<=== r871xu_ips_pwr_down.....................\n");
}
#endif
/*
 * dev_remove() - our device is being removed
*/
//rmmod module & unplug(SurpriseRemoved) will call r871xu_dev_remove() => how to recognize both
static void rtw_dev_remove(struct usb_interface *pusb_intf)
{
	struct net_device *pnetdev=usb_get_intfdata(pusb_intf);
	_adapter *padapter = (_adapter*)netdev_priv(pnetdev);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
_func_exit_;

	usb_set_intfdata(pusb_intf, NULL);

	if(padapter)
	{
		printk("+rtw_dev_remove\n");
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+dev_remove()\n"));

#ifdef CONFIG_HOSTAPD_MLME
		hostapd_mode_unload(padapter);
#endif
		
		if(drvpriv.drv_registered == _TRUE)
		{
			//printk("r871xu_dev_remove():padapter->bSurpriseRemoved == _TRUE\n");
			padapter->bSurpriseRemoved = _TRUE;
		}
		/*else
		{
			//printk("r871xu_dev_remove():module removed\n");
			padapter->hw_init_completed = _FALSE;
		}*/
		LeaveAllPowerSaveMode(padapter);

		#ifdef CONFIG_AUTOSUSPEND
		if(padapter->registrypriv.usbss_enable)
		{
			#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,35))
			usb_disable_autosuspend(padapter->dvobjpriv.pusbdev);
			#else
			padapter->dvobjpriv.pusbdev->autosuspend_disabled =  padapter->bDisableAutosuspend;// 1;//autosuspend disabled by the user
			#endif
		}
		#endif

		if(pnetdev)
		{
			unregister_netdev(pnetdev); //will call netdev_close()

#ifdef CONFIG_PROC_DEBUG
			rtw_proc_remove_one(pnetdev);
#endif
			
		}	

		cancel_all_timer(padapter);

		rtw_dev_unload(padapter);

		printk("+r871xu_dev_remove, hw_init_completed=%d\n", padapter->hw_init_completed);

		rtw_free_drv_sw(padapter);

		//after rtw_free_drv_sw(), padapter has beed freed, don't refer to it.
		
	}

	usb_put_dev(interface_to_usbdev(pusb_intf));//decrease the reference count of the usb device structure when disconnect

	//If we didn't unplug usb dongle and remove/insert modlue, driver fails on sitesurvey for the first time when device is up . 
	//Reset usb port for sitesurvey fail issue. 2009.8.13, by Thomas
	if(interface_to_usbdev(pusb_intf)->state != USB_STATE_NOTATTACHED)
	{
		printk("usb attached..., try to reset usb device\n");
		usb_reset_device(interface_to_usbdev(pusb_intf));
	}	
	
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-dev_remove()\n"));
	printk("-r871xu_dev_remove, done\n");

#ifdef CONFIG_PLATFORM_RTD2880B
	printk("wlan link down\n");
	//rtd2885_wlan_netlink_sendMsg("linkdown", "8712");
#endif


_func_exit_;

	return;

}


static int __init rtw_drv_entry(void)
{
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtw_drv_entry\n"));
	printk("rtw driver version=%s\n", DRIVERVERSION);		
	drvpriv.drv_registered = _TRUE;
	return usb_register(&drvpriv.rtw_usb_drv);
}

static void __exit rtw_drv_halt(void)
{
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtw_drv_halt\n"));
	printk("+rtw_drv_halt\n");
	drvpriv.drv_registered = _FALSE;
	usb_deregister(&drvpriv.rtw_usb_drv);
	printk("-rtw_drv_halt\n");
}


module_init(rtw_drv_entry);
module_exit(rtw_drv_halt);


/*
init (driver module)-> r8712u_drv_entry
probe (sd device)-> r871xu_drv_init(dev_init)
open (net_device) ->netdev_open
close (net_device) ->netdev_close
remove (sd device) ->r871xu_dev_remove
exit (driver module)-> r8712u_drv_halt
*/


/*
r8711s_drv_entry()
r8711u_drv_entry()
r8712s_drv_entry()
r8712u_drv_entry()
*/

