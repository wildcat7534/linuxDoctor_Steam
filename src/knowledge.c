#define _POSIX_C_SOURCE 200809L

#include "knowledge.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0U) (void)snprintf(error, error_size, "%s", message);
}

static bool copy_field(char *destination, size_t capacity, const char *value)
{
    size_t length = strlen(value);

    if (capacity == 0U || length >= capacity) return false;
    (void)memcpy(destination, value, length + 1U);
    return true;
}

static size_t split_fields(char *line, char **fields, size_t capacity)
{
    size_t count = 0U;
    char *cursor = line;

    for (;;) {
        char *separator;

        if (count == capacity) return capacity + 1U;
        fields[count++] = cursor;
        separator = strchr(cursor, '\t');
        if (separator == NULL) return count;
        *separator = '\0';
        cursor = separator + 1U;
    }
}

static bool valid_kind(const char *kind)
{
    return strcmp(kind, "game") == 0 || strcmp(kind, "steam") == 0 ||
        strcmp(kind, "controller") == 0 || strcmp(kind, "gfn") == 0 ||
        strcmp(kind, "ubuntu") == 0;
}

static bool valid_severity(const char *severity)
{
    return strcmp(severity, "info") == 0 || strcmp(severity, "warning") == 0 ||
        strcmp(severity, "problem") == 0;
}

static bool valid_version(const char *version)
{
    size_t index;

    if (version == NULL || version[0] == '\0') return false;
    for (index = 0U; version[index] != '\0'; index++) {
        unsigned char byte = (unsigned char)version[index];

        if (!isalnum(byte) && byte != '.' && byte != '-' && byte != '_') return false;
    }
    return true;
}

static bool valid_iso_date(const char *date)
{
    static const unsigned int month_days[] = {31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U};
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int limit;
    size_t index;

    if (date == NULL || strlen(date) != 10U || date[4] != '-' || date[7] != '-') return false;
    for (index = 0U; index < 10U; index++) {
        if (index != 4U && index != 7U && !isdigit((unsigned char)date[index])) return false;
    }
    year = (unsigned int)(date[0] - '0') * 1000U + (unsigned int)(date[1] - '0') * 100U +
        (unsigned int)(date[2] - '0') * 10U + (unsigned int)(date[3] - '0');
    month = (unsigned int)(date[5] - '0') * 10U + (unsigned int)(date[6] - '0');
    day = (unsigned int)(date[8] - '0') * 10U + (unsigned int)(date[9] - '0');
    if (year == 0U || month == 0U || month > 12U) return false;
    limit = month_days[month - 1U];
    if (month == 2U && (year % 400U == 0U || (year % 4U == 0U && year % 100U != 0U))) limit = 29U;
    return day > 0U && day <= limit;
}

static bool valid_https_url(const char *url)
{
    const char *host;

    if (url == NULL || strncmp(url, "https://", 8U) != 0) return false;
    host = url + 8U;
    return host[0] != '\0' && host[0] != '/' && strchr(host, '.') != NULL;
}

static bool game_installed(const SteamInfo *steam, const char *appid)
{
    size_t index;

    if (steam == NULL) return false;
    for (index = 0U; index < steam->game_count; index++) {
        if (!steam->games[index].is_tool && strcmp(steam->games[index].appid, appid) == 0) return true;
    }
    return false;
}

static bool entry_relevant(const GamingKnowledgeEntry *entry, const SteamInfo *steam,
    const GeForceNowInfo *gfn)
{
    if (strcmp(entry->kind, "game") == 0) return game_installed(steam, entry->target);
    if (strcmp(entry->kind, "steam") == 0) return steam != NULL &&
        strcmp(entry->target, "steam-client") == 0 && steam->library_count > 0U;
    if (strcmp(entry->kind, "controller") == 0) return steam != NULL &&
        strcmp(entry->target, "steam-controller") == 0 &&
        (steam->controller_count > 0U || !steam->steam_devices_installed);
    if (strcmp(entry->kind, "gfn") == 0) return gfn != NULL &&
        strcmp(entry->target, "geforce-now") == 0 && gfn->installed;
    if (strcmp(entry->kind, "ubuntu") == 0 && steam != NULL) {
        if (strcmp(entry->target, "ubuntu-26.04") == 0) return steam->ubuntu_2604;
        return strcmp(entry->target, "ubuntu") == 0 && steam->ubuntu;
    }
    return false;
}

static bool parse_entry(GamingKnowledgeEntry *entry, char *line)
{
    char *fields[8];
    size_t count = split_fields(line, fields, 8U);

    if (count != 8U || !valid_kind(fields[0]) || fields[1][0] == '\0' ||
        !valid_severity(fields[2]) || fields[3][0] == '\0' || fields[4][0] == '\0' ||
        fields[5][0] == '\0' || !valid_https_url(fields[6]) || !valid_iso_date(fields[7])) return false;
    return copy_field(entry->kind, sizeof(entry->kind), fields[0]) &&
        copy_field(entry->target, sizeof(entry->target), fields[1]) &&
        copy_field(entry->severity, sizeof(entry->severity), fields[2]) &&
        copy_field(entry->title, sizeof(entry->title), fields[3]) &&
        copy_field(entry->summary, sizeof(entry->summary), fields[4]) &&
        copy_field(entry->guidance, sizeof(entry->guidance), fields[5]) &&
        copy_field(entry->source_url, sizeof(entry->source_url), fields[6]) &&
        copy_field(entry->updated_on, sizeof(entry->updated_on), fields[7]);
}

static int parse_metadata(GamingKnowledgeBase *knowledge, const char *line)
{
    const char schema_prefix[] = "# linux-doctor-knowledge-schema\t";
    const char version_prefix[] = "# linux-doctor-knowledge-version\t";
    const char reviewed_prefix[] = "# reviewed-on\t";

    if (strncmp(line, schema_prefix, sizeof(schema_prefix) - 1U) == 0) {
        const char *value = line + sizeof(schema_prefix) - 1U;

        if (knowledge->schema_version != 0U || strcmp(value, "1") != 0) return -1;
        knowledge->schema_version = 1U;
        return 1;
    }
    if (strncmp(line, version_prefix, sizeof(version_prefix) - 1U) == 0) {
        const char *value = line + sizeof(version_prefix) - 1U;

        if (knowledge->version[0] != '\0' || !valid_version(value) ||
            !copy_field(knowledge->version, sizeof(knowledge->version), value)) return -1;
        return 1;
    }
    if (strncmp(line, reviewed_prefix, sizeof(reviewed_prefix) - 1U) == 0) {
        const char *value = line + sizeof(reviewed_prefix) - 1U;

        if (knowledge->reviewed_on[0] != '\0' || !valid_iso_date(value) ||
            !copy_field(knowledge->reviewed_on, sizeof(knowledge->reviewed_on), value)) return -1;
        return 1;
    }
    return 0;
}

static bool valid_loaded_base(const GamingKnowledgeBase *knowledge)
{
    return knowledge != NULL && knowledge->schema_version == GAMING_KNOWLEDGE_SCHEMA_VERSION &&
        knowledge->entry_count > 0U && knowledge->invalid_count == 0U && !knowledge->truncated &&
        valid_version(knowledge->version) && valid_iso_date(knowledge->reviewed_on);
}

int gaming_knowledge_load(GamingKnowledgeBase *knowledge, const char *path,
    const SteamInfo *steam, const GeForceNowInfo *gfn, char *error, size_t error_size)
{
    struct stat metadata;
    FILE *stream = NULL;
    char line[2048];
    int descriptor = -1;

    if (knowledge == NULL || path == NULL) {
        set_error(error, error_size, "Gaming knowledge destination or path is missing.");
        return -1;
    }
    *knowledge = (GamingKnowledgeBase){0};
    descriptor = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0 || fstat(descriptor, &metadata) != 0 || !S_ISREG(metadata.st_mode) ||
        metadata.st_size <= 0 || (uintmax_t)metadata.st_size > GAMING_KNOWLEDGE_MAX_FILE_BYTES) {
        if (descriptor >= 0) (void)close(descriptor);
        set_error(error, error_size, "Gaming knowledge database is unavailable, unsafe or too large.");
        return -1;
    }
    stream = fdopen(descriptor, "r");
    if (stream == NULL) {
        (void)close(descriptor);
        set_error(error, error_size, "Gaming knowledge database could not be opened.");
        return -1;
    }
    knowledge->available = true;
    while (fgets(line, sizeof(line), stream) != NULL) {
        GamingKnowledgeEntry parsed = {0};
        off_t position = ftello(stream);
        size_t length = strlen(line);

        if (position < 0 || (uintmax_t)position > GAMING_KNOWLEDGE_MAX_FILE_BYTES) {
            (void)fclose(stream);
            *knowledge = (GamingKnowledgeBase){0};
            set_error(error, error_size, "Gaming knowledge database grew beyond its size limit.");
            return -1;
        }
        if (length > 0U && line[length - 1U] == '\n') line[--length] = '\0';
        if (length > 0U && line[length - 1U] == '\r') line[--length] = '\0';
        if (length == 0U) continue;
        if (line[0] == '#') {
            if (parse_metadata(knowledge, line) < 0) knowledge->invalid_count++;
            continue;
        }
        if (knowledge->entry_count == GAMING_KNOWLEDGE_LIMIT) {
            knowledge->truncated = true;
            continue;
        }
        if (!parse_entry(&parsed, line)) {
            knowledge->invalid_count++;
            continue;
        }
        parsed.relevant = entry_relevant(&parsed, steam, gfn);
        if (parsed.relevant) knowledge->relevant_count++;
        knowledge->entries[knowledge->entry_count++] = parsed;
    }
    if (ferror(stream)) {
        (void)fclose(stream);
        set_error(error, error_size, "Gaming knowledge database could not be read completely.");
        return -1;
    }
    if (fclose(stream) != 0) {
        set_error(error, error_size, "Gaming knowledge database could not be closed.");
        return -1;
    }
    return 0;
}

int gaming_knowledge_load_validated(GamingKnowledgeBase *knowledge, const char *path,
    const SteamInfo *steam, const GeForceNowInfo *gfn, char *error, size_t error_size)
{
    if (gaming_knowledge_load(knowledge, path, steam, gfn, error, error_size) != 0) return -1;
    if (!valid_loaded_base(knowledge)) {
        *knowledge = (GamingKnowledgeBase){0};
        set_error(error, error_size, "Gaming knowledge database failed schema validation.");
        return -1;
    }
    return 0;
}

int gaming_knowledge_validate_file(const char *path, GamingKnowledgeBase *knowledge,
    char *error, size_t error_size)
{
    return gaming_knowledge_load_validated(knowledge, path, NULL, NULL, error, error_size);
}

static int data_directory(char *path, size_t path_size, char *error, size_t error_size)
{
    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    int written;

    if (xdg_data_home != NULL && xdg_data_home[0] == '/') {
        written = snprintf(path, path_size, "%s/linux-doctor", xdg_data_home);
    } else if (home != NULL && home[0] == '/') {
        written = snprintf(path, path_size, "%s/.local/share/linux-doctor", home);
    } else {
        set_error(error, error_size, "Absolute XDG_DATA_HOME or HOME is unavailable.");
        return -1;
    }
    if (written < 0 || (size_t)written >= path_size) {
        set_error(error, error_size, "Gaming knowledge data path is too long.");
        return -1;
    }
    return 0;
}

static int ensure_directory_component(const char *path, bool final, char *error, size_t error_size)
{
    struct stat metadata;

    if (mkdir(path, 0700) == 0) return 0;
    if (errno != EEXIST || (final ? lstat(path, &metadata) : stat(path, &metadata)) != 0 ||
        !S_ISDIR(metadata.st_mode) || (final && S_ISLNK(metadata.st_mode)) ||
        (final && metadata.st_uid != geteuid())) {
        set_error(error, error_size, "Cannot use gaming knowledge data directory safely.");
        return -1;
    }
    return 0;
}

static int ensure_directories(char *path, char *error, size_t error_size)
{
    char *cursor;

    for (cursor = path + 1U; *cursor != '\0'; cursor++) {
        if (*cursor != '/') continue;
        *cursor = '\0';
        if (ensure_directory_component(path, false, error, error_size) != 0) {
            *cursor = '/';
            return -1;
        }
        *cursor = '/';
    }
    return ensure_directory_component(path, true, error, error_size);
}

int gaming_knowledge_resolve_path(char *path, size_t path_size, bool *user_database,
    char *error, size_t error_size)
{
    GamingKnowledgeBase bundled;
    GamingKnowledgeBase user;
    char directory[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char user_path[GAMING_KNOWLEDGE_PATH_CAPACITY];
    const char bundled_path[] = "data/gaming-knowledge.tsv";
    bool bundled_valid;
    bool user_valid = false;
    int written;

    if (path == NULL || path_size == 0U || user_database == NULL) {
        set_error(error, error_size, "Gaming knowledge path destination is missing.");
        return -1;
    }
    *user_database = false;
    if (data_directory(directory, sizeof(directory), NULL, 0U) == 0) {
        written = snprintf(user_path, sizeof(user_path), "%s/gaming-knowledge.tsv", directory);
        user_valid = written >= 0 && (size_t)written < sizeof(user_path) &&
            gaming_knowledge_load_validated(&user, user_path, NULL, NULL, NULL, 0U) == 0;
    }
    bundled_valid = gaming_knowledge_load_validated(&bundled, bundled_path,
        NULL, NULL, NULL, 0U) == 0;
    if (user_valid && (!bundled_valid || strcmp(user.reviewed_on, bundled.reviewed_on) >= 0)) {
        written = snprintf(path, path_size, "%s", user_path);
        if (written >= 0 && (size_t)written < path_size) {
            *user_database = true;
            return 0;
        }
    }
    if (bundled_valid) {
        written = snprintf(path, path_size, "%s", bundled_path);
        if (written >= 0 && (size_t)written < path_size) return 0;
    }
    if (user_valid) {
        written = snprintf(path, path_size, "%s", user_path);
        if (written >= 0 && (size_t)written < path_size) {
            *user_database = true;
            return 0;
        }
    }
    {
        set_error(error, error_size, "No valid gaming knowledge database is available.");
        return -1;
    }
}

int gaming_knowledge_install(const char *candidate_path, char *installed_path,
    size_t installed_path_size, char *error, size_t error_size)
{
    GamingKnowledgeBase candidate;
    GamingKnowledgeBase current;
    char current_path[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char directory[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char destination[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char temporary[GAMING_KNOWLEDGE_PATH_CAPACITY];
    unsigned char buffer[8192];
    bool current_is_user = false;
    bool temporary_owned = false;
    uintmax_t copied = 0U;
    int source = -1;
    int output = -1;
    int directory_fd = -1;
    int written;
    int result = -1;
    struct stat metadata;

    if (candidate_path == NULL || data_directory(directory, sizeof(directory), error, error_size) != 0 ||
        ensure_directories(directory, error, error_size) != 0) return -1;
    written = snprintf(destination, sizeof(destination), "%s/gaming-knowledge.tsv", directory);
    if (written < 0 || (size_t)written >= sizeof(destination)) {
        set_error(error, error_size, "Gaming knowledge destination path is too long.");
        return -1;
    }
    if (installed_path != NULL && (installed_path_size == 0U || installed_path_size <= strlen(destination))) {
        set_error(error, error_size, "Installed gaming knowledge path is too long.");
        return -1;
    }
    written = snprintf(temporary, sizeof(temporary), "%s/.gaming-knowledge.tsv.tmp.XXXXXX", directory);
    if (written < 0 || (size_t)written >= sizeof(temporary)) {
        set_error(error, error_size, "Gaming knowledge temporary path is too long.");
        return -1;
    }
    source = open(candidate_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (source < 0 || fstat(source, &metadata) != 0 || !S_ISREG(metadata.st_mode) ||
        metadata.st_size <= 0 || (uintmax_t)metadata.st_size > GAMING_KNOWLEDGE_MAX_FILE_BYTES) {
        set_error(error, error_size, "Gaming knowledge candidate is missing, unsafe or too large.");
        goto cleanup;
    }
    output = mkstemp(temporary);
    if (output < 0) {
        set_error(error, error_size, "Cannot create a private gaming knowledge update.");
        goto cleanup;
    }
    temporary_owned = true;
    if (fcntl(output, F_SETFD, FD_CLOEXEC) != 0) {
        set_error(error, error_size, "Cannot protect the private gaming knowledge update.");
        goto cleanup;
    }
    for (;;) {
        ssize_t count = read(source, buffer, sizeof(buffer));
        size_t offset = 0U;

        if (count < 0) {
            if (errno == EINTR) continue;
            set_error(error, error_size, "Cannot read gaming knowledge update completely.");
            goto cleanup;
        }
        if (count == 0) break;
        if (copied + (uintmax_t)count > GAMING_KNOWLEDGE_MAX_FILE_BYTES) {
            set_error(error, error_size, "Gaming knowledge update exceeded its size limit.");
            goto cleanup;
        }
        copied += (uintmax_t)count;
        while (offset < (size_t)count) {
            ssize_t count_written = write(output, buffer + offset, (size_t)count - offset);

            if (count_written < 0) {
                if (errno == EINTR) continue;
                set_error(error, error_size, "Cannot write gaming knowledge update.");
                goto cleanup;
            }
            if (count_written == 0) {
                set_error(error, error_size, "Gaming knowledge update write made no progress.");
                goto cleanup;
            }
            offset += (size_t)count_written;
        }
    }
    if (close(source) != 0) {
        source = -1;
        set_error(error, error_size, "Cannot close gaming knowledge update source.");
        goto cleanup;
    }
    source = -1;
    if (fsync(output) != 0) {
        set_error(error, error_size, "Cannot finalize gaming knowledge update.");
        goto cleanup;
    }
    if (close(output) != 0) {
        output = -1;
        set_error(error, error_size, "Cannot finalize gaming knowledge update.");
        goto cleanup;
    }
    output = -1;
    if (gaming_knowledge_load_validated(&candidate, temporary, NULL, NULL, error, error_size) != 0) goto cleanup;
    if (gaming_knowledge_resolve_path(current_path, sizeof(current_path), &current_is_user,
        NULL, 0U) == 0 && gaming_knowledge_load_validated(&current, current_path,
        NULL, NULL, NULL, 0U) == 0 && strcmp(candidate.reviewed_on, current.reviewed_on) < 0) {
        set_error(error, error_size, "Gaming knowledge downgrade was refused.");
        goto cleanup;
    }
    if (rename(temporary, destination) != 0) {
        set_error(error, error_size, "Cannot install gaming knowledge update atomically.");
        goto cleanup;
    }
    temporary_owned = false;
    directory_fd = open(directory, O_RDONLY | O_CLOEXEC);
    if (directory_fd < 0 || fsync(directory_fd) != 0) {
        set_error(error, error_size, "Gaming knowledge update is visible but could not be made durable.");
        goto cleanup;
    }
    if (close(directory_fd) != 0) {
        directory_fd = -1;
        set_error(error, error_size, "Gaming knowledge update is visible but could not be made durable.");
        goto cleanup;
    }
    directory_fd = -1;
    if (installed_path != NULL) (void)memcpy(installed_path, destination, strlen(destination) + 1U);
    result = 0;
cleanup:
    if (source >= 0) (void)close(source);
    if (output >= 0) (void)close(output);
    if (directory_fd >= 0) (void)close(directory_fd);
    if (temporary_owned) (void)unlink(temporary);
    return result;
}
