#!/usr/bin/env bash
set -Eeuo pipefail

BASE="$HOME/embedded_lab2"
DATA="$BASE/mtd/jffs2_data"
MOUNT="$BASE/mtd/jffs2_mount"
IMAGE="$BASE/mtd/rootfs.jffs2"
DUMP="$BASE/mtd/nand_dump.bin"

mkdir -p "$DATA" "$MOUNT"

echo "=== LAB02 PART C: MTD / JFFS2 TEST ==="
date

# [LAB02-C1] Kiem tra cac MTD hien co
echo
echo "=== C1: MTD devices before nandsim ==="
cat /proc/mtd

# [LAB02-C2] Tao NAND simulator 32 MiB
echo
echo "=== C2: Load NAND simulator ==="
sudo modprobe nandsim first_id_byte=0x20 second_id_byte=0x35
sudo modprobe mtdblock
sudo modprobe jffs2

cat /proc/mtd

# [SAFETY] Chi chon dung thiet bi NAND simulator
NAND_DEV=""

for NAME_FILE in /sys/class/mtd/mtd[0-9]*/name; do
    [ -f "$NAME_FILE" ] || continue

    if [ "$(cat "$NAME_FILE")" = "NAND simulator partition 0" ]; then
        MTD_ID="$(basename "$(dirname "$NAME_FILE")")"
        NAND_DEV="/dev/$MTD_ID"
        break
    fi
done

if [ -z "$NAND_DEV" ]; then
    echo "ERROR: Khong tim thay NAND simulator!"
    exit 1
fi

# Cam ghi vao BIOS mtd0
if [ "$NAND_DEV" = "/dev/mtd0" ]; then
    echo "SAFETY ERROR: Tu choi ghi vao /dev/mtd0!"
    exit 1
fi

if [ ! -c "$NAND_DEV" ]; then
    echo "ERROR: Thiet bi $NAND_DEV khong ton tai!"
    exit 1
fi

MTD_NUM="${MTD_ID#mtd}"
MTDBLOCK="/dev/mtdblock$MTD_NUM"

if [ ! -b "$MTDBLOCK" ]; then
    echo "ERROR: Thieu $MTDBLOCK"
    exit 1
fi

echo "Safe NAND device: $NAND_DEV"
echo "Safe block device: $MTDBLOCK"

# [LAB02-C3] Tao du lieu va image JFFS2
echo
echo "=== C3: Create JFFS2 image ==="

printf 'Hello from JFFS2!\n' > "$DATA/hello.txt"
printf 'Embedded Linux Lab2\n' > "$DATA/info.txt"

mkfs.jffs2 \
    --root="$DATA" \
    --eraseblock=0x4000 \
    --output="$IMAGE" \
    --pad \
    --no-cleanmarkers

ls -lh "$IMAGE"

# [LAB02-C4] Flash image chi vao NAND simulator
echo
echo "=== C4: Flash JFFS2 image ==="

sudo flash_erase "$NAND_DEV" 0 0
sudo nandwrite -p "$NAND_DEV" "$IMAGE"

# [LAB02-C5] Mount JFFS2 va doc du lieu
echo
echo "=== C5: Mount JFFS2 ==="

sudo mount -t jffs2 "$MTDBLOCK" "$MOUNT"

echo "--- Files after mount ---"
sudo ls -lah "$MOUNT"

echo "--- hello.txt ---"
sudo cat "$MOUNT/hello.txt"

echo "--- info.txt ---"
sudo cat "$MOUNT/info.txt"

# [LAB02-C6] Ghi file kiem tra persistence
echo
echo "=== C6: Write persistence data ==="

printf 'LAB02 JFFS2 persistence OK\n' \
    | sudo tee "$MOUNT/persist.txt" >/dev/null

sync
sudo umount "$MOUNT"

# [LAB02-C7] Mount lai va kiem tra file
echo
echo "=== C7: Remount and verify persistence ==="

sudo mount -t jffs2 "$MTDBLOCK" "$MOUNT"

echo "--- Files after remount ---"
sudo ls -lah "$MOUNT"

echo "--- Read persistence data ---"
READBACK="$(sudo cat "$MOUNT/persist.txt")"
echo "$READBACK"

if [ "$READBACK" != "LAB02 JFFS2 persistence OK" ]; then
    echo "ERROR: Persistence FAILED!"
    sudo umount "$MOUNT"
    exit 1
fi

echo "PERSISTENCE TEST PASSED"

# [LAB02-C8] Xac nhan du lieu goc van con
test "$(sudo cat "$MOUNT/hello.txt")" = "Hello from JFFS2!"
test "$(sudo cat "$MOUNT/info.txt")" = "Embedded Linux Lab2"

echo "ORIGINAL DATA TEST PASSED"

sudo umount "$MOUNT"

# [LAB02-C9] Dump NAND va xem JFFS2 magic
echo
echo "=== C9: NAND dump / JFFS2 magic ==="

sudo nanddump -l 512 -f "$DUMP" "$NAND_DEV"

echo "--- First 32 bytes ---"
od -An -tx1 -N32 "$DUMP"

echo "--- JFFS2 magic expected: 85 19 ---"

# [LAB02-C10] Hoan thanh
echo
echo "=== LAB02 MTD/JFFS2: PASSED ==="
