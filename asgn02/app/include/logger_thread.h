#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H

#include <mqueue.h>
#include <stddef.h>
#include <stdint.h>

#include "sms_ipc.h"

#define SMS_DEFAULT_LOG_PATH "/tmp/sms.log"
#define SMS_LOG_ROTATE_BYTES (1024U * 1024U)
#define SMS_LOG_FLUSH_SECONDS 5U

typedef struct {
    mqd_t log_queue;
    const char *log_path;

    size_t rotate_bytes;
    unsigned int flush_seconds;

    uint64_t records_written;
    uint64_t rotations;
    uint64_t flushes;
    uint64_t errors;
} SmsLoggerContext;

void *sms_logger_thread(void *arg);

#endif
