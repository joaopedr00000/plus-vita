// SPDX-License-Identifier: AGPL-3.0-only
#include "video_present_vita.h"
#include <string.h>
#include <psp2/kernel/processmgr.h>
#define SCREEN_WIDTH PLUS_VITA_SCREEN_WIDTH
#define SCREEN_HEIGHT PLUS_VITA_SCREEN_HEIGHT
#define SCREEN_PITCH PLUS_VITA_SCREEN_PITCH
#define SCREEN_BYTES ((SCREEN_PITCH * SCREEN_HEIGHT * 4u + 0x3ffffu) & ~0x3ffffu)
int plus_vita_video_present_finish(PlusVitaVideoPresent *p){
    if(p->slots.pending<0)return 0;
    uint64_t start=sceKernelGetProcessTimeWide();
    int result=sceDisplayWaitSetFrameBuf();
    p->last_wait_us=sceKernelGetProcessTimeWide()-start;
    if(result<0)return result; /* keep both native-owned slots reserved */
    pv_slots_complete(&p->slots);
    return 0;
}
int plus_vita_video_present_fini(PlusVitaVideoPresent *p){
    if(p->displayed){
        int result=plus_vita_video_present_finish(p);
        if(result<0)return result;
        result=sceDisplaySetFrameBuf(&p->saved,SCE_DISPLAY_SETBUF_NEXTFRAME);
        if(result<0)return result;
        result=sceDisplayWaitSetFrameBuf();
        if(result<0)return result;
        p->displayed=false;
    }
    for(unsigned i=0;i<3;i++){
        if(p->blocks[i]>=0)sceKernelFreeMemBlock(p->blocks[i]);
        p->blocks[i]=-1;p->pixels[i]=NULL;
    }
    return 0;
}
static int init(PlusVitaVideoPresent *p,bool triple){
    memset(p,0,sizeof(*p));
    for(unsigned i=0;i<3;i++)p->blocks[i]=-1;
    pv_slots_init(&p->slots,2);
    p->saved.size=sizeof(p->saved);
    int result=sceDisplayGetFrameBuf(&p->saved,SCE_DISPLAY_SETBUF_NEXTFRAME);
    if(result<0||!p->saved.base)return result<0?result:-1;
    unsigned want=triple?3:2;
    for(unsigned i=0;i<want;i++){
        p->blocks[i]=sceKernelAllocMemBlock("PlusVitaPreview",SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,SCREEN_BYTES,NULL);
        result=p->blocks[i];
        if(result>=0)result=sceKernelGetMemBlockBase(p->blocks[i],(void **)&p->pixels[i]);
        if(result<0){
            if(i==2){if(p->blocks[i]>=0)sceKernelFreeMemBlock(p->blocks[i]);p->blocks[i]=-1;p->pixels[i]=NULL;break;}
            plus_vita_video_present_fini(p);return result;
        }
        memset(p->pixels[i],0,SCREEN_BYTES);
        if(i==2)p->slots.count=3;
    }
    return 0;
}
int plus_vita_video_present_init(PlusVitaVideoPresent *p){return init(p,false);}
int plus_vita_video_present_pipeline_init(PlusVitaVideoPresent *p){return init(p,true);}
uint32_t *plus_vita_video_present_back_buffer(PlusVitaVideoPresent *p){
    return p->slots.write>=0?p->pixels[p->slots.write]:NULL;
}
int plus_vita_video_present_submit(PlusVitaVideoPresent *p){
    p->last_display_us=p->last_wait_us=0;
    uint32_t *base=plus_vita_video_present_back_buffer(p);
    if(!base||p->slots.pending>=0)return -1;
    SceDisplayFrameBuf frame={.size=sizeof(frame),.base=base,.pitch=SCREEN_PITCH,
        .pixelformat=SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,.width=SCREEN_WIDTH,.height=SCREEN_HEIGHT};
    uint64_t start=sceKernelGetProcessTimeWide();
    int result=sceDisplaySetFrameBuf(&frame,SCE_DISPLAY_SETBUF_NEXTFRAME);
    p->submitted_us=sceKernelGetProcessTimeWide();
    p->last_display_us=p->submitted_us-start;
    if(result<0)return result;
    p->displayed=true;
    /* This transition is guaranteed by the preceding checks. */
    if(!pv_slots_submit(&p->slots))return -1;
    return 0;
}
int plus_vita_video_present_frame(PlusVitaVideoPresent *p){
    int result=plus_vita_video_present_submit(p);
    return result<0?result:plus_vita_video_present_finish(p);
}
