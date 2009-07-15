/*
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
 * $Id: typedefs.h,v 1.85.32.8 2009/02/26 17:07:08 Exp $
 */

#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

#ifdef __cplusplus

#define TYPEDEF_BOOL
#ifndef FALSE
#define FALSE	false
#endif
#ifndef TRUE
#define TRUE	true
#endif

#else	

#endif	

#if defined(__x86_64__)
#define TYPEDEF_UINTPTR
typedef unsigned long long int uintptr;
#endif

#if defined(LINUX_PORT)
#define TYPEDEF_UINT
#define TYPEDEF_USHORT
#define TYPEDEF_ULONG
#ifdef __KERNEL__
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19))
#define TYPEDEF_BOOL
#endif	
#endif	
#endif  

#if defined(__GNUC__) && defined(__STRICT_ANSI__)
#define TYPEDEF_INT64
#define TYPEDEF_UINT64
#endif

#if defined(__KERNEL__)

#if defined(LINUX_PORT)
#include <linux/types.h>	
#endif 

#else

#include <sys/types.h>

#endif 

#define USE_TYPEDEF_DEFAULTS

#ifdef USE_TYPEDEF_DEFAULTS
#undef USE_TYPEDEF_DEFAULTS

#ifndef TYPEDEF_BOOL
typedef	 unsigned char	bool;
#endif

#ifndef TYPEDEF_UCHAR
typedef unsigned char	uchar;
#endif

#ifndef TYPEDEF_USHORT
typedef unsigned short	ushort;
#endif

#ifndef TYPEDEF_UINT
typedef unsigned int	uint;
#endif

#ifndef TYPEDEF_ULONG
typedef unsigned long	ulong;
#endif

#ifndef TYPEDEF_UINT8
typedef unsigned char	uint8;
#endif

#ifndef TYPEDEF_UINT16
typedef unsigned short	uint16;
#endif

#ifndef TYPEDEF_UINT32
typedef unsigned int	uint32;
#endif

#ifndef TYPEDEF_UINT64
typedef unsigned long long uint64;
#endif

#ifndef TYPEDEF_UINTPTR
typedef unsigned int	uintptr;
#endif

#ifndef TYPEDEF_INT8
typedef signed char	int8;
#endif

#ifndef TYPEDEF_INT16
typedef signed short	int16;
#endif

#ifndef TYPEDEF_INT32
typedef signed int	int32;
#endif

#ifndef TYPEDEF_INT64
typedef signed long long int64;
#endif

#ifndef TYPEDEF_FLOAT32
typedef float		float32;
#endif

#ifndef TYPEDEF_FLOAT64
typedef double		float64;
#endif

#ifndef TYPEDEF_FLOAT_T

#if defined(FLOAT32)
typedef float32 float_t;
#else 
typedef float64 float_t;
#endif

#endif 

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1  
#endif

#ifndef NULL
#define	NULL	0
#endif

#ifndef OFF
#define	OFF	0
#endif

#ifndef ON
#define	ON	1  
#endif

#define	AUTO	(-1) 

#ifndef PTRSZ
#define	PTRSZ	sizeof(char*)
#endif

#ifndef INLINE

#if defined(__GNUC__)

#define INLINE __inline__

#else

#define INLINE

#endif 

#endif 

#undef TYPEDEF_BOOL
#undef TYPEDEF_UCHAR
#undef TYPEDEF_USHORT
#undef TYPEDEF_UINT
#undef TYPEDEF_ULONG
#undef TYPEDEF_UINT8
#undef TYPEDEF_UINT16
#undef TYPEDEF_UINT32
#undef TYPEDEF_UINT64
#undef TYPEDEF_UINTPTR
#undef TYPEDEF_INT8
#undef TYPEDEF_INT16
#undef TYPEDEF_INT32
#undef TYPEDEF_INT64
#undef TYPEDEF_FLOAT32
#undef TYPEDEF_FLOAT64
#undef TYPEDEF_FLOAT_T

#endif 

#include <bcmdefs.h>

#endif 
