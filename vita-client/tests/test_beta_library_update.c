#include "../src/platform/session_client.h"
#include "../src/platform/update_head.h"
#include <assert.h>
static bool fake_sha(const unsigned char *p,size_t n,unsigned char out[20]){for(unsigned i=0;i<20;i++)out[i]=(unsigned char)(i+n+(n?p[0]:0));return true;}
static void be(unsigned char *p,unsigned n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
int main(void){
    char id[128]="initial";
    assert(!pv_session_use_client(id,sizeof(id),"bad&client_id=other")&&!strcmp(id,"initial"));
    assert(pv_session_use_client(id,sizeof(id),"12345678-1234-1234-1234-123456789abc"));
    assert(!pv_session_use_client(id,sizeof(id),"12345678-1234-1234-1234-123456789abz"));
    unsigned char h[1024]={0};be(h+0xd0,256);be(h+8,272);be(h+0x10,128);be(h+0xd4,400);be(h+0xe8,512);
    assert(pv_head_rebuild(h,sizeof(h),"EP9000-PLVTB0001_00-0000000000000000",fake_sha));
    assert(!strcmp((char*)h+0x30,"EP9000-PLVTB0001_00-0000000000000000"));
    assert(h[256]||h[257]);be(h+0xe8,1020);assert(!pv_head_rebuild(h,sizeof(h),"test",fake_sha));
    return 0;
}
