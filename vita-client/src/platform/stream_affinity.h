// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_STREAM_AFFINITY_H
#define PV_STREAM_AFFINITY_H
#include "adaptive_stream.h"
#if defined(__vita__) && PLUS_VITA_ADAPTIVE_STREAM
#include <psp2/kernel/threadmgr.h>
static inline int pv_stream_worker_affinity(bool audio){
    /* Two allowed user cores rather than hard-pinning, so kernel scheduling
     * retains headroom under Wi-Fi/crypto contention. Best effort only. */
    int mask=audio?(SCE_KERNEL_CPU_MASK_USER_1|SCE_KERNEL_CPU_MASK_USER_2):
        (SCE_KERNEL_CPU_MASK_USER_0|SCE_KERNEL_CPU_MASK_USER_2);
    return sceKernelChangeThreadCpuAffinityMask(sceKernelGetThreadId(),mask);
}
#else
static inline int pv_stream_worker_affinity(bool audio){(void)audio;return 0;}
#endif
#endif
