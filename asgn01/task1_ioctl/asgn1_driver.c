#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/errno.h>

#include "asgn1_ioctl.h"

#define DEVICE_NAME   "asgn1"
#define CLASS_NAME    "asgn1_class"
#define DEVICE_MAJOR  241
#define BUFFER_SIZE   1024
#define KERNEL_PREFIX "[KERNEL]"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TODO: HoTen - MSSV");
MODULE_DESCRIPTION("Assignment 01 ioctl character device");
MODULE_VERSION("1.0");

/* Data layer */
static char device_buffer[BUFFER_SIZE];
static int buffer_len;
static int open_count;
static int write_count;
static int read_count;
static int last_write_size;
static int device_mode = 1;

static DEFINE_MUTEX(asgn1_lock);

static struct class *asgn1_class;
static struct device *asgn1_device;
static struct proc_dir_entry *asgn1_proc;

/*
 * Purpose: count successful device opens.
 * Parameters: inode and file.
 * Return: 0.
 */
static int asgn1_open(struct inode *inode,
                      struct file *file)
{
    mutex_lock(&asgn1_lock);
    open_count++;
    mutex_unlock(&asgn1_lock);

    pr_info("asgn1_driver: device opened\n");
    return 0;
}

/*
 * Purpose: log device close.
 * Parameters: inode and file.
 * Return: 0.
 */
static int asgn1_release(struct inode *inode,
                         struct file *file)
{
    pr_info("asgn1_driver: device closed\n");
    return 0;
}

/*
 * Purpose: transfer stored data to userspace.
 * Parameters: file, userspace buffer, count, offset.
 * Return: number of bytes, EOF=0, or negative errno.
 */
static ssize_t asgn1_read(struct file *file,
                          char __user *buf,
                          size_t count,
                          loff_t *ppos)
{
    size_t available;
    ssize_t result;

    if (*ppos < 0)
        return -EINVAL;

    mutex_lock(&asgn1_lock);

    if (*ppos >= buffer_len) {
        result = 0;
        goto out;
    }

    available = buffer_len - *ppos;
    count = min(count, available);

    if (copy_to_user(buf, device_buffer + *ppos, count)) {
        result = -EFAULT;
        goto out;
    }

    *ppos += count;
    read_count++;
    result = count;

out:
    mutex_unlock(&asgn1_lock);

    if (result > 0)
        pr_info("asgn1_driver: read %zd bytes\n", result);

    return result;
}

/*
 * Purpose: store userspace data in Echo or Kernel mode.
 * Parameters: file, input buffer, byte count, offset.
 * Return: bytes accepted or negative errno.
 */
static ssize_t asgn1_write(struct file *file,
                           const char __user *buf,
                           size_t count,
                           loff_t *ppos)
{
    size_t prefix_len = 0;
    size_t accepted;
    int current_mode;

    if (count == 0)
        return 0;

    mutex_lock(&asgn1_lock);

    current_mode = device_mode;

    if (current_mode == 0)
        prefix_len = strlen(KERNEL_PREFIX);

    accepted = min(count,
                   (size_t)(BUFFER_SIZE - 1) - prefix_len);

    memset(device_buffer, 0, sizeof(device_buffer));

    if (prefix_len)
        memcpy(device_buffer, KERNEL_PREFIX, prefix_len);

    if (copy_from_user(device_buffer + prefix_len,
                       buf, accepted)) {
        mutex_unlock(&asgn1_lock);
        return -EFAULT;
    }

    buffer_len = prefix_len + accepted;
    device_buffer[buffer_len] = '\0';

    write_count++;
    last_write_size = accepted;

    mutex_unlock(&asgn1_lock);

    pr_info("asgn1_driver: write %zu bytes mode=%d\n",
            accepted, current_mode);

    return accepted;
}

/*
 * Purpose: implement four required ioctl commands.
 * Parameters: file, command, userspace argument.
 * Return: 0, -EINVAL, -EFAULT or -ENOTTY.
 */
static long asgn1_ioctl(struct file *file,
                        unsigned int cmd,
                        unsigned long arg)
{
    struct asgn1_stats stats;
    int new_mode;
    char version[ASGN1_VERSION_SIZE] =
        "asgn1_driver v1.0 - MSSV";

    switch (cmd) {

    case ASGN1_RESET_BUFFER:
        mutex_lock(&asgn1_lock);

        memset(device_buffer, 0, sizeof(device_buffer));
        buffer_len = 0;
        write_count = 0;
        read_count = 0;
        last_write_size = 0;

        mutex_unlock(&asgn1_lock);

        pr_info("asgn1_driver: ioctl RESET_BUFFER\n");
        return 0;

    case ASGN1_GET_STATS:
        mutex_lock(&asgn1_lock);

        stats.open_count = open_count;
        stats.write_count = write_count;
        stats.read_count = read_count;
        stats.buffer_len = buffer_len;
        stats.last_write_size = last_write_size;

        mutex_unlock(&asgn1_lock);

        if (copy_to_user((void __user *)arg,
                         &stats, sizeof(stats)))
            return -EFAULT;

        pr_info("asgn1_driver: ioctl GET_STATS\n");
        return 0;

    case ASGN1_SET_MODE:
        if (copy_from_user(&new_mode,
                           (void __user *)arg,
                           sizeof(new_mode)))
            return -EFAULT;

        if (new_mode != 0 && new_mode != 1)
            return -EINVAL;

        mutex_lock(&asgn1_lock);
        device_mode = new_mode;
        mutex_unlock(&asgn1_lock);

        pr_info("asgn1_driver: ioctl SET_MODE=%d\n",
                new_mode);
        return 0;

    case ASGN1_GET_VERSION:
        if (copy_to_user((void __user *)arg,
                         version, sizeof(version)))
            return -EFAULT;

        pr_info("asgn1_driver: ioctl GET_VERSION\n");
        return 0;

    default:
        pr_warn("asgn1_driver: invalid ioctl 0x%x\n", cmd);
        return -ENOTTY;
    }
}

/*
 * Purpose: display driver statistics through procfs.
 * Parameters: seq_file and callback context.
 * Return: 0.
 */
static int asgn1_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&asgn1_lock);

    seq_printf(m, "Driver       : asgn1_driver\n");
    seq_printf(m, "Major        : %d\n", DEVICE_MAJOR);
    seq_printf(m, "Mode         : %d\n", device_mode);
    seq_printf(m, "Buffer length: %d\n", buffer_len);
    seq_printf(m, "Open count   : %d\n", open_count);
    seq_printf(m, "Write count  : %d\n", write_count);
    seq_printf(m, "Read count   : %d\n", read_count);

    mutex_unlock(&asgn1_lock);
    return 0;
}

/*
 * Purpose: open procfs seq_file.
 * Parameters: inode and file.
 * Return: single_open result.
 */
static int asgn1_proc_open(struct inode *inode,
                           struct file *file)
{
    return single_open(file, asgn1_proc_show, NULL);
}

static const struct proc_ops asgn1_proc_ops = {
    .proc_open    = asgn1_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/*
 * Purpose: expose mode in sysfs.
 * Parameters: device, attribute, output buffer.
 * Return: formatted byte count.
 */
static ssize_t mode_show(struct device *dev,
                         struct device_attribute *attr,
                         char *buf)
{
    int value;

    mutex_lock(&asgn1_lock);
    value = device_mode;
    mutex_unlock(&asgn1_lock);

    return sprintf(buf, "%d\n", value);
}

/*
 * Purpose: expose buffer length in sysfs.
 * Parameters: device, attribute, output buffer.
 * Return: formatted byte count.
 */
static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    int value;

    mutex_lock(&asgn1_lock);
    value = buffer_len;
    mutex_unlock(&asgn1_lock);

    return sprintf(buf, "%d\n", value);
}

static DEVICE_ATTR_RO(mode);
static DEVICE_ATTR_RO(buffer_len);

static struct attribute *asgn1_attrs[] = {
    &dev_attr_mode.attr,
    &dev_attr_buffer_len.attr,
    NULL,
};

static const struct attribute_group asgn1_attr_group = {
    .attrs = asgn1_attrs,
};

static const struct file_operations asgn1_fops = {
    .owner          = THIS_MODULE,
    .open           = asgn1_open,
    .release        = asgn1_release,
    .read           = asgn1_read,
    .write          = asgn1_write,
    .unlocked_ioctl = asgn1_ioctl,
    .llseek         = no_llseek,
};

/*
 * Purpose: register character device, procfs and sysfs.
 * Parameters: none.
 * Return: 0 or negative errno.
 */
static int __init asgn1_init(void)
{
    int ret;

    ret = register_chrdev(DEVICE_MAJOR,
                          DEVICE_NAME, &asgn1_fops);

    if (ret < 0)
        return ret;

    asgn1_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(asgn1_class)) {
        ret = PTR_ERR(asgn1_class);
        goto err_chrdev;
    }

    asgn1_device = device_create(
        asgn1_class, NULL,
        MKDEV(DEVICE_MAJOR, 0),
        NULL, DEVICE_NAME);

    if (IS_ERR(asgn1_device)) {
        ret = PTR_ERR(asgn1_device);
        goto err_class;
    }

    asgn1_proc = proc_create(
        "asgn1_info", 0444,
        NULL, &asgn1_proc_ops);

    if (!asgn1_proc) {
        ret = -ENOMEM;
        goto err_device;
    }

    ret = sysfs_create_group(
        &asgn1_device->kobj, &asgn1_attr_group);

    if (ret)
        goto err_proc;

    pr_info("asgn1_driver: loaded major=241 mode=1\n");
    return 0;

err_proc:
    proc_remove(asgn1_proc);
err_device:
    device_destroy(asgn1_class, MKDEV(DEVICE_MAJOR, 0));
err_class:
    class_destroy(asgn1_class);
err_chrdev:
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);

    return ret;
}

/*
 * Purpose: release all registered driver resources.
 * Parameters: none.
 * Return: void.
 */
static void __exit asgn1_exit(void)
{
    sysfs_remove_group(
        &asgn1_device->kobj, &asgn1_attr_group);

    proc_remove(asgn1_proc);
    device_destroy(asgn1_class, MKDEV(DEVICE_MAJOR, 0));
    class_destroy(asgn1_class);
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);

    pr_info("asgn1_driver: module unloaded\n");
}

module_init(asgn1_init);
module_exit(asgn1_exit);
