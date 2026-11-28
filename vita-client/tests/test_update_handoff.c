#include "../src/platform/update_handoff.h"
#include <assert.h>
int main(void){
    PvHandoffJob a={.size=1234,.helper_size=5678,.appver="00.31"},b={0};
    memset(a.hash,'a',64);memset(a.helper_hash,'b',64);
    char s[200];assert(pv_handoff_format(&a,s,sizeof(s)));
    assert(pv_handoff_parse(s,&b)&&b.size==a.size&&!strcmp(b.hash,a.hash)&&!strcmp(b.appver,a.appver));
    assert(!pv_handoff_parse("PVH1 1 ../../ 1 123 00.31\n",&b));
    s[strlen(s)-1]='x';assert(!pv_handoff_parse(s,&b));
    a.size=PV_UPDATE_LIMIT+1;assert(!pv_handoff_format(&a,s,sizeof(s)));
    a.size=1234;a.appver[2]='x';assert(!pv_handoff_format(&a,s,sizeof(s)));
    return 0;
}
