// SPDX-License-Identifier: AGPL-3.0-only
#include "updater_vita.h"
#include "update_policy.h"
#include "update_package.h"
#include "update_head.h"
#include "update_handoff.h"
#include <psp2/appmgr.h>
#include "curl_resolve.h"
#include <curl/curl.h>
#include <json_object.h>
#include <json_tokener.h>
#include <mbedtls/md.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/net/net.h>
#include <arpa/inet.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/sysmodule.h>
#include <psp2/promoterutil.h>
#define REPO "https://github.com/joaopedr00000/plus-vita/releases/download/"
#define RELEASES "https://api.github.com/repos/joaopedr00000/plus-vita/releases?per_page=100"
#define DOWNLOADS "ux0:Downloads"
#define VPK DOWNLOADS "/VitaPlus-update.vpk"
#define PREFS "ux0:data/VitaPlus/Settings/updates.cfg"
#define STAGE_ROOT "ux0:data/VitaPlus"
#define HANDOFF STAGE_ROOT "/Settings/update-handoff.dat"
#define HANDOFF_RESULT STAGE_ROOT "/Settings/update-handoff.result"
#define HELPER_PATH "ux0:app/" PV_HELPER_TITLE
#define MAIN_PATH "ux0:app/" PV_MAIN_TITLE
#define MAX_VPK (128u*1024u*1024u)
#define MAX_JSON (2u*1024u*1024u)
typedef struct {char tag[40],asset[80],notes[4096],hash[65],path[200];size_t size;} Candidate;
struct PvUpdater {
    pthread_t thread;bool running,dirty;
    atomic_bool done,cancel;
    atomic_uint progress;
    PvUpdateCommand command;
    unsigned lang,last_progress;
    PvUpdateReport report;
    Candidate candidate;
    PvUpdateState result;
    unsigned install_stage;
    int install_error;
    bool install_unsafe;
};
typedef struct {PvUpdater *u;FILE *file;char *mem;size_t cap,used;long status;bool progress;} Sink;
static bool is_dir(const char *p){struct stat s;return stat(p,&s)==0&&S_ISDIR(s.st_mode);}
static bool ensure_dir(const char *p){return is_dir(p)||mkdir(p,0700)==0;}
static bool url_host(const char *url,char host[80]){
    if(strncmp(url,"https://",8))return false;
    const char *p=url+8,*end=strchr(p,'/');if(!end)return false;
    size_t n=(size_t)(end-p);if(!n||n>=80)return false;
    memcpy(host,p,n);host[n]=0;
    return !strcmp(host,"github.com")||!strcmp(host,"api.github.com")||!strcmp(host,"release-assets.githubusercontent.com")||!strcmp(host,"objects.githubusercontent.com");
}
static CURLcode resolve_host(CURL *curl,const char *host,struct curl_slist **list){
    int id=sceNetResolverCreate("VitaPlusUpdate",NULL,0);if(id<0)return CURLE_COULDNT_RESOLVE_HOST;
    SceNetInAddr addr={0};int rc=sceNetResolverStartNtoa(id,host,&addr,1000000,1,0);sceNetResolverDestroy(id);
    if(rc<0)return CURLE_COULDNT_RESOLVE_HOST;
    char ip[INET_ADDRSTRLEN],entry[120];
    if(!inet_ntop(AF_INET,&addr,ip,sizeof(ip)))return CURLE_COULDNT_RESOLVE_HOST;
    snprintf(entry,sizeof(entry),"%s:443:%s",host,ip);
    *list=curl_slist_append(NULL,entry);if(!*list)return CURLE_OUT_OF_MEMORY;
    CURLcode err=curl_easy_setopt(curl,CURLOPT_IPRESOLVE,CURL_IPRESOLVE_V4);
    if(err==CURLE_OK)err=curl_easy_setopt(curl,CURLOPT_RESOLVE,*list);
    return err;
}
static size_t header(void *data,size_t size,size_t count,void *context){
    Sink *s=context;if(size&&count>SIZE_MAX/size)return 0;size_t n=size*count;
    if(n>=12&&!memcmp(data,"HTTP/",5)){
        char line[48]={0};size_t take=n<sizeof(line)-1?n:sizeof(line)-1;memcpy(line,data,take);
        char *p=strchr(line,' ');if(p)s->status=strtol(p+1,NULL,10);
    }
    return n;
}
static size_t receive(void *data,size_t size,size_t count,void *context){
    Sink *s=context;if(size&&count>SIZE_MAX/size)return 0;size_t n=size*count;
    if(atomic_load(&s->u->cancel))return 0;
    if(s->status!=200)return n;
    if(n>s->cap-s->used)return 0;
    if(s->file){if(fwrite(data,1,n,s->file)!=n)return 0;}
    else {char *p=realloc(s->mem,s->used+n+1);if(!p)return 0;s->mem=p;memcpy(p+s->used,data,n);p[s->used+n]=0;}
    s->used+=n;
    if(s->progress&&s->cap)atomic_store(&s->u->progress,(unsigned)(((uint64_t)s->used*100u)/s->cap));
    return n;
}
static int transfer_progress(void *context,curl_off_t a,curl_off_t b,curl_off_t c,curl_off_t d){
    (void)a;(void)b;(void)c;(void)d;return atomic_load(&((PvUpdater *)context)->cancel);
}
static bool fetch(PvUpdater *u,const char *initial,Sink *sink){
    char url[4096];if(strlen(initial)>=sizeof(url))return false;strcpy(url,initial);
    for(unsigned redirect=0;redirect<5&&!atomic_load(&u->cancel);redirect++){
        char host[80];if(!url_host(url,host))return false;
        CURL *curl=curl_easy_init();if(!curl)return false;
        struct curl_slist *resolve=NULL;CURLcode rc=plus_vita_curl_secure_options(curl);
        if(rc==CURLE_OK)rc=resolve_host(curl,host,&resolve);
        sink->status=0;
        if(rc==CURLE_OK&&!atomic_load(&u->cancel)){
            curl_easy_setopt(curl,CURLOPT_URL,url);curl_easy_setopt(curl,CURLOPT_USERAGENT,"VitaPlus-Updater/1");
            curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L);curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
            curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT_MS,8000L);curl_easy_setopt(curl,CURLOPT_TIMEOUT_MS,sink->file?300000L:20000L);
            curl_easy_setopt(curl,CURLOPT_LOW_SPEED_LIMIT,256L);curl_easy_setopt(curl,CURLOPT_LOW_SPEED_TIME,15L);
            curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION,header);curl_easy_setopt(curl,CURLOPT_HEADERDATA,sink);
            curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(curl,CURLOPT_WRITEDATA,sink);
            curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,transfer_progress);curl_easy_setopt(curl,CURLOPT_XFERINFODATA,u);
            rc=curl_easy_perform(curl);
        }
        long code=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code);
        char *next=NULL;curl_easy_getinfo(curl,CURLINFO_REDIRECT_URL,&next);
        bool again=rc==CURLE_OK&&code>=300&&code<400&&next&&strlen(next)<sizeof(url)&&sink->used==0;
        if(again)strcpy(url,next);
        curl_easy_cleanup(curl);curl_slist_free_all(resolve);
        if(again)continue;
        return rc==CURLE_OK&&code==200&&sink->used&&!atomic_load(&u->cancel);
    }
    return false;
}
static struct json_object *member(struct json_object *o,const char *key,enum json_type type){
    struct json_object *v=NULL;return o&&json_object_object_get_ex(o,key,&v)&&json_object_get_type(v)==type?v:NULL;
}
static const char *strval(struct json_object *o,const char *key){
    struct json_object *v=member(o,key,json_type_string);return v?json_object_get_string(v):NULL;
}
static void copy_notes(char out[4096],const char *body,unsigned lang){
    if(!body)return;
    const char *heading=(lang==2||lang==3)?"## Português (Brasil)":(lang==4||lang==5)?"## Español":"## English";
    const char *p=strstr(body,heading);if(p){const char *n=strchr(p,'\n');p=n?n+1:p;}else p=body;
    const char *end=strstr(p,"\n## ");size_t n=end?(size_t)(end-p):strlen(p);if(n>4095)n=4095;
    while(n&&((unsigned char)p[n]&0xc0)==0x80)n--;
    for(size_t j=0;j<n;j++)out[j]=(unsigned char)p[j]<32&&p[j]!='\n'&&p[j]!='\t'?' ':p[j];
    out[n]=0;
}
static PvUpdateState check(PvUpdater *u){
    Sink s={.u=u,.cap=MAX_JSON};PvVersion installed,best={0};bool found=false;
    if(!pv_version_parse(PLUS_VITA_APP_VERSION,&installed))return PV_UP_ERROR;
    if(!fetch(u,RELEASES,&s)){free(s.mem);return PV_UP_ERROR;}
    struct json_tokener *tok=json_tokener_new();if(!tok){free(s.mem);return PV_UP_ERROR;}
    json_tokener_set_flags(tok,JSON_TOKENER_STRICT);
    struct json_object *arr=json_tokener_parse_ex(tok,s.mem,(int)s.used);
    bool valid=json_tokener_get_error(tok)==json_tokener_success&&arr&&json_object_get_type(arr)==json_type_array;
    memset(&u->candidate,0,sizeof(u->candidate));
    if(valid)for(size_t j=0;j<json_object_array_length(arr);j++){
        struct json_object *release=json_object_array_get_idx(arr,j);
        const char *tag=strval(release,"tag_name");PvVersion v;char canonical[40];
        if(!tag||!pv_version_parse(tag,&v))continue;
        pv_version_tag(v,canonical);if(strcmp(canonical,tag))continue;
        struct json_object *pre=member(release,"prerelease",json_type_boolean),*draft=member(release,"draft",json_type_boolean);
        if(!pre||!draft||!pv_update_eligible(installed,v,u->report.beta,json_object_get_boolean(pre),json_object_get_boolean(draft))||(found&&pv_version_cmp(v,best)<=0))continue;
        char name[80];snprintf(name,sizeof(name),"VitaPlus-%s.vpk",canonical);
        struct json_object *assets=member(release,"assets",json_type_array);if(!assets)continue;
        unsigned packages=0,checksums=0;size_t bytes=0;
        for(size_t k=0;k<json_object_array_length(assets);k++){
            struct json_object *asset=json_object_array_get_idx(assets,k);const char *n=strval(asset,"name"),*state=strval(asset,"state");
            if(!n||!state||strcmp(state,"uploaded"))continue;
            if(!strcmp(n,name)){struct json_object *sz=member(asset,"size",json_type_int);int64_t x=sz?json_object_get_int64(sz):0;if(x>0&&x<=MAX_VPK){bytes=(size_t)x;packages++;}}
            if(!strcmp(n,"SHA256SUMS.txt"))checksums++;
        }
        if(packages!=1||checksums!=1)continue;
        memset(&u->candidate,0,sizeof(u->candidate));strcpy(u->candidate.tag,canonical);strcpy(u->candidate.asset,name);u->candidate.size=bytes;
        copy_notes(u->candidate.notes,strval(release,"body"),u->lang);best=v;found=true;
    }
    if(arr)json_object_put(arr);
    json_tokener_free(tok);free(s.mem);
    return !valid?PV_UP_ERROR:found?PV_UP_AVAILABLE:PV_UP_CURRENT;
}
static bool hash_file(const char *path,char out[65],size_t expected){
    FILE *f=fopen(path,"rb");if(!f)return false;
    mbedtls_md_context_t ctx;mbedtls_md_init(&ctx);
    const mbedtls_md_info_t *md=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    bool ok=md&&!mbedtls_md_setup(&ctx,md,0)&&!mbedtls_md_starts(&ctx);
    unsigned char buf[8192],digest[32];size_t bytes=0;
    while(ok){size_t n=fread(buf,1,sizeof(buf),f);if(!n)break;bytes+=n;ok=bytes<=expected&&!mbedtls_md_update(&ctx,buf,n);}
    ok=ok&&!ferror(f)&&bytes==expected&&!mbedtls_md_finish(&ctx,digest);
    if(fclose(f)!=0)ok=false;
    mbedtls_md_free(&ctx);
    if(ok){static const char hex[]="0123456789abcdef";for(unsigned j=0;j<32;j++){out[j*2]=hex[digest[j]>>4];out[j*2+1]=hex[digest[j]&15];}out[64]=0;}
    return ok;
}
/* One dedicated slot, replaced only after the new file passes size, SHA-256
 * and title identity. A user file with another title is never overwritten. */
static PvUpdateState download(PvUpdater *u){
    Candidate *c=&u->candidate;if(!c->size||!ensure_dir(DOWNLOADS))return PV_UP_ERROR;
    char url[256];snprintf(url,sizeof(url),REPO "%s/SHA256SUMS.txt",c->tag);
    Sink sum={.u=u,.cap=16384};bool ok=fetch(u,url,&sum)&&pv_update_checksum(sum.mem,c->asset,c->hash);free(sum.mem);if(!ok)return PV_UP_ERROR;
    char part[224];snprintf(part,sizeof(part),VPK ".%llu.part",(unsigned long long)sceKernelGetProcessTimeWide());
    int fd=open(part,O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)return PV_UP_ERROR;
    FILE *f=fdopen(fd,"wb");if(!f){close(fd);unlink(part);return PV_UP_ERROR;}
    snprintf(url,sizeof(url),REPO "%s/%s",c->tag,c->asset);
    Sink pkg={.u=u,.file=f,.cap=c->size,.progress=true};
    ok=fetch(u,url,&pkg)&&pkg.used==c->size;
    if(fclose(f)!=0)ok=false;
    char actual[65];ok=ok&&hash_file(part,actual,c->size)&&!strcmp(actual,c->hash)&&pv_update_identity(part)&&!atomic_load(&u->cancel);
    struct stat st;bool existed=stat(VPK,&st)==0;
    if(ok&&existed&&!pv_update_identity(VPK))ok=false;
    if(ok&&rename(part,VPK)!=0){
        /* FAT/newlib may not replace a destination. Preserve it in a unique
         * backup until the verified replacement is in place. */
        char backup[224];snprintf(backup,sizeof(backup),VPK ".%llu.old",(unsigned long long)sceKernelGetProcessTimeWide());
        if(!existed||stat(backup,&st)==0||!pv_update_identity(VPK)||rename(VPK,backup)!=0)ok=false;
        else if(rename(part,VPK)!=0){(void)rename(backup,VPK);ok=false;}
        else unlink(backup);
    }
    if(!ok){unlink(part);c->path[0]=0;return PV_UP_ERROR;}
    strcpy(c->path,VPK);return PV_UP_READY;
}
/* Prepare the same-title native header again, using the target SFO content
 * identifier. A missing template fails safely; VitaShell remains the fallback. */
static uint32_t sfo_le(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static bool read_small(const char *p,unsigned char *buf,size_t cap,size_t *n){
    FILE *f=fopen(p,"rb");if(!f)return false;
    *n=fread(buf,1,cap,f);bool ok=!ferror(f)&&*n<cap&&feof(f);fclose(f);return ok;
}
static bool sfo_value(const unsigned char *b,size_t n,const char *wanted,char *out,size_t cap){
    if(n<20||memcmp(b,"\0PSF",4))return false;
    uint32_t keys=sfo_le(b+8),data=sfo_le(b+12),count=sfo_le(b+16);
    if(count>128||20u+count*16u>keys||keys>=data||data>n)return false;
    for(uint32_t j=0;j<count;j++){
        const unsigned char *e=b+20+j*16;uint32_t ko=e[0]|(unsigned)e[1]<<8,len=sfo_le(e+4),off=sfo_le(e+12);
        if(ko>=data-keys||off>n-data||len>n-data-off)return false;
        const char *key=(const char *)b+keys+ko;if(!memchr(key,0,data-keys-ko))return false;
        if(!strcmp(key,wanted)){
            if(!len||len>cap||!memchr(b+data+off,0,len))return false;
            memcpy(out,b+data+off,len);return true;
        }
    }
    return false;
}
static bool sha1(const unsigned char *data,size_t n,unsigned char out[20]){
    const mbedtls_md_info_t *md=mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    return md&&mbedtls_md(md,data,n,out)==0;
}
static bool stage_header(const char *root,const char *expected_title,char appver[16]){
    char folder[180],path[200],content[48]={0},title[16]={0};
    unsigned char sfo[8192];size_t n=0;
    snprintf(path,sizeof(path),"%s/sce_sys/param.sfo",root);
    if(!read_small(path,sfo,sizeof(sfo),&n)||!sfo_value(sfo,n,"TITLE_ID",title,sizeof(title))||strcmp(title,expected_title)||!sfo_value(sfo,n,"APP_VER",appver,16))return false;
    if(!sfo_value(sfo,n,"CONTENT_ID",content,sizeof(content))||!content[0])
        snprintf(content,sizeof(content),"EP9000-%s_00-0000000000000000",title);
    /* Only our own homebrew identity is accepted. */
    char needle[16];snprintf(needle,sizeof(needle),"-%s_",expected_title);
    if(!memchr(content,0,sizeof(content))||!strstr(content,needle))return false;
    unsigned char *h=malloc(65537);if(!h)return false;
    bool ok=read_small("app0:sce_sys/package/head.bin",h,65537,&n)&&pv_head_rebuild(h,n,content,sha1);
    snprintf(folder,sizeof(folder),"%s/sce_sys/package",root);
    if(ok)ok=ensure_dir(folder);
    snprintf(path,sizeof(path),"%s/head.bin",folder);
    if(ok){
        int fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0600);
        if(fd<0)ok=false;
        else {FILE *out=fdopen(fd,"wb");if(!out){close(fd);ok=false;}else {ok=fwrite(h,1,n,out)==n;if(fclose(out)!=0)ok=false;}}
    }
    free(h);return ok;
}
/* Synchronous promoter calls run in the worker or separate helper process.
 * Never promote the currently running main title. */
static int promote(const char *path,bool remove_helper,unsigned *stage){
    uint32_t args[]={0x180000u,UINT32_MAX,UINT32_MAX,1,UINT32_MAX,UINT32_MAX};
    int paf_result=-1,rc;bool paf=false,prom=false,init=false;
    SceSysmoduleOpt opt={.flags=sizeof(SceSysmoduleOpt),.result=&paf_result,.unused={-1,-1}};
    *stage=5;
    rc=sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF,sizeof(args),args,&opt);
    if(rc>=0)paf=true;
    else if(sceSysmoduleIsLoadedInternal(SCE_SYSMODULE_INTERNAL_PAF)<0)return rc;
    *stage=6;
    rc=sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
    if(rc>=0)prom=true;
    else if(sceSysmoduleIsLoadedInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL)<0)goto done;
    *stage=7;
    rc=scePromoterUtilityInit();if(rc<0)goto done;
    init=true;*stage=8;
    if(remove_helper)rc=scePromoterUtilityDeletePkg(PV_HELPER_TITLE);
    else rc=scePromoterUtilityPromotePkgWithRif(path,1);
done:
    if(init)scePromoterUtilityExit();
    if(prom)sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
    if(paf){SceSysmoduleOpt unload={0};sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF,0,NULL,&unload);}
    return rc;
}
static bool file_size(const char *path,unsigned *out){
    struct stat st;
    if(stat(path,&st)||!S_ISREG(st.st_mode)||st.st_size<=0||(uint64_t)st.st_size>MAX_VPK)return false;
    *out=(unsigned)st.st_size;return true;
}
static bool match_hash(const char *path,unsigned size,const char *expected){
    char got[65];return hash_file(path,got,size)&&!strcmp(got,expected);
}
static bool read_job(PvHandoffJob *job){
    char raw[200];size_t n=0;
    if(!read_small(HANDOFF,(unsigned char *)raw,sizeof(raw),&n))return false;
    raw[n]=0;return pv_handoff_parse(raw,job);
}
static bool write_new(const char *path,const void *buf,size_t n){
    int fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0600);
    if(fd<0)return false;
    FILE *f=fdopen(fd,"wb");if(!f){close(fd);unlink(path);return false;}
    bool ok=fwrite(buf,1,n,f)==n;if(fclose(f)!=0)ok=false;
    if(!ok)unlink(path);
    return ok;
}
static bool read_version(const char *path,const char *title,char ver[16]){
    unsigned char b[8192];size_t n=0;char tid[16]={0};
    return read_small(path,b,sizeof(b),&n)&&sfo_value(b,n,"TITLE_ID",tid,sizeof(tid))&&!strcmp(tid,title)&&sfo_value(b,n,"APP_VER",ver,16)&&pv_handoff_version(ver);
}
/* Only alter the helper copy's own SFO. Main package remains unchanged. */
static bool helper_sfo(const char *path){
    unsigned char b[8192];size_t n=0;
    if(!read_small(path,b,sizeof(b),&n)||n<20||memcmp(b,"\0PSF",4))return false;
    uint32_t keys=sfo_le(b+8),data=sfo_le(b+12),count=sfo_le(b+16);unsigned title=0;
    if(count>128||20u+count*16u>keys||keys>=data||data>n)return false;
    for(uint32_t j=0;j<count;j++){
        unsigned char *e=b+20+j*16;uint32_t ko=e[0]|(unsigned)e[1]<<8,len=sfo_le(e+4),off=sfo_le(e+12);
        if(ko>=data-keys||off>n-data||len>n-data-off)return false;
        const char *key=(const char *)b+keys+ko;if(!memchr(key,0,data-keys-ko))return false;
        unsigned char *v=b+data+off;
        if(!strcmp(key,"TITLE_ID")){
            if(len!=10||memcmp(v,PV_MAIN_TITLE,10))return false;
            memcpy(v,PV_HELPER_TITLE,10);title++;
        }else if(!strcmp(key,"CONTENT_ID")&&len&&v[0]){
            if(!memchr(v,0,len))return false;
            char *id=strstr((char *)v,"-" PV_MAIN_TITLE "_");
            if(!id)return false;
            memcpy(id+1,PV_HELPER_TITLE,9);
        }
    }
    if(title!=1)return false;
    /* This is a newly extracted staging file, never an installed file. */
    FILE *f=fopen(path,"wb");if(!f)return false;
    bool ok=fwrite(b,1,n,f)==n;if(fclose(f)!=0)ok=false;return ok;
}
static bool copy_helper_exe(const char *path,const PvHandoffJob *job){
    FILE *in=fopen("app0:eboot.bin","rb");if(!in)return false;
    FILE *out=fopen(path,"wb");if(!out){fclose(in);return false;}
    unsigned char b[8192];size_t total=0;bool ok=true;
    while(ok){
        size_t n=fread(b,1,sizeof(b),in);if(!n)break;
        total+=n;ok=total<=job->helper_size&&fwrite(b,1,n,out)==n;
    }
    ok=ok&&!ferror(in)&&total==job->helper_size;
    fclose(in);if(fclose(out)!=0)ok=false;
    return ok&&match_hash(path,job->helper_size,job->helper_hash);
}
static bool helper_owned(const PvHandoffJob *job){
    char ver[16]={0};
    return read_version(HELPER_PATH "/sce_sys/param.sfo",PV_HELPER_TITLE,ver)&&
        match_hash(HELPER_PATH "/eboot.bin",job->helper_size,job->helper_hash);
}
static bool new_stage(char root[128]){
    snprintf(root,128,STAGE_ROOT "/UpdateStage-%llu",(unsigned long long)sceKernelGetProcessTimeWide());
    return mkdir(root,0700)==0;
}

#if PLUS_VITA_DIRECT_EXPERIMENT
#define DIRECT_ROOT STAGE_ROOT "/DirectUpdate"
#define DIRECT_JOB DIRECT_ROOT "/pending"
#define DIRECT_OK DIRECT_ROOT "/confirmed"
#define DIRECT_SLOT DOWNLOADS "/VitaPlus-B6-SHA256SUMS.txt"
static bool direct_pending;
typedef struct {unsigned size[4];char hash[4][65];} DirectJob;
static const char *const direct_files[]={DIRECT_ROOT "/old-sfo",DIRECT_ROOT "/old-eboot",DIRECT_ROOT "/new-sfo",DIRECT_ROOT "/new-eboot"};
static const char *const direct_live[]={MAIN_PATH "/sce_sys/param.sfo",MAIN_PATH "/eboot.bin"};
static bool direct_matches(const char *path,const DirectJob *j,unsigned i){
    return j->size[i]&&match_hash(path,j->size[i],j->hash[i]);
}
static bool direct_read(DirectJob *j){
    char raw[400];size_t n=0;
    if(!read_small(DIRECT_JOB,(unsigned char *)raw,sizeof(raw),&n))return false;
    raw[n]=0;char extra;
    if(sscanf(raw,"PVD1 %u %64[0-9a-f] %u %64[0-9a-f] %u %64[0-9a-f] %u %64[0-9a-f] %c",
       &j->size[0],j->hash[0],&j->size[1],j->hash[1],&j->size[2],j->hash[2],&j->size[3],j->hash[3],&extra)!=8)return false;
    for(unsigned i=0;i<4;i++)if(!j->size[i]||j->size[i]>MAX_VPK||!pv_handoff_hash(j->hash[i]))return false;
    return true;
}
static bool direct_copy(const char *from,const char *to,DirectJob *j,unsigned i){
    if(!file_size(from,&j->size[i])||!hash_file(from,j->hash[i],j->size[i]))return false;
    FILE *in=fopen(from,"rb");if(!in)return false;
    int fd=open(to,O_WRONLY|O_CREAT|O_EXCL,0600);
    if(fd<0){fclose(in);return false;}
    FILE *out=fdopen(fd,"wb");if(!out){close(fd);fclose(in);unlink(to);return false;}
    unsigned char buf[8192];size_t total=0;bool ok=true;
    while(ok){size_t n=fread(buf,1,sizeof(buf),in);if(!n)break;total+=n;ok=total<=j->size[i]&&fwrite(buf,1,n,out)==n;}
    ok=ok&&!ferror(in)&&total==j->size[i];
    fclose(in);if(fflush(out)!=0)ok=false;
    if(fclose(out)!=0)ok=false;
    ok=ok&&direct_matches(to,j,i);if(!ok)unlink(to);return ok;
}
/* Never recursively remove the persistent backup directory. */
static void direct_clean(void){
    for(unsigned i=0;i<4;i++)unlink(direct_files[i]);
    unlink(DIRECT_ROOT "/restore-sfo");unlink(DIRECT_ROOT "/restore-eboot");
    unlink(DIRECT_OK);unlink(DIRECT_JOB);rmdir(DIRECT_ROOT);
}
static bool direct_resources(const char *root,const char *relative,unsigned depth){
    if(depth>8)return false;
    char dir[400];int n=snprintf(dir,sizeof(dir),"%s%s%s",root,*relative?"/":"",relative);
    if(n<0||(size_t)n>=sizeof(dir))return false;
    DIR *d=opendir(dir);if(!d)return false;
    bool ok=true;struct dirent *e;
    while(ok&&(e=readdir(d))){
        if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
        char rel[181],src[400],dst[400];struct stat st;
        n=snprintf(rel,sizeof(rel),"%s%s%s",relative,*relative?"/":"",e->d_name);
        if(n<0||(size_t)n>=sizeof(rel)){ok=false;break;}
        n=snprintf(src,sizeof(src),"%s/%s",root,rel);if(n<0||(size_t)n>=sizeof(src)||stat(src,&st)){ok=false;break;}
        if(S_ISDIR(st.st_mode)){ok=direct_resources(root,rel,depth+1);continue;}
        if(!S_ISREG(st.st_mode)||!pv_update_safe_path(rel)){ok=false;break;}
        if(!strcmp(rel,"eboot.bin")||!strcmp(rel,"sce_sys/param.sfo"))continue;
        n=snprintf(dst,sizeof(dst),MAIN_PATH "/%s",rel);
        unsigned size;char hash[65];
        ok=n>=0&&(size_t)n<sizeof(dst)&&file_size(src,&size)&&hash_file(src,hash,size)&&match_hash(dst,size,hash);
    }
    closedir(d);return ok;
}
static bool direct_prepare(const Candidate *c,const char *root){
    /* Only the paired, private B5 -> B6 test is allowed to exercise this
     * path; all other releases still use the normal B4 helper. */
    if(strcmp(PLUS_VITA_APP_VERSION,"v0.0.6-b5")||strcmp(c->tag,"v0.0.6-b6"))return false;
    char title[32]={0},sfo[400],eboot[400],old[16]={0},next[16]={0};
    if(sceAppMgrAppParamGetString(0,12,title,sizeof(title))<0||strcmp(title,PV_MAIN_TITLE))return false;
    snprintf(sfo,sizeof(sfo),"%s/sce_sys/param.sfo",root);
    snprintf(eboot,sizeof(eboot),"%s/eboot.bin",root);
    if(!read_version(direct_live[0],PV_MAIN_TITLE,old)||strcmp(old,"00.32")||
       !read_version(sfo,PV_MAIN_TITLE,next)||strcmp(next,"00.33")||
       !direct_resources(root,"",0))return false;
    if(mkdir(DIRECT_ROOT,0700)!=0)return false; /* never overwrite an earlier recovery */
    DirectJob j={0};
    if(!direct_copy(direct_live[0],direct_files[0],&j,0)||
       !direct_copy(direct_live[1],direct_files[1],&j,1)||
       !direct_copy(sfo,direct_files[2],&j,2)||
       !direct_copy(eboot,direct_files[3],&j,3)){direct_clean();return false;}
    char raw[400];int n=snprintf(raw,sizeof(raw),"PVD1 %u %s %u %s %u %s %u %s\n",
        j.size[0],j.hash[0],j.size[1],j.hash[1],j.size[2],j.hash[2],j.size[3],j.hash[3]);
    if(n<0||(size_t)n>=sizeof(raw)||!write_new(DIRECT_JOB,raw,(size_t)n)){direct_clean();return false;}
    sceIoSync("ux0:",0);
    direct_pending=true;return true;
}
static bool direct_restore(const DirectJob *j){
    bool ok=true;
    const char *const tmp[]={DIRECT_ROOT "/restore-sfo",DIRECT_ROOT "/restore-eboot"};
    /* Roll back only known old/new files; never replace an unknown file. */
    for(unsigned i=0;i<2;i++){
        if(direct_matches(direct_live[i],j,i))continue;
        if(!direct_matches(direct_live[i],j,i+2)||!direct_matches(direct_files[i],j,i)){ok=false;continue;}
        DirectJob copy={0};
        if(!direct_copy(direct_files[i],tmp[i],&copy,i)){ok=false;continue;}
        if(!direct_matches(tmp[i],j,i)||rename(tmp[i],direct_live[i])!=0||
           !direct_matches(direct_live[i],j,i))ok=false;
        unlink(tmp[i]);
    }
    sceIoSync("ux0:",0);return ok;
}
void pv_direct_cancel_pending(void){
    if(direct_pending){direct_pending=false;direct_clean();}
}
bool pv_direct_commit_pending(void){
    if(!direct_pending)return false;
    direct_pending=false;DirectJob j={0};
    if(!direct_read(&j)||!direct_matches(direct_live[0],&j,0)||!direct_matches(direct_live[1],&j,1)||
       !direct_matches(direct_files[0],&j,0)||!direct_matches(direct_files[1],&j,1)||
       !direct_matches(direct_files[2],&j,2)||!direct_matches(direct_files[3],&j,3))return true;
    /* Unlike VitaDB, only attempt this after UI and network shutdown.
     * Never unlink a live file to work around FAT rename limitations. */
    if(sceAppMgrUmount("app0:")<0)return true;
    if(rename(direct_files[2],direct_live[0])!=0||
       !direct_matches(direct_live[0],&j,2)||
       rename(direct_files[3],direct_live[1])!=0||
       !direct_matches(direct_live[1],&j,3)){
        (void)direct_restore(&j);return true;
    }
    sceIoSync("ux0:",0);
    /* Successful LoadExec does not return. On failure restore both files. */
    sceAppMgrLoadExec("app0:eboot.bin",NULL,NULL);
    (void)direct_restore(&j);return true;
}
bool pv_direct_recover_startup(void){
    struct stat st;DirectJob j={0};
    if(stat(DIRECT_JOB,&st)!=0||stat(DIRECT_OK,&st)==0)return true;
    if(!direct_read(&j))return true; /* preserve for manual repair */
    if(!strcmp(PLUS_VITA_APP_VERSION,"v0.0.6-b6")&&
       direct_matches(direct_live[0],&j,2)&&direct_matches(direct_live[1],&j,3))return true;
    if(strcmp(PLUS_VITA_APP_VERSION,"v0.0.6-b5"))return true;
    if(direct_matches(direct_live[0],&j,0)&&direct_matches(direct_live[1],&j,1)){
        /* The B5 executable is intact. Keep backups and permit helper retry. */
        return true;
    }
    if(sceAppMgrUmount("app0:")<0)return true;
    if(direct_restore(&j))sceAppMgrLoadExec("app0:eboot.bin",NULL,NULL);
    return false;
}
void pv_direct_confirm(void){
    DirectJob j={0};
    if(!strcmp(PLUS_VITA_APP_VERSION,"v0.0.6-b6")&&direct_read(&j)&&
       direct_matches(direct_live[0],&j,2)&&direct_matches(direct_live[1],&j,3)){
        (void)write_new(DIRECT_OK,"B6\n",3);sceIoSync("ux0:",0);
    }
}
static bool direct_local_candidate(PvUpdater *u){
    if(!u->report.beta||strcmp(PLUS_VITA_APP_VERSION,"v0.0.6-b5"))return false;
    char sums[16385],hash[65];size_t n=0;unsigned size;
    if(!read_small(DIRECT_SLOT,(unsigned char *)sums,sizeof(sums),&n))return false;
    sums[n]=0;
    if(!pv_update_checksum(sums,"VitaPlus-v0.0.6-b6.vpk",hash)||!file_size(VPK,&size)||
       !match_hash(VPK,size,hash)||!pv_update_identity(VPK))return false;
    Candidate *c=&u->candidate;memset(c,0,sizeof(*c));c->size=size;
    strcpy(c->tag,"v0.0.6-b6");strcpy(c->asset,"VitaPlus-v0.0.6-b6.vpk");
    strcpy(c->hash,hash);strcpy(c->path,VPK);
    strcpy(c->notes,"Local B6 test. Verify the checksum against the GitHub Actions artifact before installing.");
    return true;
}
#endif

static PvUpdateState install_fail(PvUpdater *u,unsigned stage,int code){u->install_stage=stage;u->install_error=code;return PV_UP_ERROR;}
static PvUpdateState install(PvUpdater *u){
    Candidate *c=&u->candidate;char root[128],path[256],ver[16]={0},current[16]={0};
    struct stat st;PvHandoffJob job={0};
    if(strcmp(c->path,VPK)||!c->size||c->size>MAX_VPK||!match_hash(VPK,(unsigned)c->size,c->hash))return install_fail(u,1,-1);
    /* Do not touch another app if the reserved ID is occupied. A stale
     * pending job is handled on the next main startup. */
    if(stat(HANDOFF,&st)==0||stat(HANDOFF_RESULT,&st)==0||stat(HELPER_PATH,&st)==0)return install_fail(u,12,-1);
    job.size=(unsigned)c->size;strcpy(job.hash,c->hash);
    if(!file_size("app0:eboot.bin",&job.helper_size)||!hash_file("app0:eboot.bin",job.helper_hash,job.helper_size))return install_fail(u,12,-2);
    if(!new_stage(root))return install_fail(u,2,-errno);
    if(!pv_update_unpack(VPK,root)){pv_update_clean_stage(root);return install_fail(u,3,-1);}
    snprintf(path,sizeof(path),"%s/sce_sys/param.sfo",root);
    if(!read_version(path,PV_MAIN_TITLE,ver)||!read_version("app0:sce_sys/param.sfo",PV_MAIN_TITLE,current)||strcmp(ver,current)<=0){
        pv_update_clean_stage(root);return install_fail(u,4,-1);
    }
#if PLUS_VITA_DIRECT_EXPERIMENT
    if(u->command==PV_UP_CMD_INSTALL&&direct_prepare(c,root)){
        pv_update_clean_stage(root);return PV_UP_HANDOFF;
    }
#endif
    memcpy(job.appver,ver,6);
    if(!helper_sfo(path)){pv_update_clean_stage(root);return install_fail(u,12,-3);}
    snprintf(path,sizeof(path),"%s/eboot.bin",root);
    if(!copy_helper_exe(path,&job)||!stage_header(root,PV_HELPER_TITLE,ver)){
        pv_update_clean_stage(root);return install_fail(u,12,-4);
    }
    char request[200];
    if(!pv_handoff_format(&job,request,sizeof(request))||!write_new(HANDOFF,request,strlen(request))){
        pv_update_clean_stage(root);return install_fail(u,13,-1);
    }
    unsigned stage=0;int rc=promote(root,false,&stage);
    pv_update_clean_stage(root);
    if(rc<0||!helper_owned(&job)){
        /* Keep the job for safe ownership-checked recovery on next launch. */
        return install_fail(u,stage,rc<0?rc:-1);
    }
    rc=sceAppMgrLaunchAppByUri(0xFFFFF,"psgm:play?titleid=" PV_HELPER_TITLE);
    if(rc<0)return install_fail(u,14,rc);
    return PV_UP_HANDOFF;
}
bool pv_update_is_helper(void){
    char title[32]={0};
    return sceAppMgrAppParamGetString(0,12,title,sizeof(title))>=0&&!strcmp(title,PV_HELPER_TITLE);
}
static void helper_result(unsigned stage,int rc){
    char s[64];int n=snprintf(s,sizeof(s),"PVH1 %u %d\n",stage,rc);
    if(n>0&&(size_t)n<sizeof(s))write_new(HANDOFF_RESULT,s,(size_t)n);
}
/* This entrypoint runs under the distinct helper title, before any account,
 * networking, migration or UI initialization. Only a locally approved,
 * verified package in the fixed Downloads slot can be installed. */
int pv_update_run_helper(void){
    PvHandoffJob job;struct stat st;char current[16]={0},ver[16]={0},root[128],path[256];
    unsigned stage=15;int rc=-1;
    if(!pv_update_is_helper())return 1;
    if(stat(HANDOFF_RESULT,&st)==0)return 0;
    if(!read_job(&job)||!helper_owned(&job)||!match_hash(VPK,job.size,job.hash))goto finish;
    if(!read_version(MAIN_PATH "/sce_sys/param.sfo",PV_MAIN_TITLE,current))goto finish;
    if(!strcmp(current,job.appver)){stage=0;rc=0;goto finish;}
    if(strcmp(current,job.appver)>0)goto finish;
    stage=16;
    /* User explicitly chose Install. Close the previous foreground title
     * before promotion, following VitaShell's separate-updater pattern. */
    sceKernelDelayThread(500000);
    rc=sceAppMgrDestroyOtherApp();if(rc<0)goto finish;
    sceKernelDelayThread(300000);
    stage=2;
    if(!new_stage(root)){rc=-errno;goto finish;}
    stage=3;rc=-1;
    if(!pv_update_unpack(VPK,root))goto cleanup;
    snprintf(path,sizeof(path),"%s/sce_sys/param.sfo",root);
    stage=4;
    if(!read_version(path,PV_MAIN_TITLE,ver)||strcmp(ver,job.appver)||!stage_header(root,PV_MAIN_TITLE,ver))goto cleanup;
    rc=promote(root,false,&stage);if(rc<0)goto cleanup;
    stage=11;rc=-1;
    if(!read_version(MAIN_PATH "/sce_sys/param.sfo",PV_MAIN_TITLE,current)||strcmp(current,job.appver))goto cleanup;
    stage=0;rc=0;
cleanup:
    pv_update_clean_stage(root);
finish:
    helper_result(stage,rc);
    /* Keep the verified VPK even on success until the new main app confirms
     * the installed version and consumes the receipt. */
    /* Exit the helper normally (main calls sceKernelExitProcess). Never
     * race a launch of the just-promoted title; the user reopens it manually. */
    return 0;
}
static void recover_handoff(PvUpdater *u){
    struct stat st;PvHandoffJob job;
    if(stat(HANDOFF,&st)!=0)return;
    unsigned stage=15;int rc=-1;char raw[64],version[16]={0};size_t n=0;
    if(!read_job(&job)){u->report.state=PV_UP_ERROR;u->report.install_stage=15;u->report.install_error=-2;return;}
    if(read_small(HANDOFF_RESULT,(unsigned char *)raw,sizeof(raw),&n)){
        raw[n]=0;char extra;
        if(sscanf(raw,"PVH1 %u %d %c",&stage,&rc,&extra)!=2||stage>16){stage=15;rc=-3;}
    }
    bool installed=read_version("app0:sce_sys/param.sfo",PV_MAIN_TITLE,version)&&!strcmp(version,job.appver);
    if(installed&&stage==0&&rc==0){
        if(match_hash(VPK,job.size,job.hash))unlink(VPK);
        u->report.state=PV_UP_CURRENT;
    }else{
        u->report.state=PV_UP_ERROR;u->report.install_stage=stage?stage:11;u->report.install_error=rc?rc:-1;
        strcpy(u->report.file,VPK);
    }
    /* Delete only our exact helper, never the main app or unrelated apps.
     * If the helper is still exiting, keep the receipt for next startup. */
    if(stat(HELPER_PATH,&st)==0){
        if(!helper_owned(&job))return;
        sceKernelDelayThread(500000);
        unsigned cleanup_stage=0;
        if(promote(NULL,true,&cleanup_stage)<0)return;
    }
    unlink(HANDOFF_RESULT);unlink(HANDOFF);
}
static void *worker(void *opaque){
    PvUpdater *u=opaque;
    PvUpdateState state;
#if PLUS_VITA_DIRECT_EXPERIMENT
    if(u->command==PV_UP_CMD_CHECK&&direct_local_candidate(u))state=PV_UP_READY;
    else
#endif
    state=u->command==PV_UP_CMD_CHECK?check(u):u->command==PV_UP_CMD_DOWNLOAD?download(u):install(u);
    if(atomic_load(&u->cancel)&&u->command!=PV_UP_CMD_INSTALL&&u->command!=PV_UP_CMD_INSTALL_HELPER&&state!=PV_UP_READY)state=PV_UP_CANCELLED;
    u->result=state;atomic_store(&u->done,true);return NULL;
}
static bool save_prefs(PvUpdater *u){
    FILE *f=fopen(PREFS ".tmp","wb");if(!f)return false;
    bool ok=fprintf(f,"%u %u\n",(unsigned)u->report.beta,(unsigned)u->report.automatic)>0;
    if(fclose(f)!=0)ok=false;
    if(!ok||rename(PREFS ".tmp",PREFS)!=0){unlink(PREFS ".tmp");return false;}
    return true;
}
PvUpdater *pv_updater_create(void){
    PvUpdater *u=calloc(1,sizeof(*u));if(!u)return NULL;
    atomic_init(&u->done,false);atomic_init(&u->cancel,false);atomic_init(&u->progress,0);
    u->report.automatic=true;u->dirty=true;
    FILE *f=fopen(PREFS,"rb");if(f){unsigned b=0,a=0;char extra;if(fscanf(f,"%u %u %c",&b,&a,&extra)==2&&b<=1&&a<=1){u->report.beta=b!=0;u->report.automatic=a!=0;}fclose(f);}
    recover_handoff(u);
    return u;
}
bool pv_updater_busy(const PvUpdater *u){return u&&u->running;}
bool pv_updater_automatic(const PvUpdater *u){return u&&u->report.automatic&&!u->report.install_stage;}
void pv_updater_destroy(PvUpdater *u){
    if(!u)return;
    if(u->running){if(u->command!=PV_UP_CMD_INSTALL&&u->command!=PV_UP_CMD_INSTALL_HELPER)atomic_store(&u->cancel,true);pthread_join(u->thread,NULL);}
    free(u);
}
bool pv_updater_command(PvUpdater *u,PvUpdateCommand cmd,unsigned lang){
    if(!u)return false;
    if(cmd==PV_UP_CMD_CANCEL){if(u->running&&u->command!=PV_UP_CMD_INSTALL&&u->command!=PV_UP_CMD_INSTALL_HELPER)atomic_store(&u->cancel,true);u->dirty=true;return true;}
    if(u->running){u->dirty=true;return false;}
    if(cmd==PV_UP_CMD_CHANNEL||cmd==PV_UP_CMD_AUTO){
        bool b=u->report.beta,a=u->report.automatic;
        if(cmd==PV_UP_CMD_CHANNEL)u->report.beta=!b;else u->report.automatic=!a;
        if(!save_prefs(u)){u->report.beta=b;u->report.automatic=a;u->report.state=PV_UP_ERROR;}
        else if(cmd==PV_UP_CMD_CHANNEL){memset(&u->candidate,0,sizeof(u->candidate));u->report.state=PV_UP_IDLE;u->report.version[0]=u->report.notes[0]=u->report.file[0]=0;}
        u->dirty=true;return true;
    }
    if((cmd==PV_UP_CMD_DOWNLOAD&&u->report.state!=PV_UP_AVAILABLE)||((cmd==PV_UP_CMD_INSTALL||cmd==PV_UP_CMD_INSTALL_HELPER)&&u->report.state!=PV_UP_READY)){u->dirty=true;return false;}
    if((cmd==PV_UP_CMD_INSTALL||cmd==PV_UP_CMD_INSTALL_HELPER)&&u->install_unsafe)return false;
    u->install_stage=0;u->install_error=0;u->report.install_stage=0;u->report.install_error=0;
    u->command=cmd;u->lang=lang;atomic_store(&u->cancel,false);atomic_store(&u->done,false);atomic_store(&u->progress,0);u->last_progress=0;
    u->report.progress=0;u->report.state=cmd==PV_UP_CMD_CHECK?PV_UP_CHECKING:cmd==PV_UP_CMD_DOWNLOAD?PV_UP_DOWNLOADING:PV_UP_INSTALLING;
    pthread_attr_t attr;int rc=pthread_attr_init(&attr);
    if(!rc){rc=pthread_attr_setstacksize(&attr,256*1024);if(!rc)rc=pthread_create(&u->thread,&attr,worker,u);pthread_attr_destroy(&attr);}
    u->running=rc==0;
    if(!u->running)u->report.state=PV_UP_ERROR;
    u->dirty=true;return u->running;
}
bool pv_updater_poll(PvUpdater *u,PvUpdateReport *out){
    if(!u||!out)return false;
    if(u->running&&atomic_load(&u->done)){
        pthread_join(u->thread,NULL);u->running=false;u->report.state=u->result;
        if(u->command==PV_UP_CMD_CHECK){
            u->report.version[0]=u->report.notes[0]=u->report.file[0]=0;
            PvVersion v;if((u->result==PV_UP_AVAILABLE||u->result==PV_UP_READY)&&pv_version_parse(u->candidate.tag,&v)){pv_version_display(v,u->report.version);strcpy(u->report.notes,u->candidate.notes);}
        }
        if(u->result==PV_UP_READY)strcpy(u->report.file,u->candidate.path);
        u->report.install_stage=u->install_stage;u->report.install_error=u->install_error;
        u->dirty=true;
    }
    unsigned p=atomic_load(&u->progress);if(p!=u->last_progress){u->last_progress=p;u->report.progress=p;u->dirty=true;}
    if(!u->dirty)return false;
    *out=u->report;u->dirty=false;return true;
}
