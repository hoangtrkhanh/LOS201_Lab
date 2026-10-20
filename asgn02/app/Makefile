# ASSIGNMENT-02 reference Makefile. Inspect/adapt before submission.
HOST_CC := gcc
ARM_CC := arm-linux-gnueabihf-gcc
CPPFLAGS := -D_GNU_SOURCE -Iinclude -I../driver
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror -pthread
LDLIBS := -pthread -lrt -lm
SRC := src/sms_app.c src/sensor_thread.c src/processor_thread.c \
       src/alert_thread.c src/logger_thread.c src/circular_buffer.c
ARM_CPU := -mcpu=cortex-a9 -mfpu=vfpv3-d16 -mfloat-abi=hard
.PHONY: host arm clean
host:
	mkdir -p bin
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o bin/sms_app_x86 $(LDLIBS)
arm:
	mkdir -p bin
	$(ARM_CC) $(CPPFLAGS) $(CFLAGS) $(ARM_CPU) -static $(SRC) -o bin/sms_app_arm $(LDLIBS)
clean:
	rm -f bin/sms_app_x86 bin/sms_app_arm
