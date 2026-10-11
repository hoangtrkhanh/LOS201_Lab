KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15

obj-m += asgn1_driver.o

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) \
		ARCH=arm \
		CROSS_COMPILE=arm-linux-gnueabihf- \
		KCFLAGS="-march=armv7-a" modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) \
		ARCH=arm \
		CROSS_COMPILE=arm-linux-gnueabihf- clean
