// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_PRESENT_SLOTS_H
#define PV_PRESENT_SLOTS_H
#include <stdbool.h>
typedef struct {unsigned count;int active,pending,write;} PvPresentSlots;
static inline void pv_slots_init(PvPresentSlots *s,unsigned n){*s=(PvPresentSlots){.count=n,.active=-1,.pending=-1,.write=0};}
static inline int pv_slots_free(const PvPresentSlots *s){
    for(unsigned i=0;i<s->count;i++)if((int)i!=s->active&&(int)i!=s->pending)return (int)i;
    return -1;
}
/* Call only after the native submit succeeded. Do not reuse scanout or a
 * pending scanout. With two slots the writable slot may be temporarily absent. */
static inline bool pv_slots_submit(PvPresentSlots *s){
    if(s->pending>=0||s->write<0||(unsigned)s->write>=s->count||s->write==s->active)return false;
    s->pending=s->write;s->write=pv_slots_free(s);return true;
}
/* Call only after native confirmation; preserve an outstanding writable slot. */
static inline bool pv_slots_complete(PvPresentSlots *s){
    if(s->pending<0)return false;
    s->active=s->pending;s->pending=-1;
    if(s->write<0)s->write=pv_slots_free(s);
    return true;
}
#endif
