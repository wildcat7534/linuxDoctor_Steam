#ifndef LINUX_DOCTOR_STEAM_H
#define LINUX_DOCTOR_STEAM_H

#include <stdbool.h>
#include <stddef.h>

#define STEAM_NAME_CAPACITY 128U
#define STEAM_VERSION_CAPACITY 32U

typedef struct SteamInfo {
    bool ubuntu;
    bool ubuntu_2604;
    char ubuntu_version[STEAM_VERSION_CAPACITY];
    bool i386_available;
    bool steam_devices_installed;
    bool controller_detected;
    char controller_name[STEAM_NAME_CAPACITY];
} SteamInfo;

int steam_collect(SteamInfo *steam, char *error, size_t error_size);

#endif
