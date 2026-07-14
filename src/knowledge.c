#include "knowledge.h"

#include <stdio.h>
#include <string.h>

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

    while (count < capacity) {
        char *separator;

        fields[count++] = cursor;
        separator = strchr(cursor, '\t');
        if (separator == NULL) break;
        *separator = '\0';
        cursor = separator + 1;
    }
    return count;
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
    if (strcmp(entry->kind, "steam") == 0) return steam != NULL && steam->library_count > 0U;
    if (strcmp(entry->kind, "controller") == 0) return steam != NULL &&
        (steam->controller_count > 0U || !steam->steam_devices_installed);
    if (strcmp(entry->kind, "gfn") == 0) return gfn != NULL && gfn->installed;
    if (strcmp(entry->kind, "ubuntu") == 0) return steam != NULL && steam->ubuntu;
    return false;
}

static bool parse_entry(GamingKnowledgeEntry *entry, char *line)
{
    char *fields[8];
    size_t count = split_fields(line, fields, 8U);

    if (count != 8U || !valid_kind(fields[0]) || fields[1][0] == '\0' ||
        !valid_severity(fields[2]) || fields[3][0] == '\0' || fields[4][0] == '\0' ||
        fields[5][0] == '\0' || fields[6][0] == '\0' || fields[7][0] == '\0') return false;
    return copy_field(entry->kind, sizeof(entry->kind), fields[0]) &&
        copy_field(entry->target, sizeof(entry->target), fields[1]) &&
        copy_field(entry->severity, sizeof(entry->severity), fields[2]) &&
        copy_field(entry->title, sizeof(entry->title), fields[3]) &&
        copy_field(entry->summary, sizeof(entry->summary), fields[4]) &&
        copy_field(entry->guidance, sizeof(entry->guidance), fields[5]) &&
        copy_field(entry->source_url, sizeof(entry->source_url), fields[6]) &&
        copy_field(entry->updated_on, sizeof(entry->updated_on), fields[7]);
}

int gaming_knowledge_load(GamingKnowledgeBase *knowledge, const char *path,
    const SteamInfo *steam, const GeForceNowInfo *gfn, char *error, size_t error_size)
{
    FILE *stream;
    char line[2048];

    if (knowledge == NULL || path == NULL) {
        set_error(error, error_size, "Gaming knowledge destination or path is missing.");
        return -1;
    }
    *knowledge = (GamingKnowledgeBase){0};
    stream = fopen(path, "r");
    if (stream == NULL) {
        set_error(error, error_size, "Gaming knowledge database is unavailable.");
        return -1;
    }
    knowledge->available = true;
    while (fgets(line, sizeof(line), stream) != NULL) {
        GamingKnowledgeEntry parsed = {0};
        size_t length = strlen(line);

        if (length > 0U && line[length - 1U] == '\n') line[--length] = '\0';
        if (length > 0U && line[length - 1U] == '\r') line[--length] = '\0';
        if (length == 0U || line[0] == '#') continue;
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
