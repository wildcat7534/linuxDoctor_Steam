#ifndef LINUX_DOCTOR_REPORT_H
#define LINUX_DOCTOR_REPORT_H

#include <stdio.h>

#include "storage.h"
#include "history.h"

int report_write(FILE *stream, const StorageInfo *storage, const HistoryComparison *history);

#endif
