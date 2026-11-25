// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/lan_login_http.h"
#include <assert.h>
int main(void){
    const char *host="192.168.1.2:25525",*origin="http://192.168.1.2:25525",*code="0123456789abcdef0123456789abcdef";
    char req[1024],token[65],json[80];memset(token,'Z',64);token[64]=0;
    assert(pv_lan_private(0xc0a80102u)&&pv_lan_private(0x0a000001u)&&pv_lan_private(0xac1f0001u));
    assert(!pv_lan_private(0x08080808u)&&!pv_lan_private(0xac200001u));
    int n=snprintf(req,sizeof(req),"GET / HTTP/1.1\r\nHost: %s\r\n\r\n",host);
    assert(pv_lan_parse(req,(size_t)n,host,origin,code,json,sizeof(json))==PV_LAN_PAGE);
    n=snprintf(req,sizeof(req),"POST /connect HTTP/1.1\r\nHost: %s\r\nOrigin: %s\r\nContent-Type: text/plain\r\nContent-Length: 96\r\n\r\n%s%s",host,origin,code,token);
    assert(pv_lan_parse(req,(size_t)n-1,host,origin,code,json,sizeof(json))==PV_LAN_MORE);
    assert(pv_lan_parse(req,(size_t)n,host,origin,code,json,sizeof(json))==PV_LAN_ACCEPT);
    assert(strlen(json)==76&&!memcmp(json+10,token,64));
    assert(pv_lan_parse(req,(size_t)n,host,origin,"1123456789abcdef0123456789abcdef",json,sizeof(json))==PV_LAN_BAD);
    assert(json[0]==0);
    assert(pv_lan_parse(req,(size_t)n,host,"http://evil.invalid",code,json,sizeof(json))==PV_LAN_BAD);
    assert(pv_lan_parse(req,(size_t)n,"evil.invalid",origin,code,json,sizeof(json))==PV_LAN_BAD);
    n=snprintf(req,sizeof(req),"POST /connect HTTP/1.1\r\nHost: %s\r\nOrigin: %s\r\nContent-Type: text/plain\r\nContent-Length: 96\r\nContent-Length: 96\r\n\r\n%s%s",host,origin,code,token);
    assert(pv_lan_parse(req,(size_t)n,host,origin,code,json,sizeof(json))==PV_LAN_BAD);
    n=snprintf(req,sizeof(req),"POST /connect HTTP/1.1\r\nHost: %s\r\nOrigin: %s\r\nContent-Type: text/plain\r\nContent-Length: 96\r\nTransfer-Encoding: chunked\r\n\r\n%s%s",host,origin,code,token);
    assert(pv_lan_parse(req,(size_t)n,host,origin,code,json,sizeof(json))==PV_LAN_BAD);
    puts("LAN login HTTP parser passed");return 0;
}
