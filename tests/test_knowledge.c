#define _POSIX_C_SOURCE 200809L

#include "knowledge.h"

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void make_path(char *destination, size_t capacity, const char *directory,
    const char *suffix)
{
    int written = snprintf(destination, capacity, "%s/%s", directory, suffix);

    assert(written >= 0);
    assert((size_t)written < capacity);
}

static void copy_file(const char *source_path, const char *destination_path)
{
    unsigned char buffer[4096];
    FILE *source = fopen(source_path, "rb");
    FILE *destination;

    assert(source != NULL);
    destination = fopen(destination_path, "wb");
    assert(destination != NULL);
    for (;;) {
        size_t count = fread(buffer, 1U, sizeof(buffer), source);

        if (count > 0U) assert(fwrite(buffer, 1U, count, destination) == count);
        if (count < sizeof(buffer)) {
            assert(!ferror(source));
            break;
        }
    }
    assert(fclose(source) == 0);
    assert(fclose(destination) == 0);
}

static void write_valid_base(const char *path, const char *version, const char *reviewed_on)
{
    FILE *stream = fopen(path, "w");

    assert(stream != NULL);
    assert(fprintf(stream,
        "# linux-doctor-knowledge-schema\t1\n"
        "# linux-doctor-knowledge-version\t%s\n"
        "# reviewed-on\t%s\n"
        "game\t4242\tinfo\tFixture title\tFixture summary\tFixture guidance\t"
        "https://example.com/game\t%s\n",
        version, reviewed_on, reviewed_on) > 0);
    assert(fclose(stream) == 0);
}

static void write_oversized_file(const char *path)
{
    int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    assert(descriptor >= 0);
    assert(ftruncate(descriptor, (off_t)GAMING_KNOWLEDGE_MAX_FILE_BYTES + 1) == 0);
    assert(close(descriptor) == 0);
}

static void assert_no_update_temporary(const char *directory)
{
    const char prefix[] = ".gaming-knowledge.tsv.tmp.";
    DIR *stream = opendir(directory);
    struct dirent *entry;

    assert(stream != NULL);
    while ((entry = readdir(stream)) != NULL) {
        assert(strncmp(entry->d_name, prefix, sizeof(prefix) - 1U) != 0);
    }
    assert(closedir(stream) == 0);
}

int main(void)
{
    SteamInfo steam = {.game_count = 1U, .controller_detected = true, .controller_count = 1U,
        .controllers = {{.name = "Steam Controller", .kind = "steam"}},
        .games = {{.appid = "4242", .name = "Fixture Game"}}};
    GeForceNowInfo gfn = {0};
    GamingKnowledgeBase knowledge;
    char error[128];
    char temporary[] = "/tmp/linux-doctor-knowledge-XXXXXX";
    char original_directory[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char bundled_absolute[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char installed[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char resolved[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char user_directory[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char user_database_path[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char oversized_path[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char symlink_path[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char replacement_path[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char downgrade_path[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char relative_work[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char relative_home[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char relative_installed[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char relative_expected[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char relative_linux_doctor[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char relative_share[GAMING_KNOWLEDGE_PATH_CAPACITY];
    char relative_local[GAMING_KNOWLEDGE_PATH_CAPACITY];
    struct stat first_install;
    struct stat second_install;
    struct stat third_install;
    bool user_database = false;

    assert(getcwd(original_directory, sizeof(original_directory)) != NULL);
    make_path(bundled_absolute, sizeof(bundled_absolute), original_directory,
        "data/gaming-knowledge.tsv");
    assert(gaming_knowledge_load(NULL, "tests/fixtures/gaming-knowledge.tsv",
        &steam, &gfn, error, sizeof(error)) == -1);
    assert(gaming_knowledge_load(&knowledge, "tests/fixtures/missing.tsv",
        &steam, &gfn, error, sizeof(error)) == -1);
    assert(!knowledge.available);
    assert(gaming_knowledge_load(&knowledge, "tests/fixtures/gaming-knowledge.tsv",
        &steam, &gfn, error, sizeof(error)) == 0);
    assert(knowledge.available);
    assert(knowledge.schema_version == GAMING_KNOWLEDGE_SCHEMA_VERSION);
    assert(strcmp(knowledge.version, "fixture-1") == 0);
    assert(strcmp(knowledge.reviewed_on, "2026-07-14") == 0);
    assert(knowledge.entry_count == 3U);
    assert(knowledge.relevant_count == 2U);
    /* The existing fixture has one otherwise-valid row with a ninth column. */
    assert(knowledge.invalid_count == 1U);
    assert(knowledge.entries[0].relevant);
    assert(strcmp(knowledge.entries[0].target, "4242") == 0);
    assert(!knowledge.entries[1].relevant);
    assert(knowledge.entries[2].relevant);
    steam.games[0].is_tool = true;
    assert(gaming_knowledge_load(&knowledge, "tests/fixtures/gaming-knowledge.tsv",
        &steam, &gfn, error, sizeof(error)) == 0);
    assert(knowledge.relevant_count == 1U);
    assert(!knowledge.entries[0].relevant);
    assert(knowledge.entries[2].relevant);
    assert(gaming_knowledge_validate_file("tests/fixtures/gaming-knowledge.tsv",
        &knowledge, error, sizeof(error)) == -1);
    assert(gaming_knowledge_validate_file("data/gaming-knowledge.tsv",
        &knowledge, error, sizeof(error)) == 0);
    assert(knowledge.schema_version == GAMING_KNOWLEDGE_SCHEMA_VERSION);
    assert(mkdtemp(temporary) != NULL);

    /* A relative XDG path is ignored in favour of the absolute HOME fallback. */
    make_path(relative_work, sizeof(relative_work), temporary, "relative-work");
    make_path(relative_home, sizeof(relative_home), temporary, "relative-home");
    assert(mkdir(relative_work, 0700) == 0);
    assert(mkdir(relative_home, 0700) == 0);
    assert(chdir(relative_work) == 0);
    assert(setenv("XDG_DATA_HOME", "relative-xdg", 1) == 0);
    assert(setenv("HOME", relative_home, 1) == 0);
    assert(gaming_knowledge_install(bundled_absolute, relative_installed,
        sizeof(relative_installed), error, sizeof(error)) == 0);
    make_path(relative_local, sizeof(relative_local), relative_home, ".local");
    make_path(relative_share, sizeof(relative_share), relative_local, "share");
    make_path(relative_linux_doctor, sizeof(relative_linux_doctor), relative_share,
        "linux-doctor");
    make_path(relative_expected, sizeof(relative_expected), relative_linux_doctor,
        "gaming-knowledge.tsv");
    assert(strcmp(relative_installed, relative_expected) == 0);
    assert(access("relative-xdg", F_OK) != 0);
    assert(gaming_knowledge_resolve_path(resolved, sizeof(resolved), &user_database,
        error, sizeof(error)) == 0);
    assert(user_database);
    assert(strcmp(resolved, relative_expected) == 0);
    assert(chdir(original_directory) == 0);
    assert(unlink(relative_installed) == 0);
    assert(rmdir(relative_linux_doctor) == 0);
    assert(rmdir(relative_share) == 0);
    assert(rmdir(relative_local) == 0);
    assert(rmdir(relative_home) == 0);
    assert(rmdir(relative_work) == 0);

    assert(setenv("XDG_DATA_HOME", temporary, 1) == 0);
    assert(setenv("HOME", temporary, 1) == 0);
    make_path(user_directory, sizeof(user_directory), temporary, "linux-doctor");
    assert(mkdir(user_directory, 0700) == 0);
    make_path(user_database_path, sizeof(user_database_path), user_directory,
        "gaming-knowledge.tsv");

    /* A malformed user copy never replaces the valid bundled database. */
    copy_file("tests/fixtures/gaming-knowledge.tsv", user_database_path);
    assert(gaming_knowledge_resolve_path(resolved, sizeof(resolved), &user_database,
        error, sizeof(error)) == 0);
    assert(!user_database);
    assert(strcmp(resolved, "data/gaming-knowledge.tsv") == 0);
    assert(unlink(user_database_path) == 0);

    /* A valid but older user copy also yields to the newer bundled database. */
    write_valid_base(user_database_path, "fixture-stale", "2026-07-13");
    assert(gaming_knowledge_resolve_path(resolved, sizeof(resolved), &user_database,
        error, sizeof(error)) == 0);
    assert(!user_database);
    assert(strcmp(resolved, "data/gaming-knowledge.tsv") == 0);
    assert(unlink(user_database_path) == 0);

    make_path(oversized_path, sizeof(oversized_path), temporary, "oversized.tsv");
    write_oversized_file(oversized_path);
    assert(gaming_knowledge_validate_file(oversized_path, &knowledge,
        error, sizeof(error)) == -1);
    assert(gaming_knowledge_install(oversized_path, installed, sizeof(installed),
        error, sizeof(error)) == -1);
    assert_no_update_temporary(user_directory);

    make_path(symlink_path, sizeof(symlink_path), temporary, "knowledge-link.tsv");
    assert(symlink(bundled_absolute, symlink_path) == 0);
    assert(gaming_knowledge_validate_file(symlink_path, &knowledge,
        error, sizeof(error)) == -1);
    assert(gaming_knowledge_install(symlink_path, installed, sizeof(installed),
        error, sizeof(error)) == -1);
    assert_no_update_temporary(user_directory);

    /* Replacing and reinstalling a valid copy leaves one complete file and no temp. */
    assert(gaming_knowledge_install("data/gaming-knowledge.tsv", installed,
        sizeof(installed), error, sizeof(error)) == 0);
    assert(stat(installed, &first_install) == 0);
    make_path(replacement_path, sizeof(replacement_path), temporary, "replacement.tsv");
    write_valid_base(replacement_path, "fixture-new", "2026-07-14");
    assert(gaming_knowledge_install(replacement_path, installed,
        sizeof(installed), error, sizeof(error)) == 0);
    assert(stat(installed, &second_install) == 0);
    assert(first_install.st_ino != second_install.st_ino);
    assert(gaming_knowledge_validate_file(installed, &knowledge,
        error, sizeof(error)) == 0);
    assert(strcmp(knowledge.version, "fixture-new") == 0);
    assert(gaming_knowledge_install(replacement_path, installed,
        sizeof(installed), error, sizeof(error)) == 0);
    assert(stat(installed, &third_install) == 0);
    assert(second_install.st_ino != third_install.st_ino);
    assert_no_update_temporary(user_directory);

    make_path(downgrade_path, sizeof(downgrade_path), temporary, "downgrade.tsv");
    write_valid_base(downgrade_path, "fixture-old", "2026-07-13");
    assert(gaming_knowledge_install(downgrade_path, installed,
        sizeof(installed), error, sizeof(error)) == -1);
    assert(strstr(error, "downgrade") != NULL);
    assert(gaming_knowledge_validate_file(installed, &knowledge,
        error, sizeof(error)) == 0);
    assert(strcmp(knowledge.version, "fixture-new") == 0);
    assert(strcmp(knowledge.reviewed_on, "2026-07-14") == 0);
    assert_no_update_temporary(user_directory);

    assert(gaming_knowledge_resolve_path(resolved, sizeof(resolved), &user_database,
        error, sizeof(error)) == 0);
    assert(user_database);
    assert(strcmp(resolved, installed) == 0);
    assert(unlink(installed) == 0);
    assert(unlink(oversized_path) == 0);
    assert(unlink(symlink_path) == 0);
    assert(unlink(replacement_path) == 0);
    assert(unlink(downgrade_path) == 0);
    assert(rmdir(user_directory) == 0);
    assert(rmdir(temporary) == 0);
    assert(unsetenv("XDG_DATA_HOME") == 0);
    assert(unsetenv("HOME") == 0);
    return 0;
}
