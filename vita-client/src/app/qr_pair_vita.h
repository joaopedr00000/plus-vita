// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_QR_PAIR_VITA_H
#define PLUS_VITA_QR_PAIR_VITA_H
#include "app.h"
typedef bool (*PlusVitaPairFrame)(void *,const char *,const char *,const unsigned char *,int,const uint32_t *);
PlusVitaAppResult plus_vita_qr_pair(PlusVitaPairFrame frame,void *context,char *json,size_t cap);
#endif
