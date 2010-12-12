/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2009, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	misc_cmm.h

	Abstract:

	Handling Misc Problem

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Sean Wang	2009-08-12		Create
	John Li		2009-12-23		Modified
*/


#ifndef __MISC_CMM_H
#define __MISC_CMM_H


#define IS_ENABLE_MISC_TIMER(_pAd)							((((_pAd)->ulConfiguration & 0x00000001) == 0x00000001))
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
#define IS_ENABLE_40TO20_BY_TIMER(_pAd) 					((((_pAd)->ulConfiguration & 0x00000003) == 0x00000003))
#endif // DOT11N_DRAFT3 //
#define IS_ENABLE_TXWI_AMPDU_SIZE_BY_TIMER(_pAd)			((((_pAd)->ulConfiguration & 0x00000005) == 0x00000005))
#define IS_ENABLE_TXWI_AMPDU_DENSITY_BY_TIMER(_pAd)		((((_pAd)->ulConfiguration & 0x00000009) == 0x00000009))

#define IS_ENABLE_RATE_ADAPTIVE_BY_TIMER(_pAd) 			((((_pAd)->ulConfiguration & 0x00000011) == 0x00000011))
#define IS_ENABLE_REJECT_ORE_BA_BY_TIMER(_pAd) 			((((_pAd)->ulConfiguration & 0x00000021) == 0x00000021))
#endif // DOT11_N_SUPPORT //
#define IS_ENABLE_TX_POWER_DOWN_BY_TIMER(_pAd) 			((((_pAd)->ulConfiguration & 0x00000041) == 0x00000041))
#define IS_ENABLE_LNA_MID_GAIN_DOWN_BY_TIMER(_pAd) 		((((_pAd)->ulConfiguration & 0x00000081) == 0x00000081))

#define IS_ENABLE_WIFI_ACTIVE_PULL_LOW_BY_FORCE(_pAd)	((((_pAd)->ulConfiguration & 0x00000400) == 0x00000400))
#ifdef RTMP_PCI_SUPPORT
#define IS_ENABLE_SINGLE_CRYSTAL_SHARING_BY_FORCE(_pAd)	((((_pAd)->ulConfiguration & 0x00000800) == 0x00000800))
#endif // RTMP_PCI_SUPPORT //

#ifdef DOT11_N_SUPPORT
#define GET_PARAMETER_OF_AMPDU_SIZE(_pAd)				((((_pAd)->ulConfiguration & 0xC0000000) >> 30))
#define GET_PARAMETER_OF_AMPDU_DENSITY(_pAd)			((((_pAd)->ulConfiguration & 0x30000000) >> 28))
#define GET_PARAMETER_OF_MCS_THRESHOLD(_pAd)				((((_pAd)->ulConfiguration & 0x0C000000) >> 26))
#ifdef DOT11N_DRAFT3
#define GET_PARAMETER_OF_TXRX_THR_THRESHOLD(_pAd)		((((_pAd)->ulConfiguration & 0x03000000) >> 24))
#endif // DOT11N_DRAFT3 //
#endif // DOT11_N_SUPPORT //

/* BUSY_0: no busy, BUSY_1 to BUSY_5: more busy */
typedef enum _BUSY_DEGREE {
	BUSY_0 = 0, 
	BUSY_1,		
	BUSY_2,		
	BUSY_3,		  
	BUSY_4,		
	BUSY_5		
	
} BUSY_DEGREE;

#endif /* __MISC_CMM_H */

