#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sms_types.h"

/* Snapshot of buffer statistics */
typedef struct {
    size_t current_size;
    uint64_t total_produced;
    uint64_t total_consumed;
    uint64_t overflow_count;
} SmsBufferStats;

/* Thread-safe circular buffer */
typedef struct {
    SensorData *samples;

    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;

    uint64_t total_produced;
    uint64_t total_consumed;
    uint64_t overflow_count;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;

    bool stopping;
} SmsBuffer;

/* Buffer interface */
int sms_buffer_init(SmsBuffer *buffer, size_t capacity);

int sms_buffer_push(
    SmsBuffer *buffer,
    const SensorData *sample
);

int sms_buffer_pop(
    SmsBuffer *buffer,
    SensorData *sample
);

void sms_buffer_stop(SmsBuffer *buffer);

void sms_buffer_get_stats(
    SmsBuffer *buffer,
    SmsBufferStats *stats
);

void sms_buffer_destroy(SmsBuffer *buffer);

#endif
