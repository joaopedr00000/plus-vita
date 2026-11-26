// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_COVER_DOWNLOAD_H
#define PLUS_VITA_COVER_DOWNLOAD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdint.h>
#include "artwork_status.h"
bool plus_vita_cover_download(const char *url,unsigned char **data,size_t *size,const atomic_bool *cancel,PlusVitaArtworkResult *result);
bool plus_vita_cover_decode(const unsigned char *data,size_t size,uint32_t pixels[128*128]);
/* Bounded local images, same allocator/limits as cover art. */
bool plus_vita_theme_decode(const unsigned char *data,size_t size,uint32_t *pixels,unsigned width,unsigned height,bool crop);
#endif
