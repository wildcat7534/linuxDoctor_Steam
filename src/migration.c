#include "migration.h"

#include <stdio.h>
#include <string.h>

#define GIBIBYTE (1024ULL * 1024ULL * 1024ULL)

void migration_plan_build(MigrationPlan *plan, const StorageInfo *storage,
    const VolumeInventory *volumes, const SteamInfo *steam)
{
    size_t index;
    uint64_t target;

    if (plan == NULL) return;
    *plan = (MigrationPlan){0};
    if (storage == NULL || volumes == NULL || steam == NULL || !storage->available || storage->used_percent < 85U) return;
    target = storage->total_bytes / 5U;
    if (target <= storage->available_bytes) return;
    plan->target_free_bytes = target - storage->available_bytes;
    for (index = 0; index < volumes->count; index++) {
        const Volume *volume = &volumes->items[index];

        if (!volume->mounted || volume->read_only || volume->available_bytes <= plan->destination_available_bytes ||
            volume->mountpoint[0] == '\0' || strcmp(volume->mountpoint, "/") == 0) continue;
        (void)snprintf(plan->destination_path, sizeof(plan->destination_path), "%s", volume->mountpoint);
        (void)snprintf(plan->destination_volume, sizeof(plan->destination_volume), "%s", volume->path);
        plan->destination_available_bytes = volume->available_bytes;
    }
    if (plan->destination_path[0] == '\0') return;
    for (index = 0; index < steam->game_count && plan->game_count < MIGRATION_GAME_LIMIT; index++) {
        const SteamGame *game = &steam->games[index];

        if (!game->directory_present || game->size_bytes == 0 ||
            game->size_bytes > plan->destination_available_bytes - plan->selected_bytes) continue;
        plan->game_indexes[plan->game_count++] = index;
        plan->selected_bytes += game->size_bytes;
        if (plan->selected_bytes >= plan->target_free_bytes) break;
    }
    plan->available = plan->game_count > 0;
}
