#include "steam.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

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

const char *steam_controller_kind(const char *name)
{
    if (name == NULL || strstr(name, "Consumer Control") != NULL) return NULL;
    if (strstr(name, "Steam Controller") != NULL || strstr(name, "Steam Deck") != NULL ||
        strstr(name, "Valve Software") != NULL) return "steam";
    if (strstr(name, "Xbox") != NULL || strstr(name, "X-Box") != NULL ||
        strstr(name, "Microsoft X-Box") != NULL) return "xbox";
    if (strstr(name, "DualSense") != NULL || strstr(name, "DualShock") != NULL ||
        strstr(name, "PLAYSTATION") != NULL || strstr(name, "PlayStation") != NULL ||
        strstr(name, "Sony Interactive") != NULL) return "playstation";
    if (strstr(name, "Nintendo") != NULL || strstr(name, "Joy-Con") != NULL ||
        strstr(name, "Switch Pro") != NULL) return "nintendo";
    if (strstr(name, "8BitDo") != NULL || strstr(name, "8BITDO") != NULL) return "8bitdo";
    if (strstr(name, "Gamepad") != NULL || strstr(name, "Joystick") != NULL ||
        strstr(name, "Game Controller") != NULL) return "generic";
    return NULL;
}

static const char *controller_display_name(const char *name, const char *kind)
{
    if (strcmp(kind, "steam") != 0) return name;
    return strstr(name, "Steam Deck") != NULL ? "Steam Deck" : "Steam Controller";
}

static void add_controller(SteamInfo *steam, const char *name, const char *kind)
{
    const char *display_name = controller_display_name(name, kind);
    size_t index;

    for (index = 0U; index < steam->controller_count; index++) {
        if (strcmp(steam->controllers[index].name, display_name) == 0 &&
            strcmp(steam->controllers[index].kind, kind) == 0) return;
    }
    if (steam->controller_count == STEAM_CONTROLLER_LIMIT) {
        steam->inventory_truncated = true;
        return;
    }
    (void)snprintf(steam->controllers[steam->controller_count].name,
        sizeof(steam->controllers[steam->controller_count].name), "%s", display_name);
    (void)snprintf(steam->controllers[steam->controller_count].kind,
        sizeof(steam->controllers[steam->controller_count].kind), "%s", kind);
    steam->controller_count++;
    if (strcmp(kind, "steam") == 0 && !steam->controller_detected) {
        steam->controller_detected = true;
        (void)snprintf(steam->controller_name, sizeof(steam->controller_name), "%s", display_name);
    }
}

static void collect_controllers(SteamInfo *steam)
{
    FILE *stream = fopen("/proc/bus/input/devices", "r");
    char line[512];
    char name[STEAM_NAME_CAPACITY];
    const char *start;
    const char *end;

    if (stream == NULL) return;
    while (fgets(line, sizeof(line), stream) != NULL) {
        const char *kind;
        size_t length;

        if (strncmp(line, "N: Name=\"", 9) != 0) continue;
        start = line + 9;
        end = strchr(start, '"');
        if (end == NULL) continue;
        length = (size_t)(end - start);
        if (length == 0U || length >= sizeof(name)) continue;
        (void)memcpy(name, start, length);
        name[length] = '\0';
        kind = steam_controller_kind(name);
        if (kind != NULL) add_controller(steam, name, kind);
    }
    (void)fclose(stream);
}

static bool path_prefix(const char *path, const char *prefix)
{
    size_t length = strlen(prefix);
    return strncmp(path, prefix, length) == 0 && (path[length] == '/' || path[length] == '\0');
}

static void associate_volume(SteamLibrary *library, const VolumeInventory *volumes)
{
    size_t index;
    size_t best_length = 0;

    if (volumes == NULL) return;
    for (index = 0; index < volumes->count; index++) {
        const Volume *volume = &volumes->items[index];
        size_t length = strlen(volume->mountpoint);

        if (!volume->mounted || length == 0 || length < best_length || !path_prefix(library->path, volume->mountpoint)) continue;
        (void)snprintf(library->volume_path, sizeof(library->volume_path), "%s", volume->path);
        (void)snprintf(library->filesystem, sizeof(library->filesystem), "%s", volume->filesystem);
        library->available_bytes = volume->available_bytes;
        library->mounted = true;
        best_length = length;
    }
}

static bool add_library(SteamInfo *steam, const char *path, const VolumeInventory *volumes)
{
    SteamLibrary *library;
    size_t index;
    char steamapps[VOLUME_TEXT_CAPACITY * 2U];
    struct stat metadata;

    for (index = 0; index < steam->library_count; index++) {
        if (strcmp(steam->libraries[index].path, path) == 0) return true;
    }
    if (steam->library_count == STEAM_LIBRARY_LIMIT) {
        steam->inventory_truncated = true;
        return false;
    }
    if (snprintf(steamapps, sizeof(steamapps), "%s/steamapps", path) >= (int)sizeof(steamapps) ||
        stat(steamapps, &metadata) != 0 || !S_ISDIR(metadata.st_mode)) return false;
    for (index = 0; index < steam->library_count; index++) {
        char existing[VOLUME_TEXT_CAPACITY * 2U];
        struct stat known;

        if (snprintf(existing, sizeof(existing), "%s/steamapps", steam->libraries[index].path) < (int)sizeof(existing) &&
            stat(existing, &known) == 0 && known.st_dev == metadata.st_dev && known.st_ino == metadata.st_ino) return true;
    }
    library = &steam->libraries[steam->library_count++];
    (void)snprintf(library->path, sizeof(library->path), "%s", path);
    library->writable = access(steamapps, W_OK) == 0;
    associate_volume(library, volumes);
    if (!library->mounted) {
        struct statvfs filesystem;

        if (statvfs(steamapps, &filesystem) == 0) {
            uint64_t block_size = filesystem.f_frsize == 0 ? filesystem.f_bsize : filesystem.f_frsize;
            library->available_bytes = filesystem.f_bavail * block_size;
            library->mounted = true;
        }
    }
    return true;
}

static bool vdf_value(const char *line, const char *key, char *value, size_t value_size)
{
    const char *name = strstr(line, key);
    const char *start;
    const char *end;

    if (name == NULL) return false;
    start = strchr(name + strlen(key), '"');
    if (start == NULL) return false;
    start++;
    end = strchr(start, '"');
    if (end == NULL || (size_t)(end - start) >= value_size) return false;
    (void)memcpy(value, start, (size_t)(end - start));
    value[end - start] = '\0';
    return true;
}

static bool steam_icon_filename(const char *name)
{
    size_t index;

    if (strlen(name) != 44U || strcmp(name + 40U, ".jpg") != 0) return false;
    for (index = 0U; index < 40U; index++) {
        if (!isxdigit((unsigned char)name[index])) return false;
    }
    return true;
}

static void collect_game_icon(SteamGame *game, const char *client_root)
{
    char directory_path[VOLUME_TEXT_CAPACITY * 2U];
    DIR *directory;
    struct dirent *entry;

    if (snprintf(directory_path, sizeof(directory_path), "%s/appcache/librarycache/%s",
        client_root, game->appid) >= (int)sizeof(directory_path)) return;
    directory = opendir(directory_path);
    if (directory == NULL) return;
    while ((entry = readdir(directory)) != NULL) {
        char candidate[VOLUME_TEXT_CAPACITY * 2U];
        struct stat metadata;

        if (!steam_icon_filename(entry->d_name) ||
            snprintf(candidate, sizeof(candidate), "%s/%s", directory_path, entry->d_name) >= (int)sizeof(candidate) ||
            stat(candidate, &metadata) != 0 || !S_ISREG(metadata.st_mode) || metadata.st_size <= 0 ||
            (uintmax_t)metadata.st_size > STEAM_ICON_MAX_BYTES) continue;
        (void)snprintf(game->icon_path, sizeof(game->icon_path), "%s", candidate);
        break;
    }
    (void)closedir(directory);
}

bool steam_app_is_tool(const char *appid, const char *name)
{
    static const char *const tool_appids[] = {
        "228980", /* Steamworks Common Redistributables */
        "1070560", /* Steam Linux Runtime */
        "1391110", /* Steam Linux Runtime - Soldier */
        "1493710", /* Steam Linux Runtime - Sniper */
        "1628350" /* Steam Linux Runtime 3.0 */
    };
    size_t index;

    if (appid != NULL) {
        for (index = 0U; index < sizeof(tool_appids) / sizeof(tool_appids[0]); index++) {
            if (strcmp(appid, tool_appids[index]) == 0) return true;
        }
    }
    if (name == NULL) return false;
    return strstr(name, "Proton") != NULL || strstr(name, "Steam Linux Runtime") != NULL ||
        strstr(name, "Steam Runtime") != NULL ||
        strstr(name, "Steamworks Common Redistributables") != NULL ||
        strstr(name, "Steam Input Configs") != NULL ||
        strstr(name, "Steam Controller Configs") != NULL || strcmp(name, "SteamVR") == 0;
}

static void collect_games(SteamInfo *steam, size_t library_index, const char *client_root)
{
    SteamLibrary *library = &steam->libraries[library_index];
    char steamapps[VOLUME_TEXT_CAPACITY * 2U];
    DIR *directory;
    struct dirent *entry;

    if (snprintf(steamapps, sizeof(steamapps), "%s/steamapps", library->path) >= (int)sizeof(steamapps) ||
        (directory = opendir(steamapps)) == NULL) return;
    while ((entry = readdir(directory)) != NULL) {
        char manifest_path[VOLUME_TEXT_CAPACITY * 2U];
        char appid[32] = {0};
        char name[VOLUME_TEXT_CAPACITY] = {0};
        char install_directory[VOLUME_TEXT_CAPACITY] = {0};
        char size[32] = {0};
        FILE *manifest;
        char line[512];
        SteamGame *game;
        struct stat metadata;

        if (strncmp(entry->d_name, "appmanifest_", 12) != 0 || strstr(entry->d_name, ".acf") == NULL) continue;
        if (snprintf(manifest_path, sizeof(manifest_path), "%s/%s", steamapps, entry->d_name) >= (int)sizeof(manifest_path) ||
            (manifest = fopen(manifest_path, "r")) == NULL) continue;
        while (fgets(line, sizeof(line), manifest) != NULL) {
            if (vdf_value(line, "\"appid\"", appid, sizeof(appid))) continue;
            if (vdf_value(line, "\"name\"", name, sizeof(name))) continue;
            if (vdf_value(line, "\"installdir\"", install_directory, sizeof(install_directory))) continue;
            (void)vdf_value(line, "\"SizeOnDisk\"", size, sizeof(size));
        }
        (void)fclose(manifest);
        if (appid[0] == '\0' || steam->game_count == STEAM_GAME_LIMIT) {
            if (steam->game_count == STEAM_GAME_LIMIT) steam->inventory_truncated = true;
            continue;
        }
        game = &steam->games[steam->game_count++];
        (void)snprintf(game->appid, sizeof(game->appid), "%s", appid);
        (void)snprintf(game->name, sizeof(game->name), "%s", name[0] == '\0' ? appid : name);
        collect_game_icon(game, client_root);
        game->size_bytes = strtoull(size, NULL, 10);
        game->library_index = library_index;
        game->is_tool = steam_app_is_tool(game->appid, game->name);
        if (snprintf(manifest_path, sizeof(manifest_path), "%s/common/%s", steamapps, install_directory) < (int)sizeof(manifest_path) &&
            stat(manifest_path, &metadata) == 0 && S_ISDIR(metadata.st_mode)) game->directory_present = true;
        if (game->is_tool) {
            library->tool_count++;
            library->tool_bytes += game->size_bytes;
        } else {
            library->game_count++;
            library->game_bytes += game->size_bytes;
        }
    }
    (void)closedir(directory);
}

static void collect_library_paths(SteamInfo *steam, const char *root, const VolumeInventory *volumes)
{
    char configuration[VOLUME_TEXT_CAPACITY * 2U];
    FILE *stream;
    char line[1024];
    size_t index;
    size_t first_library = steam->library_count;

    (void)add_library(steam, root, volumes);
    if (snprintf(configuration, sizeof(configuration), "%s/steamapps/libraryfolders.vdf", root) >= (int)sizeof(configuration) ||
        (stream = fopen(configuration, "r")) == NULL) goto collect;
    while (fgets(line, sizeof(line), stream) != NULL) {
        char path[VOLUME_TEXT_CAPACITY];
        if (vdf_value(line, "\"path\"", path, sizeof(path))) (void)add_library(steam, path, volumes);
    }
    (void)fclose(stream);
collect:
    for (index = first_library; index < steam->library_count; index++) collect_games(steam, index, root);
}

static int compare_games_by_size(const void *first, const void *second)
{
    const SteamGame *left = first;
    const SteamGame *right = second;

    if (left->size_bytes < right->size_bytes) return 1;
    if (left->size_bytes > right->size_bytes) return -1;
    return strcmp(left->name, right->name);
}

int steam_collect(SteamInfo *steam, const VolumeInventory *volumes, char *error, size_t error_size)
{
    if (steam == NULL) {
        set_error(error, error_size, "Steam destination is missing.");
        return -1;
    }
    *steam = (SteamInfo){0};
    collect_os_release(steam);
    collect_dpkg(steam);
    collect_controllers(steam);
    {
        const char *home = getenv("HOME");
        char root[VOLUME_TEXT_CAPACITY];

        if (home != NULL && snprintf(root, sizeof(root), "%s/.local/share/Steam", home) < (int)sizeof(root))
            collect_library_paths(steam, root, volumes);
        if (home != NULL && snprintf(root, sizeof(root), "%s/.steam/steam", home) < (int)sizeof(root))
            collect_library_paths(steam, root, volumes);
        if (home != NULL && snprintf(root, sizeof(root), "%s/.var/app/com.valvesoftware.Steam/data/Steam", home) < (int)sizeof(root))
            collect_library_paths(steam, root, volumes);
    }
    qsort(steam->games, steam->game_count, sizeof(steam->games[0]), compare_games_by_size);
    return 0;
}
