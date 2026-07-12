#include "report.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    StorageInfo storage = {
        .available = true,
        .total_bytes = 1000U,
        .available_bytes = 500U,
        .used_percent = 50U,
        .mount_count = 1U,
        .steamapps_available = true,
        .steamapps_bytes = 123456789U,
        .mounts = {{.path = "/mnt/games", .available_bytes = 987654321U, .used_percent = 44U}}
    };
    UpdatesInfo updates = {.available = true, .age_days = 2U};
    SteamInfo steam = {.ubuntu = true, .ubuntu_2604 = true, .i386_available = true,
        .steam_devices_installed = true, .controller_detected = true,
        .controller_name = "Steam Controller", .library_count = 1U, .game_count = 1U,
        .libraries = {{.path = "/mnt/games/Steam", .filesystem = "ntfs", .writable = true,
            .game_count = 1U, .game_bytes = 123U}},
        .games = {{.appid = "123", .name = "Test Game", .size_bytes = 123U, .directory_present = true}}};
    VolumeInventory volumes = {.available = true, .count = 1U,
        .items = {{.path = "/dev/sdb2", .uuid = "test-uuid", .filesystem = "ntfs",
            .mountpoint = "/mnt/games", .size_bytes = 1000U, .available_bytes = 500U,
            .used_percent = 50U, .mounted = true}}};
    MigrationPlan migration = {.available = true, .destination_path = "/mnt/games",
        .target_free_bytes = 50U, .selected_bytes = 123U, .game_count = 1U, .game_indexes = {0U}};
    GeForceNowInfo gfn = {.installed = true, .official_flatpak = true, .ubuntu_supported = true,
        .wayland_session = true, .controller_available = true};
    HistoryComparison history = {.enabled = false};
    FILE *stream = tmpfile();
    char buffer[32768];

    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &steam, &volumes, &migration, &gfn, &history) == 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(fread(buffer, 1, sizeof(buffer) - 1, stream) > 0);
    buffer[sizeof(buffer) - 1] = '\0';
    assert(strstr(buffer, "\"storage\"") != NULL);
    assert(strstr(buffer, "\"storage_inventory\"") != NULL);
    assert(strstr(buffer, "test-uuid") != NULL);
    assert(strstr(buffer, "storage.steamapps.size") != NULL);
    assert(strstr(buffer, "storage.other_mounts.free_space") != NULL);
    assert(strstr(buffer, "/mnt/games") != NULL);
    assert(strstr(buffer, "123456789") != NULL);
    assert(strstr(buffer, "\"gaming\"") != NULL);
    assert(strstr(buffer, "steam.controller.rules") != NULL);
    assert(strstr(buffer, "steam.controller.detected") != NULL);
    assert(strstr(buffer, "steam.ubuntu.runtime") != NULL);
    assert(strstr(buffer, "\"steam_inventory\"") != NULL);
    assert(strstr(buffer, "Test Game") != NULL);
    assert(strstr(buffer, "\"steam_migration_plan\"") != NULL);
    assert(strstr(buffer, "gaming.geforce_now.availability") != NULL);
    assert(strstr(buffer, "\"updates\"") != NULL);
    assert(strstr(buffer, "\"severity\":\"ok\"") != NULL);
    assert(strstr(buffer, "steam-devices") != NULL);
    assert(fclose(stream) == 0);

    updates = (UpdatesInfo){.available = true, .age_days = 8U};
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &steam, &volumes, &migration, &gfn, &history) == 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(fread(buffer, 1, sizeof(buffer) - 1, stream) > 0);
    buffer[sizeof(buffer) - 1] = '\0';
    assert(strstr(buffer, "updates.apt.cache_age\",\"severity\":\"warning") != NULL);
    assert(fclose(stream) == 0);

    updates = (UpdatesInfo){.available = false};
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &steam, &volumes, &migration, &gfn, &history) == 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(fread(buffer, 1, sizeof(buffer) - 1, stream) > 0);
    buffer[sizeof(buffer) - 1] = '\0';
    assert(strstr(buffer, "updates.apt.cache_age\",\"severity\":\"unknown") != NULL);
    assert(fclose(stream) == 0);
    return 0;
}
