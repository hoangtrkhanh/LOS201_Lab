#!/bin/sh

echo '=== Test procfs & sysfs ==='

# [HO TRO] Chi load module neu chua duoc nap
if ! grep -q '^lab2_driver ' /proc/modules; then
    insmod /lib/modules/5.15.0/lab2_driver.ko || exit 1
fi

# Create device node if needed
mknod /dev/lab2 c 240 0 2>/dev/null || true

# Test procfs before writing
echo '--- /proc/lab2_info (before write) ---'
cat /proc/lab2_info

# Write data to character device
echo 'Embedded Linux Lab2 Test Data' > /dev/lab2

# Test procfs after writing
echo '--- /proc/lab2_info (after write) ---'
cat /proc/lab2_info

# Test sysfs
SYSFS=/sys/class/lab2_class/lab2

echo '--- sysfs attributes ---'
echo "buffer_len : $(cat "$SYSFS/buffer_len")"
echo "open_count : $(cat "$SYSFS/open_count")"
echo "last_data : $(cat "$SYSFS/last_data")"

echo '=== procfs/sysfs Test PASSED ==='
