/*
 * Key management related declarations
 * and exported functions for
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_key.h,v 1.44.2.5 2008/08/22 00:37:51 Exp $
 */

#ifndef _wlc_key_h_
#define _wlc_key_h_

typedef struct wsec_iv {
	uint32		hi;	
	uint16		lo;	
} wsec_iv_t;

#define WLC_NUMRXIVS	16	

typedef struct wsec_key {
	struct ether_addr ea;		
	uint8		idx;		
	uint8		id;		
	uint8		algo;		
	uint16		flags;		
	uint8 		algo_hw;	
	uint8 		aes_mode;	
	int8		iv_len;		
	int8		icv_len;	
	uint32		len;		

	uint8		data[DOT11_MAX_KEY_SIZE];	
	wsec_iv_t	rxiv[WLC_NUMRXIVS];		
	wsec_iv_t	txiv;		

} wsec_key_t;

#endif 
