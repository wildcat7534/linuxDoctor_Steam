#define _POSIX_C_SOURCE 200809L

#include "process.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main(void)
{
    char output[32];
    char error[128];
    ProcessResult result;
    char *print_arguments[] = {(char *)"/usr/bin/printf", (char *)"linux-doctor", NULL};
    char *long_arguments[] = {(char *)"/usr/bin/printf", (char *)"0123456789", NULL};
    char *false_arguments[] = {(char *)"/usr/bin/false", NULL};
    char *sleep_arguments[] = {(char *)"/usr/bin/sleep", (char *)"1", NULL};
    char *missing_arguments[] = {(char *)"/path/that/does/not/exist", NULL};
    char *tree_arguments[] = {(char *)"/bin/sh", (char *)"-c",
        (char *)"sleep 5 & child=$!; printf '%s' \"$child\"; wait", NULL};

    assert(process_capture(NULL, print_arguments, output, sizeof(output), 1000U,
        &result, error, sizeof(error)) == -1);
    assert(process_capture("/usr/bin/printf", print_arguments, output, sizeof(output), 1000U,
        &result, error, sizeof(error)) == 0);
    assert(result.exited && result.exit_code == 0 && !result.timed_out && !result.truncated);
    assert(strcmp(output, "linux-doctor") == 0);

    assert(process_capture("/usr/bin/printf", long_arguments, output, 5U, 1000U,
        &result, error, sizeof(error)) == 0);
    assert(result.exited && result.exit_code == 0 && result.truncated);
    assert(strcmp(output, "0123") == 0);

    assert(process_capture("/usr/bin/false", false_arguments, output, sizeof(output), 1000U,
        &result, error, sizeof(error)) == 0);
    assert(result.exited && result.exit_code != 0 && !result.timed_out);

    assert(process_capture("/usr/bin/sleep", sleep_arguments, output, sizeof(output), 20U,
        &result, error, sizeof(error)) == 0);
    assert(result.timed_out);

    assert(process_capture("/path/that/does/not/exist", missing_arguments, output, sizeof(output), 1000U,
        &result, error, sizeof(error)) == -1);

    assert(process_capture("/bin/sh", tree_arguments, output, sizeof(output), 30U,
        &result, error, sizeof(error)) == 0);
    assert(result.timed_out);
    if (output[0] != '\0') {
        pid_t descendant = (pid_t)strtol(output, NULL, 10);
        struct timespec pause = {.tv_sec = 0, .tv_nsec = 10000000L};
        for (int attempt = 0; attempt < 20 && kill(descendant, 0) == 0; attempt++) {
            (void)nanosleep(&pause, NULL);
        }
        assert(kill(descendant, 0) == -1 && errno == ESRCH);
    }
    return 0;
}
