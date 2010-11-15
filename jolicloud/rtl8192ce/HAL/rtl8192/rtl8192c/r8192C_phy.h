/******************************************************************************
 * Copyright(c) 2008 - 2010 Realtek Corporation. All rights reserved.
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
 /* Check to see if the file has been included already.  */
#ifndef __INC_HAL8192CPHYCFG_H
#define __INC_HAL8192CPHYCFG_H

#include <linux/types.h>

#define PHY_QueryBBReg 			rtl8192_QueryBBReg
#define PHY_QueryRFReg			rtl8192_phy_QueryRFReg
#define	PHY_CheckIsLegalRFPath		rtl8192_phy_CheckIsLegalRFPath

#define RT_CANNOT_IO(dev) 				0
#define GET_RF_TYPE(priv) 				priv->rf_type
#define GET_POWER_SAVE_CONTROL(priv)	(PRT_POWER_SAVE_CONTROL)(&(priv->rtllib->PowerSaveControl))

/*--------------------------Define Parameters-------------------------------*/
#define LOOP_LIMIT				5
#define MAX_STALL_TIME			50		
#define AntennaDiversityValue	0x80	
#define MAX_TXPWR_IDX_NMODE_92S	63
#define Reset_Cnt_Limit			3


#define IQK_MAC_REG_NUM	4

#define PHY_SetMacReg			PHY_SetBBReg


#define	SET_RTL8192SE_RF_SLEEP(_pAdapter)							\
{																	\
	u8		u1bTmp;												\
	u1bTmp = read_nic_byte(_pAdapter, REG_LDOV12D_CTRL);		\
	u1bTmp |= BIT0;													\
	write_nic_byte(_pAdapter, REG_LDOV12D_CTRL, u1bTmp);		\
	write_nic_byte(_pAdapter, REG_SPS_OCP_CFG, 0x0);				\
	write_nic_byte(_pAdapter, TXPAUSE, 0xFF);				\
	write_nic_word(_pAdapter, CMDR, 0x57FC);				\
	delay_us(100);													\
	write_nic_word(_pAdapter, CMDR, 0x77FC);				\
	write_nic_byte(_pAdapter, PHY_CCA, 0x0);				\
	delay_us(10);													\
	write_nic_word(_pAdapter, CMDR, 0x37FC);				\
	delay_us(10);													\
	write_nic_word(_pAdapter, CMDR, 0x77FC);				\
	delay_us(10);													\
	write_nic_word(_pAdapter, CMDR, 0x57FC);				\
}

#if 0
#define	SET_RTL8192SE_RF_HALT(_pAdapter)							\
{ 																	\
	u8		u1bTmp;												\
	do																	\
	{																	\
		u1bTmp = read_nic_byte(_pAdapter, REG_LDOV12D_CTRL);		\
		u1bTmp |= BIT0; 												\
		write_nic_byte(_pAdapter, REG_LDOV12D_CTRL, u1bTmp);		\
		write_nic_byte(_pAdapter, REG_SPS_OCP_CFG, 0x0);				\
	write_nic_byte(_pAdapter, TXPAUSE, 0xFF);				\
	write_nic_word(_pAdapter, CMDR, 0x57FC);				\
	delay_us(100);													\
	write_nic_word(_pAdapter, CMDR, 0x77FC);				\
	write_nic_byte(_pAdapter, PHY_CCA, 0x0);				\
	delay_us(10);													\
	write_nic_word(_pAdapter, CMDR, 0x37FC);				\
	delay_us(10);													\
	write_nic_word(_pAdapter, CMDR, 0x77FC);				\
	delay_us(10);													\
	write_nic_word(_pAdapter, CMDR, 0x57FC);				\
	write_nic_word(_pAdapter, CMDR, 0x0000);				\
		u1bTmp = read_nic_byte(_pAdapter, (REG_SYS_CLKR + 1));		\
		if(u1bTmp & BIT7)												\
		{																\
			u1bTmp &= ~(BIT6 | BIT7);									\
			if(!HalSetSysClk8192SE(_pAdapter, u1bTmp))					\
				break;													\
		}																\
	write_nic_byte(_pAdapter, 0x03, 0x71);					\
	write_nic_byte(_pAdapter, 0x09, 0x70);					\
	write_nic_byte(_pAdapter, 0x29, 0x68);					\
	write_nic_byte(_pAdapter, 0x28, 0x00);					\
	write_nic_byte(_pAdapter, 0x20, 0x50);					\
	write_nic_byte(_pAdapter, 0x26, 0x0E);					\
	}while(FALSE);														\
}
#endif



/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 
typedef enum _SwChnlCmdID{
	CmdID_End,
	CmdID_SetTxPowerLevel,
	CmdID_BBRegWrite10,
	CmdID_WritePortUlong,
	CmdID_WritePortUshort,
	CmdID_WritePortUchar,
	CmdID_RF_WriteReg,
}SwChnlCmdID;

typedef	enum _IO_TYPE{
	IO_CMD_PAUSE_DM_BY_SCAN = 0,	
	IO_CMD_RESUME_DM_BY_SCAN = 1,
}IO_TYPE,*PIO_TYPE;

/* 1. Switch channel related */
typedef struct _SwChnlCmd{
	SwChnlCmdID	CmdID;
	u32			Para1;
	u32			Para2;
	u32			msDelay;
}SwChnlCmd;

typedef enum _HW90_BLOCK{
	HW90_BLOCK_MAC = 0,
	HW90_BLOCK_PHY0 = 1,
	HW90_BLOCK_PHY1 = 2,
	HW90_BLOCK_RF = 3,
	HW90_BLOCK_MAXIMUM = 4, 
}HW90_BLOCK_E, *PHW90_BLOCK_E;

typedef enum _RF90_RADIO_PATH{
	RF90_PATH_A = 0,			
	RF90_PATH_B = 1,			
	RF90_PATH_C = 2,			
	RF90_PATH_D = 3,			
}RF90_RADIO_PATH_E, *PRF90_RADIO_PATH_E;
#define	RF90_PATH_MAX			2


#define CHANNEL_MAX_NUMBER		14	
#define CHANNEL_GROUP_MAX		3	


typedef enum _BaseBand_Config_Type{
	BaseBand_Config_PHY_REG = 0,			
	BaseBand_Config_AGC_TAB = 1,			
}BaseBand_Config_Type, *PBaseBand_Config_Type;

typedef enum _VERSION_8190{
	VERSION_8190_BD=0x3,
	VERSION_8190_BE
}VERSION_8190,*PVERSION_8190;

#if 0
typedef enum _VERSION_8192C{
	VERSION_8192C_TEST_CHIP_91C,
	VERSION_8192C_TEST_CHIP_88C,
	VERSION_8192C_NORMAL_CHIP,
}VERSION_8192C,*PVERSION_8192C;
#endif

typedef enum _VERSION_8192S{
	VERSION_8192S_ACUT,
	VERSION_8192S_BCUT,
	VERSION_8192S_CCUT
}VERSION_8192S,*PVERSION_8192S;

typedef enum _PHY_Rate_Tx_Power_Offset_Area{
	RA_OFFSET_LEGACY_OFDM1,
	RA_OFFSET_LEGACY_OFDM2,
	RA_OFFSET_HT_OFDM1,
	RA_OFFSET_HT_OFDM2,
	RA_OFFSET_HT_OFDM3,
	RA_OFFSET_HT_OFDM4,
	RA_OFFSET_HT_CCK,
}RA_OFFSET_AREA,*PRA_OFFSET_AREA;


/* BB/RF related */

#if 0
typedef enum _RATR_TABLE_MODE_8192S{
	RATR_INX_WIRELESS_NGB = 0,
	RATR_INX_WIRELESS_NG = 1,
	RATR_INX_WIRELESS_NB = 2,
	RATR_INX_WIRELESS_N = 3,
	RATR_INX_WIRELESS_GB = 4,
	RATR_INX_WIRELESS_G = 5,
	RATR_INX_WIRELESS_B = 6,
	RATR_INX_WIRELESS_MC = 7,
	RATR_INX_WIRELESS_A = 8,
}RATR_TABLE_MODE_8192S, *PRATR_TABLE_MODE_8192S;
#endif
typedef struct _BB_REGISTER_DEFINITION{
	u32 rfintfs;			
							
	u32 rfintfi;			
							
	u32 rfintfo; 		
							
	u32 rfintfe; 		
							
	u32 rf3wireOffset;	
							
	u32 rfLSSI_Select;	
							
	u32 rfTxGainStage;	
							
	u32 rfHSSIPara1; 	
							
	u32 rfHSSIPara2; 	
								
	u32 rfSwitchControl; 
								
	u32 rfAGCControl1; 	
								
	u32 rfAGCControl2; 	
							
	u32 rfRxIQImbalance; 
							
	u32 rfRxAFE;  		
							
	u32 rfTxIQImbalance; 
							
	u32 rfTxAFE; 		
								
	u32 rfLSSIReadBack; 	

	u32 rfLSSIReadBackPi; 	

}BB_REGISTER_DEFINITION_T, *PBB_REGISTER_DEFINITION_T;

typedef enum _ANTENNA_PATH{
        ANTENNA_NONE 	= 0x00,
		ANTENNA_D		,
		ANTENNA_C		,
		ANTENNA_CD		,
		ANTENNA_B		,
		ANTENNA_BD		,
		ANTENNA_BC		,
		ANTENNA_BCD		,
		ANTENNA_A		,
		ANTENNA_AD		,
		ANTENNA_AC		,
		ANTENNA_ACD		,
		ANTENNA_AB		,
		ANTENNA_ABD		,
		ANTENNA_ABC		,
		ANTENNA_ABCD	
} ANTENNA_PATH;

typedef struct _R_ANTENNA_SELECT_OFDM{	
	u32			r_tx_antenna:4;	
	u32			r_ant_l:4;
	u32			r_ant_non_ht:4;	
	u32			r_ant_ht1:4;
	u32			r_ant_ht2:4;
	u32			r_ant_ht_s1:4;
	u32			r_ant_non_ht_s1:4;
	u32			OFDM_TXSC:2;
	u32			Reserved:2;
}R_ANTENNA_SELECT_OFDM;

typedef struct _R_ANTENNA_SELECT_CCK{
	u8			r_cckrx_enable_2:2;	
	u8			r_cckrx_enable:2;
	u8			r_ccktx_enable:4;
}R_ANTENNA_SELECT_CCK;

/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/


/*------------------------Export Marco Definition---------------------------*/
/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/
extern	u32	rtl8192_QueryBBReg(	struct net_device* dev,
								u32		RegAddr,
								u32		BitMask	);
extern	void	PHY_SetBBReg(	struct net_device* dev,
								u32		RegAddr,
								u32		BitMask,
								u32		Data	);
extern	u32	rtl8192_phy_QueryRFReg(	struct net_device* dev,
								RF90_RADIO_PATH_E	eRFPath,
								u32				RegAddr,
								u32				BitMask	);
extern	void	PHY_SetRFReg(	struct net_device* dev,
								RF90_RADIO_PATH_E	eRFPath,
								u32				RegAddr,
								u32				BitMask,
								u32				Data	);

/* MAC/BB/RF HAL config */
extern	bool	PHY_MACConfig8192C(	struct net_device* dev);
extern	bool	PHY_BBConfig8192C(struct net_device* dev	);
extern	bool	PHY_RFConfig8192C(	struct net_device* dev	);
/* RF config */
extern	bool	PHY_ConfigRFWithHeaderFile(	struct net_device* dev,
												RF90_RADIO_PATH_E	eRFPath);

/* Read initi reg value for tx power setting. */
extern	void	PHY_GetHWRegOriginalValue(struct net_device* dev	);

extern	bool	PHY_SetRFPowerState(struct net_device* dev,
									RT_RF_POWER_STATE	eRFPowerState);
extern	void	
PHY_SetRtl8192seRfHalt(struct net_device* dev);

extern	void	PHY_GetTxPowerLevel8192C(	struct net_device* dev,
											long*    		powerlevel	);
extern	void	PHY_SetTxPowerLevel8192C(	struct net_device* dev,
											u8			channel	);
extern	bool	PHY_UpdateTxPowerDbm8192C(	struct net_device* dev,
											long		powerInDbm	);

extern	void 
PHY_ScanOperationBackup8192C(struct net_device* dev,
										u8		Operation	);

extern	void	PHY_SetBWModeCallback8192C(struct net_device *dev);
extern	void	PHY_SetBWMode8192C(	struct net_device* dev,
									HT_CHANNEL_WIDTH	ChnlWidth,
									HT_EXTCHNL_OFFSET	Offset	);

extern	bool HalSetFwCmd8192S(	struct net_device* dev,
									FW_CMD_IO_TYPE			FwCmdIO);

extern	void FillA2Entry8192C(struct net_device* dev,
										u8				index,
										u8*				val);


extern	void	PHY_SwChnlCallback8192C(struct net_device *dev);
extern	u8	PHY_SwChnl8192C(struct net_device* dev,
									u8			channel	);
extern	void	PHY_SwChnlPhy8192C(struct net_device* dev,
									u8			channel	);

extern void ChkFwCmdIoDone(struct net_device* dev);

#ifdef USE_WORKITEM	
extern 	void SetFwCmdIOWorkItemCallback(struct net_device* dev);
#else
extern	void SetFwCmdIOTimerCallback(struct net_device* dev);
#endif	
				
extern	void	PHY_FalseAlarmCounterStatistics8192S(struct net_device* dev	);

extern	bool	rtl8192_phy_CheckIsLegalRFPath(struct net_device* dev,
											u32		eRFPath	);

extern	void	
PHY_IQCalibrate(struct net_device* dev);

extern void	
PHY_SetBeaconHwReg(	struct net_device* dev,
					u16			BeaconInterval	);


extern	void
PHY_SwitchEphyParameter(
	struct net_device* dev,bool	bSleep
	);

extern	void
PHY_EnableHostClkReq(
	struct net_device* dev
	);

long
phy_TxPwrIdxToDbm(
	struct net_device* dev,
	WIRELESS_MODE	WirelessMode,
	u8			TxPwrIdx	
	);

bool
SetAntennaConfig92C(
	struct net_device* dev,
	u8		DefaultAnt	
	);

bool
rtl8192ce_phy_SetFwCmdIO(struct net_device* dev, FW_CMD_IO_TYPE FwCmdIO);
bool
HalSetIO8192C(
	struct net_device* dev,
	IO_TYPE		IOType
);
void
phy_SetIO(
    struct net_device* dev
 );

bool
PHY_ConfigRFWithParaFile(
        struct net_device* 		dev,
        u8 					pFileName,
        RF90_RADIO_PATH_E	eRFPath
        );

void
PHY_APCalibrate(
	struct net_device* dev,
	char 		delta	
	);
void
PHY_LCCalibrate(
	struct net_device* dev
	);	
bool
SetAntennaConfig92C(
	struct net_device* dev,
	u8			DefaultAnt		
);
/*--------------------------Exported Function prototype---------------------*/


#endif	

