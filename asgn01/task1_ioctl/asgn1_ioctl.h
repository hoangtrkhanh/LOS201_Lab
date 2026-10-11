#ifndef ASGN1_IOCTL_H
#define ASGN1_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define ASGN1_IOC_MAGIC     'A'
#define ASGN1_VERSION_SIZE  64

/* Struct thong ke tra ve cho userspace. */
struct asgn1_stats {
    __s32 open_count;
    __s32 write_count;
    __s32 read_count;
    __s32 buffer_len;
    __s32 last_write_size;
};

/* Command 1: khong co argument. */
#define ASGN1_RESET_BUFFER \
    _IO(ASGN1_IOC_MAGIC, 1)

/* Command 2: kernel -> userspace. */
#define ASGN1_GET_STATS \
    _IOR(ASGN1_IOC_MAGIC, 2, struct asgn1_stats)

/* Command 3: userspace -> kernel. */
#define ASGN1_SET_MODE \
    _IOW(ASGN1_IOC_MAGIC, 3, int)

/* Command 4: kernel -> userspace, buffer 64 bytes. */
#define ASGN1_GET_VERSION \
    _IOR(ASGN1_IOC_MAGIC, 4, char[ASGN1_VERSION_SIZE])

#endif
