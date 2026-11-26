// SPDX-License-Identifier: AGPL-3.0-only
#include "wallpaper_cache.h"
#include "../platform/cover_download.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#define SOURCE "ux0:data/VitaPlus-Wallpapers"
#define CACHE "ux0:data/VitaPlus-Themes-Cache"
#define PIXEL_BYTES (PV_WALL_WIDTH*PV_WALL_HEIGHT*sizeof(uint32_t))
#define MAX_SOURCE (2u*1024u*1024u)
typedef struct {uint64_t magic,stamp,size,format,hash;} CacheHeader;
static uint64_t hash_bytes(const void *data,size_t size){
    const unsigned char *p=data;uint64_t h=UINT64_C(14695981039346656037);
    for(size_t j=0;j<size;j++){h^=p[j];h*=UINT64_C(1099511628211);}return h;
}
static uint64_t epoch(void){time_t t=time(NULL);return t>=(time_t)946684800?(uint64_t)t:0;}
static void marker(char *path,unsigned id){snprintf(path,128,CACHE "/%02u.used",id);}
static void cached(char *path,unsigned id,unsigned slot){snprintf(path,128,CACHE "/%02u-%u.pvw",id,slot+1);}
static bool regular(const char *path,struct stat *st){return stat(path,st)==0&&S_ISREG(st->st_mode);}
static void stamp(unsigned id,uint64_t now){
    if(!now)return;
    char path[128];marker(path,id);FILE *f=fopen(path,"wb");if(!f)return;
    (void)fwrite(&now,sizeof(now),1,f);fclose(f);
}
static void prune(unsigned active,uint64_t now,const atomic_bool *cancel){
    if(!now)return;
    for(unsigned id=0;id<=PV_THEME_COUNT&&!atomic_load(cancel);id++){
        if(id==active)continue;
        char path[128];marker(path,id);uint64_t last=0;FILE *f=fopen(path,"rb");
        if(f){if(fread(&last,sizeof(last),1,f)!=1)last=0;fclose(f);}
        /* A damaged/missing marker must never delete a user's source images.
         * Recover expiry from the newest cache-file modification time. */
        if(last<946684800u){
            for(unsigned slot=0;slot<PV_THEME_SLOTS;slot++){struct stat st;cached(path,id,slot);
                if(regular(path,&st)&&st.st_mtime>0&&(uint64_t)st.st_mtime>last)last=(uint64_t)st.st_mtime;}
        }
        if(pv_theme_expired(now,last,id,active)){
            for(unsigned slot=0;slot<PV_THEME_SLOTS;slot++){cached(path,id,slot);remove(path);}
            marker(path,id);remove(path);
        }
    }
}
static unsigned char *read_file(const char *path,size_t *len){
    struct stat st;if(!regular(path,&st)||st.st_size<=0||(uint64_t)st.st_size>MAX_SOURCE)return NULL;
    size_t n=(size_t)st.st_size;unsigned char *data=malloc(n);if(!data)return NULL;
    FILE *f=fopen(path,"rb");if(!f){free(data);return NULL;}
    bool ok=fread(data,1,n,f)==n&&fgetc(f)==EOF&&!ferror(f);fclose(f);
    if(!ok){free(data);return NULL;}*len=n;return data;
}
static bool cache_read(const char *path,CacheHeader expected,uint32_t *pixels){
    FILE *f=fopen(path,"rb");if(!f)return false;CacheHeader actual={0};
    bool ok=fread(&actual,sizeof(actual),1,f)==1&&actual.magic==expected.magic&&actual.stamp==expected.stamp
        &&actual.size==expected.size&&actual.format==expected.format&&fread(pixels,1,PIXEL_BYTES,f)==PIXEL_BYTES
        &&fgetc(f)==EOF&&!ferror(f)&&actual.hash==hash_bytes(pixels,PIXEL_BYTES);
    fclose(f);return ok;
}
static void cache_write(const char *path,CacheHeader header,const uint32_t *pixels,const atomic_bool *cancel){
    if(atomic_load(cancel))return;
    char tmp[144];snprintf(tmp,sizeof(tmp),"%s.tmp",path);FILE *f=fopen(tmp,"wb");if(!f)return;
    header.hash=hash_bytes(pixels,PIXEL_BYTES);
    bool ok=fwrite(&header,sizeof(header),1,f)==1&&fwrite(pixels,1,PIXEL_BYTES,f)==PIXEL_BYTES;
    if(fflush(f)!=0)ok=false;
    if(fclose(f)!=0)ok=false;
    if(ok&&!atomic_load(cancel)){if(rename(tmp,path)!=0){remove(path);(void)rename(tmp,path);}}
    remove(tmp);
}
static void *worker(void *arg){
    PvWallpaperJob *j=arg;
    (void)mkdir(SOURCE,0700);(void)mkdir(CACHE,0700);
    uint64_t now=epoch();
    if(j->previous_id<=PV_THEME_COUNT&&!atomic_load(&j->cancel))stamp(j->previous_id,now);
    if(now&&(!j->last_prune||now<j->last_prune||now-j->last_prune>=3600u)){
        prune(j->id,now,&j->cancel);if(!atomic_load(&j->cancel))j->last_prune=now;
    }
    if(!atomic_load(&j->cancel)){
        stamp(j->id,now);
        char source[128],path[128];struct stat st;unsigned format=1;
        snprintf(source,sizeof(source),SOURCE "/%02u-%u.jpg",j->id,j->slot+1);
        bool exists=regular(source,&st);
        if(!exists){format=2;snprintf(source,sizeof(source),SOURCE "/%02u-%u.png",j->id,j->slot+1);exists=regular(source,&st);}
        cached(path,j->id,j->slot);
        if(!exists)remove(path);
        else if(st.st_size>0&&(uint64_t)st.st_size<=MAX_SOURCE&&!atomic_load(&j->cancel)){
            CacheHeader header={UINT64_C(0x505657414c4c3031),(uint64_t)st.st_mtime,(uint64_t)st.st_size,format,0};
            j->ok=cache_read(path,header,j->output);
            if(!j->ok){size_t n=0;unsigned char *data=read_file(source,&n);
                if(data&&!atomic_load(&j->cancel))j->ok=plus_vita_theme_decode(data,n,j->output,PV_WALL_WIDTH,PV_WALL_HEIGHT,true);
                free(data);if(j->ok)cache_write(path,header,j->output,&j->cancel);
            }
        }
    }
    j->used=now;
    if(atomic_load(&j->cancel))j->ok=false;
    atomic_store(&j->done,true);return NULL;
}
bool pv_wallpaper_start(PvWallpaperJob *j,unsigned id,unsigned slot,unsigned previous_id){
    if(j->running||id>PV_THEME_COUNT||slot>=PV_THEME_SLOTS)return false;
    if(!j->output)j->output=malloc(PIXEL_BYTES);
    if(!j->output)return false;
    j->id=id;j->slot=slot;j->previous_id=previous_id;j->ok=false;
    atomic_init(&j->done,false);atomic_init(&j->cancel,false);
    pthread_attr_t attr;if(pthread_attr_init(&attr)!=0)return false;
    int rc=pthread_attr_setstacksize(&attr,512*1024);
    if(!rc)rc=pthread_create(&j->thread,&attr,worker,j);
    pthread_attr_destroy(&attr);j->running=rc==0;return j->running;
}
bool pv_wallpaper_poll(PvWallpaperJob *j){
    if(!j->running||!atomic_load(&j->done))return false;
    pthread_join(j->thread,NULL);j->running=false;return true;
}
bool pv_wallpaper_close(PvWallpaperJob *j){
    if(j->running){atomic_store(&j->cancel,true);pv_wallpaper_poll(j);}
    if(j->running)return false;
    free(j->output);j->output=NULL;return true;
}
uint32_t *pv_wallpaper_headers(void){
    size_t n=0;unsigned char *data=read_file("app0:/branding/theme-headers.jpg",&n);
    if(!data)return NULL;
    uint32_t *out=malloc(PV_HEADER_WIDTH*PV_HEADER_HEIGHT*3*sizeof(*out));
    if(out&&!plus_vita_theme_decode(data,n,out,PV_HEADER_WIDTH,PV_HEADER_HEIGHT*3,false)){free(out);out=NULL;}
    free(data);return out;
}
