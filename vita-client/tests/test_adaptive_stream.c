// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/adaptive_stream.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(pv_profile_next(6000)==10000&&pv_profile_next(10000)==15000&&pv_profile_next(15000)==6000);
 assert(pv_profile_cap(0)==15000&&pv_profile_queue(6000)==4&&pv_profile_queue(15000)==8);
 assert(pv_profile_audio(6000)==5&&pv_profile_audio(10000)==6&&pv_profile_audio(15000)==8);
 assert(pv_profile_reorder(6000)==16000&&pv_profile_reorder(15000)==32000);
 unsigned r=16000;
 for(unsigned i=0;i<50;i++) { r=pv_reorder_adjust(16000,r,1,0); }
 assert(r==48000);
 for(unsigned i=0;i<50;i++) { r=pv_reorder_adjust(16000,r,0,0); }
 assert(r==16000);
 unsigned a=5;
 for(unsigned i=0;i<20;i++) { a=pv_audio_adjust(5,a,true); }
 assert(a==11);
 for(unsigned i=0;i<20;i++) { a=pv_audio_adjust(5,a,false); }
 assert(a==5);
 assert(pv_retry_rate(15000,15000)==12000&&pv_retry_rate(6000,6000)==4800&&pv_retry_rate(6000,2500)==2500);
 puts("adaptive streaming policy passed");return 0;
}
