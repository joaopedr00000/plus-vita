// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/app/app.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static PlusVitaAppEffect click(PlusVitaApp *app,PlusVitaAppAction action){
    PlusVitaAppView v=plus_vita_app_view(app);
    for(size_t j=0;j<v.action_count;j++)if(v.actions[j].id==action)
        return plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_TOUCH,.x=v.actions[j].x+12,.y=v.actions[j].y+12});
    assert(0);return (PlusVitaAppEffect){0};
}
int main(void){
    PlusVitaApp *app=plus_vita_app_create();assert(app);
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_STREAM_SETTINGS,.stream_profile=10000,.ui_language=2});
    PlusVitaAppEffect load=plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_START});
    assert(load.kind==PLUS_VITA_EFFECT_LOAD_ACCOUNT);
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_ACCOUNT_LOADED,.operation=load.operation,.status=PLUS_VITA_RESULT_VAULT_LOCKED,.vault_present=true});
    PlusVitaAppView v=plus_vita_app_view(app);assert(v.screen==PLUS_VITA_APP_SECURITY&&strstr(v.body,"Senha incorreta"));
    load=click(app,PLUS_VITA_ACTION_RESTORE);assert(load.kind==PLUS_VITA_EFFECT_LOAD_ACCOUNT);
    plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_ACCOUNT_LOADED,.operation=load.operation,.status=PLUS_VITA_RESULT_CANCELLED,.vault_present=true});
    assert(plus_vita_app_view(app).screen==PLUS_VITA_APP_SECURITY);
    click(app,PLUS_VITA_ACTION_BACK);assert(plus_vita_app_view(app).screen==PLUS_VITA_APP_SETTINGS);
    for(unsigned j=0;j<7;j++)plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_DOWN});
    v=plus_vita_app_view(app);assert(v.actions[v.selected].id==PLUS_VITA_ACTION_SECURITY);
    click(app,PLUS_VITA_ACTION_SECURITY);assert(plus_vita_app_view(app).screen==PLUS_VITA_APP_SECURITY);
    plus_vita_app_destroy(app);puts("Localized vault menu, locked account, retry and cancel navigation passed");
}
