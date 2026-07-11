#ifndef LINUX_DOCTOR_UPDATES_H
#define LINUX_DOCTOR_UPDATES_H

#include <stdbool.h>
#include <stddef.h>

typedef struct UpdatesInfo {
    bool available;
    unsigned int age_days;
} UpdatesInfo;

int updates_collect(UpdatesInfo *updates, char *error, size_t error_size);

#endif
