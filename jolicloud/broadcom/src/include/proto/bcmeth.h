/*
 * Broadcom Ethernettype  protocol definitions
 *
 * Copyright 2008, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: bcmeth.h,v 9.9.12.2 2008/05/02 23:24:25 Exp $
 */

#ifndef _BCMETH_H_
#define _BCMETH_H_

#if defined(__GNUC__)
#define	PACKED	__attribute__((packed))
#else
#pragma pack(1)
#define	PACKED
#endif

typedef  struct bcmeth_hdr
{
	uint16	subtype;	
	uint16	length;
	uint8	version;	
	uint8	oui[3];		

	uint16	usr_subtype;
} PACKED bcmeth_hdr_t;

#undef PACKED
#if !defined(__GNUC__)
#pragma pack()
#endif

#endif	
