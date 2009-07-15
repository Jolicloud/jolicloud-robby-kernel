/*
 * Fundamental types and constants relating to WPA
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
 * $Id: wpa.h,v 1.16 2006/04/27 01:26:35 Exp $
 */

#ifndef _proto_wpa_h_
#define _proto_wpa_h_

#include <typedefs.h>
#include <proto/ethernet.h>

#if defined(__GNUC__)
#define	PACKED	__attribute__((packed))
#else
#pragma pack(1)
#define	PACKED
#endif

#define DOT11_RC_INVALID_WPA_IE		13	
#define DOT11_RC_MIC_FAILURE		14	
#define DOT11_RC_4WH_TIMEOUT		15	
#define DOT11_RC_GTK_UPDATE_TIMEOUT	16	
#define DOT11_RC_WPA_IE_MISMATCH	17	
#define DOT11_RC_INVALID_MC_CIPHER	18	
#define DOT11_RC_INVALID_UC_CIPHER	19	
#define DOT11_RC_INVALID_AKMP		20	
#define DOT11_RC_BAD_WPA_VERSION	21	
#define DOT11_RC_INVALID_WPA_CAP	22	
#define DOT11_RC_8021X_AUTH_FAIL	23	

#define WPA2_PMKID_LEN	16

typedef struct
{
	uint8 tag;	
	uint8 length;	
	uint8 oui[3];	
	uint8 oui_type;	
	struct {
		uint8 low;
		uint8 high;
	} PACKED version;	
} PACKED wpa_ie_fixed_t;
#define WPA_IE_OUITYPE_LEN	4
#define WPA_IE_FIXED_LEN	8
#define WPA_IE_TAG_FIXED_LEN	6

typedef struct {
	uint8 tag;	
	uint8 length;	
	struct {
		uint8 low;
		uint8 high;
	} PACKED version;	
} PACKED wpa_rsn_ie_fixed_t;
#define WPA_RSN_IE_FIXED_LEN	4
#define WPA_RSN_IE_TAG_FIXED_LEN	2
typedef uint8 wpa_pmkid_t[WPA2_PMKID_LEN];

typedef struct
{
	uint8 oui[3];
	uint8 type;
} PACKED wpa_suite_t, wpa_suite_mcast_t;
#define WPA_SUITE_LEN	4

typedef struct
{
	struct {
		uint8 low;
		uint8 high;
	} PACKED count;
	wpa_suite_t list[1];
} PACKED wpa_suite_ucast_t, wpa_suite_auth_key_mgmt_t;
#define WPA_IE_SUITE_COUNT_LEN	2
typedef struct
{
	struct {
		uint8 low;
		uint8 high;
	} PACKED count;
	wpa_pmkid_t list[1];
} PACKED wpa_pmkid_list_t;

#define WPA_CIPHER_NONE		0	
#define WPA_CIPHER_WEP_40	1	
#define WPA_CIPHER_TKIP		2	
#define WPA_CIPHER_AES_OCB	3	
#define WPA_CIPHER_AES_CCM	4	
#define WPA_CIPHER_WEP_104	5	

#define IS_WPA_CIPHER(cipher)	((cipher) == WPA_CIPHER_NONE || \
				 (cipher) == WPA_CIPHER_WEP_40 || \
				 (cipher) == WPA_CIPHER_WEP_104 || \
				 (cipher) == WPA_CIPHER_TKIP || \
				 (cipher) == WPA_CIPHER_AES_OCB || \
				 (cipher) == WPA_CIPHER_AES_CCM)

#define WPA_TKIP_CM_DETECT	60	
#define WPA_TKIP_CM_BLOCK	60	

#define RSN_CAP_LEN		2	

#define RSN_CAP_PREAUTH			0x0001
#define RSN_CAP_NOPAIRWISE		0x0002
#define RSN_CAP_PTK_REPLAY_CNTR_MASK	0x000C
#define RSN_CAP_PTK_REPLAY_CNTR_SHIFT	2
#define RSN_CAP_GTK_REPLAY_CNTR_MASK	0x0030
#define RSN_CAP_GTK_REPLAY_CNTR_SHIFT	4
#define RSN_CAP_1_REPLAY_CNTR		0
#define RSN_CAP_2_REPLAY_CNTRS		1
#define RSN_CAP_4_REPLAY_CNTRS		2
#define RSN_CAP_16_REPLAY_CNTRS		3

#define WPA_CAP_4_REPLAY_CNTRS		RSN_CAP_4_REPLAY_CNTRS
#define WPA_CAP_16_REPLAY_CNTRS		RSN_CAP_16_REPLAY_CNTRS
#define WPA_CAP_REPLAY_CNTR_SHIFT	RSN_CAP_PTK_REPLAY_CNTR_SHIFT
#define WPA_CAP_REPLAY_CNTR_MASK	RSN_CAP_PTK_REPLAY_CNTR_MASK

#define WPA_CAP_LEN	RSN_CAP_LEN	

#define	WPA_CAP_WPA2_PREAUTH		RSN_CAP_PREAUTH

#undef PACKED
#if !defined(__GNUC__)
#pragma pack()
#endif

#endif 
