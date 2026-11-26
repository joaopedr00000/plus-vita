// SPDX-License-Identifier: AGPL-3.0-only
#include "app.h"
#include "utf8.h"
#include "../platform/build_identity.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <json_object.h>
#include <json_tokener.h>
#include "../platform/cloud_experimental.h"
#include "../platform/region_policy.h"
#if PLUS_VITA_ADAPTIVE_STREAM
#include "i18n.h"
#include "catalog_filter.h"
#include "custom_theme.h"
#define PV_TR(s) pv_translate(app->ui_language,(s))
#else
#define PV_TR(s) (s)
#endif

#ifndef PLUS_VITA_APP_VERSION
#define PLUS_VITA_APP_VERSION "v000"
#endif

struct plus_vita_app_t {
	PlusVitaAppView view;
	size_t welcome_selection;
	uint64_t next_operation, pending_operation;
	uint64_t account_epoch;
	unsigned artwork_metadata[PLUS_VITA_ARTWORK_METADATA_STATES];
	PlusVitaAppEffectKind waiting_for;
	PlusVitaCredential credential;
	PlusVitaCatalogEntry *catalogue;
	size_t catalogue_count, catalogue_selection;
	size_t matches[4096], matches_count;
	bool search_focus;
	unsigned platform_filter;
	bool alternative_font;
	unsigned comparison_page;
	char search[129], search_button[192], filter_button[160], catalogue_status[512], detail_status[512];
	bool imported_valid, connected;
	bool log_available;
	bool account_from_settings;
	bool remember, saved, started, retry_account, retry_save, retry_delete, forgetting, expired;
	bool vault_present, vault_legacy;
	unsigned vault_mode;
	PlusVitaAppScreen storage_return;
	char label[129], online_id[129], account_status[512];
	char store_country[8], store_language[8], language[8];
	char settings_status[256], reporting_status[512], auth_status[256];
	char streaming_status[256];
	char stream_diag_text[256],stream_diag_body[768];
	uint32_t previous_buttons;
	int previous_direction;
	uint64_t repeat_at;
	PlusVitaControlLayout controls, controls_draft;
	PlusVitaControlDevice control_device;
	unsigned binding_input;
	size_t binding_focus, binding_first, action_focus, action_first, catalogue_first;
	char controls_status[256], binding_labels[5][256];
	char controls_help[2048];
#if PLUS_VITA_PS5_EXPERIMENTAL
	char datacenters[64][128], dc_status[512], bitrate_label[96];
	const char *dc_notice;
	size_t dc_count, dc_page;
	unsigned bitrate_kbps;
	bool hud;
	bool auto_retry;
	char selected_dc[128];
	unsigned requested_kbps;
#endif
	unsigned region_mode,ui_language;
	bool regional_catalogue;
	char locale_label[128];
	uint32_t theme_primary,theme_secondary;
	bool theme_custom;
	char theme_labels[2][128];
	PvUpdateReport update;
	unsigned update_note_page;
	char update_body[768];
};

static void welcome(PlusVitaApp *app)
{
	app->view = (PlusVitaAppView){
		.screen = PLUS_VITA_APP_WELCOME,
		.title = "VitaPlus",
		.subtitle = "WELCOME TO VITAPLUS",
		.body = "Play your cloud games on the handheld you love.\nConnect your own Sony account to get started.",
		.hint = "Left stick  Move     X  Select     O  Back",
		.actions = {
			{ PLUS_VITA_ACTION_SETUP, "Get started", 64, 300, 500, 44 },
			{ PLUS_VITA_ACTION_SETTINGS, "Settings and help", 64, 350, 500, 44 },
			{ PLUS_VITA_ACTION_ABOUT, "About VitaPlus", 64, 400, 500, 44 },
			{ PLUS_VITA_ACTION_EXIT, "Exit", 64, 450, 500, 44 }
		},
		.action_count = 4, .selected = app->welcome_selection
	};
	if(app->connected)
	{
		app->view.subtitle = "YOUR ACCOUNT IS CONNECTED";
		app->view.body = app->saved ? "Your sign-in is remembered on this Vita.\nOpen your cloud catalogue and keep browsing."
			: "Open your cloud catalogue and keep browsing.\nYour sign-in stays connected until you exit the app.";
		app->view.actions[0].id = PLUS_VITA_ACTION_CATALOGUE;
		app->view.actions[0].label = "Open catalogue";
	}
	else if(app->saved)
	{
		app->view.body = "Your saved sign-in is available.\nReconnect when you are ready.";
		app->view.actions[0].id = PLUS_VITA_ACTION_RESTORE;
		app->view.actions[0].label = "Reconnect saved account";
	}
	if(app->connected || app->saved)
	{
		app->view.actions[0].y = 264;
		app->view.actions[1] = (PlusVitaAppButton){ PLUS_VITA_ACTION_ACCOUNT, "Account", 64, 310, 500, 40 };
		app->view.actions[2] = (PlusVitaAppButton){ PLUS_VITA_ACTION_SETTINGS, "Settings and help", 64, 356, 500, 40 };
		app->view.actions[3] = (PlusVitaAppButton){ PLUS_VITA_ACTION_ABOUT, "About VitaPlus", 64, 402, 500, 40 };
		app->view.actions[4] = (PlusVitaAppButton){ PLUS_VITA_ACTION_EXIT, "Exit", 64, 448, 500, 40 };
		app->view.actions[0].height = 40;
		app->view.action_count = 5;
		if(app->view.selected >= 5) app->view.selected = 0;
	}
#if PLUS_VITA_ADAPTIVE_STREAM
    if(app->update.state==PV_UP_AVAILABLE)
        for(size_t j=0;j<app->view.action_count;j++)if(app->view.actions[j].id==PLUS_VITA_ACTION_SETTINGS)app->view.actions[j].label="Updates available - Settings";
#endif
}

static void settings_view(PlusVitaApp *app)
{
	snprintf(app->settings_status, sizeof(app->settings_status), "Version %s.\nSession log: %s.\nYour sign-in files stay on this Vita.",
		PLUS_VITA_APP_VERSION, app->log_available ? "available at " PLUS_VITA_DIAGNOSTIC_VITA_PATH : "unavailable");
	app->view = (PlusVitaAppView){ .screen = PLUS_VITA_APP_SETTINGS,
		.title = "Settings and help.", .subtitle = "VITAPLUS", .body = app->settings_status,
		.hint = "Left stick  Move     X  Select     O  Back",
		.actions = {
			{ app->connected || app->saved ? PLUS_VITA_ACTION_ACCOUNT : PLUS_VITA_ACTION_SETUP,
				app->connected || app->saved ? "Account and Remember" : "Account setup", 64, 300, 832, 44 },
			{ PLUS_VITA_ACTION_CONTROLS, "Controls", 64, 350, 832, 44 },
			{ PLUS_VITA_ACTION_REPORTING, "Report an issue", 64, 400, 832, 44 },
			{ PLUS_VITA_ACTION_CONTROL_LAYOUT, "Control mapping", 64, 406, 832, 40 },
			{ PLUS_VITA_ACTION_BACK, "Back", 64, 450, 832, 40 }
		}, .action_count = 5 };
	for(size_t i=0;i<3;i++) { app->view.actions[i].y=274+(int)i*44;app->view.actions[i].height=40; }
#if PLUS_VITA_ADAPTIVE_STREAM
	app->view.actions[5]=app->view.actions[4];
	app->view.actions[4]=(PlusVitaAppButton){PLUS_VITA_ACTION_REGION,pv_region_label(app->region_mode),64,0,832,36};
	app->view.action_count=7;
	app->view.actions[7]=app->view.actions[5];
	app->view.actions[6]=(PlusVitaAppButton){PLUS_VITA_ACTION_STREAM_SETTINGS,"Streaming",64,0,832,34};
	app->view.action_count=8;
#if PLUS_VITA_ACCOUNT_VAULT
	app->view.actions[8]=app->view.actions[7];
	app->view.actions[7]=(PlusVitaAppButton){PLUS_VITA_ACTION_SECURITY,"Security and privacy",64,0,832,34};
	app->view.action_count=9;
#endif
	snprintf(app->locale_label,sizeof(app->locale_label),"%s: %s",pv_translate(app->ui_language,"Language"),pv_locale_names[pv_language(app->ui_language)]);
	app->view.actions[5]=(PlusVitaAppButton){PLUS_VITA_ACTION_LANGUAGE,app->locale_label,64,0,832,34};
 	app->view.actions[app->view.action_count]=app->view.actions[app->view.action_count-1];
	app->view.actions[app->view.action_count-1]=(PlusVitaAppButton){PLUS_VITA_ACTION_THEME,"Appearance",64,0,832,34};
	app->view.action_count++;
	app->view.actions[app->view.action_count]=app->view.actions[app->view.action_count-1];
	app->view.actions[app->view.action_count-1]=(PlusVitaAppButton){PLUS_VITA_ACTION_UPDATES,app->update.state==PV_UP_AVAILABLE?"Updates available":"Updates",64,0,832,34};
	app->view.action_count++;
	for(size_t i=0;i<app->view.action_count;i++){app->view.actions[i].y=174+(int)i*39;app->view.actions[i].height=34;}
	app->view.body="Regional discovery applies at next sign-in using your authenticated account region only. Cloud eligibility remains server controlled.";
#endif
}

#if PLUS_VITA_ADAPTIVE_STREAM
static bool update_busy(PvUpdateState state){return state==PV_UP_CHECKING||state==PV_UP_DOWNLOADING||state==PV_UP_INSTALLING;}
static void update_view(PlusVitaApp *app){
    PvUpdateReport *q=&app->update;
    static const char *const status[]={"Check for updates.","Checking GitHub releases...","Update available","No newer version in this channel.","Downloading update...","Verified VPK ready to install.","Installing update. Do not turn off the console.","Update installed. Returning to LiveArea.","Update failed. Check Wi-Fi or free space. A verified VPK remains in Downloads if installation failed.","Update cancelled."};
    unsigned st=(unsigned)q->state;
    if(st>=sizeof(status)/sizeof(status[0]))st=0;
    char notes[241]={0};
    size_t len=strlen(q->notes),offset=0;
    for(unsigned page=0;page<app->update_note_page&&offset<len;page++){
        size_t n=len-offset;if(n>220)n=220;
        while(n&&offset+n<len&&((unsigned char)q->notes[offset+n]&0xc0)==0x80)n--;
        if(!n)break;
        offset+=n;
    }
    if(offset<len){
        size_t n=len-offset;if(n>220)n=220;
        while(n&&offset+n<len&&((unsigned char)q->notes[offset+n]&0xc0)==0x80)n--;
        memcpy(notes,q->notes+offset,n);
    }
    snprintf(app->update_body,sizeof(app->update_body),"%s %s\n%s%s%s",PV_TR("Installed:"),PLUS_VITA_APP_VERSION,PV_TR(status[st]),q->version[0]?" ":"",q->version);
    size_t used=strlen(app->update_body);
    if(!update_busy(q->state)&&notes[0])snprintf(app->update_body+used,sizeof(app->update_body)-used,"\n%s",notes);
    app->view=(PlusVitaAppView){.screen=PLUS_VITA_APP_UPDATES,.title="Updates",.subtitle="SETTINGS AND HELP",.body=app->update_body,.hint="X  Select     O  Back"};
    if(update_busy(q->state)){
        app->view.hint=q->state==PV_UP_INSTALLING?"Please wait":"O  Cancel";
        if(q->state!=PV_UP_INSTALLING)app->view.actions[app->view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_UPDATE_CANCEL,"Cancel",64,440,832,42};
        return;
    }
    app->view.actions[app->view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_UPDATE_CHANNEL,q->beta?"Channel: Beta":"Channel: Stable",64,0,832,34};
    app->view.actions[app->view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_UPDATE_AUTO,q->automatic?"Automatic checks: On":"Automatic checks: Off",64,0,832,34};
    app->view.actions[app->view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_UPDATE_CHECK,"Check for updates",64,0,832,34};
    if(q->state==PV_UP_AVAILABLE)app->view.actions[app->view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_UPDATE_DOWNLOAD,"Download update",64,0,832,34};
    if(q->state==PV_UP_READY)app->view.actions[app->view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_UPDATE_INSTALL,"Install verified update",64,0,832,34};
    if(len>220)app->view.actions[app->view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_UPDATE_NOTES,"Next release notes page",64,0,832,34};
    app->view.actions[app->view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_BACK,"Back",64,0,832,34};
    int first=490-(int)app->view.action_count*40;
    for(size_t j=0;j<app->view.action_count;j++)app->view.actions[j].y=first+(int)j*40;
}
static PlusVitaAppEffect update_effect(PlusVitaApp *app,PvUpdateCommand command){
    app->view.operation=app->pending_operation=++app->next_operation;
    return (PlusVitaAppEffect){.kind=PLUS_VITA_EFFECT_UPDATE,.operation=app->pending_operation,.update_command=command,.ui_language=app->ui_language};
}
static void theme_view(PlusVitaApp *app,const char *notice)
{
    snprintf(app->theme_labels[0],sizeof(app->theme_labels[0]),"%s: #%06X",PV_TR("Primary color"),(unsigned)app->theme_primary);
    snprintf(app->theme_labels[1],sizeof(app->theme_labels[1]),"%s: #%06X",PV_TR("Secondary color"),(unsigned)app->theme_secondary);
    app->view=(PlusVitaAppView){.screen=PLUS_VITA_APP_THEME,.title="Appearance",.subtitle="SETTINGS AND HELP",
        .body=notice?notice:"Choose any RGB color using six hexadecimal digits (RRGGBB).",
        .hint="X  Select     O  Back",
        .actions={{PLUS_VITA_ACTION_PRIMARY,app->theme_labels[0],64,208,832,48},
            {PLUS_VITA_ACTION_SECONDARY,app->theme_labels[1],64,266,832,48},
            {PLUS_VITA_ACTION_THEME_RESET,"Restore original theme",64,324,832,48},
            {PLUS_VITA_ACTION_BACK,"Back to settings",64,382,832,48}},.action_count=4};
}
static void stream_settings_view(PlusVitaApp *app)
{
	unsigned profile=pv_profile_cap(app->bitrate_kbps);
	const char *name=profile==6000?"Low latency":profile==10000?"Balanced":"Maximum quality";
	snprintf(app->streaming_status,sizeof(app->streaming_status),PV_TR("Active: %s (%u Mbps).\nApplies to the next stream; actual bitrate can adapt."),PV_TR(name),profile/1000);
	app->view=(PlusVitaAppView){.screen=PLUS_VITA_APP_STREAM_SETTINGS,
		.title="Streaming",.subtitle="SETTINGS AND HELP",.body=app->streaming_status,
		.hint="X  Select     O  Back",
		.actions={{PLUS_VITA_ACTION_STREAM_LOW,"Low latency - 6 Mbps",64,274,832,40},
			{PLUS_VITA_ACTION_STREAM_BALANCED,"Balanced - 10 Mbps",64,318,832,40},
			{PLUS_VITA_ACTION_STREAM_MAX,"Maximum quality - 15 Mbps",64,362,832,40},
			{PLUS_VITA_ACTION_BACK,"Back to settings",64,406,832,40}},
		.action_count=4,.selected=profile==6000?0:profile==10000?1:2};
}
#endif

#if PLUS_VITA_ACCOUNT_VAULT
static void security_view(PlusVitaApp *app,const char *notice)
{
	app->view=(PlusVitaAppView){.screen=PLUS_VITA_APP_SECURITY,.title="Security and privacy",
		.subtitle="ACCOUNT",.body=notice?notice:app->vault_mode==2?"Saved token protected by AES-256-GCM. Use a strong local password.":"Automatic unlock: encrypted token, separate local key. Full console access can expose both.",
		.hint="X  Select     O  Back",
		.actions={{PLUS_VITA_ACTION_VAULT_SAVE,"Protect saved token",64,258,832,40},
			{PLUS_VITA_ACTION_VAULT_TOGGLE,app->vault_mode==2?"Startup password: On (turn off)":"Startup password: Off (turn on)",64,302,832,40},
			{PLUS_VITA_ACTION_VAULT_CLEAN_IMPORT,"Encrypt and remove imported JSON",64,346,832,40},
			{PLUS_VITA_ACTION_FORGET,"Forget account",64,390,832,40},
			{PLUS_VITA_ACTION_BACK,"Back to settings",64,434,832,40}},.action_count=5};
	if(app->connected&&app->vault_mode==2){
		app->view.actions[5]=app->view.actions[4];app->view.actions[4]=app->view.actions[3];
		app->view.actions[3]=app->view.actions[2];
		app->view.actions[2]=(PlusVitaAppButton){PLUS_VITA_ACTION_VAULT_CHANGE,"Change local password",64,0,832,40};
		app->view.action_count=6;
		for(size_t j=0;j<6;j++){app->view.actions[j].y=246+(int)j*42;app->view.actions[j].height=36;}
	}
	if(!app->connected){
		app->view.actions[0]=(PlusVitaAppButton){PLUS_VITA_ACTION_RESTORE,"Restore saved account",64,258,832,40};
		app->view.actions[1]=app->view.actions[3];app->view.actions[1].y=302;
		app->view.actions[2]=app->view.actions[4];app->view.actions[2].y=346;app->view.action_count=3;
	}
}
#endif

static bool controls_screen(PlusVitaAppScreen screen)
{
	return screen==PLUS_VITA_APP_CONTROL_LAYOUT || screen==PLUS_VITA_APP_BINDINGS
		|| screen==PLUS_VITA_APP_BIND_ACTIONS;
}

static void layout_view(PlusVitaApp *app)
{
	app->view=(PlusVitaAppView){.screen=PLUS_VITA_APP_CONTROL_LAYOUT,.title="Control mapping.",
		.subtitle="GAMEPLAY LAYOUT",.body=app->controls_status,.hint="D-pad / left stick  Move     X  Select     O  Cancel",
		.actions={{PLUS_VITA_ACTION_BINDINGS,"Edit input bindings",64,260,832,40},
			{PLUS_VITA_ACTION_FRONT_MOTION,app->controls_draft.front_motion ? "Front-touch movement: On" : "Front-touch movement: Off",64,304,832,40},
			{PLUS_VITA_ACTION_CONTROL_DEFAULTS,"Restore defaults",64,348,832,40},
			{PLUS_VITA_ACTION_SAVE_CONTROLS,"Save for next session",64,392,832,40},
			{PLUS_VITA_ACTION_CANCEL_CONTROLS,"Cancel",64,436,832,40}},.action_count=5};
}

/* Lists prepare only visible rows. The same viewport rule serves mappings and games. */
static void list_viewport(size_t total,size_t capacity,size_t focus,size_t *first)
{
	if(total<=capacity) *first=0;
	else {
		if(*first>total-capacity) *first=total-capacity;
		if(focus<*first) *first=focus;
		if(focus>=*first+capacity) *first=focus-capacity+1;
	}
}
static void move_list(size_t total,size_t visible,size_t *focus,size_t *first,PlusVitaAppEvent event)
{
	if(!total) return;
	if(event.kind==PLUS_VITA_EVENT_SCROLL) {
		int next=(int)*first+event.y, maximum=(int)total-(int)visible;
		*first=(size_t)(next<0 ? 0 : next>maximum ? maximum : next);
		if(*focus<*first) *focus=*first;
		if(*focus>=*first+visible) *focus=*first+visible-1;
	} else if(event.kind==PLUS_VITA_EVENT_FOCUS) {
		if(event.y>=0 && (size_t)event.y<total) *focus=(size_t)event.y;
	} else if(event.kind==PLUS_VITA_EVENT_DOWN && *focus+1<total) ++*focus;
	else if(event.kind==PLUS_VITA_EVENT_UP && *focus) --*focus;
	list_viewport(total,visible,*focus,first);
}

static void bindings_view(PlusVitaApp *app,bool destinations)
{
	size_t count=destinations ? PLUS_VITA_CONTROL_COUNT : PLUS_VITA_INPUT_COUNT;
	size_t focus=destinations ? app->action_focus : app->binding_focus;
	size_t *first=destinations ? &app->action_first : &app->binding_first;
	list_viewport(count,5,focus,first);
	app->view=(PlusVitaAppView){.screen=destinations ? PLUS_VITA_APP_BIND_ACTIONS : PLUS_VITA_APP_BINDINGS,
		.control_device=app->control_device,.list_total=count,.list_first=*first,.focus=focus,
		.list_x=64,.list_y=248,.list_width=812,.list_height=220,.row_step=44,
		.title=destinations ? plus_vita_control_input_name(app->binding_input,app->control_device) : "Choose an input.",
		.subtitle=destinations ? "CHOOSE GAME ACTION" : "GAMEPLAY BINDINGS",
		.body=destinations ? "One action per input. Several inputs may share an action."
			: app->control_device==PLUS_VITA_DEVICE_VITA
			? "Vita rear touch: top L2/R2; bottom L3/R3 by default."
			: app->control_device==PLUS_VITA_DEVICE_DISCONNECTED
			? "No controller connected. All bindings remain editable."
			: "PS TV buttons and Vita rear-touch zones are separate inputs.",
		.hint="D-pad / left stick  Move     X  Select     O  Back",.selected=focus-*first};
	for(unsigned row=0;row<5 && *first+row<count;row++) {
		unsigned index=(unsigned)*first+row;
		if(destinations) snprintf(app->binding_labels[row],sizeof(app->binding_labels[row]),"%s",PV_TR(plus_vita_control_action_name(index)));
		else snprintf(app->binding_labels[row],sizeof(app->binding_labels[row]),"%s: %s%s",
			PV_TR(plus_vita_control_input_name(index,app->control_device)),PV_TR(plus_vita_control_action_name(app->controls_draft.bindings[index])),
			PV_TR(plus_vita_control_input_available(index,app->control_device) ? ""
			: app->control_device==PLUS_VITA_DEVICE_VITA ? " (PS TV only)"
			: app->control_device==PLUS_VITA_DEVICE_DISCONNECTED ? " (no controller)" : " (Vita only)"));
		app->view.actions[app->view.action_count++]=(PlusVitaAppButton){
			destinations ? PLUS_VITA_ACTION_BIND_ACTION : PLUS_VITA_ACTION_BINDING,
			app->binding_labels[row],64,248+(int)row*44,812,40};
	}
	app->view.list_count=app->view.action_count;
	app->view.actions[app->view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_BACK,"Back",64,470,812,32};
}

static PlusVitaAppEffect edit_controls(PlusVitaApp *app,PlusVitaAppAction action)
{
	bool destinations=app->view.screen==PLUS_VITA_APP_BIND_ACTIONS;
	if(action==PLUS_VITA_ACTION_CONTROL_LAYOUT) {
		app->controls_draft=app->controls;app->binding_focus=app->binding_first=0;
		snprintf(app->controls_status,sizeof(app->controls_status),"Save for next session. Menu controls and exit stay fixed.");
		layout_view(app);
	} else if(action==PLUS_VITA_ACTION_BINDINGS) bindings_view(app,false);
	else if(action==PLUS_VITA_ACTION_BINDING) {
		app->binding_input=(unsigned)app->binding_focus;
		app->action_focus=app->action_first=0;bindings_view(app,true);
	} else if(action==PLUS_VITA_ACTION_BIND_ACTION) {
		app->controls_draft.bindings[app->binding_input]=(uint8_t)app->action_focus;
		bindings_view(app,false);
	} else if(action==PLUS_VITA_ACTION_FRONT_MOTION) {
		app->controls_draft.front_motion=!app->controls_draft.front_motion;layout_view(app);app->view.selected=1;
	} else if(action==PLUS_VITA_ACTION_CONTROL_DEFAULTS) {
		plus_vita_control_layout_defaults(&app->controls_draft);layout_view(app);app->view.selected=2;
	} else if(action==PLUS_VITA_ACTION_SAVE_CONTROLS) {
		app->pending_operation=++app->next_operation;app->waiting_for=PLUS_VITA_EFFECT_SAVE_CONTROLS;
		app->view.operation=app->pending_operation;
		return (PlusVitaAppEffect){.kind=PLUS_VITA_EFFECT_SAVE_CONTROLS,.operation=app->pending_operation,.controls=app->controls_draft};
	} else if(action==PLUS_VITA_ACTION_CANCEL_CONTROLS) settings_view(app);
	else if(action==PLUS_VITA_ACTION_BACK) {
		if(destinations) bindings_view(app,false);
		else if(app->view.screen==PLUS_VITA_APP_BINDINGS) layout_view(app);
		else settings_view(app);
	}
	return (PlusVitaAppEffect){0};
}

static void controls_view(PlusVitaApp *app)
{
	app->view = (PlusVitaAppView){ .screen = PLUS_VITA_APP_CONTROLS,.control_device=app->control_device,
		.title = "Controls.", .subtitle = "SETTINGS AND HELP",
		.body = "Menus: D-pad or left stick; Cross selects, Circle goes back.\n"
			"Touch: lift inside a button; slide away to cancel.\n"
			"In a game: Vita buttons and sticks map to PlayStation controls.\n"
			"Rear top corners: L2/R2; lower corners: L3/R3.\n"
			"Front touch moves the touchpad; two touches press it.\n"
			"Settings > Control mapping: Save applies next session.\n"
			"Hold START + SELECT for 2 seconds to stop a stream.",
		.hint = "X  Select     O  Back",
		.actions = {{ PLUS_VITA_ACTION_BACK, "Back to settings", 64, 444, 832, 52 }}, .action_count = 1 };
	if(app->control_device==PLUS_VITA_DEVICE_DS3 || app->control_device==PLUS_VITA_DEVICE_DS4) {
		bool ds4=app->control_device==PLUS_VITA_DEVICE_DS4;
		snprintf(app->controls_help,sizeof(app->controls_help),
			PV_TR("%s: pair in PS TV system settings using USB + PS.\n"
			"Menus: D-pad/stick, Cross selects, Circle goes back.\n"
			"Physical buttons, sticks, L1/R1, L2/R2 and L3/R3 work in games.\n"
			"Control mapping: assign a button to cloud touchpad click.\n"
			"Touchpad movement, motion and rumble are unavailable.\n"
			"Hold %s for 2 seconds to stop.\n"
			"Reconnect, then release all controls before playing."),
			ds4 ? "DualShock 4" : "DualShock 3",ds4 ? "OPTIONS + SHARE" : "START + SELECT");
		app->view.body=app->controls_help;
	}
	else if(app->control_device==PLUS_VITA_DEVICE_DISCONNECTED)
		app->view.body="No supported controller connected.\nPair DualShock 3 or DualShock 4 in PS TV system settings\nusing USB, then press PS. Release controls after connecting.\nControl mapping can assign a button to cloud touchpad click.";
}

static void reporting_view(PlusVitaApp *app)
{
	snprintf(app->reporting_status, sizeof(app->reporting_status), "Log: %s\n"
		"Copy with VitaShell before restarting the app.\n"
		"Review it, then attach it yourself at:\n"
		"github.com/joaopedr00000/plus-vita/issues/new\n"
		"Include steps, expected/actual result and version %s.\n"
		"Never share sign-in JSON, account files, keys or private screenshots.",
		app->log_available ? PLUS_VITA_DIAGNOSTIC_VITA_PATH : "unavailable; report without a log",
		PLUS_VITA_APP_VERSION);
	app->view = (PlusVitaAppView){ .screen = PLUS_VITA_APP_REPORTING,
		.title = "Report an issue.", .subtitle = "SETTINGS AND HELP", .body = app->reporting_status,
		.hint = "X  Select     O  Back",
		.actions = {{ PLUS_VITA_ACTION_BACK, "Back to settings", 64, 444, 832, 52 }}, .action_count = 1 };
}

static void about_view(PlusVitaApp *app)
{
	app->view=(PlusVitaAppView){.screen=PLUS_VITA_APP_ABOUT,
		.title="Made for your handheld.",.subtitle="ABOUT VITAPLUS",
#if PLUS_VITA_ADAPTIVE_STREAM
		.body="VitaPlus was born from a young man's \"greed\": to play GTA VI via streaming on his PS Vita. This dream led him to fork Plus-Vita, adding new features and experimental PS5 cloud streaming. GTA VI compatibility is not yet verified.\nCredits: Calligula87, tertiumndatur, Pylux and Chiaki.",
#else
		.body="Use your own Sony account and eligible subscription.\n\n" PLUS_VITA_ABOUT_STATUS,
#endif
		.hint="X  Select     O  Back",.actions={{PLUS_VITA_ACTION_BACK,"Back",64,444,500,52}},.action_count=1};
#if PLUS_VITA_FONT_COMPARISON
	app->view.actions[0]=(PlusVitaAppButton){PLUS_VITA_ACTION_FONT_COMPARISON,"Compare text rendering",64,380,832,52};
	app->view.actions[1]=(PlusVitaAppButton){PLUS_VITA_ACTION_BACK,"Back",64,444,832,52}; app->view.action_count=2;
#endif
}

PlusVitaApp *plus_vita_app_create(void)
{
	PlusVitaApp *app = calloc(1, sizeof(*app));
	if(!app) return NULL;
	app->remember = true;
#if PLUS_VITA_ADAPTIVE_STREAM
	app->theme_primary=PV_PRIMARY_DEFAULT;app->theme_secondary=PV_SECONDARY_DEFAULT;
#endif
	plus_vita_control_layout_defaults(&app->controls);
	welcome(app);
	return app;
}

void plus_vita_app_destroy(PlusVitaApp *app)
{
	if(!app) return;
	plus_vita_credential_clear(&app->credential);
	free(app->catalogue);
	free(app);
}
PlusVitaAppView plus_vita_app_view(const PlusVitaApp *app) {
	PlusVitaAppView view=app->view; view.account_epoch=app->account_epoch;
	memcpy(view.artwork_metadata,app->artwork_metadata,sizeof(view.artwork_metadata));
	view.account_connected=app->connected;
#if PLUS_VITA_ADAPTIVE_STREAM
	view.stream_profile=pv_profile_cap(app->bitrate_kbps);view.stream_hud=app->hud;view.region_mode=app->region_mode;view.ui_language=app->ui_language;
	view.theme_primary=app->theme_primary;view.theme_secondary=app->theme_secondary;view.theme_custom=app->theme_custom;
	view.update_progress=app->update.progress;view.update_downloading=app->update.state==PV_UP_DOWNLOADING;
    if(view.screen==PLUS_VITA_APP_ACCOUNT||view.screen==PLUS_VITA_APP_SETUP){
        int step=view.action_count>6?44:62;
        for(size_t j=0;j<view.action_count;j++){
            view.actions[j].x=544;view.actions[j].y=93+(int)j*step;view.actions[j].width=398;view.actions[j].height=step-6;
        }
    }
#endif
	if(!view.list_total && view.action_count && !(PLUS_VITA_ADAPTIVE_STREAM && view.screen==PLUS_VITA_APP_CATALOGUE)) {
		view.list_total=view.action_count;view.focus=view.selected;
		view.list_x=view.actions[0].x;view.list_y=view.actions[0].y;
		view.list_width=view.actions[0].width;view.row_step=view.actions[0].height+6;
		size_t capacity=(size_t)((502-view.list_y+6)/view.row_step);
		if(!capacity) capacity=1;
		/* The diagnostic font comparison is a horizontal sample toolbar. */
		if(view.screen==PLUS_VITA_APP_FONT_COMPARISON) capacity=view.action_count;
		view.list_count=capacity<view.list_total ? capacity : view.list_total;
		list_viewport(view.list_total,view.list_count,view.focus,&view.list_first);
		if(view.list_count<view.list_total) {
			for(size_t i=0;i<view.list_count;i++) {
				view.actions[i]=app->view.actions[view.list_first+i];
				view.actions[i].y=view.list_y+(int)i*view.row_step;
			}
			view.action_count=view.list_count;view.selected=view.focus-view.list_first;
		}
		const PlusVitaAppButton *last=&view.actions[view.action_count-1];
		view.list_height=last->y+last->height-view.list_y;
	}
	#if PLUS_VITA_ADAPTIVE_STREAM
	if(view.screen!=PLUS_VITA_APP_DETAILS && ((view.screen!=PLUS_VITA_APP_ACCOUNT&&view.screen!=PLUS_VITA_APP_AUTHORIZING)||(!app->label[0]&&!app->online_id[0])))view.title=pv_translate(app->ui_language,view.title);
	view.subtitle=pv_translate(app->ui_language,view.subtitle);
	view.hint=pv_translate(app->ui_language,view.hint);
	view.body=pv_translate(app->ui_language,view.body);
	for(size_t i=0;i<view.action_count;i++)if(view.actions[i].id!=PLUS_VITA_ACTION_GAME&&view.actions[i].id!=PLUS_VITA_ACTION_DATACENTER)view.actions[i].label=pv_translate(app->ui_language,view.actions[i].label);
	#endif
	return view;
}

static void setup_view(PlusVitaApp *app)
{
	app->pending_operation = 0;
	app->waiting_for = PLUS_VITA_EFFECT_NONE;
	app->view = (PlusVitaAppView){
		.screen = PLUS_VITA_APP_SETUP,
		.title = "Import your sign-in JSON.", .subtitle = "CONNECT TO SONY",
		.body = "Save plus-vita-auth.json in ux0:data/VitaPlus/Imports with VitaShell.\nThe JSON stays there and can sign you in next launch.\nRemember Off pauses automatic sign-in; files are locally readable.",
		.hint = "X  Select     O  Back",
		.actions = {
			{ PLUS_VITA_ACTION_IMPORT, "Import JSON", 64, 316, 832, 52 },
			{ PLUS_VITA_ACTION_REMEMBER, app->remember ? "Remember this account: On" : "Remember this account: Off", 64, 380, 832, 52 },
			{ PLUS_VITA_ACTION_BACK, "Back", 64, 444, 832, 52 }
		}, .action_count = 3
	};
#if PLUS_VITA_LAN_LOGIN
	app->view.title="Connect your Sony account.";
	app->view.body="Use your phone on the same trusted Wi-Fi. Sign in on Sony, then paste the response on the local page. No downloads. Local HTTP is unencrypted.";
	app->view.actions[0]=(PlusVitaAppButton){PLUS_VITA_ACTION_LAN_LOGIN,"Sign in with QR code",64,0,832,44};
#elif PLUS_VITA_QR_PAIRING
	app->view.title="Connect your Sony account.";
	app->view.body="Sign in only on Sony. No password is requested by VitaPlus.\nChoose browser login, encrypted QR pairing, or JSON import.";
	app->view.actions[0].y=284;app->view.actions[0].height=48;
	app->view.actions[1].y=336;app->view.actions[1].height=48;
	app->view.actions[3]=app->view.actions[2];
	app->view.actions[2]=(PlusVitaAppButton){PLUS_VITA_ACTION_SONY_LOGIN,"Sign in on Sony (email / passkey)",64,0,832,40};
	app->view.actions[3]=(PlusVitaAppButton){PLUS_VITA_ACTION_TOKEN_LOGIN,"QR login / enter token (no files)",64,0,832,40};
	app->view.actions[4]=(PlusVitaAppButton){PLUS_VITA_ACTION_QR_PAIR,"Encrypted QR (offline helper)",64,0,832,40};
	app->view.actions[5]=(PlusVitaAppButton){PLUS_VITA_ACTION_REVERSE_QR,"Scan NPSSO QR (local / unencrypted)",64,0,832,34};
	app->view.actions[6]=(PlusVitaAppButton){PLUS_VITA_ACTION_BACK,"Back",64,0,832,34};
	app->view.action_count=7;
	for(size_t i=0;i<app->view.action_count;i++){app->view.actions[i].y=210+(int)i*36;app->view.actions[i].height=32;}
#endif
#if PLUS_VITA_ADAPTIVE_STREAM
	app->view.actions[app->view.action_count]=app->view.actions[app->view.action_count-1];
	app->view.actions[app->view.action_count-1]=(PlusVitaAppButton){PLUS_VITA_ACTION_REGION,pv_region_label(app->region_mode),64,0,832,40};
	app->view.action_count++;
	for(size_t i=0;i<app->view.action_count;i++){app->view.actions[i].y=210+(int)i*36;app->view.actions[i].height=32;}
#endif
}

static void account_error(PlusVitaApp *app, const char *message)
{
	app->pending_operation = 0;
	app->waiting_for = PLUS_VITA_EFFECT_NONE;
	app->view = (PlusVitaAppView){
		.screen = PLUS_VITA_APP_ERROR, .title = "Let's try that again.",
		.subtitle = "ACCOUNT SETUP", .body = message, .hint = "X  Select     O  Back",
		.actions = {{ PLUS_VITA_ACTION_BACK, "Back", 64, 444, 500, 52 }}, .action_count = 1
	};
	if(app->retry_account || app->retry_save || app->retry_delete)
	{
		app->view.actions[0] = (PlusVitaAppButton){ PLUS_VITA_ACTION_RETRY, app->retry_delete ? "Retry removing saved account" : "Retry", 64, 380, 832, 52 };
		app->view.actions[1] = (PlusVitaAppButton){ PLUS_VITA_ACTION_BACK, "Back", 64, 444, 832, 52 };
		app->view.action_count = 2;
	}
	else if(app->saved || app->forgetting)
	{
		app->view.actions[0] = (PlusVitaAppButton){ app->forgetting ? PLUS_VITA_ACTION_CONFIRM_FORGET : PLUS_VITA_ACTION_FORGET,
			app->forgetting ? "Retry removing account" : "Forget saved account", 64, 380, 832, 52 };
		app->view.actions[1] = (PlusVitaAppButton){ PLUS_VITA_ACTION_BACK, "Back", 64, 444, 832, 52 };
		app->view.action_count = 2;
	}
}

static const char *result_message(PlusVitaAppResult result)
{
	switch(result)
	{
		case PLUS_VITA_RESULT_MISSING:
			return "No sign-in JSON was found. Transfer plus-vita-auth.json\nto ux0:data/VitaPlus/Imports with VitaShell, then choose Import JSON.";
		case PLUS_VITA_RESULT_STORAGE_ERROR:
			return "The sign-in JSON could not be read. Check it with VitaShell and try again.";
		case PLUS_VITA_RESULT_REJECTED:
			#if PLUS_VITA_LAN_LOGIN
			return "Your sign-in was rejected or expired.\nUse Sign in with QR code again.";
#else
			return "Your sign-in was rejected or has expired.\nSign in to Sony again, save a fresh JSON response and import it.";
#endif
		case PLUS_VITA_RESULT_NETWORK_ERROR:
			return "Sony could not be reached. Check Wi-Fi and try again.\nYour account details have not been displayed or logged.";
		case PLUS_VITA_RESULT_CATALOGUE_ERROR:
			return "The cloud catalogue is unavailable for this account right now.\nCheck your eligible subscription and try again later.";
		case PLUS_VITA_RESULT_BUSY:
			return "The previous request is still finishing.\nReturn to setup and try again in a moment.";
		default:
			return "Account setup could not be completed. Please try again.";
	}
}

static const char *account_name(const PlusVitaApp *app)
{
	return app->label[0] ? app->label : app->online_id[0] ? app->online_id : "Your account.";
}

static PlusVitaAppEffect operation(PlusVitaApp *app, PlusVitaAppEffectKind kind, const char *message, bool cancel)
{
	if(kind==PLUS_VITA_EFFECT_PLAY||kind==PLUS_VITA_EFFECT_AUTHORIZE) app->stream_diag_text[0]=0;
	app->waiting_for = kind;
	app->pending_operation = ++app->next_operation;
	app->view = (PlusVitaAppView){ .screen = PLUS_VITA_APP_AUTHORIZING,
		.title = account_name(app), .subtitle = app->label[0] ? "ACCOUNT LABEL" : "ACCOUNT", .body = message,
		.hint = cancel ? "O  Cancel" : "Please wait", .operation = app->pending_operation };
	if(cancel) { app->view.actions[0] = (PlusVitaAppButton){ PLUS_VITA_ACTION_BACK, "Cancel", 64, 444, 832, 52 }; app->view.action_count = 1; }
	if(kind==PLUS_VITA_EFFECT_EDIT_SEARCH) { app->view.title="Search your games."; app->view.subtitle="CATALOGUE SEARCH"; }
	if(kind==PLUS_VITA_EFFECT_EDIT_PRIMARY||kind==PLUS_VITA_EFFECT_EDIT_SECONDARY){app->view.title="Appearance";app->view.subtitle="SETTINGS AND HELP";}
	PlusVitaAppEffect effect = { .kind = kind, .operation = app->pending_operation, .credential = app->credential, .forget = app->forgetting };
	memcpy(effect.label, app->label, sizeof(effect.label));
	memcpy(effect.store_country, app->store_country, sizeof(effect.store_country));
	memcpy(effect.store_language, app->store_language, sizeof(effect.store_language));
	memcpy(effect.language, app->language, sizeof(effect.language));
	effect.region_mode=app->region_mode;
	return effect;
}

static void json_space(const char **cursor, const char *end)
{
	while(*cursor < end && (**cursor == ' ' || **cursor == '\t' || **cursor == '\r' || **cursor == '\n')) ++*cursor;
}

static bool json_text_valid(const char *text, size_t size)
{
	const char *cursor = text;
	while(cursor < text + size) if(plus_vita_utf8_next(&cursor, text + size) == UINT32_MAX) return false;
	/* json-c's strict mode still accepts NaN/Infinity and raw string controls.
	 * These are not JSON. Check their lexical forms before decoding values. */
	bool quoted = false, escaped = false;
	for(size_t i = 0; i < size; i++)
	{
		unsigned char c = (unsigned char)text[i];
		if(c < 0x20 && (quoted || (c != '\t' && c != '\r' && c != '\n'))) return false;
		if(quoted)
		{
			if(escaped) escaped = false;
			else if(c == '\\') escaped = true;
			else if(c == '"') quoted = false;
		}
		else if(c == '"') quoted = true;
		else if(c == 'N' || c == 'I') return false;
	}
	return !quoted;
}

static bool json_value(struct json_tokener *parser, const char **cursor, const char *end, struct json_object **value)
{
	json_tokener_reset(parser);
	*value = json_tokener_parse_ex(parser, *cursor, (int)(end - *cursor));
	if(json_tokener_get_error(parser) != json_tokener_success) return false;
	size_t consumed = json_tokener_get_parse_end(parser);
	if(!consumed) return false;
	*cursor += consumed;
	return true;
}

static bool parse_import_ascii(PlusVitaCredential *credential, const char *data, size_t size, char *label)
{
	const char *p = data, *end = data + size;
	/* Accept a UTF-8 BOM emitted by some save-response tools. */
	if(size >= 3 && memcmp(p, "\xef\xbb\xbf", 3) == 0) p += 3;
	json_space(&p, end);
	if(p == end || *p++ != '{') return false;
	struct json_tokener *parser = json_tokener_new_ex(16);
	if(!parser) return false;
	/* UTF-8 is validated once above, independently of tokener versions and
	 * incremental parsing of individual root keys/values. */
	json_tokener_set_flags(parser, JSON_TOKENER_STRICT | JSON_TOKENER_ALLOW_TRAILING_CHARS);
	PlusVitaCredential parsed = { 0 };
	struct json_object *value = NULL;
	bool found = false, valid = false;
	bool label_found = false;
	char parsed_label[129] = { 0 };
	/* Parse root members individually: a normal object parse would silently
	 * overwrite duplicate npsso keys, including escaped spellings of the key.
	 * json-c still validates/decodes each complete key and nested value. */
	for(;;)
	{
		json_space(&p, end);
		if(p == end || *p != '"' || !json_value(parser, &p, end, &value)) goto done;
		bool npsso = json_object_is_type(value, json_type_string)
			&& json_object_get_string_len(value) == 5 && memcmp(json_object_get_string(value), "npsso", 5) == 0;
		bool label_key = label && json_object_is_type(value, json_type_string)
			&& json_object_get_string_len(value) == 5 && memcmp(json_object_get_string(value), "label", 5) == 0;
		json_object_put(value); value = NULL;
		json_space(&p, end);
		if(p == end || *p++ != ':') goto done;
		json_space(&p, end);
		if(!json_value(parser, &p, end, &value)) goto done;
		if(npsso)
		{
			if(found || !json_object_is_type(value, json_type_string) || json_object_get_string_len(value) != 64) goto done;
			const char *token = json_object_get_string(value);
			for(unsigned i = 0; i < 64; i++)
				if(!((token[i] >= 'A' && token[i] <= 'Z') || (token[i] >= 'a' && token[i] <= 'z')
					|| (token[i] >= '0' && token[i] <= '9'))) goto done;
			found = plus_vita_credential_set(&parsed, token, 64);
		}
		if(label_key)
		{
			if(label_found || !json_object_is_type(value, json_type_string) || json_object_get_string_len(value) > 128) goto done;
			int length = json_object_get_string_len(value);
			const char *name = json_object_get_string(value);
			if(!plus_vita_utf8_label(name, (size_t)length)) goto done;
			memcpy(parsed_label, name, (size_t)length);
			label_found = true;
		}
		json_object_put(value); value = NULL;
		json_space(&p, end);
		if(p == end) goto done;
		if(*p == '}') { p++; break; }
		if(*p++ != ',') goto done;
	}
	json_space(&p, end);
	valid = found && p == end;
	if(valid) *credential = parsed;
	if(valid && label) memcpy(label, parsed_label, sizeof(parsed_label));
done:
	json_object_put(value);
	json_tokener_free(parser);
	plus_vita_credential_clear(&parsed);
	return valid;
}

static bool parse_import(PlusVitaCredential *credential, const char *data, size_t size, char *label)
{
	if(!data || !size || size > 4096 || !json_text_valid(data, size)) return false;
	/* Normalize UTF-8 to JSON's UTF-16 escapes before incremental parsing.
	 * This keeps raw Unicode handling identical across host/system json-c versions. */
	char *normalized = malloc(size * 3 + 1);
	if(!normalized) return false;
	const char *p = data, *end = data + size;
	if(size >= 3 && memcmp(p, "\xef\xbb\xbf", 3) == 0) p += 3;
	char *out = normalized;
	while(p < end)
	{
		uint32_t code = plus_vita_utf8_next(&p, end);
		if(code < 0x80) *out++ = (char)code;
		else if(code <= 0xffff) { snprintf(out, 7, "\\u%04x", (unsigned)code); out += 6; }
		else { code -= 0x10000; snprintf(out, 13, "\\u%04x\\u%04x", 0xd800u + (unsigned)((code >> 10) & 1023), 0xdc00u + (unsigned)(code & 1023)); out += 12; }
	}
	*out = 0;
	bool valid = parse_import_ascii(credential, normalized, (size_t)(out - normalized), label);
	volatile char *wipe = normalized;
	for(size_t i = 0; i < size * 3 + 1; i++) wipe[i] = 0;
	free(normalized);
	return valid;
}

static int compare_titles(const void *left, const void *right)
{
	const unsigned char *a = (const unsigned char *)((const PlusVitaCatalogEntry *)left)->name;
	const unsigned char *b = (const unsigned char *)((const PlusVitaCatalogEntry *)right)->name;
	while(*a && tolower(*a) == tolower(*b)) { a++; b++; }
	return tolower(*a) - tolower(*b);
}

static void account_view(PlusVitaApp *app)
{
	snprintf(app->account_status, sizeof(app->account_status),
		#if PLUS_VITA_ACCOUNT_VAULT
		PV_TR("Remember: %s. See Security and privacy.\nAccount label (entered by you): %s\nSony account name: %s"),
#else
		PV_TR("Remember: %s. Local files are readable with file access.\nAccount label (entered by you): %s\nSony account name: %s"),
#endif
		PV_TR(app->saved ? "On" : "Off"), app->label[0] ? app->label : PV_TR("Not set"), app->online_id[0] ? app->online_id : PV_TR("Unavailable"));
	app->view = (PlusVitaAppView){ .screen = PLUS_VITA_APP_ACCOUNT, .title = account_name(app),
		.subtitle = "ACCOUNT", .body = app->account_status, .hint = "Left stick  Move     X  Select     O  Back",
		.actions = {
			{ PLUS_VITA_ACTION_REMEMBER, app->retry_delete ? "Retry removing saved account" : app->saved ? "Remember: On (turn off)" : "Remember: Off (turn on)", 64, 284, 832, 48 },
			{ PLUS_VITA_ACTION_LABEL, "Edit account label", 64, 340, 832, 48 },
			{ PLUS_VITA_ACTION_FORGET, "Forget account", 64, 396, 832, 48 },
			{ PLUS_VITA_ACTION_BACK, "Back", 64, 452, 832, 48 }
		}, .action_count = 4 };
	if(!app->connected)
	{
		app->view.actions[0] = app->view.actions[2];
		app->view.actions[0].y = 396;
		app->view.actions[1] = app->view.actions[3];
		app->view.action_count = 2;
	}
}

static void clear_runtime(PlusVitaApp *app)
{
	app->account_epoch++;
#if PLUS_VITA_PS5_EXPERIMENTAL
	app->dc_count=app->dc_page=0;app->dc_notice=NULL;app->selected_dc[0]=0;
#endif
	plus_vita_credential_clear(&app->credential);
	free(app->catalogue);
	app->catalogue = NULL;
	app->catalogue_count = app->catalogue_selection = 0; app->catalogue_first=0;
	app->regional_catalogue=false;
	memset(app->label, 0, sizeof(app->label));
	memset(app->online_id, 0, sizeof(app->online_id));
	memset(app->store_country, 0, sizeof(app->store_country));
	memset(app->store_language, 0, sizeof(app->store_language));
	memset(app->language, 0, sizeof(app->language));
	memset(app->account_status, 0, sizeof(app->account_status));
	memset(app->catalogue_status, 0, sizeof(app->catalogue_status));
	memset(app->search, 0, sizeof(app->search));
	memset(app->search_button,0,sizeof(app->search_button)); memset(app->detail_status,0,sizeof(app->detail_status));
	app->matches_count = 0; app->search_focus = false; app->platform_filter=0;
	memset(app->artwork_metadata,0,sizeof(app->artwork_metadata));
	app->connected = app->retry_account = app->retry_save = false;
}

static PlusVitaAppEffect remove_saved(PlusVitaApp *app, bool forget, bool expired)
{
	app->forgetting = forget;
	app->expired = expired;
	app->storage_return = PLUS_VITA_APP_ACCOUNT;
	if(forget || expired) clear_runtime(app);
	return operation(app, PLUS_VITA_EFFECT_DELETE_ACCOUNT, "Removing saved sign-in from this Vita.", false);
}

static bool title_matches(const char *title, const char *query)
{
	if(!*query) return true;
	for(; *title; title++) {
		const unsigned char *a=(const unsigned char *)title, *b=(const unsigned char *)query;
		while(*a && *b && tolower(*a)==tolower(*b)) { a++; b++; }
		if(!*b) return true;
	}
	return false;
}

static int menu_direction(const PlusVitaApp *app, PlusVitaAppPad pad)
{
	uint32_t directions = pad.buttons & (PLUS_VITA_MENU_UP | PLUS_VITA_MENU_DOWN);
	if(app->view.screen == PLUS_VITA_APP_CATALOGUE)
		directions |= pad.buttons & (PLUS_VITA_MENU_LEFT | PLUS_VITA_MENU_RIGHT);
	if(directions) {
		bool forward = (directions & (PLUS_VITA_MENU_DOWN | PLUS_VITA_MENU_RIGHT)) != 0;
		bool backward = (directions & (PLUS_VITA_MENU_UP | PLUS_VITA_MENU_LEFT)) != 0;
		return forward == backward ? 0 : forward ? 1 : -1;
	}
	int axis = pad.left_y;
	if(app->view.screen == PLUS_VITA_APP_CATALOGUE
		&& abs((int)pad.left_x - 128) > abs(axis - 128)) axis = pad.left_x;
	return axis > 180 ? 1 : axis < 76 ? -1 : 0;
}

static void filter_catalogue(PlusVitaApp *app)
{
	app->matches_count = 0;
	for(size_t i=0;i<app->catalogue_count;i++){
#if PLUS_VITA_ADAPTIVE_STREAM
		if(pv_catalog_matches(app->platform_filter,app->catalogue[i].platform) && title_matches(app->catalogue[i].name,app->search))
#else
		if(title_matches(app->catalogue[i].name,app->search))
#endif
			app->matches[app->matches_count++]=i;
	}
	app->catalogue_selection = 0; app->catalogue_first=0;
}

static void catalogue_view(PlusVitaApp *app)
{
#if PLUS_VITA_ADAPTIVE_STREAM
	PlusVitaAppView v={.screen=PLUS_VITA_APP_CATALOGUE,.title="Choose your next game.",
		.subtitle=app->regional_catalogue?"REGIONAL DISCOVERY - ACCESS UNCONFIRMED":"CLOUD CATALOGUE",
		.body=app->catalogue_status,.hint="Square Search / Triangle Filter / X Play / O Back",
		.platform_filter=app->platform_filter};
	snprintf(app->catalogue_status,sizeof(app->catalogue_status),PV_TR("%u / %u games"),
		(unsigned)app->matches_count,(unsigned)app->catalogue_count);
	snprintf(app->filter_button,sizeof(app->filter_button),PV_TR("Filter: %s"),
		PV_TR(pv_catalog_filter_name(app->platform_filter)));
	snprintf(app->search_button,sizeof(app->search_button),"%s",app->search[0]?app->search:PV_TR("Search games..."));
	if(app->matches_count){
		list_viewport(app->matches_count,7,app->catalogue_selection,&app->catalogue_first);
		v.list_total=app->matches_count;v.list_first=app->catalogue_first;v.focus=app->catalogue_selection;
		v.list_x=14;v.list_y=116;v.list_width=368;v.row_step=51;v.list_height=357;
		for(size_t j=v.list_first;j<app->matches_count&&j<v.list_first+7;j++){
			const PlusVitaCatalogEntry *g=&app->catalogue[app->matches[j]];
			v.games[v.action_count]=g;
			v.actions[v.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_GAME,g->name,14,116+(int)(j-v.list_first)*51,368,49};
		}
		v.list_count=v.action_count;
		v.selected=app->catalogue_selection-v.list_first;
		v.detail=&app->catalogue[app->matches[app->catalogue_selection]];
	}
	if(!v.detail)v.title=app->search[0]?"No search results.":"No cloud games found.";
	if(app->search_focus||!v.list_count)v.selected=v.action_count;
	v.actions[v.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_SEARCH,app->search_button,21,62,app->search[0]?238:288,32};
	if(app->search[0])v.actions[v.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_CLEAR_SEARCH,"Clear search",263,62,46,32};
	for(unsigned filter=0;filter<4;filter++)
		v.actions[v.action_count++]=(PlusVitaAppButton){(PlusVitaAppAction)(PLUS_VITA_ACTION_FILTER_ALL+filter),PV_TR(filter?pv_catalog_filter_name(filter):"All"),343+(int)filter*80,64,70,28};
	if(v.detail)v.actions[v.action_count++]=(PlusVitaAppButton){v.detail->launch_id_valid?PLUS_VITA_ACTION_PLAY:PLUS_VITA_ACTION_DISABLED,
		v.detail->launch_id_valid?(pv_catalog_platform(v.detail->platform)==3?"Experimental test":"Play"):"Play unavailable",672,386,264,46};
	v.actions[v.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_BACK,"Back",866,500,80,36};
	app->view=v;
#else
	PlusVitaAppView view = {
		.screen = PLUS_VITA_APP_CATALOGUE, .title = "Choose your next game.",
		.subtitle = app->regional_catalogue ? "REGIONAL DISCOVERY - ACCESS UNCONFIRMED" : "CLOUD CATALOGUE", .body = app->catalogue_status,
		.hint = "Left stick  Browse     X  Details     Triangle  Search     O  Back"
	};
	snprintf(app->catalogue_status, sizeof(app->catalogue_status),
		PV_TR("%u / %u games"), (unsigned)app->matches_count, (unsigned)app->catalogue_count);
	if(!app->catalogue_count)
	{
		view.title = "No cloud games found.";
		view.actions[0] = (PlusVitaAppButton){ PLUS_VITA_ACTION_BACK, "Back", 64, 444, 500, 52 };
		view.action_count = 1;
	}
	else if(!app->matches_count)
	{
		view.title = "No search results.";
		view.actions[0] = (PlusVitaAppButton){ PLUS_VITA_ACTION_SEARCH, "Edit search", 64, 228, 832, 42 };
		view.actions[1] = (PlusVitaAppButton){ PLUS_VITA_ACTION_CLEAR_SEARCH, "Clear search", 64, 380, 832, 52 };
		view.actions[2] = (PlusVitaAppButton){ PLUS_VITA_ACTION_BACK, "Back", 64, 444, 832, 52 };
		view.action_count = 3;
	}
	else
	{
		list_viewport(app->matches_count,7,app->catalogue_selection,&app->catalogue_first);
		size_t first=app->catalogue_first;
		view.list_total=app->matches_count;view.list_first=first;view.focus=app->catalogue_selection;
		view.list_x=64;view.list_y=228;view.list_width=624;view.list_height=252;view.row_step=36;
		for(size_t i=first;i<app->matches_count && i<first+7;i++) {
			const PlusVitaCatalogEntry *game=&app->catalogue[app->matches[i]];
			view.games[view.action_count]=game;
			view.actions[view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_GAME,game->name,64,228+(int)(i-first)*36,624,32};
		}
		view.list_count=view.action_count;
		view.detail=&app->catalogue[app->matches[app->catalogue_selection]];
		snprintf(app->search_button,sizeof(app->search_button),PV_TR("Search: %s"),app->search[0] ? app->search : PV_TR("All games"));
		size_t search_index=view.action_count;
		view.actions[view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_SEARCH,app->search_button,64,186,624,32};
		view.actions[view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_BACK,"Back",744,448,152,32};
		view.selected=app->search_focus ? search_index : app->catalogue_selection-first;
	}
	app->view = view;
#endif
}

/* Technical codes and protocol stage names remain recognizable in issue reports. */
static void stream_diagnostic_text(PlusVitaApp *app,PlusVitaStreamDiagnostic d)
{
	app->stream_diag_text[0]=0;
	if(!d.valid)return;
	static const char *const phases[]={"Preflight","Getting Client","Getting Configuration","Starting Session",
		"Getting Tokens","Getting Server Tokens","Authorizing Session","Locking Session","Getting Datacenters",
		"Pinging","Selecting Datacenter","Allocating Streaming Slot"};
	static const char *const startup[]={"Transport","Handshake","STREAMINFO","Decoder","Ready","Transport failed",
		"Handshake failed","STREAMINFO interrupted","STREAMINFO invalid","Profile unavailable","Audio init failed",
		"Video header invalid","Header guard rejected","Decoder init failed","STREAMINFO reply failed","Provision invalid",
		"Media disconnected","Media decrypt failed","Media invalid","First frame timeout","PS3 feedback"};
	static const char *const reasons[]={"N/A","002.0011","002.0019","002.2001","R4","R5","R6","R7","R8","R9","R10","R11","R12","R13","R14","R15","R16","R17","R18","Bad Request","Unauthorized","Component 002"};
	char stage[72],http[8],native[8];
	if(d.startup&&d.startup<=sizeof(startup)/sizeof(startup[0]))
		snprintf(stage,sizeof(stage),"Video: %s (S%u)",startup[d.startup-1],d.startup-1);
	else if(d.phase<=11)snprintf(stage,sizeof(stage),"%s (%u/11)",phases[d.phase],d.phase);
	else snprintf(stage,sizeof(stage),"N/A");
	if(d.http>=100&&d.http<=599)snprintf(http,sizeof(http),"%u",d.http);else strcpy(http,"N/A");
	if(d.native_error>=0&&d.native_error<=21)snprintf(native,sizeof(native),"%d",d.native_error);else strcpy(native,"N/A");
	bool ps5=false;
#if PLUS_VITA_PS5_EXPERIMENTAL
	if(app->catalogue_count&&app->matches_count)ps5=app->catalogue[app->matches[app->catalogue_selection]].ps5_stream_id[0]!=0;
#endif
	snprintf(app->stream_diag_text,sizeof(app->stream_diag_text),
		"Phase: %s | HTTP: %s | Service: %s | E%s\nRequested: %s / H.264",
		stage,http,reasons[d.reason<=21?d.reason:0],native,ps5?"Portal":"PC");
}

static void attach_stream_diagnostic(PlusVitaApp *app)
{
	if(!app->stream_diag_text[0])return;
	snprintf(app->stream_diag_body,sizeof(app->stream_diag_body),"%s\n%s",app->view.body?app->view.body:"",app->stream_diag_text);
	app->view.body=app->stream_diag_body;
}

static void detail_view(PlusVitaApp *app, const char *notice)
{
	const PlusVitaCatalogEntry *game=&app->catalogue[app->matches[app->catalogue_selection]];
	char platform_line[128];
	snprintf(platform_line,sizeof(platform_line),PV_TR("Platform: %s"),game->platform[0] ? game->platform : PV_TR("Not specified"));
	snprintf(app->detail_status,sizeof(app->detail_status),"%s\n%s\n%s",game->name,platform_line,
		PV_TR(notice ? notice : game->launch_id_valid ? "Cloud catalogue title. Streaming eligibility is checked at launch." : "Play unavailable: required launch information is missing."));
	app->view=(PlusVitaAppView){.screen=PLUS_VITA_APP_DETAILS,.title=game->name,.subtitle="GAME DETAILS",.body=app->detail_status,.hint="X  Select     O  Back",.detail=game,
		.actions={{game->launch_id_valid ? PLUS_VITA_ACTION_PLAY : PLUS_VITA_ACTION_DISABLED,game->launch_id_valid ? "Play" : "Play unavailable",64,380,832,52},{PLUS_VITA_ACTION_BACK,"Back",64,444,832,52}},.action_count=2};
}

#if PLUS_VITA_PS5_EXPERIMENTAL
static void datacenters_view(PlusVitaApp *app)
{
	if(!app->bitrate_kbps) app->bitrate_kbps=PLUS_VITA_ADAPTIVE_STREAM ? 15000 : 8000;
	if(app->dc_notice) snprintf(app->dc_status,sizeof(app->dc_status),"%s%s%s",PV_TR(app->dc_notice),
		app->stream_diag_text[0]?"\n":"",app->stream_diag_text);
	else snprintf(app->dc_status,sizeof(app->dc_status),PV_TR("Choose a server manually. Page %u of %u. No automatic fallback."),
		(unsigned)(app->dc_page+1),(unsigned)((app->dc_count+2)/3));
	snprintf(app->bitrate_label,sizeof(app->bitrate_label),PLUS_VITA_ADAPTIVE_STREAM ? PV_TR("%s: %u Mbps (change)") : "%s%u Mbps (change)",PLUS_VITA_ADAPTIVE_STREAM ? PV_TR(app->bitrate_kbps==6000?"Low latency":app->bitrate_kbps==10000?"Balanced":"Maximum quality") : "Bitrate: ",app->bitrate_kbps/1000);
	PlusVitaAppView view={.screen=PLUS_VITA_APP_DATACENTERS,.title="Choose cloud server.",
		.subtitle="MANUAL DATACENTER",.body=app->dc_status,.hint="X  Select     O  Back"};
	size_t first=app->dc_page*3;
	for(size_t i=first;i<app->dc_count && i<first+3;i++) {
		size_t n=view.action_count;
		view.actions[view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_DATACENTER,app->datacenters[i],64,234+(int)n*38,832,32};
	}
	if(app->dc_page) {
		size_t n=view.action_count;
		view.actions[view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_DC_PREVIOUS,"Previous servers",64,234+(int)n*38,832,32};
	}
	if(first+3<app->dc_count) {
		size_t n=view.action_count;
		view.actions[view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_DC_NEXT,"Next servers",64,234+(int)n*38,832,32};
	}
	size_t n=view.action_count;
	view.actions[view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_BITRATE,app->bitrate_label,64,234+(int)n*38,832,32};
	n=view.action_count;
	view.actions[view.action_count++]=(PlusVitaAppButton){PLUS_VITA_ACTION_BACK,"Back",64,234+(int)n*38,832,32};
	 #if PLUS_VITA_ADAPTIVE_STREAM
	int top=app->dc_notice&&app->stream_diag_text[0]?280:250;
	for(size_t i=0;i<view.action_count;i++){view.actions[i].y=top+(int)i*30;view.actions[i].height=26;}
	view.actions[view.action_count]=(PlusVitaAppButton){PLUS_VITA_ACTION_HUD,app->hud?"Performance HUD: On":"Performance HUD: Off",64,top+(int)view.action_count*30,832,26};view.action_count++;
 #endif
	app->view=view;
}
static bool datacenters_read(PlusVitaApp *app,const char *json)
{
	app->dc_count=app->dc_page=0;
	if(!json || strlen(json)>65536) return false;
	struct json_object *root=json_tokener_parse(json);
	if(!root || !json_object_is_type(root,json_type_array)) {
		if(root) json_object_put(root);
		return false;
	}
	for(size_t i=0;i<json_object_array_length(root) && app->dc_count<64;i++) {
		struct json_object *row=json_object_array_get_idx(root,i), *value=NULL;
		if(!row || !json_object_is_type(row,json_type_object)
			|| !json_object_object_get_ex(row,"dataCenter",&value)
			|| !json_object_is_type(value,json_type_string)) continue;
		const char *name=json_object_get_string(value);
		if(!name || (size_t)json_object_get_string_len(value)!=strlen(name)
			|| !plus_vita_manual_dc_valid(name)) continue;
		bool duplicate=false;
		for(size_t n=0;n<app->dc_count;n++) if(!strcmp(name,app->datacenters[n])) duplicate=true;
		if(!duplicate) snprintf(app->datacenters[app->dc_count++],128,"%s",name);
	}
	json_object_put(root);
	return app->dc_count>0;
}
#endif

static void stream_stopped_view(PlusVitaApp *app)
{
	const PlusVitaCatalogEntry *game=&app->catalogue[app->matches[app->catalogue_selection]];
	app->view=(PlusVitaAppView){.screen=PLUS_VITA_APP_STREAM_STOPPED,
		.title="Cloud Play stopped.",.subtitle="SESSION INTERRUPTED",
		.body="The session stopped unexpectedly before you chose to leave.\nThis could be a connection or app problem.\nCopy " PLUS_VITA_DIAGNOSTIC_VITA_PATH " before restarting the app.",
		.hint="X  Select     O  Back",.detail=game,
		.actions={{PLUS_VITA_ACTION_PLAY,"Try Play again",64,380,832,52},
			{PLUS_VITA_ACTION_BACK,"Back to catalogue",64,444,832,52}},.action_count=2};
}

#if PLUS_VITA_FONT_COMPARISON
static void font_comparison(PlusVitaApp *app)
{
	app->view=(PlusVitaAppView){.screen=PLUS_VITA_APP_FONT_COMPARISON,
		.subtitle=app->alternative_font ? "B: FREETYPE / NOTO SANS" : "A: CURRENT SYSTEM FONT",
		.title=app->comparison_page ? "Choose your next game." : "Import your sign-in JSON.",
		.body=app->comparison_page ? "Synthetic titles. Japanese uses the system fallback." :
			"Save plus-vita-auth.json in ux0:data/VitaPlus/Imports with VitaShell.\nThe JSON stays there and can sign you in next launch.\nRemember Off pauses automatic sign-in; files are locally readable.",
		.hint="Left stick  Move     X  Select     O  Back     Square  Save image",
		.alternative_font=app->alternative_font,.comparison_page=app->comparison_page,
		.actions={{PLUS_VITA_ACTION_FONT_TOGGLE,app->alternative_font ? "Show A: System" : "Show B: FreeType",64,444,260,48},
			{PLUS_VITA_ACTION_FONT_SAMPLE,app->comparison_page ? "Show setup" : "Show tiles",350,444,260,48},
			{PLUS_VITA_ACTION_BACK,"Back",636,444,260,48}},.action_count=3};
}
#endif

static PlusVitaAppEffect import_account(PlusVitaApp *app)
{
	clear_runtime(app);
	app->pending_operation = ++app->next_operation;
	app->waiting_for = PLUS_VITA_EFFECT_READ_IMPORT;
	app->view = (PlusVitaAppView){
		.screen = PLUS_VITA_APP_AUTHORIZING, .title = "Connecting your account.",
		.subtitle = "ACCOUNT SETUP", .body = "Reading your sign-in JSON. You can cancel at any time.",
		.hint = "O  Cancel", .actions = {{ PLUS_VITA_ACTION_BACK, "Cancel", 64, 444, 500, 52 }},
		.action_count = 1
	};
	app->view.operation = app->pending_operation;
	return (PlusVitaAppEffect){ .kind = app->waiting_for, .operation = app->pending_operation };
}

static void confirm_exit(PlusVitaApp *app)
{
	app->view = (PlusVitaAppView){
		.screen = PLUS_VITA_APP_EXIT_CONFIRM,
		.title = "See you next time?", .subtitle = "EXIT VITAPLUS",
		.body = app->saved ? "Your saved sign-in will be checked next time you open VitaPlus."
			: app->connected ? "Exiting ends this sign-in. Enable Remember in Account to keep it for next launch."
			: "Come back whenever you are ready to play.",
		.hint = "X  Select     O  Back",
		.actions = {
			{ PLUS_VITA_ACTION_BACK, "Stay here", 64, 380, 500, 52 },
			{ PLUS_VITA_ACTION_EXIT, "Exit VitaPlus", 64, 444, 500, 52 }
		}, .action_count = 2
	};
}

PlusVitaAppEffect plus_vita_app_dispatch(PlusVitaApp *app, PlusVitaAppEvent event)
{
	PlusVitaAppView *view = &app->view;
	PlusVitaAppEffect effect = { .kind = PLUS_VITA_EFFECT_NONE };
	#if PLUS_VITA_ADAPTIVE_STREAM
    if(event.kind==PLUS_VITA_EVENT_UPDATE_REPORT&&event.update_report){
        bool new_release=strcmp(app->update.version,event.update_report->version)!=0;
        bool changed=app->update.state!=event.update_report->state;
        app->update=*event.update_report;
        if(new_release)app->update_note_page=0;
        if(view->screen==PLUS_VITA_APP_UPDATES){
            size_t selected=view->selected;
            app->pending_operation=0;
            update_view(app);
            if(!changed&&selected<view->action_count)view->selected=selected;
        }else if(view->screen==PLUS_VITA_APP_SETTINGS){
            size_t selected=view->selected;settings_view(app);if(selected<view->action_count)view->selected=selected;
        }else if(view->screen==PLUS_VITA_APP_WELCOME){
            size_t selected=view->selected;welcome(app);if(selected<view->action_count)view->selected=selected;
        }
        if(app->update.state==PV_UP_INSTALLED)view->screen=PLUS_VITA_APP_CLOSED;
        return effect;
    }
    if(event.kind==PLUS_VITA_EVENT_THEME_LOADED){
        if(event.theme_primary<=0xffffffu&&event.theme_secondary<=0xffffffu){
            app->theme_custom=event.theme_custom;
            app->theme_primary=event.theme_custom?event.theme_primary:PV_PRIMARY_DEFAULT;
            app->theme_secondary=event.theme_custom?event.theme_secondary:PV_SECONDARY_DEFAULT;
        }
        return effect;
    }
    if(event.kind==PLUS_VITA_EVENT_THEME_ENTERED){
        bool primary=app->waiting_for==PLUS_VITA_EFFECT_EDIT_PRIMARY;
        if((!primary&&app->waiting_for!=PLUS_VITA_EFFECT_EDIT_SECONDARY)||!event.operation||event.operation!=app->pending_operation)return effect;
        app->pending_operation=0;app->waiting_for=PLUS_VITA_EFFECT_NONE;
        uint32_t rgb;
        if(event.status==PLUS_VITA_RESULT_CANCELLED){theme_view(app,NULL);return effect;}
        if(event.status!=PLUS_VITA_RESULT_OK||!pv_theme_hex(event.data,event.data_size,&rgb)){
            theme_view(app,"Invalid color. Enter exactly six hexadecimal digits.");return effect;
        }
        if(primary)app->theme_primary=rgb;else app->theme_secondary=rgb;
        app->theme_custom=true;theme_view(app,NULL);app->view.selected=primary?0:1;return effect;
    }
	if(event.kind==PLUS_VITA_EVENT_STREAM_SETTINGS) {
		app->bitrate_kbps=pv_profile_cap(event.stream_profile);app->hud=event.stream_hud;app->region_mode=pv_region_mode(event.region_mode);app->ui_language=pv_language(event.ui_language);return effect;
	}
#endif
	if(event.kind==PLUS_VITA_EVENT_CONTROLS_LOADED) {
		if(plus_vita_control_layout_valid(&event.controls)) app->controls=event.controls;
		return effect;
	}
	if(event.kind==PLUS_VITA_EVENT_CONTROLS_SAVED) {
		if(app->waiting_for!=PLUS_VITA_EFFECT_SAVE_CONTROLS || !event.operation || event.operation!=app->pending_operation) return effect;
		app->pending_operation=0;app->waiting_for=PLUS_VITA_EFFECT_NONE;
		if(event.status==PLUS_VITA_RESULT_OK) {
			app->controls=app->controls_draft;settings_view(app);
			view->body="Controls saved. Your next gaming session will use this layout.";
		} else {
			snprintf(app->controls_status,sizeof(app->controls_status),"Save failed. Previous layout kept. Retry Save or Cancel.");
			layout_view(app);
		}
		return effect;
	}
	if(event.kind==PLUS_VITA_EVENT_CONTROL_DEVICE) {
		if(app->control_device==event.device) return effect;
		app->control_device=event.device;
		if(view->screen==PLUS_VITA_APP_BINDINGS || view->screen==PLUS_VITA_APP_BIND_ACTIONS)
			bindings_view(app,view->screen==PLUS_VITA_APP_BIND_ACTIONS);
		else if(view->screen==PLUS_VITA_APP_CONTROLS) controls_view(app);
		return effect;
	}
	if(app->waiting_for==PLUS_VITA_EFFECT_SAVE_CONTROLS) return effect;
	if(view->screen == PLUS_VITA_APP_CLOSED) return effect;
	if(event.kind == PLUS_VITA_EVENT_START)
	{
		if(app->started) return effect;
		app->started = true;
		app->log_available = event.log_available;
		return operation(app, PLUS_VITA_EFFECT_LOAD_ACCOUNT, "Checking for a remembered account.", true);
	}
	if(event.kind == PLUS_VITA_EVENT_LOG_STATUS)
	{
		if(app->log_available == event.log_available) return effect;
		app->log_available = event.log_available;
		if(view->screen == PLUS_VITA_APP_SETTINGS) settings_view(app);
		else if(view->screen == PLUS_VITA_APP_REPORTING) reporting_view(app);
		return effect;
	}
	if(event.kind == PLUS_VITA_EVENT_INTERRUPTED)
	{
		/* Wi-Fi does not own local guides, keyboards or file operations. */
		if(event.status==PLUS_VITA_RESULT_NETWORK_ERROR
			&& app->waiting_for!=PLUS_VITA_EFFECT_AUTHORIZE
			&& app->waiting_for!=PLUS_VITA_EFFECT_PLAY) return effect;
		/* Storage mutations finish atomically. Network/UI work is retired now;
		 * its operation number can no longer deliver a usable late result. */
		if(app->waiting_for == PLUS_VITA_EFFECT_SAVE_ACCOUNT
			|| app->waiting_for == PLUS_VITA_EFFECT_DELETE_ACCOUNT
			|| app->waiting_for == PLUS_VITA_EFFECT_ENABLE_AUTO) return effect;
		bool playing = app->waiting_for == PLUS_VITA_EFFECT_PLAY;
		bool pending = app->pending_operation != 0;
		app->pending_operation = 0;
		app->waiting_for = PLUS_VITA_EFFECT_NONE;
		if(playing) {
			stream_stopped_view(app);
			view->body = event.status == PLUS_VITA_RESULT_SLEEP
				? "Play ended for sleep. The old session cannot resume.\nSelect Play for a fresh cloud session when ready.\nCopy the log before restarting the app."
				: "Wi-Fi was lost. The old session cannot resume.\nReconnect Wi-Fi and select Play for a fresh session.\nCopy the log before restarting the app.";
		} else if(pending) {
			if(app->connected) {
				catalogue_view(app);
				view->body="Work was cancelled. Your account stays connected.\nContinue browsing when ready.";
			} else {
				app->retry_account = app->credential.length != 0;
				account_error(app, event.status == PLUS_VITA_RESULT_SLEEP
					? "Connection cancelled for sleep. Retry when ready."
					: "Wi-Fi was lost. Reconnect and retry when ready.");
			}
		}
		return effect;
	}
	if(event.kind == PLUS_VITA_EVENT_PAD_RESET)
	{
		app->previous_buttons = event.pad.buttons;
		app->previous_direction = menu_direction(app, event.pad);
		app->repeat_at = event.pad.time_us + 400000u;
		return effect;
	}
	if(event.kind==PLUS_VITA_EVENT_FONT_CAPTURED && view->screen==PLUS_VITA_APP_FONT_COMPARISON) {
		view->hint=event.status==PLUS_VITA_RESULT_OK ? "Synthetic screenshot saved in ux0:data/VitaPlus/Captures. Square saves again." : "Screenshot could not be saved. Check storage.";
		return effect;
	}
	if(event.kind == PLUS_VITA_EVENT_PAD)
	{
		PlusVitaAppPad pad = event.pad;
		uint32_t pressed = pad.buttons & ~app->previous_buttons;
		int direction = menu_direction(app, pad);
		bool navigate = direction && (direction != app->previous_direction || pad.time_us >= app->repeat_at);
		if(navigate) app->repeat_at = pad.time_us + (direction != app->previous_direction ? 400000u : 160000u);
		app->previous_direction = direction;
		app->previous_buttons = pad.buttons;
		if(pressed & PLUS_VITA_MENU_CIRCLE) event.kind = PLUS_VITA_EVENT_BACK;
		else if(pressed & PLUS_VITA_MENU_TRIANGLE) event.kind = PLUS_VITA_ADAPTIVE_STREAM && view->screen==PLUS_VITA_APP_CATALOGUE ? PLUS_VITA_EVENT_PLATFORM_FILTER : PLUS_VITA_EVENT_SEARCH;
		else if(PLUS_VITA_ADAPTIVE_STREAM && view->screen==PLUS_VITA_APP_CATALOGUE && (pressed & PLUS_VITA_MENU_SQUARE)) event.kind=PLUS_VITA_EVENT_SEARCH;
		else if(pressed & PLUS_VITA_MENU_CROSS) event.kind = PLUS_VITA_EVENT_ACTIVATE;
		else if(navigate) event.kind = direction > 0 ? PLUS_VITA_EVENT_DOWN : PLUS_VITA_EVENT_UP;
		else return effect;
	}
	if(event.kind == PLUS_VITA_EVENT_IMPORT_READ
		|| event.kind == PLUS_VITA_EVENT_AUTHORIZED || event.kind == PLUS_VITA_EVENT_ACCOUNT_LOADED
		|| event.kind == PLUS_VITA_EVENT_ACCOUNT_SAVED || event.kind == PLUS_VITA_EVENT_ACCOUNT_DELETED
		|| event.kind == PLUS_VITA_EVENT_AUTO_ENABLED)
	{
		if(view->screen != PLUS_VITA_APP_AUTHORIZING || !event.operation
			|| event.operation != app->pending_operation) return effect;
		if(event.kind == PLUS_VITA_EVENT_AUTO_ENABLED && app->waiting_for == PLUS_VITA_EFFECT_ENABLE_AUTO) {
			if(event.status != PLUS_VITA_RESULT_OK) account_error(app, "Automatic sign-in could not be enabled.\nCheck storage and try again from setup.");
			else { app->remember = true; setup_view(app); app->view.selected = 1; }
			return effect;
		}
		if(event.kind == PLUS_VITA_EVENT_ACCOUNT_LOADED && app->waiting_for == PLUS_VITA_EFFECT_LOAD_ACCOUNT)
		{
#if PLUS_VITA_ACCOUNT_VAULT
			app->vault_present=event.vault_present;app->vault_legacy=event.legacy_plain;app->vault_mode=event.vault_mode;
			if(event.status==PLUS_VITA_RESULT_CANCELLED||event.status==PLUS_VITA_RESULT_VAULT_LOCKED){
				app->pending_operation=0;app->waiting_for=PLUS_VITA_EFFECT_NONE;app->saved=true;
				security_view(app,event.status==PLUS_VITA_RESULT_CANCELLED?"Saved token remains locked.":"Wrong password or damaged vault. Restore again or forget the account.");return effect;
			}
#endif
			if(event.status == PLUS_VITA_RESULT_MISSING) {
				app->pending_operation = 0; app->waiting_for = PLUS_VITA_EFFECT_NONE;
				app->saved = false; app->remember = !event.restore_blocked; clear_runtime(app); welcome(app); return effect;
			}
			if(event.status != PLUS_VITA_RESULT_OK || !parse_import(&app->credential, event.data, event.data_size,
				event.retained_json ? NULL : app->label))
			{
				app->saved = true; /* Presence/error is not proof of a usable sign-in. */
				account_error(app, "The saved account could not be read.\nCheck local storage or forget the saved account and import again.");
				return effect;
			}
			app->saved = app->remember = true;
#if PLUS_VITA_ACCOUNT_VAULT
			if(event.legacy_plain)app->saved=false; /* migrate only after Sony accepts it */
#endif
			return operation(app, PLUS_VITA_EFFECT_AUTHORIZE, "Restoring your saved sign-in and loading cloud games.", true);
		}
		if(event.kind == PLUS_VITA_EVENT_ACCOUNT_SAVED && app->waiting_for == PLUS_VITA_EFFECT_SAVE_ACCOUNT)
		{
			app->pending_operation = 0;
			app->waiting_for = PLUS_VITA_EFFECT_NONE;
			#if PLUS_VITA_ACCOUNT_VAULT
			if(event.status==PLUS_VITA_RESULT_CANCELLED||event.status==PLUS_VITA_RESULT_VAULT_LOCKED){
				app->vault_present=event.vault_present;app->vault_mode=event.vault_mode;
				app->saved=app->vault_present||app->vault_legacy;app->remember=app->saved;
				security_view(app,event.status==PLUS_VITA_RESULT_CANCELLED?"Save cancelled. Previous file unchanged.":"Wrong password or damaged vault. Restore again or forget the account.");return effect;
			}
#endif
			if(event.status != PLUS_VITA_RESULT_OK)
			{
				app->retry_save = true;
				account_error(app, "Your account changes could not be saved.\nYou can keep using this app run, or retry saving before exiting.");
				return effect;
			}
			app->saved = app->remember = true;
			app->retry_save = false;
#if PLUS_VITA_ACCOUNT_VAULT
			app->vault_present=event.vault_present;app->vault_legacy=false;app->vault_mode=event.vault_mode;
			if(event.import_cleanup_failed||app->storage_return==PLUS_VITA_APP_SECURITY){security_view(app,event.import_cleanup_failed?"Vault saved, but imported JSON could not be removed.":NULL);return effect;}
#endif
			if(app->storage_return == PLUS_VITA_APP_ACCOUNT) account_view(app); else catalogue_view(app);
			return effect;
		}
		if(event.kind == PLUS_VITA_EVENT_ACCOUNT_DELETED && app->waiting_for == PLUS_VITA_EFFECT_DELETE_ACCOUNT)
		{
			app->pending_operation = 0;
			app->waiting_for = PLUS_VITA_EFFECT_NONE;
			if(event.restore_blocked) app->saved = app->remember = false;
			if(event.status != PLUS_VITA_RESULT_OK)
			{
				app->retry_delete = true;
				account_error(app, "Account files could not all be removed.\nCheck storage and retry cleanup. Removal is not complete.");
				return effect;
			}
			app->saved = app->remember = false;
			app->vault_present=app->vault_legacy=false;app->vault_mode=0;
			app->retry_delete = false;
			if(app->expired) { app->expired = false; account_error(app, result_message(PLUS_VITA_RESULT_REJECTED)); attach_stream_diagnostic(app); }
			else if(app->forgetting) { app->forgetting = false; app->welcome_selection = 0; setup_view(app); }
			else if(app->storage_return == PLUS_VITA_APP_SETUP) { setup_view(app); app->view.selected = 1; }
			else if(app->storage_return == PLUS_VITA_APP_CATALOGUE) catalogue_view(app);
			else account_view(app);
			return effect;
		}
		if(event.kind == PLUS_VITA_EVENT_IMPORT_READ && app->waiting_for == PLUS_VITA_EFFECT_READ_IMPORT)
		{
#if PLUS_VITA_QR_PAIRING
			if(event.status==PLUS_VITA_RESULT_CANCELLED) {setup_view(app);return effect;}
			#if PLUS_VITA_LAN_LOGIN
			if(event.status==PLUS_VITA_RESULT_UNSUPPORTED) {account_error(app,"Local sign-in could not start. Connect to a private Wi-Fi network and retry.");return effect;}
#else
			if(event.status==PLUS_VITA_RESULT_UNSUPPORTED) {account_error(app,"QR pairing could not start. Use Import JSON or retry on a Vita with a camera.");return effect;}
#endif
#endif
			if(event.status != PLUS_VITA_RESULT_OK)
			{
				account_error(app, result_message(event.status));
				return effect;
			}
			app->imported_valid = parse_import(&app->credential, event.data, event.data_size, NULL);
			if(!app->imported_valid)
			{
				#if PLUS_VITA_LAN_LOGIN
				account_error(app, "Invalid Sony response. Start QR sign-in again.");
#else
				account_error(app, "The JSON sign-in data was not valid. The file was kept.\nReplace it with a fresh browser JSON response and try again.");
#endif
				return effect;
			}
			app->waiting_for = PLUS_VITA_EFFECT_AUTHORIZE;
			view->body = "Checking your sign-in and loading cloud games.\nYou can cancel while this is working.";
			effect = (PlusVitaAppEffect){ .kind = PLUS_VITA_EFFECT_AUTHORIZE, .operation = event.operation, .credential = app->credential,.region_mode=app->region_mode };
		}
		else if(event.kind == PLUS_VITA_EVENT_AUTHORIZED && app->waiting_for == PLUS_VITA_EFFECT_AUTHORIZE)
		{
			if(event.status != PLUS_VITA_RESULT_OK)
			{
				app->retry_account = event.status != PLUS_VITA_RESULT_REJECTED;
				if(event.status == PLUS_VITA_RESULT_REJECTED) return remove_saved(app, false, true);
				if(event.status==PLUS_VITA_RESULT_NETWORK_ERROR&&event.auth_stage){
					const char *stage=event.auth_stage==1?"Sony authorization":event.auth_stage==2?"Sony session":"Sony store";
					const char *reason=event.auth_transport_error==2?"DNS lookup failed":event.auth_transport_error==9?"Connection failed":event.auth_transport_error==15?"Connection timed out":event.auth_transport_error==16?"TLS verification or handshake failed":event.auth_transport_error==14?"Request cancelled":"Network transport failed";
					if(event.auth_transport_error==16){
						const char *tls_reason=event.auth_curl_error==35?"TLS handshake failed":event.auth_curl_error==60?"Certificate verification failed":event.auth_curl_error==77?"CA bundle could not be loaded":"TLS setup failed";
						if(app->ui_language==2||app->ui_language==3){
							const char *pt=event.auth_curl_error==35?"Falha na negociação TLS":event.auth_curl_error==60?"Falha na validação do certificado":event.auth_curl_error==77?"Falha ao carregar certificados CA":"Falha na configuração TLS";
							snprintf(app->auth_status,sizeof(app->auth_status),"%s (E16/C%d).\n%s",pt,event.auth_curl_error,event.auth_curl_error==77?"Instale a versão atualizada do VPK. O token é mantido ao tentar novamente.":"Confira data e hora do Vita. Tentar novamente mantém o token.");
						}else snprintf(app->auth_status,sizeof(app->auth_status),"%s: %s (E16/C%d).\nCheck system date/time. Retry keeps your token.",stage,tls_reason,event.auth_curl_error);
					}else snprintf(app->auth_status,sizeof(app->auth_status),"%s: %s (E%d/C%d).\nCheck Wi-Fi, DNS and system date/time. Retry keeps your token.",stage,reason,event.auth_transport_error,event.auth_curl_error);
					account_error(app,app->auth_status);
				}else account_error(app, result_message(event.status));
				return effect;
			}
			if(event.entry_count > 4096 || (event.entry_count && !event.entries))
				account_error(app, "The catalogue could not be loaded. Please try again later.");
			else
			{
				app->regional_catalogue=event.regional_catalogue;
				memcpy(app->store_country,event.store_country,sizeof(app->store_country));
				memcpy(app->store_language,event.store_language,sizeof(app->store_language));
				memcpy(app->language,event.language,sizeof(app->language));
				const char *end = memchr(event.account_name, 0, sizeof(event.account_name));
				memset(app->online_id, 0, sizeof(app->online_id));
				if(end && plus_vita_utf8_label(event.account_name, (size_t)(end-event.account_name)))
					memcpy(app->online_id, event.account_name, (size_t)(end-event.account_name));
				if(event.entry_count)
				{
					app->catalogue = malloc(event.entry_count * sizeof(*app->catalogue));
					if(!app->catalogue) { account_error(app, "Not enough memory to load cloud games."); return effect; }
					memcpy(app->catalogue, event.entries, event.entry_count * sizeof(*app->catalogue));
					for(size_t i = 0; i < event.entry_count; i++) app->catalogue[i].name[PLUS_VITA_CATALOG_NAME_MAX - 1] = 0;
					qsort(app->catalogue, event.entry_count, sizeof(*app->catalogue), compare_titles);
				}
				app->catalogue_count = event.entry_count;
				memset(app->artwork_metadata,0,sizeof(app->artwork_metadata));
				for(size_t i=0;i<app->catalogue_count;i++) {
					PlusVitaCatalogEntry *game=&app->catalogue[i];
					game->cover_url[sizeof(game->cover_url)-1]=0;
					unsigned state=game->cover_url[0] ? PLUS_VITA_ARTWORK_AVAILABLE : (unsigned)game->cover_metadata;
					if(state>=PLUS_VITA_ARTWORK_METADATA_STATES) state=PLUS_VITA_ARTWORK_UNSUPPORTED;
					app->artwork_metadata[state]++;
				}
				filter_catalogue(app);
				app->connected = true;
				app->pending_operation = 0;
				app->waiting_for = PLUS_VITA_EFFECT_NONE;
				catalogue_view(app);
				app->retry_account = false;
				if(app->remember && !app->saved) {
					app->storage_return = PLUS_VITA_APP_CATALOGUE;
					return operation(app, PLUS_VITA_EFFECT_SAVE_ACCOUNT, "Saving your account on this Vita.", false);
				}
			}
		}
		return effect;
	}
	if(event.kind == PLUS_VITA_EVENT_SEARCH_ENTERED)
	{
		if(app->waiting_for!=PLUS_VITA_EFFECT_EDIT_SEARCH || !event.operation || event.operation!=app->pending_operation) return effect;
		app->pending_operation=0; app->waiting_for=PLUS_VITA_EFFECT_NONE;
		if(event.status==PLUS_VITA_RESULT_CANCELLED) { catalogue_view(app); return effect; }
		if(event.status!=PLUS_VITA_RESULT_OK || !plus_vita_utf8_label(event.data,event.data_size)) {
			catalogue_view(app); app->view.body="Search could not be entered. Please try again."; return effect;
		}
		memset(app->search,0,sizeof(app->search)); memcpy(app->search,event.data,event.data_size);
		filter_catalogue(app); app->search_focus=false; catalogue_view(app); return effect;
	}
	if(event.kind==PLUS_VITA_EVENT_PLAY_ENDED)
	{
		if(app->waiting_for!=PLUS_VITA_EFFECT_PLAY || !event.operation || event.operation!=app->pending_operation) return effect;
		app->pending_operation=0; app->waiting_for=PLUS_VITA_EFFECT_NONE;
		stream_diagnostic_text(app,event.stream_diagnostic);
		if(event.status==PLUS_VITA_RESULT_REJECTED) return remove_saved(app, false, true);
#if PLUS_VITA_ADAPTIVE_STREAM
		if(event.status==PLUS_VITA_RESULT_NETWORK_ERROR&&event.playback_started&&!app->auto_retry&&plus_vita_manual_dc_valid(app->selected_dc)&&app->catalogue_count) {
			app->auto_retry=true;
			effect=operation(app,PLUS_VITA_EFFECT_PLAY,"Reconnecting once to your selected server at reduced bitrate. Circle cancels.",true);
			effect.game=app->catalogue[app->matches[app->catalogue_selection]];effect.controls=app->controls;
			snprintf(effect.datacenter,sizeof(effect.datacenter),"%s",app->selected_dc);
			app->requested_kbps=pv_retry_rate(app->bitrate_kbps,app->requested_kbps);
			effect.bitrate_kbps=app->requested_kbps;effect.profile_kbps=app->bitrate_kbps;effect.hud=app->hud;
			return effect;
		}
#endif
		if(event.status==PLUS_VITA_RESULT_OK) catalogue_view(app);
		#if PLUS_VITA_PS5_EXPERIMENTAL
		else if(event.status==PLUS_VITA_RESULT_DATACENTERS) {
			app->dc_notice=NULL;
			if(datacenters_read(app,event.data)) datacenters_view(app);
			else detail_view(app,"The service returned no eligible servers. Try again later.");
		}
		else if((event.status==PLUS_VITA_RESULT_SERVICE_ERROR || event.status==PLUS_VITA_RESULT_NETWORK_ERROR) && !event.playback_started && plus_vita_manual_dc_valid(app->selected_dc)) {
			/* Use only the current title's server list. If the service returned
			 * a fresh list, prefer it to the previous discovery result. */
			if(event.data) datacenters_read(app,event.data);
			if(app->dc_count) {app->dc_notice="Selected server failed. Choose another server or go back to refresh.";datacenters_view(app);}
			else detail_view(app,"The service returned no eligible servers. Try again later.");
		}
		#endif
		else if(event.status==PLUS_VITA_RESULT_SAMPLE_SAVED)
			detail_view(app,"PS3 sample saved. Reopen FTP to retrieve it.");
		else if(event.status==PLUS_VITA_RESULT_SAMPLE_INCOMPLETE)
			detail_view(app,"PS3 sample incomplete. Reopen FTP to retrieve the diagnostic log.");
		else if(event.status==PLUS_VITA_RESULT_STREAM_STOPPED) stream_stopped_view(app);
		else if(event.status==PLUS_VITA_RESULT_SERVICE_ERROR)
#if PLUS_VITA_PS5_EXPERIMENTAL
			detail_view(app,"Server discovery failed before a list was received. Try Play again.");
#else
			detail_view(app,"The cloud service could not start this session.\nTry Play again.");
#endif
		else if(event.status==PLUS_VITA_RESULT_UNSUPPORTED)
			detail_view(app,"This title is not supported for cloud Play on this client.\nChoose another game.");
		else detail_view(app, event.status==PLUS_VITA_RESULT_CANCELLED
			? "Play was cancelled. Select Play for a fresh attempt."
			: event.status==PLUS_VITA_RESULT_BUSY
			? "The previous connection is still closing.\nTry Play again shortly."
			: "The cloud session stopped or could not start.\nCheck Wi-Fi and select Play to retry.");
		if(app->view.screen!=PLUS_VITA_APP_DATACENTERS &&
			(event.status==PLUS_VITA_RESULT_SERVICE_ERROR||event.status==PLUS_VITA_RESULT_NETWORK_ERROR||
			event.status==PLUS_VITA_RESULT_UNSUPPORTED||event.status==PLUS_VITA_RESULT_STREAM_STOPPED||
			(event.status==PLUS_VITA_RESULT_DATACENTERS&&app->view.screen==PLUS_VITA_APP_DETAILS)))
			attach_stream_diagnostic(app);
		return effect;
	}
	if(event.kind==PLUS_VITA_EVENT_PLAY_PROGRESS)
	{
		if(app->waiting_for!=PLUS_VITA_EFFECT_PLAY || !event.operation || event.operation!=app->pending_operation) return effect;
		static const char *const stages[] = {
			"Preparing a cloud session. Circle cancels.",
			"Checking your account. Circle cancels.",
			"Checking this game's eligibility. Circle cancels.",
			"Finding a cloud server. Circle cancels.",
			"Allocating your cloud session. Circle cancels.",
			"Connecting to cloud video. Hold START + SELECT to stop after play begins.",
			[PLUS_VITA_PLAY_STAGE_PS3_SAMPLE] =
				"Collecting a PS3 stream sample for up to 60 seconds.\nNo gameplay will appear. Circle cancels."
		};
		if(event.play_stage < sizeof(stages)/sizeof(stages[0])) view->body=stages[event.play_stage];
		return effect;
	}
#if PLUS_VITA_ADAPTIVE_STREAM
	if(event.kind==PLUS_VITA_EVENT_PLATFORM_FILTER && view->screen==PLUS_VITA_APP_CATALOGUE) {
		app->platform_filter=pv_catalog_next(app->platform_filter);
		app->search_focus=false;
		filter_catalogue(app);catalogue_view(app);return effect;
	}
#endif
	if(event.kind==PLUS_VITA_EVENT_SEARCH && view->screen==PLUS_VITA_APP_CATALOGUE) {
		if(PLUS_VITA_ADAPTIVE_STREAM || app->catalogue_count){
			app->search_focus=true;
			return operation(app,PLUS_VITA_EFFECT_EDIT_SEARCH,"Opening title search.",true);
		}
	}
	if(event.kind == PLUS_VITA_EVENT_LABEL_ENTERED)
	{
		if(app->waiting_for != PLUS_VITA_EFFECT_EDIT_LABEL || !event.operation || event.operation != app->pending_operation) return effect;
		app->pending_operation = 0; app->waiting_for = PLUS_VITA_EFFECT_NONE;
		if(event.status == PLUS_VITA_RESULT_CANCELLED) { account_view(app); return effect; }
		bool valid = event.status == PLUS_VITA_RESULT_OK && plus_vita_utf8_label(event.data, event.data_size);
		if(!valid) { account_error(app, "The label could not be entered.\nUse up to 32 characters and try again from Account."); return effect; }
		memset(app->label, 0, sizeof(app->label)); memcpy(app->label, event.data, event.data_size);
		if(app->saved) { app->storage_return = PLUS_VITA_APP_ACCOUNT; return operation(app, PLUS_VITA_EFFECT_SAVE_ACCOUNT, "Saving your account label.", false); }
		account_view(app); return effect;
	}
	if(event.kind == PLUS_VITA_EVENT_SETUP_READY)
	{
		if(view->screen == PLUS_VITA_APP_PREPARING && event.operation
			&& event.operation == app->pending_operation)
		{
			app->pending_operation = 0;
			*view = (PlusVitaAppView){
				.screen = PLUS_VITA_APP_SETUP_GUIDE, .title = "Sign in on your phone or PC.",
				.subtitle = "YOUR SONY ACCOUNT", .hint = "X  Select     O  Back",
				.body =
#if PLUS_VITA_LAN_LOGIN
				"Connect your phone and Vita to the same trusted Wi-Fi. Choose Continue, then Sign in with QR code. Complete Sony sign-in on your phone.",
#elif PLUS_VITA_QR_PAIRING
				"Sign in on your phone or PC. Choose Continue, then Pair with QR code or Import JSON.\nComplete Sony's sign-in prompts on your own device.",
#else
				"1. Sign in at playstation.com on your phone or PC.\n2. Open ca.account.sony.com/api/v1/ssocookie.\n3. Save the response as plus-vita-auth.json.\nKeep this file private. Complete Sony's sign-in prompts.",
#endif
				.actions = {
					{ PLUS_VITA_ACTION_CONTINUE, "Continue", 64, 380, 832, 52 },
					{ PLUS_VITA_ACTION_BACK, "Back", 64, 444, 832, 52 }
				}, .action_count = 2
			};
		}
		return effect;
	}
	if((view->screen==PLUS_VITA_APP_BINDINGS || view->screen==PLUS_VITA_APP_BIND_ACTIONS)
		&& (event.kind==PLUS_VITA_EVENT_DOWN || event.kind==PLUS_VITA_EVENT_UP
		|| event.kind==PLUS_VITA_EVENT_SCROLL || event.kind==PLUS_VITA_EVENT_FOCUS)) {
		bool destinations=view->screen==PLUS_VITA_APP_BIND_ACTIONS;
		size_t *focus=destinations ? &app->action_focus : &app->binding_focus;
		size_t *first=destinations ? &app->action_first : &app->binding_first;
		move_list(view->list_total,view->list_count,focus,first,event);
		bindings_view(app,destinations);return effect;
	}
	if(event.kind == PLUS_VITA_EVENT_TOUCH)
	{
		PlusVitaAppView visible=plus_vita_app_view(app);
		for(size_t i=0;i<visible.action_count;i++)
		{
			const PlusVitaAppButton *button=&visible.actions[i];
			if(event.x >= button->x && event.x < button->x + button->width
				&& event.y >= button->y && event.y < button->y + button->height)
			{
				view->selected = view->list_total ? i : visible.list_first+i;
				if(view->screen==PLUS_VITA_APP_BINDINGS && i<view->list_count) app->binding_focus=view->list_first+i;
				if(view->screen==PLUS_VITA_APP_BIND_ACTIONS && i<view->list_count) app->action_focus=view->list_first+i;
				if(view->screen == PLUS_VITA_APP_CATALOGUE && button->id == PLUS_VITA_ACTION_GAME)
				{
					bool retained=!app->search_focus && app->catalogue_selection==view->list_first+i;
					app->catalogue_selection=view->list_first+i;app->search_focus=false;catalogue_view(app);
					if(!retained) return effect;
				}
				return plus_vita_app_dispatch(app,
					(PlusVitaAppEvent){ .kind = PLUS_VITA_EVENT_ACTIVATE });
			}
		}
		return effect;
	}
#if PLUS_VITA_ADAPTIVE_STREAM
	if(view->screen==PLUS_VITA_APP_CATALOGUE &&
		(event.kind==PLUS_VITA_EVENT_DOWN||event.kind==PLUS_VITA_EVENT_UP||event.kind==PLUS_VITA_EVENT_SCROLL||event.kind==PLUS_VITA_EVENT_FOCUS)) {
		if(!app->matches_count){
			if(event.kind==PLUS_VITA_EVENT_DOWN && view->selected+1<view->action_count)++view->selected;
			else if(event.kind==PLUS_VITA_EVENT_UP && view->selected)--view->selected;
			return effect;
		}
		if(event.kind==PLUS_VITA_EVENT_UP && (app->search_focus||!app->catalogue_selection)){
			app->search_focus=true;catalogue_view(app);return effect;
		}
		if(event.kind==PLUS_VITA_EVENT_DOWN && app->search_focus){
			app->search_focus=false;catalogue_view(app);return effect;
		}
	}
#endif
	if(view->screen==PLUS_VITA_APP_CATALOGUE && app->matches_count
		&& (event.kind==PLUS_VITA_EVENT_DOWN || event.kind==PLUS_VITA_EVENT_UP
		|| event.kind==PLUS_VITA_EVENT_SCROLL || event.kind==PLUS_VITA_EVENT_FOCUS)) {
		if(!app->search_focus || event.kind==PLUS_VITA_EVENT_SCROLL || event.kind==PLUS_VITA_EVENT_FOCUS)
			move_list(app->matches_count,view->list_count,&app->catalogue_selection,&app->catalogue_first,event);
		app->search_focus=false;catalogue_view(app);
	}
	else if((event.kind==PLUS_VITA_EVENT_DOWN || event.kind==PLUS_VITA_EVENT_UP
		|| event.kind==PLUS_VITA_EVENT_SCROLL) && view->action_count) {
		PlusVitaAppView visible=plus_vita_app_view(app);
		move_list(visible.list_total,visible.list_count,&view->selected,&view->list_first,event);
	}
	else if(event.kind == PLUS_VITA_EVENT_BACK)
	{
#if PLUS_VITA_ADAPTIVE_STREAM
		if(view->screen==PLUS_VITA_APP_UPDATES){
            if(app->update.state==PV_UP_INSTALLING)return effect;
            if(update_busy(app->update.state))return update_effect(app,PV_UP_CMD_CANCEL);
            app->pending_operation=0;settings_view(app);return effect;
        }
		if(app->waiting_for==PLUS_VITA_EFFECT_EDIT_PRIMARY||app->waiting_for==PLUS_VITA_EFFECT_EDIT_SECONDARY){app->pending_operation=0;app->waiting_for=PLUS_VITA_EFFECT_NONE;theme_view(app,NULL);return effect;}
#endif
		if(controls_screen(view->screen)) return edit_controls(app,PLUS_VITA_ACTION_BACK);
#if PLUS_VITA_FONT_COMPARISON
		if(view->screen==PLUS_VITA_APP_FONT_COMPARISON) {
			about_view(app); return effect;
		}
#endif
		if(app->waiting_for == PLUS_VITA_EFFECT_SAVE_ACCOUNT || app->waiting_for == PLUS_VITA_EFFECT_DELETE_ACCOUNT
			|| app->waiting_for == PLUS_VITA_EFFECT_ENABLE_AUTO) return effect;
		if(view->screen == PLUS_VITA_APP_FORGET_CONFIRM) { account_view(app); return effect; }
		if(view->screen == PLUS_VITA_APP_CONTROLS || view->screen == PLUS_VITA_APP_REPORTING || view->screen == PLUS_VITA_APP_STREAM_SETTINGS || view->screen == PLUS_VITA_APP_SECURITY || view->screen==PLUS_VITA_APP_THEME
			|| (view->screen == PLUS_VITA_APP_ACCOUNT && app->account_from_settings)) {
			settings_view(app); return effect;
		}
		#if PLUS_VITA_PS5_EXPERIMENTAL
		if(view->screen==PLUS_VITA_APP_DATACENTERS) { detail_view(app,NULL); return effect; }
		#endif
		if(view->screen==PLUS_VITA_APP_DETAILS || view->screen==PLUS_VITA_APP_STREAM_STOPPED
			|| app->waiting_for==PLUS_VITA_EFFECT_EDIT_SEARCH || app->waiting_for==PLUS_VITA_EFFECT_PLAY) {
			app->pending_operation=0; app->waiting_for=PLUS_VITA_EFFECT_NONE; catalogue_view(app); return effect;
		}
		app->pending_operation = 0;
		app->waiting_for = PLUS_VITA_EFFECT_NONE;
		if(!app->connected) plus_vita_credential_clear(&app->credential);
		app->retry_account = app->retry_save = false;
		if(view->screen == PLUS_VITA_APP_ERROR && !app->connected && !app->saved) setup_view(app);
		else if(view->screen == PLUS_VITA_APP_WELCOME)
		{
			app->welcome_selection = view->selected;
			confirm_exit(app);
		}
		else welcome(app);
	}
	else if(event.kind == PLUS_VITA_EVENT_ACTIVATE && view->action_count)
	{
		PlusVitaAppAction action = view->actions[view->selected].id;
		if(action==PLUS_VITA_ACTION_CONTROL_LAYOUT || controls_screen(view->screen)) return edit_controls(app,action);
#if PLUS_VITA_FONT_COMPARISON
		if(action==PLUS_VITA_ACTION_FONT_COMPARISON || action==PLUS_VITA_ACTION_FONT_TOGGLE || action==PLUS_VITA_ACTION_FONT_SAMPLE) {
			if(action==PLUS_VITA_ACTION_FONT_TOGGLE) app->alternative_font=!app->alternative_font;
			if(action==PLUS_VITA_ACTION_FONT_SAMPLE) app->comparison_page^=1;
			font_comparison(app); return effect;
		}
#endif

		if(action==PLUS_VITA_ACTION_SEARCH) return plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_SEARCH});
		else if(action==PLUS_VITA_ACTION_PLATFORM_FILTER) return plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_PLATFORM_FILTER});
#if PLUS_VITA_ADAPTIVE_STREAM
		else if(action>=PLUS_VITA_ACTION_FILTER_ALL && action<=PLUS_VITA_ACTION_FILTER_PS5 && view->screen==PLUS_VITA_APP_CATALOGUE){
			app->platform_filter=(unsigned)(action-PLUS_VITA_ACTION_FILTER_ALL);app->search_focus=false;
			filter_catalogue(app);catalogue_view(app);return effect;
		}
#endif
		else if(action==PLUS_VITA_ACTION_CLEAR_SEARCH) { app->search[0]=0; filter_catalogue(app); app->search_focus=false; catalogue_view(app); }
		else if(action==PLUS_VITA_ACTION_GAME && view->screen==PLUS_VITA_APP_CATALOGUE) {
		detail_view(app,NULL);
#if PLUS_VITA_ADAPTIVE_STREAM
		if(app->view.detail&&app->view.detail->launch_id_valid)
			return plus_vita_app_dispatch(app,(PlusVitaAppEvent){.kind=PLUS_VITA_EVENT_ACTIVATE});
#endif
		}
		#if PLUS_VITA_PS5_EXPERIMENTAL
		else if(view->screen==PLUS_VITA_APP_DATACENTERS) {
			if(action==PLUS_VITA_ACTION_BACK) detail_view(app,NULL);
			else if(action==PLUS_VITA_ACTION_DC_PREVIOUS && app->dc_page) { app->dc_page--; datacenters_view(app); }
			else if(action==PLUS_VITA_ACTION_DC_NEXT && (app->dc_page+1)*3<app->dc_count) { app->dc_page++; datacenters_view(app); }
			else if(action==PLUS_VITA_ACTION_BITRATE) { app->bitrate_kbps=plus_vita_next_bitrate(app->bitrate_kbps); datacenters_view(app); }
			else if(action==PLUS_VITA_ACTION_HUD) { app->hud=!app->hud; datacenters_view(app); }
			else if(action==PLUS_VITA_ACTION_DATACENTER) {
				size_t index=app->dc_page*3+view->selected;
				if(index>=app->dc_count) return effect;
				PlusVitaCatalogEntry game=app->catalogue[app->matches[app->catalogue_selection]];
				effect=operation(app,PLUS_VITA_EFFECT_PLAY,"Connecting to your selected cloud server. Circle cancels.",true);
				effect.game=game; effect.controls=app->controls;
				snprintf(effect.datacenter,sizeof(effect.datacenter),"%s",app->datacenters[index]);
				effect.bitrate_kbps=app->bitrate_kbps;
				effect.profile_kbps=app->bitrate_kbps;effect.hud=app->hud;
				app->requested_kbps=effect.bitrate_kbps;app->auto_retry=false;
				snprintf(app->selected_dc,sizeof(app->selected_dc),"%s",effect.datacenter);
				return effect;
			}
			return effect;
		}
		#endif
		else if(action==PLUS_VITA_ACTION_PLAY && (view->screen==PLUS_VITA_APP_DETAILS
			|| view->screen==PLUS_VITA_APP_STREAM_STOPPED || (PLUS_VITA_ADAPTIVE_STREAM && view->screen==PLUS_VITA_APP_CATALOGUE && view->detail && view->detail->launch_id_valid))) {
			PlusVitaCatalogEntry game=*view->detail;
			effect=operation(app,PLUS_VITA_EFFECT_PLAY,"Preparing a cloud session. Circle cancels.",true);
			effect.game=game;effect.controls=app->controls;
#if PLUS_VITA_PS5_EXPERIMENTAL
			app->dc_count=app->dc_page=0;app->dc_notice=NULL;app->selected_dc[0]=0;app->auto_retry=false;
			snprintf(effect.datacenter,sizeof(effect.datacenter),"%s",PLUS_VITA_DC_DISCOVER);
			effect.bitrate_kbps=app->bitrate_kbps ? app->bitrate_kbps : (PLUS_VITA_ADAPTIVE_STREAM ? 15000 : 8000);
			effect.profile_kbps=effect.bitrate_kbps;effect.hud=app->hud;
#endif
			return effect;
		}
		else if(action == PLUS_VITA_ACTION_CATALOGUE)
		{
			app->welcome_selection = view->selected;
			catalogue_view(app);
		}
		
#if PLUS_VITA_LAN_LOGIN
		else if(action==PLUS_VITA_ACTION_LAN_LOGIN&&view->screen==PLUS_VITA_APP_SETUP){
			effect=import_account(app);effect.kind=PLUS_VITA_EFFECT_LAN_LOGIN;
			view->body="Starting local QR sign-in. Circle cancels.";return effect;
		}
#else
		else if(action == PLUS_VITA_ACTION_IMPORT)
			return import_account(app);
#if PLUS_VITA_QR_PAIRING
		else if((action==PLUS_VITA_ACTION_SONY_LOGIN||action==PLUS_VITA_ACTION_TOKEN_LOGIN)&&view->screen==PLUS_VITA_APP_SETUP) {
			effect=import_account(app);effect.kind=action==PLUS_VITA_ACTION_SONY_LOGIN?PLUS_VITA_EFFECT_SONY_LOGIN:PLUS_VITA_EFFECT_TOKEN_LOGIN;
			view->body="Sign in privately in Sony's browser. Circle cancels.";return effect;
		}
		else if((action == PLUS_VITA_ACTION_QR_PAIR||action==PLUS_VITA_ACTION_REVERSE_QR) && view->screen==PLUS_VITA_APP_SETUP) {
			effect=import_account(app);
			effect.kind=action==PLUS_VITA_ACTION_REVERSE_QR?PLUS_VITA_EFFECT_REVERSE_QR:PLUS_VITA_EFFECT_QR_PAIR;
			view->body="Pairing with your phone. Circle cancels.";
			return effect;
		}
#endif
#endif
		else if(action == PLUS_VITA_ACTION_REMEMBER)
		{
			if(view->screen == PLUS_VITA_APP_ACCOUNT)
			{
				if(app->retry_delete) return remove_saved(app, app->forgetting, app->expired);
				if(app->saved) return remove_saved(app, false, false);
				app->storage_return = PLUS_VITA_APP_ACCOUNT;
				return operation(app, PLUS_VITA_EFFECT_SAVE_ACCOUNT, "Saving sign-in on this Vita.", false);
			}
			if(app->remember) {
				PlusVitaAppEffect disable = remove_saved(app, false, false);
				app->storage_return = PLUS_VITA_APP_SETUP;
				return disable;
			}
			return operation(app, PLUS_VITA_EFFECT_ENABLE_AUTO, "Enabling remembered sign-in.", false);
		}
		else if(action == PLUS_VITA_ACTION_RESTORE)
			return operation(app, PLUS_VITA_EFFECT_LOAD_ACCOUNT, "Loading your remembered account.", true);
		else if(action == PLUS_VITA_ACTION_RETRY) {
			effect=operation(app, app->retry_delete ? PLUS_VITA_EFFECT_DELETE_ACCOUNT : app->retry_save ? PLUS_VITA_EFFECT_SAVE_ACCOUNT : PLUS_VITA_EFFECT_AUTHORIZE,
				app->retry_delete ? "Removing saved sign-in from this Vita." : app->retry_save ? "Saving your account on this Vita." : "Checking your sign-in and loading cloud games.", !app->retry_save && !app->retry_delete);
			effect.fresh_attempt=effect.kind==PLUS_VITA_EFFECT_AUTHORIZE;
			return effect;
		}
		else if(action == PLUS_VITA_ACTION_ACCOUNT) { app->account_from_settings = view->screen == PLUS_VITA_APP_SETTINGS; account_view(app); }
		else if(action == PLUS_VITA_ACTION_SETTINGS) { app->welcome_selection = view->selected; settings_view(app); }
#if PLUS_VITA_ADAPTIVE_STREAM
#if PLUS_VITA_ACCOUNT_VAULT
		else if(action==PLUS_VITA_ACTION_SECURITY && view->screen==PLUS_VITA_APP_SETTINGS)security_view(app,NULL);
		else if(view->screen==PLUS_VITA_APP_SECURITY && app->connected &&
			(action==PLUS_VITA_ACTION_VAULT_SAVE||action==PLUS_VITA_ACTION_VAULT_CHANGE||action==PLUS_VITA_ACTION_VAULT_CLEAN_IMPORT||action==PLUS_VITA_ACTION_VAULT_TOGGLE)){
			app->storage_return=PLUS_VITA_APP_SECURITY;
			effect=operation(app,PLUS_VITA_EFFECT_SAVE_ACCOUNT,"Saving sign-in on this Vita.",false);
			effect.vault_change=action==PLUS_VITA_ACTION_VAULT_CHANGE||action==PLUS_VITA_ACTION_VAULT_TOGGLE;
			if(action==PLUS_VITA_ACTION_VAULT_TOGGLE)effect.vault_target=app->vault_mode==2?3:2;
			effect.vault_clean_import=action==PLUS_VITA_ACTION_VAULT_CLEAN_IMPORT;return effect;
		}
#endif
        else if(action==PLUS_VITA_ACTION_UPDATES&&view->screen==PLUS_VITA_APP_SETTINGS)update_view(app);
        else if(view->screen==PLUS_VITA_APP_UPDATES){
            if(action==PLUS_VITA_ACTION_UPDATE_NOTES){
                if((size_t)(++app->update_note_page)*216>=strlen(app->update.notes))app->update_note_page=0;
                update_view(app);return effect;
            }
            if(action==PLUS_VITA_ACTION_BACK){settings_view(app);return effect;}
            if(action==PLUS_VITA_ACTION_UPDATE_CANCEL)return update_effect(app,PV_UP_CMD_CANCEL);
            if(action==PLUS_VITA_ACTION_UPDATE_CHANNEL)return update_effect(app,PV_UP_CMD_CHANNEL);
            if(action==PLUS_VITA_ACTION_UPDATE_AUTO)return update_effect(app,PV_UP_CMD_AUTO);
            if(action==PLUS_VITA_ACTION_UPDATE_CHECK)return update_effect(app,PV_UP_CMD_CHECK);
            if(action==PLUS_VITA_ACTION_UPDATE_DOWNLOAD)return update_effect(app,PV_UP_CMD_DOWNLOAD);
            if(action==PLUS_VITA_ACTION_UPDATE_INSTALL)return update_effect(app,PV_UP_CMD_INSTALL);
            return effect;
        }
        else if(action==PLUS_VITA_ACTION_THEME && view->screen==PLUS_VITA_APP_SETTINGS)theme_view(app,NULL);
        else if(view->screen==PLUS_VITA_APP_THEME&&(action==PLUS_VITA_ACTION_PRIMARY||action==PLUS_VITA_ACTION_SECONDARY))
            return operation(app,action==PLUS_VITA_ACTION_PRIMARY?PLUS_VITA_EFFECT_EDIT_PRIMARY:PLUS_VITA_EFFECT_EDIT_SECONDARY,"Enter a color (RRGGBB).",true);
        else if(view->screen==PLUS_VITA_APP_THEME&&action==PLUS_VITA_ACTION_THEME_RESET){
            app->theme_custom=false;app->theme_primary=PV_PRIMARY_DEFAULT;app->theme_secondary=PV_SECONDARY_DEFAULT;theme_view(app,NULL);
        }
		else if(action==PLUS_VITA_ACTION_STREAM_SETTINGS && view->screen==PLUS_VITA_APP_SETTINGS) stream_settings_view(app);
		else if(view->screen==PLUS_VITA_APP_STREAM_SETTINGS && (action==PLUS_VITA_ACTION_STREAM_LOW||action==PLUS_VITA_ACTION_STREAM_BALANCED||action==PLUS_VITA_ACTION_STREAM_MAX)){
			app->bitrate_kbps=action==PLUS_VITA_ACTION_STREAM_LOW?6000:action==PLUS_VITA_ACTION_STREAM_BALANCED?10000:15000;
			stream_settings_view(app);
		}
		else if(action==PLUS_VITA_ACTION_LANGUAGE&&view->screen==PLUS_VITA_APP_SETTINGS){
			app->ui_language=(pv_language(app->ui_language)+1)%PV_LANG_COUNT;settings_view(app);view->selected=5;
		}
		else if(action==PLUS_VITA_ACTION_REGION&&(view->screen==PLUS_VITA_APP_SETUP||view->screen==PLUS_VITA_APP_SETTINGS)){
			app->region_mode=(pv_region_mode(app->region_mode)+1)%2;
			if(view->screen==PLUS_VITA_APP_SETUP)setup_view(app);else settings_view(app);
		}
#endif
		else if(action == PLUS_VITA_ACTION_CONTROLS) controls_view(app);
		else if(action == PLUS_VITA_ACTION_REPORTING) reporting_view(app);
		else if(action == PLUS_VITA_ACTION_LABEL)
			return operation(app, PLUS_VITA_EFFECT_EDIT_LABEL, "Opening the account-label keyboard.", true);
		else if(action == PLUS_VITA_ACTION_FORGET)
		{
			app->view = (PlusVitaAppView){ .screen = PLUS_VITA_APP_FORGET_CONFIRM, .title = "Forget this account?",
				.subtitle = "ACCOUNT", .body =
#if PLUS_VITA_LAN_LOGIN
				"This clears active sign-in and saved account data.\nUse QR sign-in to connect again.",
#else
				"This clears active sign-in and saved account data.\nYour JSON stays in place, but automatic sign-in is paused.\nChoose Import JSON explicitly to connect again.",
#endif
				.hint = "X  Select     O  Back",
				.actions = {{ PLUS_VITA_ACTION_BACK, "Keep account", 64, 380, 832, 52 },
				{ PLUS_VITA_ACTION_CONFIRM_FORGET, "Forget account", 64, 444, 832, 52 }}, .action_count = 2 };
		}
		else if(action == PLUS_VITA_ACTION_CONFIRM_FORGET) return remove_saved(app, true, false);
		else if(action == PLUS_VITA_ACTION_CONTINUE) setup_view(app);
		else if(action == PLUS_VITA_ACTION_SETUP)
		{
			app->welcome_selection = view->selected;
			app->pending_operation = ++app->next_operation;
			*view = (PlusVitaAppView){
				.screen = PLUS_VITA_APP_PREPARING,
				.title = "Getting things ready.", .subtitle = "ACCOUNT SETUP",
				.body = "Preparing account setup. You can cancel at any time.",
				.hint = "O  Cancel",
				.actions = {{ PLUS_VITA_ACTION_BACK, "Cancel", 64, 444, 500, 52 }},
				.action_count = 1
			};
			view->operation = app->pending_operation;
			effect = (PlusVitaAppEffect){ .kind = PLUS_VITA_EFFECT_PREPARE_SETUP, .operation = app->pending_operation };
		}
		else if(action == PLUS_VITA_ACTION_ABOUT)
		{
			app->welcome_selection = view->selected;
			about_view(app);
		}
		else if(action == PLUS_VITA_ACTION_BACK)
		{
			if(view->screen==PLUS_VITA_APP_FONT_COMPARISON) { about_view(app); return effect; }
			if(view->screen==PLUS_VITA_APP_DETAILS || view->screen==PLUS_VITA_APP_STREAM_STOPPED) { catalogue_view(app); return effect; }
			if(view->screen==PLUS_VITA_APP_CONTROLS || view->screen==PLUS_VITA_APP_REPORTING || view->screen==PLUS_VITA_APP_STREAM_SETTINGS || view->screen==PLUS_VITA_APP_SECURITY || view->screen==PLUS_VITA_APP_THEME
				|| (view->screen==PLUS_VITA_APP_ACCOUNT && app->account_from_settings)) { settings_view(app); return effect; }
			app->pending_operation = 0;
			app->waiting_for = PLUS_VITA_EFFECT_NONE;
			if(!app->connected) plus_vita_credential_clear(&app->credential);
			if(view->screen == PLUS_VITA_APP_ERROR && !app->connected) setup_view(app);
			else welcome(app);
		}
		else if(action == PLUS_VITA_ACTION_EXIT)
		{
			if(view->screen == PLUS_VITA_APP_EXIT_CONFIRM)
			{
				*view = (PlusVitaAppView){ .screen = PLUS_VITA_APP_CLOSED };
				effect.kind = PLUS_VITA_EFFECT_EXIT;
			}
			else
			{
				app->welcome_selection = view->selected;
				confirm_exit(app);
			}
		}
	}
	return effect;
}
