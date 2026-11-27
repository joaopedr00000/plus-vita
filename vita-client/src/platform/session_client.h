// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_SESSION_CLIENT_H
#define PV_SESSION_CLIENT_H
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
static inline bool pv_session_uuid(const char *s){
    if(!s||strlen(s)!=36)return false;
    for(unsigned j=0;j<36;j++){char c=s[j];if(j==8||j==13||j==18||j==23){if(c!='-')return false;}else if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;}
    return true;
}
static inline bool pv_session_use_client(char *dst,size_t cap,const char *candidate){
    if(cap<37||!pv_session_uuid(candidate))return false;
    memcpy(dst,candidate,37);return true;
}
#endif
