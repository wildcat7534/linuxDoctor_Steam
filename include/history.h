#ifndef LINUX_DOCTOR_HISTORY_H
#define LINUX_DOCTOR_HISTORY_H

#include <stdbool.h>
#include <stddef.h>

typedef struct HistoryComparison {
    bool enabled;
    bool has_previous;
    int previous_score;
    unsigned int previous_used_percent;
} HistoryComparison;

int history_update(HistoryComparison *comparison, int score,
    unsigned int used_percent, const char *state_directory,
    char *error, size_t error_size);

#endif
