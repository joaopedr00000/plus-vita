// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/app/render.h"
#include "../src/app/theme.h"
#include "../src/app/lan_login_page.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t pixels[960*544];
static uint32_t label_colors[2];
bool plus_vita_font_draw(PlusVitaFont *font,uint32_t *p,int pitch,const char *value,PlusVitaTextBox box,int size,bool wrap,uint32_t color){
    (void)font;(void)p;(void)pitch;(void)box;(void)size;(void)wrap;
    if(value && !strcmp(value,"First"))label_colors[0]=color;
    if(value && !strcmp(value,"Second"))label_colors[1]=color;
    return true;
}
const PlusVitaCoverImage *plus_vita_covers_find(const PlusVitaCovers *covers,const char *url){
    (void)covers;(void)url;return NULL;
}
int main(void){
    PlusVitaAppView v={.screen=PLUS_VITA_APP_WELCOME,.title="VitaPlus",.body="Welcome",.subtitle="Welcome",.hint="Back",.action_count=2,
        .actions={{PLUS_VITA_ACTION_SETUP,"First",64,300,500,44},{PLUS_VITA_ACTION_SETTINGS,"Second",64,350,500,44}}};
    plus_vita_app_paint(NULL,pixels,960,&v);
    assert(pixels[0]==VP_GOLD);
    assert(pixels[100*960+900]==vp_menu_background(100));
    assert(pixels[320*960+80]==VP_GOLD && label_colors[0]==VP_DARK_INK);
    assert(pixels[370*960+80]==VP_BUTTON && label_colors[1]==VP_WHITE);
    v.selected=1;v.touch_pressed=true;v.pressed_index=1;
    plus_vita_app_paint(NULL,pixels,960,&v);
    assert(pixels[320*960+80]==VP_BUTTON && label_colors[0]==VP_WHITE);
    assert(pixels[370*960+80]==VP_GOLD_PRESSED && label_colors[1]==VP_DARK_INK);
    v.screen=PLUS_VITA_APP_CATALOGUE;v.action_count=0;
    plus_vita_app_paint(NULL,pixels,960,&v);
    assert(pixels[200*960+40]==VP_PANEL && pixels[200*960+510]==VP_PANEL);
    for(unsigned lang=0;lang<19;lang++){
        assert(strstr(pv_lan_pages[lang],"background:#161618"));
        assert(strstr(pv_lan_pages[lang],"background:#ffcf40"));
        assert(!strstr(pv_lan_pages[lang],"#0070cc"));
        assert(strstr(pv_lan_pages[lang],"<title>VitaPlus</title>"));
    }
    puts("VitaPlus dark gold theme, focus, pressed state and 19 QR pages passed");return 0;
}
