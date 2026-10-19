#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "sms_sensor.h"

int main(void)
{
    int fd;
    unsigned int interval = 3000;

    fd = open("/dev/sms_sensor", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, SMS_SET_INTERVAL, &interval) < 0) {
        perror("SMS_SET_INTERVAL");
        close(fd);
        return 1;
    }

    printf("IOCTL SUCCESS: interval = %u ms\n", interval);

    interval = 0;
    if (ioctl(fd, SMS_SET_INTERVAL, &interval) == -1 &&
        errno == EINVAL) {
        printf("INVALID INTERVAL: PASS\n");
    } else {
        printf("INVALID INTERVAL: FAIL\n");
    }

    close(fd);
    return 0;
}
