#define _POSIX_C_SOURCE 200809L

#include "future_lab_json.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static char *read_stream(FILE *stream, char *buffer, size_t capacity)
{
    size_t length;

    assert(stream != NULL);
    assert(buffer != NULL);
    assert(capacity > 0U);
    assert(fseek(stream, 0L, SEEK_SET) == 0);
    length = fread(buffer, 1U, capacity - 1U, stream);
    assert(!ferror(stream));
    assert(length < capacity);
    buffer[length] = '\0';
    return buffer;
}

static FutureLabSnapshot complete_snapshot(void)
{
    FutureLabSnapshot snapshot = {
        .cpu = {
            .state = FUTURE_LAB_STATE_AVAILABLE,
            .logical_cpu_count = 8U,
            .user_ticks = 100U,
            .nice_ticks = 2U,
            .system_ticks = 30U,
            .idle_ticks = 800U,
            .iowait_ticks = 10U,
            .irq_ticks = 3U,
            .softirq_ticks = 4U,
            .steal_ticks = 1U,
            .busy_ticks = 140U,
            .total_ticks = 950U
        },
        .load = {
            .state = FUTURE_LAB_STATE_AVAILABLE,
            .one_minute = 0.25,
            .five_minutes = 0.50,
            .fifteen_minutes = 0.75,
            .running_tasks = 2U,
            .total_tasks = 400U
        },
        .memory = {
            .state = FUTURE_LAB_STATE_AVAILABLE,
            .total_kib = 32000000U,
            .available_kib = 12000000U,
            .used_kib = 20000000U,
            .buffers_kib = 100000U,
            .cached_kib = 5000000U,
            .swap_total_kib = 8000000U,
            .swap_free_kib = 7000000U
        },
        .network = {
            .state = FUTURE_LAB_STATE_AVAILABLE,
            .observed_interface_count = 1U,
            .interface_count = 1U,
            .received_bytes = 12345U,
            .transmitted_bytes = 67890U,
            .interfaces = {{
                .received_bytes = 12345U,
                .received_packets = 120U,
                .received_errors = 1U,
                .received_dropped = 2U,
                .transmitted_bytes = 67890U,
                .transmitted_packets = 340U,
                .transmitted_errors = 3U,
                .transmitted_dropped = 4U
            }}
        },
        .disks = {
            .state = FUTURE_LAB_STATE_AVAILABLE,
            .observed_device_count = 1U,
            .device_count = 1U,
            .devices = {{
                .major = 259U,
                .minor = 0U,
                .reads_completed = 500U,
                .sectors_read = 600U,
                .writes_completed = 700U,
                .sectors_written = 800U
            }}
        },
        .gpu = {
            .state = FUTURE_LAB_STATE_AVAILABLE,
            .index = 0U,
            .name = "NVIDIA GeForce RTX 3090",
            .utilization_percent = 31.0,
            .memory_used_mib = 2466.0,
            .memory_total_mib = 24576.0,
            .temperature_celsius = 44.0,
            .power_watts = 100.06,
            .nvtop_available = true
        }
    };

    (void)snprintf(snapshot.network.interfaces[0].name,
        sizeof(snapshot.network.interfaces[0].name), "%s", "enp\"5\\s0");
    (void)snprintf(snapshot.disks.devices[0].name,
        sizeof(snapshot.disks.devices[0].name), "%s", "nvme\"0\\n1");
    return snapshot;
}

static void test_document(void)
{
    FutureLabSnapshot snapshot = complete_snapshot();
    FutureLabSampleMetadata metadata = {
        .generated_at = "2026-07-14T12:34:56.789Z",
        .boot_id = "01234567-89ab-cdef-0123-456789abcdef",
        .unix_milliseconds = 1784032496789U,
        .monotonic_milliseconds = 987654U
    };
    char buffer[32768];
    FILE *stream = tmpfile();

    assert(stream != NULL);
    assert(future_lab_json_write_document(stream, &snapshot, &metadata) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(fclose(stream) == 0);
    assert(strstr(buffer, "\"schema\":{\"name\":\"linux-doctor.future-lab.live\",\"version\":1}") != NULL);
    assert(strstr(buffer, "\"generated_at\":\"2026-07-14T12:34:56.789Z\"") != NULL);
    assert(strstr(buffer, "\"boot_id\":\"01234567-89ab-cdef-0123-456789abcdef\"") != NULL);
    assert(strstr(buffer, "\"monotonic_milliseconds\":987654") != NULL);
    assert(strstr(buffer, "\"complete\":true") != NULL);
    assert(strstr(buffer, "\"total\":950") != NULL);
    assert(strstr(buffer, "\"received_packets\":120") != NULL);
    assert(strstr(buffer, "\"sectors_written\":800") != NULL);
    assert(strstr(buffer, "\"major\":259,\"minor\":0") != NULL);
    assert(strstr(buffer, "\"gpu\":{\"state\":\"available\"") != NULL);
    assert(strstr(buffer, "\"utilization_percent\":31.0") != NULL);
    assert(strstr(buffer, "\"nvtop_available\":true") != NULL);
    assert(strstr(buffer, "\"member_rule\":\"require_identical_counter_member_sets\"") != NULL);
    assert(strstr(buffer, "\"name\":\"enp\\\"5\\\\s0\"") != NULL);
    assert(strstr(buffer, "\"name\":\"nvme\\\"0\\\\n1\"") != NULL);
    assert(buffer[strlen(buffer) - 1U] == '\n');
}

static void test_missing_sources(void)
{
    FutureLabSnapshot snapshot = {0};
    char buffer[8192];
    FILE *stream = tmpfile();

    assert(stream != NULL);
    assert(future_lab_json_write_snapshot(stream, &snapshot) == 0);
    read_stream(stream, buffer, sizeof(buffer));
    assert(fclose(stream) == 0);
    assert(strstr(buffer, "\"complete\":false") != NULL);
    assert(strstr(buffer, "\"cpu\":{\"state\":\"unknown\",\"source\":\"/proc/stat\"}") != NULL);
    assert(strstr(buffer, "\"network\":{\"state\":\"unknown\",\"source\":\"/proc/net/dev\"}") != NULL);
    assert(strstr(buffer, "\"counters\"") == NULL);
}

static void test_metadata_clock(void)
{
    FutureLabSampleMetadata metadata;
    size_t length;

    assert(future_lab_sample_metadata_now(NULL) == -1);
    assert(future_lab_sample_metadata_now(&metadata) == 0);
    length = strlen(metadata.generated_at);
    assert(length == 24U);
    assert(metadata.generated_at[23] == 'Z');
    assert(strlen(metadata.boot_id) == FUTURE_LAB_BOOT_ID_CAPACITY - 1U);
    assert(metadata.unix_milliseconds > 0U);
    assert(metadata.monotonic_milliseconds > 0U);
}

int main(void)
{
    FutureLabSnapshot snapshot = {0};
    FutureLabSampleMetadata metadata = {
        .generated_at = "2026-07-14T00:00:00.000Z",
        .boot_id = "01234567-89ab-cdef-0123-456789abcdef"
    };

    assert(future_lab_json_write_snapshot(NULL, &snapshot) == -1);
    assert(future_lab_json_write_snapshot(stdout, NULL) == -1);
    assert(future_lab_json_write_document(NULL, &snapshot, &metadata) == -1);
    assert(future_lab_json_write_document(stdout, NULL, &metadata) == -1);
    assert(future_lab_json_write_document(stdout, &snapshot, NULL) == -1);
    metadata.generated_at[0] = '\0';
    assert(future_lab_json_write_document(stdout, &snapshot, &metadata) == -1);
    test_document();
    test_missing_sources();
    test_metadata_clock();
    return 0;
}
