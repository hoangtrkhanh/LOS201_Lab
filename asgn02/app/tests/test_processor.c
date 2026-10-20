#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>

#include "processor_thread.h"

static unsigned int callback_count;

static void test_sink(
    const SensorData *sample,
    const SmsProcessorResult *result,
    void *user)
{
    (void)sample;
    (void)result;
    (void)user;

    callback_count++;
}

int main(void)
{
    SmsConfig config = {
        .interval_ms = 500,
        .temp_threshold = 40.0,
        .hum_threshold = 80.0,
        .duration_sec = 60,
        .verbose = 0
    };

    SmsProcessorState state = {0};
    SmsProcessorResult result = {0};

    SensorData sample = {
        .timestamp_ms = 1000,
        .temperature = 30.0,
        .humidity = 60.0
    };

    SmsBuffer buffer = {0};
    SmsProcessorContext ctx = {0};
    pthread_t thread;

    unsigned int i;

    puts("===== TEST 1: NORMAL SAMPLE =====");

    assert(sms_processor_evaluate(
        &state, &config, &sample, &result) == 0);

    assert(result.alarm_flags == 0);
    assert(result.window_count == 1);
    assert(fabs(result.avg_temperature - 30.0) < 0.001);

    puts("PASS: normal sample");

    puts("===== TEST 2: THREE ALARMS =====");

    sample.temperature = 42.0;
    sample.humidity = 85.0;

    assert(sms_processor_evaluate(
        &state, &config, &sample, &result) == 0);

    assert(
        result.alarm_flags & SMS_ALERT_TEMP_HIGH
    );

    assert(
        result.alarm_flags & SMS_ALERT_HUMID_HIGH
    );

    assert(
        result.alarm_flags & SMS_ALERT_TEMP_SPIKE
    );

    puts("PASS: temperature, humidity and spike");

    puts("===== TEST 3: ROLLING WINDOW =====");

    state = (SmsProcessorState){0};

    for (i = 0; i < 11; i++) {
        sample.temperature = 20.0 + (double)i;
        sample.humidity = 60.0;

        assert(sms_processor_evaluate(
            &state, &config, &sample, &result) == 0);
    }

    assert(result.window_count == 10);
    assert(fabs(result.avg_temperature - 25.5) < 0.001);
    assert(fabs(result.avg_humidity - 60.0) < 0.001);

    puts("PASS: 10-sample rolling average");

    puts("===== TEST 4: THREAD + BUFFER =====");

    assert(sms_buffer_init(&buffer, 64) == 0);

    ctx.buffer = &buffer;
    ctx.config = &config;
    ctx.sink = test_sink;

    assert(pthread_create(
        &thread, NULL,
        sms_processor_thread, &ctx) == 0);

    for (i = 0; i < 3; i++) {
        sample.timestamp_ms = 2000 + i * 500;
        sample.temperature = 25.0 + i;
        sample.humidity = 60.0;

        assert(sms_buffer_push(&buffer, &sample) == 0);
    }

    /* Wake the consumer; it must drain pending samples */
    sms_buffer_stop(&buffer);

    assert(pthread_join(thread, NULL) == 0);

    assert(ctx.processed == 3);
    assert(ctx.errors == 0);
    assert(callback_count == 3);

    sms_buffer_destroy(&buffer);

    puts("PASS: processor thread and circular buffer");
    puts("PROCESSOR THREAD TEST: PASS");

    return 0;
}
