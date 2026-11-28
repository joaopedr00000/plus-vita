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
static inline unsigned pv_profile_reorder(unsigned v){return v==6000?4000:v==10000?8000:12000;}
static inline unsigned pv_profile_reorder_initial(unsigned v){return v==6000?12000:v==10000?16000:20000;}
static inline unsigned pv_audio_block(unsigned v){return v==6000?512:960;}
static inline unsigned pv_profile_audio(unsigned v){return v==6000||v==10000?8:10;}
static inline unsigned pv_profile_queue(unsigned v){return v==6000?4:v==10000?6:8;}
static inline unsigned pv_reorder_adjust(unsigned base,unsigned current,unsigned saved,unsigned expired){
    unsigned max=base==4000?20000:base==8000?26000:30000;
    if(current<base)current=base;
    if(current>max)current=max;
    /* Recovered late frames justify waiting, not raw packet loss. On an
     * unproductive hold reduce the cap multiplicatively, with a bounded loop. */
    if(expired>=saved&&expired){
        unsigned n=expired>8?8:expired;
        while(n--&&current>base){current=current*3/4;if(current<base)current=base;}
        return current;
    }
    if(saved)return current>=max-3000?max:current+3000;
    return current>base+1000?current-1000:base;
}
static inline unsigned pv_audio_adjust(unsigned base,unsigned current,bool starved){
    if(current<base)current=base;
    if(current>16)current=16;
    return starved?(current<16?current+2>16?16:current+2:16):(current>base?current-1:base);
}
/* 10 ms per unit. A wider bounded margin absorbs burst arrivals without
 * immediately throwing away valid compressed frames. */
#define PV_AUDIO_CLEAN_UNITS 6000u
static inline unsigned pv_audio_queue_limit(unsigned target){
    if(target<8)target=8;
    if(target>16)target=16;
    return target+6;
}
/* Only skip presentation of a fully decoded picture. Compressed references
 * must always be submitted in order; show recovery/first output immediately. */
static inline bool pv_present_due(unsigned queued,uint64_t oldest_us,uint64_t since_ms,bool first_or_recovery){
    return first_or_recovery || !queued || since_ms>=50 ||
        (queued==1 && oldest_us<16000);
}
/* Smooth local backlog correction. No acceleration during startup or
 * underrun recovery. Each queued Opus unit represents 10 ms. */
static inline unsigned pv_pcm_trim(unsigned queued,unsigned target,bool stable,unsigned frames){
    if(!stable||!target||queued<=target||frames<128)return 0;
    return frames*(queued>=target+2?2u:1u)/100u;
}
static inline unsigned pv_pcm_shorten(int16_t *pcm,unsigned frames,unsigned channels,unsigned trim){
    if(!pcm||!channels||channels>2||!trim||frames<2||trim>frames/50)return frames;
    unsigned out=frames-trim;
    for(unsigned j=0;j<out;j++){
        unsigned numerator=j*(frames-1),idx=numerator/(out-1),frac=numerator%(out-1);
        unsigned next=idx+1<frames?idx+1:idx;
        for(unsigned ch=0;ch<channels;ch++){
            int32_t x=pcm[idx*channels+ch],y=pcm[next*channels+ch];
            pcm[j*channels+ch]=(int16_t)((x*(int32_t)(out-1-frac)+y*(int32_t)frac)/(int32_t)(out-1));
        }
    }
    return out;
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
