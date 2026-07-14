#define _POSIX_C_SOURCE 200809L

#include "updates.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static const char *const CANDIDATES =
    "Inst software-properties-common [0.120] (0.120.1 Ubuntu:26.04/resolute-updates [all]) []\n"
    "Inst python3-software-properties [0.120] (0.120.1 Ubuntu:26.04/resolute-updates [all])\n"
    "Inst openssl [3.0.0-1] (3.0.0-2 Ubuntu:26.04/resolute-updates Ubuntu:26.04/resolute-security [amd64])\n"
    "Inst new-dependency (1.0 Ubuntu:26.04/resolute [amd64])\n"
    "Conf software-properties-common (0.120.1 Ubuntu:26.04/resolute-updates [all])\n"
    "Remv obsolete-package [0.9]\n";

static const char *const SELECTED =
    "Inst openssl [3.0.0-1] (3.0.0-2 Ubuntu:26.04/resolute-updates Ubuntu:26.04/resolute-security [amd64])\n"
    "Conf openssl (3.0.0-2 Ubuntu:26.04/resolute-security [amd64])\n";

static const char *const METADATA =
    "Package: software-properties-common\n"
    "Source: software-properties (0.120.1)\n"
    "Section: admin\n"
    "Architecture: all\n"
    "Version: 0.120.1\n"
    "Phased-Update-Percentage: 10\n"
    "Description-en: manage the repositories that you install software from (common)\n"
    " This package contains the common files and the D-Bus backend.\n"
    " .\n"
    " It also provides shared repository helpers.\n"
    "\n"
    "Package: python3-software-properties\n"
    "Source: software-properties\n"
    "Section: python\n"
    "Architecture: all\n"
    "Version: 0.120.1\n"
    "Phased-Update-Percentage: 10\n"
    "Description-en: manage software repositories from Python\n"
    "\n"
    "Package: openssl\n"
    "Section: utils\n"
    "Architecture: amd64\n"
    "Version: 3.0.0-2\n"
    "Origin: Ubuntu\n"
    "Description-en: Secure Sockets Layer toolkit\n"
    " Command line utilities and cryptographic support.\n";

static void test_parsers(void)
{
    UpdatesInfo updates = {0};

    assert(updates_parse_candidates(NULL, &updates) == -1);
    assert(updates_parse_candidates(CANDIDATES, &updates) == 0);
    assert(updates.inventory_available);
    assert(!updates.truncated);
    assert(updates.package_count == 3U);
    assert(strcmp(updates.packages[0].name, "software-properties-common") == 0);
    assert(strcmp(updates.packages[0].installed_version, "0.120") == 0);
    assert(strcmp(updates.packages[0].candidate_version, "0.120.1") == 0);
    assert(strcmp(updates.packages[0].architecture, "all") == 0);
    assert(!updates.packages[0].security_origin);
    assert(updates.packages[2].security_origin);
    assert(updates.security_count == 1U);
    assert(updates.unknown_count == 3U);

    assert(updates_mark_selected(SELECTED, &updates) == 0);
    assert(updates.selection_available);
    assert(updates.ready_count == 1U);
    assert(updates.deferred_count == 2U);
    assert(updates.packages[2].state == APT_UPDATE_READY);
    assert(updates_mark_held("", &updates) == 0);
    assert(updates.hold_information_available);

    assert(updates_parse_metadata(METADATA, &updates) == 0);
    assert(updates.metadata_available);
    assert(updates.metadata_count == 3U);
    assert(updates.packages[0].state == APT_UPDATE_PHASED);
    assert(updates.packages[1].state == APT_UPDATE_PHASED);
    assert(updates.phased_count == 2U);
    assert(updates.deferred_count == 0U);
    assert(updates.packages[0].phased_percentage == 10U);
    assert(strcmp(updates.packages[0].source_package, "software-properties") == 0);
    assert(strstr(updates.packages[0].description, "D-Bus backend") != NULL);
    assert(strstr(updates.packages[0].description, "\n\nIt also provides") != NULL);
    assert(strstr(updates.packages[0].purpose, "dépôts logiciels") != NULL);
    assert(strcmp(updates_state_name(updates.packages[2].state), "ready") == 0);
    assert(strcmp(updates_state_name(APT_UPDATE_UNKNOWN), "unknown") == 0);
}

static void test_empty_and_invalid_outputs(void)
{
    UpdatesInfo updates = {0};

    assert(updates_parse_candidates("", &updates) == 0);
    assert(updates.inventory_available && updates.package_count == 0U && !updates.truncated);
    assert(updates_parse_candidates("unexpected output\n", &updates) == 0);
    assert(updates.inventory_available && updates.package_count == 0U && updates.truncated);
    assert(updates_parse_candidates("Inst malformed [1.0 (2.0 nowhere [amd64])\n", &updates) == 0);
    assert(updates.package_count == 0U && updates.truncated);
}

static void test_phased_and_held_states(void)
{
    UpdatesInfo updates = {0};
    const char *candidate =
        "Inst software-properties-common [0.120] (0.120.1 Ubuntu:26.04/resolute-updates [all])\n";
    const char *metadata =
        "Package: software-properties-common\n"
        "Source: software-properties\n"
        "Section: admin\n"
        "Architecture: all\n"
        "Version: 0.120.1\n"
        "Phased-Update-Percentage: 10\n"
        "Description-en: manage software repositories\n";

    assert(updates_parse_candidates(candidate, &updates) == 0);
    assert(updates_mark_selected("", &updates) == 0);
    assert(updates_mark_held("", &updates) == 0);
    assert(updates_parse_metadata(metadata, &updates) == 0);
    assert(updates.packages[0].state == APT_UPDATE_PHASED);
    assert(updates.phased_count == 1U && updates.held_count == 0U);

    assert(updates_parse_candidates(candidate, &updates) == 0);
    assert(updates_mark_selected("", &updates) == 0);
    assert(updates_mark_held("software-properties-common\n", &updates) == 0);
    assert(updates_parse_metadata(metadata, &updates) == 0);
    assert(updates.packages[0].held);
    assert(updates.packages[0].state == APT_UPDATE_DEFERRED);
    assert(updates.held_count == 1U && updates.phased_count == 0U && updates.deferred_count == 1U);
}

static void test_incomplete_states(void)
{
    UpdatesInfo updates = {0};
    const char *candidate = "Inst openssl [3.0.0-1] (3.0.0-2 Ubuntu:26.04/resolute-updates [amd64])\n";

    assert(updates_parse_candidates(candidate, &updates) == 0);
    assert(!updates.selection_available && updates.unknown_count == 1U);
    assert(updates_mark_selected("changed output format\n", &updates) == 0);
    assert(!updates.selection_available && updates.truncated && updates.unknown_count == 1U);

    assert(updates_parse_candidates(candidate, &updates) == 0);
    assert(updates_mark_selected(candidate, &updates) == 0);
    assert(updates_mark_held("", &updates) == 0);
    assert(updates_parse_metadata("", &updates) == 0);
    assert(!updates.metadata_available && updates.metadata_count == 0U);
    assert(!updates.packages[0].metadata_available);
    assert(updates.packages[0].state == APT_UPDATE_READY);
}

static void test_multiarch_epoch(void)
{
    UpdatesInfo updates = {0};
    const char *output =
        "Inst libc6:i386 [1:2.39-0ubuntu8] (1:2.39-0ubuntu9 Ubuntu:26.04/resolute-updates [i386])\n"
        "Inst libc6 [1:2.39-0ubuntu8] (1:2.39-0ubuntu9 Ubuntu:26.04/resolute-updates [amd64])\n";

    assert(updates_parse_candidates(output, &updates) == 0);
    assert(updates.package_count == 2U);
    assert(strcmp(updates.packages[0].name, "libc6:i386") == 0);
    assert(strcmp(updates.packages[0].candidate_version, "1:2.39-0ubuntu9") == 0);
    assert(strcmp(updates.packages[1].architecture, "amd64") == 0);
}

static void test_capacity_limit(void)
{
    UpdatesInfo updates = {0};
    size_t capacity = (UPDATES_MAX_PACKAGES + 4U) * 128U;
    char *output = calloc(capacity, 1U);
    size_t used = 0U;

    assert(output != NULL);
    for (size_t index = 0U; index < UPDATES_MAX_PACKAGES + 1U; index++) {
        int written = snprintf(output + used, capacity - used,
            "Inst package-%zu [1.0] (1.1 Ubuntu:26.04/resolute-updates [amd64])\n", index);
        assert(written > 0 && (size_t)written < capacity - used);
        used += (size_t)written;
    }
    assert(updates_parse_candidates(output, &updates) == 0);
    assert(updates.package_count == UPDATES_MAX_PACKAGES);
    assert(updates.truncated);
    free(output);
}

static void test_field_limit(void)
{
    UpdatesInfo updates = {0};
    char version[UPDATE_VERSION_SIZE + 32U];
    char output[UPDATE_VERSION_SIZE + 256U];

    memset(version, '1', sizeof(version) - 1U);
    version[sizeof(version) - 1U] = '\0';
    assert(snprintf(output, sizeof(output),
        "Inst package [1.0] (%s Ubuntu:26.04/resolute-updates [amd64])\n", version) > 0);
    assert(updates_parse_candidates(output, &updates) == 0);
    assert(updates.package_count == 0U && updates.truncated);
}

static void test_cache_age_requires_real_indexes(void)
{
    UpdatesInfo updates = {0};
    char root[] = "/tmp/linux-doctor-apt-cache-XXXXXX";
    char lists[512];
    char stamp[512];
    char lock_path[512];
    char index_path[512];
    FILE *stream;
    time_t now = time(NULL);
    struct timespec stamp_times[2];

    assert(mkdtemp(root) != NULL);
    assert(snprintf(lists, sizeof(lists), "%s/lists", root) > 0);
    assert(snprintf(stamp, sizeof(stamp), "%s/update-success-stamp", root) > 0);
    assert(snprintf(lock_path, sizeof(lock_path), "%s/lock", lists) > 0);
    assert(snprintf(index_path, sizeof(index_path), "%s/example_InRelease", lists) > 0);
    assert(mkdir(lists, 0700) == 0);
    assert(updates_collect_cache_age_from_paths(&updates, stamp, lists, now) == -1);
    stream = fopen(lock_path, "w");
    assert(stream != NULL && fclose(stream) == 0);
    assert(updates_collect_cache_age_from_paths(&updates, stamp, lists, now) == -1);
    stream = fopen(index_path, "w");
    assert(stream != NULL && fputs("index", stream) >= 0 && fclose(stream) == 0);
    assert(updates_collect_cache_age_from_paths(&updates, stamp, lists, now) == 0);
    assert(updates.cache_available && updates.cache_age_days == 0U);
    stream = fopen(stamp, "w");
    assert(stream != NULL && fclose(stream) == 0);
    stamp_times[0].tv_sec = now - 2 * 24 * 60 * 60;
    stamp_times[0].tv_nsec = 0;
    stamp_times[1] = stamp_times[0];
    assert(utimensat(AT_FDCWD, stamp, stamp_times, 0) == 0);
    assert(updates_collect_cache_age_from_paths(&updates, stamp, lists, now) == 0);
    assert(updates.cache_age_days == 2U);
    assert(unlink(stamp) == 0);
    assert(unlink(index_path) == 0);
    assert(unlink(lock_path) == 0);
    assert(rmdir(lists) == 0);
    assert(rmdir(root) == 0);
}

int main(void)
{
    UpdatesInfo updates;

    test_parsers();
    test_empty_and_invalid_outputs();
    test_phased_and_held_states();
    test_incomplete_states();
    test_multiarch_epoch();
    test_capacity_limit();
    test_field_limit();
    test_cache_age_requires_real_indexes();
    assert(updates_collect(NULL, NULL, 0U) == -1);
    assert(updates_collect(&updates, NULL, 0U) == 0);
    assert(!updates.inventory_available || updates.package_count <= UPDATES_MAX_PACKAGES);
    return 0;
}
