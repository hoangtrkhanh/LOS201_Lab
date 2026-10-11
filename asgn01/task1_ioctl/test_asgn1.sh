#!/bin/sh
set -eu

MODULE=/lib/modules/5.15.0/asgn1_driver.ko
CTL=/usr/bin/asgn1_ctl

echo "=== ASGN1 TASK1 TEST BEGIN ==="

# [A5-01] Neu module cu dang load, go truoc.
if grep -q '^asgn1_driver ' /proc/modules; then
    rmmod asgn1_driver
fi

# [A5-02] Load driver ARM.
insmod "$MODULE"

# [A5-03] Tao device node neu devtmpfs chua tao.
if [ ! -c /dev/asgn1 ]; then
    mknod /dev/asgn1 c 241 0
fi

lsmod | grep asgn1_driver
ls -l /dev/asgn1

# [A5-04] Kiem tra version command.
"$CTL" version

# [A5-05] Chuyen sang Kernel mode.
"$CTL" mode 0
printf 'hello\n' > /dev/asgn1
RESULT="$(cat /dev/asgn1)"

echo "Kernel mode read: [$RESULT]"
[ "$RESULT" = "[KERNEL]hello" ] || exit 1
echo "KERNEL MODE: PASS"

# [A5-06] Chuyen sang Echo mode.
"$CTL" mode 1
printf 'hello\n' > /dev/asgn1
RESULT="$(cat /dev/asgn1)"

echo "Echo mode read: [$RESULT]"
[ "$RESULT" = "hello" ] || exit 1
echo "ECHO MODE: PASS"

# [A5-07] Lay thong ke.
echo "--- GET_STATS before RESET ---"
"$CTL" stats

# [A5-08] Reset buffer va counters.
"$CTL" reset
RESULT="$(cat /dev/asgn1)"

[ -z "$RESULT" ] || exit 1
echo "RESET EMPTY BUFFER: PASS"

# [A5-09] Xac nhan cac counters ve 0.
echo "--- GET_STATS after RESET ---"
STATS="$("$CTL" stats)"
echo "$STATS"

echo "$STATS" | grep -q 'write_count *: 0'
echo "$STATS" | grep -q 'read_count *: 0'
echo "$STATS" | grep -q 'buffer_len *: 0'

echo "RESET COUNTERS: PASS"

# [A5-10] Invalid ioctl phai tra ENOTTY.
"$CTL" invalid

# [A5-11] Xem procfs/sysfs.
echo "--- PROCFS ---"
cat /proc/asgn1_info

echo "--- SYSFS ---"
cat /sys/class/asgn1_class/asgn1/mode
cat /sys/class/asgn1_class/asgn1/buffer_len

# [A5-12] Unload va kiem tra cleanup.
rmmod asgn1_driver

echo "--- DMESG TASK1 ---"
dmesg | grep asgn1_driver | tail -30

echo "=== ASGN1 TASK1 TEST PASSED ==="
