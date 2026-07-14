#define _POSIX_C_SOURCE 200809L

#include "future_lab_json.h"

#include "json.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define PROC_BOOT_ID_PATH "/proc/sys/kernel/random/boot_id"

static int write_cpu(FILE *stream, const FutureLabCpuSnapshot *cpu)
{
    if (fputs("\"cpu\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(cpu->state)) != 0 ||
        fputs(",\"source\":\"/proc/stat\"", stream) == EOF) return -1;
    if (cpu->state == FUTURE_LAB_STATE_AVAILABLE &&
        (fprintf(stream, ",\"logical_cpu_count\":%zu,\"counters\":{"
            "\"cumulative\":true,\"scope\":\"since_boot\",\"unit\":\"scheduler_ticks\","
            "\"user\":%" PRIu64 ",\"nice\":%" PRIu64 ",\"system\":%" PRIu64
            ",\"idle\":%" PRIu64 ",\"iowait\":%" PRIu64 ",\"irq\":%" PRIu64
            ",\"softirq\":%" PRIu64 ",\"steal\":%" PRIu64 ",\"busy\":%" PRIu64
            ",\"total\":%" PRIu64 "}",
            cpu->logical_cpu_count, cpu->user_ticks, cpu->nice_ticks, cpu->system_ticks,
            cpu->idle_ticks, cpu->iowait_ticks, cpu->irq_ticks, cpu->softirq_ticks,
            cpu->steal_ticks, cpu->busy_ticks, cpu->total_ticks) < 0)) return -1;
    return fputc('}', stream) == EOF ? -1 : 0;
}

static int write_load(FILE *stream, const FutureLabLoadSnapshot *load)
{
    if (fputs("\"load\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(load->state)) != 0 ||
        fputs(",\"source\":\"/proc/loadavg\"", stream) == EOF) return -1;
    if (load->state == FUTURE_LAB_STATE_AVAILABLE &&
        fprintf(stream, ",\"kind\":\"kernel_run_queue_average\","
            "\"one_minute\":%.2f,\"five_minutes\":%.2f,\"fifteen_minutes\":%.2f,"
            "\"running_tasks\":%" PRIu64 ",\"total_tasks\":%" PRIu64,
            load->one_minute, load->five_minutes, load->fifteen_minutes,
            load->running_tasks, load->total_tasks) < 0) return -1;
    return fputc('}', stream) == EOF ? -1 : 0;
}

static int write_memory(FILE *stream, const FutureLabMemorySnapshot *memory)
{
    if (fputs("\"memory\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(memory->state)) != 0 ||
        fputs(",\"source\":\"/proc/meminfo\"", stream) == EOF) return -1;
    if (memory->state == FUTURE_LAB_STATE_AVAILABLE &&
        fprintf(stream, ",\"unit\":\"KiB\",\"total\":%" PRIu64
            ",\"available\":%" PRIu64 ",\"used\":%" PRIu64
            ",\"buffers\":%" PRIu64 ",\"cached\":%" PRIu64
            ",\"swap_total\":%" PRIu64 ",\"swap_free\":%" PRIu64,
            memory->total_kib, memory->available_kib, memory->used_kib,
            memory->buffers_kib, memory->cached_kib, memory->swap_total_kib,
            memory->swap_free_kib) < 0) return -1;
    return fputc('}', stream) == EOF ? -1 : 0;
}

static int write_network(FILE *stream, const FutureLabNetworkSnapshot *network)
{
    size_t index;

    if (fputs("\"network\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(network->state)) != 0 ||
        fputs(",\"source\":\"/proc/net/dev\"", stream) == EOF) return -1;
    if (network->state != FUTURE_LAB_STATE_AVAILABLE) return fputc('}', stream) == EOF ? -1 : 0;
    if (fputs(",\"truncated\":", stream) == EOF ||
        fputs(network->truncated ? "true" : "false", stream) == EOF ||
        fprintf(stream, ",\"observed_interface_count\":%zu,\"reported_interface_count\":%zu,"
            "\"counters\":{\"cumulative\":true,\"rates_calculated\":false,"
            "\"received_bytes\":%" PRIu64 ",\"transmitted_bytes\":%" PRIu64 "},"
            "\"interfaces\":[", network->observed_interface_count, network->interface_count,
            network->received_bytes, network->transmitted_bytes) < 0) return -1;
    for (index = 0U; index < network->interface_count; index++) {
        const FutureLabNetworkInterface *interface = &network->interfaces[index];

        if (index > 0U && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"name\":", stream) == EOF || json_write_string(stream, interface->name) != 0 ||
            fprintf(stream, ",\"counters\":{\"cumulative\":true,\"received_bytes\":%" PRIu64
                ",\"received_packets\":%" PRIu64 ",\"received_errors\":%" PRIu64
                ",\"received_dropped\":%" PRIu64 ",\"transmitted_bytes\":%" PRIu64
                ",\"transmitted_packets\":%" PRIu64 ",\"transmitted_errors\":%" PRIu64
                ",\"transmitted_dropped\":%" PRIu64 "}}",
                interface->received_bytes, interface->received_packets,
                interface->received_errors, interface->received_dropped,
                interface->transmitted_bytes, interface->transmitted_packets,
                interface->transmitted_errors, interface->transmitted_dropped) < 0) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int write_disks(FILE *stream, const FutureLabDiskSnapshot *disks)
{
    size_t index;

    if (fputs("\"disks\":{\"state\":", stream) == EOF ||
        json_write_string(stream, future_lab_state_name(disks->state)) != 0 ||
        fputs(",\"source\":\"/proc/diskstats\"", stream) == EOF) return -1;
    if (disks->state != FUTURE_LAB_STATE_AVAILABLE) return fputc('}', stream) == EOF ? -1 : 0;
    if (fputs(",\"truncated\":", stream) == EOF ||
        fputs(disks->truncated ? "true" : "false", stream) == EOF ||
        fprintf(stream, ",\"observed_device_count\":%zu,\"reported_device_count\":%zu,"
            "\"skipped_pseudo_device_count\":%zu,\"counters_are_cumulative\":true,"
            "\"rates_calculated\":false,\"sector_size_not_interpreted\":true,\"devices\":[",
            disks->observed_device_count, disks->device_count,
            disks->skipped_pseudo_device_count) < 0) return -1;
    for (index = 0U; index < disks->device_count; index++) {
        const FutureLabDiskDevice *device = &disks->devices[index];

        if (index > 0U && fputc(',', stream) == EOF) return -1;
        if (fputs("{\"name\":", stream) == EOF || json_write_string(stream, device->name) != 0 ||
            fprintf(stream, ",\"major\":%" PRIu64 ",\"minor\":%" PRIu64
                ",\"counters\":{\"cumulative\":true,\"reads_completed\":%" PRIu64
                ",\"sectors_read\":%" PRIu64 ",\"writes_completed\":%" PRIu64
                ",\"sectors_written\":%" PRIu64 "}}",
                device->major, device->minor, device->reads_completed,
                device->sectors_read, device->writes_completed,
                device->sectors_written) < 0) return -1;
    }
    return fputs("]}", stream) == EOF ? -1 : 0;
}

static int read_boot_id(char *destination, size_t capacity)
{
    FILE *stream;
    size_t length;

    if (destination == NULL || capacity < FUTURE_LAB_BOOT_ID_CAPACITY) return -1;
    stream = fopen(PROC_BOOT_ID_PATH, "r");
    if (stream == NULL) return -1;
    if (fgets(destination, (int)capacity, stream) == NULL) {
        (void)fclose(stream);
        return -1;
    }
    if (fclose(stream) != 0) return -1;
    length = strcspn(destination, "\r\n");
    destination[length] = '\0';
    if (length != FUTURE_LAB_BOOT_ID_CAPACITY - 1U) return -1;
    for (size_t index = 0U; index < length; index++) {
        const bool separator = index == 8U || index == 13U || index == 18U || index == 23U;

        if ((separator && destination[index] != '-') ||
            (!separator && !isxdigit((unsigned char)destination[index]))) return -1;
    }
    return 0;
}

int future_lab_sample_metadata_now(FutureLabSampleMetadata *metadata)
{
    struct timespec realtime;
    struct timespec monotonic;
    struct tm utc;
    char second_precision[24];
    time_t seconds;
    int length;

    if (metadata == NULL) return -1;
    *metadata = (FutureLabSampleMetadata){0};
    if (read_boot_id(metadata->boot_id, sizeof(metadata->boot_id)) != 0 ||
        clock_gettime(CLOCK_REALTIME, &realtime) != 0 ||
        clock_gettime(CLOCK_MONOTONIC, &monotonic) != 0 ||
        realtime.tv_sec < 0 || monotonic.tv_sec < 0 ||
        (uint64_t)realtime.tv_sec > UINT64_MAX / 1000U ||
        (uint64_t)monotonic.tv_sec > UINT64_MAX / 1000U) return -1;
    seconds = realtime.tv_sec;
    if (gmtime_r(&seconds, &utc) == NULL ||
        strftime(second_precision, sizeof(second_precision), "%Y-%m-%dT%H:%M:%S", &utc) == 0U) return -1;
    length = snprintf(metadata->generated_at, sizeof(metadata->generated_at), "%s.%03ldZ",
        second_precision, realtime.tv_nsec / 1000000L);
    if (length < 0 || (size_t)length >= sizeof(metadata->generated_at)) return -1;
    metadata->unix_milliseconds = (uint64_t)realtime.tv_sec * 1000U +
        (uint64_t)(realtime.tv_nsec / 1000000L);
    metadata->monotonic_milliseconds = (uint64_t)monotonic.tv_sec * 1000U +
        (uint64_t)(monotonic.tv_nsec / 1000000L);
    return 0;
}

int future_lab_json_write_snapshot(FILE *stream, const FutureLabSnapshot *snapshot)
{
    bool complete;

    if (stream == NULL || snapshot == NULL) return -1;
    complete = snapshot->cpu.state == FUTURE_LAB_STATE_AVAILABLE &&
        snapshot->load.state == FUTURE_LAB_STATE_AVAILABLE &&
        snapshot->memory.state == FUTURE_LAB_STATE_AVAILABLE &&
        snapshot->network.state == FUTURE_LAB_STATE_AVAILABLE &&
        snapshot->disks.state == FUTURE_LAB_STATE_AVAILABLE;

    if (fputs("{\"read_only\":true,\"snapshot_kind\":\"single_local_snapshot\","
        "\"rates_calculated\":false,\"complete\":", stream) == EOF ||
        fputs(complete ? "true," : "false,", stream) == EOF ||
        write_cpu(stream, &snapshot->cpu) != 0 || fputc(',', stream) == EOF ||
        write_load(stream, &snapshot->load) != 0 || fputc(',', stream) == EOF ||
        write_memory(stream, &snapshot->memory) != 0 || fputc(',', stream) == EOF ||
        write_network(stream, &snapshot->network) != 0 || fputc(',', stream) == EOF ||
        write_disks(stream, &snapshot->disks) != 0 || fputc('}', stream) == EOF) return -1;
    return 0;
}

int future_lab_json_write_document(FILE *stream, const FutureLabSnapshot *snapshot,
    const FutureLabSampleMetadata *metadata)
{
    if (stream == NULL || snapshot == NULL || metadata == NULL ||
        metadata->generated_at[0] == '\0' || metadata->boot_id[0] == '\0') return -1;
    if (fputs("{\"schema\":{\"name\":", stream) == EOF ||
        json_write_string(stream, FUTURE_LAB_LIVE_SCHEMA_NAME) != 0 ||
        fprintf(stream, ",\"version\":%u},\"sample\":{\"generated_at\":",
            FUTURE_LAB_LIVE_SCHEMA_VERSION) < 0 ||
        json_write_string(stream, metadata->generated_at) != 0 ||
        fputs(",\"boot_id\":", stream) == EOF ||
        json_write_string(stream, metadata->boot_id) != 0 ||
        fprintf(stream, ",\"unix_milliseconds\":%" PRIu64
            ",\"monotonic_milliseconds\":%" PRIu64 "},"
            "\"delta_contract\":{\"elapsed_time_field\":\"sample.monotonic_milliseconds\","
            "\"sample_identity_fields\":[\"schema.name\",\"schema.version\",\"sample.boot_id\"],"
            "\"counter_rule\":\"subtract_matching_cumulative_counters\","
            "\"member_rule\":\"require_identical_counter_member_sets\","
            "\"reset_rule\":\"discard_previous_sample_if_identity_time_or_counter_changes\"},"
            "\"future_lab\":", metadata->unix_milliseconds,
            metadata->monotonic_milliseconds) < 0 ||
        future_lab_json_write_snapshot(stream, snapshot) != 0 ||
        fputs("}\n", stream) == EOF) return -1;
    return 0;
}
