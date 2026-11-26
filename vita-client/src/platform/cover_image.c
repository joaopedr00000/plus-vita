// SPDX-License-Identifier: AGPL-3.0-only
#include "cover_download.h"
#include <stdlib.h>
#include <pthread.h>
typedef union { max_align_t alignment; size_t bytes; } Allocation;
static size_t allocated;
static pthread_mutex_t memory_mutex=PTHREAD_MUTEX_INITIALIZER;
static void *image_alloc(size_t bytes)
{
	pthread_mutex_lock(&memory_mutex);
	if(bytes>8*1024*1024u-allocated) { pthread_mutex_unlock(&memory_mutex); return NULL; }
	Allocation *p=malloc(sizeof(*p)+bytes);
	if(p) { p->bytes=bytes; allocated+=bytes; }
	pthread_mutex_unlock(&memory_mutex);
	return p ? p+1 : NULL;
}
static void image_free(void *memory)
{
	if(!memory) return;
	Allocation *p=(Allocation *)memory-1;
	pthread_mutex_lock(&memory_mutex); allocated-=p->bytes; pthread_mutex_unlock(&memory_mutex);
	free(p);
}
static void *image_realloc(void *memory,size_t bytes)
{
	if(!memory) return image_alloc(bytes);
	Allocation *p=(Allocation *)memory-1;
	pthread_mutex_lock(&memory_mutex);
	if(bytes>8*1024*1024u-(allocated-p->bytes)) { pthread_mutex_unlock(&memory_mutex); return NULL; }
	size_t old=p->bytes; Allocation *grown=realloc(p,sizeof(*p)+bytes);
	if(grown) { grown->bytes=bytes; allocated=allocated-old+bytes; }
	pthread_mutex_unlock(&memory_mutex);
	return grown ? grown+1 : NULL;
}
#define STBI_MALLOC image_alloc
#define STBI_REALLOC image_realloc
#define STBI_FREE image_free
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_FAILURE_STRINGS
#define STBI_MAX_DIMENSIONS 1024
#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb_image.h"

bool plus_vita_cover_decode(const unsigned char *data,size_t size,uint32_t pixels[128*128])
{
	if(!data || !size || size>2*1024*1024u) return false;
	int width=0,height=0,channels=0;
	if(!stbi_info_from_memory(data,(int)size,&width,&height,&channels) || width<1 || height<1 || width>1024 || height>1024) return false;
	unsigned char *rgba=stbi_load_from_memory(data,(int)size,&width,&height,&channels,4);
	if(!rgba) return false;
	int w=width>=height ? 128 : width*128/height, h=height>=width ? 128 : height*128/width;
	if(w<1) w=1;
	if(h<1) h=1;
	#if PLUS_VITA_ADAPTIVE_STREAM
	const unsigned bg[]={29,29,31};
	const uint32_t background=0xff1f1d1du;
#else
	const unsigned bg[]={9,29,66};
	const uint32_t background=0xff421d09u;
#endif
	for(unsigned i=0;i<128*128;i++) pixels[i]=background;
	for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
		unsigned char *p=rgba+((y*height/h)*width+x*width/w)*4;
		unsigned a=p[3]; uint32_t color=0xff000000;
		for(unsigned c=0;c<3;c++) color|=((p[c]*a+bg[c]*(255-a))/255)<<(c*8);
		pixels[(y+(128-h)/2)*128+x+(128-w)/2]=color;
	}
	stbi_image_free(rgba);
	return true;
}

bool plus_vita_theme_decode(const unsigned char *data,size_t size,uint32_t *pixels,unsigned w,unsigned h,bool crop)
{
    if(!data||!pixels||!size||size>2u*1024u*1024u||!w||!h||w>480||h>272)return false;
    int iw=0,ih=0,ch=0;
    if(!stbi_info_from_memory(data,(int)size,&iw,&ih,&ch)||iw<1||ih<1||iw>1024||ih>1024)return false;
    unsigned char *rgba=stbi_load_from_memory(data,(int)size,&iw,&ih,&ch,4);
    if(!rgba)return false;
    unsigned sw=(unsigned)iw,sh=(unsigned)ih,x0=0,y0=0;
    if(crop){
        if(sw*h>sh*w){unsigned cw=sh*w/h;if(!cw)cw=1;x0=(sw-cw)/2;sw=cw;}
        else {unsigned hh=sw*h/w;if(!hh)hh=1;y0=(sh-hh)/2;sh=hh;}
    }
    for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
        const unsigned char *src=rgba+((y0+y*sh/h)*(unsigned)iw+x0+x*sw/w)*4;
        unsigned opacity=src[3];
        uint32_t pixel=0xff000000u;
        for(unsigned k=0;k<3;k++)pixel|=((src[k]*opacity+16u*(255-opacity))/255u)<<(8*k);
        pixels[y*w+x]=pixel;
    }
    stbi_image_free(rgba);return true;
}
