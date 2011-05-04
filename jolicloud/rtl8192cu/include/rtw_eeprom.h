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
#ifndef __RTL871X_EEPROM_H__
#define __RTL871X_EEPROM_H__

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#define	RTL8712_EEPROM_ID			0x8712
#define	EEPROM_MAX_SIZE			256
#define	CLOCK_RATE					50			//100us		

//- EEPROM opcodes
#define EEPROM_READ_OPCODE		06
#define EEPROM_WRITE_OPCODE		05
#define EEPROM_ERASE_OPCODE		07
#define EEPROM_EWEN_OPCODE		19      // Erase/write enable
#define EEPROM_EWDS_OPCODE		16      // Erase/write disable

//Country codes
#define USA							0x555320
#define EUROPE						0x1 //temp, should be provided later	
#define JAPAN						0x2 //temp, should be provided later

#ifdef CONFIG_SDIO_HCI
#define eeprom_cis0_sz	17
#define eeprom_cis1_sz	50
#endif

#define	EEPROM_CID_DEFAULT			0x0
#define	EEPROM_CID_ALPHA				0x1
#define	EEPROM_CID_Senao				0x3
#define	EEPROM_CID_NetCore				0x5
#define	EEPROM_CID_CAMEO				0X8
#define	EEPROM_CID_SITECOM				0x9
#define	EEPROM_CID_COREGA				0xB
#define	EEPROM_CID_EDIMAX_BELKIN		0xC
#define	EEPROM_CID_SERCOMM_BELKIN		0xE
#define	EEPROM_CID_CAMEO1				0xF
#define	EEPROM_CID_WNC_COREGA		0x12
#define	EEPROM_CID_CLEVO				0x13
#define	EEPROM_CID_WHQL				0xFE // added by chiyoko for dtm, 20090108

typedef enum _RT_CUSTOMER_ID
{
	RT_CID_DEFAULT = 0,
	RT_CID_8187_ALPHA0 = 1,
	RT_CID_8187_SERCOMM_PS = 2,
	RT_CID_8187_HW_LED = 3,
	RT_CID_8187_NETGEAR = 4,
	RT_CID_WHQL = 5,
	RT_CID_819x_CAMEO  = 6, 
	RT_CID_819x_RUNTOP = 7,
	RT_CID_819x_Senao = 8,
	RT_CID_TOSHIBA = 9,	// Merge by Jacken, 2008/01/31.
	RT_CID_819x_Netcore = 10,
	RT_CID_Nettronix = 11,
	RT_CID_DLINK = 12,
	RT_CID_PRONET = 13,
	RT_CID_COREGA = 14,
	RT_CID_819x_ALPHA = 15,
	RT_CID_819x_Sitecom = 16,
	RT_CID_CCX = 17, // It's set under CCX logo test and isn't demanded for CCX functions, but for test behavior like retry limit and tx report. By Bruce, 2009-02-17.      
	RT_CID_819x_Lenovo = 18,
	RT_CID_819x_QMI = 19,
	RT_CID_819x_Edimax_Belkin = 20,		
	RT_CID_819x_Sercomm_Belkin = 21,			
	RT_CID_819x_CAMEO1 = 22,
	RT_CID_819x_MSI = 23,
	RT_CID_819x_Acer = 24,
	RT_CID_819x_AzWave_ASUS = 25,
	RT_CID_819x_AzWave = 26, // For AzWave in PCIe, The ID is AzWave use and not only Asus
	RT_CID_819x_HP = 27,
	RT_CID_819x_WNC_COREGA = 28,
	RT_CID_819x_Arcadyan_Belkin = 29,
	RT_CID_819x_SAMSUNG = 30,
	RT_CID_819x_CLEVO = 31,
	RT_CID_819x_DELL = 32,
	RT_CID_819x_PRONETS = 33,
}RT_CUSTOMER_ID, *PRT_CUSTOMER_ID;

struct eeprom_priv 
{    
	u8		bAutoload;
	u8		bempty;
	u8		sys_config;
	u8		mac_addr[6];	//PermanentAddress
	u8		config0;
	u16		channel_plan;
	u8		country_string[3];	
	u8		tx_power_b[15];
	u8		tx_power_g[15];
	u8		tx_power_a[201];

	u8		bBootFromEEPROM;
	
	u8		efuse_eeprom_data[EEPROM_MAX_SIZE];
	u16		efuse_phy_max_size;
#ifdef CONFIG_SDIO_HCI
	u8		sdio_setting;	
	u32		ocr;
	u8		cis0[eeprom_cis0_sz];
	u8		cis1[eeprom_cis1_sz];	
#endif


#ifdef CONFIG_RTL8192C
	//
	// EEPROM setting.
	//
	u8				EEPROMVersion;
	u16				EEPROMVID;
	u16				EEPROMPID;
	u16				EEPROMSVID;
	u16				EEPROMSDID;
	u8				EEPROMCustomerID;
	u8				EEPROMSubCustomerID;	
	u8				EEPROMRegulatory;//ChannelPlan

	u8	 			EEPROMUsbOption;
	u8				EEPROMUsbEndPointNumber;
	u8	 			EEPROMUsbPhyParam[5];

	u8  				EEPROMThermalMeter;
	u8  				EEPROMTSSI_A;
	u8  				EEPROMTSSI_B;

	u8				EEPROMBoardType;	


	//
	// The same as 92CE. May merge 92CU and 92CE into the other struct
	//
	//u8				ThermalMeter[2];	// ThermalMeter, index 0 for RFIC0, and 1 for RFIC1
	u8				TxPwrLevelCck[RF90_PATH_MAX][CHANNEL_MAX_NUMBER];
	u8				TxPwrLevelHT40_1S[RF90_PATH_MAX][CHANNEL_MAX_NUMBER];	// For HT 40MHZ pwr
	u8				TxPwrLevelHT40_2S[RF90_PATH_MAX][CHANNEL_MAX_NUMBER];	// For HT 40MHZ pwr	
	u8				TxPwrHt20Diff[RF90_PATH_MAX][CHANNEL_MAX_NUMBER];// HT 20<->40 Pwr diff
	u8				TxPwrLegacyHtDiff[RF90_PATH_MAX][CHANNEL_MAX_NUMBER];// For HT<->legacy pwr diff
	// For power group
	u8				PwrGroupHT20[RF90_PATH_MAX][CHANNEL_MAX_NUMBER];
	u8				PwrGroupHT40[RF90_PATH_MAX][CHANNEL_MAX_NUMBER];
	

	
	//chip info from eeprom or efuse	
	//unsigned char hw_addr[6];
	//unsigned short chip_id;//EEPROMId	?
	//unsigned char EEPROMUsbOption;
	//unsigned char EEPROMUsbPhyParam[5];
	//unsigned char EEPROMUsbEndPointNumber;
	//unsigned char EEPROMVersion;
	//unsigned char eeprom_ChannelPlan;
	//unsigned char eeprom_CustomerID;
	//unsigned char eeprom_SubCustomerID;
	//unsigned char EEPROMBoardType;	
	//unsigned char RfCckChnlAreaTxPwr[2][3];//RF-A&B CCK/OFDM Tx Power Level at three channel are [1-3] [4-9] [10-14]
	//unsigned char RfOfdmChnlAreaTxPwr1T[2][3];	
	//unsigned char RfOfdmChnlAreaTxPwr2T[2][3];	
	//unsigned char EEPROMTxPowerDiff;
	//unsigned char EEPROMThermalMeter;
	//unsigned char EEPROMTSSI_A;
	//unsigned char EEPROMTSSI_B;	
	//unsigned char TxPwrHt20Diff[2][14];	// HT 20<->40 Pwr diff
	//unsigned char TxPwrLegacyHtDiff[2][14];	// For HT<->legacy OFDM pwr diff
	//unsigned char EEPROMPwrGroup[2][3];		
	//unsigned char EEPROMRegulatory;
	//unsigned char EEPROMOptional;
	
#endif //end of CONFIG_RTL8192C

};


extern void eeprom_write16(_adapter *padapter, u16 reg, u16 data);
extern u16 eeprom_read16(_adapter *padapter, u16 reg);
extern void read_eeprom_content(_adapter *padapter);
extern void eeprom_read_sz(_adapter * padapter, u16 reg,u8* data, u32 sz); 

extern void read_eeprom_content_by_attrib(_adapter *	padapter	);

#endif  //__RTL871X_EEPROM_H__
