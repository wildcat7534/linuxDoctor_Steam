#ifndef LINUX_DOCTOR_REPORT_H
#define LINUX_DOCTOR_REPORT_H

#include <stdbool.h>
#include <stdio.h>

#include "storage.h"
#include "history.h"
#include "updates.h"
#include "apps.h"
#include "steam.h"
#include "volume.h"
#include "migration.h"
#include "gfn.h"
#include "graphics.h"
#include "knowledge.h"
#include "future_lab.h"

int report_health_score(const StorageInfo *storage, const GraphicsInfo *graphics);
const char *report_health_severity(const StorageInfo *storage, const GraphicsInfo *graphics);
bool report_health_complete(const StorageInfo *storage, const GraphicsInfo *graphics);
int report_write(FILE *stream, const StorageInfo *storage, const UpdatesInfo *updates, const AppsInfo *apps,
    const SteamInfo *steam, const VolumeInventory *volumes, const MigrationPlan *migration,
    const GeForceNowInfo *gfn, const GraphicsInfo *graphics, const GamingKnowledgeBase *knowledge,
    const FutureLabSnapshot *future_lab, const HistoryComparison *history);

#endif
