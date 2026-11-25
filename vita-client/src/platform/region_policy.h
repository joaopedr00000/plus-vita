// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_REGION_POLICY_H
#define PLUS_VITA_REGION_POLICY_H
#include <stdbool.h>
#include <string.h>
/* Locale preferences are discovery hints, not account country or IP claims. */
static inline unsigned pv_region_mode(unsigned value){return value<=3?value:0;}
static inline const char *pv_region_hint(unsigned value){
    switch(pv_region_mode(value)){case 1:return "en-US";case 2:return "en-CA";case 3:return "pt-PT";default:return "";}
}
static inline const char *pv_region_label(unsigned value){
    switch(pv_region_mode(value)){case 1:return "Unsupported regions: US hint";case 2:return "Unsupported regions: CA hint";case 3:return "Unsupported regions: PT hint";default:return "Unsupported regions: Off";}
}
static inline bool pv_region_fallback(unsigned mode,bool authorized,bool session,bool store_transport,long status){
    return pv_region_mode(mode)!=0&&authorized&&session&&store_transport&&status==404;
}
#endif
