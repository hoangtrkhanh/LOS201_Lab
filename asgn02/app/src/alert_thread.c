/*
 * File: alert_thread.c
 * Purpose: POSIX message queue alarm handling
 * Author: Fill in your name and student ID
 * Date: 2026-10-20
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "alert_thread.h"

static const char *alarm_name(uint32_t type)
{
    switch (type) {
    case SMS_ALARM_TEMP_HIGH: return "TEMP_HIGH";
    case SMS_ALARM_HUMID_HIGH: return "HUMID_HIGH";
    case SMS_ALARM_TEMP_SPIKE: return "TEMP_SPIKE";
    default: return NULL;
    }
}

int sms_alert_send(mqd_t queue, const SmsAlarmMessage *message)
{
    if (message == NULL) return -EINVAL;

    int ret;
    do {
        ret = mq_send(queue, (const char *)message,
                      sizeof(*message), 0);
    } while (ret == -1 && errno == EINTR);

    return ret == 0 ? 0 : -errno;
}

static int forward_log(mqd_t queue,
                       const SmsAlarmMessage *alarm,
                       const char *name)
{
    SmsLogMessage log = {0};
    struct timespec deadline;

    log.timestamp_ms = alarm->timestamp_ms;
    log.kind = SMS_LOG_ALERT;

    snprintf(log.text, sizeof(log.text),
             "TYPE: %s VALUE: %.2f THRESHOLD: %.2f",
             name, alarm->value, alarm->threshold);

    if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
        return -errno;

    deadline.tv_nsec += 300000000L;

    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    int ret;
    do {
        ret = mq_timedsend(queue, (const char *)&log,
                           sizeof(log), 0, &deadline);
    } while (ret == -1 && errno == EINTR);

    return ret == 0 ? 0 : -errno;
}

void *sms_alert_thread(void *arg)
{
    SmsAlertContext *ctx = arg;

    if (ctx == NULL) return NULL;

    for (;;) {
        SmsAlarmMessage alarm;

        ssize_t n = mq_receive(ctx->alarm_queue,
                               (char *)&alarm,
                               sizeof(alarm), NULL);

        if (n < 0) {
            if (errno == EINTR) continue;
            ctx->receive_errors++;
            perror("[ALERT] mq_receive");
            break;
        }

        if ((size_t)n != sizeof(alarm)) {
            ctx->receive_errors++;
            continue;
        }

        if (alarm.type == SMS_ALARM_SHUTDOWN)
            break;

        const char *name = alarm_name(alarm.type);

        if (name == NULL) {
            ctx->receive_errors++;
            continue;
        }

        if (alarm.type == SMS_ALARM_TEMP_HIGH)
            ctx->temp_high_count++;

        if (alarm.type == SMS_ALARM_HUMID_HIGH)
            ctx->humid_high_count++;

        if (alarm.type == SMS_ALARM_TEMP_SPIKE)
            ctx->temp_spike_count++;

        ctx->total_alerts++;

        fprintf(stderr,
                "[ALERT][%" PRIu64 "] TYPE: %s "
                "VALUE: %.2f THRESHOLD: %.2f\n",
                alarm.timestamp_ms, name,
                alarm.value, alarm.threshold);

        if (ctx->forward_logs &&
            forward_log(ctx->log_queue, &alarm, name) != 0)
            ctx->forward_errors++;
    }

    return NULL;
}
