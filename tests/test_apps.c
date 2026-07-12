#include "apps.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    AppsInfo apps;
    FILE *stream = tmpfile();

    assert(stream != NULL);
    assert(fputs("Package: gnome-tweaks\nStatus: install ok installed\n\n", stream) >= 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(apps_parse_dpkg_status(stream, &apps) == 0);
    assert(apps.package_database_available);
    assert(apps.gnome_tweaks_installed);
    assert(fclose(stream) == 0);

    stream = tmpfile();
    assert(stream != NULL);
    assert(fputs("Package: gnome-tweaks\nStatus: deinstall ok config-files\n\n", stream) >= 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(apps_parse_dpkg_status(stream, &apps) == 0);
    assert(apps.package_database_available);
    assert(!apps.gnome_tweaks_installed);
    assert(fclose(stream) == 0);
    assert(apps_parse_dpkg_status(NULL, &apps) != 0);
    return 0;
}
