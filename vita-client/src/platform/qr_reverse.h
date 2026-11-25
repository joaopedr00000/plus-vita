// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_QR_REVERSE_H
#define PLUS_VITA_QR_REVERSE_H
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
/* Accept only a bare 64-character token or a single-member Sony response.
 * Do not extract arbitrary substrings from a URL, HTML, duplicate JSON or text.
 * Optical raw mode is not encrypted and is never an automatic fallback. */
static inline bool pv_qr_space(unsigned char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
static inline bool pv_qr_alnum(unsigned char c){return (c>='0'&&c<='9')||(c>='A'&&c<='Z')||(c>='a'&&c<='z');}
static inline bool pv_qr_take(const unsigned char **p,const unsigned char *end,const char *text){
    size_t n=strlen(text);
    if((size_t)(end-*p)<n||memcmp(*p,text,n))return false;
    *p+=n;return true;
}
static inline void pv_qr_trim(const unsigned char **p,const unsigned char *end){while(*p<end&&pv_qr_space(**p))++*p;}
static inline bool pv_qr_reverse_decode(const unsigned char *data,size_t len,char *json,size_t cap){
    if(!json||cap<77)return false;
    memset(json,0,cap);
    if(!data||!len||len>512)return false;
    const unsigned char *p=data,*end=data+len;
    pv_qr_trim(&p,end);
    while(end>p&&pv_qr_space(end[-1]))--end;
    bool object=p<end&&*p=='{';
    if(object){
        ++p;pv_qr_trim(&p,end);
        if(!pv_qr_take(&p,end,"\"npsso\""))return false;
        pv_qr_trim(&p,end);
        if(!pv_qr_take(&p,end,":"))return false;
        pv_qr_trim(&p,end);
        if(!pv_qr_take(&p,end,"\""))return false;
    }
    if((size_t)(end-p)<64)return false;
    const unsigned char *token=p;
    for(size_t i=0;i<64;i++)if(!pv_qr_alnum(p[i]))return false;
    p+=64;
    if(object){
        if(!pv_qr_take(&p,end,"\""))return false;
        pv_qr_trim(&p,end);
        if(!pv_qr_take(&p,end,"}"))return false;
    }
    if(p!=end)return false;
    return snprintf(json,cap,"{\"npsso\":\"%.*s\"}",64,(const char *)token)==76;
}
#endif
