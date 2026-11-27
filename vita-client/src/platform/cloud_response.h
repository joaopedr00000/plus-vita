// SPDX-License-Identifier: AGPL-3.0-only
#ifndef VP_CLOUD_RESPONSE_H
#define VP_CLOUD_RESPONSE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <json_object.h>
#include <json_tokener.h>

/* Borrowed array. Accept known API envelope variants, never synthesize rows. */
static inline struct json_object *vp_cloud_dc_array(struct json_object *root)
{
    if(!root) return NULL;
    if(json_object_is_type(root,json_type_array)) return root;
    if(!json_object_is_type(root,json_type_object)) return NULL;
    static const char *const keys[]={"dataCenters","datacenters","dataCenterList","datacenterList"};
    struct json_object *list=NULL;
    for(size_t n=0;n<sizeof(keys)/sizeof(keys[0]);n++){
        struct json_object *v=NULL;
        if(!json_object_object_get_ex(root,keys[n],&v)) continue;
        if(list || !v || !json_object_is_type(v,json_type_array)) return NULL;
        list=v;
    }
    return list;
}

/* Session-specific client IDs are supplied by the service, never invented. */
static inline bool vp_cloud_uuid(const char *s)
{
    if(!s || strlen(s)!=36) return false;
    for(unsigned n=0;n<36;n++){
        char c=s[n];
        if(n==8||n==13||n==18||n==23){if(c!='-')return false;}
        else if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))) return false;
    }
    return true;
}
static inline bool vp_cloud_session_ids(struct json_object *root,char cloud[128],char stream[128])
{
    const char *keys[]={"gkClientId","streamServerClientId"};
    char *dst[]={cloud,stream};
    if(!root || !json_object_is_type(root,json_type_object)) return false;
    /* Con 0.5.2 validates each returned ID independently. Invalid or absent
     * values must not replace the original ID or invalidate the session. */
    for(unsigned n=0;n<2;n++){
        struct json_object *v=NULL;
        if(!json_object_object_get_ex(root,keys[n],&v))continue;
        if(!v || !json_object_is_type(v,json_type_string) || json_object_get_string_len(v)!=36) continue;
        const char *next=json_object_get_string(v);
        if(vp_cloud_uuid(next))memcpy(dst[n],next,37);
    }
    return true;
}

/* Decode the opaque OAuth query value exactly once. '+' is literal in a URI
 * query (not a form body). Reject duplicates, empty codes, bad escapes,
 * control characters, and unreasonable lengths. Never log the value. */
static inline int vp_cloud_hex(unsigned char c)
{
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}
static inline char *vp_cloud_oauth_code(const char *url)
{
    if(!url||strlen(url)>16384)return NULL;
    const char *q=strchr(url,'?'),*fragment=strchr(url,'#');
    if(!q||(fragment&&fragment<q))return NULL;
    const char *end=fragment?fragment:url+strlen(url);
    const char *value=NULL;size_t len=0;
    for(const char *p=q+1;p<end;){
        const char *next=memchr(p,'&',(size_t)(end-p));if(!next)next=end;
        if(next-p>=5&&!memcmp(p,"code=",5)){
            if(value)return NULL;
            value=p+5;len=(size_t)(next-value);
        }
        if(next==end)break;
        p=next+1;
    }
    if(!value||!len||len>12288)return NULL;
    char *out=malloc(len+1);if(!out)return NULL;
    size_t n=0;
    for(size_t j=0;j<len;j++){
        unsigned char ch=(unsigned char)value[j];
        if(ch=='%'){
            if(j+2>=len)goto invalid;
            int hi=vp_cloud_hex((unsigned char)value[j+1]),lo=vp_cloud_hex((unsigned char)value[j+2]);
            if(hi<0||lo<0)goto invalid;
            ch=(unsigned char)((hi<<4)|lo);j+=2;
        }
        if(ch<33||ch>126||n>=4096)goto invalid;
        out[n++]=(char)ch;
    }
    out[n]=0;return out;
invalid:
    {volatile char *wipe=out;for(size_t j=0;j<n;j++)wipe[j]=0;}
    free(out);return NULL;
}
static inline bool vp_cloud_key_valid(const char *key,size_t len)
{
    if(!key||!len||len>=16384)return false;
    for(size_t i=0;i<len;i++)if((unsigned char)key[i]<32||(unsigned char)key[i]>126)return false;
    return true;
}
/* Only an explicit sessionKey in a successful JSON response may rotate a
 * session key. Header takes precedence; absent/malformed values are ignored.
 * No secret or raw response body crosses the diagnostic boundary. */
static inline char *vp_cloud_response_key(const char *header,const char *body)
{
    if(header&&*header){
        size_t n=strlen(header);if(!vp_cloud_key_valid(header,n))return NULL;
        char *copy=malloc(n+1);if(copy)memcpy(copy,header,n+1);return copy;
    }
    if(!body||strlen(body)>65536)return NULL;
    struct json_object *root=json_tokener_parse(body),*v=NULL;
    char *copy=NULL;
    if(root&&json_object_is_type(root,json_type_object)&&json_object_object_get_ex(root,"sessionKey",&v)&&v&&json_object_is_type(v,json_type_string)){
        const char *s=json_object_get_string(v);size_t n=(size_t)json_object_get_string_len(v);
        if(vp_cloud_key_valid(s,n)){copy=malloc(n+1);if(copy){memcpy(copy,s,n);copy[n]=0;}}
    }
    if(root)json_object_put(root);
    return copy;
}

/* Finite numeric categories only: never persist response text or credentials. */
static inline unsigned vp_cloud_event_field(struct json_object *row)
{
    if(!row || !json_object_is_type(row,json_type_object))return 0;
    struct json_object *v=NULL;
    if(json_object_object_get_ex(row,"eventCode",&v) && v && json_object_is_type(v,json_type_string)){
        const char *s=json_object_get_string(v);
        if(!strcmp(s,"002.0011"))return 1; /* authorization rejected */
        if(!strcmp(s,"002.0019"))return 2; /* ping eligibility */
        if(!strcmp(s,"002.2001"))return 3; /* subscription required */
    }
    static const char *const names[]={"noGameForEntitlementId","invalidGameSpecification","noAvailableDatacenter","sessionNotLocked","allocationFailed","deviceNotActivated","notAuthorized","noCapacity",
        "invalidAuthCode","invalidRequest","missingEntitlementId","invalidRequestBody","AUTHORIZATION_FAILED","noGameForEntitlement","invalidSession"};
    const char *const keys[]={"reason","name","error","code"};
    for(unsigned k=0;k<sizeof(keys)/sizeof(keys[0]);k++)if(json_object_object_get_ex(row,keys[k],&v)&&v&&json_object_is_type(v,json_type_string)){
        const char *s=json_object_get_string(v);
        for(unsigned n=0;n<sizeof(names)/sizeof(names[0]);n++)if(!strcmp(s,names[n]))return n+4;
    }
    /* Generic descriptions convey only the broad category, not its cause. */
    if(json_object_object_get_ex(row,"description",&v)&&v&&json_object_is_type(v,json_type_string)){
        const char *s=json_object_get_string(v);
        if(!strcmp(s,"Bad Request"))return 19;
        if(!strcmp(s,"Unauthorized"))return 20;
    }
    return 0;
}
static inline unsigned vp_cloud_event(const char *s)
{
    if(!s || strlen(s)>8192)return 0;
    if(!strcmp(s,"002.0011"))return 1;
    if(!strcmp(s,"002.0019"))return 2;
    if(!strcmp(s,"002.2001"))return 3;
    struct json_object *root=json_tokener_parse(s);
    if(!root)return 0;
    unsigned code=vp_cloud_event_field(root);
    struct json_object *errors=NULL;
    if(!code&&json_object_object_get_ex(root,"errors",&errors)&&errors&&json_object_is_type(errors,json_type_array)){
        size_t count=json_object_array_length(errors);
        for(size_t n=0;n<count&&n<16&&!code;n++)code=vp_cloud_event_field(json_object_array_get_idx(errors,n));
    }
    if(!code){
        struct json_object *v=NULL;
        if(json_object_object_get_ex(root,"componentCode",&v)&&v&&json_object_is_type(v,json_type_string)&&!strcmp(json_object_get_string(v),"002"))code=21;
    }
    json_object_put(root);
    return code;
}
#endif
