#ifndef LINUX_DOCTOR_JSON_H
#define LINUX_DOCTOR_JSON_H

#include <stdio.h>

int json_write_string(FILE *stream, const char *value);
int json_write_key(FILE *stream, const char *key);

#endif
