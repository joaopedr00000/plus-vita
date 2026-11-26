// SPDX-License-Identifier: AGPL-3.0-only
#include "render.h"
#include <string.h>
#include <stdio.h>
#include "../platform/build_identity.h"
#include "../platform/adaptive_stream.h"
#include "theme.h"
#include "theme_presets.h"
#include "theme_motifs.h"
#if PLUS_VITA_ADAPTIVE_STREAM
#include "i18n.h"
#include "catalog_filter.h"
#endif
#define WIDTH 960
#define HEIGHT 544
#define RGB(r,g,b) (0xff000000u | ((uint32_t)(b)<<16) | ((uint32_t)(g)<<8) | (r))
#if PLUS_VITA_ADAPTIVE_STREAM
#define MENU_ACCENT VP_GOLD
#define MENU_BUTTON VP_BUTTON
#define MENU_PRESSED VP_GOLD_PRESSED
#define MENU_PANEL VP_PANEL
#define MENU_WHITE VP_WHITE
#define MENU_BODY VP_BODY
#define MENU_MUTED VP_MUTED
#define MENU_INK VP_DARK_INK
#else
#define MENU_ACCENT RGB(246,198,91)
#define MENU_BUTTON RGB(17,57,125)
#define MENU_PRESSED RGB(77,180,244)
#define MENU_PANEL RGB(9,29,66)
#define MENU_WHITE RGB(243,247,252)
#define MENU_BODY RGB(210,224,247)
#define MENU_MUTED RGB(195,211,239)
#define MENU_INK RGB(9,29,66)
#endif
/* Native pixel sizes from the hardware-selected A setup and tile views.
 * All screens share this painter and the system PVF path. Keep compact
 * Account/tile text at 18px and the accepted heading at 36px. */
typedef enum {
	TEXT_MICRO=14, TEXT_HINT=16, TEXT_COMPACT=18, TEXT_BODY=20,
	TEXT_CONTROL=22, TEXT_BRAND=24, TEXT_TITLE=26, TEXT_HEADING=36
} TextRole;
typedef struct { uint32_t *pixels; int pitch; PlusVitaFont *font; unsigned language; bool custom; uint32_t primary,secondary; } Canvas;
static const char *localized(Canvas *d,const char *s){
#if PLUS_VITA_ADAPTIVE_STREAM
	return pv_translate(d->language,s);
#else
	(void)d;return s;
#endif
}
static void rectangle(Canvas *display, int x, int y, int width, int height, uint32_t color)
{
	int right = x + width, bottom = y + height;
	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(right > WIDTH) right = WIDTH;
	if(bottom > HEIGHT) bottom = HEIGHT;
	for(int row = y; row < bottom; row++)
		for(int col = x; col < right; col++) display->pixels[row * display->pitch + col] = color;
}

static void rounded(Canvas *display, int x, int y, int width, int height, uint32_t color)
{
	for(int row = 0; row < height; row++)
	{
		int inset = row == 0 || row == height - 1 ? 5 : row < 3 || row >= height - 3 ? 2 : 0;
		rectangle(display, x + inset, y + row, width - 2 * inset, 1, color);
	}
}

static void text(Canvas *display, const char *value, int x, int y, TextRole role, int width, int height, bool wrap, uint32_t color)
{
	plus_vita_font_draw(display->font, display->pixels, display->pitch, value,
		(PlusVitaTextBox){x, y, width, height}, role, wrap, color);
}
static void cover(Canvas *display,const PlusVitaCovers *covers,const PlusVitaCatalogEntry *game,int x,int y)
{
	const PlusVitaCoverImage *image=game ? plus_vita_covers_find(covers,game->cover_url) : NULL;
	rectangle(display,x,y,128,128,MENU_PANEL);
	if(image && image->state==PLUS_VITA_COVER_READY) {
		for(int row=0;row<128;row++) for(int col=0;col<128;col++) display->pixels[(y+row)*display->pitch+x+col]=image->pixels[row*128+col];
	} else text(display,localized(display,!game || (!game->cover_url[0] && game->cover_metadata==PLUS_VITA_ARTWORK_MISSING) ? "No cover" : !game->cover_url[0] || !covers ? "Cover unavailable" : !image || image->state==PLUS_VITA_COVER_LOADING ? "Loading cover" : "Cover unavailable"),x+6,y+48,TEXT_MICRO,116,58,true,MENU_MUTED);
}

#if PLUS_VITA_ADAPTIVE_STREAM
/* Native 960x544 version of the approved reference. The actual Sony
 * artwork is always aspect-fitted in the main well; thumbnails crop to fill. */
static int corner_inset(int row,int height,int radius)
{
	int dy=row<radius?radius-1-row:row>=height-radius?row-(height-radius):0;
	if(!dy)return 0;
	int dx=radius;
	while(dx>0 && dx*dx+dy*dy>radius*radius)dx--;
	return radius-dx;
}
static uint32_t mix(uint32_t a,uint32_t b,unsigned t)
{
	uint32_t out=0xff000000u;
	for(unsigned shift=0;shift<24;shift+=8)
		out|=((((a>>shift)&255)*(255-t)+((b>>shift)&255)*t+127)/255)<<shift;
	return out;
}
static void round_gradient(Canvas *d,int x,int y,int w,int h,int radius,uint32_t top,uint32_t bottom)
{
	if(w<1||h<1)return;
	if(radius>w/2)radius=w/2;
	if(radius>h/2)radius=h/2;
	for(int row=0;row<h;row++){
		int in=corner_inset(row,h,radius);
		rectangle(d,x+in,y+row,w-2*in,1,mix(top,bottom,(unsigned)(row*255/(h>1?h-1:1))));
	}
}
static void outline(Canvas *d,int x,int y,int w,int h,int rad,uint32_t border,uint32_t top,uint32_t bottom)
{
	round_gradient(d,x,y,w,h,rad,border,border);
	round_gradient(d,x+1,y+1,w-2,h-2,rad>1?rad-1:0,top,bottom);
}
static void line(Canvas *d,int x,int y,int x1,int y1,int thick,uint32_t color)
{
	int dx=x1-x,dy=y1-y,steps=dx<0?-dx:dx,ay=dy<0?-dy:dy;
	if(ay>steps)steps=ay;
	if(!steps){rectangle(d,x,y,thick,thick,color);return;}
	for(int j=0;j<=steps;j++)rectangle(d,x+j*dx/steps,y+j*dy/steps,thick,thick,color);
}
static void ring(Canvas *d,int cx,int cy,int rad,int thick,uint32_t color)
{
	for(int y=-rad;y<=rad;y++)for(int x=-rad;x<=rad;x++){
		int q=x*x+y*y;
		if(q<=rad*rad&&q>=(rad-thick)*(rad-thick))rectangle(d,cx+x,cy+y,1,1,color);
	}
}
static void symbol(Canvas *d,int cx,int cy,unsigned type,uint32_t color)
{
	if(type==0){rectangle(d,cx-5,cy-5,11,2,color);rectangle(d,cx-5,cy+4,11,2,color);rectangle(d,cx-5,cy-5,2,11,color);rectangle(d,cx+4,cy-5,2,11,color);}
	else if(type==1){line(d,cx,cy-6,cx-6,cy+5,2,color);line(d,cx,cy-6,cx+6,cy+5,2,color);line(d,cx-6,cy+5,cx+6,cy+5,2,color);}
	else if(type==2){line(d,cx-5,cy-5,cx+5,cy+5,2,color);line(d,cx+5,cy-5,cx-5,cy+5,2,color);}
	else ring(d,cx,cy,7,2,color);
}
static void library_art(Canvas *d,const PlusVitaCoverImage *image,int x,int y,int w,int h,bool thumbnail)
{
	round_gradient(d,x,y,w,h,9,RGB(35,35,38),RGB(20,20,22));
	if(!image||image->state!=PLUS_VITA_COVER_READY)return;
	int left=0,right=127,top=0,bottom=127;
	const uint32_t pad=0xff1f1d1du;
	while(left<right){bool blank=true;for(int k=0;k<128;k++)if(image->pixels[k*128+left]!=pad){blank=false;break;}if(!blank)break;left++;}
	while(right>left){bool blank=true;for(int k=0;k<128;k++)if(image->pixels[k*128+right]!=pad){blank=false;break;}if(!blank)break;right--;}
	while(top<bottom){bool blank=true;for(int k=0;k<128;k++)if(image->pixels[top*128+k]!=pad){blank=false;break;}if(!blank)break;top++;}
	while(bottom>top){bool blank=true;for(int k=0;k<128;k++)if(image->pixels[bottom*128+k]!=pad){blank=false;break;}if(!blank)break;bottom--;}
	int sw=right-left+1,sh=bottom-top+1,dw=w,dh=h;
	if(thumbnail){
		if(sw*h>sh*w){int crop=sh*w/h;if(crop<1)crop=1;left+=(sw-crop)/2;sw=crop;}
		else {int crop=sw*h/w;if(crop<1)crop=1;top+=(sh-crop)/2;sh=crop;}
	}else if(sw*h>sh*w)dh=w*sh/sw;else dw=h*sw/sh;
	if(dw<1)dw=1;
	if(dh<1)dh=1;
	int ox=(w-dw)/2,oy=(h-dh)/2;
	for(int row=0;row<dh;row++){
		int in=corner_inset(row+oy,h,9);
		for(int col=0;col<dw;col++)if(col+ox>=in&&col+ox<w-in)
			d->pixels[(y+oy+row)*d->pitch+x+ox+col]=image->pixels[(top+row*sh/dh)*128+left+col*sw/dw];
	}
}
static void library_logo(Canvas *d,const uint32_t *logo)
{
	if(!logo)return;
	/* Crop only the surrounding background from the exact README logo.
	 * The bright silhouette bounds retain the dark Vita inside the cloud. */
	for(int y=0;y<47;y++){
		int sy=23+y*73/47,l=128,rr=-1;
		for(int sx=12;sx<112;sx++){
			uint32_t px=logo[sy*128+sx];unsigned red=px&255,green=(px>>8)&255,blue=(px>>16)&255;
			if(red>115&&(green>90||blue>110)){if(sx<l)l=sx;rr=sx;}
		}
		for(int x=0;x<67;x++){
			int sx=12+x*100/67;
			if(sx>=l&&sx<=rr)d->pixels[(7+y)*d->pitch+24+x]=logo[sy*128+sx];
		}
	}
}
static uint32_t primary(Canvas *d,uint32_t fallback){return d->custom?d->primary:fallback;}
static uint32_t secondary(Canvas *d,uint32_t fallback){return d->custom?d->secondary:fallback;}
static uint32_t tint(Canvas *d,uint32_t fallback,unsigned amount){return d->custom?mix(RGB(18,18,20),d->secondary,amount):fallback;}
static uint32_t readable(Canvas *d,uint32_t fallback){return d->custom?mix(d->primary,RGB(255,255,255),125):fallback;}
static void theme_motif(Canvas *d,unsigned id,int x,int y,int size,uint32_t color,int thick)
{
    if(!id||id>PV_THEME_COUNT)return;
    const PvThemeMotif *m=&pv_theme_motifs[id];
    for(unsigned k=0;k<m->count;k++){
        const PvThemeLine *l=&m->lines[k];
        line(d,x+l->x0*size/16,y+l->y0*size/16,x+l->x1*size/16,y+l->y1*size/16,thick,color);
    }
}
static void disk(Canvas *d,int cx,int cy,int rad,uint32_t color)
{
    for(int y=-rad;y<=rad;y++)for(int x=-rad;x<=rad;x++)if(x*x+y*y<=rad*rad)rectangle(d,cx+x,cy+y,1,1,color);
}
static void theme_icon(Canvas *d,const PlusVitaAppView *v,unsigned id,int x,int y,int size)
{
    int art=pv_theme_art_index(id);
    if(art>=0&&v->theme_wordmark){
        for(int row=0;row<size;row++)for(int col=0;col<size;col++){
            unsigned sx=(unsigned)(art%4)*24+(unsigned)(col*24/size),sy=(unsigned)(art/4)*24+(unsigned)(row*24/size);
            rectangle(d,x+col,y+row,1,1,v->theme_wordmark[sy*96+sx]);
        }
        return;
    }
    uint32_t c=pv_theme_pixel(pv_presets[pv_theme_id(id)].primary),s=pv_theme_pixel(pv_presets[pv_theme_id(id)].secondary);
    int cx=x+size/2,cy=y+size/2,k=size/4;
    round_gradient(d,x,y,size,size,7,mix(RGB(15,15,18),c,74),mix(RGB(9,9,11),s,45));
    if(id==0){
        if(v->library_logo){for(int row=0;row<size;row++)for(int col=0;col<size;col++)rectangle(d,x+col,y+row,1,1,v->library_logo[(row*128/size)*128+col*128/size]);}
        else {disk(d,cx,cy-3,k,MENU_WHITE);rounded(d,x+size/5,cy,size*3/5,size/5,MENU_WHITE);}
    }else if(id==1){
        /* Compact cloud-shaped Vice City easter egg, never a banner. */
        disk(d,cx,cy-k/2,k,mix(c,s,120));disk(d,cx-k,cy,k/2,c);disk(d,cx+k,cy,k/2,s);
        rounded(d,cx-k*2,cy,size-2,size/5,mix(c,s,120));
        rounded(d,cx-k,cy-k/2,k*2,k,RGB(15,15,20));
        line(d,cx-k/2,cy-k/2,cx,cy+k/3,2,RGB(255,175,214));line(d,cx,cy+k/3,cx+k/2,cy-k/2,2,RGB(255,175,214));
        rectangle(d,cx+k-3,cy+k/2,6,8,MENU_ACCENT);
    }else if(id==7){
        disk(d,cx,cy,k+3,MENU_WHITE);for(int yy=-k-3;yy<0;yy++)for(int xx=-k-3;xx<=k+3;xx++)if(xx*xx+yy*yy<=(k+3)*(k+3))rectangle(d,cx+xx,cy+yy,1,1,RGB(222,40,39));
        rectangle(d,cx-k-3,cy-1,2*k+7,3,RGB(20,20,23));disk(d,cx,cy,5,RGB(20,20,23));disk(d,cx,cy,3,MENU_WHITE);
    }else if(id==17){
        disk(d,cx,cy,k+3,RGB(255,150,20));for(int j=0;j<4;j++){int xx=cx+(j%2?4:-4),yy=cy+(j/2?4:-4);rectangle(d,xx-2,yy-1,5,3,RGB(187,28,22));rectangle(d,xx-1,yy-2,3,5,RGB(187,28,22));}
    }else if(id==16){ring(d,cx,cy,k+4,5,RGB(255,216,53));ring(d,cx,cy,k+1,1,RGB(255,249,173));}
    else if(id==9){rectangle(d,cx-k,cy-k,2*k,2*k,RGB(126,83,51));rectangle(d,cx-k,cy-k,2*k,k/2,RGB(83,176,51));for(int j=0;j<5;j++)rectangle(d,cx-k+j*(k*2/5),cy-k/2+(j%2)*4,3,4,RGB(76,124,39));}
    else if(id==6){
        disk(d,cx,cy-3,k+4,RGB(231,45,40));rounded(d,cx-k,cy+1,k*2,k+5,RGB(244,217,170));
        disk(d,cx-k/2,cy-k/2,4,MENU_WHITE);disk(d,cx+k/2,cy-k/2,4,MENU_WHITE);disk(d,cx,cy-k+1,3,MENU_WHITE);
        rectangle(d,cx-4,cy+3,2,5,RGB(28,24,24));rectangle(d,cx+3,cy+3,2,5,RGB(28,24,24));
    }else if(id==19){
        disk(d,cx,cy,k+4,RGB(226,170,71));rounded(d,cx-k-5,cy+1,(k+5)*2,7,RGB(232,184,84));rectangle(d,cx-k,cy-1,2*k,4,RGB(187,32,40));
    }else if(id==25){
        for(int j=0;j<4;j++)disk(d,cx+(j%2?5:-5),cy+(j/2?5:-5),7,RGB(120,205,90));line(d,cx,cy,cx-3,cy+k+7,3,MENU_WHITE);
    }else if(id==39){
        ring(d,cx,cy,k+3,4,MENU_WHITE);disk(d,cx,cy-k-3,4,RGB(230,83,32));disk(d,cx-k-2,cy+k/2,4,RGB(230,83,32));disk(d,cx+k+2,cy+k/2,4,RGB(230,83,32));
    }else if(id==34||id==40){
        outline(d,x+5,cy-k+1,size-10,2*k,6,MENU_WHITE,RGB(19,19,22),RGB(19,19,22));rectangle(d,cx-k+3,cy-k+4,2*k-6,2*k-7,c);disk(d,x+9,cy,2,MENU_WHITE);disk(d,x+size-9,cy,2,MENU_WHITE);
    }else if(id==20){for(int yy=0;yy<4;yy++)for(int xx=0;xx<4;xx++)rectangle(d,x+size/6+xx*(size/6),y+size/6+yy*(size/6),size/6,size/6,((xx+yy)%2)?RGB(15,18,18):RGB(32,170,117));}
    else if(id==28){ring(d,cx,cy,k+4,5,RGB(78,230,66));ring(d,cx,cy,k-2,3,RGB(172,255,70));ring(d,cx,cy,k/2,2,RGB(95,219,232));}
    else if(id>=46){ring(d,x+size/4,cy+k/2,k/2+2,3,MENU_WHITE);ring(d,x+size*3/4,cy+k/2,k/2+2,3,MENU_WHITE);line(d,x+size/4,cy+k/2,cx,cy-k/2,3,c);line(d,cx,cy-k/2,x+size*3/4,cy+k/2,3,c);line(d,cx,cy-k/2,cx+k,cy-k,3,MENU_WHITE);}
    else theme_motif(d,id,x+size/8,y+size/8,size*3/4,mix(c,MENU_WHITE,150),size>40?3:2);
}
static void local_wallpaper(Canvas *d,const PlusVitaAppView *v)
{
    if(!v->theme_wallpaper)return;
    for(int y=0;y<HEIGHT;y++)for(int x=0;x<WIDTH;x++)
        d->pixels[y*d->pitch+x]=mix(RGB(8,8,10),v->theme_wallpaper[(y/2)*480+x/2],70);
}
static void theme_header(Canvas *d,const PlusVitaAppView *v)
{
    if(v->theme_id==1||v->theme_id==2||v->theme_id==12||v->theme_id==42)theme_icon(d,v,v->theme_id,29,6,45);
    else library_logo(d,v->library_logo);
    text(d,"VitaPlus",99,11,TEXT_TITLE,127,39,false,MENU_WHITE);
}
static void library_background(Canvas *d)
{
	for(int y=0;y<HEIGHT;y++)rectangle(d,0,y,WIDTH,1,RGB(12+y*6/544,12+y*6/544,14+y*6/544));
	/* Two layered gold ribbons: precomputed integer curves, no texture I/O. */
	for(int x=397;x<960;x++){
		int delta=x-755;
		int upper=36+(delta*delta)/2600+(960-x)/14;
		int lower=485-((x-550)*(x-550))/3000+(x-400)/45;
		for(int b=0;b<14;b++){
			int shade=14-b;
			rectangle(d,x,upper+b*3,1,3,tint(d,RGB(18+shade*3,17+shade*2,19+shade),(unsigned)shade*8));
			rectangle(d,x,lower-b*3,1,3,tint(d,RGB(18+shade*3,17+shade*2,19+shade),(unsigned)shade*8));
		}
		rectangle(d,x,upper,1,1,secondary(d,RGB(186,148,66)));
		rectangle(d,x,lower,1,1,secondary(d,RGB(163,127,53)));
	}
	rectangle(d,0,497,960,47,RGB(10,10,12));
}
static void library_status(Canvas *d,const PlusVitaAppView *v)
{
	uint32_t ink=v->network_connected?MENU_WHITE:MENU_MUTED;
	if(v->network_known){
		for(int y=-13;y<=-2;y++)for(int x=-13;x<=13;x++){
			int q=x*x+y*y;
			if((q>=121&&q<=169)||(q>=49&&q<=81))rectangle(d,787+x,22+y,1,1,ink);
		}
		rectangle(d,785,21,4,3,ink);
		if(!v->network_connected)line(d,777,8,797,24,2,RGB(190,100,85));
	}
	char buf[24];
	if(v->clock_valid){snprintf(buf,sizeof(buf),"%02u:%02u",v->clock_hour,v->clock_minute);text(d,buf,810,7,TEXT_HINT,60,26,false,MENU_WHITE);}
	if(v->battery_valid){
		snprintf(buf,sizeof(buf),"%u%%",v->battery_percent);text(d,buf,879,7,TEXT_HINT,44,26,false,MENU_WHITE);
		outline(d,925,9,26,14,3,MENU_WHITE,RGB(20,20,22),RGB(20,20,22));rectangle(d,951,13,3,6,MENU_WHITE);
		rectangle(d,927,11,(int)(22u*(v->battery_percent>100?100:v->battery_percent)/100u),10,readable(d,MENU_ACCENT));
	}
}
static void library_button(Canvas *d,const PlusVitaAppView *v,const PlusVitaCovers *covers,size_t index)
{
	const PlusVitaAppButton *b=&v->actions[index];
	bool focus=index==v->selected,pressed=v->touch_pressed&&index==v->pressed_index;
	if(b->id==PLUS_VITA_ACTION_GAME&&index<v->list_count){
		bool chosen=v->focus==v->list_first+index;const PlusVitaCatalogEntry *game=v->games[index];
		if(chosen||focus||pressed)outline(d,b->x,b->y,b->width,b->height,9,primary(d,RGB(249,213,122)),tint(d,RGB(96,79,43),84),pressed?tint(d,RGB(95,70,28),108):tint(d,RGB(43,34,20),34));
		else rectangle(d,b->x+8,b->y+b->height-1,b->width-16,1,RGB(51,51,53));
		const PlusVitaCoverImage *art=game?plus_vita_covers_find(covers,game->cover_url):NULL;
		library_art(d,art,b->x+7,b->y+4,67,42,true);
		text(d,b->label,b->x+87,b->y+6,TEXT_HINT,222,38,true,MENU_WHITE);
		if(game&&game->platform[0]){
			outline(d,b->x+315,b->y+13,43,23,7,chosen?readable(d,MENU_ACCENT):RGB(111,111,115),RGB(26,26,28),RGB(18,18,20));
			text(d,game->platform,b->x+323,b->y+14,TEXT_MICRO,32,22,false,MENU_WHITE);
		}
		return;
	}
	if(b->id==PLUS_VITA_ACTION_BACK)return; /* painted with footer icon; hitbox remains live */
	bool filter=b->id>=PLUS_VITA_ACTION_FILTER_ALL&&b->id<=PLUS_VITA_ACTION_FILTER_PS5;
	bool active=filter&&(unsigned)(b->id-PLUS_VITA_ACTION_FILTER_ALL)==v->platform_filter;
	bool play=b->id==PLUS_VITA_ACTION_PLAY;
	bool filled=active||play||pressed;
	uint32_t edge=(filled||focus)?primary(d,RGB(255,216,125)):RGB(119,119,123);
	int rad=filter?14:9;
	outline(d,b->x,b->y,b->width,b->height,rad,edge,filled?primary(d,RGB(255,216,126)):RGB(36,36,38),filled?secondary(d,RGB(185,137,44)):RGB(21,21,23));
	const char *label=b->id==PLUS_VITA_ACTION_CLEAR_SEARCH?"X":b->label;
	if(b->id==PLUS_VITA_ACTION_SEARCH){
		ring(d,b->x+19,b->y+13,7,2,MENU_MUTED);line(d,b->x+24,b->y+18,b->x+30,b->y+24,2,MENU_MUTED);
		text(d,label,b->x+45,b->y+4,TEXT_HINT,b->width-50,b->height-7,false,MENU_BODY);
	}else if(play){
		int cx=b->x+100;
		for(int j=-9;j<=9;j++)rectangle(d,cx-7,b->y+23+j,15-((j<0?-j:j)*15/10),1,(d->custom?pv_theme_ink(mix(d->primary,d->secondary,128)):MENU_INK));
		/* Long experimental label needs the complete button width. */
		bool ps5=v->detail&&pv_catalog_platform(v->detail->platform)==3;
		if(ps5){
			round_gradient(d,b->x+2,b->y+2,b->width-4,b->height-4,7,primary(d,RGB(255,216,126)),secondary(d,RGB(185,137,44)));
			text(d,label,b->x+15,b->y+10,TEXT_CONTROL,b->width-30,34,false,(d->custom?pv_theme_ink(mix(d->primary,d->secondary,128)):MENU_INK));
		}else text(d,label,cx+15,b->y+8,TEXT_TITLE,b->width-115,38,false,(d->custom?pv_theme_ink(mix(d->primary,d->secondary,128)):MENU_INK));
	}else {
		int inset=filter?15:10;
		text(d,label,b->x+inset,b->y+3,filter?TEXT_HINT:TEXT_MICRO,b->width-inset-5,b->height-4,false,filled?(d->custom?pv_theme_ink(mix(d->primary,d->secondary,128)):MENU_INK):MENU_WHITE);
	}
}
static void footer(Canvas *d,const PlusVitaAppView *v)
{
	ring(d,25,520,10,1,MENU_MUTED);text(d,"i",22,511,TEXT_HINT,12,22,false,MENU_WHITE);
	text(d,v->body,50,511,TEXT_MICRO,488,28,false,MENU_MUTED);
	rectangle(d,563,507,1,26,RGB(135,135,138));
	static const char *const labels[]={"Search","Filter","Play","Back"};
	static const int starts[]={600,703,800,878};
	for(unsigned j=0;j<4;j++){
		int x=starts[j];ring(d,x,520,12,1,RGB(105,105,109));symbol(d,x,520,j,j==2?MENU_WHITE:readable(d,MENU_ACCENT));
		text(d,localized(d,labels[j]),x+17,509,TEXT_HINT,(j==3?960:starts[j+1])-x-18,28,false,MENU_WHITE);
	}
}
static void library_paint(Canvas *d,const PlusVitaAppView *v,const PlusVitaCovers *covers)
{
	library_background(d);local_wallpaper(d,v);theme_header(d,v);
	rectangle(d,228,12,1,30,readable(d,MENU_ACCENT));
	text(d,localized(d,"Game Library"),248,5,TEXT_CONTROL,490,30,false,MENU_WHITE);
	text(d,v->subtitle,248,34,TEXT_MICRO,500,23,false,MENU_MUTED);
	library_status(d,v);text(d,PLUS_VITA_BUILD_LABEL,865,56,TEXT_MICRO,89,25,false,MENU_MUTED);
	rectangle(d,325,65,1,24,RGB(150,150,153));
	rectangle(d,0,104,960,1,RGB(75,75,78));rectangle(d,395,105,1,392,RGB(75,75,78));rectangle(d,0,497,960,1,RGB(75,75,78));
	if(v->list_total>v->list_count&&v->list_count){
		int thumb=(int)((size_t)v->list_height*v->list_count/v->list_total);if(thumb<8)thumb=8;
		int offset=(int)((size_t)(v->list_height-thumb)*v->list_first/(v->list_total-v->list_count));
		round_gradient(d,386,v->list_y+offset,6,thumb,3,RGB(118,118,120),RGB(95,95,98));
	}
	bool ps5=v->detail?pv_catalog_platform(v->detail->platform)==3:v->platform_filter==3;
	if(v->detail){
		const PlusVitaCatalogEntry *g=v->detail;
		const PlusVitaCoverImage *art=v->preview_eligible?plus_vita_covers_find(covers,g->cover_url):NULL;
		library_art(d,art,418,122,234,318,false);
		if(!art||art->state!=PLUS_VITA_COVER_READY){
			const char *status=!g->cover_url[0]?"No cover":!covers||(art&&art->state==PLUS_VITA_COVER_FAILED)?"Cover unavailable":"Loading cover";
			text(d,localized(d,status),432,263,TEXT_HINT,208,60,true,MENU_MUTED);
		}
		text(d,g->name,670,117,TEXT_TITLE,270,57,true,MENU_WHITE);
		outline(d,672,177,57,25,7,readable(d,MENU_ACCENT),RGB(24,24,26),RGB(15,15,17));
		text(d,g->platform[0]?g->platform:localized(d,"Not specified"),681,179,TEXT_HINT,46,23,false,readable(d,MENU_ACCENT));
		if(ps5)text(d,localized(d,"PS5 streaming is not available yet. Support is planned."),672,207,TEXT_MICRO,264,56,true,readable(d,MENU_ACCENT));
		else text(d,localized(d,"Cloud streaming"),672,210,TEXT_BODY,264,46,true,MENU_BODY);
		rectangle(d,672,270,264,1,RGB(108,108,110));
		text(d,localized(d,"Status:"),678,279,TEXT_HINT,105,23,false,MENU_MUTED);
		text(d,localized(d,ps5?"Unavailable":g->launch_id_valid?"Check eligibility":"Unavailable"),789,279,TEXT_HINT,147,23,false,readable(d,MENU_ACCENT));
		text(d,localized(d,"Platform:"),678,303,TEXT_HINT,105,23,false,MENU_MUTED);
		text(d,g->platform[0]?g->platform:localized(d,"Not specified"),789,303,TEXT_HINT,147,23,false,MENU_WHITE);
		text(d,localized(d,"Game ID:"),678,326,TEXT_HINT,105,23,false,MENU_MUTED);
		text(d,g->launch_id_valid?g->id:localized(d,"Unavailable"),789,326,TEXT_MICRO,147,23,false,MENU_WHITE);
		text(d,localized(d,"Playtime: unavailable | Trophies: unavailable"),678,350,TEXT_MICRO,258,34,true,MENU_MUTED);
	}else{
		text(d,localized(d,v->title?v->title:"No cloud games found."),24,145,TEXT_BODY,350,100,true,MENU_BODY);
		if(ps5)text(d,localized(d,"PS5 streaming is not available yet. Support is planned."),418,150,TEXT_BODY,510,100,true,readable(d,MENU_ACCENT));
	}
	for(size_t j=0;j<v->action_count;j++)library_button(d,v,covers,j);
	footer(d,v);
}
/* All non-library screens use the same header, dark cards, ribbons and footer.
 * Hitboxes come from the application model, never from a static screenshot. */
static void menu_icon(Canvas *d,PlusVitaAppAction id,int x,int y,uint32_t c)
{
    if(id==PLUS_VITA_ACTION_THEME||id==PLUS_VITA_ACTION_THEME_PRESETS){disk(d,x,y,11,c);disk(d,x+4,y+4,4,RGB(23,23,25));for(int j=0;j<3;j++)disk(d,x-6+j*5,y-4+(j%2)*3,2,RGB(23,23,25));}
    else if(id==PLUS_VITA_ACTION_PRIMARY||id==PLUS_VITA_ACTION_SECONDARY){ring(d,x,y+2,9,2,c);line(d,x,y-13,x-7,y-4,2,c);line(d,x,y-13,x+7,y-4,2,c);}
    else if(id==PLUS_VITA_ACTION_WALLPAPERS){outline(d,x-12,y-9,25,19,3,c,RGB(25,25,27),RGB(25,25,27));disk(d,x+5,y-4,2,c);line(d,x-10,y+7,x-3,y-2,2,c);line(d,x-3,y-2,x+7,y+7,2,c);}
    else if(id==PLUS_VITA_ACTION_CONTROLS||id==PLUS_VITA_ACTION_CONTROL_LAYOUT){outline(d,x-13,y-8,27,17,7,c,RGB(25,25,27),RGB(25,25,27));line(d,x-7,y-4,x-7,y+4,2,c);line(d,x-11,y,x-3,y,2,c);disk(d,x+6,y-2,2,c);disk(d,x+9,y+3,2,c);}
    else if(id==PLUS_VITA_ACTION_STREAM_SETTINGS||id==PLUS_VITA_ACTION_REGION){ring(d,x,y,12,2,c);ring(d,x,y,7,2,c);disk(d,x,y,2,c);}
    else if(id==PLUS_VITA_ACTION_ACCOUNT||id==PLUS_VITA_ACTION_SETUP){ring(d,x,y-5,5,2,c);ring(d,x,y+8,10,2,c);}
    else if(id==PLUS_VITA_ACTION_SECURITY){outline(d,x-10,y-2,21,14,3,c,RGB(25,25,27),RGB(25,25,27));ring(d,x,y-5,7,2,c);}
    else if(id==PLUS_VITA_ACTION_REPORTING){line(d,x-13,y,x-7,y,2,c);line(d,x-7,y,x-3,y-10,2,c);line(d,x-3,y-10,x+3,y+10,2,c);line(d,x+3,y+10,x+7,y,2,c);line(d,x+7,y,x+13,y,2,c);}
    else if(id==PLUS_VITA_ACTION_BACK||id==PLUS_VITA_ACTION_THEME_PREV){line(d,x+7,y,x-7,y,2,c);line(d,x-7,y,x,y-7,2,c);line(d,x-7,y,x,y+7,2,c);}
    else if(id==PLUS_VITA_ACTION_THEME_NEXT){line(d,x-7,y,x+7,y,2,c);line(d,x+7,y,x,y-7,2,c);line(d,x+7,y,x,y+7,2,c);}
    else {ring(d,x,y,10,2,c);line(d,x,y-4,x,y+5,2,c);disk(d,x,y-7,1,c);}
}
static void menu_card(Canvas *d,const PlusVitaAppView *v,size_t index)
{
    const PlusVitaAppButton *b=&v->actions[index];
    bool focus=index==v->selected,pressed=v->touch_pressed&&index==v->pressed_index;
    bool highlighted=focus||pressed;
    uint32_t border=highlighted?readable(d,MENU_ACCENT):RGB(91,91,96);
    uint32_t top=highlighted?mix(RGB(20,20,23),primary(d,MENU_ACCENT),96):RGB(34,34,37);
    uint32_t bot=highlighted?mix(RGB(16,16,18),secondary(d,RGB(185,137,44)),65):RGB(17,17,19);
    outline(d,b->x,b->y,b->width,b->height,9,border,top,bot);
    if(highlighted&&b->height>=30)round_gradient(d,b->x+3,b->y+3,b->width-6,2,1,border,mix(border,top,90));
    if(b->id==PLUS_VITA_ACTION_THEME_PICK){
        unsigned id=0;for(unsigned j=0;j<=PV_THEME_COUNT;j++)if(!strcmp(b->label,pv_presets[j].name)){id=j;break;}
        theme_icon(d,v,id,b->x+(b->width-42)/2,b->y+4,42);
        text(d,b->label,b->x+7,b->y+49,TEXT_HINT,b->width-14,b->height-50,true,MENU_WHITE);
        if(id==v->theme_id){disk(d,b->x+b->width-14,b->y+13,5,readable(d,MENU_ACCENT));}
        return;
    }
    int icon=b->height>=32&&b->width>=145?1:0;
    if(icon)menu_icon(d,b->id,b->x+24,b->y+b->height/2,highlighted?readable(d,MENU_ACCENT):MENU_WHITE);
    int inset=icon?46:12;
    TextRole role=b->height<32?TEXT_MICRO:b->height<45?TEXT_HINT:TEXT_COMPACT;
    int y=b->y+(b->height>=60?17:b->height>=40?6:1);
    int reserve=0;
    if(b->id==PLUS_VITA_ACTION_PRIMARY||b->id==PLUS_VITA_ACTION_SECONDARY){
        reserve=64;uint32_t swatch=pv_theme_pixel(b->id==PLUS_VITA_ACTION_PRIMARY?v->theme_primary:v->theme_secondary);
        outline(d,b->x+b->width-58,b->y+5,44,b->height-10,5,MENU_WHITE,swatch,swatch);
    }
    text(d,b->label,b->x+inset,y,role,b->width-inset-12-reserve-(b->height>=45&&reserve==0?16:0),b->y+b->height-y-2,b->height>=46&&b->height<60,MENU_WHITE);
    if(b->height>=45&&b->width>=180&&reserve==0){line(d,b->x+b->width-20,b->y+b->height/2-5,b->x+b->width-15,b->y+b->height/2,2,MENU_MUTED);line(d,b->x+b->width-15,b->y+b->height/2,b->x+b->width-20,b->y+b->height/2+5,2,MENU_MUTED);}
}
static void menu_chrome(Canvas *d,const PlusVitaAppView *v)
{
    library_background(d);local_wallpaper(d,v);
    /* Darken the shared header/footer independently of any custom wallpaper. */
    round_gradient(d,0,0,960,64,0,RGB(13,13,15),RGB(23,23,25));
    theme_header(d,v);rectangle(d,228,12,1,30,readable(d,MENU_ACCENT));
    text(d,v->title?v->title:"VitaPlus",248,4,TEXT_COMPACT,500,31,false,MENU_WHITE);
    text(d,v->subtitle,248,34,TEXT_MICRO,500,24,false,MENU_MUTED);
    library_status(d,v);
    text(d,PLUS_VITA_BUILD_LABEL,828,34,TEXT_MICRO,126,25,false,MENU_MUTED);
    rectangle(d,0,64,960,1,RGB(105,105,109));
    rectangle(d,0,497,960,47,RGB(10,10,12));rectangle(d,0,497,960,1,RGB(105,105,109));
    ring(d,25,520,10,1,MENU_MUTED);text(d,"i",22,510,TEXT_HINT,12,25,false,MENU_WHITE);
    text(d,v->hint,46,508,TEXT_HINT,900,30,false,MENU_WHITE);
}
static void menu_paint(Canvas *d,const PlusVitaAppView *v)
{
    menu_chrome(d,v);
    if(v->screen==PLUS_VITA_APP_THEME_PRESETS){
        text(d,v->body,18,78,TEXT_BODY,704,40,false,MENU_WHITE);
        outline(d,732,126,212,364,10,RGB(91,91,96),RGB(29,29,32),RGB(14,14,16));
        text(d,localized(d,"Active theme"),746,142,TEXT_HINT,184,28,false,MENU_MUTED);
        text(d,pv_presets[pv_theme_id(v->theme_id)].name,746,174,TEXT_BODY,184,56,true,MENU_WHITE);
        text(d,localized(d,"Primary color"),746,238,TEXT_HINT,184,28,false,MENU_WHITE);
        text(d,localized(d,"Secondary color"),746,312,TEXT_HINT,184,28,false,MENU_WHITE);
        char hex[16];snprintf(hex,sizeof(hex),"#%06X",(unsigned)v->theme_primary);outline(d,746,266,184,35,6,RGB(85,85,89),RGB(25,25,28),RGB(18,18,20));rectangle(d,754,273,20,20,pv_theme_pixel(v->theme_primary));text(d,hex,782,271,TEXT_HINT,144,28,false,MENU_WHITE);
        snprintf(hex,sizeof(hex),"#%06X",(unsigned)v->theme_secondary);outline(d,746,340,184,35,6,RGB(85,85,89),RGB(25,25,28),RGB(18,18,20));rectangle(d,754,347,20,20,pv_theme_pixel(v->theme_secondary));text(d,hex,782,345,TEXT_HINT,144,28,false,MENU_WHITE);
        text(d,localized(d,"X  Select     O  Back"),746,405,TEXT_HINT,184,68,true,MENU_MUTED);
    }else if(v->screen==PLUS_VITA_APP_ACCOUNT||v->screen==PLUS_VITA_APP_SETUP){
        outline(d,18,78,508,412,12,RGB(91,91,96),RGB(30,30,33),RGB(14,14,16));
        menu_icon(d,PLUS_VITA_ACTION_ACCOUNT,58,112,readable(d,MENU_ACCENT));
        text(d,v->title?v->title:"VitaPlus",84,94,TEXT_TITLE,426,42,false,MENU_WHITE);
        rectangle(d,32,148,480,1,RGB(73,73,77));
        text(d,v->body,36,165,TEXT_BODY,472,220,true,MENU_BODY);
        if(v->screen==PLUS_VITA_APP_ACCOUNT){
            uint32_t status=v->account_connected?RGB(124,220,119):MENU_MUTED;
            ring(d,48,448,7,2,status);text(d,localized(d,v->account_connected?"Connected":"Not connected"),64,434,TEXT_COMPACT,430,38,false,status);
        }
    }else{
        int bottom=v->action_count?v->actions[0].y-8:480;
        if(bottom>90) {
            outline(d,18,77,924,bottom-77,9,RGB(62,62,65),RGB(26,26,29),RGB(15,15,17));
            text(d,v->body,32,85,bottom<150?TEXT_MICRO:TEXT_COMPACT,896,bottom-88,true,MENU_BODY);
        }
    }
    for(size_t j=0;j<v->action_count;j++)menu_card(d,v,j);
    if(v->list_total>v->list_count&&v->list_count&&v->list_height>0){
        int height=v->list_height,thumb=(int)((size_t)height*v->list_count/v->list_total);if(thumb<8)thumb=8;
        int offset=(int)((size_t)(height-thumb)*v->list_first/(v->list_total-v->list_count));
        rectangle(d,v->list_x+v->list_width+8,v->list_y,4,height,RGB(48,48,52));
        rectangle(d,v->list_x+v->list_width+8,v->list_y+offset,4,thumb,readable(d,MENU_ACCENT));
    }
}
#endif

 #if !PLUS_VITA_ADAPTIVE_STREAM
static void wordmark(Canvas *display,const PlusVitaBranding *branding)
{
	const uint32_t *image=plus_vita_branding_wordmark(branding);
	if(!image) {
		text(display,"VitaPlus",64,25,TEXT_BRAND,400,38,false,MENU_ACCENT);
		return;
	}
	for(unsigned y=0;y<40;y++) for(unsigned x=0;x<224;x++) {
		uint32_t source=image[y*224+x], alpha=source>>24;
		uint32_t *destination=&display->pixels[(20+y)*display->pitch+64+x];
		if(alpha==255) *destination=source;
		else if(alpha) {
			uint32_t result=0xff000000u;
			for(unsigned shift=0;shift<24;shift+=8)
				result|=(((((source>>shift)&255)*alpha+((*destination>>shift)&255)*(255-alpha)+127)/255)<<shift);
			*destination=result;
		}
	}
}

#endif

void plus_vita_app_paint_with_resources(PlusVitaFont *font,uint32_t *pixels,int pitch,const PlusVitaAppView *view,const PlusVitaCovers *covers,const PlusVitaBranding *branding)
{
	Canvas canvas = {.pixels=pixels,.pitch=pitch,.font=font}; Canvas *display = &canvas;
#if PLUS_VITA_ADAPTIVE_STREAM
	canvas.language=view->ui_language;canvas.custom=view->theme_custom;
	canvas.primary=pv_theme_pixel(view->theme_primary);canvas.secondary=pv_theme_pixel(view->theme_secondary);
	(void)branding;
	if(view->screen==PLUS_VITA_APP_CATALOGUE){library_paint(display,view,covers);return;}
	if(view->screen!=PLUS_VITA_APP_FONT_COMPARISON){menu_paint(display,view);return;}
    menu_chrome(display,view);
#else
	const uint32_t *background=plus_vita_branding_background(branding);
	for(int y = 0; y < HEIGHT; y++) {
		if(background) memcpy(pixels+y*pitch,background+y*WIDTH,WIDTH*sizeof(*pixels));
		else rectangle(display, 0, y, WIDTH, 1, RGB(0, 25 + y / 20, 70 + y / 6));
	}
	rectangle(display, 0, 0, WIDTH, 4, MENU_ACCENT);
	wordmark(display,branding);
#endif
	text(display, PLUS_VITA_BUILD_LABEL, 712, 32, TEXT_MICRO, 220, 24, false, MENU_MUTED);
	text(display, view->subtitle, 64, 94, TEXT_HINT, 832, 26, false, display->custom?pv_theme_pixel(view->theme_primary):MENU_ACCENT);
	text(display, view->title, 64, 128, TEXT_HEADING, 832, 54, false, MENU_WHITE);
	int body_bottom = view->action_count ? view->actions[0].y - 12 : 480;
	if(view->screen==PLUS_VITA_APP_CATALOGUE) {
		body_bottom=196;
		text(display,view->body,744,186,TEXT_MICRO,152,24,false,MENU_BODY);
	}
	if(view->screen==PLUS_VITA_APP_FONT_COMPARISON) body_bottom=view->comparison_page ? 224 : 302;
	text(display, view->body, 64, 196, (view->screen == PLUS_VITA_APP_ACCOUNT || view->screen==PLUS_VITA_APP_DETAILS || view->screen==PLUS_VITA_APP_DATACENTERS || view->screen==PLUS_VITA_APP_STREAM_STOPPED || (PLUS_VITA_ADAPTIVE_STREAM && (view->screen==PLUS_VITA_APP_ABOUT || view->screen==PLUS_VITA_APP_CONTROLS))) ? TEXT_COMPACT : TEXT_BODY,
		view->screen == PLUS_VITA_APP_WELCOME ? 520 : view->screen==PLUS_VITA_APP_DETAILS && !PLUS_VITA_ADAPTIVE_STREAM ? 624 : 832, body_bottom - 196, view->screen!=PLUS_VITA_APP_CATALOGUE, MENU_BODY);
	if(view->screen==PLUS_VITA_APP_CATALOGUE && view->detail) {
		if(view->preview_eligible || !view->detail->cover_url[0]) cover(display,covers,view->detail,744,216);
		else {
			rectangle(display,744,216,128,128,MENU_PANEL);
			text(display,localized(display,"Loading cover"),750,264,TEXT_MICRO,116,58,true,MENU_MUTED);
		}
	}
	if(!PLUS_VITA_ADAPTIVE_STREAM && view->screen==PLUS_VITA_APP_DETAILS) cover(display,covers,view->detail,744,216);
	if(view->screen==PLUS_VITA_APP_FONT_COMPARISON) {
		if(view->comparison_page) {
			static const PlusVitaCatalogEntry samples[]={
				{.name="Agjpqy: Complete letters"},{.name="Müller 日本"},
				{.name="A long game title with complete letters and deliberate overflow"}};
			for(unsigned i=0;i<3;i++) {
				int x=64+(int)i*286;
				rounded(display,x,228,260,204,i==1 ? MENU_ACCENT : MENU_BUTTON);
				cover(display,NULL,&samples[i],x+66,238);
				text(display,samples[i].name,x+12,374,TEXT_COMPACT,236,48,true,i==1 ? MENU_INK : MENU_WHITE);
			}
		} else {
			text(display,"14 px: Agjpqy é Å Müller",64,310,TEXT_MICRO,832,24,false,MENU_BODY);
			text(display,"16 px: Agjpqy é Å Müller",64,342,TEXT_HINT,832,28,false,MENU_BODY);
			text(display,"18 px: Agjpqy é Å Müller 日本",64,376,TEXT_COMPACT,832,32,false,MENU_BODY);
		}
	}
	if(view->list_total>view->list_count && view->list_count) {
		int height=view->list_height;
		int thumb=(int)((size_t)height*view->list_count/view->list_total);
		if(thumb<8) thumb=8;
		int offset=(int)((size_t)(height-thumb)*view->list_first/(view->list_total-view->list_count));
		rectangle(display,view->list_x+view->list_width+8,view->list_y,4,height,MENU_BUTTON);
		rectangle(display,view->list_x+view->list_width+8,view->list_y+offset,4,thumb,MENU_ACCENT);
	}
	for(size_t i = 0; i < view->action_count; i++)
	{
		const PlusVitaAppButton *button = &view->actions[i];
		bool selected = i == view->selected;
		bool pressed = view->touch_pressed && i == view->pressed_index;
		rounded(display, button->x, button->y, button->width, button->height,
			pressed ? (display->custom?display->secondary:MENU_PRESSED) : selected ? (display->custom?display->primary:MENU_ACCENT) : MENU_BUTTON);
#if PLUS_VITA_ADAPTIVE_STREAM
		if(view->screen==PLUS_VITA_APP_CATALOGUE && button->id==PLUS_VITA_ACTION_SEARCH && !selected && !pressed){
			/* Make the search entry visually distinct from normal menu buttons. */
			rounded(display,button->x,button->y,button->width,button->height,MENU_ACCENT);
			rounded(display,button->x+2,button->y+2,button->width-4,button->height-4,MENU_PANEL);
		}
#endif
		bool compact=button->height<48;
		int inset=button->height<=32 ? 2 : compact ? 4 : 8;
		text(display,button->label,button->x+20,button->y+inset,
			compact ? TEXT_COMPACT : TEXT_CONTROL,button->width-40,button->height-2*inset,false,
			selected || pressed ? (display->custom?pv_theme_ink(pressed?display->secondary:display->primary):MENU_INK) : MENU_WHITE);
		if(view->screen==PLUS_VITA_APP_THEME&&(button->id==PLUS_VITA_ACTION_PRIMARY||button->id==PLUS_VITA_ACTION_SECONDARY)){
			uint32_t swatch=pv_theme_pixel(button->id==PLUS_VITA_ACTION_PRIMARY?view->theme_primary:view->theme_secondary);
			rectangle(display,button->x+button->width-72,button->y+6,56,button->height-12,MENU_WHITE);
			rectangle(display,button->x+button->width-70,button->y+8,52,button->height-16,swatch);
		}
	}
	text(display, view->hint, 64, 509, TEXT_HINT, 832, 28, false, MENU_MUTED);
}

void plus_vita_app_paint(PlusVitaFont *font,uint32_t *pixels,int pitch,const PlusVitaAppView *view)
{ plus_vita_app_paint_with_covers(font,pixels,pitch,view,NULL); }

void plus_vita_app_paint_with_covers(PlusVitaFont *font,uint32_t *pixels,int pitch,const PlusVitaAppView *view,const PlusVitaCovers *covers)
{ plus_vita_app_paint_with_resources(font,pixels,pitch,view,covers,NULL); }
