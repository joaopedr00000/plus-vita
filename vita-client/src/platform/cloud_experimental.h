// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_CLOUD_EXPERIMENTAL_H
#define PLUS_VITA_CLOUD_EXPERIMENTAL_H
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "adaptive_stream.h"
#ifndef PLUS_VITA_PS5_EXPERIMENTAL
#define PLUS_VITA_PS5_EXPERIMENTAL 0
#endif
#define PLUS_VITA_DC_DISCOVER "__list__"
#define PLUS_VITA_DC_LIST_READY "PLUS_VITA_DATACENTER_LIST_READY"
static inline bool plus_vita_cloud_ascii_id(const char *value,size_t max)
{
	if(!value || !*value || strlen(value)>=max) return false;
	for(const unsigned char *p=(const unsigned char *)value;*p;p++)
		if(!((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') || (*p>='0' && *p<='9') || *p=='-' || *p=='_' || *p=='.')) return false;
	return true;
}
static inline bool plus_vita_manual_dc_valid(const char *value)
{
	return plus_vita_cloud_ascii_id(value,128) && strcmp(value,"Auto") && strcmp(value,PLUS_VITA_DC_DISCOVER);
}
static inline bool plus_vita_cloud_identifier_valid(const char *value)
{
	return plus_vita_cloud_ascii_id(value,128) && strlen(value)>=8;
}
static inline bool plus_vita_bitrate_valid(unsigned value)
{
	#if PLUS_VITA_ADAPTIVE_STREAM
	return value>=2500 && value<=15000;
#else
	return value==4000 || value==8000 || value==12000 || value==15000;
#endif
}
static inline unsigned plus_vita_next_bitrate(unsigned value)
{
	#if PLUS_VITA_ADAPTIVE_STREAM
	return pv_profile_next(value);
#else
	switch(value) { case 4000: return 8000; case 8000: return 12000; case 12000: return 15000; default: return 4000; }
#endif
}
#endif
