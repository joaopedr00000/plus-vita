// SPDX-License-Identifier: AGPL-3.0-only
#include "device_auth.h"
#include <stddef.h>

#define PV_MAX_LIFETIME_S 1800u
#define PV_MAX_INTERVAL_S 300u

static uint64_t add_ms(uint64_t a,uint64_t b)
{
    return UINT64_MAX-a<b?UINT64_MAX:a+b;
}
static bool pending(PvDeviceAuth *a,uint64_t now)
{
    if(!a||a->state!=PV_DEVICE_PENDING)return false;
    if(now<a->last_now_ms){a->state=PV_DEVICE_FAILED;a->in_flight=false;return false;}
    a->last_now_ms=now;
    if(now>=a->deadline_ms){a->state=PV_DEVICE_EXPIRED;a->in_flight=false;return false;}
    return true;
}
bool pv_device_begin(PvDeviceAuth *a,uint64_t now,unsigned expires_s,unsigned interval_s)
{
    if(!a)return false;
    *a=(PvDeviceAuth){.state=PV_DEVICE_FAILED};
    if(!expires_s||expires_s>PV_MAX_LIFETIME_S||interval_s>PV_MAX_INTERVAL_S)return false;
    uint64_t interval=(uint64_t)(interval_s?interval_s:5u)*1000u;
    uint64_t expiry=(uint64_t)expires_s*1000u;
    if(UINT64_MAX-now<expiry||interval>=expiry)return false;
    *a=(PvDeviceAuth){.state=PV_DEVICE_PENDING,.deadline_ms=now+expiry,
        .next_poll_ms=now+interval,.interval_ms=interval,.last_now_ms=now};
    return true;
}
bool pv_device_poll_due(PvDeviceAuth *a,uint64_t now)
{
    if(!pending(a,now)||a->in_flight||now<a->next_poll_ms)return false;
    a->in_flight=true;
    return true;
}
bool pv_device_reply(PvDeviceAuth *a,uint64_t now,PvDeviceReply reply)
{
    if(!pending(a,now)||!a->in_flight)return false;
    a->in_flight=false;
    switch(reply){
    case PV_DEVICE_REPLY_PENDING:break;
    case PV_DEVICE_REPLY_SLOW_DOWN:
        a->interval_ms=add_ms(a->interval_ms,5000u);
        break;
    case PV_DEVICE_REPLY_TIMEOUT:
        a->interval_ms=add_ms(a->interval_ms,a->interval_ms);
        break;
    case PV_DEVICE_REPLY_GRANTED:a->state=PV_DEVICE_GRANTED;return true;
    case PV_DEVICE_REPLY_DENIED:a->state=PV_DEVICE_DENIED;return true;
    case PV_DEVICE_REPLY_EXPIRED:a->state=PV_DEVICE_EXPIRED;return true;
    default:a->state=PV_DEVICE_FAILED;return true;
    }
    a->next_poll_ms=add_ms(now,a->interval_ms);
    return true;
}
void pv_device_cancel(PvDeviceAuth *a)
{
    if(a&&a->state==PV_DEVICE_PENDING){a->state=PV_DEVICE_CANCELLED;a->in_flight=false;}
}
