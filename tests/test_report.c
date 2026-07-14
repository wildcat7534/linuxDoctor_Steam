#include "report.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void read_stream(FILE *stream, char *buffer, size_t capacity)
{
    size_t length;

    assert(stream != NULL);
    assert(buffer != NULL);
    assert(capacity > 1U);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    length = fread(buffer, 1U, capacity - 1U, stream);
    assert(length > 0U);
    assert(!ferror(stream));
    assert(fgetc(stream) == EOF);
    buffer[length] = '\0';
}

int main(void)
{
    StorageInfo storage = {
        .available = true,
        .total_bytes = 1000U,
        .available_bytes = 500U,
        .used_percent = 50U,
        .mount_count = 1U,
        .steamapps_available = true,
        .steamapps_bytes = 123456789U,
        .mounts = {{.path = "/mnt/games", .available_bytes = 987654321U, .used_percent = 44U}}
    };
    UpdatesInfo updates = {
        .cache_available = true,
        .cache_age_days = 2U,
        .inventory_available = true,
        .selection_available = true,
        .hold_information_available = true,
        .metadata_available = true,
        .package_count = 2U,
        .ready_count = 1U,
        .phased_count = 1U,
        .security_count = 1U,
        .metadata_count = 2U,
        .packages = {
            {
                .name = "software-properties-common",
                .installed_version = "0.120",
                .candidate_version = "0.120.1",
                .architecture = "all",
                .repository = "Ubuntu:26.04/resolute-updates",
                .source_package = "software-properties",
                .section = "admin",
                .origin = "Ubuntu",
                .description = "manage repositories \"safely\" <script>",
                .purpose = "Gestion des dépôts logiciels et des sources utilisées par APT.",
                .state = APT_UPDATE_PHASED,
                .metadata_available = true,
                .phased_percentage_available = true,
                .phased_percentage = 10U
            },
            {
                .name = "openssl",
                .installed_version = "3.0.0-1",
                .candidate_version = "3.0.0-2",
                .architecture = "amd64",
                .repository = "Ubuntu:26.04/resolute-security",
                .source_package = "openssl",
                .section = "utils",
                .origin = "Ubuntu",
                .description = "Secure Sockets Layer toolkit",
                .purpose = "Utilitaire général utilisé par le système ou par d'autres applications.",
                .state = APT_UPDATE_READY,
                .security_origin = true,
                .metadata_available = true
            }
        }
    };
    AppsInfo apps = {.package_database_available = true, .gnome_tweaks_installed = false};
    SteamInfo steam = {.ubuntu = true, .ubuntu_2604 = true, .i386_available = true,
        .steam_devices_installed = true, .controller_detected = true,
        .controller_name = "Steam Controller", .controller_count = 2U,
        .controllers = {{.name = "Steam Controller", .kind = "steam"},
            {.name = "Xbox Wireless Controller", .kind = "xbox"}},
        .library_count = 1U, .game_count = 1U,
        .libraries = {{.path = "/mnt/games/Steam", .filesystem = "ntfs", .writable = true,
            .game_count = 1U, .game_bytes = 123U}},
        .games = {{.appid = "123", .name = "Test Game",
            .icon_path = "tests/fixtures/steam-home/.local/share/Steam/appcache/librarycache/4242/0123456789abcdef0123456789abcdef01234567.jpg",
            .size_bytes = 123U, .directory_present = true}}};
    VolumeInventory volumes = {.available = true, .count = 1U,
        .items = {{.path = "/dev/sdb2", .uuid = "test-uuid", .filesystem = "ntfs",
            .mountpoint = "/mnt/games", .size_bytes = 1000U, .available_bytes = 500U,
            .used_percent = 50U, .mounted = true}}};
    MigrationPlan migration = {.available = true, .destination_path = "/mnt/games",
        .target_free_bytes = 50U, .selected_bytes = 123U, .game_count = 1U, .game_indexes = {0U}};
    GeForceNowInfo gfn = {.installed = true, .official_flatpak = true, .ubuntu_supported = true,
        .wayland_session = true, .controller_available = true};
    GraphicsInfo graphics = {.device_inventory_available = true, .device_count = 1U,
        .session_available = true, .session_type = "wayland", .wayland_session = true,
        .vulkan_loader_available = true, .vulkan_icd_count = 2U,
        .opengl_loader_available = true,
        .devices = {{.card = "card0", .vendor = "NVIDIA", .vendor_id = "10DE",
            .device_id = "2204", .driver = "nvidia", .boot_vga = true}}};
    GamingKnowledgeBase knowledge = {.available = true, .entry_count = 1U, .relevant_count = 1U,
        .entries = {{.kind = "game", .target = "123", .severity = "warning",
            .title = "Known fixture issue", .summary = "Fixture summary",
            .guidance = "Fixture guidance", .source_url = "https://example.com/123",
            .updated_on = "2026-07-14", .relevant = true}}};
    HistoryComparison history = {.enabled = false};
    FILE *stream = tmpfile();
    char buffer[65536];
    GraphicsInfo headless_graphics = graphics;
    GraphicsInfo incomplete_graphics = graphics;
    GraphicsInfo headless_incomplete_graphics;

    headless_graphics.session_available = false;
    (void)snprintf(headless_graphics.session_type, sizeof(headless_graphics.session_type), "%s", "tty");
    headless_graphics.wayland_session = false;
    assert(strcmp(report_health_severity(&storage, &headless_graphics), "unknown") == 0);
    assert(report_health_score(&storage, &headless_graphics) == 96);
    assert(!report_health_complete(&storage, &headless_graphics));
    storage.used_percent = 95U;
    assert(strcmp(report_health_severity(&storage, &headless_graphics), "problem") == 0);
    assert(report_health_score(&storage, &headless_graphics) == 45);
    assert(!report_health_complete(&storage, &headless_graphics));
    storage.used_percent = 50U;
    incomplete_graphics.vulkan_loader_available = false;
    headless_incomplete_graphics = incomplete_graphics;
    headless_incomplete_graphics.session_available = false;
    headless_incomplete_graphics.wayland_session = false;
    assert(strcmp(report_health_severity(&storage, &incomplete_graphics), "warning") == 0);
    assert(report_health_score(&storage, &incomplete_graphics) == 65);
    assert(report_health_complete(&storage, &incomplete_graphics));
    assert(strcmp(report_health_severity(&storage, &headless_incomplete_graphics), "warning") == 0);
    assert(!report_health_complete(&storage, &headless_incomplete_graphics));
    assert(strcmp(report_health_severity(&storage, &graphics), "info") == 0);
    assert(report_health_score(&storage, &graphics) == 95);
    assert(report_health_complete(&storage, &graphics));

    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &apps, &steam, &volumes, &migration, &gfn, &graphics, &knowledge, &history) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(strstr(buffer, "\"storage\"") != NULL);
    assert(strstr(buffer, "\"generated_at\"") != NULL);
    assert(strstr(buffer, "\"scope\":[\"storage\",\"graphics\"],\"complete\":true") != NULL);
    assert(strstr(buffer, "\"storage_inventory\"") != NULL);
    assert(strstr(buffer, "test-uuid") != NULL);
    assert(strstr(buffer, "storage.steamapps.size") != NULL);
    assert(strstr(buffer, "storage.other_mounts.free_space") != NULL);
    assert(strstr(buffer, "/mnt/games") != NULL);
    assert(strstr(buffer, "123456789") != NULL);
    assert(strstr(buffer, "\"gaming\"") != NULL);
    assert(strstr(buffer, "steam.controller.rules") != NULL);
    assert(strstr(buffer, "steam.controller.detected") != NULL);
    assert(strstr(buffer, "steam.ubuntu.runtime") != NULL);
    assert(strstr(buffer, "\"steam_inventory\"") != NULL);
    assert(strstr(buffer, "\"controller_detected\":true") != NULL);
    assert(strstr(buffer, "\"controller_count\":2") != NULL);
    assert(strstr(buffer, "Xbox Wireless Controller") != NULL);
    assert(strstr(buffer, "\"gfn_inventory\":{\"installed\":true") != NULL);
    assert(strstr(buffer, "Test Game") != NULL);
    assert(strstr(buffer, "\"icon_data_uri\":\"data:image/jpeg;base64,") != NULL);
    assert(strstr(buffer, "\"gaming_knowledge\"") != NULL);
    assert(strstr(buffer, "Known fixture issue") != NULL);
    assert(strstr(buffer, "\"steam_migration_plan\"") != NULL);
    assert(strstr(buffer, "gaming.geforce_now.availability") != NULL);
    assert(strstr(buffer, "\"updates\"") != NULL);
    assert(strstr(buffer, "\"updates_inventory\"") != NULL);
    assert(strstr(buffer, "\"candidates\":2") != NULL);
    assert(strstr(buffer, "\"name\":\"software-properties-common\",\"architecture\":\"all\"") != NULL);
    assert(strstr(buffer, "\"security_origin\":false,\"held\":false,\"metadata_available\":true,\"state\":\"phased\"") != NULL);
    assert(strstr(buffer, "\"phased_percentage\":10") != NULL);
    assert(strstr(buffer, "resolute-security") != NULL);
    assert(strstr(buffer, "\"security_origin\":true,\"held\":false,\"metadata_available\":true,\"state\":\"ready\"") != NULL);
    assert(strstr(buffer, "manage repositories \\\"safely\\\" <script>") != NULL);
    assert(strstr(buffer, "updates.apt.candidates") != NULL);
    assert(strstr(buffer, "./scripts/refresh-updates.sh") != NULL);
    assert(strstr(buffer, "\"apps\"") != NULL);
    assert(strstr(buffer, "desktop.gnome_tweaks") != NULL);
    assert(strstr(buffer, "\"graphics_inventory\"") != NULL);
    assert(strstr(buffer, "\"graphics\"") != NULL);
    assert(strstr(buffer, "graphics.gpu.driver") != NULL);
    assert(strstr(buffer, "graphics.vulkan.loader") != NULL);
    assert(strstr(buffer, "graphics.opengl.loader") != NULL);
    assert(strstr(buffer, "graphics.session") != NULL);
    assert(strstr(buffer, "\"id\":\"graphics\",\"name\":\"Graphismes\",\"icon\":\"⚡\",\"status\":\"info\"") != NULL);
    assert(strstr(buffer, "95 % : le pilote noyau, Vulkan, OpenGL") != NULL);
    assert(strstr(buffer, "95 % : les prérequis Steam mesurables") != NULL);
    assert(strstr(buffer, "Fichiers Vulkan détectés") != NULL);
    assert(strstr(buffer, "\"severity\":\"ok\"") != NULL);
    assert(strstr(buffer, "steam-devices") != NULL);
    assert(fclose(stream) == 0);

    updates = (UpdatesInfo){.cache_available = true, .cache_age_days = 8U,
        .inventory_available = true, .selection_available = true,
        .hold_information_available = true, .metadata_available = true};
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &apps, &steam, &volumes, &migration, &gfn, &graphics, &knowledge, &history) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(strstr(buffer, "updates.apt.cache_age\",\"severity\":\"warning") != NULL);
    assert(fclose(stream) == 0);

    updates = (UpdatesInfo){.cache_available = false};
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &apps, &steam, &volumes, &migration, &gfn, &graphics, &knowledge, &history) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(strstr(buffer, "updates.apt.cache_age\",\"severity\":\"unknown") != NULL);
    assert(fclose(stream) == 0);

    updates = (UpdatesInfo){.cache_available = true, .cache_age_days = 1U,
        .inventory_available = true, .package_count = 1U, .unknown_count = 1U,
        .packages = {{.name = "example", .state = APT_UPDATE_UNKNOWN}}};
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &apps, &steam, &volumes, &migration, &gfn, &graphics, &knowledge, &history) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(strstr(buffer, "État actuel des candidats non confirmé") != NULL);
    assert(strstr(buffer, "\"unknown\":1") != NULL);
    assert(fclose(stream) == 0);

    updates.selection_available = true;
    updates.hold_information_available = true;
    updates.packages[0].state = APT_UPDATE_DEFERRED;
    updates.unknown_count = 0U;
    updates.deferred_count = 1U;
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &apps, &steam, &volumes, &migration, &gfn, &graphics, &knowledge, &history) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(strstr(buffer, "Descriptions APT incomplètes") != NULL);
    assert(fclose(stream) == 0);

    updates.truncated = true;
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &apps, &steam, &volumes, &migration, &gfn, &graphics, &knowledge, &history) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(strstr(buffer, "Liste des mises à jour partielle") != NULL);
    assert(fclose(stream) == 0);

    graphics.vulkan_loader_available = false;
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &apps, &steam, &volumes, &migration, &gfn, &graphics, &knowledge, &history) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(strstr(buffer, "graphics.vulkan.loader\",\"severity\":\"warning") != NULL);
    assert(strstr(buffer, "\"score\": 65") != NULL);
    assert(strstr(buffer, "\"label\": \"warning\"") != NULL);
    assert(fclose(stream) == 0);

    graphics = (GraphicsInfo){0};
    stream = tmpfile();
    assert(stream != NULL);
    assert(report_write(stream, &storage, &updates, &apps, &steam, &volumes, &migration, &gfn, &graphics, &knowledge, &history) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(strstr(buffer, "graphics.gpu.driver\",\"severity\":\"unknown") != NULL);
    assert(strstr(buffer, "\"score\": 96") != NULL);
    assert(strstr(buffer, "\"complete\":false") != NULL);
    assert(strstr(buffer, "\"label\": \"unknown\"") != NULL);
    assert(fclose(stream) == 0);
    return 0;
}
