#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "alert_thread.h"

int main(void)
{
    char alarm_name[64], log_name[64];

    snprintf(alarm_name, sizeof(alarm_name),
             "/sms_alarm_%ld", (long)getpid());

    snprintf(log_name, sizeof(log_name),
             "/sms_log_%ld", (long)getpid());

    struct mq_attr aa = {
        .mq_maxmsg = 5,
        .mq_msgsize = sizeof(SmsAlarmMessage)
    };

    struct mq_attr la = {
        .mq_maxmsg = 5,
        .mq_msgsize = sizeof(SmsLogMessage)
    };

    mqd_t aq = mq_open(alarm_name,
                       O_CREAT | O_EXCL | O_RDWR,
                       0600, &aa);

    if (aq == (mqd_t)-1) {
        perror("mq_open alarm");
        return 1;
    }

    mqd_t lq = mq_open(log_name,
                       O_CREAT | O_EXCL | O_RDWR,
                       0600, &la);

    if (lq == (mqd_t)-1) {
        perror("mq_open log");
        mq_close(aq);
        mq_unlink(alarm_name);
        return 1;
    }

    SmsAlertContext ctx = {
        .alarm_queue = aq,
        .log_queue = lq,
        .forward_logs = true
    };

    pthread_t thread;

    assert(pthread_create(&thread, NULL,
                          sms_alert_thread, &ctx) == 0);

    SmsAlarmMessage alarms[] = {
        {1000, SMS_ALARM_TEMP_HIGH, 42.5, 40.0},
        {2000, SMS_ALARM_HUMID_HIGH, 85.0, 80.0},
        {3000, SMS_ALARM_TEMP_SPIKE, 8.5, 5.0},
        {0, SMS_ALARM_SHUTDOWN, 0, 0}
    };

    for (size_t i = 0; i < 4; i++)
        assert(sms_alert_send(aq, &alarms[i]) == 0);

    assert(pthread_join(thread, NULL) == 0);

    assert(ctx.total_alerts == 3);
    assert(ctx.temp_high_count == 1);
    assert(ctx.humid_high_count == 1);
    assert(ctx.temp_spike_count == 1);
    assert(ctx.receive_errors == 0);
    assert(ctx.forward_errors == 0);

    for (size_t i = 0; i < 3; i++) {
        SmsLogMessage log;

        ssize_t n = mq_receive(lq, (char *)&log,
                               sizeof(log), NULL);

        assert(n == (ssize_t)sizeof(log));
        assert(log.kind == SMS_LOG_ALERT);

        printf("[LOG] %s\n", log.text);
    }

    assert(mq_close(aq) == 0);
    assert(mq_close(lq) == 0);
    assert(mq_unlink(alarm_name) == 0);
    assert(mq_unlink(log_name) == 0);

    puts("ALERT THREAD TEST: PASS");

    return 0;
}
