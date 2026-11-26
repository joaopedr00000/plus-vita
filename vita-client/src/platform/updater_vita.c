// SPDX-License-Identifier: AGPL-3.0-only
#include "updater_vita.h"
#include "update_policy.h"
#include "update_package.h"
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
/* The installed homebrew's promoter header belongs to this same title.
 * Prefer the installed header rather than embedding a foreign package blob.
 * If unavailable, fail closed and preserve the VPK for VitaShell. */
static bool stage_header(const char *root){
    char folder[180],path[200];
    snprintf(folder,sizeof(folder),"%s/sce_sys/package",root);
    if(!ensure_dir(folder))return false;
    snprintf(path,sizeof(path),"%s/head.bin",folder);
    FILE *in=fopen("app0:sce_sys/package/head.bin","rb");if(!in)return false;
    int fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0600);
    if(fd<0){fclose(in);return false;}
    FILE *out=fdopen(fd,"wb");if(!out){close(fd);fclose(in);return false;}
    unsigned char buf[4096];size_t total=0;bool ok=true;
    while(ok){size_t n=fread(buf,1,sizeof(buf),in);if(!n)break;total+=n;ok=total<=65536&&fwrite(buf,1,n,out)==n;}
    ok=ok&&!ferror(in)&&total>=256;fclose(in);if(fclose(out)!=0)ok=false;
    return ok;
}
static PvUpdateState install(PvUpdater *u){
    Candidate *c=&u->candidate;char actual[65];
    if(!c->path[0]||!hash_file(c->path,actual,c->size)||strcmp(actual,c->hash))return PV_UP_ERROR;
    char root[128];snprintf(root,sizeof(root),STAGE_ROOT "/UpdateStage-%llu",(unsigned long long)sceKernelGetProcessTimeWide());
    if(mkdir(root,0700)!=0)return PV_UP_ERROR;
    if(!pv_update_unpack(c->path,root)||!stage_header(root)){pv_update_clean_stage(root);return PV_UP_ERROR;}
    uint32_t args[]={0x180000u,UINT32_MAX,UINT32_MAX,1,UINT32_MAX,UINT32_MAX};int paf_result=-1;
    SceSysmoduleOpt opt={.flags=sizeof(SceSysmoduleOpt),.result=&paf_result,.unused={-1,-1}};
    if(sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF,sizeof(args),args,&opt)<0&&sceSysmoduleIsLoadedInternal(SCE_SYSMODULE_INTERNAL_PAF)<0){pv_update_clean_stage(root);return PV_UP_ERROR;}
    if(sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL)<0&&sceSysmoduleIsLoadedInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL)<0){pv_update_clean_stage(root);return PV_UP_ERROR;}
    int rc=scePromoterUtilityInit();if(rc<0){pv_update_clean_stage(root);return PV_UP_ERROR;}
    rc=scePromoterUtilityPromotePkgWithRif(root,0);
    if(rc>=0){
        int state=1;uint64_t start=sceKernelGetProcessTimeWide();
        while(state){
            rc=scePromoterUtilityGetState(&state);if(rc<0)break;
            if(state&&sceKernelGetProcessTimeWide()-start>180000000ull)return PV_UP_ERROR;
            if(state)sceKernelDelayThread(100000);
        }
        if(rc>=0){int result=-1;rc=scePromoterUtilityGetResult(&result);if(rc>=0)rc=result;}
        if(state&&rc<0)return PV_UP_ERROR;
    }
    scePromoterUtilityExit();
    pv_update_clean_stage(root);
    if(rc<0)return PV_UP_ERROR;
    if(hash_file(c->path,actual,c->size)&&!strcmp(actual,c->hash))unlink(c->path);
    return PV_UP_INSTALLED;
}
static void *worker(void *opaque){
    PvUpdater *u=opaque;
    PvUpdateState state=u->command==PV_UP_CMD_CHECK?check(u):u->command==PV_UP_CMD_DOWNLOAD?download(u):install(u);
    if(atomic_load(&u->cancel)&&u->command!=PV_UP_CMD_INSTALL&&state!=PV_UP_READY)state=PV_UP_CANCELLED;
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
    return u;
}
bool pv_updater_busy(const PvUpdater *u){return u&&u->running;}
bool pv_updater_automatic(const PvUpdater *u){return u&&u->report.automatic;}
void pv_updater_destroy(PvUpdater *u){
    if(!u)return;
    if(u->running){if(u->command!=PV_UP_CMD_INSTALL)atomic_store(&u->cancel,true);pthread_join(u->thread,NULL);}
    free(u);
}
bool pv_updater_command(PvUpdater *u,PvUpdateCommand cmd,unsigned lang){
    if(!u)return false;
    if(cmd==PV_UP_CMD_CANCEL){if(u->running&&u->command!=PV_UP_CMD_INSTALL)atomic_store(&u->cancel,true);u->dirty=true;return true;}
    if(u->running){u->dirty=true;return false;}
    if(cmd==PV_UP_CMD_CHANNEL||cmd==PV_UP_CMD_AUTO){
        bool b=u->report.beta,a=u->report.automatic;
        if(cmd==PV_UP_CMD_CHANNEL)u->report.beta=!b;else u->report.automatic=!a;
        if(!save_prefs(u)){u->report.beta=b;u->report.automatic=a;u->report.state=PV_UP_ERROR;}
        else if(cmd==PV_UP_CMD_CHANNEL){memset(&u->candidate,0,sizeof(u->candidate));u->report.state=PV_UP_IDLE;u->report.version[0]=u->report.notes[0]=u->report.file[0]=0;}
        u->dirty=true;return true;
    }
    if((cmd==PV_UP_CMD_DOWNLOAD&&u->report.state!=PV_UP_AVAILABLE)||(cmd==PV_UP_CMD_INSTALL&&u->report.state!=PV_UP_READY)){u->dirty=true;return false;}
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
            PvVersion v;if(u->result==PV_UP_AVAILABLE&&pv_version_parse(u->candidate.tag,&v)){pv_version_display(v,u->report.version);strcpy(u->report.notes,u->candidate.notes);}
        }
        if(u->result==PV_UP_READY)strcpy(u->report.file,u->candidate.path);
        u->dirty=true;
    }
    unsigned p=atomic_load(&u->progress);if(p!=u->last_progress){u->last_progress=p;u->report.progress=p;u->dirty=true;}
    if(!u->dirty)return false;
    *out=u->report;u->dirty=false;return true;
}
