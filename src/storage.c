#include "storage.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) {
        (void)snprintf(error, error_size, "%s", message);
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
    return 0;
}
