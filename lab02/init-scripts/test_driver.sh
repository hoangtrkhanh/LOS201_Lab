#!/bin/sh

echo '=== LAB-02: Device Driver Test ==='

# [1] Load module
echo '[1] Loading lab2_driver...'

# [HO TRO] Khong load lai neu module da duoc auto-load boi rcS
if ! grep -q '^lab2_driver ' /proc/modules; then
    insmod /lib/modules/5.15.0/lab2_driver.ko
    if [ $? -ne 0 ]; then
        echo 'ERROR: insmod failed!'
        exit 1
    fi
fi

# [2] Confirm loaded module
echo '[2] Loaded modules:'
lsmod | grep lab2

# [3] Create device node
echo '[3] Creating device node...'
mknod /dev/lab2 c 240 0 2>/dev/null || \
    echo '(node already exists)'
ls -la /dev/lab2

# [4] WRITE
echo '[4] Writing to device...'
echo 'Hello from userspace, LAB-02!' > /dev/lab2

# [5] READ
echo '[5] Reading from device...'
DATA=$(cat /dev/lab2)
echo "Read back: [$DATA]"

# [6] Multiple write/read tests
echo '[6] Multiple write/read test...'
for i in 1 2 3; do
    echo "Message_$i" > /dev/lab2
    cat /dev/lab2
done

# [7] Kernel log
echo '[7] Kernel messages (dmesg):'
dmesg | grep lab2 | tail -10

echo '=== Test PASSED ==='
