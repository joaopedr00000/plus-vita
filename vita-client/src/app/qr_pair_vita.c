// SPDX-License-Identifier: AGPL-3.0-only
#include "qr_pair_vita.h"
#include "../platform/qr_pair_crypto.h"
#include <psp2/camera.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/rng.h>
#include <psp2/net/net.h>
#include <qrcodegen.h>
#include <quirc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define W 640
#define H 360
#define BYTES (W*H*4u)
#define ALLOC ((BYTES+0x3ffffu)&~0x3ffffu)
#define DURATION 180000000ULL
static bool qr_screen(PlusVitaPairFrame frame,void *ctx,const char *heading,const char *message,const char *content)
{
	uint8_t temp[qrcodegen_BUFFER_LEN_FOR_VERSION(9)],qr[qrcodegen_BUFFER_LEN_FOR_VERSION(9)];
	unsigned char modules[53*53]={0};
	bool ok=qrcodegen_encodeText(content,temp,qr,qrcodegen_Ecc_MEDIUM,1,9,qrcodegen_Mask_AUTO,true);
	if(ok){int n=qrcodegen_getSize(qr);for(int y=0;y<n;y++)for(int x=0;x<n;x++)modules[y*n+x]=qrcodegen_getModule(qr,x,y);ok=frame(ctx,heading,message,modules,n,NULL);}
	plus_vita_pair_clear(temp,sizeof(temp));plus_vita_pair_clear(qr,sizeof(qr));plus_vita_pair_clear(modules,sizeof(modules));return ok;
}
/* QR URLs go only to Sony. The unique fragment is never sent in HTTP requests.
 * It is navigation metadata, not an OAuth credential or a pairing authorization. */
static bool sony_page(PlusVitaPairFrame frame,void *ctx,const char *heading,const char *message,const char *base)
{
	uint8_t nonce[16]={0};char url[128]={0},hex[33]={0};
	if(sceKernelGetRandomNumber(nonce,sizeof(nonce))<0)return false;
	unsigned nonzero=0;
	for(unsigned i=0;i<sizeof(nonce);i++){nonzero|=nonce[i];snprintf(hex+2*i,3,"%02x",(unsigned)nonce[i]);}
	int n=snprintf(url,sizeof(url),"%s#pv-%s",base,hex);
	bool ok=nonzero&&n>0&&(size_t)n<sizeof(url)&&qr_screen(frame,ctx,heading,message,url);
	plus_vita_pair_clear(nonce,sizeof(nonce));plus_vita_pair_clear(hex,sizeof(hex));plus_vita_pair_clear(url,sizeof(url));
	return ok;
}
static unsigned buttons(void){SceCtrlData data={0};sceCtrlPeekBufferPositive(0,&data,1);return data.buttons;}
static bool next_page(uint64_t start)
{
	unsigned prev=buttons();while(sceKernelGetProcessTimeWide()-start<DURATION){
		unsigned b=buttons(),pressed=b&~prev;prev=b;
		if(pressed&SCE_CTRL_CIRCLE)return false;
		if(pressed&SCE_CTRL_CROSS)return true;
		sceKernelDelayThread(20000);
	}return false;
}
PlusVitaAppResult plus_vita_sony_login(PlusVitaPairFrame frame,void *ctx,bool sign_in_first)
{
	if(!frame)return PLUS_VITA_RESULT_UNSUPPORTED;
	if(sign_in_first){
		if(!sony_page(frame,ctx,"1 / 2  Official Sony sign-in","Scan with your phone. Sign in ONLY at playstation.com using your email and password or passkey. Complete Sony verification. X Next / O Cancel","https://www.playstation.com/"))return PLUS_VITA_RESULT_UNSUPPORTED;
		if(!next_page(sceKernelGetProcessTimeWide()))return PLUS_VITA_RESULT_CANCELLED;
	}
	if(!sony_page(frame,ctx,sign_in_first?"2 / 2  Sony token":"Open the official Sony token page","Use the same signed-in phone browser. If needed, sign in at playstation.com first. Read only the 64-character npsso value. X Enter token privately / O Cancel","https://ca.account.sony.com/api/v1/ssocookie"))return PLUS_VITA_RESULT_UNSUPPORTED;
	if(!next_page(sceKernelGetProcessTimeWide()))return PLUS_VITA_RESULT_CANCELLED;
	/* Erase both display buffers before invoking the masked IME. */
	frame(ctx,"Enter the Sony token","Never enter your Sony password here.",NULL,0,NULL);
	frame(ctx,"Enter the Sony token","Never enter your Sony password here.",NULL,0,NULL);
	return PLUS_VITA_RESULT_OK;
}
PlusVitaAppResult plus_vita_qr_pair(PlusVitaPairFrame frame,void *ctx,char *json,size_t cap)
{
	PlusVitaAppResult result=PLUS_VITA_RESULT_UNSUPPORTED;
	uint8_t key[32]={0},entropy[32]={0};SceNetEtherAddr device={0};char code[72]={0},msg[500]={0};uint32_t *pixels=NULL;
	SceUID block=-1;struct quirc *decoder=NULL;bool opened=false,started=false;
	struct quirc_code *q=NULL;struct quirc_data *d=NULL;
	if(!frame||!json||cap<77)return result;
	plus_vita_pair_clear(json,cap);
	uint64_t start=sceKernelGetProcessTimeWide();
	if(!frame(ctx,"1 / 3  Sign in to Sony","On your phone, sign in at playstation.com. Open the official Sony response link in the downloaded offline helper. Copy the response privately. X Next / O Cancel",NULL,0,NULL))goto done;
	if(!next_page(start)){result=PLUS_VITA_RESULT_CANCELLED;goto done;}
	/* No shared/static QR, clock-derived key or fallback to predictable entropy. */
	if(sceKernelGetRandomNumber(entropy,sizeof(entropy))<0||sceNetGetMacAddress(&device,0)<0||sizeof(device)!=6||!plus_vita_pair_make_key(entropy,(const uint8_t *)&device,key))goto done;
	plus_vita_pair_clear(entropy,sizeof(entropy));plus_vita_pair_clear(&device,sizeof(device));
	memcpy(code,"PVKEY3:",7);for(unsigned i=0;i<sizeof(key);i++)snprintf(code+7+i*2,3,"%02x",(unsigned)key[i]);
	snprintf(msg,sizeof(msg),"Scan/copy this one-time key into the offline helper, or type both lines together.\n%.32s\n%s\nPaste Sony's response and generate an encrypted QR. Keep both screens private.\nX  Scan phone QR    O  Cancel",code+7,code+39);
	start=sceKernelGetProcessTimeWide();
	if(!qr_screen(frame,ctx,"2 / 3  Pair privately",msg,code))goto done;
	if(!next_page(start)){result=PLUS_VITA_RESULT_CANCELLED;goto done;}
	block=sceKernelAllocMemBlock("PVQrCamera",SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,ALLOC,NULL);
	if(block<0||sceKernelGetMemBlockBase(block,(void **)&pixels)<0)goto done;
	decoder=quirc_new();q=calloc(1,sizeof(*q));d=calloc(1,sizeof(*d));
	if(!decoder||!q||!d||quirc_resize(decoder,W,H)<0)goto done;
	SceCameraInfo info={.size=sizeof(info),.format=SCE_CAMERA_FORMAT_ABGR,.resolution=SCE_CAMERA_RESOLUTION_640_360,.framerate=15,.sizeIBase=BYTES,.pIBase=pixels};
	if(sceCameraOpen(SCE_CAMERA_DEVICE_BACK,&info)<0)goto done;
	opened=true;
	if(sceCameraStart(SCE_CAMERA_DEVICE_BACK)<0)goto done;
	started=true;result=PLUS_VITA_RESULT_CANCELLED;
	if(!frame(ctx,"3 / 3  Scan encrypted QR","Aim the rear camera at your phone. O  Cancel.",NULL,0,NULL))goto done;
	while(sceKernelGetProcessTimeWide()-start<DURATION&&!(buttons()&SCE_CTRL_CIRCLE)){
		SceCameraRead read={.size=sizeof(read),.mode=0};
		if(sceCameraRead(SCE_CAMERA_DEVICE_BACK,&read)>=0){
			uint8_t *image=quirc_begin(decoder,NULL,NULL);
			for(unsigned i=0;i<W*H;i++){uint32_t v=pixels[i];image[i]=(uint8_t)(((v&255)+((v>>8)&255)+((v>>16)&255))/3);}
			quirc_end(decoder);
			for(int i=0;i<quirc_count(decoder);i++){
				quirc_extract(decoder,i,q);
				if(!quirc_decode(q,d)&&sceKernelGetProcessTimeWide()-start<DURATION&&d->payload_len>0&&plus_vita_pair_decode(key,d->payload,(size_t)d->payload_len,json,cap)){result=PLUS_VITA_RESULT_OK;goto done;}
				plus_vita_pair_clear(d,sizeof(*d));plus_vita_pair_clear(q,sizeof(*q));
			}
			if(!frame(ctx,"3 / 3  Scan encrypted QR","Aim the rear camera at the phone. O  Cancel. The key expires after three minutes.",NULL,0,pixels))break;
		}
		sceKernelDelayThread(50000);
	}
done:
	if(started)sceCameraStop(SCE_CAMERA_DEVICE_BACK);
	if(opened)sceCameraClose(SCE_CAMERA_DEVICE_BACK);
	if(pixels)plus_vita_pair_clear(pixels,BYTES);
	if(block>=0)sceKernelFreeMemBlock(block);
	if(decoder){int w=0,h=0;uint8_t *image=quirc_begin(decoder,&w,&h);if(image&&w>0&&h>0)plus_vita_pair_clear(image,(size_t)w*h);quirc_destroy(decoder);}
	if(q){plus_vita_pair_clear(q,sizeof(*q));free(q);}
	if(d){plus_vita_pair_clear(d,sizeof(*d));free(d);}
	plus_vita_pair_clear(entropy,sizeof(entropy));plus_vita_pair_clear(&device,sizeof(device));plus_vita_pair_clear(key,sizeof(key));plus_vita_pair_clear(code,sizeof(code));plus_vita_pair_clear(msg,sizeof(msg));
	if(result!=PLUS_VITA_RESULT_OK)plus_vita_pair_clear(json,cap);
	frame(ctx,"QR pairing finished","Returning to account setup...",NULL,0,NULL);
	frame(ctx,"QR pairing finished","Returning to account setup...",NULL,0,NULL);
	return result;
}
