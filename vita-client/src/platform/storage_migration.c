// SPDX-License-Identifier: AGPL-3.0-only
#ifndef __vita__
#define _POSIX_C_SOURCE 200809L
#endif
#include "storage_migration.h"
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define CAP 512
static bool join(char out[CAP],const char *parent,const char *name){
    int n=snprintf(out,CAP,"%s/%s",parent,name);return n>0&&n<CAP;
}
static int kind(const char *path,struct stat *st){
#ifdef __vita__
    int rc=stat(path,st);
#else
    int rc=lstat(path,st);
#endif
    if(rc!=0)return errno==ENOENT?0:-1;
    return S_ISREG(st->st_mode)?1:S_ISDIR(st->st_mode)?2:-1;
}
static bool directory(const char *path){
    struct stat st;int k=kind(path,&st);return k==2||(k==0&&mkdir(path,0700)==0);
}
static void wipe(void *p,size_t n){volatile unsigned char *b=p;while(n--)*b++=0;}
static bool equal_files(const char *a,const char *b){
    FILE *fa=fopen(a,"rb"),*fb=fopen(b,"rb");bool ok=fa&&fb;
    unsigned char aa[4096],bb[4096];
    if(ok)for(;;){
        size_t na=fread(aa,1,sizeof(aa),fa),nb=fread(bb,1,sizeof(bb),fb);
        if(na!=nb||memcmp(aa,bb,na)||ferror(fa)||ferror(fb)){ok=false;break;}
        if(!na)break;
    }
    if(fa&&fclose(fa)!=0)ok=false;
    if(fb&&fclose(fb)!=0)ok=false;
    wipe(aa,sizeof(aa));wipe(bb,sizeof(bb));return ok;
}
static bool move_entry(const char *src,const char *dst,unsigned depth){
    if(depth>12)return false;
    struct stat a,b;int ka=kind(src,&a);if(!ka)return true;if(ka<0)return false;
    int kb=kind(dst,&b);if(kb<0)return false;
    if(!kb){if(rename(src,dst)!=0)return false;return kind(dst,&b)==ka&&(ka!=1||a.st_size==b.st_size);}
    if(ka!=kb)return false;
    if(ka==1){if(a.st_size!=b.st_size||!equal_files(src,dst))return false;return unlink(src)==0;}
    for(;;){
        DIR *dir=opendir(src);if(!dir)return false;
        struct dirent *e;char name[256]={0};bool ok=true;errno=0;
        while((e=readdir(dir))){
            if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
            size_t len=strlen(e->d_name);
            if(!len||len>=sizeof(name)||strchr(e->d_name,'/')){ok=false;break;}
            memcpy(name,e->d_name,len+1);break;
        }
        if(!e&&errno)ok=false;
        if(closedir(dir)!=0)ok=false;
        if(!ok)return false;
        if(!name[0])return rmdir(src)==0;
        char x[CAP],y[CAP];if(!join(x,src,name)||!join(y,dst,name)||!move_entry(x,y,depth+1))return false;
    }
}
static bool move_relative(const char *from,const char *old,const char *to,const char *name){
    char a[CAP],b[CAP];return join(a,from,old)&&join(b,to,name)&&move_entry(a,b,0);
}
static bool dated_log(const char *name){
    const char *p="VitaPlus-Diagnostic-";size_t n=strlen(p);
    if(strncmp(name,p,n)||strlen(name)!=n+20||strcmp(name+n+16,".log"))return false;
    for(unsigned j=0;j<16;j++){char c=name[n+j];if(j==2||j==5||j==10||j==13){if(c!='-')return false;}else if(c<'0'||c>'9')return false;}
    return true;
}
static bool archive_logs(const char *data,const char *archive){
    static const char *const names[]={"plus-vita-allocation.log","plus-vita-v0.0.1.log","plus-vita-catalog-r2.log","plus-vita-catalog-r3.log","plus-vita-ps3-replay-53.log","VitaPlus.log"};
    for(unsigned j=0;j<sizeof(names)/sizeof(names[0]);j++)if(!move_relative(data,names[j],archive,names[j]))return false;
    for(;;){
        DIR *dir=opendir(data);if(!dir)return false;
        struct dirent *e;char name[64]={0};bool ok=true;errno=0;
        while((e=readdir(dir)))if(dated_log(e->d_name)){memcpy(name,e->d_name,strlen(e->d_name)+1);break;}
        if(!e&&errno)ok=false;
        if(closedir(dir)!=0)ok=false;
        if(!ok)return false;
        if(!name[0])return true;
        if(!move_relative(data,name,archive,name))return false;
    }
}
bool pv_storage_migrate(const char *data){
    if(!data||!directory(data))return false;
    char root[CAP],cache[CAP],archive[CAP];
    if(!join(root,data,"VitaPlus")||!directory(root)||!move_relative(data,"plus-vita",root,"Account"))return false;
    if(!join(cache,root,"Cache")||!directory(cache))return false;
    if(!move_relative(data,"VitaPlus-Covers",cache,"Covers")||
       !move_relative(data,"VitaPlus-Themes-Cache",cache,"Themes")||
       !move_relative(data,"plus-vita-ps5-catalog",cache,"PS5-Catalog")||
       !move_relative(data,"VitaPlus-Wallpapers",root,"Wallpapers"))return false;
    const char *const dirs[]={"Account","Account/Keys","Cache/Covers","Cache/Themes","Cache/PS5-Catalog","Wallpapers","Settings","Imports","Logs","Logs/Archive","Captures"};
    for(unsigned j=0;j<sizeof(dirs)/sizeof(dirs[0]);j++){char p[CAP];if(!join(p,root,dirs[j])||!directory(p))return false;}
    const char *const old[]={"plus-vita-auth.json","plus-vita-controls.dat","plus-vita-stream.cfg","plus-vita-theme.cfg","plus-vita-font-A-tiles.ppm","plus-vita-font-A-setup.ppm","plus-vita-font-B-tiles.ppm","plus-vita-font-B-setup.ppm"};
    const char *const next[]={"Imports/plus-vita-auth.json","Settings/controls.dat","Settings/stream.cfg","Settings/theme.cfg","Captures/plus-vita-font-A-tiles.ppm","Captures/plus-vita-font-A-setup.ppm","Captures/plus-vita-font-B-tiles.ppm","Captures/plus-vita-font-B-setup.ppm"};
    for(unsigned j=0;j<sizeof(old)/sizeof(old[0]);j++)if(!move_relative(data,old[j],root,next[j]))return false;
    return join(archive,root,"Logs/Archive")&&archive_logs(data,archive);
}
