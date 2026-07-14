#ifndef LINUX_DOCTOR_FUTURE_LAB_H
#define LINUX_DOCTOR_FUTURE_LAB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define FUTURE_LAB_NETWORK_INTERFACE_LIMIT 32U
#define FUTURE_LAB_INTERFACE_NAME_CAPACITY 64U
#define FUTURE_LAB_DISK_LIMIT 64U
#define FUTURE_LAB_DISK_NAME_CAPACITY 64U
#define FUTURE_LAB_GPU_NAME_CAPACITY 128U

typedef enum FutureLabState {
    FUTURE_LAB_STATE_UNKNOWN = 0,
    FUTURE_LAB_STATE_AVAILABLE = 1
} FutureLabState;

typedef struct FutureLabCpuSnapshot {
    FutureLabState state;
    size_t logical_cpu_count;
    /* Cumulative scheduler ticks since boot. Two snapshots are needed for a usage rate. */
    uint64_t user_ticks;
    uint64_t nice_ticks;
    uint64_t system_ticks;
    uint64_t idle_ticks;
    uint64_t iowait_ticks;
    uint64_t irq_ticks;
    uint64_t softirq_ticks;
    uint64_t steal_ticks;
    uint64_t busy_ticks;
    uint64_t total_ticks;
} FutureLabCpuSnapshot;

typedef struct FutureLabLoadSnapshot {
    FutureLabState state;
    /* Kernel run-queue averages from /proc/loadavg, not percentages. */
    double one_minute;
    double five_minutes;
    double fifteen_minutes;
    uint64_t running_tasks;
    uint64_t total_tasks;
} FutureLabLoadSnapshot;

typedef struct FutureLabMemorySnapshot {
    FutureLabState state;
    uint64_t total_kib;
    uint64_t available_kib;
    /* total_kib - available_kib: memory unavailable for new work without reclaim. */
    uint64_t used_kib;
    uint64_t buffers_kib;
    uint64_t cached_kib;
    uint64_t swap_total_kib;
    uint64_t swap_free_kib;
} FutureLabMemorySnapshot;

typedef struct FutureLabNetworkInterface {
    char name[FUTURE_LAB_INTERFACE_NAME_CAPACITY];
    uint64_t received_bytes;
    uint64_t received_packets;
    uint64_t received_errors;
    uint64_t received_dropped;
    uint64_t transmitted_bytes;
    uint64_t transmitted_packets;
    uint64_t transmitted_errors;
    uint64_t transmitted_dropped;
} FutureLabNetworkInterface;

typedef struct FutureLabNetworkSnapshot {
    FutureLabState state;
    bool truncated;
    size_t observed_interface_count;
    size_t interface_count;
    FutureLabNetworkInterface interfaces[FUTURE_LAB_NETWORK_INTERFACE_LIMIT];
    /* Cumulative totals for all observed interfaces, including loopback. */
    uint64_t received_bytes;
    uint64_t transmitted_bytes;
} FutureLabNetworkSnapshot;

typedef struct FutureLabDiskDevice {
    char name[FUTURE_LAB_DISK_NAME_CAPACITY];
    uint64_t major;
    uint64_t minor;
    /* Cumulative kernel counters. Sector size must not be inferred here. */
    uint64_t reads_completed;
    uint64_t sectors_read;
    uint64_t writes_completed;
    uint64_t sectors_written;
} FutureLabDiskDevice;

typedef struct FutureLabDiskSnapshot {
    FutureLabState state;
    bool truncated;
    size_t observed_device_count;
    size_t skipped_pseudo_device_count;
    size_t device_count;
    FutureLabDiskDevice devices[FUTURE_LAB_DISK_LIMIT];
} FutureLabDiskSnapshot;

typedef struct FutureLabGpuSnapshot {
    FutureLabState state;
    uint64_t index;
    char name[FUTURE_LAB_GPU_NAME_CAPACITY];
    double utilization_percent;
    double memory_used_mib;
    double memory_total_mib;
    double temperature_celsius;
    double power_watts;
    bool nvtop_available;
} FutureLabGpuSnapshot;

typedef struct FutureLabSnapshot {
    FutureLabCpuSnapshot cpu;
    FutureLabLoadSnapshot load;
    FutureLabMemorySnapshot memory;
    FutureLabNetworkSnapshot network;
    FutureLabDiskSnapshot disks;
    FutureLabGpuSnapshot gpu;
} FutureLabSnapshot;

const char *future_lab_state_name(FutureLabState state);
int future_lab_parse_cpu_stat(FILE *stream, FutureLabCpuSnapshot *snapshot);
int future_lab_parse_loadavg(FILE *stream, FutureLabLoadSnapshot *snapshot);
int future_lab_parse_meminfo(FILE *stream, FutureLabMemorySnapshot *snapshot);
int future_lab_parse_net_dev(FILE *stream, FutureLabNetworkSnapshot *snapshot);
int future_lab_parse_diskstats(FILE *stream, FutureLabDiskSnapshot *snapshot);
int future_lab_parse_nvidia_smi(FILE *stream, FutureLabGpuSnapshot *snapshot);
int future_lab_collect(FutureLabSnapshot *snapshot, char *error, size_t error_size);

#endif
