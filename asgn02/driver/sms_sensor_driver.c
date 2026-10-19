/*
 * ASSIGNMENT-02: Sensor Monitoring System
 * File: sms_sensor_driver.c
 * Purpose: Simulated temperature and humidity sensor driver
 *
 * Author: [Your name and student ID]
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/mutex.h>
#include <linux/errno.h>

#include "sms_sensor.h"

/* ===== A. DRIVER STATE ===== */

static DEFINE_MUTEX(sms_lock);

static unsigned int interval_ms =
    SMS_DEFAULT_INTERVAL_MS;

static unsigned int read_count;
static unsigned int last_temp_centi;
static unsigned int last_hum_centi;

static u64 start_time_ms;
static u64 last_read_ms;

static struct proc_dir_entry *sms_proc_entry;

/* ===== B. DEVICE READ ===== */
static ssize_t sms_read(
    struct file *file,
    char __user *buf,
    size_t count,
    loff_t *ppos)
{
    char line[128];
unsigned int temperature;
unsigned int humidity;
u64 now_ms;
int len;

/* Không đọc lại khi đã hết một mẫu */
if (*ppos > 0)
    return 0;
/* Khong xu ly neu userspace yeu cau 0 byte */
if (count == 0)
    return 0;

/* Bao ve trang thai chung cua driver */
mutex_lock(&sms_lock);
/* Lấy thời gian monotonic */
now_ms = ktime_to_ms(ktime_get());
/* Chua du khoang thoi gian de doc mau moi */
if (last_read_ms != 0 &&
    now_ms - last_read_ms < interval_ms) {
    mutex_unlock(&sms_lock);
    return -EAGAIN;
}
/* Tạo dữ liệu cảm biến */
temperature = 1500U + get_random_u32() % 3001U;
humidity = 3000U + get_random_u32() % 6001U;

/* Chuyển dữ liệu thành chuỗi CSV */
len = scnprintf(line, sizeof(line),
    "%llu,%u.%02u,%u.%02u\n",
    (unsigned long long)now_ms,
    temperature / 100, temperature % 100,
    humidity / 100, humidity % 100);
/* Kiem tra buffer cua userspace */
if (count < (size_t)len) {
    mutex_unlock(&sms_lock);
    return -EINVAL;
}

/* Tra CSV ve userspace */
if (copy_to_user(buf, line, len)) {
    mutex_unlock(&sms_lock);
    return -EFAULT;
}

/* Cap nhat thong ke sau khi copy thanh cong */
read_count++;
last_temp_centi = temperature;
last_hum_centi = humidity;
last_read_ms = now_ms;

/* Cap nhat vi tri doc */
*ppos += len;

/* Giai phong mutex */
mutex_unlock(&sms_lock);

/* Tra so byte doc thanh cong */
return len;
}

/* ===== C. IOCTL ===== */

static long sms_ioctl(
    struct file *file,
    unsigned int cmd,
    unsigned long arg)
{
unsigned int new_interval;

/* Kiểm tra ioctl command */
if (cmd != SMS_SET_INTERVAL)
    return -ENOTTY;

/* Nhận khoảng thời gian từ userspace */
if (copy_from_user(&new_interval,
                   (void __user *)arg,
                   sizeof(new_interval)))
    return -EFAULT;

/* Không cho phép interval bằng 0 */
if (new_interval == 0)
    return -EINVAL;

/* Bảo vệ biến dùng chung */
mutex_lock(&sms_lock);
interval_ms = new_interval;
mutex_unlock(&sms_lock);

return 0;
}

/* ===== D. FILE OPERATIONS ===== */

static const struct file_operations sms_fops = {
    .owner          = THIS_MODULE,
    .read           = sms_read,
    .unlocked_ioctl = sms_ioctl,
    .llseek         = no_llseek,
};

/* ===== E. MISC DEVICE ===== */

static struct miscdevice sms_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = SMS_DEVICE_NAME,
    .fops  = &sms_fops,
};

/* ===== F. PROCFS STATISTICS ===== */

static int sms_proc_show(
    struct seq_file *m,
    void *v)
{
    u64 uptime_seconds =
        div_u64((u64)(ktime_to_ms(ktime_get()) - start_time_ms), 1000U);

    mutex_lock(&sms_lock);

    seq_printf(m, "read_count: %u\n", read_count);

    seq_printf(m, "last_temperature: %u.%02u\n",
               last_temp_centi / 100,
               last_temp_centi % 100);

    seq_printf(m, "last_humidity: %u.%02u\n",
               last_hum_centi / 100,
               last_hum_centi % 100);

    seq_printf(m, "driver_uptime_seconds: %llu\n",
               (unsigned long long)uptime_seconds);

    seq_printf(m, "interval_ms: %u\n", interval_ms);

    mutex_unlock(&sms_lock);

    return 0;
}

static int sms_proc_open(
    struct inode *inode,
    struct file *file)
{
    return single_open(file, sms_proc_show, NULL);
}

static const struct proc_ops sms_proc_ops = {
    .proc_open    = sms_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* ===== G. MODULE INITIALIZATION ===== */

static int __init sms_init(void)
{
    int ret;

    start_time_ms = ktime_to_ms(ktime_get());
    last_read_ms = 0;

    ret = misc_register(&sms_device);

    if (ret) {
        pr_err("sms_sensor: misc_register failed\n");
        return ret;
    }

    sms_proc_entry = proc_create(
        SMS_PROC_NAME,
        0444,
        NULL,
        &sms_proc_ops
    );

    if (!sms_proc_entry) {
        misc_deregister(&sms_device);
        return -ENOMEM;
    }

    pr_info("sms_sensor: driver initialized\n");

    return 0;
}

/* ===== H. MODULE CLEANUP ===== */

static void __exit sms_exit(void)
{
    proc_remove(sms_proc_entry);
    misc_deregister(&sms_device);

    pr_info("sms_sensor: driver unloaded\n");
}

module_init(sms_init);
module_exit(sms_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION(
    "ASSIGNMENT-02 Simulated Sensor Driver"
);
MODULE_VERSION("1.0");
