// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/qr_reverse.h"
#include <assert.h>
int main(void){
    char token[65],json[80],src[256];
    memset(token,'Z',64);token[64]=0;
    assert(pv_qr_reverse_decode((const unsigned char *)token,64,json,sizeof(json)));
    assert(strlen(json)==76&&!memcmp(json+10,token,64));
    snprintf(src,sizeof(src)," { \"npsso\" : \"%s\" } \n",token);
    assert(pv_qr_reverse_decode((const unsigned char *)src,strlen(src),json,sizeof(json)));
    snprintf(src,sizeof(src),"{\"npsso\":\"%s\",\"npsso\":\"%s\"}",token,token);
    assert(!pv_qr_reverse_decode((const unsigned char *)src,strlen(src),json,sizeof(json)));
    assert(json[0]==0);
    snprintf(src,sizeof(src),"https://example.invalid/?token=%s",token);
    assert(!pv_qr_reverse_decode((const unsigned char *)src,strlen(src),json,sizeof(json)));
    assert(!pv_qr_reverse_decode((const unsigned char *)token,63,json,sizeof(json)));
    assert(!pv_qr_reverse_decode((const unsigned char *)token,65,json,sizeof(json)));
    token[10]='-';
    assert(!pv_qr_reverse_decode((const unsigned char *)token,64,json,sizeof(json)));
    assert(!pv_qr_reverse_decode(NULL,64,json,sizeof(json)));
    assert(!pv_qr_reverse_decode((const unsigned char *)src,513,json,sizeof(json)));
    puts("reverse QR strict token validation passed");
    return 0;
}
