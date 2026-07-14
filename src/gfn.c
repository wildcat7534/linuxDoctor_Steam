#include "gfn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void gfn_collect(GeForceNowInfo *gfn, const SteamInfo *steam)
{
    const char *home;
    char user_path[4096];
    const char *session;

    if (gfn == NULL) return;
    *gfn = (GeForceNowInfo){0};
    home = getenv("HOME");
    gfn->official_flatpak = access("/var/lib/flatpak/app/com.nvidia.geforcenow", F_OK) == 0;
    if (!gfn->official_flatpak && home != NULL &&
        snprintf(user_path, sizeof(user_path), "%s/.local/share/flatpak/app/com.nvidia.geforcenow", home) < (int)sizeof(user_path))
        gfn->official_flatpak = access(user_path, F_OK) == 0;
    gfn->installed = gfn->official_flatpak;
    gfn->ubuntu_supported = steam != NULL && steam->ubuntu && strcmp(steam->ubuntu_version, "24.04") >= 0;
    gfn->controller_available = steam != NULL && steam->controller_count > 0U;
    session = getenv("XDG_SESSION_TYPE");
    gfn->wayland_session = session != NULL && strcmp(session, "wayland") == 0;
}
