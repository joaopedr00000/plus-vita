// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/present_slots.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    for(unsigned count=2;count<=3;count++){
        PvPresentSlots s;pv_slots_init(&s,count);
        for(unsigned j=0;j<1000;j++){
            assert(s.write>=0&&s.write!=s.active&&s.pending<0);
            int submitted=s.write,prior=s.active;
            assert(pv_slots_submit(&s)&&s.pending==submitted&&s.active==prior);
            assert(!pv_slots_submit(&s));
            if(count==3||prior<0)assert(s.write>=0&&s.write!=s.pending&&s.write!=s.active);
            else assert(s.write<0);
            int writable=s.write;
            assert(pv_slots_complete(&s)&&s.active==submitted&&s.pending<0&&s.write>=0&&s.write!=s.active);
            if(writable>=0)assert(s.write==writable);
            assert(!pv_slots_complete(&s));
        }
    }
    puts("Double and triple scanout ownership policy passed");return 0;
}
