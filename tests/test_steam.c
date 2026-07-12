#define _POSIX_C_SOURCE 200809L

#include "steam.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    SteamInfo steam;

    assert(steam_collect(NULL, NULL, NULL, 0) == -1);
    assert(setenv("HOME", "tests/fixtures/steam-home", 1) == 0);
    assert(steam_collect(&steam, NULL, NULL, 0) == 0);
    if (steam.controller_detected) assert(steam.controller_name[0] != '\0');
    if (steam.ubuntu_2604) assert(steam.ubuntu);
    assert(steam.library_count == 1U);
    assert(steam.game_count == 1U);
    assert(strcmp(steam.games[0].appid, "4242") == 0);
    assert(steam.games[0].directory_present);
    assert(unsetenv("HOME") == 0);
    return 0;
}
