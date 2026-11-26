// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/update_package.h"
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
int main(int argc,char **argv){
    assert(argc==4);assert(mkdir(argv[2],0700)==0);
    bool expected=!strcmp(argv[3],"ok");
    assert(pv_update_identity(argv[1])==expected);
    assert(pv_update_unpack(argv[1],argv[2])==expected);
    pv_update_clean_stage(argv[2]);assert(access(argv[2],F_OK)!=0);
    puts("VPK fixture policy passed");return 0;
}
