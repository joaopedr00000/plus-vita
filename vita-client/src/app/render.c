// SPDX-License-Identifier: AGPL-3.0-only
#include "render.h"
#include <string.h>
#include "../platform/build_identity.h"
#include "../platform/adaptive_stream.h"
#include "theme.h"
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
	TEXT_CONTROL=22, TEXT_BRAND=24, TEXT_HEADING=36
} TextRole;
typedef struct { uint32_t *pixels; int pitch; PlusVitaFont *font; unsigned language; } Canvas;
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
/* Reuse decoded art; never download or allocate in the painter. Remove the
 * decoder's uniform letterbox margins before fitting into the portrait well. */
static void library_art(Canvas *display,const PlusVitaCoverImage *image,int x,int y,int w,int h)
{
	rounded(display,x,y,w,h,RGB(38,38,40));
	if(!image || image->state!=PLUS_VITA_COVER_READY)return;
	int left=0,right=127,top=0,bottom=127;
	const uint32_t pad=0xff1f1d1du;
	while(left<right){bool blank=true;for(int k=0;k<128;k++)if(image->pixels[k*128+left]!=pad){blank=false;break;}if(!blank)break;left++;}
	while(right>left){bool blank=true;for(int k=0;k<128;k++)if(image->pixels[k*128+right]!=pad){blank=false;break;}if(!blank)break;right--;}
	while(top<bottom){bool blank=true;for(int k=0;k<128;k++)if(image->pixels[top*128+k]!=pad){blank=false;break;}if(!blank)break;top++;}
	while(bottom>top){bool blank=true;for(int k=0;k<128;k++)if(image->pixels[bottom*128+k]!=pad){blank=false;break;}if(!blank)break;bottom--;}
	int sw=right-left+1,sh=bottom-top+1,dw=w,dh=h;
	if(sw*h>sh*w)dh=w*sh/sw;else dw=h*sw/sh;
	if(dw<1)dw=1;
	if(dh<1)dh=1;
	x+=(w-dw)/2;y+=(h-dh)/2;
	for(int row=0;row<dh;row++)for(int col=0;col<dw;col++)
		display->pixels[(y+row)*display->pitch+x+col]=image->pixels[(top+row*sh/dh)*128+left+col*sw/dw];
}
static void library_background(Canvas *display)
{
	for(int y=0;y<HEIGHT;y++){
		/* Subtle warm gold ribbon behind the content, no large texture. */
		rectangle(display,0,y,WIDTH,1,RGB(15+y*9/544,15+y*7/544,17+y*4/544));
	}
	for(int x=405;x<960;x++){
		int bend=x-680;
		int y=55+(bend*bend)/2200;
		rectangle(display,x,y,1,2,RGB(112,87,35));
		rectangle(display,x,487-(bend*bend)/2600,1,1,RGB(92,70,28));
	}
	rectangle(display,0,0,960,3,MENU_ACCENT);
	/* Small directional gold mark matching the reference, not an app icon change. */
	rounded(display,24,12,12,17,MENU_ACCENT);rounded(display,24,33,12,17,MENU_ACCENT);
	rounded(display,8,25,13,12,MENU_ACCENT);rounded(display,39,25,13,12,MENU_ACCENT);
	text(display,"VitaPlus",58,14,TEXT_BRAND,180,37,false,MENU_WHITE);
	rectangle(display,209,14,1,36,RGB(145,113,47));
}
static void library_button(Canvas *display,const PlusVitaAppView *view,const PlusVitaCovers *covers,size_t i)
{
	const PlusVitaAppButton *b=&view->actions[i];
	bool focus=i==view->selected,pressed=view->touch_pressed&&i==view->pressed_index;
	if(b->id==PLUS_VITA_ACTION_GAME && i<view->list_count){
		bool chosen=view->focus==view->list_first+i;
		const PlusVitaCatalogEntry *game=view->games[i];
		if(chosen||focus||pressed){
			rounded(display,b->x,b->y,b->width,b->height,MENU_ACCENT);
			rounded(display,b->x+2,b->y+2,b->width-4,b->height-4,pressed?RGB(104,78,33):RGB(61,49,29));
		}else rectangle(display,b->x,b->y+b->height-1,b->width,1,RGB(51,51,53));
		const PlusVitaCoverImage *art=game?plus_vita_covers_find(covers,game->cover_url):NULL;
		library_art(display,art,b->x+7,b->y+5,52,38);
		text(display,b->label,b->x+68,b->y+7,TEXT_COMPACT,230,36,false,MENU_WHITE);
		if(game && game->platform[0]){
			rounded(display,b->x+302,b->y+9,56,30,chosen?MENU_ACCENT:RGB(82,82,86));
			rounded(display,b->x+304,b->y+11,52,26,RGB(22,22,24));
			text(display,game->platform,b->x+311,b->y+12,TEXT_HINT,43,25,false,chosen?MENU_ACCENT:MENU_WHITE);
		}
		return;
	}
	bool filter=b->id>=PLUS_VITA_ACTION_FILTER_ALL&&b->id<=PLUS_VITA_ACTION_FILTER_PS5;
	bool active=filter && (unsigned)(b->id-PLUS_VITA_ACTION_FILTER_ALL)==view->platform_filter;
	bool play=b->id==PLUS_VITA_ACTION_PLAY;
	uint32_t edge=(active||focus||play)?MENU_ACCENT:RGB(105,105,110);
	rounded(display,b->x,b->y,b->width,b->height,edge);
	rounded(display,b->x+2,b->y+2,b->width-4,b->height-4,pressed?MENU_PRESSED:(active||play)?MENU_ACCENT:RGB(36,36,39));
	const char *label=b->id==PLUS_VITA_ACTION_CLEAR_SEARCH?"X":b->label;
	int inset=b->id==PLUS_VITA_ACTION_SEARCH?32:10;
	if(b->id==PLUS_VITA_ACTION_SEARCH){
		/* Magnifier built from bounded framebuffer rectangles. */
		rectangle(display,b->x+9,b->y+10,12,2,MENU_MUTED);
		rectangle(display,b->x+9,b->y+20,12,2,MENU_MUTED);
		rectangle(display,b->x+8,b->y+12,2,8,MENU_MUTED);
		rectangle(display,b->x+20,b->y+12,2,8,MENU_MUTED);
		for(int k=0;k<7;k++)rectangle(display,b->x+20+k,b->y+21+k,2,2,MENU_MUTED);
	}
	text(display,label,b->x+inset,b->y+(play?9:5),play?TEXT_CONTROL:TEXT_HINT,b->width-inset-6,b->height-10,false,(active||play||pressed)?MENU_INK:MENU_WHITE);
}
static void library_paint(Canvas *display,const PlusVitaAppView *view,const PlusVitaCovers *covers)
{
	library_background(display);
	text(display,view->subtitle,224,12,TEXT_HINT,510,24,false,MENU_WHITE);
	text(display,view->body,224,36,TEXT_MICRO,510,22,false,MENU_MUTED);
	text(display,PLUS_VITA_BUILD_LABEL,800,17,TEXT_MICRO,144,24,false,MENU_MUTED);
	rectangle(display,0,108,960,1,RGB(75,75,78));
	rectangle(display,396,109,1,377,RGB(75,75,78));
	rectangle(display,0,489,960,1,RGB(75,75,78));
	if(view->list_total>view->list_count && view->list_count){
		int thumb=(int)((size_t)view->list_height*view->list_count/view->list_total);
		if(thumb<8)thumb=8;
		int offset=(int)((size_t)(view->list_height-thumb)*view->list_first/(view->list_total-view->list_count));
		rectangle(display,386,view->list_y,5,view->list_height,RGB(44,44,46));
		rectangle(display,386,view->list_y+offset,5,thumb,RGB(118,118,120));
	}
	bool ps5=view->detail?pv_catalog_platform(view->detail->platform)==3:view->platform_filter==3;
	if(view->detail){
		const PlusVitaCatalogEntry *g=view->detail;
		const PlusVitaCoverImage *art=view->preview_eligible?plus_vita_covers_find(covers,g->cover_url):NULL;
		library_art(display,art,410,118,230,352);
		if(!art || art->state!=PLUS_VITA_COVER_READY){
			const char *status=!g->cover_url[0]?"No cover":!covers||(art&&art->state==PLUS_VITA_COVER_FAILED)?"Cover unavailable":"Loading cover";
			text(display,localized(display,status),422,268,TEXT_HINT,206,62,true,MENU_MUTED);
		}
		text(display,g->name,656,120,TEXT_BRAND,288,100,true,MENU_WHITE);
		rounded(display,656,225,68,32,MENU_ACCENT);
		rounded(display,658,227,64,28,RGB(25,25,27));
		text(display,g->platform[0]?g->platform:localized(display,"Not specified"),665,228,TEXT_HINT,54,26,false,MENU_ACCENT);
		rectangle(display,656,269,288,1,RGB(79,79,82));
		if(g->launch_id_valid)text(display,g->id,656,280,TEXT_MICRO,288,46,true,MENU_MUTED);
		text(display,localized(display,"Playtime: unavailable | Trophies: unavailable"),656,330,TEXT_HINT,288,52,true,MENU_MUTED);
		if(ps5)text(display,localized(display,"PS5 streaming is not available yet. Support is planned."),656,382,TEXT_MICRO,288,47,true,MENU_ACCENT);
	}else {
		text(display,localized(display,view->title?view->title:"No cloud games found."),24,145,TEXT_BODY,350,100,true,MENU_BODY);
		if(ps5)text(display,localized(display,"PS5 streaming is not available yet. Support is planned."),414,150,TEXT_BODY,510,100,true,MENU_ACCENT);
	}
	for(size_t i=0;i<view->action_count;i++)library_button(display,view,covers,i);
	text(display,view->hint,20,505,TEXT_HINT,802,31,false,MENU_MUTED);
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
	Canvas canvas = {pixels, pitch, font, 0}; Canvas *display = &canvas;
#if PLUS_VITA_ADAPTIVE_STREAM
	canvas.language=view->ui_language;
	(void)branding;
	for(int y=0;y<HEIGHT;y++)rectangle(display,0,y,WIDTH,1,vp_menu_background((unsigned)y));
	rectangle(display,0,0,WIDTH,4,MENU_ACCENT);
	text(display,"VitaPlus",64,25,TEXT_BRAND,400,38,false,MENU_WHITE);
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
	#if PLUS_VITA_ADAPTIVE_STREAM
	if(view->screen==PLUS_VITA_APP_CATALOGUE){library_paint(display,view,covers);return;}
#endif
	text(display, view->subtitle, 64, 94, TEXT_HINT, 832, 26, false, MENU_ACCENT);
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
			pressed ? MENU_PRESSED : selected ? MENU_ACCENT : MENU_BUTTON);
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
			selected || pressed ? MENU_INK : MENU_WHITE);
	}
	text(display, view->hint, 64, 509, TEXT_HINT, 832, 28, false, MENU_MUTED);
}

void plus_vita_app_paint(PlusVitaFont *font,uint32_t *pixels,int pitch,const PlusVitaAppView *view)
{ plus_vita_app_paint_with_covers(font,pixels,pitch,view,NULL); }

void plus_vita_app_paint_with_covers(PlusVitaFont *font,uint32_t *pixels,int pitch,const PlusVitaAppView *view,const PlusVitaCovers *covers)
{ plus_vita_app_paint_with_resources(font,pixels,pitch,view,covers,NULL); }
