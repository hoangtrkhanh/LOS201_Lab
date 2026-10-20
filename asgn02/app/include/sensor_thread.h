#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

#include <stdbool.h>
#include <stdatomic.h>

#include "sms_types.h"
#include "circular_buffer.h"

typedef struct {
    SmsBuffer *buffer;
    const SmsConfig *config;
    atomic_bool *stop_requested;

    bool use_mock;

    unsigned int samples_produced;
    unsigned int read_errors;
    unsigned int eagain_count;
    unsigned int overflow_count;

    /* Single producer: Sensor Thread; main reads after pthread_join */
    int64_t *jitter_us;
    size_t jitter_capacity;
    size_t jitter_count;
} SensorThreadContext;

/* Parse a CSV line from /dev/sms_sensor */
int sms_sensor_parse_csv(
    const char *line,
    SensorData *sample
);

/* Sensor Thread entry point */
void *sms_sensor_thread(void *arg);

#endif
