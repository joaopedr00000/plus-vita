// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_CUSTOM_THEME_H
#define PV_CUSTOM_THEME_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define PV_PRIMARY_DEFAULT 0xffcf40u
#define PV_SECONDARY_DEFAULT 0xb9892cu
static inline bool pv_theme_hex(const char *s,size_t n,uint32_t *out){
    if(!s||!out)return false;
    if(n==7&&s[0]=='#'){s++;n--;}
    if(n!=6)return false;
    uint32_t v=0;
    for(size_t k=0;k<n;k++){
        unsigned ch=(unsigned char)s[k],d;
        if(ch>='0'&&ch<='9')d=ch-'0';
        else if(ch>='a'&&ch<='f')d=ch-'a'+10;
        else if(ch>='A'&&ch<='F')d=ch-'A'+10;
        else return false;
        v=(v<<4)|d;
    }
    *out=v;return true;
}
static inline uint32_t pv_theme_pixel(uint32_t rgb){
    return 0xff000000u|((rgb&255u)<<16)|(rgb&0xff00u)|((rgb>>16)&255u);
}
static inline uint32_t pv_theme_ink(uint32_t p){
    unsigned lum=299u*(p&255u)+587u*((p>>8)&255u)+114u*((p>>16)&255u);
    return lum>=145000u?0xff0d1113u:0xfff0f6f8u;
}
#endif
