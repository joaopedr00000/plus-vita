// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_UPDATER_VITA_H
#define PV_UPDATER_VITA_H
#include "update_state.h"
typedef struct PvUpdater PvUpdater;
bool pv_update_is_helper(void);
int pv_update_run_helper(void);
PvUpdater *pv_updater_create(void);
void pv_updater_destroy(PvUpdater *u);
bool pv_updater_command(PvUpdater *u,PvUpdateCommand cmd,unsigned language);
bool pv_updater_poll(PvUpdater *u,PvUpdateReport *out);
bool pv_updater_automatic(const PvUpdater *u);
bool pv_updater_busy(const PvUpdater *u);
#endif
