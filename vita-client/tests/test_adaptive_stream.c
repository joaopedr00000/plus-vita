// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/adaptive_stream.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(pv_profile_next(6000)==10000&&pv_profile_next(10000)==15000&&pv_profile_next(15000)==6000);
 assert(pv_profile_cap(0)==15000&&pv_profile_queue(6000)==4&&pv_profile_queue(15000)==8);
 assert(pv_profile_audio(6000)==4&&pv_profile_audio(10000)==4&&pv_profile_audio(15000)==6);
 assert(pv_profile_reorder(6000)==16000&&pv_profile_reorder(15000)==32000);
 unsigned r=16000;
 for(unsigned i=0;i<50;i++) { r=pv_reorder_adjust(16000,r,1,0); }
 assert(r==48000);
 for(unsigned i=0;i<50;i++) { r=pv_reorder_adjust(16000,r,0,0); }
 assert(r==16000);
 assert(pv_reorder_adjust(16000,32000,0,1)==31000);
 assert(pv_reorder_adjust(16000,16000,0,10)==16000);
 assert(pv_audio_queue_limit(5)==9&&pv_audio_queue_limit(8)==12&&pv_audio_queue_limit(11)==15);
 assert(pv_audio_queue_limit(0)==8&&pv_audio_queue_limit(100)==15);
 assert(pv_present_due(4,100000,1,true));
 assert(pv_present_due(0,100000,1,false));
 assert(pv_present_due(1,15999,1,false)&&!pv_present_due(1,16000,1,false));
 assert(!pv_present_due(2,0,49,false)&&pv_present_due(2,0,50,false));
 assert(!pv_input_stale(100000,0)&&!pv_input_stale(100999,1000)&&pv_input_stale(101000,1000));
 assert(!pv_input_stale(999,1000));
 assert(pv_pcm_trim(5,4,true,480)==4&&pv_pcm_trim(6,4,true,480)==9);
 assert(pv_pcm_trim(8,4,false,480)==0&&pv_pcm_trim(4,4,true,480)==0);
 int16_t pcm[960];for(unsigned j=0;j<480;j++){pcm[j*2]=(int16_t)j;pcm[j*2+1]=-(int16_t)j;}
 assert(pv_pcm_shorten(pcm,480,2,9)==471&&pcm[940]==479&&pcm[941]==-479);
 unsigned a=5;
 for(unsigned i=0;i<20;i++) { a=pv_audio_adjust(5,a,true); }
 assert(a==11);
 for(unsigned i=0;i<20;i++) { a=pv_audio_adjust(5,a,false); }
 assert(a==5);
 assert(pv_retry_rate(15000,15000)==12000&&pv_retry_rate(6000,6000)==4800&&pv_retry_rate(6000,2500)==2500);
 puts("adaptive streaming policy passed");return 0;
}
