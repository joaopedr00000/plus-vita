// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PLUS_VITA_DEVICE_AUTH_H
#define PLUS_VITA_DEVICE_AUTH_H
#include <stdbool.h>
#include <stdint.h>

/* Transport-independent RFC 8628 scheduling. No Sony endpoint, credential,
 * token or device identity is embedded here. Only enable a real provider
 * after its device authorization contract and token audience are verified. */
typedef enum {
    PV_DEVICE_IDLE, PV_DEVICE_PENDING, PV_DEVICE_GRANTED,
    PV_DEVICE_DENIED, PV_DEVICE_EXPIRED, PV_DEVICE_CANCELLED, PV_DEVICE_FAILED
} PvDeviceState;
typedef enum {
    PV_DEVICE_REPLY_PENDING, PV_DEVICE_REPLY_SLOW_DOWN,
    PV_DEVICE_REPLY_TIMEOUT, PV_DEVICE_REPLY_GRANTED,
    PV_DEVICE_REPLY_DENIED, PV_DEVICE_REPLY_EXPIRED, PV_DEVICE_REPLY_ERROR
} PvDeviceReply;
typedef struct {
    PvDeviceState state;
    uint64_t deadline_ms, next_poll_ms, interval_ms, last_now_ms;
    bool in_flight;
} PvDeviceAuth;
bool pv_device_begin(PvDeviceAuth *a, uint64_t now_ms, unsigned expires_s, unsigned interval_s);
bool pv_device_poll_due(PvDeviceAuth *a,uint64_t now_ms);
bool pv_device_reply(PvDeviceAuth *a,uint64_t now_ms,PvDeviceReply reply);
void pv_device_cancel(PvDeviceAuth *a);
#endif
