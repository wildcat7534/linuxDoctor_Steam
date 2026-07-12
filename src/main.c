#include "report.h"
#include "storage.h"
#include "history.h"
#include "updates.h"
#include "steam.h"
#include "volume.h"
#include "migration.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void print_usage(const char *program)
{
    (void)fprintf(stderr, "Usage: %s [--history] [--output FILE]\n", program);
}

int main(int argc, char **argv)
{
    const char *output_path = "report.json";
    bool history_enabled = false;
    StorageInfo storage;
    UpdatesInfo updates;
    SteamInfo steam;
    VolumeInventory volumes;
    MigrationPlan migration;
    HistoryComparison history = {0};
    char error[256];
    FILE *output;

    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--history") == 0) {
            history_enabled = true;
        } else if (strcmp(argv[index], "--output") == 0 && index + 1 < argc) {
            output_path = argv[++index];
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }
    if (storage_collect_root(&storage, error, sizeof(error)) != 0) {
        (void)fprintf(stderr, "Linux Doctor: unable to collect storage: %s\n", error);
        return 1;
    }
    if (volume_collect(&volumes, error, sizeof(error)) != 0) {
        (void)fprintf(stderr, "Linux Doctor: unable to collect volumes: %s\n", error);
        return 1;
    }
    if (updates_collect(&updates, error, sizeof(error)) != 0) {
        updates = (UpdatesInfo){0};
    }
    if (steam_collect(&steam, &volumes, error, sizeof(error)) != 0) {
        steam = (SteamInfo){0};
    }
    migration_plan_build(&migration, &storage, &volumes, &steam);
    if (history_enabled && history_update(&history, storage.used_percent >= 95U ? 45 : storage.used_percent >= 85U ? 75 : 96,
        storage.used_percent, NULL, error, sizeof(error)) != 0) {
        (void)fprintf(stderr, "Linux Doctor: unable to update history: %s\n", error);
        return 1;
    }
    output = fopen(output_path, "w");
    if (output == NULL) {
        (void)fprintf(stderr, "Linux Doctor: cannot write %s: %s\n", output_path, strerror(errno));
        return 1;
    }
    if (report_write(output, &storage, &updates, &steam, &volumes, &migration, &history) != 0) {
        (void)fclose(output);
        (void)fprintf(stderr, "Linux Doctor: cannot write report %s\n", output_path);
        return 1;
    }
    if (fclose(output) != 0) {
        (void)fprintf(stderr, "Linux Doctor: cannot write report %s\n", output_path);
        return 1;
    }
    (void)printf("Linux Doctor: report written to %s\n", output_path);
    return 0;
}
