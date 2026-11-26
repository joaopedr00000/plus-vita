// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_UPDATE_POLICY_H
#define PV_UPDATE_POLICY_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {unsigned major,minor,patch,beta;} PvVersion;
static inline bool pv_version_number(const char **s,unsigned *n){
    const char *p=*s;unsigned v=0;
    if(*p<'0'||*p>'9'||(*p=='0'&&p[1]>='0'&&p[1]<='9'))return false;
    do {unsigned digit=(unsigned)(*p++-'0');if(v>(9999u-digit)/10u)return false;v=v*10u+digit;}while(*p>='0'&&*p<='9');
    *n=v;*s=p;return true;
}
static inline bool pv_version_parse(const char *s,PvVersion *v){
    if(!s||!v)return false;
    if(*s=='v')s++;
    PvVersion x={0};
    if(!pv_version_number(&s,&x.major)||*s++!='.'||!pv_version_number(&s,&x.minor)||*s++!='.'||!pv_version_number(&s,&x.patch))return false;
    if(*s){if(s[0]!='-'||s[1]!='b')return false;s+=2;if(!pv_version_number(&s,&x.beta)||!x.beta)return false;}
    if(*s)return false;
    *v=x;return true;
}
static inline int pv_version_cmp(PvVersion a,PvVersion b){
    const unsigned aa[]={a.major,a.minor,a.patch,a.beta},bb[]={b.major,b.minor,b.patch,b.beta};
    for(unsigned j=0;j<4;j++)if(aa[j]!=bb[j])return aa[j]>bb[j]?1:-1;
    return 0;
}
static inline bool pv_update_eligible(PvVersion installed,PvVersion candidate,bool beta_channel,bool prerelease,bool draft){
    return !draft&&((candidate.beta!=0)==prerelease)&&(beta_channel||!candidate.beta)&&pv_version_cmp(candidate,installed)>0;
}
static inline void pv_version_tag(PvVersion v,char out[40]){
    if(v.beta)snprintf(out,40,"v%u.%u.%u-b%u",v.major,v.minor,v.patch,v.beta);
    else snprintf(out,40,"v%u.%u.%u",v.major,v.minor,v.patch);
}
static inline void pv_version_display(PvVersion v,char out[40]){
    if(v.beta)snprintf(out,40,"%u.%u.%u B%u",v.major,v.minor,v.patch,v.beta);
    else snprintf(out,40,"%u.%u.%u",v.major,v.minor,v.patch);
}
static inline bool pv_update_hex(char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');}
static inline bool pv_update_checksum(const char *text,const char *name,char out[65]){
    if(!text||!name)return false;
    bool found=false;
    while(*text){
        const char *end=strchr(text,'\n');if(!end)end=text+strlen(text);
        const char *p=text;size_t n=(size_t)(end-p);if(n&&p[n-1]=='\r')n--;
        if(n>=66&&p[64]==' '&&(p[65]==' '||p[65]=='*')&&n-66==strlen(name)&&!memcmp(p+66,name,n-66)){
            if(found)return false;
            for(unsigned j=0;j<64;j++){if(!pv_update_hex(p[j]))return false;out[j]=p[j]>='A'&&p[j]<='F'?(char)(p[j]+32):p[j];}
            out[64]=0;found=true;
        }
        text=*end?end+1:end;
    }
    return found;
}
static inline bool pv_update_safe_path(const char *p){
    if(!p||!*p||strlen(p)>180)return false;
    if(strcmp(p,"eboot.bin")&&strcmp(p,"cacert.pem")&&strncmp(p,"sce_sys/",8)&&strncmp(p,"branding/",9)&&strncmp(p,"fonts/",6)&&strncmp(p,"licenses/",9))return false;
    const char *start=p;
    for(;;p++){
        unsigned char c=(unsigned char)*p;
        if(!c||c=='/'){
            size_t n=(size_t)(p-start);if(!n||(n==1&&*start=='.')||(n==2&&start[0]=='.'&&start[1]=='.'))return false;
            if(!c)return true;
            start=p+1;
        }else if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return false;
    }
}
#endif
