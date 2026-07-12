#include "apps.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0U) (void)snprintf(error, error_size, "%s", message);
}

int apps_parse_dpkg_status(FILE *stream, AppsInfo *apps)
{
    char line[512];
    bool gnome_tweaks_package = false;
    bool installed = false;

    if (stream == NULL || apps == NULL) return -1;
    *apps = (AppsInfo){.package_database_available = true};
    while (fgets(line, sizeof(line), stream) != NULL) {
        if (line[0] == '\n') {
            if (gnome_tweaks_package && installed) {
                apps->gnome_tweaks_installed = true;
                return 0;
            }
            gnome_tweaks_package = false;
            installed = false;
            continue;
        }
        if (strncmp(line, "Package: ", 9U) == 0) {
            gnome_tweaks_package = strncmp(line + 9U, "gnome-tweaks", 12U) == 0 &&
                (line[21] == '\n' || line[21] == '\r' || line[21] == '\0');
        } else if (gnome_tweaks_package && strncmp(line, "Status: ", 8U) == 0 &&
            strstr(line + 8U, "install ok installed") != NULL) {
            installed = true;
        }
    }
    if (ferror(stream)) return -1;
    if (gnome_tweaks_package && installed) apps->gnome_tweaks_installed = true;
    return 0;
}

int apps_collect(AppsInfo *apps, char *error, size_t error_size)
{
    FILE *stream;
    int result;

    if (apps == NULL) {
        set_error(error, error_size, "Apps destination is missing.");
        return -1;
    }
    *apps = (AppsInfo){0};
    stream = fopen("/var/lib/dpkg/status", "r");
    if (stream == NULL) {
        set_error(error, error_size, strerror(errno));
        return -1;
    }
    result = apps_parse_dpkg_status(stream, apps);
    if (fclose(stream) != 0 && result == 0) result = -1;
    if (result != 0) set_error(error, error_size, "Unable to read the local DPKG status database.");
    return result;
}
