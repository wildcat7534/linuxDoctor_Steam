#ifndef LINUX_DOCTOR_VOLUME_H
#define LINUX_DOCTOR_VOLUME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VOLUME_LIMIT 32U
#define VOLUME_TEXT_CAPACITY 256U

typedef struct Volume {
    char path[VOLUME_TEXT_CAPACITY];
    char parent_path[VOLUME_TEXT_CAPACITY];
    char uuid[VOLUME_TEXT_CAPACITY];
    char label[VOLUME_TEXT_CAPACITY];
    char filesystem[32];
    char partition_label[VOLUME_TEXT_CAPACITY];
    char partition_type[64];
    char mountpoint[VOLUME_TEXT_CAPACITY];
    char transport[32];
    char model[VOLUME_TEXT_CAPACITY];
    uint64_t size_bytes;
    uint64_t available_bytes;
    unsigned int used_percent;
    bool mounted;
    bool read_only;
    bool removable;
    bool windows_system_component;
    bool windows_data_partition;
    bool windows_confirmed;
    bool windows_protected;
} Volume;

typedef struct VolumeInventory {
    Volume items[VOLUME_LIMIT];
    size_t count;
    bool available;
    bool truncated;
} VolumeInventory;

int volume_collect(VolumeInventory *inventory, char *error, size_t error_size);
int volume_parse_lsblk(VolumeInventory *inventory, const char *json, char *error, size_t error_size);

#endif
