/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/


#ifndef __CRYPT_MD5_H__
#define __CRYPT_MD5_H__


#ifndef	uint8
#define	uint8  unsigned	char
#endif

#ifndef	uint32
#define	uint32 unsigned	long int
#endif

#define MD5_MAC_LEN 16

typedef struct _MD5_CTX {
    ULONG   Buf[4];             // buffers of four states
	UCHAR   Input[64];          // input message
	ULONG   LenInBitCount[2];   // length counter for input message, 0 up to 64 bits	                            
}   MD5_CTX;

VOID MD5Init(MD5_CTX *pCtx);
VOID MD5Update(MD5_CTX *pCtx, UCHAR *pData, ULONG LenInBytes);
VOID MD5Final(UCHAR Digest[16], MD5_CTX *pCtx);
VOID MD5Transform(ULONG Buf[4], ULONG Mes[16]);

void md5_mac(u8 *key, size_t key_len, u8 *data, size_t data_len, u8 *mac);
void hmac_md5(u8 *key, size_t key_len, u8 *data, size_t data_len, u8 *mac);

//
// SHA context
//
typedef	struct _SHA_CTX
{
	ULONG   Buf[5];             // buffers of five states
	UCHAR   Input[80];          // input message
	ULONG   LenInBitCount[2];   // length counter for input message, 0 up to 64 bits
	
}	SHA_CTX;

VOID SHAInit(SHA_CTX *pCtx);
UCHAR SHAUpdate(SHA_CTX *pCtx, UCHAR *pData, ULONG LenInBytes);
VOID SHAFinal(SHA_CTX *pCtx, UCHAR Digest[20]);
VOID SHATransform(ULONG Buf[5], ULONG Mes[20]);

#define SHA_DIGEST_LEN 20

VOID	HMAC_SHA1(
	IN	UCHAR	*text,
	IN	UINT	text_len,
	IN	UCHAR	*key,
	IN	UINT	key_len,
	IN	UCHAR	*digest);
	
#define RT_HMAC_MD5(Key, KeyL, Meg, MegL, MAC, MACL) \
    hmac_md5((Key), (KeyL), (Meg), (MegL), (MAC))

#define RT_HMAC_SHA1(Key, KeyL, Meg, MegL, MAC, MACL) \
    HMAC_SHA1((Meg), (MegL), (Key), (KeyL), (MAC))


#endif /* __CRYPT_MD5_H__ */

