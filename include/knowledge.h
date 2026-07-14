#ifndef LINUX_DOCTOR_KNOWLEDGE_H
#define LINUX_DOCTOR_KNOWLEDGE_H

#include <stdbool.h>
#include <stddef.h>

#include "gfn.h"
#include "steam.h"

#define GAMING_KNOWLEDGE_LIMIT 128U
#define GAMING_KNOWLEDGE_KIND_CAPACITY 16U
#define GAMING_KNOWLEDGE_TARGET_CAPACITY 64U
#define GAMING_KNOWLEDGE_TITLE_CAPACITY 160U
#define GAMING_KNOWLEDGE_SUMMARY_CAPACITY 320U
#define GAMING_KNOWLEDGE_GUIDANCE_CAPACITY 512U
#define GAMING_KNOWLEDGE_SOURCE_CAPACITY 320U
#define GAMING_KNOWLEDGE_DATE_CAPACITY 16U
#define GAMING_KNOWLEDGE_VERSION_CAPACITY 32U
#define GAMING_KNOWLEDGE_PATH_CAPACITY 1024U
#define GAMING_KNOWLEDGE_MAX_FILE_BYTES 524288U
#define GAMING_KNOWLEDGE_SCHEMA_VERSION 1U

typedef struct GamingKnowledgeEntry {
    char kind[GAMING_KNOWLEDGE_KIND_CAPACITY];
    char target[GAMING_KNOWLEDGE_TARGET_CAPACITY];
    char severity[16];
    char title[GAMING_KNOWLEDGE_TITLE_CAPACITY];
    char summary[GAMING_KNOWLEDGE_SUMMARY_CAPACITY];
    char guidance[GAMING_KNOWLEDGE_GUIDANCE_CAPACITY];
    char source_url[GAMING_KNOWLEDGE_SOURCE_CAPACITY];
    char updated_on[GAMING_KNOWLEDGE_DATE_CAPACITY];
    bool relevant;
} GamingKnowledgeEntry;

typedef struct GamingKnowledgeBase {
    bool available;
    bool truncated;
    bool user_database;
    unsigned int schema_version;
    size_t entry_count;
    size_t relevant_count;
    size_t invalid_count;
    char version[GAMING_KNOWLEDGE_VERSION_CAPACITY];
    char reviewed_on[GAMING_KNOWLEDGE_DATE_CAPACITY];
    GamingKnowledgeEntry entries[GAMING_KNOWLEDGE_LIMIT];
} GamingKnowledgeBase;

int gaming_knowledge_load(GamingKnowledgeBase *knowledge, const char *path,
    const SteamInfo *steam, const GeForceNowInfo *gfn, char *error, size_t error_size);
int gaming_knowledge_load_validated(GamingKnowledgeBase *knowledge, const char *path,
    const SteamInfo *steam, const GeForceNowInfo *gfn, char *error, size_t error_size);
int gaming_knowledge_validate_file(const char *path, GamingKnowledgeBase *knowledge,
    char *error, size_t error_size);
int gaming_knowledge_resolve_path(char *path, size_t path_size, bool *user_database,
    char *error, size_t error_size);
int gaming_knowledge_install(const char *candidate_path, char *installed_path,
    size_t installed_path_size, char *error, size_t error_size);

#endif
