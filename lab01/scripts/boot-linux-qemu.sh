#!/bin/bash
set -e

OUTPUT="$HOME/embedded_lab1/output"

qemu-system-arm \
  -M vexpress-a9 \
  -cpu cortex-a9 \
  -m 512M \
  -smp 2 \
  -nographic \
  -kernel "$OUTPUT/zImage" \
  -dtb "$OUTPUT/vexpress-v2p-ca9.dtb" \
  -initrd "$OUTPUT/initramfs.cpio.gz" \
  -append 'console=ttyAMA0,115200 rdinit=/sbin/init mem=512M'
