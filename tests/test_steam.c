#include "steam.h"

#include <assert.h>

int main(void)
{
    SteamInfo steam;

    assert(steam_collect(NULL, NULL, 0) == -1);
    assert(steam_collect(&steam, NULL, 0) == 0);
    if (steam.controller_detected) assert(steam.controller_name[0] != '\0');
    if (steam.ubuntu_2604) assert(steam.ubuntu);
    return 0;
}
