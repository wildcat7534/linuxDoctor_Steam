#include "history.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define HISTORY_LIMIT 30U
#define HISTORY_PATH_CAPACITY 4096U
#define HISTORY_FILENAME "snapshots-v2.csv"

typedef struct HistoryRecord {
    long long timestamp;
    int score;
    unsigned int used_percent;
} HistoryRecord;

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) (void)snprintf(error, error_size, "%s", message);
}

static int create_directories(const char *directory)
{
    char path[HISTORY_PATH_CAPACITY];
    char *cursor;

    if (snprintf(path, sizeof(path), "%s", directory) >= (int)sizeof(path)) return -1;
    for (cursor = path + 1; *cursor != '\0'; cursor++) {
        if (*cursor == '/') {
            *cursor = '\0';
            if (mkdir(path, 0700) != 0 && errno != EEXIST) return -1;
            *cursor = '/';
        }
    }
    return mkdir(path, 0700) == 0 || errno == EEXIST ? 0 : -1;
}

static int resolve_directory(char *directory, size_t directory_size, const char *requested)
{
    const char *base;

    if (requested != NULL) return snprintf(directory, directory_size, "%s", requested) < (int)directory_size ? 0 : -1;
    base = getenv("XDG_STATE_HOME");
    if (base != NULL && base[0] != '\0') return snprintf(directory, directory_size, "%s/linux-doctor", base) < (int)directory_size ? 0 : -1;
    base = getenv("HOME");
    if (base == NULL || base[0] == '\0') return -1;
    return snprintf(directory, directory_size, "%s/.local/state/linux-doctor", base) < (int)directory_size ? 0 : -1;
}

static size_t read_records(const char *path, HistoryRecord records[HISTORY_LIMIT])
{
    FILE *stream = fopen(path, "r");
    HistoryRecord record;
    size_t count = 0;

    if (stream == NULL) return 0;
    while (fscanf(stream, "%lld,%d,%u\n", &record.timestamp, &record.score, &record.used_percent) == 3) {
        if (count == HISTORY_LIMIT) {
            (void)memmove(records, records + 1, (HISTORY_LIMIT - 1U) * sizeof(*records));
            count--;
        }
        records[count++] = record;
    }
    (void)fclose(stream);
    return count;
}

static int write_records(const char *path, const HistoryRecord *records, size_t count)
{
    char temporary_path[HISTORY_PATH_CAPACITY];
    FILE *stream;
    size_t index;

    if (snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", path) >= (int)sizeof(temporary_path)) return -1;
    stream = fopen(temporary_path, "w");
    if (stream == NULL) return -1;
    for (index = 0; index < count; index++) {
        if (fprintf(stream, "%lld,%d,%u\n", records[index].timestamp, records[index].score, records[index].used_percent) < 0) {
            (void)fclose(stream);
            (void)unlink(temporary_path);
            return -1;
        }
    }
    if (fclose(stream) != 0 || rename(temporary_path, path) != 0) {
        (void)unlink(temporary_path);
        return -1;
    }
    return chmod(path, 0600);
}

int history_update(HistoryComparison *comparison, int score,
    unsigned int used_percent, const char *state_directory,
    char *error, size_t error_size)
{
    char directory[HISTORY_PATH_CAPACITY];
    char path[HISTORY_PATH_CAPACITY];
    HistoryRecord records[HISTORY_LIMIT];
    size_t count;

    if (comparison == NULL || resolve_directory(directory, sizeof(directory), state_directory) != 0 || create_directories(directory) != 0 ||
        snprintf(path, sizeof(path), "%s/%s", directory, HISTORY_FILENAME) >= (int)sizeof(path)) {
        set_error(error, error_size, "Unable to prepare the local history directory.");
        return -1;
    }
    *comparison = (HistoryComparison){.enabled = true, .current_score_complete = true};
    count = read_records(path, records);
    if (count > 0) {
        comparison->has_previous = true;
        comparison->previous_score = records[count - 1U].score;
        comparison->previous_used_percent = records[count - 1U].used_percent;
    }
    if (count == HISTORY_LIMIT) {
        (void)memmove(records, records + 1, (HISTORY_LIMIT - 1U) * sizeof(*records));
        count--;
    }
    records[count++] = (HistoryRecord){.timestamp = (long long)time(NULL), .score = score, .used_percent = used_percent};
    if (write_records(path, records, count) != 0) {
        set_error(error, error_size, "Unable to write local history.");
        return -1;
    }
    return 0;
}
