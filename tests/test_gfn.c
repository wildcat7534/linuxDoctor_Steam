#include "gfn.h"

#include <assert.h>

int main(void)
{
    GeForceNowInfo gfn;
    SteamInfo steam = {.ubuntu = true, .ubuntu_version = "26.04", .controller_detected = true};

    gfn_collect(NULL, &steam);
    gfn_collect(&gfn, &steam);
    assert(gfn.ubuntu_supported);
    assert(gfn.controller_available);
    return 0;
}
