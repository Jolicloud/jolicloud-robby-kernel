/*
 * Minimal debug/trace/assert driver definitions for
 * Broadcom 802.11 Networking Adapter.
 *
 * Copyright 2008, Broadcom Corporation
 * All Rights Reserved.
 * 
 *  	Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2, available at
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html (the "GPL"), with the
 * following added to such license:
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, regardless of the
 * license terms of these independent modules, and to copy and distribute the
 * resulting executable under terms of your choice, provided that you also meet,
 * for each linked independent module, the terms and conditions of the license
 * of that module. An independent module is a module which is not derived from
 * this software.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: wl_dbg.h,v 1.99.2.5 2008/11/12 23:41:23 Exp $
 */

#ifndef _wl_dbg_h_
#define _wl_dbg_h_

#ifdef	BCMDBG

#define	WL_ERROR(args)		do {if (wl_msg_level & WL_ERROR_VAL) printf args;} while (0)
#define	WL_TRACE(args)		do {if (wl_msg_level & WL_TRACE_VAL) printf args;} while (0)
#define WL_WSEC(args)		do {if (wl_msg_level & WL_WSEC_VAL) printf args;} while (0)
#define	WL_INFORM(args)		do {if (wl_msg_level & WL_INFORM_VAL) printf args;} while (0)
#define WL_APSTA_UPDN(args) do {if (wl_apsta_dbg & WL_APSTA_UPDN_VAL) {WL_APSTA(args);}} while (0)
#define WL_APSTA_RX(args) do {if (wl_apsta_dbg & WL_APSTA_RX_VAL) {WL_APSTA(args);}} while (0)

#else	

#ifdef BCMDBG_ERR
#define	WL_ERROR(args)		printf args
#else
#define	WL_ERROR(args)
#endif 
#define	WL_TRACE(args)
#define WL_APSTA_UPDN(args)
#define WL_APSTA_RX(args)
#define WL_WSEC(args)
#define WL_WSEC_DUMP(args)
#define	WL_INFORM(args)

#endif 

extern uint32 wl_msg_level;
extern uint32 wl_msg_level2;
#endif 
