// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_QR_PAIR_CRYPTO_H
#define PLUS_VITA_QR_PAIR_CRYPTO_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
void plus_vita_pair_clear(void *p,size_t len);
bool plus_vita_pair_decode(const uint8_t key[16],const unsigned char *payload,size_t len,char *json,size_t cap);
#endif
