// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/app/app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/app/custom_theme.h"
static void click(PlusVitaApp *app,PlusVitaAppAction action){
    PlusVitaAppView v=plus_vita_app_view(app);
    for(size_t j=0;j<v.action_count;j++)if(v.actions[j].id==action){
        plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_TOUCH,.x=v.actions[j].x+12,.y=v.actions[j].y+12});return;
    }
    assert(0 && "Action not visible");
}
int main(void){
    PlusVitaApp *app=plus_vita_app_create();assert(app);
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_STREAM_SETTINGS,.stream_profile=10000,.ui_language=2});
    click(app,PLUS_VITA_ACTION_SETTINGS);
    click(app,PLUS_VITA_ACTION_STREAM_SETTINGS);
    PlusVitaAppView v=plus_vita_app_view(app);
    assert(v.screen==PLUS_VITA_APP_STREAM_SETTINGS && v.selected==1 && v.stream_profile==10000);
    assert(!strcmp(v.actions[0].label,"Latência baixa - 6 Mbps"));
    click(app,PLUS_VITA_ACTION_STREAM_LOW);
    v=plus_vita_app_view(app);assert(v.selected==0 && v.stream_profile==6000 && strstr(v.body,"6 Mbps"));
    click(app,PLUS_VITA_ACTION_STREAM_MAX);
    v=plus_vita_app_view(app);assert(v.selected==2 && v.stream_profile==15000 && strstr(v.body,"15 Mbps"));
    click(app,PLUS_VITA_ACTION_BACK);
    assert(plus_vita_app_view(app).screen==PLUS_VITA_APP_SETTINGS);
    click(app,PLUS_VITA_ACTION_STREAM_SETTINGS);
    assert(plus_vita_app_view(app).selected==2);
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_BACK});
    assert(plus_vita_app_view(app).screen==PLUS_VITA_APP_SETTINGS);
    uint32_t color=0;
    assert(pv_theme_hex("#00abFF",7,&color)&&color==0x00abff);
    assert(pv_theme_hex("000000",6,&color)&&color==0);
    assert(!pv_theme_hex("12345G",6,&color)&&!pv_theme_hex("1234567",7,&color));
    for(unsigned j=0;j<8;j++)plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_DOWN});
    click(app,PLUS_VITA_ACTION_THEME);
    v=plus_vita_app_view(app);assert(v.screen==PLUS_VITA_APP_THEME);
    assert(v.theme_primary==PV_PRIMARY_DEFAULT&&!v.theme_custom);
    PlusVitaAppEffect edit=plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_ACTIVATE});
    assert(edit.kind==PLUS_VITA_EFFECT_EDIT_PRIMARY);
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_THEME_ENTERED,.operation=edit.operation,.status=PLUS_VITA_RESULT_OK,.data="0088ff",.data_size=6});
    v=plus_vita_app_view(app);assert(v.theme_custom&&v.theme_primary==0x0088ff);
    click(app,PLUS_VITA_ACTION_THEME_RESET);
    v=plus_vita_app_view(app);assert(!v.theme_custom&&v.theme_primary==PV_PRIMARY_DEFAULT);
    plus_vita_app_destroy(app);
    puts("Streaming profiles, localized labels and menu navigation passed");return 0;
}
