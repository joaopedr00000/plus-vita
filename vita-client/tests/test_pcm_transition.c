// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/pcm_transition.h"
#include <assert.h>
#include <limits.h>
int main(void){
    int16_t pcm[960],tail[2]={INT16_MAX,INT16_MIN};
    for(unsigned i=0;i<480;i++){pcm[2*i]=INT16_MIN;pcm[2*i+1]=INT16_MAX;}
    pv_pcm_transition(pcm,480,2,tail,true);
    assert(pcm[0]==INT16_MAX&&pcm[1]==INT16_MIN);
    assert(pcm[478]==INT16_MIN&&pcm[479]==INT16_MAX);
    assert(pcm[480]==INT16_MIN&&pcm[481]==INT16_MAX&&pcm[959]==INT16_MAX);
    for(unsigned i=1;i<240;i++){assert(pcm[2*i]<=pcm[2*i-2]);assert(pcm[2*i+1]>=pcm[2*i-1]);}
    int16_t mono[4]={12000,12000,12000,12000};
    pv_pcm_transition(mono,4,1,tail,false);
    assert(mono[0]==0&&mono[1]==4000&&mono[2]==8000&&mono[3]==12000);
    int16_t one=123;pv_pcm_transition(&one,1,1,tail,true);assert(one==123);
    return 0;
}
