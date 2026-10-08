# LAB 01 - Part B: U-Boot

## 1. Objective

Build and test U-Boot for the ARM Cortex-A9 target
using QEMU vexpress-a9.

## 2. Environment

- Bootloader: U-Boot 2022.04
- Target CPU: ARM Cortex-A9
- Board: vexpress-a9
- RAM: 512 MiB
- Cross-Compiler: arm-linux-gnueabihf

## 3. Build Result

U-Boot was successfully compiled.

Output: lab01/output/u-boot
Configuration: lab01/configs/u-boot.config

## 4. QEMU Test

U-Boot started successfully on QEMU.

The following commands were tested:

- version
- bdinfo
- printenv
- flinfo
- setenv
- saveenv

## 5. Environment Configuration

bootargs:
console=ttyAMA0,115200 root=/dev/ram0 rw init=/sbin/init mem=512M

bootcmd:
bootz 0x60100000 0x64000000 0x60000000

The boot addresses were configured for the experiment.
A valid ramdisk image and its size must still be provided
before the boot command can work correctly.

## 6. Problem and Solution

Problem:
saveenv failed with a Flash sector boundary error.

Investigation:
QEMU reported two Flash banks with 128 KiB sectors.

Solution:
Changed CONFIG_ENV_ADDR from 0x47F80000
to 0x45F80000 and rebuilt U-Boot.

Result:
saveenv successfully wrote the environment to Flash.

Persistence across QEMU restarts has not been tested.

## 7. Linux Boot Attempt

The Linux Kernel started but stopped with:

VFS: Unable to mount root fs on unknown-block(0,0)

A usable root filesystem was not available.

## 8. Conclusion

Successfully built and executed U-Boot in QEMU,
configured bootloader environment variables,
and resolved the saveenv Flash address problem.

The next task is to build BusyBox and initramfs,
then complete Linux boot into the shell.
