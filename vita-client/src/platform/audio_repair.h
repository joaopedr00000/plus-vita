// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_AUDIO_REPAIR_H
#define PV_AUDIO_REPAIR_H
#include <stdint.h>
#include <stdbool.h>
/* Packed PSNOW audio layout: source frames start at packet frame_index;
 * trailing redundant units represent frame_index-fec_count ... frame_index-1.
 * Do not use this layout for the experimental unitized PSCLOUD protocol. */
typedef struct { uint16_t next; bool valid; } PvAudioRepair;
typedef struct { unsigned repair,skip; } PvAudioRepairPlan;
static inline PvAudioRepairPlan pv_audio_repair_plan(PvAudioRepair *state,uint16_t first,unsigned sources,unsigned fec){
    PvAudioRepairPlan p={0};
    if(!sources)return p;
    if(!state->valid){state->valid=true;state->next=(uint16_t)(first+sources);return p;}
    uint16_t delta=(uint16_t)(first-state->next);
    if(delta<0x8000u){
        p.repair=delta<fec?delta:fec;
        state->next=(uint16_t)(first+sources);
    }else{
        unsigned old=(uint16_t)(state->next-first);
        p.skip=old<sources?old:sources;
        if(p.skip<sources)state->next=(uint16_t)(first+sources);
    }
    return p;
}
#endif
