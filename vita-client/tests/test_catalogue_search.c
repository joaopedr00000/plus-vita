// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/app/app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static PlusVitaAppEffect send(PlusVitaApp *a,PlusVitaAppEventKind kind){
    return plus_vita_app_dispatch(a,(PlusVitaAppEvent){.kind=kind});
}
static PlusVitaAppEffect click(PlusVitaApp *a,PlusVitaAppAction action){
    PlusVitaAppView v=plus_vita_app_view(a);
    for(size_t i=0;i<v.action_count;i++)if(v.actions[i].id==action)
        return plus_vita_app_dispatch(a,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_TOUCH,.x=v.actions[i].x+12,.y=v.actions[i].y+10});
    assert(0&&"Missing catalogue action");
    return (PlusVitaAppEffect){0};
}
static void entered(PlusVitaApp *a,PlusVitaAppEffect e,PlusVitaAppResult result,const char *query){
    assert(e.kind==PLUS_VITA_EFFECT_EDIT_SEARCH);
    plus_vita_app_dispatch(a,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_SEARCH_ENTERED,.operation=e.operation,
        .status=result,.data=query,.data_size=query?strlen(query):0});
}
static void search(PlusVitaApp *a,const char *query){
    entered(a,click(a,PLUS_VITA_ACTION_SEARCH),PLUS_VITA_RESULT_OK,query);
}
static PlusVitaApp *connected(void){
    PlusVitaApp *a=plus_vita_app_create();assert(a);
    PlusVitaAppEffect load=send(a,PLUS_VITA_EVENT_START);
    char token[65],json[96];memset(token,'A',64);token[64]=0;
    int n=snprintf(json,sizeof(json),"{\"npsso\":\"%s\"}",token);
    assert(n>0&&(size_t)n<sizeof(json));
    PlusVitaAppEffect auth=plus_vita_app_dispatch(a,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_ACCOUNT_LOADED,.operation=load.operation,.data=json,.data_size=(size_t)n});
    PlusVitaCatalogEntry entries[]={
        {.name="Alpha Racing",.id="FAKE00001",.platform="PS3",.launch_id_valid=true},
        {.name="Beta Racing",.id="FAKE00002",.platform="PS4",.launch_id_valid=true},
        {.name="Racing Future",.id="FAKE00003",.platform="PS5",.ps5_stream_id="FAKE-ENTITLEMENT",.launch_id_valid=true},
        {.name="Adventure 日本",.id="FAKE00004",.platform="PS4",.launch_id_valid=true},
        {.name="Racing Classic",.id="FAKE00005",.platform="PS4",.launch_id_valid=true},
        {.name="Racing One",.id="FAKE00006",.platform="PS4",.launch_id_valid=true},
        {.name="Racing Two",.id="FAKE00007",.platform="PS4",.launch_id_valid=true},
        {.name="Racing Three",.id="FAKE00008",.platform="PS4",.launch_id_valid=true},
        {.name="Racing Four",.id="FAKE00009",.platform="PS4",.launch_id_valid=true},
        {.name="Racing Five",.id="FAKE00010",.platform="PS4",.launch_id_valid=true}
    };
    plus_vita_app_dispatch(a,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_AUTHORIZED,.operation=auth.operation,
        .entries=entries,.entry_count=sizeof(entries)/sizeof(entries[0]),.store_country="CH",.store_language="de",.language="de-CH"});
    return a;
}
int main(void){
    PlusVitaApp *a=connected();
    PlusVitaAppView v=plus_vita_app_view(a);
    assert(v.screen==PLUS_VITA_APP_CATALOGUE&&v.list_total==10&&v.list_count==7);
    search(a,"rAcInG");
    v=plus_vita_app_view(a);
    assert(v.list_total==9&&v.list_count==7&&v.action_count==16);
    assert(strstr(v.actions[v.list_count].label,"rAcInG"));
    assert(v.actions[v.list_count+1].id==PLUS_VITA_ACTION_CLEAR_SEARCH);
    click(a,PLUS_VITA_ACTION_FILTER_PS3);
    v=plus_vita_app_view(a);assert(v.platform_filter==1&&v.list_total==1&&v.detail&&!strcmp(v.detail->platform,"PS3"));
    click(a,PLUS_VITA_ACTION_FILTER_PS4);
    v=plus_vita_app_view(a);assert(v.platform_filter==2&&v.list_total==7);
    entered(a,click(a,PLUS_VITA_ACTION_SEARCH),PLUS_VITA_RESULT_CANCELLED,NULL);
    v=plus_vita_app_view(a);assert(v.list_total==7&&v.platform_filter==2);
    search(a,"日本");v=plus_vita_app_view(a);
    assert(v.list_total==1&&v.detail&&strstr(v.detail->name,"日本"));
    search(a,"missing");
    v=plus_vita_app_view(a);assert(!v.list_total&&!v.detail&&!strcmp(v.title,"No search results."));
    send(a,PLUS_VITA_EVENT_DOWN);v=plus_vita_app_view(a);
    assert(v.actions[v.selected].id==PLUS_VITA_ACTION_CLEAR_SEARCH);
    send(a,PLUS_VITA_EVENT_ACTIVATE);
    v=plus_vita_app_view(a);assert(v.list_total==8&&v.platform_filter==2);
    send(a,PLUS_VITA_EVENT_UP);
    v=plus_vita_app_view(a);assert(v.actions[v.selected].id==PLUS_VITA_ACTION_SEARCH);
    entered(a,send(a,PLUS_VITA_EVENT_ACTIVATE),PLUS_VITA_RESULT_OK,"Beta");
    v=plus_vita_app_view(a);assert(v.list_total==1&&!strcmp(v.detail->name,"Beta Racing"));
    PlusVitaAppEffect play=click(a,PLUS_VITA_ACTION_GAME);
    assert(play.kind==PLUS_VITA_EFFECT_PLAY&&!strcmp(play.game.name,"Beta Racing"));
    send(a,PLUS_VITA_EVENT_BACK);
    v=plus_vita_app_view(a);assert(v.screen==PLUS_VITA_APP_CATALOGUE&&v.list_total==1);
    PlusVitaAppEffect square=plus_vita_app_dispatch(a,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_PAD,.pad={.buttons=PLUS_VITA_MENU_SQUARE,.left_x=128,.left_y=128}});
    entered(a,square,PLUS_VITA_RESULT_OK,"");
    v=plus_vita_app_view(a);assert(v.list_total==8&&v.platform_filter==2);
    plus_vita_app_dispatch(a,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_PAD_RESET,.pad={.left_x=128,.left_y=128}});
    plus_vita_app_dispatch(a,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_PAD,.pad={.buttons=PLUS_VITA_MENU_TRIANGLE,.left_x=128,.left_y=128}});
    v=plus_vita_app_view(a);assert(v.platform_filter==3&&v.list_total==1&&v.detail&&!strcmp(v.detail->platform,"PS5"));
    bool experimental=false;
    for(size_t i=0;i<v.action_count;i++)if(v.actions[i].id==PLUS_VITA_ACTION_PLAY)
        experimental=!strcmp(v.actions[i].label,"Experimental test");
    assert(experimental);
    plus_vita_app_destroy(a);
    puts("Catalogue search, combined filters, UTF-8, clear, cancel, focus and shortcuts passed");
    return 0;
}
