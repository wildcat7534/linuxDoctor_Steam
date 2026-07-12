#ifndef LINUX_DOCTOR_MIGRATION_H
#define LINUX_DOCTOR_MIGRATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "steam.h"
#include "storage.h"
#include "volume.h"

#define MIGRATION_GAME_LIMIT 32U

typedef struct MigrationPlan {
    bool available;
    char destination_path[VOLUME_TEXT_CAPACITY];
    char destination_volume[VOLUME_TEXT_CAPACITY];
    uint64_t destination_available_bytes;
    uint64_t target_free_bytes;
    uint64_t selected_bytes;
    size_t game_indexes[MIGRATION_GAME_LIMIT];
    size_t game_count;
} MigrationPlan;

void migration_plan_build(MigrationPlan *plan, const StorageInfo *storage,
    const VolumeInventory *volumes, const SteamInfo *steam);

#endif
