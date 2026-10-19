# Reference build scaffold, NOT a working driver source.
KDIR ?= $(HOME)/embedded_lab1/kernel/linux-5.15
obj-m += sms_sensor_driver.o
.PHONY: arm clean
arm:
	$(MAKE) -C $(KDIR) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(CURDIR) modules
clean:
	$(MAKE) -C $(KDIR) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(CURDIR) clean
