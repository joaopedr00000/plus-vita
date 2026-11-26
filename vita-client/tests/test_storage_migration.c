// SPDX-License-Identifier: AGPL-3.0-only
#define _POSIX_C_SOURCE 200809L
#include "../src/platform/storage_migration.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
static void put(const char *p,const char *data){FILE *f=fopen(p,"wb");assert(f);assert(fputs(data,f)>=0);assert(!fclose(f));}
static void check(const char *p,const char *data){FILE *f=fopen(p,"rb");assert(f);char b[128]={0};assert(fread(b,1,sizeof(b)-1,f)==strlen(data));assert(!strcmp(b,data));assert(!fclose(f));}
int main(void){
    char path[]="/tmp/pv-storage-XXXXXX";assert(mkdtemp(path));assert(!chdir(path));
    assert(!mkdir("ux0:data",0700));assert(!mkdir("ur0:data",0700));
    assert(!mkdir("ur0:data/plus-vita",0700));put("ur0:data/plus-vita/account.key","untouched");
    assert(!mkdir("ux0:data/plus-vita",0700));put("ux0:data/plus-vita/account.json","encrypted");
    put("ux0:data/plus-vita/account.blocked","");put("ux0:data/plus-vita-controls.dat","controls");
    assert(!mkdir("ux0:data/VitaPlus-Wallpapers",0700));put("ux0:data/VitaPlus-Wallpapers/01-1.jpg","image");
    put("ux0:data/VitaPlus-Diagnostic-26-11-2026-10-12.log","diagnostic");
    put("ux0:data/unrelated.txt","unrelated");
    assert(pv_storage_migrate("ux0:data"));
    assert(access("ux0:data/plus-vita",F_OK)!=0);
    check("ux0:data/VitaPlus/Account/account.json","encrypted");
    check("ux0:data/VitaPlus/Settings/controls.dat","controls");
    check("ux0:data/VitaPlus/Wallpapers/01-1.jpg","image");
    check("ux0:data/VitaPlus/Logs/Archive/VitaPlus-Diagnostic-26-11-2026-10-12.log","diagnostic");
    check("ur0:data/plus-vita/account.key","untouched");check("ux0:data/unrelated.txt","unrelated");
    assert(pv_storage_migrate("ux0:data"));
    assert(!mkdir("ux0:data/plus-vita",0700));put("ux0:data/plus-vita/account.json","different");
    assert(!pv_storage_migrate("ux0:data"));
    check("ux0:data/plus-vita/account.json","different");check("ux0:data/VitaPlus/Account/account.json","encrypted");
    put("ux0:data/plus-vita/account.json","encrypted");assert(pv_storage_migrate("ux0:data"));assert(access("ux0:data/plus-vita",F_OK)!=0);
    check("ur0:data/plus-vita/account.key","untouched");puts("ux0-only migration, idempotency and conflict tests passed");return 0;
}
