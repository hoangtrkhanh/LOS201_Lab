# ASSIGNMENT-02 - Smart Monitoring System

Embedded Linux Systems - Linux 5.15 / ARMv7 Cortex-A9

## Part A - Simulated Sensor Driver

### Components
- driver/sms_sensor_driver.c: Kernel driver
- driver/sms_sensor.h: Device and ioctl definitions
- driver/Makefile: ARM kernel module build
- tests/test_ioctl.c: Userspace ioctl test

### Features
- Character device: /dev/sms_sensor
- Procfs: /proc/sms_stats
- Simulated temperature: 15.00 - 45.00 C
- Simulated humidity: 30.00 - 90.00%
- CSV output: timestamp,temperature,humidity
- Configurable sampling interval via ioctl
- Mutex-protected shared state

### Build
Run make arm in the driver directory.
Adjust the Makefile kernel path for your host.

### QEMU Test Results
- Module loading: PASS
- CSV sensor reading: PASS
- Procfs statistics: PASS
- SMS_SET_INTERVAL ioctl: PASS
- Invalid interval rejection: PASS
- EAGAIN before sampling interval: PASS
- Read after sampling interval: PASS

### Status
Part A: Core functionality tested on QEMU ARMv7.
Part B: Multithreaded application pending.
