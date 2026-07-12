#ifndef LINUX_DOCTOR_REPORT_H
#define LINUX_DOCTOR_REPORT_H

#include <stdio.h>

#include "storage.h"
#include "history.h"
#include "updates.h"
#include "steam.h"
#include "volume.h"

int report_write(FILE *stream, const StorageInfo *storage, const UpdatesInfo *updates,
    const SteamInfo *steam, const VolumeInventory *volumes, const HistoryComparison *history);

#endif
