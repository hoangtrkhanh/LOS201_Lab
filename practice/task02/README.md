# Task 02 – Read Error Counter

## 1. Objective

Extend the Assignment 02 sensor driver with a new counter named `read_err_cnt`.

The counter tracks failed read callbacks that return a negative error code.

## 2. Base project

- Assignment 02 – Task 1
- Previous exercise: Task 01 – `read_cnt`
- Driver: `asgn02/driver/sms_sensor_driver.c`
- Target: ARMv7 QEMU, Linux Kernel 5.15

## 3. Implementation

- Add `static unsigned int read_err_cnt;`.
- Increment the counter when `sms_read()` returns `-EAGAIN`, `-EINVAL`, or `-EFAULT`.
- Protect the counter using the existing `sms_lock` mutex.
- Display the counter in `/proc/sms_stats`.
- Preserve the original `read_count` and Task 01 `read_cnt` behavior.

## 4. Expected behavior

| Read result | read_cnt | read_count | read_err_cnt |
|---|---:|---:|---:|
| Successful read | +1 | +1 | +0 |
| EOF | +1 | +0 | +0 |
| Negative error | +1 | +0 | +1 |

## 5. Verification

Run the driver in ARMv7 QEMU and verify:

- Invalid buffer increments `read_err_cnt`.
- Successful read does not increment `read_err_cnt`.
- EOF does not increment `read_err_cnt`.
- Task 01 sysfs attribute remains functional.
- Kernel module unload removes its registered interfaces.

Record actual test results separately after verification.
