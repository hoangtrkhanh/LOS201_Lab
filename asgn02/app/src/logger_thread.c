/*
 * File: logger_thread.c
 * Purpose: POSIX queue logger, flush and rotation
 * Author: Replace with your name and student ID
 * Date: 2026-10-20
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "logger_thread.h"

static const char *kind_name(uint32_t kind)
{
    switch (kind) {
    case SMS_LOG_SAMPLE: return "SAMPLE";
    case SMS_LOG_ALERT: return "ALERT";
    case SMS_LOG_SYSTEM: return "SYSTEM";
    default: return "UNKNOWN";
    }
}

static int rotate_log(
    SmsLoggerContext *ctx,
    FILE **fp,
    size_t *current_bytes,
    const char *path)
{
    size_t n = strlen(path);
    char *previous = malloc(n + 3);

    if (previous == NULL)
        return -1;

    snprintf(previous, n + 3, "%s.1", path);

    if (fflush(*fp) != 0) {
        free(previous);
        return -1;
    }

    if (fclose(*fp) != 0) {
        *fp = NULL;
        free(previous);
        return -1;
    }

    *fp = NULL;

    if (rename(path, previous) != 0) {
        free(previous);
        return -1;
    }

    free(previous);

    *fp = fopen(path, "w");

    if (*fp == NULL)
        return -1;

    *current_bytes = 0;
    ctx->rotations++;

    return 0;
}

static int append_record(
    SmsLoggerContext *ctx,
    FILE **fp,
    size_t *current_bytes,
    size_t limit,
    const char *path,
    const SmsLogMessage *msg)
{
    char line[512];
    char stamp[32];
    struct timespec now;
    struct tm tm_local;

    if (clock_gettime(CLOCK_REALTIME, &now) != 0 ||
        localtime_r(&now.tv_sec, &tm_local) == NULL)
        return -1;

    if (strftime(stamp, sizeof(stamp),
                 "%Y-%m-%d %H:%M:%S",
                 &tm_local) == 0)
        return -1;

    int len = snprintf(
        line, sizeof(line),
        "%s.%03ld [%s] [sensor_ms=%" PRIu64 "] %s\n",
        stamp,
        now.tv_nsec / 1000000L,
        kind_name(msg->kind),
        msg->timestamp_ms,
        msg->text
    );

    if (len < 0 || (size_t)len >= sizeof(line))
        return -1;

    if (*current_bytes > 0 &&
        *current_bytes + (size_t)len > limit) {
        if (rotate_log(ctx, fp, current_bytes, path) != 0)
            return -1;
    }

    if (fwrite(line, 1, (size_t)len, *fp) != (size_t)len)
        return -1;

    *current_bytes += (size_t)len;
    ctx->records_written++;

    return 0;
}

void *sms_logger_thread(void *arg)
{
    SmsLoggerContext *ctx = arg;

    if (ctx == NULL)
        return NULL;

    const char *path = ctx->log_path ?
        ctx->log_path : SMS_DEFAULT_LOG_PATH;

    size_t limit = ctx->rotate_bytes ?
        ctx->rotate_bytes : SMS_LOG_ROTATE_BYTES;

    unsigned int period = ctx->flush_seconds ?
        ctx->flush_seconds : SMS_LOG_FLUSH_SECONDS;

    FILE *fp = fopen(path, "a");

    if (fp == NULL) {
        ctx->errors++;
        perror("[LOGGER] fopen");
        return NULL;
    }

    struct stat st;

    if (fstat(fileno(fp), &st) != 0) {
        ctx->errors++;
        fclose(fp);
        return NULL;
    }

    size_t current_bytes = (size_t)st.st_size;

    struct timespec next_flush;

    clock_gettime(CLOCK_MONOTONIC, &next_flush);
    next_flush.tv_sec += period;

    for (;;) {
        struct timespec deadline;

        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += 1;

        SmsLogMessage msg = {0};

        ssize_t received = mq_timedreceive(
            ctx->log_queue,
            (char *)&msg,
            sizeof(msg),
            NULL,
            &deadline
        );

        if (received == (ssize_t)sizeof(msg)) {

            if (msg.kind == SMS_LOG_SHUTDOWN)
                break;

            msg.text[SMS_LOG_TEXT_SIZE - 1] = '\0';

            if (append_record(
                    ctx, &fp, &current_bytes,
                    limit, path, &msg) != 0) {
                ctx->errors++;
                break;
            }

        } else if (received >= 0) {
            ctx->errors++;

        } else if (errno != ETIMEDOUT && errno != EINTR) {
            ctx->errors++;
            perror("[LOGGER] mq_timedreceive");
            break;
        }

        struct timespec now;

        if (clock_gettime(CLOCK_MONOTONIC, &now) == 0 &&
            (now.tv_sec > next_flush.tv_sec ||
            (now.tv_sec == next_flush.tv_sec &&
             now.tv_nsec >= next_flush.tv_nsec))) {

            if (fp != NULL && fflush(fp) != 0) {
                ctx->errors++;
                break;
            }

            ctx->flushes++;
            next_flush = now;
            next_flush.tv_sec += period;
        }
    }

    if (fp != NULL) {
        if (fflush(fp) != 0)
            ctx->errors++;
        else
            ctx->flushes++;

        if (fclose(fp) != 0)
            ctx->errors++;
    }

    return NULL;
}
