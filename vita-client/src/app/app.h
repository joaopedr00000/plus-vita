// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_APP_H
#define PLUS_VITA_APP_H
#include <stddef.h>
#include <stdint.h>
#include "../platform/auth_probe.h"
#include "../platform/control_layout.h"

typedef struct plus_vita_app_t PlusVitaApp;
typedef enum {
	PLUS_VITA_APP_WELCOME,
	PLUS_VITA_APP_ABOUT,
	PLUS_VITA_APP_PREPARING,
	PLUS_VITA_APP_SETUP_GUIDE,
	PLUS_VITA_APP_SETUP,
	PLUS_VITA_APP_AUTHORIZING,
	PLUS_VITA_APP_CATALOGUE,
	PLUS_VITA_APP_ERROR,
	PLUS_VITA_APP_EXIT_CONFIRM,
	PLUS_VITA_APP_CLOSED, PLUS_VITA_APP_ACCOUNT, PLUS_VITA_APP_FORGET_CONFIRM, PLUS_VITA_APP_DETAILS,
	PLUS_VITA_APP_FONT_COMPARISON, PLUS_VITA_APP_SETTINGS, PLUS_VITA_APP_CONTROLS,
	PLUS_VITA_APP_REPORTING, PLUS_VITA_APP_STREAM_STOPPED,
	PLUS_VITA_APP_CONTROL_LAYOUT, PLUS_VITA_APP_BINDINGS, PLUS_VITA_APP_BIND_ACTIONS,
	PLUS_VITA_APP_DATACENTERS, PLUS_VITA_APP_STREAM_SETTINGS, PLUS_VITA_APP_SECURITY
} PlusVitaAppScreen;
typedef enum {
	PLUS_VITA_ACTION_SETUP,
	PLUS_VITA_ACTION_CATALOGUE,
	PLUS_VITA_ACTION_ABOUT,
	PLUS_VITA_ACTION_EXIT,
	PLUS_VITA_ACTION_BACK,
	PLUS_VITA_ACTION_IMPORT,
	PLUS_VITA_ACTION_GAME, PLUS_VITA_ACTION_CONTINUE,
	PLUS_VITA_ACTION_REMEMBER, PLUS_VITA_ACTION_ACCOUNT, PLUS_VITA_ACTION_RESTORE,
	PLUS_VITA_ACTION_RETRY, PLUS_VITA_ACTION_LABEL, PLUS_VITA_ACTION_FORGET, PLUS_VITA_ACTION_CONFIRM_FORGET,
	PLUS_VITA_ACTION_SEARCH, PLUS_VITA_ACTION_CLEAR_SEARCH, PLUS_VITA_ACTION_PLAY, PLUS_VITA_ACTION_DISABLED,
	PLUS_VITA_ACTION_PREVIOUS_PAGE, PLUS_VITA_ACTION_NEXT_PAGE,
	PLUS_VITA_ACTION_FONT_COMPARISON, PLUS_VITA_ACTION_FONT_TOGGLE, PLUS_VITA_ACTION_FONT_SAMPLE,
	PLUS_VITA_ACTION_SETTINGS, PLUS_VITA_ACTION_CONTROLS, PLUS_VITA_ACTION_REPORTING,
	PLUS_VITA_ACTION_CONTROL_LAYOUT, PLUS_VITA_ACTION_BINDINGS, PLUS_VITA_ACTION_BINDING,
	PLUS_VITA_ACTION_BIND_ACTION, PLUS_VITA_ACTION_FRONT_MOTION, PLUS_VITA_ACTION_CONTROL_DEFAULTS,
	PLUS_VITA_ACTION_SAVE_CONTROLS, PLUS_VITA_ACTION_CANCEL_CONTROLS,
	PLUS_VITA_ACTION_BIND_PREVIOUS, PLUS_VITA_ACTION_BIND_NEXT,
	PLUS_VITA_ACTION_DATACENTER, PLUS_VITA_ACTION_DC_PREVIOUS, PLUS_VITA_ACTION_DC_NEXT, PLUS_VITA_ACTION_BITRATE, PLUS_VITA_ACTION_QR_PAIR, PLUS_VITA_ACTION_HUD, PLUS_VITA_ACTION_REGION, PLUS_VITA_ACTION_SONY_LOGIN, PLUS_VITA_ACTION_TOKEN_LOGIN, PLUS_VITA_ACTION_REVERSE_QR, PLUS_VITA_ACTION_LAN_LOGIN, PLUS_VITA_ACTION_LANGUAGE, PLUS_VITA_ACTION_PLATFORM_FILTER, PLUS_VITA_ACTION_STREAM_SETTINGS, PLUS_VITA_ACTION_STREAM_LOW, PLUS_VITA_ACTION_STREAM_BALANCED, PLUS_VITA_ACTION_STREAM_MAX, PLUS_VITA_ACTION_SECURITY, PLUS_VITA_ACTION_VAULT_SAVE, PLUS_VITA_ACTION_VAULT_CHANGE, PLUS_VITA_ACTION_VAULT_CLEAN_IMPORT, PLUS_VITA_ACTION_VAULT_TOGGLE
} PlusVitaAppAction;
typedef struct {
	PlusVitaAppAction id;
	const char *label;
	int x, y, width, height;
} PlusVitaAppButton;
typedef struct {
	PlusVitaAppScreen screen;
	const char *title, *subtitle, *body, *hint;
	PlusVitaAppButton actions[10];
	size_t action_count, selected;
	/* Stable list focus is independent of the bounded visible action index. */
	size_t list_total, list_first, list_count, focus;
	int list_x, list_y, list_width, list_height, row_step;
	uint64_t operation;
	const PlusVitaCatalogEntry *games[7], *detail;
	uint64_t account_epoch;
	unsigned artwork_metadata[PLUS_VITA_ARTWORK_METADATA_STATES];
	bool account_connected;
	bool alternative_font;
	unsigned comparison_page;
	bool touch_pressed;
	bool preview_eligible;
	size_t pressed_index;
	unsigned controls_page;
	PlusVitaControlDevice control_device;
	unsigned stream_profile;
	bool stream_hud;
	unsigned region_mode;
	unsigned ui_language;
	unsigned platform_filter;
} PlusVitaAppView;
typedef enum {
	PLUS_VITA_EVENT_PAD,
	PLUS_VITA_EVENT_UP, PLUS_VITA_EVENT_DOWN,
	PLUS_VITA_EVENT_ACTIVATE, PLUS_VITA_EVENT_BACK,
	PLUS_VITA_EVENT_TOUCH, PLUS_VITA_EVENT_SETUP_READY,
	PLUS_VITA_EVENT_IMPORT_READ,
	PLUS_VITA_EVENT_AUTHORIZED, PLUS_VITA_EVENT_START, PLUS_VITA_EVENT_ACCOUNT_LOADED,
	PLUS_VITA_EVENT_ACCOUNT_SAVED, PLUS_VITA_EVENT_ACCOUNT_DELETED, PLUS_VITA_EVENT_LABEL_ENTERED,
	PLUS_VITA_EVENT_PAD_RESET, PLUS_VITA_EVENT_AUTO_ENABLED, PLUS_VITA_EVENT_SEARCH_ENTERED, PLUS_VITA_EVENT_SEARCH, PLUS_VITA_EVENT_PLAY_ENDED,
	PLUS_VITA_EVENT_FONT_CAPTURED, PLUS_VITA_EVENT_LOG_STATUS, PLUS_VITA_EVENT_PLAY_PROGRESS,
	PLUS_VITA_EVENT_INTERRUPTED,
	PLUS_VITA_EVENT_TOUCH_DOWN, PLUS_VITA_EVENT_TOUCH_MOVE,
	PLUS_VITA_EVENT_TOUCH_UP, PLUS_VITA_EVENT_TOUCH_CANCEL,
	PLUS_VITA_EVENT_SCROLL, PLUS_VITA_EVENT_FOCUS,
	PLUS_VITA_EVENT_CONTROLS_LOADED, PLUS_VITA_EVENT_CONTROLS_SAVED, PLUS_VITA_EVENT_CONTROL_DEVICE, PLUS_VITA_EVENT_STREAM_SETTINGS
} PlusVitaAppEventKind;
typedef enum {
	PLUS_VITA_RESULT_OK, PLUS_VITA_RESULT_STORAGE_ERROR, PLUS_VITA_RESULT_MISSING,
	PLUS_VITA_RESULT_REJECTED, PLUS_VITA_RESULT_NETWORK_ERROR, PLUS_VITA_RESULT_CATALOGUE_ERROR,
	PLUS_VITA_RESULT_CANCELLED, PLUS_VITA_RESULT_BUSY, PLUS_VITA_RESULT_STREAM_STOPPED,
	PLUS_VITA_RESULT_SLEEP, PLUS_VITA_RESULT_SERVICE_ERROR, PLUS_VITA_RESULT_UNSUPPORTED,
	PLUS_VITA_RESULT_SAMPLE_SAVED, PLUS_VITA_RESULT_SAMPLE_INCOMPLETE,
	PLUS_VITA_RESULT_DATACENTERS, PLUS_VITA_RESULT_VAULT_LOCKED
} PlusVitaAppResult;

enum { PLUS_VITA_PLAY_STAGE_PS3_SAMPLE = 6 };

/* Vita button bits; the platform adapter verifies these against the SDK. */
enum {
	PLUS_VITA_MENU_UP = 0x10, PLUS_VITA_MENU_RIGHT = 0x20,
	PLUS_VITA_MENU_DOWN = 0x40, PLUS_VITA_MENU_LEFT = 0x80,
	PLUS_VITA_MENU_CIRCLE = 0x2000, PLUS_VITA_MENU_CROSS = 0x4000,
	PLUS_VITA_MENU_TRIANGLE = 0x1000
};
typedef struct {
	uint32_t buttons;
	unsigned char left_x, left_y;
	uint64_t time_us;
} PlusVitaAppPad;
typedef struct {
	PlusVitaAppEventKind kind;
	PlusVitaAppPad pad;
	int x, y;
	uint64_t operation;
	PlusVitaAppResult status;
	const char *data;
	size_t data_size;
	const PlusVitaCatalogEntry *entries;
	size_t entry_count;
	bool restore_blocked, retained_json;
	bool legacy_plain, vault_present, import_cleanup_failed;
	unsigned vault_mode; /* 2 password, 3 automatic */
	bool playback_started;
	bool regional_catalogue;
	unsigned stream_profile;
	bool stream_hud;
	unsigned region_mode;
	unsigned ui_language;
	bool log_available;
	unsigned play_stage;
	unsigned auth_stage;
	int auth_transport_error;
	int auth_curl_error;
	char store_country[8], store_language[8], language[8];
	char account_name[129];
	PlusVitaControlLayout controls;
	PlusVitaControlDevice device;
} PlusVitaAppEvent;
typedef enum {
	PLUS_VITA_EFFECT_NONE, PLUS_VITA_EFFECT_EXIT, PLUS_VITA_EFFECT_PREPARE_SETUP,
	PLUS_VITA_EFFECT_READ_IMPORT,
	PLUS_VITA_EFFECT_AUTHORIZE, PLUS_VITA_EFFECT_LOAD_ACCOUNT, PLUS_VITA_EFFECT_SAVE_ACCOUNT,
	PLUS_VITA_EFFECT_DELETE_ACCOUNT, PLUS_VITA_EFFECT_EDIT_LABEL, PLUS_VITA_EFFECT_ENABLE_AUTO,
	PLUS_VITA_EFFECT_EDIT_SEARCH, PLUS_VITA_EFFECT_PLAY, PLUS_VITA_EFFECT_SAVE_CONTROLS, PLUS_VITA_EFFECT_QR_PAIR, PLUS_VITA_EFFECT_SONY_LOGIN, PLUS_VITA_EFFECT_TOKEN_LOGIN, PLUS_VITA_EFFECT_REVERSE_QR, PLUS_VITA_EFFECT_LAN_LOGIN
} PlusVitaAppEffectKind;
/* Authorization effects contain sensitive material for the Sony adapter only.
 * Never log or render an effect/event; clear its credential after consumption. */
typedef struct {
	PlusVitaAppEffectKind kind;
	uint64_t operation;
	PlusVitaCredential credential;
	char label[129];
	bool forget;
	bool fresh_attempt;
	bool vault_change, vault_clean_import;
	unsigned vault_target; /* 0 keep/default automatic, 2 password, 3 automatic */
	/* Filled only by native UI immediately before storage worker dispatch. */
	PlusVitaCredential vault_password, vault_previous;
	PlusVitaCatalogEntry game;
	char datacenter[128]; /* Empty only in legacy builds. */
	unsigned bitrate_kbps;
	unsigned profile_kbps;
	bool hud;
	unsigned region_mode;
	unsigned ui_language;
	char store_country[8], store_language[8], language[8];
	PlusVitaControlLayout controls;
} PlusVitaAppEffect;

PlusVitaApp *plus_vita_app_create(void);
void plus_vita_app_destroy(PlusVitaApp *app);
PlusVitaAppView plus_vita_app_view(const PlusVitaApp *app);
PlusVitaAppEffect plus_vita_app_dispatch(PlusVitaApp *app, PlusVitaAppEvent event);
#endif
