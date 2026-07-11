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
    HistoryComparison history = {.enabled = false};
    FILE *stream = tmpfile();
    char buffer[32768];

    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &history) == 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(fread(buffer, 1, sizeof(buffer) - 1, stream) > 0);
    buffer[sizeof(buffer) - 1] = '\0';
    assert(strstr(buffer, "\"storage\"") != NULL);
    assert(strstr(buffer, "storage.steamapps.size") != NULL);
    assert(strstr(buffer, "storage.other_mounts.free_space") != NULL);
    assert(strstr(buffer, "/mnt/games") != NULL);
    assert(strstr(buffer, "123456789") != NULL);
    assert(strstr(buffer, "\"steam\"") != NULL);
    assert(strstr(buffer, "steam.controller.rules") != NULL);
    assert(strstr(buffer, "\"updates\"") != NULL);
    assert(strstr(buffer, "\"severity\":\"ok\"") != NULL);
    assert(strstr(buffer, "steam-devices") != NULL);
    assert(fclose(stream) == 0);

    updates = (UpdatesInfo){.available = true, .age_days = 8U};
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &history) == 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(fread(buffer, 1, sizeof(buffer) - 1, stream) > 0);
    buffer[sizeof(buffer) - 1] = '\0';
    assert(strstr(buffer, "updates.apt.cache_age\",\"severity\":\"warning") != NULL);
    assert(fclose(stream) == 0);

    updates = (UpdatesInfo){.available = false};
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &history) == 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(fread(buffer, 1, sizeof(buffer) - 1, stream) > 0);
    buffer[sizeof(buffer) - 1] = '\0';
    assert(strstr(buffer, "updates.apt.cache_age\",\"severity\":\"unknown") != NULL);
    assert(fclose(stream) == 0);
    return 0;
}
