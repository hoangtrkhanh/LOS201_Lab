/*
 * File: circular_buffer.c
 * Purpose: Thread-safe circular buffer for sensor data
 * Assignment: ASSIGNMENT-02
 * Author: Dien ten va MSSV cua ban
 * Date: 2026-10-20
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "circular_buffer.h"

/* ==========================================
 * B.2.1 - INITIALIZE
 * ========================================== */

int sms_buffer_init(SmsBuffer *buffer, size_t capacity)
{
    int ret;

    if (buffer == NULL || capacity == 0)
        return -EINVAL;

    memset(buffer, 0, sizeof(*buffer));

    buffer->samples = calloc(capacity, sizeof(SensorData));

    if (buffer->samples == NULL)
        return -ENOMEM;

    buffer->capacity = capacity;

    ret = pthread_mutex_init(&buffer->mutex, NULL);

    if (ret != 0) {
        free(buffer->samples);
        buffer->samples = NULL;
        return -ret;
    }

    ret = pthread_cond_init(&buffer->not_empty, NULL);

    if (ret != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->samples);
        buffer->samples = NULL;
        return -ret;
    }

    return 0;
}

/* ==========================================
 * B.2.2 - PUSH
 * ========================================== */

int sms_buffer_push(
    SmsBuffer *buffer,
    const SensorData *sample)
{
    int ret;

    if (buffer == NULL || sample == NULL ||
        buffer->samples == NULL)
        return -EINVAL;

    ret = pthread_mutex_lock(&buffer->mutex);

    if (ret != 0)
        return -ret;

    if (buffer->stopping) {
        pthread_mutex_unlock(&buffer->mutex);
        return -ECANCELED;
    }

    if (buffer->count == buffer->capacity) {
        buffer->overflow_count++;
        pthread_mutex_unlock(&buffer->mutex);
        return -ENOSPC;
    }

    buffer->samples[buffer->head] = *sample;

    buffer->head =
        (buffer->head + 1) % buffer->capacity;

    buffer->count++;
    buffer->total_produced++;

    pthread_cond_signal(&buffer->not_empty);

    pthread_mutex_unlock(&buffer->mutex);

    return 0;
}

/* ==========================================
 * B.2.3 - POP
 * ========================================== */

int sms_buffer_pop(
    SmsBuffer *buffer,
    SensorData *sample)
{
    int ret;

    if (buffer == NULL || sample == NULL ||
        buffer->samples == NULL)
        return -EINVAL;

    ret = pthread_mutex_lock(&buffer->mutex);

    if (ret != 0)
        return -ret;

    while (buffer->count == 0 && !buffer->stopping) {
        ret = pthread_cond_wait(
            &buffer->not_empty,
            &buffer->mutex
        );

        if (ret != 0) {
            pthread_mutex_unlock(&buffer->mutex);
            return -ret;
        }
    }

    if (buffer->count == 0 && buffer->stopping) {
        pthread_mutex_unlock(&buffer->mutex);
        return -ECANCELED;
    }

    *sample = buffer->samples[buffer->tail];

    buffer->tail =
        (buffer->tail + 1) % buffer->capacity;

    buffer->count--;
    buffer->total_consumed++;

    pthread_mutex_unlock(&buffer->mutex);

    return 0;
}

/* ==========================================
 * B.2.4 - STOP
 * ========================================== */

void sms_buffer_stop(SmsBuffer *buffer)
{
    if (buffer == NULL || buffer->samples == NULL)
        return;

    if (pthread_mutex_lock(&buffer->mutex) != 0)
        return;

    buffer->stopping = true;

    pthread_cond_broadcast(&buffer->not_empty);

    pthread_mutex_unlock(&buffer->mutex);
}

/* ==========================================
 * B.2.5 - GET STATISTICS
 * ========================================== */

void sms_buffer_get_stats(
    SmsBuffer *buffer,
    SmsBufferStats *stats)
{
    if (buffer == NULL || stats == NULL ||
        buffer->samples == NULL)
        return;

    if (pthread_mutex_lock(&buffer->mutex) != 0)
        return;

    stats->current_size = buffer->count;

    stats->total_produced = buffer->total_produced;

    stats->total_consumed = buffer->total_consumed;

    stats->overflow_count = buffer->overflow_count;

    pthread_mutex_unlock(&buffer->mutex);
}

/* ==========================================
 * B.2.6 - DESTROY
 * ========================================== */

void sms_buffer_destroy(SmsBuffer *buffer)
{
    if (buffer == NULL || buffer->samples == NULL)
        return;

    /*
     * All producer and consumer threads must be joined
     * before destroying this buffer.
     */

    pthread_cond_destroy(&buffer->not_empty);

    pthread_mutex_destroy(&buffer->mutex);

    free(buffer->samples);

    buffer->samples = NULL;
    buffer->capacity = 0;
    buffer->count = 0;
    buffer->head = 0;
    buffer->tail = 0;
}

