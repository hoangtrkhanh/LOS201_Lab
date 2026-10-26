/*
 * TASK 03: WRITE COUNTER TEST
 *
 * Verify that:
 * - sms_write() is invoked twice.
 * - Both writes are rejected with EOPNOTSUPP.
 * - The driver can count rejected write requests.
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/sms_sensor"

int main(void)
{
    int fd;
    int i;
    ssize_t ret;
    char data = 'A';

    /* Open the sensor device for writing */
    fd = open(DEVICE_PATH, O_WRONLY);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Make exactly two write() system calls */
    for (i = 0; i < 2; i++) {

        /* Reset errno before the write */
        errno = 0;

        /* Request writing one byte */
        ret = write(fd, &data, 1);

        /* The driver deliberately rejects non-empty writes */
        if (ret == -1 && errno == EOPNOTSUPP) {
            printf("[PASS] write #%d rejected as expected\n",
                   i + 1);
        } else {
            printf("[FAIL] write #%d: ret=%ld errno=%d\n",
                   i + 1, (long)ret, errno);

            close(fd);
            return 1;
        }
    }

    /* Close the device */
    close(fd);

    printf("[PASS] Two write() calls completed\n");

    return 0;
}
