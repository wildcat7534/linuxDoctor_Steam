#include "volume.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    VolumeInventory inventory;
    char error[256];
    const char *fixture = "{\"blockdevices\":[{\"path\":\"/dev/sdb\",\"type\":\"disk\",\"size\":\"1000\",\"children\":[{\"path\":\"/dev/sdb1\",\"pkname\":\"sdb\",\"type\":\"part\",\"size\":\"100\",\"fstype\":\"ntfs\",\"parttype\":\"de94bba4\",\"mountpoints\":[null]},{\"path\":\"/dev/sdb2\",\"pkname\":\"sdb\",\"type\":\"part\",\"size\":\"900\",\"fstype\":\"ntfs\",\"uuid\":\"test-uuid\",\"label\":\"Games\",\"parttype\":\"ebd0a0a2\",\"mountpoints\":[null],\"ro\":false,\"rm\":false,\"transport\":\"usb\",\"model\":\"Disk\"}]}]}";

    assert(volume_collect(NULL, error, sizeof(error)) == -1);
    assert(volume_parse_lsblk(NULL, fixture, error, sizeof(error)) == -1);
    assert(volume_parse_lsblk(&inventory, fixture, error, sizeof(error)) == 0);
    assert(inventory.count == 2U);
    assert(strcmp(inventory.items[1].path, "/dev/sdb2") == 0);
    assert(strcmp(inventory.items[1].filesystem, "ntfs") == 0);
    assert(!inventory.items[1].mounted);
    assert(inventory.items[0].windows_protected);
    assert(inventory.items[1].windows_protected);
    assert(volume_collect(&inventory, error, sizeof(error)) == 0);
    assert(inventory.available);
    assert(inventory.count <= VOLUME_LIMIT);
    return 0;
}
