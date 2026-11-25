// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_LAN_LOGIN_HTTP_H
#define PV_LAN_LOGIN_HTTP_H
#include "qr_reverse.h"
#include <stdint.h>
#define PV_LAN_MAX 8192u
typedef enum { PV_LAN_MORE,PV_LAN_BAD,PV_LAN_PAGE,PV_LAN_ACCEPT } PvLanRequest;
static inline bool pv_lan_private(uint32_t ip){return ip>>24==10u||ip>>20==0xac1u||ip>>16==0xc0a8u;}
static inline bool pv_lan_eq(const char *p,size_t n,const char *s){return strlen(s)==n&&!memcmp(p,s,n);}
static inline bool pv_lan_name(const char *p,size_t n,const char *s){
    if(strlen(s)!=n)return false;
    for(size_t i=0;i<n;i++){char c=p[i];if(c>='A'&&c<='Z')c=(char)(c+32);if(c!=s[i])return false;}
    return true;
}
static inline PvLanRequest pv_lan_parse(const char *buf,size_t len,const char *host,const char *origin,const char *code,char *json,size_t cap){
    if(!json||cap<77)return PV_LAN_BAD;
    memset(json,0,cap);
    if(!buf||!host||!origin||!code||strlen(code)!=32||len>PV_LAN_MAX||memchr(buf,0,len))return PV_LAN_BAD;
    size_t h=0;
    for(size_t i=3;i<len;i++)if(!memcmp(buf+i-3,"\r\n\r\n",4)){h=i+1;break;}
    if(!h)return len==PV_LAN_MAX?PV_LAN_BAD:PV_LAN_MORE;
    const char *end=buf+h,*p=buf,*e=p;
    while(e+1<end&&memcmp(e,"\r\n",2))++e;
    if(e+1>=end)return PV_LAN_BAD;
    bool get=pv_lan_eq(p,(size_t)(e-p),"GET / HTTP/1.1"),post=pv_lan_eq(p,(size_t)(e-p),"POST /connect HTTP/1.1");
    if(!get&&!post)return PV_LAN_BAD;
    p=e+2;
    bool hh=false,ho=false,hl=false,ht=false;size_t body=0;
    while(p<end-2){
        e=p;while(e+1<end&&memcmp(e,"\r\n",2))++e;
        if(e+1>=end)return PV_LAN_BAD;
        const char *c=memchr(p,':',(size_t)(e-p));
        if(!c||c==p)return PV_LAN_BAD;
        for(const char *x=p;x<c;x++)if(!pv_qr_alnum((unsigned char)*x)&&*x!='-')return PV_LAN_BAD;
        const char *v=c+1,*last=e;
        while(v<last&&(*v==' '||*v=='\t'))++v;
        while(last>v&&(last[-1]==' '||last[-1]=='\t'))--last;
        size_t k=(size_t)(c-p),n=(size_t)(last-v);
        for(const char *x=v;x<last;x++)if((unsigned char)*x<32u&&*x!='\t')return PV_LAN_BAD;
        if(pv_lan_name(p,k,"host")){if(hh||!pv_lan_eq(v,n,host))return PV_LAN_BAD;hh=true;}
        else if(pv_lan_name(p,k,"origin")){if(ho||!pv_lan_eq(v,n,origin))return PV_LAN_BAD;ho=true;}
        else if(pv_lan_name(p,k,"content-length")){
            if(hl||!n||n>3)return PV_LAN_BAD;
            hl=true;for(size_t i=0;i<n;i++){if(v[i]<'0'||v[i]>'9')return PV_LAN_BAD;body=body*10u+(unsigned)(v[i]-'0');}
        }else if(pv_lan_name(p,k,"content-type")){
            if(ht||(!pv_lan_eq(v,n,"text/plain")&&!pv_lan_eq(v,n,"text/plain;charset=UTF-8")&&!pv_lan_eq(v,n,"text/plain; charset=UTF-8")))return PV_LAN_BAD;
            ht=true;
        }else if(pv_lan_name(p,k,"transfer-encoding")||pv_lan_name(p,k,"expect"))return PV_LAN_BAD;
        p=e+2;
    }
    if(!hh)return PV_LAN_BAD;
    if(get)return !body&&len==h?PV_LAN_PAGE:PV_LAN_BAD;
    if(!ho||!hl||!ht||body!=96)return PV_LAN_BAD;
    if(len<h+body)return PV_LAN_MORE;
    if(len!=h+body)return PV_LAN_BAD;
    unsigned mismatch=0;for(size_t i=0;i<32;i++)mismatch|=(unsigned char)buf[h+i]^(unsigned char)code[i];
    if(mismatch)return PV_LAN_BAD;
    return pv_qr_reverse_decode((const unsigned char *)buf+h+32,64,json,cap)?PV_LAN_ACCEPT:PV_LAN_BAD;
}
#endif
