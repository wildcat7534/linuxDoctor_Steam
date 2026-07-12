#define _POSIX_C_SOURCE 200809L

#include "volume.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

#define LSBLK_OUTPUT_CAPACITY 131072U

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0) (void)snprintf(error, error_size, "%s", message);
}

static const char *skip_space(const char *cursor)
{
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) cursor++;
    return cursor;
}

static const char *skip_string(const char *cursor)
{
    if (*cursor != '"') return cursor;
    cursor++;
    while (*cursor != '\0') {
        if (*cursor == '\\' && cursor[1] != '\0') cursor += 2;
        else if (*cursor++ == '"') break;
    }
    return cursor;
}

static const char *skip_value(const char *cursor)
{
    int depth;

    cursor = skip_space(cursor);
    if (*cursor == '"') return skip_string(cursor);
    if (*cursor != '{' && *cursor != '[') {
        while (*cursor != '\0' && *cursor != ',' && *cursor != '}' && *cursor != ']') cursor++;
        return cursor;
    }
    depth = 0;
    do {
        if (*cursor == '"') cursor = skip_string(cursor);
        else {
            if (*cursor == '{' || *cursor == '[') depth++;
            if (*cursor == '}' || *cursor == ']') depth--;
            cursor++;
        }
    } while (*cursor != '\0' && depth > 0);
    return cursor;
}

static bool read_string(const char *cursor, char *destination, size_t destination_size)
{
    size_t length = 0;

    cursor = skip_space(cursor);
    if (*cursor != '"' || destination_size == 0) return false;
    cursor++;
    while (*cursor != '\0' && *cursor != '"' && length + 1U < destination_size) {
        if (*cursor == '\\' && cursor[1] != '\0') cursor++;
        destination[length++] = *cursor++;
    }
    destination[length] = '\0';
    return *cursor == '"';
}

static bool find_field(const char *object, const char *key, const char **value)
{
    const char *cursor = skip_space(object);
    size_t key_length = strlen(key);

    if (*cursor != '{') return false;
    cursor++;
    for (;;) {
        const char *name;
        const char *end;

        cursor = skip_space(cursor);
        if (*cursor == '}') return false;
        if (*cursor != '"') return false;
        name = ++cursor;
        end = strchr(name, '"');
        if (end == NULL) return false;
        cursor = skip_space(end + 1);
        if (*cursor++ != ':') return false;
        cursor = skip_space(cursor);
        if ((size_t)(end - name) == key_length && strncmp(name, key, key_length) == 0) {
            *value = cursor;
            return true;
        }
        cursor = skip_space(skip_value(cursor));
        if (*cursor == ',') cursor++;
        else if (*cursor != '}') return false;
    }
}

static void read_first_array_string(const char *value, char *destination, size_t destination_size)
{
    value = skip_space(value);
    if (*value == '[') {
        value = skip_space(value + 1);
        if (*value == '"') (void)read_string(value, destination, destination_size);
    } else if (*value == '"') {
        (void)read_string(value, destination, destination_size);
    }
}

static int run_lsblk(char output[LSBLK_OUTPUT_CAPACITY], char *error, size_t error_size)
{
    int descriptors[2];
    posix_spawn_file_actions_t actions;
    pid_t process;
    const char *arguments[] = {"lsblk", "--json", "--bytes", "--output",
        "PATH,PKNAME,TYPE,SIZE,FSTYPE,UUID,LABEL,PARTLABEL,PARTTYPE,MOUNTPOINTS,RO,RM,TRAN,MODEL", NULL};
    size_t length = 0;
    ssize_t read_count;
    int status;

    if (pipe(descriptors) != 0 || posix_spawn_file_actions_init(&actions) != 0) {
        set_error(error, error_size, strerror(errno));
        return -1;
    }
    (void)posix_spawn_file_actions_adddup2(&actions, descriptors[1], STDOUT_FILENO);
    (void)posix_spawn_file_actions_addclose(&actions, descriptors[0]);
    (void)posix_spawn_file_actions_addclose(&actions, descriptors[1]);
    if (posix_spawnp(&process, "lsblk", &actions, NULL, (char *const *)arguments, environ) != 0) {
        (void)posix_spawn_file_actions_destroy(&actions);
        (void)close(descriptors[0]);
        (void)close(descriptors[1]);
        set_error(error, error_size, "Unable to start lsblk.");
        return -1;
    }
    (void)posix_spawn_file_actions_destroy(&actions);
    (void)close(descriptors[1]);
    while (length + 1U < LSBLK_OUTPUT_CAPACITY &&
        (read_count = read(descriptors[0], output + length, LSBLK_OUTPUT_CAPACITY - length - 1U)) > 0) length += (size_t)read_count;
    (void)close(descriptors[0]);
    output[length] = '\0';
    if (waitpid(process, &status, 0) < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0 || length + 1U == LSBLK_OUTPUT_CAPACITY) {
        set_error(error, error_size, "lsblk did not return a complete inventory.");
        return -1;
    }
    return 0;
}

static void collect_object(VolumeInventory *inventory, const char *object)
{
    const char *value;
    Volume volume = {0};
    char type[32] = {0};
    struct statvfs filesystem;

    if (!find_field(object, "type", &value) || !read_string(value, type, sizeof(type)) ||
        (strcmp(type, "part") != 0 && strcmp(type, "disk") != 0) ||
        !find_field(object, "path", &value) || !read_string(value, volume.path, sizeof(volume.path))) return;
    if (find_field(object, "fstype", &value)) (void)read_string(value, volume.filesystem, sizeof(volume.filesystem));
    if (find_field(object, "uuid", &value)) (void)read_string(value, volume.uuid, sizeof(volume.uuid));
    if (find_field(object, "label", &value)) (void)read_string(value, volume.label, sizeof(volume.label));
    if (find_field(object, "partlabel", &value)) (void)read_string(value, volume.partition_label, sizeof(volume.partition_label));
    if (find_field(object, "parttype", &value)) (void)read_string(value, volume.partition_type, sizeof(volume.partition_type));
    if (find_field(object, "transport", &value)) (void)read_string(value, volume.transport, sizeof(volume.transport));
    if (find_field(object, "model", &value)) (void)read_string(value, volume.model, sizeof(volume.model));
    if (find_field(object, "mountpoints", &value)) read_first_array_string(value, volume.mountpoint, sizeof(volume.mountpoint));
    if (find_field(object, "pkname", &value)) {
        char parent[128];
        if (read_string(value, parent, sizeof(parent))) (void)snprintf(volume.parent_path, sizeof(volume.parent_path), "/dev/%s", parent);
    }
    if (find_field(object, "size", &value)) volume.size_bytes = strtoull(value, NULL, 10);
    if (find_field(object, "ro", &value)) volume.read_only = strncmp(value, "true", 4) == 0 || *value == '1';
    if (find_field(object, "rm", &value)) volume.removable = strncmp(value, "true", 4) == 0 || *value == '1';
    volume.mounted = volume.mountpoint[0] != '\0';
    volume.windows_system_component = strstr(volume.partition_type, "e3c9e316") != NULL ||
        strstr(volume.partition_type, "de94bba4") != NULL || strstr(volume.partition_type, "0x27") != NULL ||
        strstr(volume.partition_label, "Microsoft") != NULL || strstr(volume.label, "Réservé au système") != NULL;
    volume.windows_data_partition = strcmp(volume.filesystem, "ntfs") == 0 &&
        (strstr(volume.partition_type, "ebd0a0a2") != NULL || strstr(volume.partition_type, "0x7") != NULL);
    if (volume.mounted) {
        DIR *directory = opendir(volume.mountpoint);
        struct dirent *entry;

        if (directory != NULL) {
            while ((entry = readdir(directory)) != NULL) {
                char windows_directory[VOLUME_TEXT_CAPACITY * 2U];
                struct stat metadata;

                if (strcasecmp(entry->d_name, "windows") != 0) continue;
                if (snprintf(windows_directory, sizeof(windows_directory), "%s/%s/System32", volume.mountpoint, entry->d_name) < (int)sizeof(windows_directory) &&
                    stat(windows_directory, &metadata) == 0 && S_ISDIR(metadata.st_mode)) volume.windows_confirmed = true;
                break;
            }
            (void)closedir(directory);
        }
    }
    if (volume.filesystem[0] == '\0' && !volume.mounted) return;
    if (volume.mounted && statvfs(volume.mountpoint, &filesystem) == 0) {
        uint64_t block_size = filesystem.f_frsize == 0 ? filesystem.f_bsize : filesystem.f_frsize;
        volume.available_bytes = filesystem.f_bavail * block_size;
        if (filesystem.f_blocks > 0) volume.used_percent = (unsigned int)(((filesystem.f_blocks - filesystem.f_bavail) * 100U) / filesystem.f_blocks);
    }
    if (inventory->count == VOLUME_LIMIT) {
        inventory->truncated = true;
        return;
    }
    inventory->items[inventory->count++] = volume;
}

static void mark_windows_disks(VolumeInventory *inventory)
{
    size_t first;

    for (first = 0; first < inventory->count; first++) {
        bool confirmed = false;
        size_t second;

        for (second = 0; second < inventory->count; second++) {
            if (strcmp(inventory->items[first].parent_path, inventory->items[second].parent_path) != 0) continue;
            confirmed = confirmed || inventory->items[second].windows_confirmed;
        }
        if (confirmed) {
            for (second = 0; second < inventory->count; second++) {
                if (strcmp(inventory->items[first].parent_path, inventory->items[second].parent_path) == 0)
                    inventory->items[second].windows_protected = true;
            }
        }
    }
}

static void walk_objects(VolumeInventory *inventory, const char *cursor)
{
    while (*cursor != '\0') {
        if (*cursor == '"') cursor = skip_string(cursor);
        else if (*cursor == '{') {
            collect_object(inventory, cursor);
            cursor++;
        } else cursor++;
    }
}

int volume_collect(VolumeInventory *inventory, char *error, size_t error_size)
{
    char output[LSBLK_OUTPUT_CAPACITY];

    if (inventory == NULL) {
        set_error(error, error_size, "Volume destination is missing.");
        return -1;
    }
    if (run_lsblk(output, error, error_size) != 0) return -1;
    return volume_parse_lsblk(inventory, output, error, error_size);
}

int volume_parse_lsblk(VolumeInventory *inventory, const char *json, char *error, size_t error_size)
{
    if (inventory == NULL || json == NULL || strstr(json, "\"blockdevices\"") == NULL) {
        set_error(error, error_size, "lsblk JSON is missing blockdevices.");
        return -1;
    }
    *inventory = (VolumeInventory){0};
    walk_objects(inventory, json);
    mark_windows_disks(inventory);
    inventory->available = true;
    return 0;
}
