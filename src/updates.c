#include "updates.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static const char *const CACHE_PATHS[] = {
    "/var/lib/apt/periodic/update-success-stamp",
    "/var/lib/apt/lists"
};

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) (void)snprintf(error, error_size, "%s", message);
}

int updates_collect(UpdatesInfo *updates, char *error, size_t error_size)
{
    struct stat metadata;
    time_t now;
    size_t index;

    if (updates == NULL) {
        set_error(error, error_size, "Updates destination is missing.");
        return -1;
    }
    *updates = (UpdatesInfo){0};
    now = time(NULL);
    if (now == (time_t)-1) {
        set_error(error, error_size, "System time is unavailable.");
        return -1;
    }
    for (index = 0; index < sizeof(CACHE_PATHS) / sizeof(CACHE_PATHS[0]); index++) {
        if (stat(CACHE_PATHS[index], &metadata) == 0) {
            time_t age = now > metadata.st_mtime ? now - metadata.st_mtime : 0;
            updates->available = true;
            updates->age_days = (unsigned int)(age / (24 * 60 * 60));
            return 0;
        }
    }
    set_error(error, error_size, strerror(errno));
    return -1;
}
