// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_CATALOG_FILTER_H
#define PV_CATALOG_FILTER_H
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
/* Filter the existing server-supplied entries; never invent availability. */
static inline unsigned pv_catalog_platform(const char *s)
{
    if(!s||strlen(s)!=3)return 0;
    if(toupper((unsigned char)s[0])!='P'||toupper((unsigned char)s[1])!='S')return 0;
    return s[2]>='3'&&s[2]<='5'&&!s[3]?(unsigned)(s[2]-'2'):0;
}
static inline bool pv_catalog_matches(unsigned filter,const char *platform)
{
    return filter==0||pv_catalog_platform(platform)==filter;
}
static inline bool pv_catalog_entry_matches(unsigned filter,const char *platform,bool owned){return filter==4?owned:pv_catalog_matches(filter,platform);}
static inline unsigned pv_catalog_next(unsigned filter){return (filter+1u)%5u;}
static inline const char *pv_catalog_filter_name(unsigned filter)
{
    static const char *const names[]={"All games","PS3","PS4","PS5","My games"};
    return names[filter<5?filter:0];
}
#endif
