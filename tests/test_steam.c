#define _POSIX_C_SOURCE 200809L

#include "steam.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    SteamInfo steam;

    assert(strcmp(steam_controller_kind("Valve Software Steam Controller Puck Mouse"), "steam") == 0);
    assert(strcmp(steam_controller_kind("Microsoft X-Box One pad"), "xbox") == 0);
    assert(strcmp(steam_controller_kind("Sony DualSense Wireless Controller"), "playstation") == 0);
    assert(strcmp(steam_controller_kind("Nintendo Switch Pro Controller"), "nintendo") == 0);
    assert(strcmp(steam_controller_kind("8BitDo Ultimate Gamepad"), "8bitdo") == 0);
    assert(steam_controller_kind("Headset Consumer Control") == NULL);
    assert(steam_controller_kind("Gaming Mouse") == NULL);
    assert(steam_collect(NULL, NULL, NULL, 0) == -1);
    assert(setenv("HOME", "tests/fixtures/steam-home", 1) == 0);
    assert(steam_collect(&steam, NULL, NULL, 0) == 0);
    if (steam.controller_detected) assert(steam.controller_name[0] != '\0');
    if (steam.controller_detected) assert(steam.controller_count > 0U);
    if (steam.ubuntu_2604) assert(steam.ubuntu);
    assert(steam.library_count == 1U);
    assert(steam.game_count == 1U);
    assert(strcmp(steam.games[0].appid, "4242") == 0);
    assert(steam.games[0].directory_present);
    assert(strstr(steam.games[0].icon_path, "0123456789abcdef0123456789abcdef01234567.jpg") != NULL);
    assert(unsetenv("HOME") == 0);
    return 0;
}
