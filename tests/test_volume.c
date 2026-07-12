#include "volume.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    VolumeInventory inventory;
    char error[256];
    const char *fixture = "{\"blockdevices\":[{\"path\":\"/dev/sdb\",\"type\":\"disk\",\"size\":\"1000\",\"children\":[{\"path\":\"/dev/sdb2\",\"type\":\"part\",\"size\":\"900\",\"fstype\":\"ntfs\",\"uuid\":\"test-uuid\",\"label\":\"Games\",\"mountpoints\":[null],\"ro\":false,\"rm\":false,\"transport\":\"usb\",\"model\":\"Disk\"}]}]}";

    assert(volume_collect(NULL, error, sizeof(error)) == -1);
    assert(volume_parse_lsblk(NULL, fixture, error, sizeof(error)) == -1);
    assert(volume_parse_lsblk(&inventory, fixture, error, sizeof(error)) == 0);
    assert(inventory.count == 1U);
    assert(strcmp(inventory.items[0].path, "/dev/sdb2") == 0);
    assert(strcmp(inventory.items[0].filesystem, "ntfs") == 0);
    assert(!inventory.items[0].mounted);
    assert(volume_collect(&inventory, error, sizeof(error)) == 0);
    assert(inventory.available);
    assert(inventory.count <= VOLUME_LIMIT);
    return 0;
}
