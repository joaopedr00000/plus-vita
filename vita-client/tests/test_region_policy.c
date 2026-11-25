// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/region_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    assert(pv_region_mode(4)==0&&pv_region_mode(99)==0);
    assert(!strcmp(pv_region_hint(1),"en-US"));
    assert(!strcmp(pv_region_hint(2),"en-CA"));
    assert(!strcmp(pv_region_hint(3),"pt-PT"));
    assert(pv_region_fallback(1,true,true,true,404));
    assert(!pv_region_fallback(0,true,true,true,404));
    assert(!pv_region_fallback(1,false,true,true,404));
    assert(!pv_region_fallback(1,true,false,true,404));
    assert(!pv_region_fallback(1,true,true,false,404));
    assert(!pv_region_fallback(1,true,true,true,401));
    assert(!pv_region_fallback(1,true,true,true,403));
    assert(!pv_region_fallback(1,true,true,true,500));
    puts("regional discovery policy passed");
    return 0;
}
