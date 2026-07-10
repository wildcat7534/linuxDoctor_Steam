#include "json.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    char buffer[64];
    FILE *stream = tmpfile();

    assert(stream != NULL);
    assert(json_write_string(stream, "quote: \" newline: \n") == 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    assert(fgets(buffer, sizeof(buffer), stream) != NULL);
    assert(strcmp(buffer, "\"quote: \\\" newline: \\n\"") == 0);
    assert(fclose(stream) == 0);
    return 0;
}
