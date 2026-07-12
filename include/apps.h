#ifndef LINUX_DOCTOR_APPS_H
#define LINUX_DOCTOR_APPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct AppsInfo {
    bool package_database_available;
    bool gnome_tweaks_installed;
} AppsInfo;

int apps_parse_dpkg_status(FILE *stream, AppsInfo *apps);
int apps_collect(AppsInfo *apps, char *error, size_t error_size);

#endif
