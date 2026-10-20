#ifndef SMS_IPC_H
#define SMS_IPC_H

#include <stdint.h>

#define SMS_LOG_TEXT_SIZE 192

/* Alarm message types */
typedef enum {
    SMS_ALARM_TEMP_HIGH = 1,
    SMS_ALARM_HUMID_HIGH = 2,
    SMS_ALARM_TEMP_SPIKE = 3,
    SMS_ALARM_SHUTDOWN = 255
} SmsAlarmType;

/* Log message types */
typedef enum {
    SMS_LOG_SAMPLE = 1,
    SMS_LOG_ALERT = 2,
    SMS_LOG_SYSTEM = 3,
    SMS_LOG_SHUTDOWN = 255
} SmsLogType;

/* Message sent to Alert Thread */
typedef struct {
    uint64_t timestamp_ms;
    uint32_t type;
    double value;
    double threshold;
} SmsAlarmMessage;

/* Message sent to Logger Thread */
typedef struct {
    uint64_t timestamp_ms;
    uint32_t kind;
    char text[SMS_LOG_TEXT_SIZE];
} SmsLogMessage;

#endif
