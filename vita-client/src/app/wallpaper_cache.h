// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_WALLPAPER_CACHE_H
#define PV_WALLPAPER_CACHE_H
#include "theme_presets.h"
#include <stdatomic.h>
#include <pthread.h>
#define PV_WALL_WIDTH 480u
#define PV_WALL_HEIGHT 272u
#define PV_HEADER_WIDTH 96u
#define PV_HEADER_HEIGHT 72u
typedef struct {
    pthread_t thread;
    atomic_bool done,cancel;
    bool running,ok;
    unsigned id,slot,previous_id;
    uint64_t used,last_prune;
    uint32_t *output;
} PvWallpaperJob;
bool pv_wallpaper_start(PvWallpaperJob *job,unsigned id,unsigned slot,unsigned previous_id);
bool pv_wallpaper_poll(PvWallpaperJob *job);
bool pv_wallpaper_close(PvWallpaperJob *job);
uint32_t *pv_wallpaper_headers(void);
#endif
