// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_STREAM_OPTIONS_H
#define PV_STREAM_OPTIONS_H
#include <stdbool.h>
#include <stdio.h>
#define PV_OPT_REORDER (1u<<0)
#define PV_OPT_VIDEO_CATCHUP (1u<<1)
#define PV_OPT_TRIPLE (1u<<2)
#define PV_OPT_AUDIO_ADAPT (1u<<3)
#define PV_OPT_PCM_TRIM (1u<<4)
#define PV_OPT_AUDIO_SMALL (1u<<5)
#define PV_OPT_FAST_INPUT (1u<<6)
#define PV_OPT_AFFINITY (1u<<7)
#define PV_OPT_BENCHMARK (1u<<8)
#define PV_OPT_AUDIO_BOUND (1u<<9)
#define PV_OPT_PS5_PC (1u<<10) /* opt-in PC fallback; default PS5 Portal envelope */
#define PV_OPT_COUNT 11u
#define PV_OPT_ALL ((1u<<PV_OPT_COUNT)-1u)
#define PV_OPT_DEFAULT (PV_OPT_VIDEO_CATCHUP|PV_OPT_AUDIO_ADAPT|PV_OPT_FAST_INPUT)
/* Keep new B4 experiments and diagnostic overhead opt-in. */
static inline bool pv_opt(unsigned flags,unsigned bit){return (flags&bit)!=0;}
static inline bool pv_options_valid(unsigned flags){return !(flags&~PV_OPT_ALL);}
static inline unsigned pv_options_sanitize(unsigned flags){return pv_options_valid(flags)?flags:PV_OPT_DEFAULT;}
/* Only migrate audio flags. Preserve every unrelated preference. */
static inline unsigned pv_audio_migrate_options(unsigned flags){
    return pv_options_sanitize(flags)&~(PV_OPT_PCM_TRIM|PV_OPT_AUDIO_SMALL|PV_OPT_AUDIO_BOUND);
}
static inline bool pv_options_parse(const char *text,unsigned *out){
    unsigned v=0,mask=0;char extra;
    if(!text||!out||sscanf(text,"%u %u %c",&v,&mask,&extra)!=2||v!=1||!pv_options_valid(mask))return false;
    *out=mask;return true;
}
#endif
