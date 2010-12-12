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


#include "crypt_hmac.h"


#if defined(__cplusplus)
extern "C"
{
#endif

/* initialise the HMAC context to zero */
void hmac_sha_begin(hmac_ctx cx[1])
{
    memset(cx, 0, sizeof(hmac_ctx));
}

/* input the HMAC key (can be called multiple times)    */
int hmac_sha_key(const unsigned char key[], unsigned int key_len, hmac_ctx cx[1])
{
    if(cx->klen == HMAC_IN_DATA)                /* error if further key input   */
        return HMAC_BAD_MODE;                   /* is attempted in data mode    */

    if(cx->klen + key_len > HASH_INPUT_SIZE)    /* if the key has to be hashed  */
    {
        if(cx->klen <= HASH_INPUT_SIZE)         /* if the hash has not yet been */
        {                                       /* started, initialise it and   */
            sha_begin(cx->ctx);                /* hash stored key characters   */
            sha_hash(cx->key, cx->klen, cx->ctx);
        }

        sha_hash(key, key_len, cx->ctx);       /* hash long key data into hash */
    }
    else                                        /* otherwise store key data     */
        memcpy(cx->key + cx->klen, key, key_len);

    cx->klen += key_len;                        /* update the key length count  */
    return HMAC_OK;
}

/* input the HMAC data (can be called multiple times) - */
/* note that this call terminates the key input phase   */
void hmac_sha_data(const unsigned char data[], unsigned int data_len, hmac_ctx cx[1])
{   unsigned int i;

    if(cx->klen != HMAC_IN_DATA)                /* if not yet in data phase */
    {
        if(cx->klen > HASH_INPUT_SIZE)          /* if key is being hashed   */
        {                                       /* complete the hash and    */
            sha_end(cx->key, cx->ctx);         /* store the result as the  */
            cx->klen = HASH_OUTPUT_SIZE;        /* key and set new length   */
        }

        /* pad the key if necessary */
        memset(cx->key + cx->klen, 0, HASH_INPUT_SIZE - cx->klen);

        /* xor ipad into key value  */
        for(i = 0; i < (HASH_INPUT_SIZE >> 2); ++i)
            ((unsigned int*)cx->key)[i] ^= 0x36363636;

        /* and start hash operation */
        sha_begin(cx->ctx);
        sha_hash(cx->key, HASH_INPUT_SIZE, cx->ctx);

        /* mark as now in data mode */
        cx->klen = HMAC_IN_DATA;
    }

    /* hash the data (if any)       */
    if(data_len)
        sha_hash(data, data_len, cx->ctx);
}

/* compute and output the MAC value */
void hmac_sha_end(unsigned char mac[], unsigned int mac_len, hmac_ctx cx[1])
{   unsigned char dig[HASH_OUTPUT_SIZE];
    unsigned int i;

    /* if no data has been entered perform a null data phase        */
    if(cx->klen != HMAC_IN_DATA)
        hmac_sha_data((const unsigned char*)0, 0, cx);

    sha_end(dig, cx->ctx);         /* complete the inner hash      */

    /* set outer key value using opad and removing ipad */
    for(i = 0; i < (HASH_INPUT_SIZE >> 2); ++i)
        ((unsigned int*)cx->key)[i] ^= 0x36363636 ^ 0x5c5c5c5c;

    /* perform the outer hash operation */
    sha_begin(cx->ctx);
    sha_hash(cx->key, HASH_INPUT_SIZE, cx->ctx);
    sha_hash(dig, HASH_OUTPUT_SIZE, cx->ctx);
    sha_end(dig, cx->ctx);

    /* output the hash value            */
    for(i = 0; i < mac_len; ++i)
        mac[i] = dig[i];
}

/* 'do it all in one go' subroutine     */
void hmac_sha(const unsigned char key[], unsigned int key_len,
          const unsigned char data[], unsigned int data_len,
          unsigned char mac[], unsigned int mac_len)
{   hmac_ctx    cx[1];

    hmac_sha_begin(cx);
    hmac_sha_key(key, key_len, cx);
    hmac_sha_data(data, data_len, cx);
    hmac_sha_end(mac, mac_len, cx);
}

#if defined(__cplusplus)
}
#endif


/* End of crypt_hmac.c */

