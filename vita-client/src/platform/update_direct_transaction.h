// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_UPDATE_DIRECT_TRANSACTION_H
#define PV_UPDATE_DIRECT_TRANSACTION_H
#include <stdbool.h>
typedef enum {PV_DIRECT_BOOT_OLD,PV_DIRECT_BOOT_NEW,PV_DIRECT_BOOT_MIXED,PV_DIRECT_BOOT_UNKNOWN} PvDirectBoot;
static inline PvDirectBoot pv_direct_boot_state(bool old_sfo,bool old_exe,bool new_sfo,bool new_exe){
    if(new_sfo&&new_exe)return PV_DIRECT_BOOT_NEW;
    if(old_sfo&&old_exe)return PV_DIRECT_BOOT_OLD;
    if((old_sfo&&new_exe)||(new_sfo&&old_exe))return PV_DIRECT_BOOT_MIXED;
    return PV_DIRECT_BOOT_UNKNOWN;
}
typedef enum {PV_DIRECT_UNMOUNT_FAILED=1,PV_DIRECT_SFO_FAILED,PV_DIRECT_EBOOT_FAILED,PV_DIRECT_SYNC_FAILED,PV_DIRECT_RESTART_RETURNED,PV_DIRECT_ROLLBACK_FAILED} PvDirectResult;
typedef struct {
    void *context;
    bool (*unmount)(void *);
    bool (*replace)(void *,unsigned);
    bool (*sync)(void *);
    void (*restart)(void *);
    bool (*restore)(void *);
} PvDirectOps;
/* Success never returns: the restart replaces the process. Every failure
 * after unmount invokes recovery, including a returned restart. */
static inline PvDirectResult pv_direct_execute(PvDirectOps o){
    if(!o.unmount(o.context))return PV_DIRECT_UNMOUNT_FAILED;
    PvDirectResult failure=PV_DIRECT_SFO_FAILED;
    if(o.replace(o.context,0)){
        failure=PV_DIRECT_EBOOT_FAILED;
        if(o.replace(o.context,1)){
            failure=PV_DIRECT_SYNC_FAILED;
            if(o.sync(o.context)){
                failure=PV_DIRECT_RESTART_RETURNED;
                o.restart(o.context);
            }
        }
    }
    return o.restore(o.context)?failure:PV_DIRECT_ROLLBACK_FAILED;
}
#endif
