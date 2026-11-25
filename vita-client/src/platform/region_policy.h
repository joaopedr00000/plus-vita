// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_REGION_POLICY_H
#define PLUS_VITA_REGION_POLICY_H
#include <stdbool.h>
#include <string.h>
/* Legacy US/CA/PT settings migrate to opt-in same-account discovery only. */
static inline unsigned pv_region_mode(unsigned value){return value>=1&&value<=3?1:0;}
static inline const char *pv_region_label(unsigned value){
    return pv_region_mode(value)?"Regional discovery (account only): On":"Regional discovery (account only): Off";
}
static inline bool pv_region_account_locale_valid(const char *country,const char *language){
    if(!country||!language||strlen(country)!=2||strlen(language)!=2)return false;
    for(unsigned i=0;i<2;i++){
        unsigned char c=(unsigned char)country[i],l=(unsigned char)language[i];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z'))||
           !((l>='A'&&l<='Z')||(l>='a'&&l<='z')))return false;
    }
    return true;
}
/* A 404 is not an entitlement grant: only query the same actual account locale. */
static inline bool pv_region_fallback(unsigned mode,bool authorized,bool session,bool store_transport,long status){
    return pv_region_mode(mode)!=0&&authorized&&session&&store_transport&&status==404;
}
#endif
