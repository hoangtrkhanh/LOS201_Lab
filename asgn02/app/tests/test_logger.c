#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger_thread.h"

int main(void)
{
    char qname[64], path[128], rotated[132];

    snprintf(qname, sizeof(qname),
             "/sms_logger_test_%ld", (long)getpid());

    snprintf(path, sizeof(path),
             "/tmp/sms_logger_test_%ld.log", (long)getpid());

    snprintf(rotated, sizeof(rotated), "%s.1", path);

    struct mq_attr attrs = {
        .mq_maxmsg = 8,
        .mq_msgsize = sizeof(SmsLogMessage)
    };

    mqd_t q = mq_open(
        qname, O_CREAT | O_EXCL | O_RDWR,
        0600, &attrs
    );

    if (q == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    SmsLoggerContext ctx = {
        .log_queue = q,
        .log_path = path,
        .rotate_bytes = 450,
        .flush_seconds = 5
    };

    pthread_t tid;

    assert(pthread_create(
        &tid, NULL, sms_logger_thread, &ctx) == 0);

    SmsLogMessage msg = {
        .timestamp_ms = 12345,
        .kind = SMS_LOG_SYSTEM
    };

    snprintf(msg.text, sizeof(msg.text),
             "startup marker");

    assert(mq_send(q, (const char *)&msg,
                   sizeof(msg), 0) == 0);

    /* Verify periodic flush before shutdown */
    sleep(6);

    FILE *fp = fopen(path, "r");
    assert(fp != NULL);

    char buffer[4096];

    size_t n = fread(
        buffer, 1, sizeof(buffer) - 1, fp
    );

    buffer[n] = '\0';
    fclose(fp);

    assert(strstr(buffer, "startup marker") != NULL);

    puts("PERIODIC FLUSH: PASS");

    /* Write enough messages to force log rotation */
    for (unsigned int i = 0; i < 12; i++) {
        msg.timestamp_ms = 20000 + i;
        msg.kind = SMS_LOG_SAMPLE;

        snprintf(
            msg.text, sizeof(msg.text),
            "sample=%u temperature=32.50 "
            "humidity=65.10 "
            "padding=XXXXXXXXXXXXXXXXXXXX",
            i
        );

        assert(mq_send(
            q, (const char *)&msg,
            sizeof(msg), 0) == 0);
    }

    msg.kind = SMS_LOG_SHUTDOWN;

    assert(mq_send(
        q, (const char *)&msg,
        sizeof(msg), 0) == 0);

    assert(pthread_join(tid, NULL) == 0);

    assert(ctx.records_written == 13);
    assert(ctx.rotations >= 1);
    assert(ctx.errors == 0);

    assert(access(path, F_OK) == 0);
    assert(access(rotated, F_OK) == 0);

    printf(
        "Records: %llu  Rotations: %llu  "
        "Flushes: %llu  Errors: %llu\n",
        (unsigned long long)ctx.records_written,
        (unsigned long long)ctx.rotations,
        (unsigned long long)ctx.flushes,
        (unsigned long long)ctx.errors
    );

    assert(mq_close(q) == 0);
    assert(mq_unlink(qname) == 0);

    unlink(path);
    unlink(rotated);

    puts("LOGGER THREAD TEST: PASS");

    return 0;
}
