// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/update_direct_transaction.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {unsigned fail,step;bool restored;char trace[16];} Fixture;
static bool action(Fixture *f,char c){f->trace[f->step++]=c;return f->step!=f->fail;}
static bool unmount(void *p){return action(p,'U');}
static bool replace(void *p,unsigned i){return action(p,i?'E':'S');}
static bool syncfs(void *p){return action(p,'F');}
static void restart(void *p){(void)action(p,'X');}
static bool restore(void *p){Fixture *f=p;f->restored=true;(void)action(f,'R');return f->fail!=6;}
int main(void){
    const char *expected[]={"US EFXR","U","USR","USER","USEFR","USEFXR","USEFXR"};
    for(unsigned i=0;i<=6;i++){
        Fixture f={.fail=i};
        PvDirectOps ops={&f,unmount,replace,syncfs,restart,restore};
        PvDirectResult r=pv_direct_execute(ops);
        assert(r==(i==1?PV_DIRECT_UNMOUNT_FAILED:i==2?PV_DIRECT_SFO_FAILED:i==3?PV_DIRECT_EBOOT_FAILED:i==4?PV_DIRECT_SYNC_FAILED:i==6?PV_DIRECT_ROLLBACK_FAILED:PV_DIRECT_RESTART_RETURNED));
        assert(f.restored==(i!=1));
        assert(!strcmp(f.trace,i==0?"USEFXR":expected[i]));
    }
    puts("Direct updater order and rollback paths validated");
    return 0;
}
