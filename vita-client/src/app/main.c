// SPDX-License-Identifier: AGPL-3.0-only
#include "ui_vita.h"
#if PLUS_VITA_ADAPTIVE_STREAM
#include "../platform/updater_vita.h"
#endif
#include <psp2/kernel/processmgr.h>

int main(void)
{
	int result;
#if PLUS_VITA_ADAPTIVE_STREAM
	if(pv_update_is_helper())result=pv_update_run_helper();
	else
#endif
	{
#if PLUS_VITA_DIRECT_EXPERIMENT
        if(!pv_direct_recover_startup()) {sceKernelExitProcess(1);return 1;}
#endif
        result=plus_vita_app_run();
#if PLUS_VITA_DIRECT_EXPERIMENT
        if(pv_direct_commit_pending())result=0;
#endif
    }
	sceKernelExitProcess(result);
	return result;
}
