#ifndef LINUX_DOCTOR_UPDATES_H
#define LINUX_DOCTOR_UPDATES_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#define UPDATES_MAX_PACKAGES 128U
#define UPDATE_NAME_SIZE 256U
#define UPDATE_VERSION_SIZE 256U
#define UPDATE_ARCHITECTURE_SIZE 32U
#define UPDATE_REPOSITORY_SIZE 512U
#define UPDATE_SOURCE_SIZE 256U
#define UPDATE_SECTION_SIZE 64U
#define UPDATE_ORIGIN_SIZE 64U
#define UPDATE_DESCRIPTION_SIZE 768U
#define UPDATE_PURPOSE_SIZE 192U

typedef enum AptUpdateState {
    APT_UPDATE_UNKNOWN = 0,
    APT_UPDATE_READY,
    APT_UPDATE_PHASED,
    APT_UPDATE_DEFERRED
} AptUpdateState;

typedef struct AptUpdatePackage {
    char name[UPDATE_NAME_SIZE];
    char installed_version[UPDATE_VERSION_SIZE];
    char candidate_version[UPDATE_VERSION_SIZE];
    char architecture[UPDATE_ARCHITECTURE_SIZE];
    char repository[UPDATE_REPOSITORY_SIZE];
    char source_package[UPDATE_SOURCE_SIZE];
    char section[UPDATE_SECTION_SIZE];
    char origin[UPDATE_ORIGIN_SIZE];
    char description[UPDATE_DESCRIPTION_SIZE];
    char purpose[UPDATE_PURPOSE_SIZE];
    AptUpdateState state;
    bool security_origin;
    bool held;
    bool metadata_available;
    bool phased_percentage_available;
    unsigned int phased_percentage;
} AptUpdatePackage;

typedef struct UpdatesInfo {
    bool cache_available;
    unsigned int cache_age_days;
    bool inventory_available;
    bool selection_available;
    bool hold_information_available;
    bool metadata_available;
    bool truncated;
    size_t package_count;
    size_t ready_count;
    size_t phased_count;
    size_t deferred_count;
    size_t unknown_count;
    size_t security_count;
    size_t held_count;
    size_t metadata_count;
    AptUpdatePackage packages[UPDATES_MAX_PACKAGES];
} UpdatesInfo;

int updates_collect(UpdatesInfo *updates, char *error, size_t error_size);
int updates_collect_cache_age_from_paths(UpdatesInfo *updates, const char *stamp_path,
    const char *lists_path, time_t now);
int updates_parse_candidates(const char *output, UpdatesInfo *updates);
int updates_mark_selected(const char *output, UpdatesInfo *updates);
int updates_mark_held(const char *output, UpdatesInfo *updates);
int updates_parse_metadata(const char *output, UpdatesInfo *updates);
const char *updates_state_name(AptUpdateState state);

#endif
