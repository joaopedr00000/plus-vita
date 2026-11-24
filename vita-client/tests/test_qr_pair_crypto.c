// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/qr_pair_crypto.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void read_payload(char p[176])
{
    assert(fgets(p,176,stdin));p[strcspn(p,"\r\n")]=0;assert(strlen(p)==PLUS_VITA_PAIR_PAYLOAD_LEN);
}
int main(void)
{
    char p[176]={0},q[176]={0},out[80]={0};
    uint8_t entropy[32],key1[32],key2[32],key3[32],zero[32]={0};
    const uint8_t d1[6]={2,0,0,0,0,1},d2[6]={2,0,0,0,0,2};
    for(unsigned i=0;i<sizeof(entropy);i++)entropy[i]=(uint8_t)(i+1);
    assert(plus_vita_pair_make_key(entropy,d1,key1));
    assert(plus_vita_pair_make_key(entropy,d2,key2));
    assert(memcmp(key1,key2,32)!=0);
    entropy[0]^=1;assert(plus_vita_pair_make_key(entropy,d1,key3));assert(memcmp(key1,key3,32)!=0);
    assert(!plus_vita_pair_make_key(zero,d1,key3));
    read_payload(p);read_payload(q);assert(strcmp(p,q)!=0);
    assert(plus_vita_pair_decode(key1,(const unsigned char *)p,strlen(p),out,sizeof(out)));
    assert(strlen(out)==76);for(unsigned i=10;i<74;i++)assert(out[i]=='Z');
    assert(plus_vita_pair_decode(key2,(const unsigned char *)q,strlen(q),out,sizeof(out)));
    assert(!plus_vita_pair_decode(key1,(const unsigned char *)q,strlen(q),out,sizeof(out))&&out[0]==0);
    assert(!plus_vita_pair_decode(key2,(const unsigned char *)p,strlen(p),out,sizeof(out))&&out[0]==0);
    p[60]=p[60]=='A'?'B':'A'; /* mutate ciphertext, not session ID */
    assert(!plus_vita_pair_decode(key1,(const unsigned char *)p,strlen(p),out,sizeof(out))&&out[0]==0);
    assert(!plus_vita_pair_decode(key1,(const unsigned char *)p,171,out,sizeof(out)));
    puts("QR per-device key, per-session entropy, two-device interoperability and cross-session rejection: passed");
    return 0;
}
