#include "../src/platform/stream_options.h"
#include <assert.h>
int main(void){
    unsigned v=99;assert(pv_options_parse("1 0\n",&v)&&v==0);
    assert(pv_options_parse("1 1023\n",&v)&&v==PV_OPT_ALL);
    assert(!pv_options_parse("2 0",&v)&&!pv_options_parse("1 1024",&v)&&!pv_options_parse("1 1 extra",&v));
    assert(pv_options_sanitize(0)==0&&pv_options_sanitize(0xffffffffu)==PV_OPT_DEFAULT);
    for(unsigned j=0;j<PV_OPT_COUNT;j++)assert(pv_opt(PV_OPT_ALL,1u<<j)&&!pv_opt(0,1u<<j));
    return 0;
}
