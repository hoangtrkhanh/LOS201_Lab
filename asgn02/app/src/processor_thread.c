/*
 * File: processor_thread.c
 * Purpose: Process sensor samples and detect alarms.
 * Author: Fill in your name and student ID.
 * Date: 2026-10-20
 */

#include <errno.h>
#include <math.h>
#include <string.h>

#include "processor_thread.h"

/*
 * Process one sample:
 * - Update rolling window
 * - Calculate averages
 * - Detect high temperature, humidity and spikes
 */
int sms_processor_evaluate(
    SmsProcessorState *state,
    const SmsConfig *config,
    const SensorData *sample,
    SmsProcessorResult *result)
{
    unsigned int flags = 0;
    double change = 0.0;
    size_t index;

    if (state == NULL || config == NULL ||
        sample == NULL || result == NULL)
        return -EINVAL;

    if (!isfinite(sample->temperature) ||
        !isfinite(sample->humidity))
        return -EINVAL;

    /* Compare with previous sample */
    if (state->has_previous) {
        change = fabs(
            sample->temperature -
            state->previous_temperature
        );

        if (change > SMS_TEMP_SPIKE_THRESHOLD)
            flags |= SMS_ALERT_TEMP_SPIKE;
    }

    if (sample->temperature > config->temp_threshold)
        flags |= SMS_ALERT_TEMP_HIGH;

    if (sample->humidity > config->hum_threshold)
        flags |= SMS_ALERT_HUMID_HIGH;

    /* Insert sample into rolling window */
    index = state->next;

    if (state->count == SMS_ROLLING_WINDOW) {
        /* Remove the oldest value from running sums */
        state->temp_sum -= state->temperatures[index];
        state->hum_sum -= state->humidities[index];
    } else {
        state->count++;
    }

    state->temperatures[index] = sample->temperature;
    state->humidities[index] = sample->humidity;

    state->temp_sum += sample->temperature;
    state->hum_sum += sample->humidity;

    state->next =
        (index + 1U) % SMS_ROLLING_WINDOW;

    state->previous_temperature = sample->temperature;
    state->has_previous = true;

    /* Return the processing result */
    result->avg_temperature =
        state->temp_sum / (double)state->count;

    result->avg_humidity =
        state->hum_sum / (double)state->count;

    result->temperature_change = change;
    result->window_count = state->count;
    result->alarm_flags = flags;

    return 0;
}

/*
 * Processor Thread:
 * - Wait for samples in circular buffer
 * - Evaluate every sample
 * - Forward results through a callback
 */
void *sms_processor_thread(void *arg)
{
    SmsProcessorContext *ctx = arg;
    SmsProcessorState state = {0};

    SensorData sample;
    SmsProcessorResult result;

    int ret;

    if (ctx == NULL ||
        ctx->buffer == NULL ||
        ctx->config == NULL)
        return NULL;

    for (;;) {
        ret = sms_buffer_pop(ctx->buffer, &sample);

        /* The buffer has stopped and is empty */
        if (ret == -ECANCELED)
            break;

        if (ret != 0) {
            ctx->errors++;
            break;
        }

        ret = sms_processor_evaluate(
            &state,
            ctx->config,
            &sample,
            &result
        );

        if (ret != 0) {
            ctx->errors++;
            continue;
        }

        ctx->processed++;

        if (result.alarm_flags & SMS_ALERT_TEMP_HIGH)
            ctx->alarm_events++;

        if (result.alarm_flags & SMS_ALERT_HUMID_HIGH)
            ctx->alarm_events++;

        if (result.alarm_flags & SMS_ALERT_TEMP_SPIKE)
            ctx->alarm_events++;

        /*
         * B.5/B.6 will provide a sink that sends
         * alarm and log messages through POSIX queues.
         */
        if (ctx->sink != NULL) {
            ctx->sink(
                &sample,
                &result,
                ctx->sink_user
            );
        }
    }

    return NULL;
}
