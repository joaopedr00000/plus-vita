// SPDX-License-Identifier: AGPL-3.0-only
#include "qr_pair_crypto.h"
#include <mbedtls/base64.h>
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <stdio.h>
#include <string.h>
void plus_vita_pair_clear(void *p,size_t len)
{
	volatile unsigned char *b=p;while(len--)*b++=0;
}
static bool sha256(const uint8_t *input,size_t len,uint8_t output[32])
{
	const mbedtls_md_info_t *md=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	return md && mbedtls_md(md,input,len,output)==0;
}
bool plus_vita_pair_make_key(const uint8_t entropy[32],const uint8_t device[6],uint8_t key[32])
{
	if(!key)return false;
	plus_vita_pair_clear(key,32);
	if(!entropy||!device)return false;
	unsigned nonzero=0,device_nonzero=0;
	for(size_t i=0;i<32;i++)nonzero|=entropy[i];
	for(size_t i=0;i<6;i++)device_nonzero|=device[i];
	if(!nonzero||!device_nonzero)return false;
	uint8_t input[53]={0};
	memcpy(input,"PlusVitaQR3-key",15);
	memcpy(input+15,entropy,32);
	memcpy(input+47,device,6);
	bool ok=sha256(input,sizeof(input),key);
	plus_vita_pair_clear(input,sizeof(input));
	if(!ok)plus_vita_pair_clear(key,32);
	return ok;
}
static bool session_id(const uint8_t key[32],uint8_t id[32])
{
	uint8_t input[47];
	memcpy(input,"PlusVitaQR3-sid",15);
	memcpy(input+15,key,32);
	bool ok=sha256(input,sizeof(input),id);
	plus_vita_pair_clear(input,sizeof(input));
	return ok;
}
bool plus_vita_pair_decode(const uint8_t key[32],const unsigned char *payload,size_t len,char *json,size_t cap)
{
	/* 32-byte session ID, 12-byte IV, 64-byte ciphertext, 16-byte GCM tag. */
	uint8_t raw[124]={0},plain[65]={0},expected[32]={0};size_t n=0;bool ok=false;
	if(!json||cap<77)return false;
	plus_vita_pair_clear(json,cap);
	if(!key||!payload||len!=PLUS_VITA_PAIR_PAYLOAD_LEN||memcmp(payload,"PV3:",4))goto done;
	if(mbedtls_base64_decode(raw,sizeof(raw),&n,payload+4,len-4)||n!=sizeof(raw))goto done;
	if(!session_id(key,expected))goto done;
	unsigned mismatch=0;
	for(size_t i=0;i<32;i++)mismatch|=raw[i]^expected[i];
	if(mismatch)goto done;
	mbedtls_gcm_context g;mbedtls_gcm_init(&g);
	int rc=mbedtls_gcm_setkey(&g,MBEDTLS_CIPHER_ID_AES,key,256);
	if(!rc)rc=mbedtls_gcm_auth_decrypt(&g,64,raw+32,12,raw,32,raw+108,16,raw+44,plain);
	mbedtls_gcm_free(&g);
	if(rc)goto done;
	for(size_t i=0;i<64;i++)if(!((plain[i]>='0'&&plain[i]<='9')||(plain[i]>='A'&&plain[i]<='Z')||(plain[i]>='a'&&plain[i]<='z')))goto done;
	ok=snprintf(json,cap,"{\"npsso\":\"%s\"}",(char *)plain)==76;
done:
	plus_vita_pair_clear(raw,sizeof(raw));plus_vita_pair_clear(plain,sizeof(plain));plus_vita_pair_clear(expected,sizeof(expected));
	if(!ok)plus_vita_pair_clear(json,cap);
	return ok;
}
