// SPDX-License-Identifier: AGPL-3.0-only
#include "app.h"
#include <assert.h>
#include <string.h>
static PlusVitaAppEffect send(PlusVitaApp *app,PlusVitaAppEventKind kind){
 return plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=kind});
}
static PlusVitaAppEffect tap(PlusVitaApp *app,PlusVitaAppAction id){
 PlusVitaAppView v=plus_vita_app_view(app);
 for(size_t i=0;i<v.action_count;i++)if(v.actions[i].id==id)
  return plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_TOUCH,.x=v.actions[i].x+8,.y=v.actions[i].y+8});
 assert(!"missing action");return (PlusVitaAppEffect){0};
}
int main(void){
 PlusVitaApp *app=plus_vita_app_create();assert(app);
 PlusVitaAppEffect load=send(app,PLUS_VITA_EVENT_START);
 const char json[]="{\"npsso\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\"}";
 PlusVitaAppEffect auth=plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_ACCOUNT_LOADED,.operation=load.operation,.data=json,.data_size=sizeof(json)-1});
 PlusVitaCatalogEntry entries[]={
 {.name="Alpha",.id="SYNTHETIC3",.platform="PS3",.launch_id_valid=true},
 {.name="Beta",.id="SYNTHETIC4",.platform="ps4",.launch_id_valid=true},
 {.name="Gamma",.id="SYNTHETIC5",.platform="PS5",.launch_id_valid=true,.ps5_stream_id="synthetic-stream"},
 {.name="Unknown",.id="SYNTHETIC0"}};
 plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_AUTHORIZED,.operation=auth.operation,.entries=entries,.entry_count=4});
 PlusVitaAppView v=plus_vita_app_view(app);
 assert(v.screen==PLUS_VITA_APP_CATALOGUE && v.list_total==4 && v.platform_filter==0);
 assert(v.actions[v.list_count].id==PLUS_VITA_ACTION_SEARCH);
 assert(v.action_count==v.list_count+7);
 tap(app,PLUS_VITA_ACTION_FILTER_PS3);v=plus_vita_app_view(app);
 assert(v.list_total==1 && v.platform_filter==1 && !strcmp(v.detail->name,"Alpha"));
 send(app,PLUS_VITA_EVENT_PLATFORM_FILTER);v=plus_vita_app_view(app);
 assert(v.list_total==1 && v.platform_filter==2 && !strcmp(v.detail->name,"Beta"));
 send(app,PLUS_VITA_EVENT_PLATFORM_FILTER);v=plus_vita_app_view(app);
 assert(v.list_total==1 && v.platform_filter==3 && !strcmp(v.detail->name,"Gamma"));
 PlusVitaAppEffect play=send(app,PLUS_VITA_EVENT_ACTIVATE);
 assert(play.kind==PLUS_VITA_EFFECT_PLAY && !strcmp(play.game.id,"SYNTHETIC5") && !strcmp(play.game.ps5_stream_id,"synthetic-stream"));
 plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_PLAY_ENDED,.operation=play.operation,.status=PLUS_VITA_RESULT_OK});
 v=plus_vita_app_view(app);assert(v.platform_filter==3 && v.list_total==1);
 tap(app,PLUS_VITA_ACTION_FILTER_ALL);v=plus_vita_app_view(app);
 assert(v.platform_filter==0 && v.list_total==4);
 plus_vita_app_destroy(app);
 return 0;
}
