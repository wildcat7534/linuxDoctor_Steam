#ifndef LINUX_DOCTOR_STORAGE_H
#define LINUX_DOCTOR_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct StorageInfo {
    bool available;
    uint64_t total_bytes;
    uint64_t available_bytes;
    unsigned int used_percent;
} StorageInfo;

int storage_collect_root(StorageInfo *storage, char *error, size_t error_size);

#endif
