#include "json.h"

#include <stdint.h>

int json_write_string(FILE *stream, const char *value)
{
    const unsigned char *cursor = (const unsigned char *)value;

    if (stream == NULL || value == NULL || fputc('"', stream) == EOF) {
        return -1;
    }
    while (*cursor != '\0') {
        switch (*cursor) {
        case '"': if (fputs("\\\"", stream) == EOF) return -1; break;
        case '\\': if (fputs("\\\\", stream) == EOF) return -1; break;
        case '\b': if (fputs("\\b", stream) == EOF) return -1; break;
        case '\f': if (fputs("\\f", stream) == EOF) return -1; break;
        case '\n': if (fputs("\\n", stream) == EOF) return -1; break;
        case '\r': if (fputs("\\r", stream) == EOF) return -1; break;
        case '\t': if (fputs("\\t", stream) == EOF) return -1; break;
        default:
            if (*cursor < 0x20U) {
                if (fprintf(stream, "\\u%04x", (uint32_t)*cursor) < 0) return -1;
            } else if (fputc(*cursor, stream) == EOF) {
                return -1;
            }
        }
        cursor++;
    }
    return fputc('"', stream) == EOF ? -1 : 0;
}

int json_write_key(FILE *stream, const char *key)
{
    return json_write_string(stream, key) == 0 && fputc(':', stream) != EOF ? 0 : -1;
}
