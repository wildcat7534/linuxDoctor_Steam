#ifndef LINUX_DOCTOR_STORAGE_H
#define LINUX_DOCTOR_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STORAGE_MOUNT_LIMIT 12U
#define STORAGE_PATH_CAPACITY 256U

typedef struct StorageMount {
    char path[STORAGE_PATH_CAPACITY];
    uint64_t available_bytes;
    unsigned int used_percent;
} StorageMount;

typedef struct StorageInfo {
    bool available;
    uint64_t total_bytes;
    uint64_t available_bytes;
    unsigned int used_percent;
    StorageMount mounts[STORAGE_MOUNT_LIMIT];
    size_t mount_count;
    bool steamapps_available;
    uint64_t steamapps_bytes;
} StorageInfo;

int storage_collect_root(StorageInfo *storage, char *error, size_t error_size);

#endif
