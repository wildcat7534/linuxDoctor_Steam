#define _POSIX_C_SOURCE 200809L

#include "storage.h"

#include <errno.h>
#include <dirent.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) {
        (void)snprintf(error, error_size, "%s", message);
    }
}

static bool is_local_storage_type(const char *type)
{
    static const char *const ignored[] = {
        "autofs", "cgroup", "cgroup2", "devpts", "devtmpfs", "efivarfs", "mqueue", "overlay",
        "proc", "pstore", "securityfs", "squashfs", "sysfs", "tmpfs", "tracefs"
    };
    size_t index;

    for (index = 0; index < sizeof(ignored) / sizeof(ignored[0]); index++) {
        if (strcmp(type, ignored[index]) == 0) return false;
    }
    return true;
}

static void collect_mounts(StorageInfo *storage)
{
    FILE *mounts = setmntent("/proc/self/mounts", "r");
    struct mntent *entry;

    if (mounts == NULL) return;
    while ((entry = getmntent(mounts)) != NULL && storage->mount_count < STORAGE_MOUNT_LIMIT) {
        struct statvfs filesystem;
        uint64_t block_size;
        uint64_t total_blocks;
        uint64_t available_blocks;
        StorageMount *mount;

        if (strcmp(entry->mnt_dir, "/") == 0 || strcmp(entry->mnt_dir, "/boot") == 0 ||
            strcmp(entry->mnt_dir, "/boot/efi") == 0 || hasmntopt(entry, "bind") != NULL ||
            strncmp(entry->mnt_dir, "/snap/", 6) == 0 || strncmp(entry->mnt_dir, "/var/snap/", 10) == 0 ||
            !is_local_storage_type(entry->mnt_type) ||
            statvfs(entry->mnt_dir, &filesystem) != 0) continue;
        block_size = filesystem.f_frsize == 0 ? filesystem.f_bsize : filesystem.f_frsize;
        total_blocks = filesystem.f_blocks;
        available_blocks = filesystem.f_bavail;
        if (block_size == 0 || total_blocks == 0) continue;
        mount = &storage->mounts[storage->mount_count];
        if (snprintf(mount->path, sizeof(mount->path), "%s", entry->mnt_dir) >= (int)sizeof(mount->path)) continue;
        mount->available_bytes = available_blocks * block_size;
        mount->used_percent = (unsigned int)(((total_blocks - available_blocks) * 100U) / total_blocks);
        storage->mount_count++;
    }
    (void)endmntent(mounts);
}

static uint64_t directory_size(const char *path, dev_t device, bool *complete)
{
    DIR *directory = opendir(path);
    struct dirent *entry;
    uint64_t total = 0;

    if (directory == NULL) {
        *complete = false;
        return 0;
    }
    while ((entry = readdir(directory)) != NULL) {
        char child[STORAGE_PATH_CAPACITY * 2U];
        struct stat metadata;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >= (int)sizeof(child)) {
            *complete = false;
            continue;
        }
        if (lstat(child, &metadata) != 0) {
            *complete = false;
            continue;
        }
        if (metadata.st_dev != device || S_ISLNK(metadata.st_mode)) continue;
        if (S_ISDIR(metadata.st_mode)) total += directory_size(child, device, complete);
        else if (S_ISREG(metadata.st_mode) && metadata.st_size > 0) total += (uint64_t)metadata.st_size;
    }
    (void)closedir(directory);
    return total;
}

static void collect_steamapps(StorageInfo *storage)
{
    const char *home = getenv("HOME");
    const char *const suffixes[] = {
        ".local/share/Steam/steamapps", ".steam/steam/steamapps",
        ".var/app/com.valvesoftware.Steam/data/Steam/steamapps"
    };
    size_t index;

    if (home == NULL || home[0] == '\0') return;
    for (index = 0; index < sizeof(suffixes) / sizeof(suffixes[0]); index++) {
        char path[STORAGE_PATH_CAPACITY * 2U];
        struct stat metadata;
        bool complete = true;

        if (snprintf(path, sizeof(path), "%s/%s", home, suffixes[index]) >= (int)sizeof(path) ||
            stat(path, &metadata) != 0 || !S_ISDIR(metadata.st_mode)) continue;
        storage->steamapps_bytes = directory_size(path, metadata.st_dev, &complete);
        storage->steamapps_available = complete;
        return;
    }
}

int storage_collect_root(StorageInfo *storage, char *error, size_t error_size)
{
    struct statvfs filesystem;
    uint64_t block_size;
    uint64_t total_blocks;
    uint64_t available_blocks;

    if (storage == NULL) {
        set_error(error, error_size, "Storage destination is missing.");
        return -1;
    }
    *storage = (StorageInfo){0};
    if (statvfs("/", &filesystem) != 0) {
        set_error(error, error_size, strerror(errno));
        return -1;
    }
    block_size = filesystem.f_frsize == 0 ? filesystem.f_bsize : filesystem.f_frsize;
    total_blocks = filesystem.f_blocks;
    available_blocks = filesystem.f_bavail;
    if (block_size == 0 || total_blocks == 0) {
        set_error(error, error_size, "Root filesystem has no measurable capacity.");
        return -1;
    }
    storage->total_bytes = total_blocks * block_size;
    storage->available_bytes = available_blocks * block_size;
    storage->used_percent = (unsigned int)(((total_blocks - available_blocks) * 100U) / total_blocks);
    storage->available = true;
    collect_mounts(storage);
    collect_steamapps(storage);
    return 0;
}
