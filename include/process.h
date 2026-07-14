#ifndef LINUX_DOCTOR_PROCESS_H
#define LINUX_DOCTOR_PROCESS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct ProcessResult {
    bool exited;
    bool timed_out;
    bool truncated;
    int exit_code;
} ProcessResult;

int process_capture(const char *path, char *const argv[], char *output, size_t output_size,
    unsigned int timeout_ms, ProcessResult *result, char *error, size_t error_size);

#endif
