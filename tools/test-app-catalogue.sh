#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
dir=build/debug/app-catalogue-host
mkdir -p "$dir"
printf '#define JSON_C_HAVE_INTTYPES_H 1\n#define JSON_C_HAVE_STDINT_H 1\n' > "$dir/json_config.h"
cc -std=c11 -g -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -Ivita-client/src/app -Iexternal/json-c -I"$dir" vita-client/src/app/app.c vita-client/src/platform/control_layout.c \
 vita-client/src/app/utf8.c vita-client/src/platform/credential.c vita-client/tests/app_catalogue_test.c \
 -l:libjson-c.so.5 -o "$dir/test"
"$dir/test"

cc -std=c11 -Wall -Wextra -Werror vita-client/tests/test_catalog_filter.c -o "$dir/filter"
"$dir/filter"

# Exercise the actual Beta 3 list, not just the legacy search UI.
cc -std=c11 -g -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -DPLUS_VITA_ADAPTIVE_STREAM=1 -DPLUS_VITA_PS5_EXPERIMENTAL=1 -DPLUS_VITA_LAN_LOGIN=1 -DPLUS_VITA_QR_PAIRING=1 \
 -Ivita-client/src/app -Iexternal/json-c -I"$dir" vita-client/src/app/app.c vita-client/src/platform/control_layout.c \
 vita-client/src/app/utf8.c vita-client/src/platform/credential.c vita-client/tests/app_catalogue_filter_test.c \
 -l:libjson-c.so.5 -o "$dir/filter-ui"
"$dir/filter-ui"

# Exercise search together with the release catalogue platform filter and controls.
cc -std=c11 -g -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -DPLUS_VITA_ADAPTIVE_STREAM=1 -DPLUS_VITA_PS5_EXPERIMENTAL=1 -DPLUS_VITA_LAN_LOGIN=1 -DPLUS_VITA_QR_PAIRING=1 \
 -Ivita-client/src/app -Iexternal/json-c -I"$dir" vita-client/src/app/app.c vita-client/src/platform/control_layout.c \
 vita-client/src/app/utf8.c vita-client/src/platform/credential.c vita-client/tests/test_catalogue_search.c \
 -l:libjson-c.so.5 -o "$dir/search-ui"
"$dir/search-ui"
