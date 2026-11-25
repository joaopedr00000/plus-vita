// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/region_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    assert(pv_region_mode(4)==0&&pv_region_mode(99)==0);
    assert(pv_region_mode(1)==1&&pv_region_mode(2)==1&&pv_region_mode(3)==1);
    assert(pv_region_account_locale_valid("BR","pt"));
    assert(pv_region_account_locale_valid("US","en"));
    assert(pv_region_account_locale_valid("CH","de-CH"));
    assert(pv_region_account_locale_valid("BR","pt-BR"));
    assert(!pv_region_account_locale_valid("US","en/US"));
    assert(!pv_region_account_locale_valid("USA","en"));
    assert(!pv_region_account_locale_valid("US","e/"));
    assert(!pv_region_account_locale_valid("","en"));
    assert(!pv_region_account_locale_valid(NULL,"en"));
    assert(pv_region_fallback(1,true,true,true,404));
    assert(!pv_region_fallback(0,true,true,true,404));
    assert(!pv_region_fallback(1,false,true,true,404));
    assert(!pv_region_fallback(1,true,false,true,404));
    assert(!pv_region_fallback(1,true,true,false,404));
    assert(!pv_region_fallback(1,true,true,true,401));
    assert(!pv_region_fallback(1,true,true,true,403));
    assert(!pv_region_fallback(1,true,true,true,500));
    assert(!pv_region_fallback(1,true,true,true,429));
    puts("regional discovery policy passed");
    return 0;
}
