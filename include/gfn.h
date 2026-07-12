#ifndef LINUX_DOCTOR_GFN_H
#define LINUX_DOCTOR_GFN_H

#include <stdbool.h>

#include "steam.h"

typedef struct GeForceNowInfo {
    bool installed;
    bool official_flatpak;
    bool ubuntu_supported;
    bool wayland_session;
    bool controller_available;
} GeForceNowInfo;

void gfn_collect(GeForceNowInfo *gfn, const SteamInfo *steam);

#endif
