// SPDX-License-Identifier: AGPL-3.0-only
#include "device_auth.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    PvDeviceAuth a={0};
    assert(!pv_device_begin(&a,0,0,5));
    assert(!pv_device_begin(&a,0,1801,5));
    assert(!pv_device_begin(&a,0,180,301));
    assert(!pv_device_begin(&a,UINT64_MAX-1000,180,5));
    assert(pv_device_begin(&a,100,180,0));
    assert(!pv_device_poll_due(&a,5099));
    assert(pv_device_poll_due(&a,5100));
    assert(!pv_device_poll_due(&a,5101));
    assert(pv_device_reply(&a,5102,PV_DEVICE_REPLY_PENDING));
    assert(!pv_device_poll_due(&a,10101));
    assert(pv_device_poll_due(&a,10102));
    assert(pv_device_reply(&a,10103,PV_DEVICE_REPLY_SLOW_DOWN));
    assert(a.interval_ms==10000);
    assert(!pv_device_poll_due(&a,20102));
    assert(pv_device_poll_due(&a,20103));
    assert(pv_device_reply(&a,20104,PV_DEVICE_REPLY_TIMEOUT));
    assert(a.interval_ms==20000);
    assert(pv_device_poll_due(&a,40104));
    assert(pv_device_reply(&a,40105,PV_DEVICE_REPLY_GRANTED));
    assert(a.state==PV_DEVICE_GRANTED);
    assert(!pv_device_poll_due(&a,40106));
    assert(pv_device_begin(&a,0,180,5));
    assert(pv_device_poll_due(&a,5000));
    pv_device_cancel(&a);
    assert(!pv_device_reply(&a,5001,PV_DEVICE_REPLY_GRANTED));
    assert(a.state==PV_DEVICE_CANCELLED);
    assert(pv_device_begin(&a,0,10,5));
    assert(pv_device_poll_due(&a,5000));
    assert(!pv_device_reply(&a,10000,PV_DEVICE_REPLY_GRANTED));
    assert(a.state==PV_DEVICE_EXPIRED);
    assert(pv_device_begin(&a,100,180,5));
    assert(!pv_device_poll_due(&a,99));
    assert(a.state==PV_DEVICE_FAILED);
    assert(pv_device_begin(&a,0,180,5));
    assert(!pv_device_reply(&a,1,PV_DEVICE_REPLY_GRANTED));
    assert(pv_device_poll_due(&a,5000));
    assert(pv_device_reply(&a,5001,PV_DEVICE_REPLY_DENIED));
    assert(a.state==PV_DEVICE_DENIED);
    assert(!pv_device_poll_due(&a,6000));
    puts("device authorization timing and terminal states passed");
    return 0;
}
