#include "migration.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    StorageInfo storage = {.available = true, .total_bytes = 1000U, .available_bytes = 50U, .used_percent = 95U};
    VolumeInventory volumes = {.count = 2U, .items = {
        {.path = "/dev/nvme0n1p3", .mountpoint = "/mnt/windows", .mounted = true, .windows_protected = true, .available_bytes = 1900U},
        {.path = "/dev/sdb2", .mountpoint = "/mnt/games", .mounted = true, .available_bytes = 900U}}};
    SteamInfo steam = {.game_count = 3U, .games = {
        {.name = "Runtime", .size_bytes = 200U, .directory_present = true, .is_tool = true},
        {.name = "Large", .size_bytes = 160U, .directory_present = true},
        {.name = "Small", .size_bytes = 40U, .directory_present = true}}};
    MigrationPlan plan;

    migration_plan_build(&plan, &storage, &volumes, &steam);
    assert(plan.available);
    assert(plan.game_count == 1U);
    assert(plan.game_indexes[0] == 1U);
    assert(plan.selected_bytes == 160U);
    assert(plan.target_free_bytes == 150U);
    assert(strcmp(plan.destination_path, "/mnt/games") == 0);
    return 0;
}
