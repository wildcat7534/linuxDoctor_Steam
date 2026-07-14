#define _POSIX_C_SOURCE 200809L

#include "updates.h"

#include "process.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define APT_OUTPUT_SIZE (1024U * 1024U)
#define APT_METADATA_OUTPUT_SIZE (4U * 1024U * 1024U)
#define APT_TIMEOUT_MS 20000U
#define UPDATE_LINE_SIZE 4096U

#define APT_STAMP_PATH "/var/lib/apt/periodic/update-success-stamp"
#define APT_LISTS_PATH "/var/lib/apt/lists"

typedef struct ParsedUpdate {
    char name[UPDATE_NAME_SIZE];
    char installed_version[UPDATE_VERSION_SIZE];
    char candidate_version[UPDATE_VERSION_SIZE];
    char architecture[UPDATE_ARCHITECTURE_SIZE];
    char repository[UPDATE_REPOSITORY_SIZE];
} ParsedUpdate;

typedef struct AptMetadata {
    char package[UPDATE_NAME_SIZE];
    char version[UPDATE_VERSION_SIZE];
    char architecture[UPDATE_ARCHITECTURE_SIZE];
    char source[UPDATE_SOURCE_SIZE];
    char section[UPDATE_SECTION_SIZE];
    char origin[UPDATE_ORIGIN_SIZE];
    char description[UPDATE_DESCRIPTION_SIZE];
    bool phased_percentage_available;
    unsigned int phased_percentage;
    int description_priority;
    bool reading_description;
    bool truncated;
} AptMetadata;

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0U) (void)snprintf(error, error_size, "%s", message);
}

static bool copy_span(char *destination, size_t destination_size, const char *start, const char *end)
{
    size_t length;
    bool truncated;

    if (destination_size == 0U) return true;
    while (start < end && isspace((unsigned char)*start)) start++;
    while (end > start && isspace((unsigned char)end[-1])) end--;
    length = (size_t)(end - start);
    truncated = length >= destination_size;
    if (truncated) length = destination_size - 1U;
    memcpy(destination, start, length);
    destination[length] = '\0';
    return truncated;
}

static bool copy_string(char *destination, size_t destination_size, const char *source)
{
    int written;

    if (destination_size == 0U) return true;
    written = snprintf(destination, destination_size, "%s", source);
    return written < 0 || (size_t)written >= destination_size;
}

static bool next_line(const char **cursor, char *line, size_t line_size, bool *truncated)
{
    const char *start;
    const char *end;
    size_t length;

    if (cursor == NULL || *cursor == NULL || **cursor == '\0') return false;
    start = *cursor;
    end = strchr(start, '\n');
    if (end == NULL) end = start + strlen(start);
    length = (size_t)(end - start);
    if (length > 0U && start[length - 1U] == '\r') length--;
    if (length >= line_size) {
        length = line_size - 1U;
        if (truncated != NULL) *truncated = true;
    }
    memcpy(line, start, length);
    line[length] = '\0';
    *cursor = *end == '\n' ? end + 1 : end;
    return true;
}

static bool contains_security_suite(const char *repository)
{
    const char *match = repository;

    while ((match = strstr(match, "-security")) != NULL) {
        unsigned char following = (unsigned char)match[9];
        if (following == '\0' || isspace(following) || following == ',' || following == '/' || following == ')') return true;
        match += 9;
    }
    return false;
}

static int parse_inst_line(const char *line, ParsedUpdate *parsed)
{
    const char *name_start;
    const char *name_end;
    const char *installed_start;
    const char *installed_end;
    const char *candidate_start;
    const char *candidate_end;
    const char *parenthesis_end;
    const char *architecture_start = NULL;
    const char *architecture_end = NULL;
    const char *cursor;

    if (strncmp(line, "Inst ", 5U) != 0) return 0;
    name_start = line + 5U;
    while (*name_start != '\0' && isspace((unsigned char)*name_start)) name_start++;
    name_end = name_start;
    while (*name_end != '\0' && !isspace((unsigned char)*name_end)) name_end++;
    cursor = name_end;
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) cursor++;
    if (*cursor != '[') return 0;
    installed_start = cursor + 1;
    installed_end = strchr(installed_start, ']');
    if (installed_end == NULL) return -1;
    cursor = installed_end + 1;
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) cursor++;
    if (*cursor != '(') return -1;
    candidate_start = cursor + 1;
    while (*candidate_start != '\0' && isspace((unsigned char)*candidate_start)) candidate_start++;
    candidate_end = candidate_start;
    while (*candidate_end != '\0' && !isspace((unsigned char)*candidate_end) && *candidate_end != ')') candidate_end++;
    parenthesis_end = strrchr(candidate_end, ')');
    if (parenthesis_end == NULL || candidate_end == candidate_start) return -1;
    for (cursor = candidate_end; cursor < parenthesis_end; cursor++) {
        if (*cursor == '[') architecture_start = cursor + 1;
        else if (*cursor == ']' && architecture_start != NULL) architecture_end = cursor;
    }
    if (architecture_start == NULL || architecture_end == NULL || architecture_end <= architecture_start) return -1;
    *parsed = (ParsedUpdate){0};
    if (copy_span(parsed->name, sizeof(parsed->name), name_start, name_end) ||
        copy_span(parsed->installed_version, sizeof(parsed->installed_version), installed_start, installed_end) ||
        copy_span(parsed->candidate_version, sizeof(parsed->candidate_version), candidate_start, candidate_end) ||
        copy_span(parsed->architecture, sizeof(parsed->architecture), architecture_start, architecture_end) ||
        copy_span(parsed->repository, sizeof(parsed->repository), candidate_end, architecture_start - 1)) return -1;
    return parsed->name[0] != '\0' && parsed->installed_version[0] != '\0' &&
        parsed->candidate_version[0] != '\0' ? 1 : -1;
}

static bool known_non_inst_line(const char *line)
{
    return line[0] == '\0' || strncmp(line, "Conf ", 5U) == 0 ||
        strncmp(line, "Remv ", 5U) == 0 || strncmp(line, "Purg ", 5U) == 0;
}

static bool same_package(const AptUpdatePackage *package, const ParsedUpdate *parsed)
{
    return strcmp(package->name, parsed->name) == 0 &&
        strcmp(package->architecture, parsed->architecture) == 0 &&
        strcmp(package->candidate_version, parsed->candidate_version) == 0;
}

static const char *section_name(const char *section)
{
    const char *slash = strrchr(section, '/');
    return slash == NULL ? section : slash + 1;
}

static void set_package_purpose(AptUpdatePackage *package)
{
    const char *section = section_name(package->section);
    const char *purpose;

    if (strstr(package->source_package, "software-properties") != NULL ||
        strstr(package->name, "software-properties") != NULL) {
        purpose = "Gestion des dépôts logiciels et des sources utilisées par APT.";
    } else if (strncmp(package->name, "linux-image", 11U) == 0 ||
        strncmp(package->name, "linux-modules", 13U) == 0 || strncmp(package->name, "linux-headers", 13U) == 0) {
        purpose = "Composant du noyau Linux, qui relie le système au matériel et aux pilotes.";
    } else if (strstr(package->name, "nvidia") != NULL || strstr(package->name, "mesa") != NULL ||
        strstr(package->name, "vulkan") != NULL) {
        purpose = "Composant de la pile graphique ou d'un pilote utilisé pour l'affichage et le rendu.";
    } else if (strncmp(package->name, "python3-", 8U) == 0) {
        purpose = "Module Python 3 utilisé par une application ou un outil du système.";
    } else if (strcmp(section, "libs") == 0) {
        purpose = "Bibliothèque partagée utilisée en arrière-plan par d'autres logiciels.";
    } else if (strcmp(section, "admin") == 0) {
        purpose = "Outil ou composant d'administration du système.";
    } else if (strcmp(section, "net") == 0) {
        purpose = "Composant lié au réseau ou aux communications.";
    } else if (strcmp(section, "devel") == 0) {
        purpose = "Composant destiné au développement ou à la compilation de logiciels.";
    } else if (strcmp(section, "games") == 0) {
        purpose = "Composant lié aux jeux ou à leur environnement d'exécution.";
    } else if (strcmp(section, "x11") == 0 || strcmp(section, "gnome") == 0 ||
        strcmp(section, "kde") == 0 || strcmp(section, "video") == 0) {
        purpose = "Composant de l'environnement graphique ou de l'affichage.";
    } else if (strcmp(section, "utils") == 0) {
        purpose = "Utilitaire général utilisé par le système ou par d'autres applications.";
    } else {
        purpose = "Composant logiciel installé ; la description APT ci-dessous précise son rôle.";
    }
    (void)snprintf(package->purpose, sizeof(package->purpose), "%s", purpose);
}

static void recalculate_counts(UpdatesInfo *updates)
{
    updates->ready_count = 0U;
    updates->phased_count = 0U;
    updates->deferred_count = 0U;
    updates->unknown_count = 0U;
    updates->security_count = 0U;
    updates->held_count = 0U;
    updates->metadata_count = 0U;
    for (size_t index = 0U; index < updates->package_count; index++) {
        AptUpdatePackage *package = &updates->packages[index];
        if (package->state == APT_UPDATE_READY) updates->ready_count++;
        else if (package->state == APT_UPDATE_PHASED) updates->phased_count++;
        else if (package->state == APT_UPDATE_DEFERRED) updates->deferred_count++;
        else updates->unknown_count++;
        if (package->security_origin) updates->security_count++;
        if (package->held) updates->held_count++;
        if (package->metadata_available) updates->metadata_count++;
    }
}

const char *updates_state_name(AptUpdateState state)
{
    if (state == APT_UPDATE_READY) return "ready";
    if (state == APT_UPDATE_PHASED) return "phased";
    if (state == APT_UPDATE_DEFERRED) return "deferred";
    return "unknown";
}

int updates_parse_candidates(const char *output, UpdatesInfo *updates)
{
    const char *cursor = output;
    char line[UPDATE_LINE_SIZE];
    bool line_truncated = false;

    if (output == NULL || updates == NULL) return -1;
    updates->inventory_available = false;
    updates->selection_available = false;
    updates->hold_information_available = false;
    updates->metadata_available = false;
    updates->package_count = 0U;
    updates->ready_count = 0U;
    updates->phased_count = 0U;
    updates->deferred_count = 0U;
    updates->unknown_count = 0U;
    updates->security_count = 0U;
    updates->truncated = false;
    while (next_line(&cursor, line, sizeof(line), &line_truncated)) {
        ParsedUpdate parsed;
        int parsed_result;
        bool duplicate = false;

        if (strncmp(line, "Inst ", 5U) != 0) {
            if (!known_non_inst_line(line)) updates->truncated = true;
            continue;
        }
        parsed_result = parse_inst_line(line, &parsed);
        if (parsed_result < 0) {
            updates->truncated = true;
            continue;
        }
        if (parsed_result == 0) continue;
        for (size_t index = 0U; index < updates->package_count; index++) {
            if (same_package(&updates->packages[index], &parsed)) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;
        if (updates->package_count >= UPDATES_MAX_PACKAGES) {
            updates->truncated = true;
            continue;
        }
        AptUpdatePackage *package = &updates->packages[updates->package_count++];
        *package = (AptUpdatePackage){0};
        (void)snprintf(package->name, sizeof(package->name), "%s", parsed.name);
        (void)snprintf(package->installed_version, sizeof(package->installed_version), "%s", parsed.installed_version);
        (void)snprintf(package->candidate_version, sizeof(package->candidate_version), "%s", parsed.candidate_version);
        (void)snprintf(package->architecture, sizeof(package->architecture), "%s", parsed.architecture);
        (void)snprintf(package->repository, sizeof(package->repository), "%s", parsed.repository);
        package->security_origin = contains_security_suite(parsed.repository);
        package->state = APT_UPDATE_UNKNOWN;
        set_package_purpose(package);
    }
    if (line_truncated) updates->truncated = true;
    updates->inventory_available = true;
    recalculate_counts(updates);
    return 0;
}

int updates_mark_held(const char *output, UpdatesInfo *updates)
{
    const char *cursor = output;
    char line[UPDATE_LINE_SIZE];
    bool line_truncated = false;

    if (output == NULL || updates == NULL) return -1;
    for (size_t index = 0U; index < updates->package_count; index++) updates->packages[index].held = false;
    while (next_line(&cursor, line, sizeof(line), &line_truncated)) {
        const char *start = line;
        const char *end = line + strlen(line);
        char package_name[UPDATE_NAME_SIZE];

        if (copy_span(package_name, sizeof(package_name), start, end)) {
            line_truncated = true;
            continue;
        }
        if (package_name[0] == '\0') continue;
        for (size_t index = 0U; index < updates->package_count; index++) {
            AptUpdatePackage *package = &updates->packages[index];
            if (strcmp(package->name, package_name) != 0) continue;
            package->held = true;
            package->state = APT_UPDATE_DEFERRED;
        }
    }
    if (line_truncated) updates->truncated = true;
    updates->hold_information_available = !line_truncated;
    recalculate_counts(updates);
    return 0;
}

int updates_mark_selected(const char *output, UpdatesInfo *updates)
{
    const char *cursor = output;
    char line[UPDATE_LINE_SIZE];
    bool line_truncated = false;
    bool parse_error = false;

    if (output == NULL || updates == NULL) return -1;
    for (size_t index = 0U; index < updates->package_count; index++) {
        AptUpdatePackage *package = &updates->packages[index];
        package->state = package->held ? APT_UPDATE_DEFERRED :
            package->phased_percentage_available && package->phased_percentage < 100U
                ? APT_UPDATE_PHASED : APT_UPDATE_DEFERRED;
    }
    while (next_line(&cursor, line, sizeof(line), &line_truncated)) {
        ParsedUpdate parsed;
        int parsed_result;

        if (strncmp(line, "Inst ", 5U) != 0) {
            if (!known_non_inst_line(line)) parse_error = true;
            continue;
        }
        parsed_result = parse_inst_line(line, &parsed);
        if (parsed_result < 0) {
            parse_error = true;
            continue;
        }
        if (parsed_result == 0) continue;
        for (size_t index = 0U; index < updates->package_count; index++) {
            if (same_package(&updates->packages[index], &parsed)) updates->packages[index].state = APT_UPDATE_READY;
        }
    }
    if (line_truncated || parse_error) {
        updates->truncated = true;
        for (size_t index = 0U; index < updates->package_count; index++) updates->packages[index].state = APT_UPDATE_UNKNOWN;
    }
    updates->selection_available = !line_truncated && !parse_error;
    recalculate_counts(updates);
    return 0;
}

static int description_priority(const char *key)
{
    if (strcmp(key, "Description-fr") == 0) return 3;
    if (strcmp(key, "Description-en") == 0 || strcmp(key, "Description") == 0) return 2;
    if (strncmp(key, "Description-", 12U) == 0) return 1;
    return 0;
}

static void append_description(AptMetadata *metadata, const char *text, bool paragraph)
{
    size_t used = strlen(metadata->description);
    const char *separator = used == 0U || metadata->description[used - 1U] == '\n'
        ? "" : paragraph ? "\n\n" : " ";
    size_t separator_length = strlen(separator);
    size_t text_length = strlen(text);
    size_t available;

    while (*text == ' ') text++;
    text_length = strlen(text);
    if (strcmp(text, ".") == 0) text_length = 0U;
    available = sizeof(metadata->description) - used - 1U;
    if (separator_length > available) {
        metadata->truncated = true;
        return;
    }
    memcpy(metadata->description + used, separator, separator_length);
    used += separator_length;
    available -= separator_length;
    if (text_length > available) {
        text_length = available;
        metadata->truncated = true;
    }
    memcpy(metadata->description + used, text, text_length);
    metadata->description[used + text_length] = '\0';
}

static void metadata_apply(UpdatesInfo *updates, AptMetadata *metadata)
{
    char base_name[UPDATE_NAME_SIZE];
    const char *colon;

    if (metadata->package[0] == '\0') return;
    for (size_t index = 0U; index < updates->package_count; index++) {
        AptUpdatePackage *package = &updates->packages[index];
        colon = strrchr(package->name, ':');
        if (colon == NULL) {
            (void)snprintf(base_name, sizeof(base_name), "%s", package->name);
        } else {
            if (copy_span(base_name, sizeof(base_name), package->name, colon)) {
                updates->truncated = true;
                continue;
            }
        }
        if (strcmp(base_name, metadata->package) != 0 || strcmp(package->candidate_version, metadata->version) != 0 ||
            (metadata->architecture[0] != '\0' && strcmp(package->architecture, metadata->architecture) != 0)) continue;
        if (!package->metadata_available) updates->metadata_count++;
        package->metadata_available = true;
        if (copy_string(package->source_package, sizeof(package->source_package),
                metadata->source[0] != '\0' ? metadata->source : metadata->package) ||
            copy_string(package->section, sizeof(package->section), metadata->section) ||
            copy_string(package->origin, sizeof(package->origin), metadata->origin) ||
            copy_string(package->description, sizeof(package->description), metadata->description) ||
            metadata->truncated) updates->truncated = true;
        package->phased_percentage_available = metadata->phased_percentage_available;
        package->phased_percentage = metadata->phased_percentage;
        if (package->state == APT_UPDATE_DEFERRED && updates->hold_information_available && !package->held &&
            package->phased_percentage_available &&
            package->phased_percentage < 100U) package->state = APT_UPDATE_PHASED;
        set_package_purpose(package);
    }
}

int updates_parse_metadata(const char *output, UpdatesInfo *updates)
{
    const char *cursor = output;
    char line[UPDATE_LINE_SIZE];
    bool line_truncated = false;
    AptMetadata metadata = {0};

    if (output == NULL || updates == NULL) return -1;
    updates->metadata_count = 0U;
    updates->metadata_available = false;
    for (size_t index = 0U; index < updates->package_count; index++) {
        AptUpdatePackage *package = &updates->packages[index];
        package->metadata_available = false;
        package->source_package[0] = '\0';
        package->section[0] = '\0';
        package->origin[0] = '\0';
        package->description[0] = '\0';
        package->phased_percentage_available = false;
        package->phased_percentage = 0U;
        set_package_purpose(package);
    }
    while (next_line(&cursor, line, sizeof(line), &line_truncated)) {
        char *colon;

        if (line[0] == '\0') {
            metadata_apply(updates, &metadata);
            metadata = (AptMetadata){0};
            continue;
        }
        if (line[0] == ' ' || line[0] == '\t') {
            if (metadata.reading_description) append_description(&metadata, line, strcmp(line + 1, ".") == 0);
            continue;
        }
        metadata.reading_description = false;
        colon = strchr(line, ':');
        if (colon == NULL) continue;
        *colon = '\0';
        const char *value = colon + 1;
        while (*value != '\0' && isspace((unsigned char)*value)) value++;
        if (strcmp(line, "Package") == 0) {
            if (copy_string(metadata.package, sizeof(metadata.package), value)) metadata.truncated = true;
        } else if (strcmp(line, "Version") == 0) {
            if (copy_string(metadata.version, sizeof(metadata.version), value)) metadata.truncated = true;
        } else if (strcmp(line, "Architecture") == 0) {
            if (copy_string(metadata.architecture, sizeof(metadata.architecture), value)) metadata.truncated = true;
        } else if (strcmp(line, "Source") == 0) {
            const char *end = value;
            while (*end != '\0' && !isspace((unsigned char)*end)) end++;
            if (copy_span(metadata.source, sizeof(metadata.source), value, end)) metadata.truncated = true;
        } else if (strcmp(line, "Section") == 0) {
            if (copy_string(metadata.section, sizeof(metadata.section), value)) metadata.truncated = true;
        } else if (strcmp(line, "Origin") == 0) {
            if (copy_string(metadata.origin, sizeof(metadata.origin), value)) metadata.truncated = true;
        } else if (strcmp(line, "Phased-Update-Percentage") == 0) {
            char *end = NULL;
            unsigned long percentage = strtoul(value, &end, 10);
            if (end != value && percentage <= 100UL) {
                metadata.phased_percentage_available = true;
                metadata.phased_percentage = (unsigned int)percentage;
            }
        } else {
            int priority = description_priority(line);
            if (priority >= metadata.description_priority && priority > 0) {
                metadata.description[0] = '\0';
                append_description(&metadata, value, false);
                metadata.description_priority = priority;
                metadata.reading_description = true;
            }
        }
    }
    metadata_apply(updates, &metadata);
    if (line_truncated) updates->truncated = true;
    updates->metadata_available = updates->metadata_count == updates->package_count;
    recalculate_counts(updates);
    return 0;
}

static bool apt_lists_have_index(const char *lists_path)
{
    DIR *directory = opendir(lists_path);
    struct dirent *entry;
    bool found = false;

    if (directory == NULL) return false;
    while ((entry = readdir(directory)) != NULL) {
        char path[4096];
        struct stat metadata;
        const char *name = entry->d_name;
        int written;
        bool index_name = strstr(name, "_InRelease") != NULL || strstr(name, "_Release") != NULL ||
            strstr(name, "_Packages") != NULL || strstr(name, "_Sources") != NULL;

        if (name[0] == '.' || !index_name) continue;
        written = snprintf(path, sizeof(path), "%s/%s", lists_path, name);
        if (written < 0 || (size_t)written >= sizeof(path)) continue;
        if (stat(path, &metadata) == 0 && S_ISREG(metadata.st_mode)) {
            found = true;
            break;
        }
    }
    (void)closedir(directory);
    return found;
}

int updates_collect_cache_age_from_paths(UpdatesInfo *updates, const char *stamp_path,
    const char *lists_path, time_t now)
{
    struct stat stamp_metadata;
    struct stat lists_metadata;
    time_t timestamp;
    time_t age;

    if (updates == NULL || stamp_path == NULL || lists_path == NULL || now == (time_t)-1) return -1;
    updates->cache_available = false;
    updates->cache_age_days = 0U;
    if (!apt_lists_have_index(lists_path) || stat(lists_path, &lists_metadata) != 0 ||
        !S_ISDIR(lists_metadata.st_mode)) return -1;
    timestamp = lists_metadata.st_mtime;
    if (stat(stamp_path, &stamp_metadata) == 0 && S_ISREG(stamp_metadata.st_mode)) {
        timestamp = stamp_metadata.st_mtime;
    }
    age = now > timestamp ? now - timestamp : 0;
    updates->cache_available = true;
    updates->cache_age_days = (unsigned int)(age / (24 * 60 * 60));
    return 0;
}

static bool command_succeeded(const ProcessResult *result)
{
    return result->exited && !result->timed_out && result->exit_code == 0;
}

static void collect_metadata(UpdatesInfo *updates, char *error, size_t error_size)
{
    char *output;
    char **arguments;
    char (*specifications)[UPDATE_NAME_SIZE + UPDATE_ARCHITECTURE_SIZE + UPDATE_VERSION_SIZE + 3U];
    ProcessResult result;
    size_t argument_count = updates->package_count + 4U;

    if (updates->package_count == 0U) {
        updates->metadata_available = true;
        return;
    }
    output = malloc(APT_METADATA_OUTPUT_SIZE);
    arguments = calloc(argument_count, sizeof(*arguments));
    specifications = calloc(updates->package_count, sizeof(*specifications));
    if (output == NULL || arguments == NULL || specifications == NULL) {
        free(output);
        free(arguments);
        free(specifications);
        return;
    }
    arguments[0] = (char *)"/usr/bin/apt-cache";
    arguments[1] = (char *)"show";
    arguments[2] = (char *)"--";
    for (size_t index = 0U; index < updates->package_count; index++) {
        AptUpdatePackage *package = &updates->packages[index];
        if (strrchr(package->name, ':') != NULL) {
            (void)snprintf(specifications[index], sizeof(specifications[index]), "%s=%s",
                package->name, package->candidate_version);
        } else {
            (void)snprintf(specifications[index], sizeof(specifications[index]), "%s:%s=%s",
                package->name, package->architecture, package->candidate_version);
        }
        arguments[index + 3U] = specifications[index];
    }
    arguments[updates->package_count + 3U] = NULL;
    if (process_capture("/usr/bin/apt-cache", arguments, output, APT_METADATA_OUTPUT_SIZE,
        APT_TIMEOUT_MS, &result, error, error_size) == 0 && command_succeeded(&result)) {
        (void)updates_parse_metadata(output, updates);
        if (result.truncated) updates->truncated = true;
    }
    free(specifications);
    free(arguments);
    free(output);
}

int updates_collect(UpdatesInfo *updates, char *error, size_t error_size)
{
    char *candidate_output;
    char *selected_output;
    char *hold_output;
    ProcessResult candidate_result;
    ProcessResult selected_result;
    ProcessResult hold_result;
    char *candidate_arguments[] = {
        (char *)"/usr/bin/apt-get", (char *)"--simulate", (char *)"--quiet=2", (char *)"--assume-no",
        (char *)"--ignore-hold",
        (char *)"-o", (char *)"Debug::NoLocking=1",
        (char *)"-o", (char *)"APT::Get::Show-User-Simulation-Note=0",
        (char *)"-o", (char *)"APT::Get::Always-Include-Phased-Updates=true",
        (char *)"dist-upgrade", NULL
    };
    char *selected_arguments[] = {
        (char *)"/usr/bin/apt-get", (char *)"--simulate", (char *)"--quiet=2", (char *)"--assume-no",
        (char *)"--with-new-pkgs",
        (char *)"-o", (char *)"Debug::NoLocking=1",
        (char *)"-o", (char *)"APT::Get::Show-User-Simulation-Note=0",
        (char *)"upgrade", NULL
    };
    char *hold_arguments[] = {(char *)"/usr/bin/apt-mark", (char *)"showhold", NULL};

    if (updates == NULL) {
        set_error(error, error_size, "Updates destination is missing.");
        return -1;
    }
    *updates = (UpdatesInfo){0};
    (void)updates_collect_cache_age_from_paths(updates, APT_STAMP_PATH, APT_LISTS_PATH, time(NULL));
    candidate_output = malloc(APT_OUTPUT_SIZE);
    selected_output = malloc(APT_OUTPUT_SIZE);
    hold_output = malloc(APT_OUTPUT_SIZE);
    if (candidate_output == NULL || selected_output == NULL || hold_output == NULL) {
        free(candidate_output);
        free(selected_output);
        free(hold_output);
        set_error(error, error_size, "Cannot allocate APT command output.");
        return 0;
    }
    if (process_capture("/usr/bin/apt-get", candidate_arguments, candidate_output, APT_OUTPUT_SIZE,
        APT_TIMEOUT_MS, &candidate_result, error, error_size) == 0 && command_succeeded(&candidate_result)) {
        (void)updates_parse_candidates(candidate_output, updates);
        if (candidate_result.truncated) updates->truncated = true;
    }
    if (updates->inventory_available && process_capture("/usr/bin/apt-get", selected_arguments,
        selected_output, APT_OUTPUT_SIZE, APT_TIMEOUT_MS, &selected_result, error, error_size) == 0 &&
        command_succeeded(&selected_result)) {
        (void)updates_mark_selected(selected_output, updates);
        if (selected_result.truncated) {
            updates->truncated = true;
            updates->selection_available = false;
            for (size_t index = 0U; index < updates->package_count; index++) {
                updates->packages[index].state = APT_UPDATE_UNKNOWN;
            }
            recalculate_counts(updates);
        }
    }
    if (updates->inventory_available && process_capture("/usr/bin/apt-mark", hold_arguments,
        hold_output, APT_OUTPUT_SIZE, APT_TIMEOUT_MS, &hold_result, error, error_size) == 0 &&
        command_succeeded(&hold_result)) {
        (void)updates_mark_held(hold_output, updates);
        if (hold_result.truncated) {
            updates->truncated = true;
            updates->hold_information_available = false;
        }
    }
    if (updates->inventory_available) collect_metadata(updates, error, error_size);
    free(candidate_output);
    free(selected_output);
    free(hold_output);
    return 0;
}
