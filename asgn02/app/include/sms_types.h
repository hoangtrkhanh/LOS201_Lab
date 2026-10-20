#ifndef SMS_TYPES_H
#define SMS_TYPES_H

#include <stdint.h>
#include <stddef.h>

/* Default application settings */
#define SMS_BUFFER_CAPACITY 64U
#define SMS_APP_DEFAULT_INTERVAL_MS 500U
#define SMS_DEFAULT_TEMP_THRESHOLD 40.0
#define SMS_DEFAULT_HUM_THRESHOLD 80.0
#define SMS_DEFAULT_DURATION_SEC 60U

/* One sample received from the kernel driver */
typedef struct {
    uint64_t timestamp_ms;
    double temperature;
    double humidity;
} SensorData;

/* Command-line configuration */
typedef struct {
    unsigned int interval_ms;
    double temp_threshold;
    double hum_threshold;
    unsigned int duration_sec;
    int verbose;
} SmsConfig;

#endif
