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
    /* Only a recovered late frame is evidence that waiting helped.
     * Loss alone must not indefinitely ratchet up the latency budget. */
    if(saved)return current>=44000?48000:current+4000;
    if(expired)return current>base+1000?current-1000:base;
    return current>base+2000?current-2000:base;
}
static inline unsigned pv_audio_adjust(unsigned base,unsigned current,bool starved){
    if(current<base)current=base;
    if(current>11)current=11;
    return starved?(current<11?current+1:11):(current>base?current-1:base);
}
/* Each compressed Opus unit is 10 ms. Retain a four-unit jitter margin,
 * but never allow the adaptive backlog to grow to the old fixed 160 ms. */
static inline unsigned pv_audio_queue_limit(unsigned target){
    if(target<5)target=5;
    if(target>11)target=11;
    return target+4;
}
/* Only skip presentation of a fully decoded picture. Compressed references
 * must always be submitted in order; show recovery/first output immediately. */
static inline bool pv_present_due(unsigned queued,uint64_t oldest_us,uint64_t since_ms,bool first_or_recovery){
    return first_or_recovery || !queued || since_ms>=50 ||
        (queued==1 && oldest_us<16000);
}
static inline bool pv_input_stale(uint64_t now,uint64_t last_good){
    return last_good && now>=last_good && now-last_good>=100000;
}
static inline unsigned pv_retry_rate(unsigned cap,unsigned requested){
    cap=pv_profile_cap(cap);
    unsigned reduced=requested-requested/5;
    if(reduced<2500)reduced=2500;
    return reduced>cap?cap:reduced;
}
#endif
