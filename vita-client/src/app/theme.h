// SPDX-License-Identifier: AGPL-3.0-only
#ifndef VITAPLUS_THEME_H
#define VITAPLUS_THEME_H
#include <stdint.h>
/* Vita framebuffer format A8B8G8R8. Warm, high-contrast LiveArea palette. */
#define VP_RGB(r,g,b) (0xff000000u | ((uint32_t)(b)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(r))
#define VP_GOLD VP_RGB(255,207,64)
#define VP_GOLD_PRESSED VP_RGB(218,168,48)
#define VP_BUTTON VP_RGB(51,51,54)
#define VP_PANEL VP_RGB(32,32,35)
#define VP_WHITE VP_RGB(248,246,240)
#define VP_BODY VP_RGB(225,222,215)
#define VP_MUTED VP_RGB(197,194,187)
#define VP_DARK_INK VP_RGB(19,17,13)
#define VP_QR_BG VP_RGB(22,22,24)
#define VP_HUD_BG VP_RGB(14,14,16)
static inline uint32_t vp_menu_background(unsigned y){
    if(y>543u)y=543u;
    return VP_RGB(15u+y*26u/543u,15u+y*20u/543u,17u+y*10u/543u);
}
#endif
