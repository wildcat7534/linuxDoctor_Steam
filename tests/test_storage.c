#include "storage.h"

#include <assert.h>

int main(void)
{
    StorageInfo storage;
    char error[256];

    assert(storage_collect_root(&storage, error, sizeof(error)) == 0);
    assert(storage.available);
    assert(storage.total_bytes > 0);
    assert(storage.available_bytes <= storage.total_bytes);
    assert(storage.used_percent <= 100U);
    assert(storage.mount_count <= STORAGE_MOUNT_LIMIT);
    return 0;
}
