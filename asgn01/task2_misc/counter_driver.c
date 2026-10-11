#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/errno.h>

#define DEVICE_NAME "counter"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TODO: HoTen - MSSV");
MODULE_DESCRIPTION("ASGN01: Atomic misc counter device");
MODULE_VERSION("1.0");

/* Data layer: atomic state and statistics. */
static atomic_t counter = ATOMIC_INIT(0);
static atomic_t total_reads = ATOMIC_INIT(0);
static atomic_t total_writes = ATOMIC_INIT(0);

/*
 * Mutex keeps read-and-increment as one logical operation.
 * atomic_t keeps individual counter operations atomic.
 */
static DEFINE_MUTEX(counter_lock);

static struct proc_dir_entry *counter_proc;

/*
 * counter_read:
 * Parameters: file, userspace buffer, count, offset.
 * Purpose: return current counter then increment it.
 * Return: bytes read, 0 at EOF, or negative errno.
 */
static ssize_t counter_read(struct file *file,
                            char __user *buf,
                            size_t count, loff_t *ppos)
{
    char text[32];
    int value;
    int length;
    size_t copied;

    if (*ppos != 0 || count == 0)
        return 0;

    mutex_lock(&counter_lock);

    value = atomic_read(&counter);
    length = scnprintf(text, sizeof(text), "%d\n", value);
    copied = min(count, (size_t)length);

    if (copy_to_user(buf, text, copied)) {
        mutex_unlock(&counter_lock);
        return -EFAULT;
    }

    *ppos += copied;

    atomic_inc(&counter);
    atomic_inc(&total_reads);

    mutex_unlock(&counter_lock);

    pr_info("counter_driver: read=%d, incremented\n",
            value);

    return copied;
}

/*
 * counter_write:
 * Parameters: file, userspace input, byte count, offset.
 * Purpose: parse non-negative ASCII integer and set counter.
 * Return: input bytes accepted or -EINVAL / -EFAULT.
 */
static ssize_t counter_write(struct file *file,
                             const char __user *buf,
                             size_t count, loff_t *ppos)
{
    char text[32];
    int value;
    int ret;

    if (count == 0)
        return 0;

    if (count >= sizeof(text))
        return -EINVAL;

    if (copy_from_user(text, buf, count))
        return -EFAULT;

    text[count] = '\0';

    ret = kstrtoint(text, 10, &value);
    if (ret || value < 0)
        return -EINVAL;

    mutex_lock(&counter_lock);

    atomic_set(&counter, value);
    atomic_inc(&total_writes);

    mutex_unlock(&counter_lock);

    pr_info("counter_driver: set counter=%d\n", value);

    return count;
}

/*
 * counter_proc_show:
 * Parameters: seq_file, unused context.
 * Purpose: expose current counter and operation statistics.
 * Return: 0.
 */
static int counter_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&counter_lock);

    seq_printf(m, "Counter value: %d\n",
               atomic_read(&counter));
    seq_printf(m, "Total reads  : %d\n",
               atomic_read(&total_reads));
    seq_printf(m, "Total writes : %d\n",
               atomic_read(&total_writes));

    mutex_unlock(&counter_lock);

    return 0;
}

/*
 * counter_proc_open:
 * Purpose: register seq_file callback for /proc/counter_info.
 * Return: result of single_open.
 */
static int counter_proc_open(struct inode *inode,
                             struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}

static const struct proc_ops counter_proc_ops = {
    .proc_open    = counter_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static const struct file_operations counter_fops = {
    .owner  = THIS_MODULE,
    .read   = counter_read,
    .write  = counter_write,
    .llseek = no_llseek,
};

/* Linux misc subsystem uses major 10. */
static struct miscdevice counter_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &counter_fops,
    .mode  = 0666,
};

/*
 * counter_init:
 * Purpose: initialize state, register misc device and procfs.
 * Return: 0 on success or a negative error code.
 */
static int __init counter_init(void)
{
    int ret;

    atomic_set(&counter, 0);
    atomic_set(&total_reads, 0);
    atomic_set(&total_writes, 0);

    ret = misc_register(&counter_miscdev);
    if (ret)
        return ret;

    counter_proc = proc_create(
        "counter_info", 0444, NULL, &counter_proc_ops);

    if (!counter_proc) {
        misc_deregister(&counter_miscdev);
        return -ENOMEM;
    }

    pr_info("counter_driver: loaded, major=10, minor=%d\n",
            counter_miscdev.minor);
    return 0;
}

/*
 * counter_exit:
 * Purpose: remove procfs entry and misc device.
 * Return: void.
 */
static void __exit counter_exit(void)
{
    proc_remove(counter_proc);
    misc_deregister(&counter_miscdev);

    pr_info("counter_driver: module unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);
