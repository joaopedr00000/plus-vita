// SPDX-License-Identifier: AGPL-3.0-only
#include "qr_pair_crypto.h"
#include <mbedtls/base64.h>
#include <mbedtls/gcm.h>
#include <stdio.h>
#include <string.h>
void plus_vita_pair_clear(void *p,size_t len)
{
	volatile unsigned char *b=p;while(len--)*b++=0;
}
bool plus_vita_pair_decode(const uint8_t key[16],const unsigned char *payload,size_t len,char *json,size_t cap)
{
	uint8_t raw[92]={0},plain[65]={0};size_t n=0;bool ok=false;
	if(!json||cap<77)return false;
	plus_vita_pair_clear(json,cap);
	if(!key||!payload||len!=128||memcmp(payload,"PV2:",4))goto done;
	if(mbedtls_base64_decode(raw,sizeof(raw),&n,payload+4,len-4)||n!=sizeof(raw))goto done;
	mbedtls_gcm_context g;mbedtls_gcm_init(&g);
	int rc=mbedtls_gcm_setkey(&g,MBEDTLS_CIPHER_ID_AES,key,128);
	if(!rc)rc=mbedtls_gcm_auth_decrypt(&g,64,raw,12,(const unsigned char *)"PlusVitaQR2",11,raw+76,16,raw+12,plain);
	mbedtls_gcm_free(&g);
	if(rc)goto done;
	for(size_t i=0;i<64;i++)if(!((plain[i]>='0'&&plain[i]<='9')||(plain[i]>='A'&&plain[i]<='Z')||(plain[i]>='a'&&plain[i]<='z')))goto done;
	ok=snprintf(json,cap,"{\"npsso\":\"%s\"}",(char *)plain)==76;
done:
	plus_vita_pair_clear(raw,sizeof(raw));plus_vita_pair_clear(plain,sizeof(plain));
	if(!ok)plus_vita_pair_clear(json,cap);
	return ok;
}
