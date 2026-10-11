#!/bin/sh
set -eu

MODULE=/lib/modules/5.15.0/counter_driver.ko

echo "=== ASGN1 TASK2 TEST BEGIN ==="

# [B3-01] Go module cu neu da load.
if grep -q '^counter_driver ' /proc/modules; then
    rmmod counter_driver
fi

# [B3-02] Nap Misc Driver.
insmod "$MODULE"

lsmod | grep counter_driver

# [B3-03] Tim minor number duoc Kernel cap.
MINOR="$(awk '$2 == "counter" {print $1}' /proc/misc)"

[ -n "$MINOR" ] || {
    echo "ERROR: /proc/misc missing counter"
    exit 1
}

echo "Misc major=10 minor=$MINOR"

# [B3-04] Tao device node neu chua co.
if [ ! -c /dev/counter ]; then
    mknod /dev/counter c 10 "$MINOR"
fi

ls -l /dev/counter

# [B3-05] Xem thong ke truoc khi doc/ghi.
echo "--- PROC BEFORE WRITE ---"
cat /proc/counter_info

# [B3-06] Auto increment on read.
FIRST="$(cat /dev/counter)"
SECOND="$(cat /dev/counter)"
THIRD="$(cat /dev/counter)"

echo "Read 1: $FIRST"
echo "Read 2: $SECOND"
echo "Read 3: $THIRD"

[ "$FIRST" = "0" ] || exit 1
[ "$SECOND" = "1" ] || exit 1
[ "$THIRD" = "2" ] || exit 1

echo "AUTO INCREMENT: PASS"

# [B3-07] Dat counter ve 100.
printf '100\n' > /dev/counter

echo "--- PROC AFTER WRITE 100 ---"
cat /proc/counter_info

VALUE="$(cat /dev/counter)"
echo "After set 100, read: $VALUE"

[ "$VALUE" = "100" ] || exit 1
echo "SET 100: PASS"

# [B3-08] Thu gia tri am, phai bi tu choi.
if printf '%s\n' '-5' > /dev/counter 2>/dev/null; then
    echo "ERROR: negative value accepted"
    exit 1
fi
echo "NEGATIVE VALUE REJECTED: PASS"

# [B3-09] Thu chuoi khong phai so.
if printf 'abc\n' > /dev/counter 2>/dev/null; then
    echo "ERROR: invalid string accepted"
    exit 1
fi
echo "INVALID STRING REJECTED: PASS"

# [B3-10] Reset counter ve 0.
printf '0\n' > /dev/counter

echo "--- PROC AFTER RESET ---"
cat /proc/counter_info

VALUE="$(cat /dev/counter)"
echo "After reset, read: $VALUE"

[ "$VALUE" = "0" ] || exit 1
echo "RESET TO ZERO: PASS"

# [B3-11] Go driver va kiem tra cleanup.
rmmod counter_driver

echo "--- DMESG TASK2 ---"
dmesg | grep counter_driver | tail -30

echo "=== ASGN1 TASK2 TEST PASSED ==="
