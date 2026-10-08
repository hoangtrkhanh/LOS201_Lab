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
