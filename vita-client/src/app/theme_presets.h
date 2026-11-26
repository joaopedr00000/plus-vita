// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_THEME_PRESETS_H
#define PV_THEME_PRESETS_H
#include "custom_theme.h"
#define PV_THEME_COUNT 50u
#define PV_THEME_SLOTS 3u
#define PV_THEME_RETENTION_SECONDS (10u*24u*60u*60u)
#define PV_THEME_ROTATION_US UINT64_C(60000000)
/* Legacy wallpaper IDs are retained so previously saved images remain accessible. */
static inline unsigned pv_theme_id(unsigned id){return id<=PV_THEME_COUNT?id:0;}
static inline bool pv_theme_expired(uint64_t now,uint64_t last,unsigned id,unsigned active){
    return id<=PV_THEME_COUNT&&id!=active&&last>=UINT64_C(946684800)&&now>=last&&now-last>=PV_THEME_RETENTION_SECONDS;
}
#endif
