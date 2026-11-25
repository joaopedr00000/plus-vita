// SPDX-License-Identifier: AGPL-3.0-only
#ifndef VP_ACCOUNT_VAULT_H
#define VP_ACCOUNT_VAULT_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/version.h>

#define PV_VAULT_ROUNDS 200000u
#define PV_VAULT_HEADER 32u
#define PV_VAULT_TAG 16u
#define PV_VAULT_MAX 4096u
#define PV_VAULT_OVERHEAD (PV_VAULT_HEADER+PV_VAULT_TAG)
static inline void pv_vault_wipe(void *p,size_t n){volatile unsigned char *b=p;while(n--)*b++=0;}
static inline bool pv_vault_password(const char *p,size_t n){
    if(!p||n<12||n>64)return false;
    for(size_t j=0;j<n;j++)if((unsigned char)p[j]<33||(unsigned char)p[j]>126)return false;
    return true;
}
static inline bool pv_vault_is(const void *p,size_t n){return p&&n>=4&&!memcmp(p,"PVV1",4);}
static inline bool pv_vault_key(const char *p,size_t n,const uint8_t salt[16],uint8_t key[32]){
    if(!pv_vault_password(p,n))return false;
#if MBEDTLS_VERSION_NUMBER >= 0x03030000
    return mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,(const unsigned char *)p,n,salt,16,PV_VAULT_ROUNDS,32,key)==0;
#else
    mbedtls_md_context_t ctx;mbedtls_md_init(&ctx);
    const mbedtls_md_info_t *md=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    int r=md?mbedtls_md_setup(&ctx,md,1):-1;
    if(!r)r=mbedtls_pkcs5_pbkdf2_hmac(&ctx,(const unsigned char *)p,n,salt,16,PV_VAULT_ROUNDS,32,key);
    mbedtls_md_free(&ctx);return r==0;
#endif
}
/* PVV1 || salt[16] || nonce[12] || encrypted JSON || tag[16].
 * The caller must supply 28 fresh bytes from the system CSPRNG on each save.
 * The version, salt and nonce are authenticated as AAD. */
static inline bool pv_vault_seal(const char *p,size_t n,const uint8_t entropy[28],
    const uint8_t *plain,size_t len,uint8_t *out,size_t cap,size_t *written){
    if(written)*written=0;
    if(!out||!written||!entropy||!plain||!len||len>PV_VAULT_MAX||cap<len+PV_VAULT_OVERHEAD)return false;
    uint8_t key[32]={0};bool ok=false;
    memcpy(out,"PVV1",4);memcpy(out+4,entropy,28);
    mbedtls_gcm_context g;mbedtls_gcm_init(&g);
    if(pv_vault_key(p,n,out+4,key)&&!mbedtls_gcm_setkey(&g,MBEDTLS_CIPHER_ID_AES,key,256))
        ok=mbedtls_gcm_crypt_and_tag(&g,MBEDTLS_GCM_ENCRYPT,len,out+20,12,out,PV_VAULT_HEADER,
            plain,out+PV_VAULT_HEADER,PV_VAULT_TAG,out+PV_VAULT_HEADER+len)==0;
    mbedtls_gcm_free(&g);pv_vault_wipe(key,sizeof(key));
    if(ok)*written=len+PV_VAULT_OVERHEAD;else pv_vault_wipe(out,cap);
    return ok;
}
static inline bool pv_vault_open(const char *p,size_t n,const uint8_t *in,size_t len,uint8_t *out,size_t cap,size_t *written){
    if(written)*written=0;
    if(!out||!written)return false;
    pv_vault_wipe(out,cap);
    if(!pv_vault_is(in,len)||len<=PV_VAULT_OVERHEAD||len>PV_VAULT_MAX+PV_VAULT_OVERHEAD||cap<len-PV_VAULT_OVERHEAD)return false;
    size_t size=len-PV_VAULT_OVERHEAD;uint8_t key[32]={0};bool ok=false;
    mbedtls_gcm_context g;mbedtls_gcm_init(&g);
    if(pv_vault_key(p,n,in+4,key)&&!mbedtls_gcm_setkey(&g,MBEDTLS_CIPHER_ID_AES,key,256))
        ok=mbedtls_gcm_auth_decrypt(&g,size,in+20,12,in,PV_VAULT_HEADER,in+PV_VAULT_HEADER+size,PV_VAULT_TAG,in+PV_VAULT_HEADER,out)==0;
    mbedtls_gcm_free(&g);pv_vault_wipe(key,sizeof(key));
    if(ok)*written=size;else pv_vault_wipe(out,cap);
    return ok;
}
#endif
