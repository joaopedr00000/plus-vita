// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/app/app.h"
#include "../src/platform/cloud_experimental.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static PlusVitaAppEffect click(PlusVitaApp *app,PlusVitaAppAction action){
    PlusVitaAppView v=plus_vita_app_view(app);
    for(size_t j=0;j<v.action_count;j++)if(v.actions[j].id==action)
        return plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_TOUCH,.x=v.actions[j].x+12,.y=v.actions[j].y+10});
    assert(0 && "Action missing");return (PlusVitaAppEffect){0};
}
static void ended(PlusVitaApp *app,PlusVitaAppEffect e,PlusVitaAppResult result,const char *json){
    assert(e.kind==PLUS_VITA_EFFECT_PLAY);
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_PLAY_ENDED,.operation=e.operation,.status=result,.data=json});
}
static PlusVitaApp *connected(void){
    PlusVitaApp *app=plus_vita_app_create();assert(app);
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_STREAM_SETTINGS,.stream_profile=10000,.ui_language=2});
    PlusVitaAppEffect load=plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_START});
    const char json[]="{\"npsso\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\"}";
    PlusVitaAppEffect auth=plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_ACCOUNT_LOADED,.operation=load.operation,.data=json,.data_size=sizeof(json)-1});
    PlusVitaCatalogEntry entry={.name="Synthetic PS5",.id="FAKEPPSA1",.platform="PS5",.ps5_stream_id="FAKE-ENTITLEMENT",.launch_id_valid=true};
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_AUTHORIZED,.operation=auth.operation,.entries=&entry,.entry_count=1,.store_country="CH",.store_language="de",.language="de-CH"});
    return app;
}
int main(void){
    PlusVitaApp *app=connected();
    PlusVitaAppEffect discover=click(app,PLUS_VITA_ACTION_GAME);
    assert(!strcmp(discover.datacenter,PLUS_VITA_DC_DISCOVER));
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_PLAY_ENDED,.operation=discover.operation,
        .status=PLUS_VITA_RESULT_SERVICE_ERROR,.stream_diagnostic={.valid=true,.phase=7,.http=403,.reason=1,.native_error=1}});
    PlusVitaAppView v=plus_vita_app_view(app);
    assert(v.screen==PLUS_VITA_APP_DETAILS && strstr(v.body,"Falha antes"));
    assert(strstr(v.body,"Locking Session (7/11)")&&strstr(v.body,"HTTP: 403")&&strstr(v.body,"Service: 002.0011")&&strstr(v.body,"Portal / H.264"));
    discover=click(app,PLUS_VITA_ACTION_PLAY);
    ended(app,discover,PLUS_VITA_RESULT_DATACENTERS,"[]");
    v=plus_vita_app_view(app);assert(v.screen==PLUS_VITA_APP_DETAILS && strstr(v.body,"não retornou"));
    discover=click(app,PLUS_VITA_ACTION_PLAY);
    ended(app,discover,PLUS_VITA_RESULT_DATACENTERS,"[{\"dataCenter\":\"test-a\"},{\"dataCenter\":\"test-b\"}]");
    v=plus_vita_app_view(app);assert(v.screen==PLUS_VITA_APP_DATACENTERS && !strcmp(v.actions[0].label,"test-a"));
    PlusVitaAppEffect e=click(app,PLUS_VITA_ACTION_DATACENTER);
    assert(!strcmp(e.datacenter,"test-a") && e.bitrate_kbps==10000);
    ended(app,e,PLUS_VITA_RESULT_SERVICE_ERROR,"[{\"dataCenter\":\"test-b\"}]");
    v=plus_vita_app_view(app);assert(v.screen==PLUS_VITA_APP_DATACENTERS && strstr(v.body,"Falha no servidor") && !strcmp(v.actions[0].label,"test-b"));
    e=click(app,PLUS_VITA_ACTION_DATACENTER);
    assert(!strcmp(e.datacenter,"test-b"));
    ended(app,e,PLUS_VITA_RESULT_NETWORK_ERROR,NULL);
    assert(plus_vita_app_view(app).screen==PLUS_VITA_APP_DATACENTERS);
    click(app,PLUS_VITA_ACTION_BACK);
    discover=click(app,PLUS_VITA_ACTION_PLAY);
    ended(app,discover,PLUS_VITA_RESULT_SERVICE_ERROR,NULL);
    assert(plus_vita_app_view(app).screen==PLUS_VITA_APP_DETAILS);
    assert(!strstr(plus_vita_app_view(app).body,"HTTP: 403"));
    plus_vita_app_destroy(app);
    puts("PS5 discovery, empty list, retry and manual selection regression passed");return 0;
}
