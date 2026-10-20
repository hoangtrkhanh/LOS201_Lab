#ifndef PROCESSOR_THREAD_H
#define PROCESSOR_THREAD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sms_types.h"
#include "circular_buffer.h"

#define SMS_ROLLING_WINDOW 10U
#define SMS_TEMP_SPIKE_THRESHOLD 5.0

/* Alarm flags */
#define SMS_ALERT_TEMP_HIGH  (1U << 0)
#define SMS_ALERT_HUMID_HIGH (1U << 1)
#define SMS_ALERT_TEMP_SPIKE (1U << 2)

/* State of the 10-sample rolling window */
typedef struct {
    double temperatures[SMS_ROLLING_WINDOW];
    double humidities[SMS_ROLLING_WINDOW];

    double temp_sum;
    double hum_sum;

    size_t count;
    size_t next;

    double previous_temperature;
    bool has_previous;
} SmsProcessorState;

/* Result of processing one sample */
typedef struct {
    double avg_temperature;
    double avg_humidity;
    double temperature_change;

    size_t window_count;
    unsigned int alarm_flags;
} SmsProcessorResult;

/* Callback: later used to forward data to IPC queues */
typedef void (*SmsProcessorSink)(
    const SensorData *sample,
    const SmsProcessorResult *result,
    void *user
);

typedef struct {
    SmsBuffer *buffer;
    const SmsConfig *config;

    SmsProcessorSink sink;
    void *sink_user;

    uint64_t processed;
    uint64_t alarm_events;
    uint64_t errors;
} SmsProcessorContext;

int sms_processor_evaluate(
    SmsProcessorState *state,
    const SmsConfig *config,
    const SensorData *sample,
    SmsProcessorResult *result
);

void *sms_processor_thread(void *arg);

#endif
