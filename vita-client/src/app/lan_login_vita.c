// SPDX-License-Identifier: AGPL-3.0-only
#include "lan_login_vita.h"
#include "lan_login_page.h"
#include "../platform/lan_login_http.h"
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/rng.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <qrcodegen.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define PV_LAN_WINDOW 180000000ULL
static void wipe(void *v,size_t n){volatile unsigned char *p=v;while(n--)*p++=0;}
static bool active(uint64_t deadline){SceCtrlData p={0};sceCtrlPeekBufferPositive(0,&p,1);return !(p.buttons&SCE_CTRL_CIRCLE)&&sceKernelGetProcessTimeWide()<deadline;}
static bool nonblock(int s){int yes=1;return sceNetSetsockopt(s,SCE_NET_SOL_SOCKET,SCE_NET_SO_NBIO,&yes,sizeof(yes))>=0;}
static bool send_all(int s,const char *p,size_t n,uint64_t deadline){
    while(n&&active(deadline)){
        int r=sceNetSend(s,p,(unsigned)n,0);
        if(r>0){p+=r;n-=(size_t)r;}else if(!r)return false;else sceKernelDelayThread(10000);
    }
    return n==0;
}
static void reply(int s,bool page,bool accepted,uint64_t deadline){
    const char *body=page?pv_lan_page:accepted?"Received. Confirm on Vita.":"Invalid or expired pairing.";
    char h[640];int n=snprintf(h,sizeof(h),
        "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %u\r\nConnection: close\r\nCache-Control: no-store\r\nReferrer-Policy: no-referrer\r\nX-Content-Type-Options: nosniff\r\nX-Frame-Options: DENY\r\nContent-Security-Policy: default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'none'\r\n\r\n",
        page||accepted?"200 OK":"400 Bad Request",page?"text/html; charset=utf-8":"text/plain; charset=utf-8",(unsigned)strlen(body));
    if(n>0&&(size_t)n<sizeof(h)&&send_all(s,h,(size_t)n,deadline))send_all(s,body,strlen(body),deadline);
}
static bool draw_qr(PlusVitaPairFrame frame,void *ctx,const char *url){
    uint8_t tmp[qrcodegen_BUFFER_LEN_FOR_VERSION(9)],qr[qrcodegen_BUFFER_LEN_FOR_VERSION(9)],modules[53*53]={0};
    bool ok=qrcodegen_encodeText(url,tmp,qr,qrcodegen_Ecc_MEDIUM,1,9,qrcodegen_Mask_AUTO,true);
    if(ok){int n=qrcodegen_getSize(qr);for(int y=0;y<n;y++)for(int x=0;x<n;x++)modules[y*n+x]=qrcodegen_getModule(qr,x,y);
        ok=frame(ctx,"Sign in from your phone","Connect both devices to the same trusted Wi-Fi. Scan this QR, sign in on Sony, paste the Sony response on the local page. No downloads. Local HTTP is unencrypted. O Cancel. Expires in 3 minutes.",modules,n,NULL);}
    wipe(tmp,sizeof(tmp));wipe(qr,sizeof(qr));wipe(modules,sizeof(modules));return ok;
}
static bool confirm(PlusVitaPairFrame frame,void *ctx,uint64_t deadline){
    if(!frame(ctx,"Sign-in received","X Connect to Sony with the received token. O Cancel.",NULL,0,NULL))return false;
    SceCtrlData p={0};sceCtrlPeekBufferPositive(0,&p,1);unsigned prev=p.buttons;
    while(active(deadline)){sceCtrlPeekBufferPositive(0,&p,1);unsigned pressed=p.buttons&~prev;prev=p.buttons;if(pressed&SCE_CTRL_CROSS)return true;sceKernelDelayThread(20000);}
    return false;
}
PlusVitaAppResult plus_vita_lan_login(PlusVitaPairFrame frame,void *ctx,char *json,size_t cap){
    if(!frame||!json||cap<77)return PLUS_VITA_RESULT_UNSUPPORTED;
    wipe(json,cap);
    PlusVitaAppResult result=PLUS_VITA_RESULT_UNSUPPORTED;
    SceNetCtlInfo ip={0},mask={0};SceNetSockaddrIn address={0};unsigned int addrlen=sizeof(address);
    uint32_t netmask=0;char host[32]={0},origin[48]={0},url[96]={0},code[33]={0};
    uint8_t random[16]={0};char *request=NULL;int listener=-1;
    if(sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS,&ip)<0||sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_NETMASK,&mask)<0)goto done;
    if(sceNetInetPton(SCE_NET_AF_INET,ip.ip_address,&address.sin_addr)<=0||sceNetInetPton(SCE_NET_AF_INET,mask.netmask,&netmask)<=0)goto done;
    uint32_t local=sceNetNtohl(address.sin_addr.s_addr),subnet=sceNetNtohl(netmask);
    if(!pv_lan_private(local)||!subnet||subnet==UINT32_MAX||((~subnet)&((~subnet)+1u)))goto done;
    if(sceKernelGetRandomNumber(random,sizeof(random))<0)goto done;
    unsigned nonzero=0;for(size_t i=0;i<sizeof(random);i++){nonzero|=random[i];snprintf(code+2*i,3,"%02x",(unsigned)random[i]);}
    if(!nonzero)goto done;
    address.sin_len=sizeof(address);address.sin_family=SCE_NET_AF_INET;address.sin_port=0;
    listener=sceNetSocket("PVLocalLogin",SCE_NET_AF_INET,SCE_NET_SOCK_STREAM,SCE_NET_IPPROTO_TCP);
    if(listener<0||!nonblock(listener)||sceNetBind(listener,(const SceNetSockaddr *)&address,sizeof(address))<0||sceNetListen(listener,2)<0||sceNetGetsockname(listener,(SceNetSockaddr *)&address,&addrlen)<0)goto done;
    unsigned port=sceNetNtohs(address.sin_port);if(!port)goto done;
    int n=snprintf(host,sizeof(host),"%s:%u",ip.ip_address,port);if(n<=0||(size_t)n>=sizeof(host))goto done;
    n=snprintf(origin,sizeof(origin),"http://%s",host);if(n<=0||(size_t)n>=sizeof(origin))goto done;
    n=snprintf(url,sizeof(url),"%s/#%s",origin,code);if(n<=0||(size_t)n>=sizeof(url))goto done;
    request=calloc(PV_LAN_MAX,1);if(!request)goto done;
    uint64_t deadline=sceKernelGetProcessTimeWide()+PV_LAN_WINDOW;
    if(!draw_qr(frame,ctx,url))goto done;
    result=PLUS_VITA_RESULT_CANCELLED;
    unsigned attempts=0;
    while(active(deadline)&&attempts<40u){
        SceNetSockaddrIn peer={0};unsigned int plen=sizeof(peer);
        int client=sceNetAccept(listener,(SceNetSockaddr *)&peer,&plen);
        if(client<0){sceKernelDelayThread(20000);continue;}
        ++attempts;uint32_t remote=sceNetNtohl(peer.sin_addr.s_addr);
        if(peer.sin_family!=SCE_NET_AF_INET||!pv_lan_private(remote)||(remote&subnet)!=(local&subnet)||!nonblock(client)){sceNetSocketClose(client);continue;}
        uint64_t until=sceKernelGetProcessTimeWide()+5000000ULL;if(until>deadline)until=deadline;
        size_t used=0;PvLanRequest state=PV_LAN_MORE;
        while(active(until)&&state==PV_LAN_MORE&&used<PV_LAN_MAX){
            int got=sceNetRecv(client,request+used,(unsigned)(PV_LAN_MAX-used),0);
            if(got>0){used+=(size_t)got;state=pv_lan_parse(request,used,host,origin,code,json,cap);}
            else if(!got)break;else sceKernelDelayThread(10000);
        }
        if(state==PV_LAN_ACCEPT){
            reply(client,false,true,until);sceNetSocketClose(client);sceNetSocketClose(listener);listener=-1;
            wipe(request,PV_LAN_MAX);wipe(code,sizeof(code));wipe(url,sizeof(url));
            result=confirm(frame,ctx,deadline)?PLUS_VITA_RESULT_OK:PLUS_VITA_RESULT_CANCELLED;goto done;
        }
        reply(client,state==PV_LAN_PAGE,false,until);sceNetSocketClose(client);wipe(request,PV_LAN_MAX);
    }
done:
    if(listener>=0)sceNetSocketClose(listener);
    if(request){wipe(request,PV_LAN_MAX);free(request);}
    wipe(random,sizeof(random));wipe(code,sizeof(code));wipe(url,sizeof(url));wipe(host,sizeof(host));wipe(origin,sizeof(origin));
    if(result!=PLUS_VITA_RESULT_OK)wipe(json,cap);
    frame(ctx,"Local login finished","Returning to Plus-Vita.",NULL,0,NULL);
    frame(ctx,"Local login finished","Returning to Plus-Vita.",NULL,0,NULL);
    return result;
}
