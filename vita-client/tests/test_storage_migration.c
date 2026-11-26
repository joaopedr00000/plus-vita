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
    assert(!mkdir("ur0:data/plus-vita",0700));put("ur0:data/plus-vita/account.key","0123456789abcdef0123456789abcdef");
    put("ur0:data/unrelated.txt","untouched");
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
    check("ur0:data/plus-vita/account.key","0123456789abcdef0123456789abcdef");check("ux0:data/unrelated.txt","unrelated");
    assert(pv_storage_migrate("ux0:data"));
    assert(!mkdir("ux0:data/plus-vita",0700));put("ux0:data/plus-vita/account.json","different");
    assert(!pv_storage_migrate("ux0:data"));
    check("ux0:data/plus-vita/account.json","different");check("ux0:data/VitaPlus/Account/account.json","encrypted");
    put("ux0:data/plus-vita/account.json","encrypted");assert(pv_storage_migrate("ux0:data"));assert(access("ux0:data/plus-vita",F_OK)!=0);
    check("ur0:data/plus-vita/account.key","0123456789abcdef0123456789abcdef");assert(pv_key_storage_migrate("ur0:data"));
    assert(access("ur0:data/plus-vita",F_OK)!=0);
    check("ur0:data/VitaPlus/account.key","0123456789abcdef0123456789abcdef");
    check("ur0:data/unrelated.txt","untouched");
    assert(pv_key_storage_migrate("ur0:data"));
    assert(!mkdir("ur0:data/plus-vita",0700));
    put("ur0:data/plus-vita/account.key","fedcba9876543210fedcba9876543210");
    assert(!pv_key_storage_migrate("ur0:data"));
    check("ur0:data/plus-vita/account.key","fedcba9876543210fedcba9876543210");
    check("ur0:data/VitaPlus/account.key","0123456789abcdef0123456789abcdef");
    put("ur0:data/plus-vita/account.key","short");
    assert(!pv_key_storage_migrate("ur0:data"));
    check("ur0:data/plus-vita/account.key","short");
    put("ur0:data/plus-vita/account.key","0123456789abcdef0123456789abcdef");
    put("ur0:data/plus-vita/account.key.tmp","staged");
    put("ur0:data/plus-vita/unknown.txt","untouched");
    assert(pv_key_storage_migrate("ur0:data"));
    assert(access("ur0:data/plus-vita/account.key",F_OK)!=0);
    check("ur0:data/VitaPlus/account.key.tmp","staged");
    check("ur0:data/plus-vita/unknown.txt","untouched");
    check("ur0:data/VitaPlus/account.key","0123456789abcdef0123456789abcdef");
    puts("ux0 storage and restricted ur0 key migration, idempotency and conflict tests passed");return 0;
}
