// SPDX-License-Identifier: AGPL-3.0-only
#ifndef VP_CLOUD_RESPONSE_H
#define VP_CLOUD_RESPONSE_H
#include <stdbool.h>
#include <stddef.h>
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
    const char *next[2]={NULL,NULL};
    if(!root || !json_object_is_type(root,json_type_object)) return false;
    for(unsigned n=0;n<2;n++){
        struct json_object *v=NULL;
        if(!json_object_object_get_ex(root,keys[n],&v))continue;
        if(!v || !json_object_is_type(v,json_type_string) || json_object_get_string_len(v)!=36) return false;
        next[n]=json_object_get_string(v);
        if(!vp_cloud_uuid(next[n]))return false;
    }
    for(unsigned n=0;n<2;n++)if(next[n])memcpy(dst[n],next[n],37);
    return true;
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
    static const char *const names[]={"noGameForEntitlementId","invalidGameSpecification","noAvailableDatacenter","sessionNotLocked","allocationFailed","deviceNotActivated","notAuthorized","noCapacity"};
    const char *const keys[]={"reason","name"};
    for(unsigned k=0;k<2;k++)if(json_object_object_get_ex(row,keys[k],&v)&&v&&json_object_is_type(v,json_type_string)){
        const char *s=json_object_get_string(v);
        for(unsigned n=0;n<sizeof(names)/sizeof(names[0]);n++)if(!strcmp(s,names[n]))return n+4;
    }
    return 0;
}
static inline unsigned vp_cloud_event(const char *s)
{
    if(!s || strlen(s)>8192)return 0;
    struct json_object *root=json_tokener_parse(s);
    if(!root)return 0;
    unsigned code=vp_cloud_event_field(root);
    struct json_object *errors=NULL;
    if(!code&&json_object_object_get_ex(root,"errors",&errors)&&errors&&json_object_is_type(errors,json_type_array)){
        size_t count=json_object_array_length(errors);
        for(size_t n=0;n<count&&n<16&&!code;n++)code=vp_cloud_event_field(json_object_array_get_idx(errors,n));
    }
    json_object_put(root);
    return code;
}
#endif
