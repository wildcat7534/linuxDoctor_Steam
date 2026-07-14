#define _POSIX_C_SOURCE 200809L

#include "future_lab.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROC_STAT_PATH "/proc/stat"
#define PROC_LOADAVG_PATH "/proc/loadavg"
#define PROC_MEMINFO_PATH "/proc/meminfo"
#define PROC_NET_DEV_PATH "/proc/net/dev"
#define PROC_DISKSTATS_PATH "/proc/diskstats"

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0U) (void)snprintf(error, error_size, "%s", message);
}

static void skip_spaces(const char **cursor)
{
    while (cursor != NULL && *cursor != NULL && isspace((unsigned char)**cursor)) (*cursor)++;
}

static int parse_u64(const char **cursor, uint64_t *value)
{
    char *end;
    uintmax_t parsed;

    if (cursor == NULL || *cursor == NULL || value == NULL) return -1;
    skip_spaces(cursor);
    if (!isdigit((unsigned char)**cursor)) return -1;
    errno = 0;
    parsed = strtoumax(*cursor, &end, 10);
    if (end == *cursor || errno == ERANGE || parsed > UINT64_MAX) return -1;
    *cursor = end;
    *value = (uint64_t)parsed;
    return 0;
}

static int parse_decimal(const char **cursor, double *value)
{
    const char *position;
    double result = 0.0;
    double place = 0.1;
    bool has_digit = false;

    if (cursor == NULL || *cursor == NULL || value == NULL) return -1;
    skip_spaces(cursor);
    position = *cursor;
    while (isdigit((unsigned char)*position)) {
        result = result * 10.0 + (double)(*position - '0');
        if (!isfinite(result)) return -1;
        position++;
        has_digit = true;
    }
    if (*position == '.') {
        position++;
        while (isdigit((unsigned char)*position)) {
            result += (double)(*position - '0') * place;
            place *= 0.1;
            position++;
            has_digit = true;
        }
    }
    if (!has_digit || (!isspace((unsigned char)*position) && *position != '\0')) return -1;
    *cursor = position;
    *value = result;
    return 0;
}

static int add_u64(uint64_t *total, uint64_t value)
{
    if (total == NULL || UINT64_MAX - *total < value) return -1;
    *total += value;
    return 0;
}

static bool cpu_line_name(const char *line)
{
    const char *position;

    if (line == NULL || strncmp(line, "cpu", 3U) != 0 || !isdigit((unsigned char)line[3])) return false;
    position = line + 3U;
    while (isdigit((unsigned char)*position)) position++;
    return isspace((unsigned char)*position) != 0;
}

static int parse_cpu_values(const char *line, FutureLabCpuSnapshot *snapshot)
{
    const char *cursor = line + 3U;
    uint64_t values[8] = {0};
    size_t count = 0U;

    while (true) {
        uint64_t value;

        skip_spaces(&cursor);
        if (*cursor == '\0') break;
        if (parse_u64(&cursor, &value) != 0 ||
            (!isspace((unsigned char)*cursor) && *cursor != '\0')) return -1;
        if (count < sizeof(values) / sizeof(values[0])) values[count] = value;
        count++;
    }
    if (count < 4U) return -1;
    snapshot->user_ticks = values[0];
    snapshot->nice_ticks = values[1];
    snapshot->system_ticks = values[2];
    snapshot->idle_ticks = values[3];
    snapshot->iowait_ticks = values[4];
    snapshot->irq_ticks = values[5];
    snapshot->softirq_ticks = values[6];
    snapshot->steal_ticks = values[7];
    if (add_u64(&snapshot->busy_ticks, snapshot->user_ticks) != 0 ||
        add_u64(&snapshot->busy_ticks, snapshot->nice_ticks) != 0 ||
        add_u64(&snapshot->busy_ticks, snapshot->system_ticks) != 0 ||
        add_u64(&snapshot->busy_ticks, snapshot->irq_ticks) != 0 ||
        add_u64(&snapshot->busy_ticks, snapshot->softirq_ticks) != 0 ||
        add_u64(&snapshot->busy_ticks, snapshot->steal_ticks) != 0) return -1;
    snapshot->total_ticks = snapshot->busy_ticks;
    if (add_u64(&snapshot->total_ticks, snapshot->idle_ticks) != 0 ||
        add_u64(&snapshot->total_ticks, snapshot->iowait_ticks) != 0 ||
        snapshot->total_ticks == 0U) return -1;
    return 0;
}

int future_lab_parse_cpu_stat(FILE *stream, FutureLabCpuSnapshot *snapshot)
{
    FutureLabCpuSnapshot parsed = {0};
    char line[1024];
    bool aggregate_found = false;

    if (stream == NULL || snapshot == NULL) return -1;
    *snapshot = (FutureLabCpuSnapshot){0};
    while (fgets(line, sizeof(line), stream) != NULL) {
        if (!aggregate_found && strncmp(line, "cpu", 3U) == 0 && isspace((unsigned char)line[3])) {
            if (parse_cpu_values(line, &parsed) != 0) return -1;
            aggregate_found = true;
        } else if (cpu_line_name(line)) {
            if (parsed.logical_cpu_count == SIZE_MAX) return -1;
            parsed.logical_cpu_count++;
        }
    }
    if (ferror(stream) || !aggregate_found || parsed.logical_cpu_count == 0U) return -1;
    parsed.state = FUTURE_LAB_STATE_AVAILABLE;
    *snapshot = parsed;
    return 0;
}

int future_lab_parse_loadavg(FILE *stream, FutureLabLoadSnapshot *snapshot)
{
    FutureLabLoadSnapshot parsed = {0};
    char line[256];
    const char *cursor;

    if (stream == NULL || snapshot == NULL) return -1;
    *snapshot = (FutureLabLoadSnapshot){0};
    if (fgets(line, sizeof(line), stream) == NULL) return -1;
    cursor = line;
    if (parse_decimal(&cursor, &parsed.one_minute) != 0 ||
        parse_decimal(&cursor, &parsed.five_minutes) != 0 ||
        parse_decimal(&cursor, &parsed.fifteen_minutes) != 0 ||
        parse_u64(&cursor, &parsed.running_tasks) != 0 || *cursor != '/' ||
        (++cursor, parse_u64(&cursor, &parsed.total_tasks)) != 0 ||
        parsed.total_tasks == 0U || parsed.running_tasks > parsed.total_tasks) return -1;
    parsed.state = FUTURE_LAB_STATE_AVAILABLE;
    *snapshot = parsed;
    return 0;
}

static int parse_meminfo_value(const char *line, const char *key, uint64_t *value, bool *found)
{
    size_t key_length;
    const char *cursor;

    if (line == NULL || key == NULL || value == NULL || found == NULL) return -1;
    key_length = strlen(key);
    if (strncmp(line, key, key_length) != 0 || line[key_length] != ':') return 0;
    cursor = line + key_length + 1U;
    if (parse_u64(&cursor, value) != 0) return -1;
    skip_spaces(&cursor);
    if (strncmp(cursor, "kB", 2U) != 0) return -1;
    cursor += 2U;
    skip_spaces(&cursor);
    if (*cursor != '\0') return -1;
    *found = true;
    return 1;
}

int future_lab_parse_meminfo(FILE *stream, FutureLabMemorySnapshot *snapshot)
{
    FutureLabMemorySnapshot parsed = {0};
    char line[512];
    bool found[6] = {false};

    if (stream == NULL || snapshot == NULL) return -1;
    *snapshot = (FutureLabMemorySnapshot){0};
    while (fgets(line, sizeof(line), stream) != NULL) {
        int result;

        result = parse_meminfo_value(line, "MemTotal", &parsed.total_kib, &found[0]);
        if (result < 0) return -1;
        if (result > 0) continue;
        result = parse_meminfo_value(line, "MemAvailable", &parsed.available_kib, &found[1]);
        if (result < 0) return -1;
        if (result > 0) continue;
        result = parse_meminfo_value(line, "Buffers", &parsed.buffers_kib, &found[2]);
        if (result < 0) return -1;
        if (result > 0) continue;
        result = parse_meminfo_value(line, "Cached", &parsed.cached_kib, &found[3]);
        if (result < 0) return -1;
        if (result > 0) continue;
        result = parse_meminfo_value(line, "SwapTotal", &parsed.swap_total_kib, &found[4]);
        if (result < 0) return -1;
        if (result > 0) continue;
        result = parse_meminfo_value(line, "SwapFree", &parsed.swap_free_kib, &found[5]);
        if (result < 0) return -1;
    }
    if (ferror(stream)) return -1;
    for (size_t index = 0U; index < sizeof(found) / sizeof(found[0]); index++) {
        if (!found[index]) return -1;
    }
    if (parsed.total_kib == 0U || parsed.available_kib > parsed.total_kib ||
        parsed.swap_free_kib > parsed.swap_total_kib) return -1;
    parsed.used_kib = parsed.total_kib - parsed.available_kib;
    parsed.state = FUTURE_LAB_STATE_AVAILABLE;
    *snapshot = parsed;
    return 0;
}

static int parse_network_line(const char *line, FutureLabNetworkInterface *interface)
{
    const char *name_begin = line;
    const char *name_end;
    const char *cursor;
    uint64_t fields[16];
    size_t name_length;

    if (line == NULL || interface == NULL) return -1;
    while (isspace((unsigned char)*name_begin)) name_begin++;
    name_end = strchr(name_begin, ':');
    if (name_end == NULL) return -1;
    while (name_end > name_begin && isspace((unsigned char)name_end[-1])) name_end--;
    name_length = (size_t)(name_end - name_begin);
    if (name_length == 0U || name_length >= sizeof(interface->name)) return -1;
    cursor = strchr(name_begin, ':') + 1U;
    for (size_t index = 0U; index < sizeof(fields) / sizeof(fields[0]); index++) {
        if (parse_u64(&cursor, &fields[index]) != 0 ||
            (!isspace((unsigned char)*cursor) && *cursor != '\0')) return -1;
    }
    skip_spaces(&cursor);
    if (*cursor != '\0') return -1;
    *interface = (FutureLabNetworkInterface){0};
    (void)memcpy(interface->name, name_begin, name_length);
    interface->name[name_length] = '\0';
    interface->received_bytes = fields[0];
    interface->received_packets = fields[1];
    interface->received_errors = fields[2];
    interface->received_dropped = fields[3];
    interface->transmitted_bytes = fields[8];
    interface->transmitted_packets = fields[9];
    interface->transmitted_errors = fields[10];
    interface->transmitted_dropped = fields[11];
    return 0;
}

int future_lab_parse_net_dev(FILE *stream, FutureLabNetworkSnapshot *snapshot)
{
    FutureLabNetworkSnapshot parsed = {0};
    char line[1024];

    if (stream == NULL || snapshot == NULL) return -1;
    *snapshot = (FutureLabNetworkSnapshot){0};
    if (fgets(line, sizeof(line), stream) == NULL || strstr(line, "Inter-|") == NULL ||
        fgets(line, sizeof(line), stream) == NULL || strstr(line, "face |") == NULL) return -1;
    while (fgets(line, sizeof(line), stream) != NULL) {
        FutureLabNetworkInterface interface;

        if (parse_network_line(line, &interface) != 0) return -1;
        if (parsed.observed_interface_count == SIZE_MAX ||
            add_u64(&parsed.received_bytes, interface.received_bytes) != 0 ||
            add_u64(&parsed.transmitted_bytes, interface.transmitted_bytes) != 0) return -1;
        parsed.observed_interface_count++;
        if (parsed.interface_count < FUTURE_LAB_NETWORK_INTERFACE_LIMIT) {
            parsed.interfaces[parsed.interface_count++] = interface;
        } else {
            parsed.truncated = true;
        }
    }
    if (ferror(stream) || parsed.observed_interface_count == 0U) return -1;
    parsed.state = FUTURE_LAB_STATE_AVAILABLE;
    *snapshot = parsed;
    return 0;
}

static bool numbered_device_name(const char *name, const char *prefix)
{
    const char *suffix;
    size_t prefix_length;

    if (name == NULL || prefix == NULL) return false;
    prefix_length = strlen(prefix);
    if (strncmp(name, prefix, prefix_length) != 0) return false;
    suffix = name + prefix_length;
    if (!isdigit((unsigned char)*suffix)) return false;
    while (isdigit((unsigned char)*suffix)) suffix++;
    return *suffix == '\0';
}

static bool pseudo_disk_name(const char *name)
{
    return numbered_device_name(name, "loop") || numbered_device_name(name, "ram") ||
        numbered_device_name(name, "zram");
}

static int parse_diskstats_line(const char *line, FutureLabDiskDevice *device)
{
    const char *cursor = line;
    const char *name_begin;
    size_t name_length;
    uint64_t ignored;
    uint64_t fields[11];
    size_t field_count = 0U;

    if (line == NULL || device == NULL) return -1;
    if (parse_u64(&cursor, &ignored) != 0 || !isspace((unsigned char)*cursor) ||
        parse_u64(&cursor, &ignored) != 0 || !isspace((unsigned char)*cursor)) return -1;
    skip_spaces(&cursor);
    name_begin = cursor;
    while (*cursor != '\0' && !isspace((unsigned char)*cursor)) cursor++;
    name_length = (size_t)(cursor - name_begin);
    if (name_length == 0U || name_length >= sizeof(device->name)) return -1;
    while (true) {
        uint64_t value;

        skip_spaces(&cursor);
        if (*cursor == '\0') break;
        if (parse_u64(&cursor, &value) != 0 ||
            (!isspace((unsigned char)*cursor) && *cursor != '\0')) return -1;
        if (field_count < sizeof(fields) / sizeof(fields[0])) fields[field_count] = value;
        field_count++;
    }
    if (field_count < sizeof(fields) / sizeof(fields[0])) return -1;
    *device = (FutureLabDiskDevice){0};
    (void)memcpy(device->name, name_begin, name_length);
    device->name[name_length] = '\0';
    device->reads_completed = fields[0];
    device->sectors_read = fields[2];
    device->writes_completed = fields[4];
    device->sectors_written = fields[6];
    return 0;
}

int future_lab_parse_diskstats(FILE *stream, FutureLabDiskSnapshot *snapshot)
{
    FutureLabDiskSnapshot parsed = {0};
    char line[1024];
    size_t parsed_line_count = 0U;

    if (stream == NULL || snapshot == NULL) return -1;
    *snapshot = (FutureLabDiskSnapshot){0};
    while (fgets(line, sizeof(line), stream) != NULL) {
        FutureLabDiskDevice device;

        if (parse_diskstats_line(line, &device) != 0 || parsed_line_count == SIZE_MAX) return -1;
        parsed_line_count++;
        if (pseudo_disk_name(device.name)) {
            if (parsed.skipped_pseudo_device_count == SIZE_MAX) return -1;
            parsed.skipped_pseudo_device_count++;
            continue;
        }
        if (parsed.observed_device_count == SIZE_MAX) return -1;
        parsed.observed_device_count++;
        if (parsed.device_count < FUTURE_LAB_DISK_LIMIT) {
            parsed.devices[parsed.device_count++] = device;
        } else {
            parsed.truncated = true;
        }
    }
    if (ferror(stream) || parsed_line_count == 0U) return -1;
    parsed.state = FUTURE_LAB_STATE_AVAILABLE;
    *snapshot = parsed;
    return 0;
}

const char *future_lab_state_name(FutureLabState state)
{
    return state == FUTURE_LAB_STATE_AVAILABLE ? "available" : "unknown";
}

static void collect_cpu(FutureLabSnapshot *snapshot)
{
    FILE *stream = fopen(PROC_STAT_PATH, "r");

    if (stream == NULL) return;
    (void)future_lab_parse_cpu_stat(stream, &snapshot->cpu);
    (void)fclose(stream);
}

static void collect_load(FutureLabSnapshot *snapshot)
{
    FILE *stream = fopen(PROC_LOADAVG_PATH, "r");

    if (stream == NULL) return;
    (void)future_lab_parse_loadavg(stream, &snapshot->load);
    (void)fclose(stream);
}

static void collect_memory(FutureLabSnapshot *snapshot)
{
    FILE *stream = fopen(PROC_MEMINFO_PATH, "r");

    if (stream == NULL) return;
    (void)future_lab_parse_meminfo(stream, &snapshot->memory);
    (void)fclose(stream);
}

static void collect_network(FutureLabSnapshot *snapshot)
{
    FILE *stream = fopen(PROC_NET_DEV_PATH, "r");

    if (stream == NULL) return;
    (void)future_lab_parse_net_dev(stream, &snapshot->network);
    (void)fclose(stream);
}

static void collect_disks(FutureLabSnapshot *snapshot)
{
    FILE *stream = fopen(PROC_DISKSTATS_PATH, "r");

    if (stream == NULL) return;
    (void)future_lab_parse_diskstats(stream, &snapshot->disks);
    (void)fclose(stream);
}

int future_lab_collect(FutureLabSnapshot *snapshot, char *error, size_t error_size)
{
    if (snapshot == NULL) {
        set_error(error, error_size, "Invalid Future Lab snapshot.");
        return -1;
    }
    *snapshot = (FutureLabSnapshot){0};
    if (error != NULL && error_size > 0U) error[0] = '\0';
    collect_cpu(snapshot);
    collect_load(snapshot);
    collect_memory(snapshot);
    collect_network(snapshot);
    collect_disks(snapshot);
    if (snapshot->cpu.state != FUTURE_LAB_STATE_AVAILABLE ||
        snapshot->load.state != FUTURE_LAB_STATE_AVAILABLE ||
        snapshot->memory.state != FUTURE_LAB_STATE_AVAILABLE ||
        snapshot->network.state != FUTURE_LAB_STATE_AVAILABLE ||
        snapshot->disks.state != FUTURE_LAB_STATE_AVAILABLE) {
        set_error(error, error_size, "Some local /proc metrics are unavailable; inspect each section state.");
    }
    return 0;
}
