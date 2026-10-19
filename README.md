# LOS201 - Embedded Linux Labs

## LAB 01 - Kernel and Boot Process

Target: ARM Cortex-A9 / QEMU vexpress-a9

### Part A - Linux Kernel
Status: Completed

- Linux Kernel: 5.15
- Architecture: ARMv7-A
- Cross-Compiler: arm-linux-gnueabihf
- Kernel image: zImage
- Device Tree: vexpress-v2p-ca9.dtb
- Successful build flags: KCFLAGS="-march=armv7-a"

### Part B - U-Boot
Status: Completed - U-Boot built, environment saved, expected Kernel Panic recorded

### Part C - BusyBox and initramfs
Status: Completed - BusyBox initramfs booted successfully on QEMU

## LAB 02 - Device Driver and File System

Target: ARM Cortex-A9 / QEMU vexpress-a9

### Part A - Character Device Driver
Status: Completed - ARM kernel module built and read/write tested.

### Part B - procfs and sysfs
Status: Completed - /proc/lab2_info and sysfs attributes tested.

### Part C - MTD and JFFS2
Status: Completed - NAND simulator, JFFS2 and persistence tested on Ubuntu Host.

### Part D - BusyBox Auto-load
Status: Completed - module auto-loaded successfully during QEMU boot.

## ASSIGNMENT 01 - Linux Kernel Drivers

Target: ARM Cortex-A9 / QEMU vexpress-a9  
Kernel: Linux 5.15  
Architecture: ARMv7-A  
Cross-compiler: `arm-linux-gnueabihf-`

This section documents the two driver implementation tasks and their QEMU test evidence.

### Task 1 - IOCTL Character Device Driver

Status: Implemented; the committed QEMU test log reports **TASK1 TEST PASSED**.

Files:
- Driver: [asgn1_driver.c](asgn01/task1_ioctl/asgn1_driver.c)
- IOCTL definitions: [asgn1_ioctl.h](asgn01/task1_ioctl/asgn1_ioctl.h)
- User-space test utility source: [asgn1_ctl.c](asgn01/task1_ioctl/asgn1_ctl.c)
- User-space utility binary: [asgn1_ctl](asgn01/task1_ioctl/asgn1_ctl)
- Kernel module Makefile: [Makefile](asgn01/task1_ioctl/Makefile)
- QEMU test script: [test_asgn1.sh](asgn01/task1_ioctl/test_asgn1.sh)

Implemented and tested features:
- Character device at `/dev/asgn1`.
- IOCTL commands: `RESET_BUFFER`, `GET_STATS`, `SET_MODE`, and `GET_VERSION`.
- Kernel mode and echo mode.
- Buffer/statistics reset and statistics retrieval.
- Invalid IOCTL handling with `ENOTTY`.
- Driver information through procfs and mode/buffer information through sysfs.

Evidence:
- [Task 1 test output](asgn01/logs/test_asgn1_output.txt)
- [Full QEMU session](asgn01/logs/qemu_session.txt)
- [Kernel dmesg log](asgn01/logs/dmesg_full.txt)

### Task 2 - Misc Counter Driver

Status: Implemented; the committed QEMU test log reports **TASK2 TEST PASSED**.

Files:
- Driver: [counter_driver.c](asgn01/task2_misc/counter_driver.c)
- Kernel module Makefile: [Makefile](asgn01/task2_misc/Makefile)
- QEMU test script: [test_counter.sh](asgn01/task2_misc/test_counter.sh)

Implemented and tested features:
- Misc device registered with dynamic minor number and major number 10.
- Device node: `/dev/counter`.
- Reading returns the current counter value and increments it.
- Writing sets the counter to a non-negative integer.
- Negative values and invalid non-numeric input are rejected.
- Counter value and read/write totals are exposed through `/proc/counter_info`.

Evidence:
- [Task 2 test output](asgn01/logs/test_counter_output.txt)
- [Full QEMU session](asgn01/logs/qemu_session.txt)
- [Kernel dmesg log](asgn01/logs/dmesg_full.txt)

### Build the kernel modules

The Makefiles expect the Linux 5.15 kernel tree at `$HOME/embedded_lab1/kernel/linux-5.15` and the ARM cross-compiler to be installed.

```bash
# Build Task 1 IOCTL driver
cd ~/LOS201_Lab/asgn01/task1_ioctl
make

# Build Task 2 misc counter driver
cd ~/LOS201_Lab/asgn01/task2_misc
make
```

Run the test scripts in the configured QEMU/initramfs test environment; they expect the modules and test utility at the paths specified inside each script.

### Test summary

| Task | Recorded result | Evidence |
|---|---|---|
| Task 1 - IOCTL driver | PASS | [test_asgn1_output.txt](asgn01/logs/test_asgn1_output.txt) |
| Task 2 - Misc counter | PASS | [test_counter_output.txt](asgn01/logs/test_counter_output.txt) |
| Combined QEMU run | Both tasks exited with status 0 | [qemu_session.txt](asgn01/logs/qemu_session.txt) |

> **Note:** The current Task 1 version string contains the placeholder `MSSV`, and both driver sources contain placeholder author metadata (`HoTen - MSSV`). Replace these values with your own details if your course requires them.


## LAB 03 - Debugging and Performance Analysis

**Status:** Completed

### Objectives
- Remote debugging using GDB and gdbserver.
- Memory leak detection using Valgrind.
- System call analysis using strace.
- CPU profiling using perf and FlameGraph.
- Real-time latency measurement using cyclictest.

### Results
- Identified 3 bugs using GDB.
- Fixed 16,900 bytes of memory leaks.
- Successfully analyzed system calls on ARMv7 QEMU.
- Identified heavy_compute() as a CPU hotspot.
- Generated latency histograms.

### Files
- [Source Code](./lab03/apps/)
- [Logs](./lab03/logs/)
- [ARM initramfs](./lab03/output/)
- [Full LAB03](./lab03/)

---

---

## ASSIGNMENT-02 – Smart Monitoring System

### Part A – Simulated Sensor Driver

**Environment**
- Linux Kernel 5.15
- ARMv7 Cortex-A9
- QEMU Versatile Express

**Source files**
- `asgn02/driver/sms_sensor_driver.c` – Kernel driver
- `asgn02/driver/sms_sensor.h` – Device and ioctl definitions
- `asgn02/driver/Makefile` – ARM module build
- `asgn02/tests/test_ioctl.c` – IOCTL verification

**Implemented features**
- Character device `/dev/sms_sensor`
- CSV sensor readings
- Simulated temperature and humidity
- `/proc/sms_stats` statistics
- `SMS_SET_INTERVAL` ioctl
- Mutex protection and sampling interval control

**Testing results**
- Kernel module loading: PASS
- CSV sensor readings: PASS
- Procfs statistics: PASS
- IOCTL interval configuration: PASS
- Invalid interval rejection: PASS
- EAGAIN before minimum interval: PASS
- Reading after interval: PASS

**Build**

From the repository root, run `cd asgn02/driver` followed by `make arm`. Adjust the kernel source path in the Makefile for your environment.

**Progress**
- Part A: Core driver functionality tested on QEMU
- Part B: Multithreaded userspace application – pending
