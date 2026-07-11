#include "steam.h"

#include <stdio.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) (void)snprintf(error, error_size, "%s", message);
}

static void copy_value(char *destination, size_t destination_size, const char *value)
{
    size_t length = strcspn(value, "\n");

    if (length >= 2U && value[0] == '"' && value[length - 1U] == '"') {
        value++;
        length -= 2U;
    }
    if (length >= destination_size) length = destination_size - 1U;
    (void)memcpy(destination, value, length);
    destination[length] = '\0';
}

static void collect_os_release(SteamInfo *steam)
{
    FILE *stream = fopen("/etc/os-release", "r");
    char line[256];

    if (stream == NULL) return;
    while (fgets(line, sizeof(line), stream) != NULL) {
        if (strncmp(line, "ID=", 3) == 0) steam->ubuntu = strncmp(line + 3, "ubuntu", 6) == 0 ||
            strncmp(line + 3, "\"ubuntu\"", 8) == 0;
        else if (strncmp(line, "VERSION_ID=", 11) == 0) copy_value(steam->ubuntu_version,
            sizeof(steam->ubuntu_version), line + 11);
    }
    steam->ubuntu_2604 = steam->ubuntu && strcmp(steam->ubuntu_version, "26.04") == 0;
    (void)fclose(stream);
}

static void collect_dpkg(SteamInfo *steam)
{
    FILE *stream = fopen("/var/lib/dpkg/status", "r");
    char line[512];
    bool in_package = false;
    FILE *architectures;

    if (stream != NULL) {
        while (fgets(line, sizeof(line), stream) != NULL) {
            if (strncmp(line, "Package: ", 9) == 0) in_package = strcmp(line + 9, "steam-devices\n") == 0;
            else if (in_package && strncmp(line, "Status: ", 8) == 0 && strstr(line, "install ok installed") != NULL) {
                steam->steam_devices_installed = true;
                break;
            }
        }
        (void)fclose(stream);
    }
    architectures = fopen("/var/lib/dpkg/arch", "r");
    if (architectures == NULL) return;
    while (fgets(line, sizeof(line), architectures) != NULL) {
        if (strcmp(line, "i386\n") == 0) {
            steam->i386_available = true;
            break;
        }
    }
    (void)fclose(architectures);
}

static void collect_controller(SteamInfo *steam)
{
    FILE *stream = fopen("/proc/bus/input/devices", "r");
    char line[512];
    const char *name;
    const char *end;

    if (stream == NULL) return;
    while (fgets(line, sizeof(line), stream) != NULL) {
        if (strncmp(line, "N: Name=\"", 9) != 0 || strstr(line, "Steam Controller") == NULL) continue;
        name = line + 9;
        end = strchr(name, '"');
        if (end == NULL) continue;
        steam->controller_detected = true;
        if ((size_t)(end - name) >= sizeof(steam->controller_name)) end = name + sizeof(steam->controller_name) - 1U;
        (void)memcpy(steam->controller_name, name, (size_t)(end - name));
        steam->controller_name[end - name] = '\0';
        break;
    }
    (void)fclose(stream);
}

int steam_collect(SteamInfo *steam, char *error, size_t error_size)
{
    if (steam == NULL) {
        set_error(error, error_size, "Steam destination is missing.");
        return -1;
    }
    *steam = (SteamInfo){0};
    collect_os_release(steam);
    collect_dpkg(steam);
    collect_controller(steam);
    return 0;
}
