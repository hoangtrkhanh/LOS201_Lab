# LOS201 - Embedded Linux Systems

Repository for Embedded Linux Systems laboratory and individual assignment work.

## Environment

- Host: Ubuntu Linux
- Target: ARMv7 Cortex-A9
- Emulator: QEMU Versatile Express (vexpress-a9)
- Kernel: Linux 5.15
- Toolchain: arm-linux-gnueabihf

## Contents

- [LAB-01 - Kernel Configuration & Embedded Linux Boot](#lab-01)
- [LAB-02 - Device Driver & File System Management](#lab-02)
- [LAB-03 - Debugging & Performance Analysis](#lab-03)
- [ASSIGNMENT-01 - How to Write a Device Driver](#assignment-01)
- [ASSIGNMENT-02 - Sensor Monitoring System](#assignment-02)

---

<a id="lab-01"></a>
## LAB-01 - Kernel Configuration & Embedded Linux Boot

**Scope of the exercise:**

- Cau hinh va cross-compile Linux kernel cho ARM.
- Tim hieu U-Boot, Device Tree Blob va qua trinh boot.
- Xay dung BusyBox root filesystem, initramfs va boot tren QEMU.

**Repository files:**

- `lab01/logs/kernel-panic.txt`
- `lab01/logs/linux-boot-success.txt`
- `lab01/logs/u-boot-environment.txt`
- `lab01/logs/u-boot-startup.txt`
- `lab01/scripts/boot-linux-qemu.sh`

Browse: [lab01/](lab01/)

---

<a id="lab-02"></a>
## LAB-02 - Device Driver & File System Management

**Scope of the exercise:**

- Xay dung va quan ly Linux kernel module.
- Tim hieu character device, read/write, procfs va sysfs.
- Thuc hanh MTD/JFFS2 va BusyBox init.

**Repository files:**

- `lab02/driver/Makefile`
- `lab02/driver/lab2_driver.c`
- `lab02/init-scripts/test_driver.sh`
- `lab02/init-scripts/test_procfs.sh`
- `lab02/logs/boot_log.txt`
- `lab02/logs/driver_test.txt`
- `lab02/logs/mtd_jffs2.txt`
- `lab02/logs/procfs_test.txt`
- `lab02/scripts/run_mtd_test.sh`

Browse: [lab02/](lab02/)

---

<a id="lab-03"></a>
## LAB-03 - Debugging & Performance Analysis

**Scope of the exercise:**

- Debug chuong trinh bang GDB.
- Phan tich loi bo nho bang Valgrind.
- Theo doi system calls bang strace.
- Phan tich CPU performance bang perf.
- Danh gia scheduling latency bang cyclictest.

**Repository files:**

- `lab03/apps/buggy_app.c`
- `lab03/apps/leaky_app.c`
- `lab03/apps/leaky_app_arm_measure.c`
- `lab03/apps/leaky_app_fixed.c`
- `lab03/apps/perf_target.c`
- `lab03/logs/final_check_summary.txt`
- `lab03/logs/gdb_bug1.txt`
- `lab03/logs/gdb_bug2_3.txt`
- `lab03/logs/gdb_session.txt`
- `lab03/logs/hist_standard.txt`
- `lab03/logs/hist_standard_raw.txt`
- `lab03/logs/hist_under_load.txt`
- ... and 21 more files

Browse: [lab03/](lab03/)

---

<a id="assignment-01"></a>
## ASSIGNMENT-01 - How to Write a Device Driver

**Scope of the exercise:**

- Mo rong Linux device driver voi ioctl interface.
- Tim hieu va trien khai misc device driver.
- Phan tich thiet ke va so sanh cac cach dang ky driver.

**Repository files:**

- `asgn01/logs/dmesg_full.txt`
- `asgn01/logs/qemu_session.txt`
- `asgn01/logs/test_asgn1_output.txt`
- `asgn01/logs/test_counter_output.txt`
- `asgn01/task1_ioctl/Makefile`
- `asgn01/task1_ioctl/asgn1_ctl.c`
- `asgn01/task1_ioctl/asgn1_driver.c`
- `asgn01/task1_ioctl/asgn1_ioctl.h`
- `asgn01/task1_ioctl/test_asgn1.sh`
- `asgn01/task2_misc/Makefile`
- `asgn01/task2_misc/counter_driver.c`
- `asgn01/task2_misc/test_counter.sh`

Browse: [asgn01/](asgn01/)

---

<a id="assignment-02"></a>
## ASSIGNMENT-02 - Sensor Monitoring System

**Scope of the exercise:**

- Part A: Mo phong temperature/humidity kernel driver.
- Cung cap /dev/sms_sensor va /proc/sms_stats.
- Dieu khien sampling interval bang SMS_SET_INTERVAL.
- Part B: Sensor, Processor, Alert va Logger threads.
- Tich hop IPC, logging, debugging va real-time evaluation.

**Repository files:**

- `asgn02/driver/Makefile`
- `asgn02/driver/sms_sensor.h`
- `asgn02/driver/sms_sensor_driver.c`
- `asgn02/tests/test_ioctl.c`

Browse: [asgn02/](asgn02/)

### Verified progress

**Part A - Kernel Driver:**

- ARMv7 module build: PASS
- QEMU module loading: PASS
- CSV sensor readings: PASS
- procfs statistics: PASS
- SMS_SET_INTERVAL ioctl: PASS
- Invalid interval rejection: PASS
- Sampling interval enforcement: PASS

**Part B - Multithreaded Application:**

- Development pending.

**Driver build:**

Run `make arm` from `asgn02/driver/`.
The Makefile kernel source path may need adjustment.

---

## Notes

The sections describe the scope of each exercise.
Only explicitly verified results are marked PASS.
Source directories contain the implementation files.

---

## Assignment 02 - Sensor Monitoring System

**Embedded Linux Systems - Capstone Assignment**

**Environment:** Ubuntu Linux, ARMv7 Cortex-A9, QEMU vexpress-a9, Linux Kernel 5.15.

**[View Assignment 02 Source Code](asgn02/)**

### Task 1 - Multi-threaded Application

- Custom Kernel Driver: `/dev/sms_sensor`
- Driver statistics: `/proc/sms_stats`
- ioctl: configurable sensor sampling interval
- Sensor Thread: periodic data acquisition
- Processor Thread: rolling averages and alert detection
- Alert Thread: POSIX message queue
- Logger Thread: timestamped file logging
- Circular Buffer: pthread mutex and condition variable

### Task 2 - Memory Management and Driver Integration

- ARM/QEMU integration test completed
- 117 samples collected in 60 seconds at 500 ms interval
- Valgrind: 0 memory errors and no heap leaks detected
- Driver read counter verified

### Task 3 - Debugging and Profiling

- GDB: multi-thread debugging and backtrace
- Valgrind: memory leak analysis
- strace: system call profiling
- perf: CPU hotspot analysis

### Task 4 - Real-Time Scheduling

Comparison of SCHED_OTHER and SCHED_FIFO with 1,092 jitter samples per mode.

| Metric | SCHED_OTHER | SCHED_FIFO |
|---|---:|---:|
| Priority | 0 | 50 |
| Samples | 1092 | 1092 |
| Min jitter (us) | 359 | 251 |
| Average jitter (us) | 5627.29 | 5497.91 |
| Max jitter (us) | 11315 | 10873 |
| P99 jitter (us) | 10680 | 10495 |
| Missed deadlines | 109 | 108 |

### Task 5 - Technical Report and Demonstration

- System architecture and thread synchronization analysis
- Technical report: pending completion
- Demo video (3-5 minutes): pending completion
- Final source package: pending completion

### Source Code and Test Evidence

- [Application Source](asgn02/app/src/)
- [Application Headers](asgn02/app/include/)
- [Kernel Driver](asgn02/driver/)
- [Unit Tests](asgn02/app/tests/)
- [Debugging and Benchmark Logs](asgn02/logs/)

