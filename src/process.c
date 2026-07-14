#define _POSIX_C_SOURCE 200809L

#include "process.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size > 0U) (void)snprintf(error, error_size, "%s", message);
}

static long long monotonic_milliseconds(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return -1;
    return (long long)now.tv_sec * 1000LL + now.tv_nsec / 1000000LL;
}

static char **environment_with_c_locale(void)
{
    size_t count = 0U;
    size_t output_count = 0U;
    char **environment;

    while (environ[count] != NULL) count++;
    environment = calloc(count + 2U, sizeof(*environment));
    if (environment == NULL) return NULL;
    for (size_t index = 0U; index < count; index++) {
        if (strncmp(environ[index], "LC_ALL=", 7U) != 0) environment[output_count++] = environ[index];
    }
    environment[output_count++] = (char *)"LC_ALL=C";
    environment[output_count] = NULL;
    return environment;
}

static void terminate_child(pid_t child)
{
    int status;

    if (kill(-child, SIGKILL) != 0) (void)kill(child, SIGKILL);
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }
}

static void terminate_group_after_leader_exit(pid_t child)
{
    (void)kill(-child, SIGKILL);
}

int process_capture(const char *path, char *const argv[], char *output, size_t output_size,
    unsigned int timeout_ms, ProcessResult *result, char *error, size_t error_size)
{
    int descriptors[2];
    int flags;
    int spawn_result;
    int child_status = 0;
    bool child_finished = false;
    bool output_finished = false;
    size_t written = 0U;
    pid_t child;
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    char **child_environment;
    long long deadline;

    if (path == NULL || argv == NULL || output == NULL || output_size == 0U || result == NULL || timeout_ms == 0U) {
        set_error(error, error_size, "Invalid process capture arguments.");
        return -1;
    }
    *result = (ProcessResult){0};
    output[0] = '\0';
    if (pipe(descriptors) != 0) {
        set_error(error, error_size, strerror(errno));
        return -1;
    }
    if (posix_spawn_file_actions_init(&actions) != 0) {
        (void)close(descriptors[0]);
        (void)close(descriptors[1]);
        set_error(error, error_size, "Cannot initialize process file actions.");
        return -1;
    }
    if (posix_spawn_file_actions_addclose(&actions, descriptors[0]) != 0 ||
        posix_spawn_file_actions_adddup2(&actions, descriptors[1], STDOUT_FILENO) != 0 ||
        posix_spawn_file_actions_addclose(&actions, descriptors[1]) != 0 ||
        posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0) != 0) {
        (void)posix_spawn_file_actions_destroy(&actions);
        (void)close(descriptors[0]);
        (void)close(descriptors[1]);
        set_error(error, error_size, "Cannot configure process file actions.");
        return -1;
    }
    if (posix_spawnattr_init(&attributes) != 0) {
        (void)posix_spawn_file_actions_destroy(&actions);
        (void)close(descriptors[0]);
        (void)close(descriptors[1]);
        set_error(error, error_size, "Cannot initialize process attributes.");
        return -1;
    }
    if (posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP) != 0 ||
        posix_spawnattr_setpgroup(&attributes, 0) != 0) {
        (void)posix_spawn_file_actions_destroy(&actions);
        (void)posix_spawnattr_destroy(&attributes);
        (void)close(descriptors[0]);
        (void)close(descriptors[1]);
        set_error(error, error_size, "Cannot configure process group.");
        return -1;
    }
    child_environment = environment_with_c_locale();
    if (child_environment == NULL) {
        (void)posix_spawn_file_actions_destroy(&actions);
        (void)posix_spawnattr_destroy(&attributes);
        (void)close(descriptors[0]);
        (void)close(descriptors[1]);
        set_error(error, error_size, "Cannot allocate child environment.");
        return -1;
    }
    spawn_result = posix_spawn(&child, path, &actions, &attributes, argv, child_environment);
    free(child_environment);
    (void)posix_spawn_file_actions_destroy(&actions);
    (void)posix_spawnattr_destroy(&attributes);
    (void)close(descriptors[1]);
    if (spawn_result != 0) {
        (void)close(descriptors[0]);
        set_error(error, error_size, strerror(spawn_result));
        return -1;
    }
    flags = fcntl(descriptors[0], F_GETFL, 0);
    if (flags < 0 || fcntl(descriptors[0], F_SETFL, flags | O_NONBLOCK) != 0) {
        terminate_child(child);
        (void)close(descriptors[0]);
        set_error(error, error_size, strerror(errno));
        return -1;
    }
    deadline = monotonic_milliseconds();
    if (deadline < 0) {
        terminate_child(child);
        (void)close(descriptors[0]);
        set_error(error, error_size, "Monotonic clock is unavailable.");
        return -1;
    }
    deadline += (long long)timeout_ms;

    while (!child_finished || !output_finished) {
        long long now = monotonic_milliseconds();
        int remaining;
        int wait_result;

        if (now < 0 || now >= deadline) {
            result->timed_out = true;
            if (child_finished) terminate_group_after_leader_exit(child);
            else terminate_child(child);
            child_finished = true;
            break;
        }
        remaining = (int)(deadline - now);
        if (remaining > 50) remaining = 50;
        if (!output_finished) {
            struct pollfd descriptor = {.fd = descriptors[0], .events = POLLIN | POLLHUP};
            int poll_result = poll(&descriptor, 1U, remaining);

            if (poll_result < 0 && errno != EINTR) {
                if (child_finished) terminate_group_after_leader_exit(child);
                else terminate_child(child);
                (void)close(descriptors[0]);
                set_error(error, error_size, strerror(errno));
                return -1;
            }
            if (poll_result > 0 && (descriptor.revents & (POLLIN | POLLHUP)) != 0) {
                for (;;) {
                    char chunk[4096];
                    ssize_t count = read(descriptors[0], chunk, sizeof(chunk));

                    if (count > 0) {
                        size_t available = output_size - 1U - written;
                        size_t copy_size = (size_t)count < available ? (size_t)count : available;
                        if (copy_size > 0U) {
                            memcpy(output + written, chunk, copy_size);
                            written += copy_size;
                        }
                        if (copy_size < (size_t)count) result->truncated = true;
                    } else if (count == 0) {
                        output_finished = true;
                        break;
                    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    } else if (errno != EINTR) {
                        if (child_finished) terminate_group_after_leader_exit(child);
                        else terminate_child(child);
                        (void)close(descriptors[0]);
                        set_error(error, error_size, strerror(errno));
                        return -1;
                    }
                }
            }
        } else {
            (void)poll(NULL, 0U, remaining);
        }
        if (!child_finished) {
            wait_result = waitpid(child, &child_status, WNOHANG);
            if (wait_result == child) child_finished = true;
            else if (wait_result < 0 && errno != EINTR) {
                (void)close(descriptors[0]);
                set_error(error, error_size, strerror(errno));
                return -1;
            }
        }
    }
    output[written] = '\0';
    (void)close(descriptors[0]);
    if (result->timed_out) return 0;
    result->exited = child_finished && WIFEXITED(child_status);
    result->exit_code = result->exited ? WEXITSTATUS(child_status) : -1;
    return 0;
}
