// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_STORAGE_PATHS_H
#define PV_STORAGE_PATHS_H
#if defined(__vita__) && PLUS_VITA_ADAPTIVE_STREAM
#define PV_NEW_STORAGE 1
#define PV_ROOT "ux0:data/VitaPlus"
#define PV_ACCOUNT_DIR PV_ROOT "/Account"
#define PV_KEY_DIR "ur0:data/VitaPlus"
#define PV_IMPORT_FILE PV_ROOT "/Imports/plus-vita-auth.json"
#define PV_CONTROLS_FILE PV_ROOT "/Settings/controls.dat"
#define PV_THEME_FILE PV_ROOT "/Settings/theme.cfg"
#define PV_STREAM_FILE PV_ROOT "/Settings/stream.cfg"
#define PV_STREAM_OPTIONS_FILE PV_ROOT "/Settings/stream-options.cfg"
#define PV_COVER_DIR PV_ROOT "/Cache/Covers"
#define PV_WALL_SOURCE PV_ROOT "/Wallpapers"
#define PV_WALL_CACHE PV_ROOT "/Cache/Themes"
#define PV_PS5_CATALOG PV_ROOT "/Cache/PS5-Catalog"
#define PV_DIAG_DIR PV_ROOT "/Logs/"
#define PV_CAPTURE_DIR PV_ROOT "/Captures/"
#else
#define PV_NEW_STORAGE 0
#define PV_ACCOUNT_DIR "ux0:data/plus-vita"
#define PV_KEY_DIR "ur0:data/plus-vita"
#define PV_IMPORT_FILE "ux0:data/plus-vita-auth.json"
#define PV_CONTROLS_FILE "ux0:data/plus-vita-controls.dat"
#define PV_THEME_FILE "ux0:data/plus-vita-theme.cfg"
#define PV_STREAM_FILE "ux0:data/plus-vita-stream.cfg"
#define PV_STREAM_OPTIONS_FILE "ux0:data/plus-vita-stream-options.cfg"
#define PV_COVER_DIR "ux0:data/VitaPlus-Covers"
#define PV_WALL_SOURCE "ux0:data/VitaPlus-Wallpapers"
#define PV_WALL_CACHE "ux0:data/VitaPlus-Themes-Cache"
#define PV_PS5_CATALOG "ux0:data/plus-vita-ps5-catalog"
#define PV_DIAG_DIR "ux0:data/"
#define PV_CAPTURE_DIR "ux0:data/"
#endif
#endif
