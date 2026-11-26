// SPDX-License-Identifier: AGPL-3.0-only
#ifndef __vita__
#define _POSIX_C_SOURCE 200809L
#endif
#include "update_package.h"
#include "update_policy.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#define LIMIT (128u*1024u*1024u)
static uint16_t le16(const unsigned char *p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t le32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static bool is_dir(const char *p){
    struct stat s;
#ifdef __vita__
    return stat(p,&s)==0&&S_ISDIR(s.st_mode);
#else
    return lstat(p,&s)==0&&S_ISDIR(s.st_mode);
#endif
}
static bool parents(char *path,size_t start){
    for(char *p=path+start;*p;p++)if(*p=='/'){*p=0;bool ok=is_dir(path)||mkdir(path,0700)==0;*p='/';if(!ok)return false;}
    return true;
}
static bool sfo_title(const unsigned char *b,size_t n){
    if(n<20||memcmp(b,"\0PSF",4))return false;
    uint32_t keys=le32(b+8),data=le32(b+12),count=le32(b+16);
    if(count>128||20u+count*16u>keys||keys>=data||data>n)return false;
    unsigned match=0;
    for(uint32_t j=0;j<count;j++){
        const unsigned char *e=b+20+j*16;uint32_t ko=le16(e),len=le32(e+4),off=le32(e+12);
        if(ko>=data-keys||off>n-data||len>n-data-off)return false;
        const char *key=(const char *)b+keys+ko;
        if(!memchr(key,0,data-keys-ko))return false;
        if(!strcmp(key,"TITLE_ID")){if(len!=10||memcmp(b+data+off,"PLVTB0001",10))return false;match++;}
    }
    return match==1;
}
typedef struct {FILE *out;unsigned char *mem;size_t size,used;uLong crc;} Writer;
static bool emit(Writer *w,const unsigned char *p,size_t n){
    if(n>w->size-w->used)return false;
    if(w->out&&fwrite(p,1,n,w->out)!=n)return false;
    if(w->mem)memcpy(w->mem+w->used,p,n);
    w->crc=crc32(w->crc,p,(uInt)n);w->used+=n;return true;
}
static bool extract(FILE *in,Writer *w,unsigned method,uint32_t packed,uint32_t crc){
    unsigned char ib[8192],ob[8192];uint32_t left=packed;bool ok=true;
    w->crc=crc32(0L,Z_NULL,0);
    if(method==0){
        if(packed!=w->size)return false;
        while(left){size_t n=left<sizeof(ib)?left:sizeof(ib);if(fread(ib,1,n,in)!=n||!emit(w,ib,n))return false;left-=(uint32_t)n;}
    }else{
        z_stream z={0};if(inflateInit2(&z,-MAX_WBITS)!=Z_OK)return false;
        int rc=Z_OK;
        while(ok&&rc!=Z_STREAM_END){
            if(!z.avail_in&&left){size_t n=left<sizeof(ib)?left:sizeof(ib);if(fread(ib,1,n,in)!=n){ok=false;break;}left-=(uint32_t)n;z.next_in=ib;z.avail_in=(uInt)n;}
            z.next_out=ob;z.avail_out=sizeof(ob);rc=inflate(&z,Z_NO_FLUSH);
            size_t n=sizeof(ob)-z.avail_out;
            if(!emit(w,ob,n)||(rc!=Z_OK&&rc!=Z_STREAM_END)||(!n&&!z.avail_in&&!left&&rc!=Z_STREAM_END))ok=false;
        }
        ok=ok&&rc==Z_STREAM_END&&!left&&!z.avail_in;inflateEnd(&z);
    }
    return ok&&w->used==w->size&&(uint32_t)w->crc==crc;
}
/* Read normalized ZIP local records emitted by our release workflow. The
 * package is authenticated by its SHA-256 before extraction. */
static bool scan(const char *vpk,const char *root){
    if(!vpk||(root&&(strlen(root)>180||!is_dir(root))))return false;
    FILE *in=fopen(vpk,"rb");if(!in)return false;
    bool ok=false,eboot=false,sfo=false;uint64_t total=0;unsigned entries=0;
    for(;;){
        unsigned char h[30];if(fread(h,1,4,in)!=4)break;
        if(le32(h)==0x02014b50u){ok=eboot&&sfo;break;}
        if(le32(h)!=0x04034b50u||++entries>128||fread(h+4,1,26,in)!=26)break;
        unsigned flags=le16(h+6),method=le16(h+8),name_len=le16(h+26),extra=le16(h+28);
        uint32_t crc=le32(h+14),packed=le32(h+18),size=le32(h+22);
        total+=size;if((flags&~0x0800u)||(method!=0&&method!=8)||!name_len||name_len>180||total>LIMIT||packed>LIMIT)break;
        char name[181];if(fread(name,1,name_len,in)!=name_len||memchr(name,0,name_len))break;name[name_len]=0;
        if(!pv_update_safe_path(name)||fseek(in,extra,SEEK_CUR))break;
        bool identity=!strcmp(name,"sce_sys/param.sfo"),self=!strcmp(name,"eboot.bin");
        if(identity&&(sfo||size>8192||!size))break;
        if(self&&eboot)break;
        unsigned char sfo_buf[8192];FILE *out=NULL;
        if(root){
            char path[400];int n=snprintf(path,sizeof(path),"%s/%s",root,name);
            if(n<0||(size_t)n>=sizeof(path)||!parents(path,strlen(root)+1))break;
            int fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0600);if(fd<0)break;
            out=fdopen(fd,"wb");if(!out){close(fd);break;}
        }
        if(!root&&!identity){
            if(fseek(in,packed,SEEK_CUR))break;
            if(self)eboot=size>=4;
            continue;
        }
        Writer w={.out=out,.mem=identity?sfo_buf:NULL,.size=size};
        bool extracted=extract(in,&w,method,packed,crc);if(out&&fclose(out)!=0)extracted=false;
        if(!extracted)break;
        if(self)eboot=size>=4;
        if(identity){sfo=sfo_title(sfo_buf,size);if(!sfo)break;}
    }
    fclose(in);return ok;
}
bool pv_update_unpack(const char *vpk,const char *root){return root&&scan(vpk,root);}
bool pv_update_identity(const char *vpk){return scan(vpk,NULL);}
static void clean(const char *root,unsigned depth){
    if(depth>8)return;
    DIR *dir=opendir(root);if(!dir)return;
    struct dirent *e;
    while((e=readdir(dir))){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
        char p[512];int n=snprintf(p,sizeof(p),"%s/%s",root,e->d_name);if(n<0||(size_t)n>=sizeof(p))continue;
        if(is_dir(p))clean(p,depth+1);else unlink(p);
    }
    closedir(dir);rmdir(root);
}
void pv_update_clean_stage(const char *root){
    if(!root)return;
    const char *last=strrchr(root,'/');if(!last||strncmp(last+1,"UpdateStage-",12))return;
    const char *p=last+13;if(!*p)return;
    while(*p){if(*p<'0'||*p>'9')return;p++;}
    clean(root,0);
}
