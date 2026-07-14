#include "history.h"

#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    char directory[128];
    char path[160];
    char error[256];
    HistoryComparison comparison;

    (void)snprintf(directory, sizeof(directory), "/tmp/linux-doctor-history-%ld", (long)getpid());
    assert(mkdir(directory, 0700) == 0);
    assert(history_update(&comparison, 93, 85, directory, error, sizeof(error)) == 0);
    assert(comparison.enabled && comparison.current_score_complete && !comparison.has_previous);
    assert(history_update(&comparison, 96, 80, directory, error, sizeof(error)) == 0);
    assert(comparison.has_previous);
    assert(comparison.previous_score == 93);
    assert(comparison.previous_used_percent == 85U);
    (void)snprintf(path, sizeof(path), "%s/snapshots-v2.csv", directory);
    assert(unlink(path) == 0);
    assert(rmdir(directory) == 0);
    return 0;
}
