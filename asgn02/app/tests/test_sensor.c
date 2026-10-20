#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

#include "sensor_thread.h"

int main(void)
{
    SmsBuffer buffer = {0};
    SmsBufferStats stats = {0};
    SensorData sample = {0};

    atomic_bool stop_requested;
    pthread_t thread;

    SmsConfig config = {
        .interval_ms = 100,
        .temp_threshold = 40.0,
        .hum_threshold = 80.0,
        .duration_sec = 1,
        .verbose = 1
    };

    SensorThreadContext ctx = {
        .buffer = &buffer,
        .config = &config,
        .stop_requested = &stop_requested,
        .use_mock = true
    };

    struct timespec duration = {
        .tv_sec = 0,
        .tv_nsec = 600000000L
    };

    assert(sms_sensor_parse_csv(
        "94537,41.25,48.34\n", &sample) == 0);

    assert(sample.timestamp_ms == 94537);
    assert(sample.temperature > 41.24);
    assert(sample.temperature < 41.26);

    assert(sms_sensor_parse_csv(
        "invalid,csv,data", &sample) != 0);

    assert(sms_buffer_init(
        &buffer, SMS_BUFFER_CAPACITY) == 0);

    atomic_init(&stop_requested, false);

    assert(pthread_create(
        &thread, NULL,
        sms_sensor_thread, &ctx) == 0);

    nanosleep(&duration, NULL);

    atomic_store(&stop_requested, true);

    assert(pthread_join(thread, NULL) == 0);

    sms_buffer_get_stats(&buffer, &stats);

    printf("Produced: %llu\n",
           (unsigned long long)stats.total_produced);

    printf("Read errors: %u\n", ctx.read_errors);

    assert(stats.total_produced >= 3);
    assert(ctx.samples_produced == stats.total_produced);
    assert(ctx.read_errors == 0);

    sms_buffer_stop(&buffer);
    sms_buffer_destroy(&buffer);

    puts("SENSOR THREAD TEST: PASS");

    return 0;
}
