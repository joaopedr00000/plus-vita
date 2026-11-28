// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/update_policy.h"
#include <assert.h>
int main(void){
    PvVersion a,b,c,d,s;
    assert(pv_version_parse("v0.0.6",&a));assert(pv_version_parse("v0.0.6-b1",&b));
    assert(pv_version_parse("v0.0.6-b2",&c));assert(pv_version_parse("v0.0.6-b10",&d));assert(pv_version_parse("v0.0.7",&s));
    assert(pv_version_cmp(a,b)<0&&pv_version_cmp(b,c)<0&&pv_version_cmp(c,d)<0&&pv_version_cmp(d,s)<0);
    assert(!pv_update_eligible(a,b,false,true,false));assert(pv_update_eligible(a,b,true,true,false));
    assert(!pv_update_eligible(c,a,false,false,false));assert(pv_update_eligible(c,s,false,false,false));
    assert(!pv_update_eligible(a,b,true,false,false));assert(!pv_update_eligible(a,s,true,false,true));
    assert(!pv_update_eligible(s,s,true,false,false));
    assert(!pv_version_parse("v0.0.6-b0",&a));assert(!pv_version_parse("v0.0.6-2",&a));assert(!pv_version_parse("v0.0.6-b01",&a));assert(!pv_version_parse("v0.0.6-b99999",&a));assert(!pv_version_parse("v0.0",&a));
    char tag[40],display[40];pv_version_tag(c,tag);pv_version_display(c,display);assert(!strcmp(tag,"v0.0.6-b2")&&!strcmp(display,"0.0.6 B2"));
    char hash[65];assert(pv_update_checksum("0123456789abcdef0123456789abcdef0123456789abcdef0123456789ABCDEF  VitaPlus-v0.0.6-b2.vpk\r\n","VitaPlus-v0.0.6-b2.vpk",hash));
    assert(!strcmp(hash,"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"));
    assert(!pv_update_checksum("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef  other.vpk\n","VitaPlus-v0.0.6-b2.vpk",hash));
    assert(pv_update_safe_path("sce_sys/livearea/contents/template.xml")&&pv_update_safe_path("eboot.bin"));
    assert(!pv_update_safe_path("sce_sys/../eboot.bin")&&!pv_update_safe_path("sce_sys//x")&&!pv_update_safe_path("ux0:app/x")&&!pv_update_safe_path("sce_sys/..\\x"));
    assert(pv_version_parse("v0.0.7-b1",&b));
    assert(pv_update_eligible(s,b,true,true,false)&&!pv_update_eligible(s,b,false,true,false));
    assert(pv_version_parse("v0.0.7-b2",&c));
    assert(pv_update_eligible(b,c,true,true,false)&&!pv_update_eligible(b,c,false,true,false));
    assert(pv_version_parse("v0.0.7-b3",&d));
    assert(pv_update_eligible(c,d,true,true,false)&&!pv_update_eligible(c,d,false,true,false));
    assert(pv_version_parse("v0.0.7-b4",&b));
    assert(pv_update_eligible(d,b,true,true,false)&&!pv_update_eligible(d,b,false,true,false));
    puts("Update policy tests passed");return 0;
}
