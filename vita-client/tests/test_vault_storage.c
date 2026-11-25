// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/app/account_vita.h"
#include "../src/platform/account_vault.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#define STORE "ux0:data/plus-vita/account.json"
#define IMPORT "ux0:data/plus-vita-auth.json"
void plus_vita_auth_probe(const PlusVitaCredential *p,PlusVitaAuthProbeResult *r){(void)p;(void)r;assert(0);}
void plus_vita_auth_probe_region(const PlusVitaCredential *p,PlusVitaAuthProbeResult *r,unsigned mode){(void)p;(void)r;(void)mode;assert(0);}
void plus_vita_auth_probe_result_fini(PlusVitaAuthProbeResult *r){(void)r;}
void plus_vita_request_cancel_set(bool cancel){(void)cancel;}
static PlusVitaAppEvent run(PlusVitaAccount *a,PlusVitaAppEffect *e,char *data,size_t cap){
    assert(plus_vita_account_start(a,e));
    PlusVitaAppEvent r={0};bool done=false;
    for(unsigned j=0;j<30000;j++){
        if(plus_vita_account_poll(a,&r)){done=true;break;}
        struct timespec ts={0,1000000};nanosleep(&ts,NULL);
    }
    assert(done);
    if(data&&r.data){assert(r.data_size<cap);memcpy(data,r.data,r.data_size);data[r.data_size]=0;}
    r.data=NULL;plus_vita_account_release_result(a);return r;
}
static void raw(const char *path,const char *token){
    FILE *f=fopen(path,"wb");assert(f);assert(fprintf(f,"{\"npsso\":\"%s\",\"label\":\"Test\"}",token)>0);assert(!fclose(f));
}
static void sealed(const char *token){
    unsigned char buf[512];FILE *f=fopen(STORE,"rb");assert(f);size_t n=fread(buf,1,sizeof(buf),f);assert(!fclose(f));
    assert(pv_vault_is(buf,n)&&n>PV_VAULT_OVERHEAD);
    for(size_t j=0;j+64<=n;j++)assert(memcmp(buf+j,token,64));
    assert(access("ux0:data/plus-vita/account.tmp",F_OK)!=0);
}
static void pass(PlusVitaCredential *p,const char *s){assert(plus_vita_credential_set(p,s,strlen(s)));}
int main(void){
    assert(!mkdir("ux0:data",0700));assert(!mkdir("ux0:data/plus-vita",0700));assert(!mkdir("ur0:data",0700));
    char token[65];memset(token,'A',64);token[64]=0;
    raw(STORE,token);raw(IMPORT,token);
    PlusVitaAccount *a=plus_vita_account_create();assert(a);
    PlusVitaAppEffect e={.kind=PLUS_VITA_EFFECT_LOAD_ACCOUNT,.operation=1};
    char data[4097]={0};
    PlusVitaAppEvent r=run(a,&e,data,sizeof(data));assert(r.status==PLUS_VITA_RESULT_OK&&r.legacy_plain&&strstr(data,token));
    e.kind=PLUS_VITA_EFFECT_SAVE_ACCOUNT;e.vault_target=2;pass(&e.credential,token);pass(&e.vault_password,"example-local-phrase");strcpy(e.label,"Test");
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_OK);sealed(token);assert(access(IMPORT,F_OK)!=0);
    raw(IMPORT,token); /* Recreate import to prove failed saves preserve it. */
    PlusVitaCredential original=e.vault_password;
    e.kind=PLUS_VITA_EFFECT_LOAD_ACCOUNT;pass(&e.vault_password,"wrong-local-phrase");
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_VAULT_LOCKED);
    e.kind=PLUS_VITA_EFFECT_SAVE_ACCOUNT;e.vault_clean_import=true;
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_VAULT_LOCKED);assert(access(IMPORT,F_OK)==0);sealed(token);
    e.vault_password=original;
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_OK);assert(access(IMPORT,F_OK)!=0);
    e.vault_change=true;e.vault_clean_import=false;e.vault_previous=original;pass(&e.vault_password,"another-local-phrase");
    PlusVitaCredential changed=e.vault_password;
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_OK);sealed(token);
    e.kind=PLUS_VITA_EFFECT_LOAD_ACCOUNT;e.vault_password=original;
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_VAULT_LOCKED);
    e.vault_password=changed;
    r=run(a,&e,data,sizeof(data));assert(r.status==PLUS_VITA_RESULT_OK&&r.vault_present&&strstr(data,token));
    /* A different account's plaintext import must never be removed. */
    char other[65];memset(other,'B',64);other[64]=0;raw(IMPORT,other);
    e.kind=PLUS_VITA_EFFECT_SAVE_ACCOUNT;e.vault_change=false;
    r=run(a,&e,NULL,0);assert(r.status==PLUS_VITA_RESULT_OK&&r.import_cleanup_failed&&access(IMPORT,F_OK)==0);
    assert(!remove(IMPORT));raw(IMPORT,token);
    /* Switch off prompting once with the old password; then restore with no password. */
    e.kind=PLUS_VITA_EFFECT_SAVE_ACCOUNT;e.vault_target=3;e.vault_change=true;e.vault_previous=changed;
    plus_vita_credential_clear(&e.vault_password);
    r=run(a,&e,NULL,0);assert(r.status==PLUS_VITA_RESULT_OK&&r.vault_mode==3);sealed(token);
    assert(plus_vita_account_file_kind()==3);
    assert(access(IMPORT,F_OK)!=0);
    assert(access("ur0:data/plus-vita/account.key",F_OK)==0);
    e.kind=PLUS_VITA_EFFECT_LOAD_ACCOUNT;plus_vita_credential_clear(&e.vault_previous);
    r=run(a,&e,data,sizeof(data));assert(r.status==PLUS_VITA_RESULT_OK&&r.vault_mode==3&&strstr(data,token));
    assert(!rename("ur0:data/plus-vita/account.key","ur0:data/plus-vita/key.backup"));
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_VAULT_LOCKED);
    assert(!rename("ur0:data/plus-vita/key.backup","ur0:data/plus-vita/account.key"));
    /* Turn prompting back on and confirm the automatic key is removed. */
    e.kind=PLUS_VITA_EFFECT_SAVE_ACCOUNT;e.vault_target=2;e.vault_change=true;e.vault_password=changed;
    r=run(a,&e,NULL,0);assert(r.status==PLUS_VITA_RESULT_OK&&r.vault_mode==2);
    assert(access("ur0:data/plus-vita/account.key",F_OK)!=0);
    e.kind=PLUS_VITA_EFFECT_LOAD_ACCOUNT;plus_vita_credential_clear(&e.vault_password);
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_VAULT_LOCKED);
    e.vault_password=changed;
    assert(run(a,&e,data,sizeof(data)).status==PLUS_VITA_RESULT_OK);
    FILE *f=fopen(STORE,"r+b");assert(f);assert(!fseek(f,PV_VAULT_HEADER,SEEK_SET));int b=fgetc(f);assert(b!=EOF);assert(!fseek(f,-1,SEEK_CUR));assert(fputc(b^1,f)!=EOF);assert(!fclose(f));
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_VAULT_LOCKED);
    e.kind=PLUS_VITA_EFFECT_DELETE_ACCOUNT;
    assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_OK);assert(access(STORE,F_OK)!=0);
    e.kind=PLUS_VITA_EFFECT_LOAD_ACCOUNT;
    r=run(a,&e,NULL,0);assert(r.status==PLUS_VITA_RESULT_MISSING&&r.restore_blocked);
    /* Fresh account saves automatically by default with no password. */
    raw(IMPORT,token);
    e.kind=PLUS_VITA_EFFECT_SAVE_ACCOUNT;e.vault_target=0;e.vault_change=false;
    plus_vita_credential_clear(&e.vault_password);plus_vita_credential_clear(&e.vault_previous);
    r=run(a,&e,NULL,0);assert(r.status==PLUS_VITA_RESULT_OK&&r.vault_mode==3);
    assert(access(IMPORT,F_OK)!=0);
    e.kind=PLUS_VITA_EFFECT_LOAD_ACCOUNT;assert(run(a,&e,data,sizeof(data)).status==PLUS_VITA_RESULT_OK);
    e.kind=PLUS_VITA_EFFECT_DELETE_ACCOUNT;assert(run(a,&e,NULL,0).status==PLUS_VITA_RESULT_OK);
    assert(access("ur0:data/plus-vita/account.key",F_OK)!=0);
    assert(plus_vita_account_close(a));pv_vault_wipe(&e,sizeof(e));pv_vault_wipe(&original,sizeof(original));pv_vault_wipe(&changed,sizeof(changed));pv_vault_wipe(data,sizeof(data));
    puts("Vault storage migration, authenticated replacement, verified automatic cleanup, password rotation and forget passed");
}
