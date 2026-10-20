/*
 * File: sensor_thread.c
 * Purpose: Sensor acquisition thread
 * Assignment: ASSIGNMENT-02
 * Author: Dien ten va MSSV
 * Date: 2026-10-20
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "sensor_thread.h"
#include "sms_sensor.h"

/* Parse timestamp, temperature and humidity */
int sms_sensor_parse_csv(
    const char *line,
    SensorData *sample)
{
    unsigned long long timestamp;
    double temperature;
    double humidity;
    int end = 0;

    if (line == NULL || sample == NULL)
        return -EINVAL;

    if (sscanf(line, " %llu,%lf,%lf %n",
               &timestamp,
               &temperature,
               &humidity,
               &end) != 3)
        return -EINVAL;

    if (end == 0 || line[end] != '\0')
        return -EINVAL;

    if (!isfinite(temperature) ||
        !isfinite(humidity))
        return -EINVAL;

    if (temperature < 15.0 || temperature > 45.0 ||
        humidity < 30.0 || humidity > 90.0)
        return -ERANGE;

    sample->timestamp_ms = (uint64_t)timestamp;
    sample->temperature = temperature;
    sample->humidity = humidity;

    return 0;
}

/* Read exactly one sample from the kernel device */
static int sensor_read_device(SensorData *sample)
{
    char line[128];
    ssize_t length;
    int fd;
    int saved_errno;

    fd = open(SMS_DEVICE_PATH, O_RDONLY);

    if (fd < 0)
        return -errno;

    length = read(fd, line, sizeof(line) - 1);

    saved_errno = errno;

    close(fd);

    if (length < 0)
        return -saved_errno;

    if (length == 0)
        return -EIO;

    line[length] = '\0';

    return sms_sensor_parse_csv(line, sample);
}

/* Configure sampling interval inside kernel driver */
static int sensor_configure_driver(unsigned int interval)
{
    int fd;
    int result;
    int saved_errno;

    fd = open(SMS_DEVICE_PATH, O_RDONLY);

    if (fd < 0)
        return -errno;

    result = ioctl(fd, SMS_SET_INTERVAL, &interval);

    saved_errno = errno;

    close(fd);

    if (result < 0)
        return -saved_errno;

    return 0;
}

/* Generate simulated data for host testing */
static int sensor_mock_sample(
    SensorData *sample,
    unsigned int *seed)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return -errno;

    sample->timestamp_ms =
        (uint64_t)now.tv_sec * 1000ULL +
        (uint64_t)now.tv_nsec / 1000000ULL;

    sample->temperature =
        15.0 + (double)(rand_r(seed) % 3001U) / 100.0;

    sample->humidity =
        30.0 + (double)(rand_r(seed) % 6001U) / 100.0;

    return 0;
}


/*
 * Retry transient -EAGAIN without discarding
 * the entire sampling period.
 *
 * Maximum additional wait: 200 ms.
 */
static int sensor_read_with_retry(
    SensorThreadContext *ctx,
    SensorData *sample)
{
    int result = sensor_read_device(sample);

    for (unsigned int i = 0;
         result == -EAGAIN && i < 20U &&
         !atomic_load(ctx->stop_requested);
         ++i) {

        struct timespec pause = {
            .tv_sec = 0,
            .tv_nsec = 10000000L
        };

        ctx->eagain_count++;

        while (nanosleep(&pause, &pause) == -1) {
            if (errno != EINTR)
                return -errno;

            if (atomic_load(ctx->stop_requested))
                return -EINTR;
        }

        if (atomic_load(ctx->stop_requested))
            return -EINTR;

        result = sensor_read_device(sample);
    }

    return result;
}

/* Add milliseconds to an absolute deadline */
static void sensor_add_ms(
    struct timespec *timepoint,
    unsigned int milliseconds)
{
    timepoint->tv_sec += milliseconds / 1000U;

    timepoint->tv_nsec +=
        (long)(milliseconds % 1000U) * 1000000L;

    if (timepoint->tv_nsec >= 1000000000L) {
        timepoint->tv_sec++;
        timepoint->tv_nsec -= 1000000000L;
    }
}

/* Main Sensor Thread */
void *sms_sensor_thread(void *arg)
{
    SensorThreadContext *ctx = arg;
    struct timespec next_wakeup;
    SensorData sample;

    unsigned int seed;
    int result;

    if (ctx == NULL || ctx->buffer == NULL ||
        ctx->config == NULL ||
        ctx->stop_requested == NULL)
        return NULL;

    if (ctx->config->interval_ms == 0) {
        ctx->read_errors++;
        return NULL;
    }

    if (!ctx->use_mock) {
        result = sensor_configure_driver(
            ctx->config->interval_ms
        );

        if (result != 0) {
            fprintf(stderr,
                    "[SENSOR] ioctl failed: %d\n",
                    result);
            ctx->read_errors++;
            atomic_store(ctx->stop_requested, true);
            return NULL;
        }
    }

    seed = (unsigned int)time(NULL);

    if (clock_gettime(
            CLOCK_MONOTONIC,
            &next_wakeup) != 0) {
        ctx->read_errors++;
        atomic_store(ctx->stop_requested, true);
        return NULL;
    }

    unsigned long long missed_deadlines = 0;

    while (!atomic_load(ctx->stop_requested)) {

        if (ctx->use_mock) {
            result = sensor_mock_sample(&sample, &seed);
        } else {
            result = sensor_read_with_retry(ctx, &sample);
        }

        if (result == 0) {

            int push_result =
                sms_buffer_push(ctx->buffer, &sample);

            if (push_result == 0) {
                ctx->samples_produced++;

                if (ctx->config->verbose) {
                    printf(
                        "[SENSOR] %llu,%.2f,%.2f\n",
                        (unsigned long long)
                            sample.timestamp_ms,
                        sample.temperature,
                        sample.humidity
                    );
                    fflush(stdout);
                }

            } else if (push_result == -ENOSPC) {
                ctx->overflow_count++;

                fprintf(stderr,
                        "[SENSOR] buffer full\n");

            } else if (push_result == -ECANCELED) {
                break;

            } else {
                ctx->read_errors++;
            }

        } else if (result == -EAGAIN) {
            ctx->eagain_count++;

        } else if (result == -EINTR) {
            /* Retry on the next sampling cycle */

        } else {
            ctx->read_errors++;

            fprintf(stderr,
                    "[SENSOR] read error: %d\n",
                    result);
        }

        sensor_add_ms(
            &next_wakeup,
            ctx->config->interval_ms
        );

        /*
         * Skip expired absolute deadlines.
         * Keep missed deadlines separate from wake-up jitter.
         */
        {
            struct timespec before_sleep;

            if (clock_gettime(CLOCK_MONOTONIC,
                              &before_sleep) != 0) {
                ctx->read_errors++;
                atomic_store(ctx->stop_requested, true);
                break;
            }

            if (ctx->config->interval_ms == 0) {
                ctx->read_errors++;
                atomic_store(ctx->stop_requested, true);
                break;
            }

            while (
                next_wakeup.tv_sec < before_sleep.tv_sec ||
                (next_wakeup.tv_sec == before_sleep.tv_sec &&
                 next_wakeup.tv_nsec <= before_sleep.tv_nsec)
            ) {
                sensor_add_ms(
                    &next_wakeup,
                    ctx->config->interval_ms
                );

                missed_deadlines++;
            }
        }

        while (!atomic_load(ctx->stop_requested)) {

            result = clock_nanosleep(
                CLOCK_MONOTONIC,
                TIMER_ABSTIME,
                &next_wakeup,
                NULL
            );

            if (result == 0) {
                struct timespec actual;
                if (ctx->jitter_us != NULL &&
                    ctx->jitter_count < ctx->jitter_capacity &&
                    clock_gettime(CLOCK_MONOTONIC, &actual) == 0) {
                    int64_t delta_ns =
                        (int64_t)(actual.tv_sec - next_wakeup.tv_sec) *
                            1000000000LL +
                        (int64_t)(actual.tv_nsec - next_wakeup.tv_nsec);
                    ctx->jitter_us[ctx->jitter_count++] =
                        delta_ns / 1000LL;
                }
                break;
            }

            if (result != EINTR) {
                ctx->read_errors++;
                break;
            }
        }
    }

    printf("SCHED_DIAG missed_deadlines=%llu\n",
           missed_deadlines);

    return NULL;
}
