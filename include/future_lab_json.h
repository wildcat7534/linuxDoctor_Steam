#ifndef LINUX_DOCTOR_FUTURE_LAB_JSON_H
#define LINUX_DOCTOR_FUTURE_LAB_JSON_H

#include "future_lab.h"

#include <stdint.h>
#include <stdio.h>

#define FUTURE_LAB_LIVE_SCHEMA_NAME "linux-doctor.future-lab.live"
#define FUTURE_LAB_LIVE_SCHEMA_VERSION 1U
#define FUTURE_LAB_GENERATED_AT_CAPACITY 32U
#define FUTURE_LAB_BOOT_ID_CAPACITY 37U

typedef struct FutureLabSampleMetadata {
    char generated_at[FUTURE_LAB_GENERATED_AT_CAPACITY];
    char boot_id[FUTURE_LAB_BOOT_ID_CAPACITY];
    uint64_t unix_milliseconds;
    uint64_t monotonic_milliseconds;
} FutureLabSampleMetadata;

int future_lab_sample_metadata_now(FutureLabSampleMetadata *metadata);
int future_lab_json_write_snapshot(FILE *stream, const FutureLabSnapshot *snapshot);
int future_lab_json_write_document(FILE *stream, const FutureLabSnapshot *snapshot,
    const FutureLabSampleMetadata *metadata);

#endif
