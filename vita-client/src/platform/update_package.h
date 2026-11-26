// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_UPDATE_PACKAGE_H
#define PV_UPDATE_PACKAGE_H
#include <stdbool.h>
bool pv_update_unpack(const char *vpk,const char *root);
bool pv_update_identity(const char *vpk);
void pv_update_clean_stage(const char *root);
#endif
