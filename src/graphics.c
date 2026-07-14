#define _POSIX_C_SOURCE 200809L

#include "graphics.h"

#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#define DRM_CLASS_PATH "/sys/class/drm"

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0U) (void)snprintf(error, error_size, "%s", message);
}

static void copy_text(char *destination, size_t capacity, const char *source)
{
    if (destination == NULL || capacity == 0U) return;
    (void)snprintf(destination, capacity, "%s", source != NULL ? source : "");
}

static void trim_line(char *line)
{
    size_t length;

    if (line == NULL) return;
    length = strlen(line);
    while (length > 0U && (line[length - 1U] == '\n' || line[length - 1U] == '\r')) {
        line[--length] = '\0';
    }
}

static void set_vendor(GraphicsDevice *device)
{
    const char *identifier;

    if (device == NULL) return;
    identifier = device->vendor_id;
    if (strncasecmp(identifier, "0x", 2U) == 0) identifier += 2;
    if (strcasecmp(identifier, "10de") == 0) {
        copy_text(device->vendor, sizeof(device->vendor), "NVIDIA");
    } else if (strcasecmp(identifier, "1002") == 0) {
        copy_text(device->vendor, sizeof(device->vendor), "AMD");
    } else if (strcasecmp(identifier, "8086") == 0) {
        copy_text(device->vendor, sizeof(device->vendor), "Intel");
    } else if (strcasecmp(identifier, "13b5") == 0) {
        copy_text(device->vendor, sizeof(device->vendor), "Arm");
    } else if (strcasecmp(identifier, "5143") == 0) {
        copy_text(device->vendor, sizeof(device->vendor), "Qualcomm");
    } else {
        copy_text(device->vendor, sizeof(device->vendor), "Inconnu");
    }
}

static void parse_pci_id(GraphicsDevice *device, const char *identifier)
{
    const char *separator;
    size_t vendor_length;
    size_t device_length;

    if (device == NULL || identifier == NULL) return;
    separator = strchr(identifier, ':');
    if (separator == NULL) return;
    vendor_length = (size_t)(separator - identifier);
    device_length = strlen(separator + 1);
    if (vendor_length == 0U || vendor_length >= sizeof(device->vendor_id) ||
        device_length == 0U || device_length >= sizeof(device->device_id)) return;
    (void)memcpy(device->vendor_id, identifier, vendor_length);
    device->vendor_id[vendor_length] = '\0';
    (void)memcpy(device->device_id, separator + 1, device_length + 1U);
}

int graphics_parse_uevent(FILE *stream, GraphicsDevice *device)
{
    char line[512];

    if (stream == NULL || device == NULL) return -1;
    while (fgets(line, sizeof(line), stream) != NULL) {
        trim_line(line);
        if (strncmp(line, "DRIVER=", 7U) == 0) {
            copy_text(device->driver, sizeof(device->driver), line + 7U);
        } else if (strncmp(line, "PCI_ID=", 7U) == 0) {
            parse_pci_id(device, line + 7U);
        }
    }
    if (ferror(stream)) return -1;
    set_vendor(device);
    return 0;
}

static bool is_card_name(const char *name)
{
    size_t index;

    if (name == NULL || strncmp(name, "card", 4U) != 0 || name[4] == '\0') return false;
    for (index = 4U; name[index] != '\0'; index++) {
        if (!isdigit((unsigned char)name[index])) return false;
    }
    return true;
}

static int read_text_file(const char *path, char *destination, size_t capacity)
{
    FILE *stream;

    if (path == NULL || destination == NULL || capacity == 0U) return -1;
    stream = fopen(path, "r");
    if (stream == NULL) return -1;
    if (fgets(destination, (int)capacity, stream) == NULL) {
        (void)fclose(stream);
        return -1;
    }
    trim_line(destination);
    return fclose(stream) == 0 ? 0 : -1;
}

static void remove_hex_prefix(char *identifier)
{
    size_t length;

    if (identifier == NULL || strncasecmp(identifier, "0x", 2U) != 0) return;
    length = strlen(identifier + 2U);
    (void)memmove(identifier, identifier + 2U, length + 1U);
}

static bool read_boot_vga(const char *path)
{
    char value[8];

    return read_text_file(path, value, sizeof(value)) == 0 && strcmp(value, "1") == 0;
}

static void complete_device_from_sysfs(GraphicsDevice *device, const char *card)
{
    char path[512];

    if (device == NULL || card == NULL) return;
    if (device->vendor_id[0] == '\0' &&
        snprintf(path, sizeof(path), "%s/%s/device/vendor", DRM_CLASS_PATH, card) < (int)sizeof(path)) {
        (void)read_text_file(path, device->vendor_id, sizeof(device->vendor_id));
        remove_hex_prefix(device->vendor_id);
    }
    if (device->device_id[0] == '\0' &&
        snprintf(path, sizeof(path), "%s/%s/device/device", DRM_CLASS_PATH, card) < (int)sizeof(path)) {
        (void)read_text_file(path, device->device_id, sizeof(device->device_id));
        remove_hex_prefix(device->device_id);
    }
    if (snprintf(path, sizeof(path), "%s/%s/device/boot_vga", DRM_CLASS_PATH, card) < (int)sizeof(path)) {
        device->boot_vga = read_boot_vga(path);
    }
    set_vendor(device);
}

static void collect_devices(GraphicsInfo *graphics)
{
    DIR *directory;
    struct dirent *entry;

    directory = opendir(DRM_CLASS_PATH);
    if (directory == NULL) return;
    graphics->device_inventory_available = true;
    for (;;) {
        GraphicsDevice *device;
        char path[512];
        FILE *stream;

        errno = 0;
        entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) graphics->device_inventory_available = false;
            break;
        }
        if (!is_card_name(entry->d_name)) continue;
        if (graphics->device_count >= GRAPHICS_DEVICE_LIMIT) {
            graphics->inventory_truncated = true;
            continue;
        }
        device = &graphics->devices[graphics->device_count++];
        *device = (GraphicsDevice){0};
        copy_text(device->card, sizeof(device->card), entry->d_name);
        if (snprintf(path, sizeof(path), "%s/%s/device/uevent", DRM_CLASS_PATH, entry->d_name) < (int)sizeof(path)) {
            stream = fopen(path, "r");
            if (stream != NULL) {
                (void)graphics_parse_uevent(stream, device);
                (void)fclose(stream);
            }
        }
        complete_device_from_sysfs(device, entry->d_name);
    }
    (void)closedir(directory);
}

static bool shared_library_available(const char *name)
{
    void *handle;

    handle = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
    if (handle == NULL) return false;
    (void)dlclose(handle);
    return true;
}

static bool has_json_suffix(const char *name)
{
    size_t length;

    if (name == NULL) return false;
    length = strlen(name);
    return length > 5U && strcmp(name + length - 5U, ".json") == 0;
}

static bool manifest_file_available(const char *path)
{
    static const char key[] = "\"library_path\"";
    struct stat metadata;
    FILE *stream;
    size_t matched = 0U;
    int character;
    bool found = false;

    if (path == NULL || stat(path, &metadata) != 0 || !S_ISREG(metadata.st_mode)) return false;
    stream = fopen(path, "r");
    if (stream == NULL) return false;
    while ((character = fgetc(stream)) != EOF) {
        if ((char)character == key[matched]) {
            matched++;
            if (matched == sizeof(key) - 1U) {
                found = true;
                break;
            }
        } else {
            matched = (char)character == key[0] ? 1U : 0U;
        }
    }
    (void)fclose(stream);
    return found;
}

static size_t count_icd_manifests(const char *path)
{
    DIR *directory;
    struct dirent *entry;
    size_t count = 0U;

    directory = opendir(path);
    if (directory == NULL) return 0U;
    while ((entry = readdir(directory)) != NULL) {
        char manifest_path[4096];

        if (!has_json_suffix(entry->d_name) ||
            snprintf(manifest_path, sizeof(manifest_path), "%s/%s", path, entry->d_name) >= (int)sizeof(manifest_path)) continue;
        if (manifest_file_available(manifest_path) && count < SIZE_MAX) count++;
    }
    (void)closedir(directory);
    return count;
}

static size_t count_manifest_file_list(const char *paths)
{
    const char *cursor = paths;
    size_t count = 0U;

    while (cursor != NULL && cursor[0] != '\0') {
        const char *separator = strchr(cursor, ':');
        size_t length = separator != NULL ? (size_t)(separator - cursor) : strlen(cursor);
        char path[4096];

        if (length > 0U && length < sizeof(path)) {
            struct stat metadata;

            (void)memcpy(path, cursor, length);
            path[length] = '\0';
            if (stat(path, &metadata) == 0 && S_ISDIR(metadata.st_mode)) {
                count += count_icd_manifests(path);
            } else if (manifest_file_available(path) && count < SIZE_MAX) {
                count++;
            }
        }
        cursor = separator != NULL ? separator + 1U : NULL;
    }
    return count;
}

static size_t count_manifest_directory_list(const char *paths)
{
    const char *cursor = paths;
    size_t count = 0U;

    while (cursor != NULL && cursor[0] != '\0') {
        const char *separator = strchr(cursor, ':');
        size_t length = separator != NULL ? (size_t)(separator - cursor) : strlen(cursor);
        char path[4096];

        if (length > 0U && length + sizeof("/vulkan/icd.d") <= sizeof(path)) {
            (void)memcpy(path, cursor, length);
            path[length] = '\0';
            (void)strcat(path, "/vulkan/icd.d");
            count += count_icd_manifests(path);
        }
        cursor = separator != NULL ? separator + 1U : NULL;
    }
    return count;
}

static size_t collect_vulkan_icds(void)
{
    const char *override_paths;
    const char *additional_paths;
    const char *config_home;
    const char *config_directories;
    const char *data_home;
    const char *data_directories;
    const char *home;
    char user_path[4096];
    size_t count = 0U;

    override_paths = getenv("VK_DRIVER_FILES");
    if (override_paths == NULL || override_paths[0] == '\0') override_paths = getenv("VK_ICD_FILENAMES");
    if (override_paths != NULL && override_paths[0] != '\0') return count_manifest_file_list(override_paths);

    count += count_icd_manifests("/etc/vulkan/icd.d");
    home = getenv("HOME");
    config_home = getenv("XDG_CONFIG_HOME");
    if (config_home != NULL && config_home[0] != '\0' &&
        snprintf(user_path, sizeof(user_path), "%s/vulkan/icd.d", config_home) < (int)sizeof(user_path)) {
        count += count_icd_manifests(user_path);
    } else if (home != NULL && home[0] != '\0' &&
        snprintf(user_path, sizeof(user_path), "%s/.config/vulkan/icd.d", home) < (int)sizeof(user_path)) {
        count += count_icd_manifests(user_path);
    }
    config_directories = getenv("XDG_CONFIG_DIRS");
    count += count_manifest_directory_list(config_directories != NULL && config_directories[0] != '\0' ?
        config_directories : "/etc/xdg");

    data_home = getenv("XDG_DATA_HOME");
    if (data_home != NULL && data_home[0] != '\0' &&
        snprintf(user_path, sizeof(user_path), "%s/vulkan/icd.d", data_home) < (int)sizeof(user_path)) {
        count += count_icd_manifests(user_path);
    } else if (home != NULL && home[0] != '\0' &&
        snprintf(user_path, sizeof(user_path), "%s/.local/share/vulkan/icd.d", home) < (int)sizeof(user_path)) {
        count += count_icd_manifests(user_path);
    }
    data_directories = getenv("XDG_DATA_DIRS");
    count += count_manifest_directory_list(data_directories != NULL && data_directories[0] != '\0' ?
        data_directories : "/usr/local/share:/usr/share");

    additional_paths = getenv("VK_ADD_DRIVER_FILES");
    if (additional_paths != NULL && additional_paths[0] != '\0') count += count_manifest_file_list(additional_paths);
    return count;
}

static void collect_session(GraphicsInfo *graphics)
{
    const char *session;
    const char *wayland_display;
    const char *x11_display;

    session = getenv("XDG_SESSION_TYPE");
    if (session != NULL && session[0] != '\0') {
        copy_text(graphics->session_type, sizeof(graphics->session_type), session);
    }
    wayland_display = getenv("WAYLAND_DISPLAY");
    x11_display = getenv("DISPLAY");
    if (graphics->session_type[0] == '\0' && wayland_display != NULL && wayland_display[0] != '\0') {
        copy_text(graphics->session_type, sizeof(graphics->session_type), "wayland");
    } else if (graphics->session_type[0] == '\0' && x11_display != NULL && x11_display[0] != '\0') {
        copy_text(graphics->session_type, sizeof(graphics->session_type), "x11");
    }
    graphics->wayland_session = strcasecmp(graphics->session_type, "wayland") == 0;
    graphics->x11_session = strcasecmp(graphics->session_type, "x11") == 0;
    graphics->session_available = graphics->wayland_session || graphics->x11_session;
}

int graphics_collect(GraphicsInfo *graphics, char *error, size_t error_size)
{
    if (graphics == NULL) {
        set_error(error, error_size, "Graphics destination is missing.");
        return -1;
    }
    *graphics = (GraphicsInfo){0};
    collect_devices(graphics);
    collect_session(graphics);
    graphics->vulkan_loader_available = shared_library_available("libvulkan.so.1");
    graphics->vulkan_icd_count = collect_vulkan_icds();
    graphics->opengl_loader_available = shared_library_available("libGL.so.1");
    return 0;
}
