// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/app/render.h"
#include "../src/app/theme.h"
#include "../src/app/custom_theme.h"
#include "../src/app/lan_login_page.h"
#include "../src/app/i18n.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t pixels[960*544];
static uint32_t label_colors[2];
static PlusVitaCoverImage artwork;
static bool metrics_seen,ps5_seen,status_seen,account_body_seen;
static uint32_t logo[128*128];
bool plus_vita_font_draw(PlusVitaFont *font,uint32_t *p,int pitch,const char *value,PlusVitaTextBox box,int size,bool wrap,uint32_t color){
    (void)font;(void)p;(void)pitch;(void)box;(void)size;(void)wrap;
    if(value && !strcmp(value,"First"))label_colors[0]=color;
    if(value && !strcmp(value,"Second"))label_colors[1]=color;
    if(value && strstr(value,"Playtime: unavailable"))metrics_seen=true;
    if(value && !strcmp(value,"97%"))status_seen=true;
    if(value && !strcmp(value,"Account test details")){assert(box.height>=180&&box.x<500);account_body_seen=true;}
    if(value && strstr(value,"PS5 streaming is not available yet"))ps5_seen=true;
    return true;
}
const PlusVitaCoverImage *plus_vita_covers_find(const PlusVitaCovers *covers,const char *url){
    return covers && url && !strcmp(url,artwork.url) ? &artwork : NULL;
}
int main(void){
    PlusVitaAppView v={.screen=PLUS_VITA_APP_WELCOME,.title="VitaPlus",.body="Welcome",.subtitle="Welcome",.hint="Back",.action_count=2,
        .actions={{PLUS_VITA_ACTION_SETUP,"First",64,300,500,44},{PLUS_VITA_ACTION_SETTINGS,"Second",64,350,500,44}}};
    plus_vita_app_paint(NULL,pixels,960,&v);
    assert(pixels[64*960+40]==VP_RGB(105,105,109));
    assert(pixels[300*960+110]==VP_RGB(255,216,125) && label_colors[0]==VP_DARK_INK);
    assert(pixels[350*960+110]==VP_RGB(119,119,123) && label_colors[1]==VP_WHITE);
    v.selected=1;v.touch_pressed=true;v.pressed_index=1;
    plus_vita_app_paint(NULL,pixels,960,&v);
    assert(pixels[300*960+110]==VP_RGB(119,119,123) && label_colors[0]==VP_WHITE);
    assert(pixels[350*960+110]==VP_RGB(255,216,125) && label_colors[1]==VP_DARK_INK);
    /* Compare actual rasterized button pixels across screens, not a mock palette. */
    uint32_t sample[44][500];
    for(int y=0;y<44;y++)memcpy(sample[y],pixels+(350+y)*960+64,500*sizeof(uint32_t));
    v.screen=PLUS_VITA_APP_CATALOGUE;
    plus_vita_app_paint(NULL,pixels,960,&v);
    for(int y=4;y<40;y++)for(int x=10;x<490;x++)assert(sample[y][x]==pixels[(350+y)*960+64+x]);
    v.screen=PLUS_VITA_APP_WELCOME;
    v.theme_custom=true;v.theme_primary=0x2277ff;v.theme_secondary=0xa000cc;
    v.selected=0;v.touch_pressed=false;
    plus_vita_app_paint(NULL,pixels,960,&v);
    assert(label_colors[1]==VP_WHITE);
    v.theme_primary=0;v.theme_secondary=0xffffff;
    plus_vita_app_paint(NULL,pixels,960,&v);
    assert(label_colors[1]==VP_WHITE);
    v.theme_custom=false;
    v.screen=PLUS_VITA_APP_THEME;v.battery_valid=true;v.battery_percent=97;v.action_count=1;
    v.actions[0]=(PlusVitaAppButton){PLUS_VITA_ACTION_PRIMARY,"First",64,182,832,44};
    plus_vita_app_paint(NULL,pixels,960,&v);assert(status_seen&&label_colors[0]==VP_DARK_INK);
    v.screen=PLUS_VITA_APP_ACCOUNT;v.body="Account test details";v.actions[0]=(PlusVitaAppButton){PLUS_VITA_ACTION_BACK,"Back",544,93,398,56};
    plus_vita_app_paint(NULL,pixels,960,&v);assert(account_body_seen);
    /* A catalogue view has its own count in body, not the previous account details. */
    v=(PlusVitaAppView){.screen=PLUS_VITA_APP_CATALOGUE,.title="No cloud games found.",.body="0 games",.subtitle="Game Library",.hint="Back"};
    plus_vita_app_paint(NULL,pixels,960,&v);
    assert(pixels[104*960+40]==VP_RGB(75,75,78) && pixels[200*960+395]==VP_RGB(75,75,78));
    for(unsigned k=0;k<128*128;k++)logo[k]=0xffffffff;
    v.library_logo=logo;v.clock_valid=true;v.clock_hour=10;v.clock_minute=24;v.battery_valid=true;v.battery_percent=97;v.network_known=true;v.network_connected=true;
    plus_vita_app_paint(NULL,pixels,960,&v);
    assert(pixels[25*960+50]==0xffffffff && pixels[15*960+930]==VP_GOLD);
    PlusVitaCatalogEntry game={.name="Synthetic PS5",.platform="PS5",.id="FAKEPPSA1",.launch_id_valid=true,
        .cover_url="https://image.api.playstation.com/test.png"};
    strcpy(artwork.url,game.cover_url);artwork.state=PLUS_VITA_COVER_READY;
    for(unsigned i=0;i<128*128;i++)artwork.pixels[i]=0xff123456;
    v.detail=&game;v.preview_eligible=true;
    plus_vita_app_paint_with_covers(NULL,pixels,960,&v,(const PlusVitaCovers *)&artwork);
    assert(pixels[292*960+525]==0xff123456 && metrics_seen && ps5_seen);
    v.preview_eligible=false;
    plus_vita_app_paint_with_covers(NULL,pixels,960,&v,(const PlusVitaCovers *)&artwork);
    assert(pixels[292*960+525]!=0xff123456);
    /* All indicator interior pixels are solid, independently of row focus. */
    v=(PlusVitaAppView){.screen=PLUS_VITA_APP_STREAM_OPTIONS,.title="Streaming",.body="Test description",.action_count=2,
        .actions={{PLUS_VITA_ACTION_OPT_REORDER,"First",24,86,542,34},{PLUS_VITA_ACTION_OPT_HUD,"Second",24,126,542,34}}};
    for(unsigned focus=0;focus<2;focus++){
        v.selected=focus;
        for(unsigned mask=0;mask<4;mask++){
            v.stream_options=(mask&1)?PV_OPT_REORDER:0;
            v.stream_hud=(mask&2)!=0;
            plus_vita_app_paint(NULL,pixels,960,&v);
            for(unsigned j=0;j<2;j++){
                uint32_t expected=(mask&(1u<<j))?VP_RGB(240,211,111):VP_RGB(27,27,30);
                int x=522,y=92+40*(int)j;
                for(int dy=5;dy<17;dy++)for(int dx=5;dx<17;dx++)
                    assert(pixels[(y+dy)*960+x+dx]==expected);
            }
        }
    }
    /* Compare controller icon pixels (not only colors) with catalogue originals.
     * Two standard 172px slots start at x=600, glyph centers 612 and 784. */
    uint32_t glyphs[2][25][25];
    v=(PlusVitaAppView){.screen=PLUS_VITA_APP_CATALOGUE,.body="0 games"};
    plus_vita_app_paint(NULL,pixels,960,&v);
    const int original[2]={800,878};
    for(unsigned g=0;g<2;g++)for(int y=0;y<25;y++)for(int x=0;x<25;x++)
        glyphs[g][y][x]=pixels[(508+y)*960+original[g]-12+x];
    const int centers[2]={612,784};
    const PlusVitaAppScreen menus[]={PLUS_VITA_APP_WELCOME,PLUS_VITA_APP_SETTINGS,PLUS_VITA_APP_SETUP,PLUS_VITA_APP_ACCOUNT,PLUS_VITA_APP_THEME,PLUS_VITA_APP_STREAM_SETTINGS,PLUS_VITA_APP_CONTROLS,PLUS_VITA_APP_SECURITY,PLUS_VITA_APP_DETAILS,PLUS_VITA_APP_DATACENTERS,PLUS_VITA_APP_REPORTING,PLUS_VITA_APP_STREAM_STOPPED};
    for(unsigned lang=0;lang<19;lang++){
        for(unsigned k=0;k<sizeof(menus)/sizeof(menus[0]);k++){
            v=(PlusVitaAppView){.screen=menus[k],.ui_language=lang,.hint=pv_translate(lang,"X  Select     O  Back")};
            plus_vita_app_paint(NULL,pixels,960,&v);
            for(unsigned g=0;g<2;g++)for(int y=0;y<25;y++)for(int x=0;x<25;x++){
                uint32_t expected=glyphs[g][y][x];
                if(expected==VP_WHITE||expected==VP_GOLD||expected==VP_RGB(105,105,109))
                    assert(expected==pixels[(508+y)*960+centers[g]-12+x]);
            }
        }
        v=(PlusVitaAppView){.screen=PLUS_VITA_APP_AUTHORIZING,.ui_language=lang,.hint=pv_translate(lang,"O  Cancel")};
        plus_vita_app_paint(NULL,pixels,960,&v);
        assert(pixels[508*960+784]==VP_RGB(105,105,109));
        v.hint=pv_translate(lang,"Please wait");
        plus_vita_app_paint(NULL,pixels,960,&v);
        assert(pixels[508*960+784]!=VP_RGB(105,105,109));
        assert(strstr(pv_lan_pages[lang],"background:#161618"));
        assert(strstr(pv_lan_pages[lang],"background:#ffcf40"));
        assert(!strstr(pv_lan_pages[lang],"#0070cc"));
        assert(strstr(pv_lan_pages[lang],"<title>VitaPlus</title>"));
    }
    puts("VitaPlus dark gold theme, focus, pressed state and 19 QR pages passed");return 0;
}
