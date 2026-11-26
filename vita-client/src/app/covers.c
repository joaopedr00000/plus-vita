// SPDX-License-Identifier: AGPL-3.0-only
#include "covers.h"
#include "../platform/cover_download.h"
#include "../platform/cover_policy.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#define COVER_DISK_SLOTS 64u
#define COVER_CACHE_MAGIC UINT64_C(0x5056434f56455231)
struct plus_vita_covers_t {
	PlusVitaCoverImage slots[6], output;
	pthread_t thread;
	atomic_bool complete,cancel;
	bool running;
	unsigned next, generation, job_generation;
	PlusVitaArtworkResult last_result;
	char directory[256];
};
/* A fixed-size direct-mapped cache, private to this device. Fingerprints
 * identify public URLs without writing the URL, account data or query string.
 * They are cache keys/checksums, not an authentication or encryption scheme.
 * At most 64 decoded images (~4 MiB) plus one temporary file. */
static uint64_t cache_hash(const void *data,size_t bytes,bool reverse)
{
	const unsigned char *p=data;
	uint64_t hash=reverse?UINT64_C(0x84222325cbf29ce4):UINT64_C(0xcbf29ce484222325);
	for(size_t i=0;i<bytes;i++){
		hash^=p[reverse?bytes-1-i:i];
		hash*=UINT64_C(0x100000001b3);
	}
	return hash;
}
static bool cache_path(const PlusVitaCovers *covers,char path[320],uint64_t keys[4])
{
	if(!covers->directory[0] || !plus_vita_cover_url_allowed(covers->output.url))return false;
	if(mkdir(covers->directory,0700)!=0 && errno!=EEXIST)return false;
	size_t len=strlen(covers->output.url);
	keys[0]=COVER_CACHE_MAGIC;
	keys[1]=cache_hash(covers->output.url,len,false);
	keys[2]=cache_hash(covers->output.url,len,true);
	keys[3]=0;
	int n=snprintf(path,320,"%s/%02u.pvc",covers->directory,(unsigned)(keys[1]%COVER_DISK_SLOTS));
	return n>0 && n<320;
}
static bool cache_read(PlusVitaCovers *covers)
{
	char path[320];uint64_t expected[4],actual[4];
	if(!cache_path(covers,path,expected))return false;
	FILE *f=fopen(path,"rb");if(!f)return false;
	bool ok=fread(actual,sizeof(actual),1,f)==1 &&
		!memcmp(actual,expected,3*sizeof(uint64_t)) &&
		fread(covers->output.pixels,sizeof(covers->output.pixels),1,f)==1 &&
		fgetc(f)==EOF && !ferror(f) &&
		actual[3]==cache_hash(covers->output.pixels,sizeof(covers->output.pixels),false);
	fclose(f);return ok;
}
static void cache_write(PlusVitaCovers *covers)
{
	char path[320],temporary[336];uint64_t keys[4];
	if(atomic_load(&covers->cancel) || !cache_path(covers,path,keys))return;
	int n=snprintf(temporary,sizeof(temporary),"%s.tmp",path);
	if(n<=0 || (size_t)n>=sizeof(temporary))return;
	keys[3]=cache_hash(covers->output.pixels,sizeof(covers->output.pixels),false);
	FILE *f=fopen(temporary,"wb");if(!f)return;
	bool ok=fwrite(keys,sizeof(keys),1,f)==1 &&
		fwrite(covers->output.pixels,sizeof(covers->output.pixels),1,f)==1;
	if(fflush(f)!=0)ok=false;
	if(fclose(f)!=0)ok=false;
	if(ok && !atomic_load(&covers->cancel)){
		/* Some Vita filesystems cannot rename over an existing cache slot. */
		if(rename(temporary,path)!=0){remove(path);rename(temporary,path);}
	}
	remove(temporary);
}
static void *load_cover(void *context)
{
	PlusVitaCovers *covers=context; unsigned char *data=NULL; size_t size=0;
	bool ok=!atomic_load(&covers->cancel) && cache_read(covers);
	if(ok)covers->output.result=PLUS_VITA_ARTWORK_OK;
	else if(!atomic_load(&covers->cancel)){
		ok=plus_vita_cover_download(covers->output.url,&data,&size,&covers->cancel,&covers->output.result);
		if(ok && !atomic_load(&covers->cancel) && !plus_vita_cover_decode(data,size,covers->output.pixels)){
			ok=false;covers->output.result=PLUS_VITA_ARTWORK_DECODE;
		}
		if(ok && !atomic_load(&covers->cancel))cache_write(covers);
	}
	if(atomic_load(&covers->cancel)){ok=false;covers->output.result=PLUS_VITA_ARTWORK_CANCELLED;}
	free(data); covers->output.state=ok ? PLUS_VITA_COVER_READY : PLUS_VITA_COVER_FAILED;
	atomic_store(&covers->complete,true); return NULL;
}
PlusVitaCovers *plus_vita_covers_create_cached(const char *directory)
{
	PlusVitaCovers *covers=calloc(1,sizeof(*covers));
	if(covers){
		atomic_init(&covers->complete,false);atomic_init(&covers->cancel,false);
		if(directory && strlen(directory)<sizeof(covers->directory))strcpy(covers->directory,directory);
	}
	return covers;
}
PlusVitaCovers *plus_vita_covers_create(void){return plus_vita_covers_create_cached(NULL);}
const PlusVitaCoverImage *plus_vita_covers_find(const PlusVitaCovers *covers,const char *url)
{
	if(!covers || !url || !*url) return NULL;
	for(unsigned i=0;i<6;i++) if(strcmp(covers->slots[i].url,url)==0) return &covers->slots[i];
	return NULL;
}
bool plus_vita_covers_poll(PlusVitaCovers *covers)
{
	if(!covers || !covers->running || !atomic_load(&covers->complete)) return false;
	pthread_join(covers->thread,NULL); covers->running=false;
	covers->last_result=atomic_load(&covers->cancel) || covers->generation!=covers->job_generation
		? PLUS_VITA_ARTWORK_CANCELLED : covers->output.result;
	if(covers->generation==covers->job_generation)
		for(unsigned i=0;i<6;i++) if(strcmp(covers->slots[i].url,covers->output.url)==0) {
			if(atomic_load(&covers->cancel)) memset(&covers->slots[i],0,sizeof(covers->slots[i]));
			else covers->slots[i]=covers->output;
		}
	return true;
}
PlusVitaArtworkResult plus_vita_covers_last_result(const PlusVitaCovers *covers)
{ return covers ? covers->last_result : PLUS_VITA_ARTWORK_UNAVAILABLE; }
void plus_vita_covers_request(PlusVitaCovers *covers,const char *const *urls,size_t count)
{
	if(!covers) return;
	bool visible=false;
	for(size_t i=0;i<count;i++) if(urls[i] && strcmp(urls[i],covers->output.url)==0) visible=true;
	if(covers->running) { if(!visible) atomic_store(&covers->cancel,true); return; }
	for(size_t i=0;i<count;i++) {
		const char *url=urls[i];
		if(!plus_vita_cover_url_allowed(url) || plus_vita_covers_find(covers,url)) continue;
		PlusVitaCoverImage *slot=&covers->slots[covers->next++ % 6];
		memset(slot,0,sizeof(*slot)); strcpy(slot->url,url); slot->state=PLUS_VITA_COVER_LOADING;
		memset(&covers->output,0,sizeof(covers->output)); strcpy(covers->output.url,url);
		covers->job_generation=covers->generation; atomic_store(&covers->complete,false); atomic_store(&covers->cancel,false);
		pthread_attr_t attr; bool ready=pthread_attr_init(&attr)==0;
		int result=ready ? pthread_attr_setstacksize(&attr,512*1024) : -1;
		if(!result) result=pthread_create(&covers->thread,&attr,load_cover,covers);
		if(ready) pthread_attr_destroy(&attr);
		covers->running=result==0;
		if(result) slot->state=PLUS_VITA_COVER_FAILED;
		return;
	}
}
void plus_vita_covers_retry(PlusVitaCovers *covers,const char *url)
{
	if(!covers || !url) return;
	for(unsigned i=0;i<6;i++) if(covers->slots[i].state==PLUS_VITA_COVER_FAILED
		&& strcmp(covers->slots[i].url,url)==0) memset(&covers->slots[i],0,sizeof(covers->slots[i]));
}
void plus_vita_covers_reset(PlusVitaCovers *covers)
{
	if(!covers) return;
	atomic_store(&covers->cancel,true); covers->generation++;
	memset(covers->slots,0,sizeof(covers->slots));
}
bool plus_vita_covers_close(PlusVitaCovers *covers)
{
	if(!covers) return true;
	atomic_store(&covers->cancel,true); plus_vita_covers_poll(covers);
	if(covers->running) return false; /* Live worker retains bounded ownership until process exit. */
	free(covers); return true;
}
