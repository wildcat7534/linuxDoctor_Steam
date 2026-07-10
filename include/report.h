#ifndef LINUX_DOCTOR_REPORT_H
#define LINUX_DOCTOR_REPORT_H

#include <stdio.h>

#include "storage.h"

int report_write(FILE *stream, const StorageInfo *storage);

#endif
