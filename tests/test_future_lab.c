#define _POSIX_C_SOURCE 200809L

#include "future_lab.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static FILE *fixture_stream(const char *contents)
{
    FILE *stream = tmpfile();

    assert(stream != NULL);
    assert(fputs(contents, stream) >= 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    return stream;
}

static void test_cpu(void)
{
    FutureLabCpuSnapshot cpu;
    FILE *stream = fixture_stream(
        "cpu  100 5 40 800 10 2 3 4 1 1\n"
        "cpu0 25 1 10 200 2 0 1 1 0 0\n"
        "cpu1 25 1 10 200 2 0 1 1 0 0\n"
        "intr 123\n");

    assert(future_lab_parse_cpu_stat(stream, &cpu) == 0);
    assert(fclose(stream) == 0);
    assert(cpu.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(cpu.logical_cpu_count == 2U);
    assert(cpu.busy_ticks == 154U);
    assert(cpu.total_ticks == 964U);
    assert(cpu.idle_ticks == 800U);

    stream = fixture_stream("cpu 1 2 3 4\ncpu0 1 2 3 4\n");
    assert(future_lab_parse_cpu_stat(stream, &cpu) == 0);
    assert(cpu.total_ticks == 10U);
    assert(cpu.iowait_ticks == 0U);
    assert(fclose(stream) == 0);

    stream = fixture_stream("cpu invalid 2 3 4\ncpu0 1 2 3 4\n");
    assert(future_lab_parse_cpu_stat(stream, &cpu) == -1);
    assert(cpu.state == FUTURE_LAB_STATE_UNKNOWN);
    assert(fclose(stream) == 0);

    stream = fixture_stream(
        "cpu 18446744073709551615 1 0 1\n"
        "cpu0 1 0 0 1\n");
    assert(future_lab_parse_cpu_stat(stream, &cpu) == -1);
    assert(cpu.state == FUTURE_LAB_STATE_UNKNOWN);
    assert(fclose(stream) == 0);
}

static void test_load(void)
{
    FutureLabLoadSnapshot load;
    FILE *stream = fixture_stream("0.42 1.25 3.50 2/345 9876\n");

    assert(future_lab_parse_loadavg(stream, &load) == 0);
    assert(fclose(stream) == 0);
    assert(load.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(load.one_minute > 0.419 && load.one_minute < 0.421);
    assert(load.fifteen_minutes == 3.5);
    assert(load.running_tasks == 2U);
    assert(load.total_tasks == 345U);

    stream = fixture_stream("-1.0 1.0 1.0 1/10 42\n");
    assert(future_lab_parse_loadavg(stream, &load) == -1);
    assert(load.state == FUTURE_LAB_STATE_UNKNOWN);
    assert(fclose(stream) == 0);

    stream = fixture_stream("1.0 1.0 1.0 11/10 42\n");
    assert(future_lab_parse_loadavg(stream, &load) == -1);
    assert(fclose(stream) == 0);
}

static void test_memory(void)
{
    FutureLabMemorySnapshot memory;
    FILE *stream = fixture_stream(
        "MemTotal:       32000000 kB\n"
        "MemFree:         1000000 kB\n"
        "MemAvailable:   12000000 kB\n"
        "Buffers:          500000 kB\n"
        "Cached:          7000000 kB\n"
        "SwapCached:            0 kB\n"
        "SwapTotal:       8000000 kB\n"
        "SwapFree:        6000000 kB\n");

    assert(future_lab_parse_meminfo(stream, &memory) == 0);
    assert(fclose(stream) == 0);
    assert(memory.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(memory.used_kib == 20000000U);
    assert(memory.cached_kib == 7000000U);
    assert(memory.swap_total_kib == 8000000U);

    stream = fixture_stream(
        "MemTotal: 100 kB\nMemAvailable: 200 kB\nBuffers: 0 kB\n"
        "Cached: 0 kB\nSwapTotal: 0 kB\nSwapFree: 0 kB\n");
    assert(future_lab_parse_meminfo(stream, &memory) == -1);
    assert(memory.state == FUTURE_LAB_STATE_UNKNOWN);
    assert(fclose(stream) == 0);

    stream = fixture_stream("MemTotal: 100 kB\n");
    assert(future_lab_parse_meminfo(stream, &memory) == -1);
    assert(fclose(stream) == 0);
}

static const char network_header[] =
    "Inter-|   Receive                                                |  Transmit\n"
    " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n";

static void test_network(void)
{
    FutureLabNetworkSnapshot network;
    FILE *stream = fixture_stream(
        "Inter-|   Receive                                                |  Transmit\n"
        " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        "    lo: 100 2 0 0 0 0 0 0 200 3 0 0 0 0 0 0\n"
        "enp5s0: 3000 30 1 2 0 0 0 0 4000 40 3 4 0 0 0 0\n");

    assert(future_lab_parse_net_dev(stream, &network) == 0);
    assert(fclose(stream) == 0);
    assert(network.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(network.interface_count == 2U);
    assert(network.observed_interface_count == 2U);
    assert(!network.truncated);
    assert(strcmp(network.interfaces[1].name, "enp5s0") == 0);
    assert(network.interfaces[1].received_errors == 1U);
    assert(network.interfaces[1].transmitted_dropped == 4U);
    assert(network.received_bytes == 3100U);
    assert(network.transmitted_bytes == 4200U);

    stream = fixture_stream(
        "Inter-| Receive | Transmit\n"
        " face |bytes packets errs drop fifo frame compressed multicast|bytes packets errs drop fifo colls carrier compressed\n"
        "lo: 1 2 3\n");
    assert(future_lab_parse_net_dev(stream, &network) == -1);
    assert(network.state == FUTURE_LAB_STATE_UNKNOWN);
    assert(fclose(stream) == 0);
}

static void test_network_limit(void)
{
    FutureLabNetworkSnapshot network;
    FILE *stream = tmpfile();

    assert(stream != NULL);
    assert(fputs(network_header, stream) >= 0);
    for (size_t index = 0U; index < FUTURE_LAB_NETWORK_INTERFACE_LIMIT + 1U; index++) {
        assert(fprintf(stream, "if%zu: 1 1 0 0 0 0 0 0 2 1 0 0 0 0 0 0\n", index) > 0);
    }
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(future_lab_parse_net_dev(stream, &network) == 0);
    assert(fclose(stream) == 0);
    assert(network.truncated);
    assert(network.interface_count == FUTURE_LAB_NETWORK_INTERFACE_LIMIT);
    assert(network.observed_interface_count == FUTURE_LAB_NETWORK_INTERFACE_LIMIT + 1U);
    assert(network.received_bytes == FUTURE_LAB_NETWORK_INTERFACE_LIMIT + 1U);
    assert(network.transmitted_bytes == 2U * (FUTURE_LAB_NETWORK_INTERFACE_LIMIT + 1U));
}

static void test_disks(void)
{
    FutureLabDiskSnapshot disks;
    FILE *stream = fixture_stream(
        "   8       0 sda 100 1 2000 30 50 2 4000 60 0 90 100\n"
        "   8       1 sda1 80 1 1500 20 40 2 3000 50 0 70 80\n"
        "   7       0 loop0 5 0 20 1 0 0 0 0 0 1 1\n"
        " 253       0 dm-0 70 0 1200 10 30 0 2400 20 0 40 50 0 0 0 0\n"
        " 254       0 ramdisk 1 0 2 0 3 0 4 0 0 0 0\n");

    assert(future_lab_parse_diskstats(stream, &disks) == 0);
    assert(fclose(stream) == 0);
    assert(disks.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(disks.observed_device_count == 4U);
    assert(disks.device_count == 4U);
    assert(disks.skipped_pseudo_device_count == 1U);
    assert(!disks.truncated);
    assert(strcmp(disks.devices[0].name, "sda") == 0);
    assert(disks.devices[0].major == 8U);
    assert(disks.devices[0].minor == 0U);
    assert(disks.devices[0].reads_completed == 100U);
    assert(disks.devices[0].sectors_read == 2000U);
    assert(disks.devices[0].writes_completed == 50U);
    assert(disks.devices[0].sectors_written == 4000U);
    assert(strcmp(disks.devices[2].name, "dm-0") == 0);

    stream = fixture_stream(
        "7 0 loop0 1 0 1 0 1 0 1 0 0 0 0\n"
        "1 0 ram0 1 0 1 0 1 0 1 0 0 0 0\n"
        "252 0 zram0 1 0 1 0 1 0 1 0 0 0 0\n");
    assert(future_lab_parse_diskstats(stream, &disks) == 0);
    assert(disks.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(disks.observed_device_count == 0U);
    assert(disks.skipped_pseudo_device_count == 3U);
    assert(fclose(stream) == 0);

    stream = fixture_stream("8 0 sda 1 0 2\n");
    assert(future_lab_parse_diskstats(stream, &disks) == -1);
    assert(disks.state == FUTURE_LAB_STATE_UNKNOWN);
    assert(fclose(stream) == 0);

    stream = fixture_stream(
        "8 0 sda 18446744073709551616 0 2 0 1 0 2 0 0 0 0\n");
    assert(future_lab_parse_diskstats(stream, &disks) == -1);
    assert(fclose(stream) == 0);
}

static void test_disk_limit(void)
{
    FutureLabDiskSnapshot disks;
    FILE *stream = tmpfile();

    assert(stream != NULL);
    for (size_t index = 0U; index < FUTURE_LAB_DISK_LIMIT + 1U; index++) {
        assert(fprintf(stream, "8 %zu sd%zu 1 0 2 0 3 0 4 0 0 0 0\n", index, index) > 0);
    }
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(future_lab_parse_diskstats(stream, &disks) == 0);
    assert(fclose(stream) == 0);
    assert(disks.truncated);
    assert(disks.device_count == FUTURE_LAB_DISK_LIMIT);
    assert(disks.observed_device_count == FUTURE_LAB_DISK_LIMIT + 1U);
}

static void test_live_collection(void)
{
    FutureLabSnapshot snapshot;
    char error[160];

    assert(future_lab_collect(NULL, error, sizeof(error)) == -1);
    assert(error[0] != '\0');
    assert(future_lab_collect(&snapshot, error, sizeof(error)) == 0);
    assert(snapshot.cpu.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(snapshot.load.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(snapshot.memory.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(snapshot.network.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(snapshot.disks.state == FUTURE_LAB_STATE_AVAILABLE);
    assert(error[0] == '\0');
}

int main(void)
{
    FutureLabCpuSnapshot cpu;

    assert(strcmp(future_lab_state_name(FUTURE_LAB_STATE_AVAILABLE), "available") == 0);
    assert(strcmp(future_lab_state_name(FUTURE_LAB_STATE_UNKNOWN), "unknown") == 0);
    assert(strcmp(future_lab_state_name((FutureLabState)99), "unknown") == 0);
    assert(future_lab_parse_cpu_stat(NULL, &cpu) == -1);
    assert(future_lab_parse_cpu_stat(stdin, NULL) == -1);
    test_cpu();
    test_load();
    test_memory();
    test_network();
    test_network_limit();
    test_disks();
    test_disk_limit();
    test_live_collection();
    return 0;
}
