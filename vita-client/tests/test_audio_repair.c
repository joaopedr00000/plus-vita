#include "../src/platform/audio_repair.h"
#include <assert.h>
int main(void){
    PvAudioRepair r={0};PvAudioRepairPlan p=pv_audio_repair_plan(&r,100,2,4);
    assert(!p.repair&&!p.skip&&r.next==102);
    p=pv_audio_repair_plan(&r,104,2,4);assert(p.repair==2&&!p.skip&&r.next==106);
    p=pv_audio_repair_plan(&r,104,2,4);assert(!p.repair&&p.skip==2&&r.next==106);
    p=pv_audio_repair_plan(&r,105,3,4);assert(!p.repair&&p.skip==1&&r.next==108);
    p=pv_audio_repair_plan(&r,120,2,3);assert(p.repair==3&&!p.skip&&r.next==122);
    r=(PvAudioRepair){.next=65534,.valid=true};
    p=pv_audio_repair_plan(&r,1,2,4);assert(p.repair==3&&!p.skip&&r.next==3);
    p=pv_audio_repair_plan(&r,65535,2,4);assert(!p.repair&&p.skip==2&&r.next==3);
    p=pv_audio_repair_plan(&r,3,1,5);assert(!p.repair&&!p.skip&&r.next==4);
    return 0;
}
