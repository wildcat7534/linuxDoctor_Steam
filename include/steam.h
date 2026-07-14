#ifndef LINUX_DOCTOR_STEAM_H
#define LINUX_DOCTOR_STEAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "volume.h"

#define STEAM_NAME_CAPACITY 128U
#define STEAM_VERSION_CAPACITY 32U
#define STEAM_LIBRARY_LIMIT 16U
#define STEAM_GAME_LIMIT 256U
#define STEAM_CONTROLLER_LIMIT 16U
#define STEAM_ICON_MAX_BYTES 65536U

typedef struct SteamControllerInfo {
    char name[STEAM_NAME_CAPACITY];
    char kind[24];
} SteamControllerInfo;

typedef struct SteamLibrary {
    char path[VOLUME_TEXT_CAPACITY];
    char volume_path[VOLUME_TEXT_CAPACITY];
    char filesystem[32];
    uint64_t available_bytes;
    uint64_t game_bytes;
    size_t game_count;
    bool mounted;
    bool writable;
} SteamLibrary;

typedef struct SteamGame {
    char appid[32];
    char name[VOLUME_TEXT_CAPACITY];
    char icon_path[VOLUME_TEXT_CAPACITY * 2U];
    uint64_t size_bytes;
    size_t library_index;
    bool directory_present;
} SteamGame;

typedef struct SteamInfo {
    bool ubuntu;
    bool ubuntu_2604;
    char ubuntu_version[STEAM_VERSION_CAPACITY];
    bool i386_available;
    bool steam_devices_installed;
    bool controller_detected;
    char controller_name[STEAM_NAME_CAPACITY];
    SteamControllerInfo controllers[STEAM_CONTROLLER_LIMIT];
    size_t controller_count;
    SteamLibrary libraries[STEAM_LIBRARY_LIMIT];
    size_t library_count;
    SteamGame games[STEAM_GAME_LIMIT];
    size_t game_count;
    bool inventory_truncated;
} SteamInfo;

int steam_collect(SteamInfo *steam, const VolumeInventory *volumes, char *error, size_t error_size);
const char *steam_controller_kind(const char *name);

#endif
