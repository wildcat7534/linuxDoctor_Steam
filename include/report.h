#ifndef LINUX_DOCTOR_REPORT_H
#define LINUX_DOCTOR_REPORT_H

#include <stdio.h>

#include "storage.h"
#include "history.h"
#include "updates.h"
#include "apps.h"
#include "steam.h"
#include "volume.h"
#include "migration.h"
#include "gfn.h"

int report_write(FILE *stream, const StorageInfo *storage, const UpdatesInfo *updates, const AppsInfo *apps,
    const SteamInfo *steam, const VolumeInventory *volumes, const MigrationPlan *migration,
    const GeForceNowInfo *gfn, const HistoryComparison *history);

#endif
