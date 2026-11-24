// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/qr_pair_crypto.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    char payload[132]={0},out[80]={0};
    uint8_t key[16];
    for(unsigned i=0;i<sizeof(key);i++)key[i]=(uint8_t)i;
    assert(fgets(payload,sizeof(payload),stdin));
    payload[strcspn(payload,"\r\n")]=0;
    assert(strlen(payload)==128);
    assert(plus_vita_pair_decode(key,(const unsigned char *)payload,strlen(payload),out,sizeof(out)));
    assert(strlen(out)==76);
    for(unsigned i=10;i<74;i++)assert(out[i]=='Z');
    key[0]^=1;
    assert(!plus_vita_pair_decode(key,(const unsigned char *)payload,strlen(payload),out,sizeof(out))&&out[0]==0);
    key[0]^=1;
    payload[40]=payload[40]=='A'?'B':'A';
    assert(!plus_vita_pair_decode(key,(const unsigned char *)payload,strlen(payload),out,sizeof(out))&&out[0]==0);
    assert(!plus_vita_pair_decode(key,(const unsigned char *)payload,127,out,sizeof(out)));
    puts("QR browser/native format, incorrect key, modified ciphertext and truncated payload: passed");
    return 0;
}
