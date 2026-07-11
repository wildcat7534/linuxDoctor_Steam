#include "report.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    StorageInfo storage = {
        .available = true,
        .total_bytes = 1000U,
        .available_bytes = 500U,
        .used_percent = 50U
    };
    HistoryComparison history = {.enabled = false};
    FILE *stream = tmpfile();
    char buffer[32768];

    assert(stream != NULL);
    assert(report_write(stream, &storage, &history) == 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(fread(buffer, 1, sizeof(buffer) - 1, stream) > 0);
    buffer[sizeof(buffer) - 1] = '\0';
    assert(strstr(buffer, "\"storage\"") != NULL);
    assert(strstr(buffer, "\"gaming\"") != NULL);
    assert(strstr(buffer, "steam-devices") != NULL);
    assert(fclose(stream) == 0);
    return 0;
}
