// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_PCM_TRANSITION_H
#define PV_PCM_TRANSITION_H
#include <stdint.h>
#include <stdbool.h>
/* A 5 ms transition is confined to discontinuities. At 48 kHz it spans
 * 240 frames. Use signed 32-bit math and a convex blend; no clipping,
 * phase changes, resampling or additional output frames. */
#define PV_PCM_TRANSITION_FRAMES 240u
static inline void pv_pcm_transition(int16_t *pcm,unsigned frames,unsigned channels,const int16_t tail[2],bool have_tail){
    if(!pcm||!frames||channels<1||channels>2)return;
    unsigned n=frames<PV_PCM_TRANSITION_FRAMES?frames:PV_PCM_TRANSITION_FRAMES;
    for(unsigned i=0;i<n;i++){
        for(unsigned c=0;c<channels;c++){
            int32_t x=have_tail?tail[c]:0,y=pcm[i*channels+c];
            pcm[i*channels+c]=(int16_t)(n==1?y:(x*(int32_t)(n-1-i)+y*(int32_t)i)/(int32_t)(n-1));
        }
    }
}
#endif
