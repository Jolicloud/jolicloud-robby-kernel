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

/******************************************************************************
 * 
 *     (c) Copyright  2008, RealTEK Technologies Inc. All Rights Reserved.
 * 
 * Module:	HalRf.h	( Header File)
 * 
 * Note:	Collect every HAL RF type exter API or constant.	 
 *
 * Function:	
 * 		 
 * Export:	
 * 
 * Abbrev:	
 * 
 * History:
 * Data			Who		Remark
 * 
 * 09/25/2008	MHC		Create initial version.
 * 
 * 
******************************************************************************/
#ifndef _HAL_RF_H_
#define _HAL_RF_H_
/* Check to see if the file has been included already.  */


/*--------------------------Define Parameters-------------------------------*/

//
// For RF 6052 Series
//
#define		RF6052_MAX_TX_PWR			0x3F
#define		RF6052_MAX_REG				0x3F
#define		RF6052_MAX_PATH				2
/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 

/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/

/*------------------------Export Marco Definition---------------------------*/

/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/
//======================================================
// Function prototypes for HalPhy8225.c
//1======================================================

extern	void	PHY_SetRF8225OfdmTxPower(	IN	PADAPTER		Adapter,
												IN	u8			powerlevel);
extern	void	PHY_SetRF8225CckTxPower(	IN	PADAPTER		Adapter,
											IN	u8			powerlevel	);
extern	void	PHY_SetRF8225Bandwidth(	IN	PADAPTER		Adapter,
											IN	HT_CHANNEL_WIDTH	Bandwidth);	
extern	int	PHY_RF8225_Config(	IN	PADAPTER		Adapter	);


//1======================================================
// Function prototypes for HalPhy8256.c
//1======================================================
extern	void	PHY_SetRF8256OFDMTxPower(	IN	PADAPTER	Adapter,
											IN	u8			powerlevel	);
extern	void	PHY_SetRF8256CCKTxPower(	IN	PADAPTER	Adapter,
											IN	u8			powerlevel	);
extern	void	PHY_SetRF8256Bandwidth(	IN	PADAPTER		Adapter,
										IN	HT_CHANNEL_WIDTH	Bandwidth);	
extern	int	PHY_RF8256_Config(	IN	PADAPTER		Adapter	);


//
// RF RL6052 Series API
//
extern	void		RF_ChangeTxPath(	IN	PADAPTER	Adapter, 
										IN	u16		DataRate);
extern	void		PHY_RF6052SetBandwidth(	
										IN	PADAPTER				Adapter,
										IN	HT_CHANNEL_WIDTH		Bandwidth);	
extern	VOID	PHY_RF6052SetCckTxPower(
										IN	PADAPTER	Adapter,
										IN	u8*		pPowerlevel);
extern	VOID	PHY_RF6052SetOFDMTxPower(
										IN	PADAPTER	Adapter,
										IN	u8*		pPowerLevel,
										IN	u8		Channel);
extern	int	PHY_RF6052_Config(	IN	PADAPTER		Adapter	);

//
// RF Shadow operation relative API
//
extern	u32
PHY_RFShadowRead(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset);
extern	VOID
PHY_RFShadowWrite(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset,
	IN	u32				Data);
extern	BOOLEAN
PHY_RFShadowCompare(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset);
extern	VOID
PHY_RFShadowRecorver(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset);
extern	VOID
PHY_RFShadowCompareAll(
	IN	PADAPTER			Adapter);
extern	VOID
PHY_RFShadowRecorverAll(
	IN	PADAPTER			Adapter);
extern	VOID
PHY_RFShadowCompareFlagSet(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset,
	IN	u8				Type);
extern	VOID
PHY_RFShadowRecorverFlagSet(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset,
	IN	u8				Type);
extern	VOID
PHY_RFShadowCompareFlagSetAll(
	IN	PADAPTER			Adapter);
extern	VOID
PHY_RFShadowRecorverFlagSetAll(
	IN	PADAPTER			Adapter);
extern	VOID
PHY_RFShadowRefresh(
	IN	PADAPTER			Adapter);
/*--------------------------Exported Function prototype---------------------*/


#endif/* End of HalRf.h */

