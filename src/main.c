#include "report.h"
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

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void print_usage(const char *program)
{
    (void)fprintf(stderr, "Usage: %s [--history] [--output FILE] [--version]\n"
        "       %s --check-knowledge FILE\n"
        "       %s --install-knowledge FILE\n", program, program, program);
}

int main(int argc, char **argv)
{
    const char *output_path = "report.json";
    const char *check_knowledge_path = NULL;
    const char *install_knowledge_path = NULL;
    bool history_enabled = false;
    StorageInfo storage;
    UpdatesInfo updates;
    AppsInfo apps;
    SteamInfo steam;
    VolumeInventory volumes;
    MigrationPlan migration;
    GeForceNowInfo gfn;
    GraphicsInfo graphics;
    GamingKnowledgeBase knowledge;
    FutureLabSnapshot future_lab;
    HistoryComparison history = {0};
    char error[256];
    FILE *output;

    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--history") == 0) {
            history_enabled = true;
        } else if (strcmp(argv[index], "--output") == 0 && index + 1 < argc) {
            output_path = argv[++index];
        } else if (strcmp(argv[index], "--version") == 0) {
            (void)printf("Linux Doctor Gamer Edition %s\n", LINUX_DOCTOR_VERSION);
            return 0;
        } else if (strcmp(argv[index], "--check-knowledge") == 0 && index + 1 < argc) {
            check_knowledge_path = argv[++index];
        } else if (strcmp(argv[index], "--install-knowledge") == 0 && index + 1 < argc) {
            install_knowledge_path = argv[++index];
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }
    if (check_knowledge_path != NULL && install_knowledge_path != NULL) {
        print_usage(argv[0]);
        return 2;
    }
    if (check_knowledge_path != NULL) {
        if (gaming_knowledge_validate_file(check_knowledge_path, &knowledge, error, sizeof(error)) != 0) {
            (void)fprintf(stderr, "Linux Doctor: invalid knowledge database: %s\n", error);
            return 1;
        }
        (void)printf("Linux Doctor: knowledge database %s reviewed %s (%zu entries) is valid.\n",
            knowledge.version, knowledge.reviewed_on, knowledge.entry_count);
        return 0;
    }
    if (install_knowledge_path != NULL) {
        char installed_path[GAMING_KNOWLEDGE_PATH_CAPACITY];

        if (gaming_knowledge_install(install_knowledge_path, installed_path,
            sizeof(installed_path), error, sizeof(error)) != 0) {
            (void)fprintf(stderr, "Linux Doctor: cannot install knowledge database: %s\n", error);
            return 1;
        }
        (void)printf("Linux Doctor: knowledge database installed in %s\n", installed_path);
        return 0;
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
    if (apps_collect(&apps, error, sizeof(error)) != 0) {
        apps = (AppsInfo){0};
    }
    if (steam_collect(&steam, &volumes, error, sizeof(error)) != 0) {
        steam = (SteamInfo){0};
    }
    migration_plan_build(&migration, &storage, &volumes, &steam);
    gfn_collect(&gfn, &steam);
    {
        char knowledge_path[GAMING_KNOWLEDGE_PATH_CAPACITY];
        bool user_database = false;
        bool knowledge_loaded = false;

        if (gaming_knowledge_resolve_path(knowledge_path, sizeof(knowledge_path),
            &user_database, error, sizeof(error)) == 0 &&
            gaming_knowledge_load_validated(&knowledge, knowledge_path, &steam, &gfn,
                error, sizeof(error)) == 0) {
            knowledge.user_database = user_database;
            knowledge_loaded = true;
        }
        if (!knowledge_loaded && gaming_knowledge_load_validated(&knowledge, "data/gaming-knowledge.tsv",
            &steam, &gfn, error, sizeof(error)) == 0) knowledge_loaded = true;
        if (!knowledge_loaded) knowledge = (GamingKnowledgeBase){0};
    }
    if (graphics_collect(&graphics, error, sizeof(error)) != 0) {
        graphics = (GraphicsInfo){0};
    }
    if (future_lab_collect(&future_lab, error, sizeof(error)) != 0) {
        future_lab = (FutureLabSnapshot){0};
    }
    if (history_enabled) {
        if (!report_health_complete(&storage, &graphics)) {
            history = (HistoryComparison){.enabled = true, .current_score_complete = false};
        } else if (history_update(&history, report_health_score(&storage, &graphics),
            storage.used_percent, NULL, error, sizeof(error)) != 0) {
            (void)fprintf(stderr, "Linux Doctor: unable to update history: %s\n", error);
            return 1;
        }
    }
    output = fopen(output_path, "w");
    if (output == NULL) {
        (void)fprintf(stderr, "Linux Doctor: cannot write %s: %s\n", output_path, strerror(errno));
        return 1;
    }
    if (report_write(output, &storage, &updates, &apps, &steam, &volumes, &migration,
        &gfn, &graphics, &knowledge, &future_lab, &history) != 0) {
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
