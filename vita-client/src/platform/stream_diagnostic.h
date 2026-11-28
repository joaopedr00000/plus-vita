// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_STREAM_DIAGNOSTIC_H
#define PV_STREAM_DIAGNOSTIC_H
#include <stdbool.h>
/* Numeric, bounded snapshot only. No server-provided text or identifiers. */
typedef struct {
    bool valid;
    unsigned phase, http, reason, startup;
    unsigned client_mode; /* actual envelope: 1 PC, 2 Portal; 0 legacy or unavailable */ /* startup = enum + 1; 0 unavailable */
    int native_error; /* -1 unavailable */
} PlusVitaStreamDiagnostic;
#endif
