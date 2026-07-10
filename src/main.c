#include "report.h"
#include "storage.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static void print_usage(const char *program)
{
    (void)fprintf(stderr, "Usage: %s [--output FILE]\n", program);
}

int main(int argc, char **argv)
{
    const char *output_path = "report.json";
    StorageInfo storage;
    char error[256];
    FILE *output;

    if (argc == 3 && strcmp(argv[1], "--output") == 0) {
        output_path = argv[2];
    } else if (argc != 1) {
        print_usage(argv[0]);
        return 2;
    }
    if (storage_collect_root(&storage, error, sizeof(error)) != 0) {
        (void)fprintf(stderr, "Linux Doctor: unable to collect storage: %s\n", error);
        return 1;
    }
    output = fopen(output_path, "w");
    if (output == NULL) {
        (void)fprintf(stderr, "Linux Doctor: cannot write %s: %s\n", output_path, strerror(errno));
        return 1;
    }
    if (report_write(output, &storage) != 0) {
        (void)fclose(output);
        (void)fprintf(stderr, "Linux Doctor: cannot write report %s\n", output_path);
        return 1;
    }
    if (fclose(output) != 0) {
        (void)fprintf(stderr, "Linux Doctor: cannot write report %s\n", output_path);
        return 1;
    }
    (void)printf("Linux Doctor: report written to %s\n", output_path);
    return 0;
}
