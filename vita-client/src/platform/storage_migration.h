// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_STORAGE_MIGRATION_H
#define PV_STORAGE_MIGRATION_H
#include <stdbool.h>
/* User data on ux0; run before all storage workers. Fail closed on conflicts. */
bool pv_storage_migrate(const char *data);
/* Internal partition: only the two explicitly named vault key files. */
bool pv_key_storage_migrate(const char *data);
#endif
