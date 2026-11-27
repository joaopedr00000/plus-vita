// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_UPDATE_STATE_H
#define PV_UPDATE_STATE_H
#include <stdbool.h>
typedef enum {PV_UP_IDLE,PV_UP_CHECKING,PV_UP_AVAILABLE,PV_UP_CURRENT,PV_UP_DOWNLOADING,PV_UP_READY,PV_UP_INSTALLING,PV_UP_INSTALLED,PV_UP_ERROR,PV_UP_CANCELLED} PvUpdateState;
typedef enum {PV_UP_CMD_CHECK,PV_UP_CMD_DOWNLOAD,PV_UP_CMD_INSTALL,PV_UP_CMD_CANCEL,PV_UP_CMD_CHANNEL,PV_UP_CMD_AUTO} PvUpdateCommand;
typedef struct {
    PvUpdateState state;
    bool beta,automatic;
    unsigned progress;
    unsigned install_stage;
    int install_error;
    char version[40],notes[4096],file[200];
} PvUpdateReport;
#endif
