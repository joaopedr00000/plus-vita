// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_UPDATE_HEAD_H
#define PV_UPDATE_HEAD_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
typedef bool (*PvSha1)(const unsigned char *,size_t,unsigned char[20]);
static inline uint32_t pv_head_be32(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static inline bool pv_head_span(size_t n,size_t at,size_t len){return at<=n&&len<=n-at;}
/* Homebrew package digest format, as documented/implemented by VitaShell
 * (TheFloW, GPL-3.0-or-later, package_installer.c). Use only the installed
 * same-title template; no foreign package header or license is embedded. */
static inline bool pv_head_digest(PvSha1 sha,const unsigned char *p,size_t len,unsigned char out[16]){
    unsigned char first[20],last[20],mix[64]={0};
    if(!sha(p,len,first))return false;
    memcpy(mix,first+4,8);memcpy(mix+8,first+4,8);memcpy(mix+16,first+12,4);
    mix[20]=first[16];mix[21]=first[1];mix[22]=first[2];mix[23]=first[3];
    memcpy(mix+24,mix+16,8);
    if(!sha(mix,sizeof(mix),last))return false;
    memcpy(out,last,16);return true;
}
static inline bool pv_head_rebuild(unsigned char *h,size_t n,const char *content,PvSha1 sha){
    if(!h||!content||!sha||n<0xec||strlen(content)>=48)return false;
    uint32_t a=pv_head_be32(h+0xd0),b=pv_head_be32(h+8),l=pv_head_be32(h+0x10),o=pv_head_be32(h+0xd4),c=pv_head_be32(h+0xe8);
    if(a<0xec||c<0xec||l<64||!pv_head_span(n,0,a)||!pv_head_span(n,a,16)||!pv_head_span(n,b,l-64)||!pv_head_span(n,o,16)||!pv_head_span(n,c,16)||o<0xec)return false;
    memset(h+0x30,0,48);memcpy(h+0x30,content,strlen(content));
    unsigned char digest[16];
    if(!pv_head_digest(sha,h,a,digest))return false;
    memcpy(h+a,digest,16);
    if(!pv_head_digest(sha,h+b,l-64,digest))return false;
    memcpy(h+o,digest,16);
    if(!pv_head_digest(sha,h,c,digest))return false;
    memcpy(h+c,digest,16);
    return true;
}
#endif
