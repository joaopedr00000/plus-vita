// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/account_vault.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    uint8_t entropy[28]={0},cipher[256],out[256],second[256];size_t n=0,m=0,k=0;
    for(unsigned j=0;j<28;j++)entropy[j]=(uint8_t)(j+1);
    const char pass[]="example-local-phrase",bad[]="different-phrase";
    const uint8_t data[]="synthetic account data";
    assert(!pv_vault_password("123456",6));
    assert(pv_vault_seal(pass,strlen(pass),entropy,data,sizeof(data)-1,cipher,sizeof(cipher),&n));
    assert(n==sizeof(data)-1+PV_VAULT_OVERHEAD&&pv_vault_is(cipher,n));
    assert(pv_vault_open(pass,strlen(pass),cipher,n,out,sizeof(out),&m)&&m==sizeof(data)-1&&!memcmp(out,data,m));
    assert(!pv_vault_open(bad,strlen(bad),cipher,n,out,sizeof(out),&m));
    for(unsigned j=0;j<sizeof(out);j++)assert(out[j]==0);
    cipher[PV_VAULT_HEADER]^=1;
    assert(!pv_vault_open(pass,strlen(pass),cipher,n,out,sizeof(out),&m));
    cipher[PV_VAULT_HEADER]^=1;
    assert(!pv_vault_open(pass,strlen(pass),cipher,n-1,out,sizeof(out),&m));
    entropy[27]^=1;
    assert(pv_vault_seal(pass,strlen(pass),entropy,data,sizeof(data)-1,second,sizeof(second),&k)&&k==n&&memcmp(second,cipher,n));
    assert(pv_vault_open(pass,strlen(pass),second,k,out,sizeof(out),&m));
    puts("Vault encryption, wrong password, tamper, truncation and nonce tests passed");return 0;
}
