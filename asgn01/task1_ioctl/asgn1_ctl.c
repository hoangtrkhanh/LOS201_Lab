#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "asgn1_ioctl.h"

/*
 * main:
 * Arguments: reset | stats | mode 0/1 | version | invalid.
 * Purpose: exercise the ioctl interface from userspace.
 * Return: 0 on success, 1 on failure.
 */
int main(int argc, char **argv)
{
    int fd;
    int result = -1;

    if (argc < 2) {
        fprintf(stderr,
                "Usage: %s reset|stats|mode 0|mode 1|version|invalid\n",
                argv[0]);
        return 1;
    }

    fd = open("/dev/asgn1", O_RDWR);
    if (fd < 0) {
        perror("open /dev/asgn1");
        return 1;
    }

    if (strcmp(argv[1], "reset") == 0) {
        result = ioctl(fd, ASGN1_RESET_BUFFER);
        if (result == 0)
            puts("RESET_BUFFER: OK");

    } else if (strcmp(argv[1], "stats") == 0) {
        struct asgn1_stats stats = {0};

        result = ioctl(fd, ASGN1_GET_STATS, &stats);

        if (result == 0) {
            printf("open_count      : %d\n",
                   stats.open_count);
            printf("write_count     : %d\n",
                   stats.write_count);
            printf("read_count      : %d\n",
                   stats.read_count);
            printf("buffer_len      : %d\n",
                   stats.buffer_len);
            printf("last_write_size : %d\n",
                   stats.last_write_size);
        }

    } else if (strcmp(argv[1], "mode") == 0) {
        int mode;

        if (argc != 3 ||
            (strcmp(argv[2], "0") != 0 &&
             strcmp(argv[2], "1") != 0)) {
            fprintf(stderr, "Mode must be 0 or 1\n");
            close(fd);
            return 1;
        }

        mode = atoi(argv[2]);
        result = ioctl(fd, ASGN1_SET_MODE, &mode);

        if (result == 0)
            printf("SET_MODE: %d OK\n", mode);

    } else if (strcmp(argv[1], "version") == 0) {
        char version[ASGN1_VERSION_SIZE] = {0};

        result = ioctl(fd, ASGN1_GET_VERSION, version);

        if (result == 0)
            printf("GET_VERSION: %s\n", version);

    } else if (strcmp(argv[1], "invalid") == 0) {
        result = ioctl(fd, _IO('Z', 99));

        if (result == -1 && errno == ENOTTY) {
            puts("INVALID_IOCTL: ENOTTY OK");
            close(fd);
            return 0;
        }

        fprintf(stderr,
                "INVALID_IOCTL: expected ENOTTY\n");
        close(fd);
        return 1;

    } else {
        fprintf(stderr, "Unknown command\n");
        close(fd);
        return 1;
    }

    if (result < 0)
        perror("ioctl");

    close(fd);
    return result < 0 ? 1 : 0;
}
