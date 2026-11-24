// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_ADAPTIVE_STREAM_H
#define PLUS_VITA_ADAPTIVE_STREAM_H
#include <stdint.h>
#include <stdbool.h>
#ifndef PLUS_VITA_ADAPTIVE_STREAM
#define PLUS_VITA_ADAPTIVE_STREAM 0
#endif
static inline unsigned pv_profile_cap(unsigned v){return v==6000||v==10000||v==15000?v:15000;}
static inline unsigned pv_profile_next(unsigned v){return v==6000?10000:v==10000?15000:6000;}
static inline unsigned pv_profile_reorder(unsigned v){return v==6000?16000:v==10000?24000:32000;}
static inline unsigned pv_profile_audio(unsigned v){return v==6000?5:v==10000?6:8;}
static inline unsigned pv_profile_queue(unsigned v){return v==6000?4:v==10000?6:8;}
static inline unsigned pv_reorder_adjust(unsigned base,unsigned current,unsigned saved,unsigned expired){
    if(current<base)current=base;
    if(current>48000)current=48000;
    if(saved||expired)return current>=44000?48000:current+4000;
    return current>base+2000?current-2000:base;
}
static inline unsigned pv_audio_adjust(unsigned base,unsigned current,bool starved){
    if(current<base)current=base;
    if(current>11)current=11;
    return starved?(current<11?current+1:11):(current>base?current-1:base);
}
static inline unsigned pv_retry_rate(unsigned cap,unsigned requested){
    cap=pv_profile_cap(cap);
    unsigned reduced=requested-requested/5;
    if(reduced<2500)reduced=2500;
    return reduced>cap?cap:reduced;
}
#endif
