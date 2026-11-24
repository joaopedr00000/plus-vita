// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_QR_PAIR_CRYPTO_H
#define PLUS_VITA_QR_PAIR_CRYPTO_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
void plus_vita_pair_clear(void *p,size_t len);
#define PLUS_VITA_PAIR_KEY_BYTES 32
#define PLUS_VITA_PAIR_PAYLOAD_LEN 172
/* Mix fresh kernel entropy with a local device discriminator; neither raw input is sent. */
bool plus_vita_pair_make_key(const uint8_t entropy[32],const uint8_t device[6],uint8_t key[32]);
bool plus_vita_pair_decode(const uint8_t key[32],const unsigned char *payload,size_t len,char *json,size_t cap);
#endif
