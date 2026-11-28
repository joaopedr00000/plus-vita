// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_UPDATE_HANDOFF_H
#define PV_UPDATE_HANDOFF_H
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#define PV_MAIN_TITLE "PLVTB0001"
#define PV_HELPER_TITLE "PLVTU0001"
#define PV_UPDATE_LIMIT (128u*1024u*1024u)
typedef struct {unsigned size,helper_size;char hash[65],helper_hash[65],appver[6];} PvHandoffJob;
static inline bool pv_handoff_version(const char *s){
    return s&&strlen(s)==5&&s[2]=='.'&&s[0]>='0'&&s[0]<='9'&&s[1]>='0'&&s[1]<='9'&&s[3]>='0'&&s[3]<='9'&&s[4]>='0'&&s[4]<='9';
}
static inline bool pv_handoff_hash(const char *s){
    if(!s||strlen(s)!=64)return false;
    for(unsigned j=0;j<64;j++)if(!((s[j]>='0'&&s[j]<='9')||(s[j]>='a'&&s[j]<='f')))return false;
    return true;
}
static inline bool pv_handoff_valid(const PvHandoffJob *j){
    return j&&j->size&&j->size<=PV_UPDATE_LIMIT&&j->helper_size&&j->helper_size<=PV_UPDATE_LIMIT&&pv_handoff_hash(j->hash)&&pv_handoff_hash(j->helper_hash)&&pv_handoff_version(j->appver);
}
static inline bool pv_handoff_format(const PvHandoffJob *j,char *out,size_t cap){
    if(!pv_handoff_valid(j))return false;
    int n=snprintf(out,cap,"PVH1 %u %s %u %s %s\n",j->size,j->hash,j->helper_size,j->helper_hash,j->appver);
    return n>0&&(size_t)n<cap;
}
static inline bool pv_handoff_parse(const char *s,PvHandoffJob *out){
    if(!s||!out)return false;
    PvHandoffJob j={0};char trailing,canonical[200];
    if(sscanf(s,"PVH1 %9u %64[0-9a-f] %9u %64[0-9a-f] %5[0-9.] %c",&j.size,j.hash,&j.helper_size,j.helper_hash,j.appver,&trailing)!=5||!pv_handoff_format(&j,canonical,sizeof(canonical))||strcmp(s,canonical))return false;
    *out=j;return true;
}
#endif
