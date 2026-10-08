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
