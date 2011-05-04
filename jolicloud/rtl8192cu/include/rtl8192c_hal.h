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
#ifndef __RTL8192C_HAL_H__
#define __RTL8192C_HAL_H__

#include "Hal8192CPhyReg.h"
#include "Hal8192CPhyCfg.h"
#include "rtl8192c_dm.h"

#if (DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE)

	#define RTL819X_DEFAULT_RF_TYPE			RF_2T2R
	//#define RTL819X_DEFAULT_RF_TYPE			RF_1T2R
	#define RTL819X_TOTAL_RF_PATH				2

	//2TODO:  The following need to check!!
	#define	RTL8192C_FW_IMG					"rtl8192CE\\rtl8192cfw.bin"
	#define	RTL8188C_FW_IMG					"rtl8192CE\\rtl8192cfw.bin"

	#define RTL8188C_PHY_REG					"rtl8192CE\\PHY_REG_1T.txt"
	#define RTL8188C_PHY_RADIO_A				"rtl8192CE\\radio_a_1T.txt"
	#define RTL8188C_PHY_RADIO_B				"rtl8192CE\\radio_b_1T.txt"
	#define RTL8188C_AGC_TAB					"rtl8192CE\\AGC_TAB_1T.txt"
	#define RTL8188C_PHY_MACREG				"rtl8192CE\\MACREG_1T.txt"

	#define RTL8192C_PHY_REG					"rtl8192CE\\PHY_REG_2T.txt"
	#define RTL8192C_PHY_RADIO_A				"rtl8192CE\\radio_a_2T.txt"
	#define RTL8192C_PHY_RADIO_B				"rtl8192CE\\radio_b_2T.txt"
	#define RTL8192C_AGC_TAB					"rtl8192CE\\AGC_TAB_2T.txt"
	#define RTL8192C_PHY_MACREG				"rtl8192CE\\MACREG_2T.txt"

	#define RTL819X_PHY_MACPHY_REG			"rtl8192CE\\MACPHY_reg.txt"
	#define RTL819X_PHY_MACPHY_REG_PG		"rtl8192CE\\MACPHY_reg_PG.txt"
	#define RTL819X_PHY_MACREG				"rtl8192CE\\MAC_REG.txt"
	#define RTL819X_PHY_REG					"rtl8192CE\\PHY_REG.txt"
	#define RTL819X_PHY_REG_1T2R				"rtl8192CE\\PHY_REG_1T2R.txt"
	#define RTL819X_PHY_REG_to1T1R				"rtl8192CE\\phy_to1T1R_a.txt"
	#define RTL819X_PHY_REG_to1T2R				"rtl8192CE\\phy_to1T2R.txt"
	#define RTL819X_PHY_REG_to2T2R				"rtl8192CE\\phy_to2T2R.txt"
	#define RTL819X_PHY_REG_PG					"rtl8192CE\\PHY_REG_PG.txt"
	#define RTL819X_AGC_TAB					"rtl8192CE\\AGC_TAB.txt"
	#define RTL819X_PHY_RADIO_A				"rtl8192CE\\radio_a.txt"
	#define RTL819X_PHY_RADIO_A_1T			"rtl8192CE\\radio_a_1t.txt"
	#define RTL819X_PHY_RADIO_A_2T			"rtl8192CE\\radio_a_2t.txt"
	#define RTL819X_PHY_RADIO_B				"rtl8192CE\\radio_b.txt"
	#define RTL819X_PHY_RADIO_B_GM			"rtl8192CE\\radio_b_gm.txt"
	#define RTL819X_PHY_RADIO_C				"rtl8192CE\\radio_c.txt"
	#define RTL819X_PHY_RADIO_D				"rtl8192CE\\radio_d.txt"
	#define RTL819X_EEPROM_MAP				"rtl8192CE\\8192ce.map"
	#define RTL819X_EFUSE_MAP					"rtl8192CE\\8192ce.map"

	// The file name "_2T" is for 92CE, "_1T"  is for 88CE. Modified by tynli. 2009.11.24.
	#define Rtl819XFwImageArray				Rtl8192CEFwImgArray
	#define Rtl819XMAC_Array					Rtl8192CEMAC_2T_Array
	#define Rtl819XAGCTAB_2TArray				Rtl8192CEAGCTAB_2TArray
	#define Rtl819XAGCTAB_1TArray				Rtl8192CEAGCTAB_1TArray
	#define Rtl819XPHY_REG_2TArray			Rtl8192CEPHY_REG_2TArray
	#define Rtl819XPHY_REG_1TArray			Rtl8192CEPHY_REG_1TArray
	#define Rtl819XRadioA_2TArray				Rtl8192CERadioA_2TArray
	#define Rtl819XRadioA_1TArray				Rtl8192CERadioA_1TArray
	#define Rtl819XRadioB_2TArray				Rtl8192CERadioB_2TArray
	#define Rtl819XRadioB_1TArray				Rtl8192CERadioB_1TArray
	#define Rtl819XPHY_REG_Array_PG 			Rtl8192CEPHY_REG_Array_PG

#elif (DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)

	#include "Hal8192CUHWImg.h"

	//2TODO: We should define 8192S firmware related macro settings here!!
	#define RTL819X_DEFAULT_RF_TYPE			RF_1T2R
	#define RTL819X_TOTAL_RF_PATH			2

	//TODO:  The following need to check!!
	#define	RTL8192C_FW_TSMC_IMG				"rtl8192CU\\rtl8192cfwT.bin"
	#define	RTL8192C_FW_UMC_IMG				"rtl8192CU\\rtl8192cfwU.bin"
	#define	RTL8723_FW_UMC_IMG				"rtl8192CU\\rtl8723fw.bin"
	
	//#define RTL819X_FW_BOOT_IMG   				"rtl8192CU\\boot.img"
	//#define RTL819X_FW_MAIN_IMG				"rtl8192CU\\main.img"
	//#define RTL819X_FW_DATA_IMG				"rtl8192CU\\data.img"

	#define RTL8188C_PHY_REG					"rtl8188CU\\PHY_REG.txt"
	#define RTL8188C_PHY_RADIO_A				"rtl8188CU\\radio_a.txt"
	#define RTL8188C_PHY_RADIO_B				"rtl8188CU\\radio_b.txt"
	#define RTL8188C_PHY_RADIO_A_mCard			"rtl8192CU\\radio_a_1T_mCard.txt"
	#define RTL8188C_PHY_RADIO_B_mCard			"rtl8192CU\\radio_b_1T_mCard.txt" 
	#define RTL8188C_PHY_RADIO_A_HP			"rtl8192CU\\radio_a_1T_HP.txt"
	#define RTL8188C_AGC_TAB					"rtl8188CU\\AGC_TAB.txt"
	#define RTL8188C_PHY_MACREG				"rtl8188CU\\MACREG.txt"

	#define RTL8192C_PHY_REG					"rtl8192CU\\PHY_REG.txt"
	#define RTL8192C_PHY_RADIO_A				"rtl8192CU\\radio_a.txt"
	#define RTL8192C_PHY_RADIO_B				"rtl8192CU\\radio_b.txt"
	#define RTL8192C_AGC_TAB					"rtl8192CU\\AGC_TAB.txt"
	#define RTL8192C_PHY_MACREG				"rtl8192CU\\MACREG.txt"

	#define RTL819X_PHY_REG_PG				"rtl8192CU\\PHY_REG_PG.txt"
#if 0
	#define RTL819X_PHY_MACPHY_REG			"rtl8192CU\\MACPHY_reg.txt"
	#define RTL819X_PHY_MACPHY_REG_PG		"rtl8192CU\\MACPHY_reg_PG.txt"
	#define RTL819X_PHY_MACREG				"rtl8192CU\\MAC_REG.txt"
	#define RTL819X_PHY_REG					"rtl8192CU\\PHY_REG.txt"
	#define RTL819X_PHY_REG_1T2R				"rtl8192CU\\PHY_REG_1T2R.txt"
	#define RTL819X_PHY_REG_to1T1R			"rtl8192CU\\phy_to1T1R_a.txt"
	#define RTL819X_PHY_REG_to1T2R			"rtl8192CU\\phy_to1T2R.txt"
	#define RTL819X_PHY_REG_to2T2R			"rtl8192CU\\phy_to2T2R.txt"
	//#define RTL819X_PHY_REG_PG				"rtl8192CU\\PHY_REG_PG.txt"
	#define RTL819X_AGC_TAB					"rtl8192CU\\AGC_TAB.txt"
	#define RTL819X_PHY_RADIO_A				"rtl8192CU\\radio_a.txt"
	#define RTL819X_PHY_RADIO_B				"rtl8192CU\\radio_b.txt"
	#define RTL819X_PHY_RADIO_B_GM			"rtl8192CU\\radio_b_gm.txt"
	#define RTL819X_PHY_RADIO_C				"rtl8192CU\\radio_c.txt"
	#define RTL819X_PHY_RADIO_D				"rtl8192CU\\radio_d.txt"
	#define RTL819X_EEPROM_MAP				"rtl8192CU\\8192cu.map"
	#define RTL819X_EFUSE_MAP					"rtl8192CU\\8192cu.map"
	#define RTL819X_PHY_RADIO_A_1T			"rtl8192CU\\radio_a_1t.txt"
	#define RTL819X_PHY_RADIO_A_2T			"rtl8192CU\\radio_a_2t.txt"
#endif

	// The file name "_2T" is for 92CU, "_1T"  is for 88CU. Modified by tynli. 2009.11.24.
	#define Rtl819XFwImageArray					Rtl8192CUFwTSMCImgArray
	#define Rtl819XFwTSMCImageArray			Rtl8192CUFwTSMCImgArray
	#define Rtl819XFwUMCImageArray				Rtl8192CUFwUMCImgArray
			
	#define Rtl819XMAC_Array					Rtl8192CUMAC_2T_Array
	#define Rtl819XAGCTAB_2TArray					Rtl8192CUAGCTAB_2TArray
	#define Rtl819XAGCTAB_1TArray					Rtl8192CUAGCTAB_1TArray
	#define Rtl819XAGCTAB_1T_HPArray			Rtl8192CUAGCTAB_1T_HPArray
	#define Rtl819XPHY_REG_2TArray				Rtl8192CUPHY_REG_2TArray
	#define Rtl819XPHY_REG_1TArray				Rtl8192CUPHY_REG_1TArray
	#define Rtl819XPHY_REG_1T_mCardArray		Rtl8192CUPHY_REG_1T_mCardArray 					
	#define Rtl819XPHY_REG_2T_mCardArray		Rtl8192CUPHY_REG_2T_mCardArray	
	#define Rtl819XPHY_REG_1T_HPArray			Rtl8192CUPHY_REG_1T_HPArray
	#define Rtl819XRadioA_2TArray					Rtl8192CURadioA_2TArray
	#define Rtl819XRadioA_1TArray					Rtl8192CURadioA_1TArray
	#define Rtl819XRadioA_1T_mCardArray			Rtl8192CURadioA_1T_mCardArray			
	#define Rtl819XRadioB_2TArray					Rtl8192CURadioB_2TArray
	#define Rtl819XRadioB_1TArray					Rtl8192CURadioB_1TArray	
	#define Rtl819XRadioB_1T_mCardArray			Rtl8192CURadioB_1T_mCardArray
	#define Rtl819XRadioA_1T_HPArray			Rtl8192CURadioA_1T_HPArray	
	#define Rtl819XPHY_REG_Array_PG 			Rtl8192CUPHY_REG_Array_PG
	#define Rtl819XPHY_REG_Array_PG_mCard 		Rtl8192CUPHY_REG_Array_PG_mCard			
	#define Rtl819XPHY_REG_Array_PG_HP			Rtl8192CUPHY_REG_Array_PG_HP		
		
#endif


enum RTL871X_HCI_TYPE {

	RTL8192C_SDIO,
	RTL8192C_USB,
	RTL8192C_PCIE
};

#define PageNum_128(_Len)		(u32)(((_Len)>>7) + ((_Len)&0x7F ? 1:0))

#define FW_8192C_SIZE					16384//16k
#define FW_8192C_START_ADDRESS		0x1000
#define FW_8192C_END_ADDRESS		0x3FFF

#define MAX_PAGE_SIZE			4096	// @ page : 4k bytes

#define IS_FW_HEADER_EXIST(_pFwHdr)	((le16_to_cpu(_pFwHdr->Signature)&0xFFF0) == 0x92C0 ||\
									(le16_to_cpu(_pFwHdr->Signature)&0xFFF0) == 0x88C0)

typedef enum _FIRMWARE_SOURCE{
	FW_SOURCE_IMG_FILE = 0,
	FW_SOURCE_HEADER_FILE = 1,		//from header file
}FIRMWARE_SOURCE, *PFIRMWARE_SOURCE;

typedef struct _RT_FIRMWARE{
	FIRMWARE_SOURCE	eFWSource;
	u8			szFwBuffer[FW_8192C_SIZE];
	u32			ulFwLength;
}RT_FIRMWARE, *PRT_FIRMWARE, RT_FIRMWARE_92C, *PRT_FIRMWARE_92C;

//
// This structure must be cared byte-ordering
//
// Added by tynli. 2009.12.04.
typedef struct _RT_8192C_FIRMWARE_HDR {//8-byte alinment required

	//--- LONG WORD 0 ----
	u16		Signature;	// 92C0: test chip; 92C, 88C0: test chip; 88C1: MP A-cut; 92C1: MP A-cut
	u8		Category;	// AP/NIC and USB/PCI
	u8		Function;	// Reserved for different FW function indcation, for further use when driver needs to download different FW in different conditions
	u16		Version;		// FW Version
	u8		Subversion;	// FW Subversion, default 0x00
	u16		Rsvd1;


	//--- LONG WORD 1 ----
	u8		Month;	// Release time Month field
	u8		Date;	// Release time Date field
	u8		Hour;	// Release time Hour field
	u8		Minute;	// Release time Minute field
	u16		RamCodeSize;	// The size of RAM code
	u16		Rsvd2;

	//--- LONG WORD 2 ----
	u32		SvnIdx;	// The SVN entry index
	u32		Rsvd3;

	//--- LONG WORD 3 ----
	u32		Rsvd4;
	u32		Rsvd5;

}RT_8192C_FIRMWARE_HDR, *PRT_8192C_FIRMWARE_HDR;

#define DRIVER_EARLY_INT_TIME		0x05
#define BCN_DMA_ATIME_INT_TIME		0x02

#define USB_HIGH_SPEED_BULK_SIZE	512
#define USB_FULL_SPEED_BULK_SIZE	64

#if USB_RX_AGGREGATION_92C

typedef enum _USB_RX_AGG_MODE{
	USB_RX_AGG_DISABLE,
	USB_RX_AGG_DMA,
	USB_RX_AGG_USB,
	USB_RX_AGG_MIX
}USB_RX_AGG_MODE;

#define MAX_RX_DMA_BUFFER_SIZE	10240		// 10K for 8192C RX DMA buffer

#endif


#define TX_SELE_HQ			BIT(0)		// High Queue
#define TX_SELE_LQ			BIT(1)		// Low Queue
#define TX_SELE_NQ			BIT(2)		// Normal Queue


// Note: We will divide number of page equally for each queue other than public queue!

#define TX_TOTAL_PAGE_NUMBER		0xF8
#define TX_PAGE_BOUNDARY		(TX_TOTAL_PAGE_NUMBER + 1)

// For Normal Chip Setting
// (HPQ + LPQ + NPQ + PUBQ) shall be TX_TOTAL_PAGE_NUMBER
//#define NORMAL_PAGE_NUM_PUBQ		0x56
//#define NORMAL_PAGE_NUM_PUBQ		0xb0
#define NORMAL_PAGE_NUM_PUBQ		0xE7


// For Test Chip Setting
// (HPQ + LPQ + PUBQ) shall be TX_TOTAL_PAGE_NUMBER
#define TEST_PAGE_NUM_PUBQ		0x7E


// For Test Chip Setting
#define WMM_TEST_TX_TOTAL_PAGE_NUMBER	0xF5
#define WMM_TEST_TX_PAGE_BOUNDARY	(WMM_TEST_TX_TOTAL_PAGE_NUMBER + 1) //F6

#define WMM_TEST_PAGE_NUM_PUBQ		0xA3
#define WMM_TEST_PAGE_NUM_HPQ		0x29
#define WMM_TEST_PAGE_NUM_LPQ		0x29


//Note: For Normal Chip Setting ,modify later
#define WMM_NORMAL_TX_TOTAL_PAGE_NUMBER	0xF5
#define WMM_NORMAL_TX_PAGE_BOUNDARY	(WMM_TEST_TX_TOTAL_PAGE_NUMBER + 1) //F6

#define WMM_NORMAL_PAGE_NUM_PUBQ	0xB0
#define WMM_NORMAL_PAGE_NUM_HPQ		0x29
#define WMM_NORMAL_PAGE_NUM_LPQ		0x1C
#define WMM_NORMAL_PAGE_NUM_NPQ		0x1C

//-------------------------------------------------------------------------
//	Chip specific
//-------------------------------------------------------------------------
#define CHIP_92C  					BIT(0)
#define CHIP_92C_1T2R				BIT(1)
#define CHIP_8723					BIT(2) // RTL8723 With BT feature
#define CHIP_8723_DRV_REV			BIT(3) // RTL8723 Driver Revised
#define NORMAL_CHIP  				BIT(4)
#define CHIP_VENDOR_UMC			BIT(5)
#define CHIP_VENDOR_UMC_B_CUT		BIT(6) // Chip version for ECO

#define IS_NORMAL_CHIP(version)  	(((version) & NORMAL_CHIP) ? _TRUE : _FALSE) 
#define IS_92C_SERIAL(version)   		(((version) & CHIP_92C) ? _TRUE : _FALSE)
#define IS_8723_SERIES(version)   	(((version) & CHIP_8723) ? _TRUE : _FALSE)
#define IS_92C_1T2R(version)			(((version) & CHIP_92C) && ((version) & CHIP_92C_1T2R))
#define IS_VENDOR_UMC(version)		(((version) & CHIP_VENDOR_UMC) ? _TRUE : _FALSE)
#define IS_VENDOR_UMC_A_CUT(version)	(((version) & CHIP_VENDOR_UMC) ? (((version) & (BIT6|BIT7)) ? _FALSE : _TRUE) : _FALSE)

#define IS_VENDOR_8723_A_CUT(version)	(((version) & CHIP_VENDOR_UMC) ? (((version) & (BIT6)) ? _FALSE : _TRUE) : _FALSE)

// 20100707 Joseph: Add vendor information into chip version definition.
// 20100902 Roger: Add UMC B-Cut and RTL8723 chip info definition.
/*
|    BIT 7   |     BIT6     |                BIT 5                | BIT 4              |       BIT 3     |  BIT 2   |  BIT 1   |   BIT 0    |
+--------+---------+---------------------- +------------ +----------- +------ +-----------------+
|Reserved | UMC BCut |Manufacturer(TSMC/UMC)  | TEST/NORMAL | 8723 Version | 8723?   | 1T2R?  | 88C/92C |
*/
typedef enum _VERSION_8192C{
	VERSION_TEST_CHIP_88C = 0x00,
	VERSION_TEST_CHIP_92C = 0x01,
	VERSION_NORMAL_TSMC_CHIP_88C = 0x10,
	VERSION_NORMAL_TSMC_CHIP_92C = 0x11,
	VERSION_NORMAL_TSMC_CHIP_92C_1T2R = 0x13,
	VERSION_NORMAL_UMC_CHIP_88C_A_CUT = 0x30,
	VERSION_NORMAL_UMC_CHIP_92C_A_CUT = 0x31,
	VERSION_NORMAL_UMC_CHIP_92C_1T2R_A_CUT = 0x33,		
	VERSION_NORMA_UMC_CHIP_8723_1T1R_A_CUT = 0x34,
	VERSION_NORMA_UMC_CHIP_8723_1T1R_B_CUT = 0x3c,
	VERSION_NORMAL_UMC_CHIP_88C_B_CUT = 0x70,
	VERSION_NORMAL_UMC_CHIP_92C_B_CUT = 0x71,
	VERSION_NORMAL_UMC_CHIP_92C_1T2R_B_CUT = 0x73,	
}VERSION_8192C,*PVERSION_8192C;

#define CHIP_BONDING_92C_1T2R	0x1
#define CHIP_BONDING_IDENTIFIER(_value)	(((_value)>>22)&0x3)
//-------------------------------------------------------------------------
//	Channel Plan
//-------------------------------------------------------------------------
enum ChannelPlan{
	CHPL_FCC	= 0,
	CHPL_IC		= 1,
	CHPL_ETSI	= 2,
	CHPL_SPAIN	= 3,
	CHPL_FRANCE	= 4,
	CHPL_MKK	= 5,
	CHPL_MKK1	= 6,
	CHPL_ISRAEL	= 7,
	CHPL_TELEC	= 8,
	CHPL_GLOBAL	= 9,
	CHPL_WORLD	= 10,
};

typedef struct _TxPowerInfo{
	u8 CCKIndex[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT40_1SIndex[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT40_2SIndexDiff[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT20IndexDiff[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 OFDMIndexDiff[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT40MaxOffset[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 HT20MaxOffset[RF90_PATH_MAX][CHANNEL_GROUP_MAX];
	u8 TSSI_A;
	u8 TSSI_B;
}TxPowerInfo, *PTxPowerInfo;

struct	_hal_ops {
	u32 (*hal_init)(PADAPTER padapter);
	u32 (*hal_deinit)(PADAPTER padapter);

	u32  (*inirp_init)(PADAPTER adapter);
	u32  (*inirp_deinit)(PADAPTER adapter);

	void (*intf_chip_configure)(PADAPTER Adapter);

	void (*read_adapter_info)(PADAPTER Adapter);

	void (*set_bwmode_handler)(PADAPTER padapter, HT_CHANNEL_WIDTH Bandwidth, u8 Offset);
	void (*set_channel_handler)(PADAPTER padapter, u8 channel);

	void (*process_phy_info)(PADAPTER padapter, void *prframe);
	void	(*hal_dm_watchdog)(PADAPTER Adapter);
};

struct hal_priv
{
	struct _hal_ops	hal_ops;

	unsigned short HardwareType;
	unsigned short VersionID;
	unsigned short CustomerID;

	unsigned short FirmwareVersion;
	unsigned short FirmwareVersionRev;
	unsigned short FirmwareSubVersion;

	//current WIFI_PHY values
	unsigned long ReceiveConfig;
	unsigned char CurrentChannel;
	WIRELESS_MODE CurrentWirelessMode;
	HT_CHANNEL_WIDTH CurrentChannelBW;
	unsigned char nCur40MhzPrimeSC;// Control channel sub-carrier

	//rf_ctrl
	unsigned char rf_chip;
	unsigned char rf_type;
	unsigned char NumTotalRFPath;


	BB_REGISTER_DEFINITION_T	PHYRegDef[4];	//Radio A/B/C/D


	// Read/write are allow for following hardware information variables
	u8					framesync;
	unsigned int			framesyncC34;
	u8					framesyncMonitor;
	u8					DefaultInitialGain[4];
	u8					pwrGroupCnt;
	u32					MCSTxPowerLevelOriginalOffset[7][16];	// 7 gropus of pwr diff by rates
	u32					CCKTxPowerLevelOriginalOffset;
	u8					TxPowerLevelCCK[14];			// CCK channel 1~14
	u8					TxPowerLevelOFDM24G[14];		// OFDM 2.4G channel 1~14
	u8					TxPowerLevelOFDM5G[14];			// OFDM 5G
	u8					AntennaTxPwDiff[3];	// Antenna gain offset, index 0 for B, 1 for C, and 2 for D

	//u8					ThermalMeter[2];	// ThermalMeter, index 0 for RFIC0, and 1 for RFIC1
	u32					AntennaTxPath;					// Antenna path Tx
	u32					AntennaRxPath;					// Antenna path Rx
	u8					BoardType;
	u8					BluetoothCoexist;
	u8					ExternalPA;

	u32					LedControlNum;
	u32					LedControlMode;
	//u32					TxPowerTrackControl;
	u8					b1x1RecvCombine;	// for 1T1R receive combining

	u8					bCCKinCH14;

	//vivi, for tx power tracking, 20080407
	//u2Byte					TSSI_13dBm;
	//u4Byte					Pwr_Track;
	// The current Tx Power Level
	unsigned char	CurrentCckTxPwrIdx;
	unsigned char	CurrentOfdm24GTxPwrIdx;

	unsigned char	LegacyHTTxPowerDiff;// Legacy to HT rate power diff

	unsigned char bCckHighPower;

	unsigned int	RfRegChnlVal[2];

	//
	// The same as 92CE.
	//
	//u8					ThermalMeter[2];	// ThermalMeter, index 0 for RFIC0, and 1 for RFIC1
	u8					ThermalValue;
	u8					ThermalValue_LCK;
	u8					ThermalValue_IQK;
	u8					bRfPiEnable;

	//for APK
	u32					APKoutput[2][2];	//path A/B; output1_1a/output1_2a
	u8					bAPKdone;
	u8					bAPKThermalMeterIgnore;

	//for TxPwrTracking
	int					RegE94;
	int 					RegE9C;
	int					RegEB4;
	int					RegEBC;

	//for IQK
	u32				IQKInitialized;

	unsigned int		ADDA_backup[16];
	unsigned int		IQK_MAC_backup[4];
	u32				IQK_BB_backup[10];
	//RDG enable
	BOOLEAN	 	bRDGEnable;
	u8		bDumpRxPkt;

	//for host message to fw
	u8 LastHMEBoxNum;

#ifdef CONFIG_USB_HCI

	// For 92C USB endpoint setting
	//

	unsigned int UsbBulkOutSize;

	int	RtNumInPipes;	// Number of Rx pipes in the ring, RtInPipe.
	int	RtNumOutPipes;	// Number of Tx pipes in the ring, RtOutPipe.

	unsigned char	OutEpQueueSel;
	unsigned char OutEpNumber;

	unsigned char Queue2EPNum[8];//for out endpoint number mapping

#if USB_TX_AGGREGATION_92C
	unsigned char			UsbTxAggMode;
	unsigned char			UsbTxAggDescNum;
#endif
#if USB_RX_AGGREGATION_92C
	u16				HwRxPageSize;				// Hardware setting
	u32				MaxUsbRxAggBlock;

	USB_RX_AGG_MODE	UsbRxAggMode;
	u8				UsbRxAggBlockCount;			// USB Block count. Block size is 512-byte in hight speed and 64-byte in full speed
	u8				UsbRxAggBlockTimeout;
	u8				UsbRxAggPageCount;			// 8192C DMA page count
	u8				UsbRxAggPageTimeout;
#endif

#endif

	u8 				fw_ractrl;
	u8				RegTxPause;
	u32				RegBcnCtrlVal;
	// Beacon function related global variable.
	u8				RegFwHwTxQCtrl;
	u8				RegReg542;
#ifdef CONFIG_ANTENNA_DIVERSITY
	u8				CurAntenna;	
	u8				AntDivCfg;

	//SW Antenna Switch
	s32				RSSI_sum_A;
	s32				RSSI_sum_B;
	s32				RSSI_cnt_A;
	s32				RSSI_cnt_B;
	BOOLEAN			RSSI_test;
	
#endif
#ifdef CONFIG_BT_COEXIST
	struct btcoexist_priv		bt_coexist;	
#endif


};


int FirmwareDownload92C(IN	PADAPTER Adapter);
void NicIFReadAdapterInfo8192C(PADAPTER Adapter);
void rtl8192c_ReadChipVersion(IN PADAPTER Adapter);



typedef struct hal_priv HAL_DATA_TYPE, *PHAL_DATA_TYPE;
typedef struct eeprom_priv EEPROM_EFUSE_PRIV, *PEEPROM_EFUSE_PRIV;

#define GET_HAL_DATA(priv)	(&priv->halpriv)
#define GET_RF_TYPE(priv)	(GET_HAL_DATA(priv)->rf_type)

#define GET_EEPROM_EFUSE_PRIV(priv)	(&priv->eeprompriv)

#endif

