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
