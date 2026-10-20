#ifndef ALERT_THREAD_H
#define ALERT_THREAD_H

#include <stdbool.h>
#include <stdint.h>
#include <mqueue.h>

#include "sms_ipc.h"

typedef struct {
    mqd_t alarm_queue;
    mqd_t log_queue;

    bool forward_logs;

    uint64_t temp_high_count;
    uint64_t humid_high_count;
    uint64_t temp_spike_count;

    uint64_t total_alerts;
    uint64_t receive_errors;
    uint64_t forward_errors;
} SmsAlertContext;

/* Send an alarm to the POSIX message queue */
int sms_alert_send(
    mqd_t queue,
    const SmsAlarmMessage *message
);

/* Alert Thread entry point */
void *sms_alert_thread(void *arg);

#endif
