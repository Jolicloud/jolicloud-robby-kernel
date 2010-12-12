
/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    dfs.h

    Abstract:
    Support DFS function.

    Revision History:
    Who       When            What
    --------  ----------      ----------------------------------------------
    Fonchi    03-12-2007      created
*/

#define RADAR_PULSE 1
#define RADAR_WIDTH 2

#define WIDTH_RD_IDLE 0
#define WIDTH_RD_CHECK 1



/*************************************************************************
  *
  *	DFS Radar related definitions.
  *
  ************************************************************************/  
//#define CARRIER_DETECT_TASK_NUM	6
//#define RADAR_DETECT_TASK_NUM	7

// McuRadarState && McuCarrierState for 2880-SW-MCU
#define FREE_FOR_TX				0
#define WAIT_CTS_BEING_SENT		1
#define DO_DETECTION			2

// McuRadarEvent
#define RADAR_EVENT_CTS_SENT			0x01 // Host signal MCU that CTS has been sent
#define RADAR_EVENT_CTS_CARRIER_SENT	0x02 // Host signal MCU that CTS has been sent (Carrier)
#define RADAR_EVENT_RADAR_DETECTING		0x04 // Radar detection is on going, carrier detection hold back
#define RADAR_EVENT_CARRIER_DETECTING	0x08 // Carrier detection is on going, radar detection hold back
#define RADAR_EVENT_WIDTH_RADAR			0x10 // BBP == 2 radar detected
#define RADAR_EVENT_CTS_KICKED			0x20 // Radar detection need to sent double CTS, first CTS sent

// McuRadarCmd
#define DETECTION_STOP			0
#define RADAR_DETECTION			1
#define CARRIER_DETECTION		2

#if defined(RTMP_RBUS_SUPPORT) || defined(DFS_INTERRUPT_SUPPORT)
#define RADAR_GPIO_DEBUG	0x01 // GPIO external debug
#define RADAR_SIMULATE		0x02 // simulate a short pulse hit this channel
#define RADAR_SIMULATE2		0x04 // print any hit
#define RADAR_LOG			0x08 // log record and ready for print

// Both Old and New DFS
#define RADAR_DONT_SWITCH		0x10 // Don't Switch channel when hit

#ifdef DFS_HARDWARE_SUPPORT
// New DFS only
#define RADAR_DEBUG_EVENT			0x01 // print long pulse debug event
#define RADAR_DEBUG_FLAG_1			0x02
#define RADAR_DEBUG_FLAG_2			0x04
#define RADAR_DEBUG_FLAG_3			0x08
#define RADAR_DEBUG_SILENCE			0x4
#define RADAR_DEBUG_SW_SILENCE		0x8
#endif // DFS_HARDWARE_SUPPORT //

#ifdef DFS_HARDWARE_SUPPORT
VOID NewRadarDetectionStart(
	IN PRTMP_ADAPTER pAd);

VOID NewRadarDetectionStop(
	IN PRTMP_ADAPTER pAd);

void modify_table1(
	IN PRTMP_ADAPTER pAd, 
	IN ULONG idx, 
	IN ULONG value);

void modify_table2(
	IN PRTMP_ADAPTER pAd, 
	IN ULONG idx, 
	IN ULONG value);


 VOID NewTimerCB_Radar(
 	IN PRTMP_ADAPTER pAd);
 
 void schedule_dfs_task(
 	IN PRTMP_ADAPTER pAd);


#endif // DFS_HARDWARE_SUPPORT //

#endif // defined (RTMP_RBUS_SUPPORT) || defined(DFS_INTERRUPT_SUPPORT)  //


void MCURadarDetect(PRTMP_ADAPTER pAd);

#ifdef TONE_RADAR_DETECT_SUPPORT
void RTMPHandleRadarInterrupt(PRTMP_ADAPTER  pAd);
#else

#ifdef DFS_HARDWARE_SUPPORT
#if defined (RTMP_RBUS_SUPPORT) || defined(DFS_INTERRUPT_SUPPORT)
void RTMPHandleRadarInterrupt(PRTMP_ADAPTER  pAd);
#endif // defined (RTMP_RBUS_SUPPORT) || defined(DFS_INTERRUPT_SUPPORT) //
#endif // DFS_HARDWARE_SUPPORT //
#endif // TONE_RADAR_DETECT_SUPPORT //

#ifdef TONE_RADAR_DETECT_SUPPORT
INT Set_CarrierCriteria_Proc(IN PRTMP_ADAPTER pAd, IN PSTRING arg);
int Set_CarrierReCheck_Proc(IN PRTMP_ADAPTER pAd, IN PSTRING arg);
INT Set_CarrierStopCheck_Proc(IN PRTMP_ADAPTER pAd, IN PSTRING arg);
void NewCarrierDetectionStart(PRTMP_ADAPTER pAd);
#endif // TONE_RADAR_DETECT_SUPPORT //

#ifdef DFS_SOFTWARE_SUPPORT
VOID BbpRadarDetectionStart(
	IN PRTMP_ADAPTER pAd);

VOID BbpRadarDetectionStop(
	IN PRTMP_ADAPTER pAd);

VOID RadarDetectionStart(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN CTS_Protect,
	IN UINT8 CTSPeriod);

VOID RadarDetectionStop(
	IN PRTMP_ADAPTER	pAd);
#endif // DFS_SOFTWARE_SUPPORT //

VOID RadarDetectPeriodic(
	IN PRTMP_ADAPTER	pAd);
	

BOOLEAN RadarChannelCheck(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			Ch);

ULONG JapRadarType(
	IN PRTMP_ADAPTER pAd);

ULONG RTMPBbpReadRadarDuration(
	IN PRTMP_ADAPTER	pAd);

ULONG RTMPReadRadarDuration(
	IN PRTMP_ADAPTER	pAd);

VOID RTMPCleanRadarDuration(
	IN PRTMP_ADAPTER	pAd);

VOID RTMPPrepareRDCTSFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pDA,
	IN	ULONG			Duration,
	IN  UCHAR           RTSRate,
	IN  ULONG           CTSBaseAddr,
	IN  UCHAR			FrameGap);
#ifdef DFS_SOFTWARE_SUPPORT
VOID RTMPPrepareRadarDetectParams(
	IN PRTMP_ADAPTER	pAd);
#endif // DFS_SOFTWARE_SUPPORT //

INT Set_ChMovingTime_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PSTRING arg);

INT Set_LongPulseRadarTh_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PSTRING arg);


