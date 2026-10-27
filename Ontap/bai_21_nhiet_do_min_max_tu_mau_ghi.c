/*
 * ============================================================================
 * BÀI 21/22 - THEO DÕI NHIỆT ĐỘ THẤP NHẤT / CAO NHẤT TRONG LAB-02
 * ============================================================================
 * NGUỒN GỐC: Bài 2 trong nhóm 3 câu hỏi của Assignment 02.
 * PHIÊN BẢN LUYỆN TẬP TRÊN LAB-02 (ĐIỀU CHỈNH SO VỚI ĐỀ GỐC):
 *   - Vì LAB-02 KHÔNG tự tạo mẫu nhiệt độ, mỗi lần userspace ghi chuỗi nhiệt độ
 *     theo định dạng 25.37 hoặc 25.37\n vào /dev/lab2 được xem là một mẫu.
 *   - Mỗi mẫu hợp lệ cập nhật min/max (số nguyên nhân 100, không dùng float).
 *   - /proc/lab2_info có hai dòng min_temperature, max_temperature.
 *   - Trước mẫu đầu tiên, hai giá trị phải là N/A.
 *   - Các chuỗi KHÔNG phải nhiệt độ vẫn được ghi đè buffer như driver gốc,
 *     nhưng KHÔNG được dùng để cập nhật min/max.
 *   - Chỉ có phần [BÀI 21] mới được thêm vào bản nguồn gốc đã làm đẹp.
 * ============================================================================
 */

/*
 * ============================================================================
 * LAB-02: CHARACTER DEVICE DRIVER
 * ============================================================================
 * Mục tiêu:
 *   - Tạo character device /dev/lab2 để đọc/ghi dữ liệu qua bộ đệm nội bộ.
 *   - Cung cấp thông tin thống kê qua /proc/lab2_info.
 *   - Cung cấp các thuộc tính chỉ đọc qua sysfs.
 *   - Sử dụng mutex để bảo vệ thao tác cập nhật trạng thái dùng chung.
 *
 * Ghi chú:
 *   Đây là phiên bản sắp xếp lại bố cục và viết comment bằng tiếng Việt.
 *   Logic gốc của LAB-02 được giữ nguyên; chưa thêm các bài tập mở rộng.
 * ============================================================================
 */

/* ============================================================================
 * 01. CÁC THƯ VIỆN KERNEL CẦN THIẾT
 * ============================================================================ */
#include <linux/module.h>    /* Khai báo thông tin và vòng đời kernel module */
#include <linux/kernel.h>    /* Các macro thông báo kernel: pr_info, pr_err, ... */
#include <linux/fs.h>        /* Character device và struct file_operations */
#include <linux/uaccess.h>   /* Sao chép dữ liệu kernel ↔ userspace */
#include <linux/cdev.h>      /* Thư viện hỗ trợ character device */
#include <linux/device.h>    /* Device model, class, device và sysfs */
#include <linux/proc_fs.h>   /* Tạo và quản lý /proc/lab2_info */
#include <linux/seq_file.h>  /* Hỗ trợ xuất thông tin procfs theo trình tự */
#include <linux/slab.h>      /* Thư viện quản lý bộ nhớ kernel */
#include <linux/mutex.h>     /* Đồng bộ truy cập dữ liệu dùng chung */

/* ============================================================================
 * 02. THÔNG TIN MODULE VÀ CÁC HẰNG SỐ CẤU HÌNH
 * ============================================================================ */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Lab");
MODULE_DESCRIPTION("LAB-02: Simple Character Device Driver");
MODULE_VERSION("1.0");

#define DEVICE_NAME     "lab2"        /* Tên device: /dev/lab2 */
#define CLASS_NAME      "lab2_class"  /* Tên class trong /sys/class/ */
#define BUFFER_SIZE     1024          /* Dung lượng buffer nội bộ (byte) */
#define DRIVER_MAJOR    240           /* Major number cố định của LAB-02 */
/* Dải major 240–254 dành cho mục đích cục bộ/thử nghiệm. */

/* ============================================================================
 * 03. TRẠNG THÁI DÙNG CHUNG CỦA DRIVER
 * ============================================================================ */
static int major_number = DRIVER_MAJOR;       /* Major number đang sử dụng */
static char device_buffer[BUFFER_SIZE];       /* Nơi lưu dữ liệu đã ghi */
static int buffer_len = 0;                    /* Số byte dữ liệu hợp lệ */
static int open_count = 0;                    /* Tổng số lần mở device */

/* [BÀI 21] Biến min/max lưu phần trăm độ C: 25.37°C được lưu thành 2537. */
static int min_temperature_x100;               /* [BÀI 21] Nhiệt độ thấp nhất. */
static int max_temperature_x100;               /* [BÀI 21] Nhiệt độ cao nhất. */
static bool has_temperature_sample;            /* [BÀI 21] Phân biệt chưa có mẫu với giá trị 0. */

static struct class *lab2_class = NULL;       /* Device class đã đăng ký */
static struct device *lab2_device = NULL;     /* Đối tượng device trong kernel */
static struct proc_dir_entry *proc_entry;     /* Entry /proc/lab2_info */

/*
 * Mutex dùng để đồng bộ thao tác cập nhật dữ liệu dùng chung.
 * Nếu nhiều process cùng thao tác, mutex giúp tránh xung đột khi cập nhật.
 */
static DEFINE_MUTEX(lab2_mutex);

/* ============================================================================
 * [BÀI 21] BỘ PHÂN TÍCH NHIỆT ĐỘ FIXED-POINT
 * ============================================================================
 * Mục đích:
 *   Chuyển chuỗi đúng dạng [+|-]DDD.DD hoặc [+|-]DDD.DD\n sang số nguyên x100.
 * Lý do:
 *   Không sử dụng float trong kernel; tránh nhầm dữ liệu chữ thành mẫu nhiệt độ.
 * Giới hạn thực hành: phần nguyên tối đa 1000, ví dụ -15.25, 45.00.
 * Trả true nếu toàn bộ chuỗi có định dạng hợp lệ, false nếu không.
 */
static bool lab2_parse_temperature_x100(const char *s, size_t len, int *result)
{
    size_t i = 0;
    int sign = 1;
    int integer_part = 0;
    int decimal_part;
    bool have_digit = false;

    if (!len)
        return false;

    if (s[len - 1] == '\n') /* [BÀI 21] Chấp nhận newline từ lệnh echo. */
        len--;

    if (i < len && (s[i] == '-' || s[i] == '+')) {
        if (s[i] == '-')
            sign = -1;
        i++;
    }

    while (i < len && s[i] >= '0' && s[i] <= '9') {
        have_digit = true;
        integer_part = integer_part * 10 + (s[i] - '0');
        if (integer_part > 1000) /* [BÀI 21] Chặn giá trị vô lý/tràn số. */
            return false;
        i++;
    }

    if (!have_digit || i >= len || s[i] != '.')
        return false;
    i++;

    /* [BÀI 21] Yêu cầu đúng HAI chữ số sau dấu chấm. */
    if (len - i != 2 ||
        s[i] < '0' || s[i] > '9' ||
        s[i + 1] < '0' || s[i + 1] > '9')
        return false;

    decimal_part = (s[i] - '0') * 10 + (s[i + 1] - '0');
    *result = sign * (integer_part * 100 + decimal_part);
    return true;
}

/* ============================================================================
 * 04. CÁC HÀM XỬ LÝ THAO TÁC VỚI /dev/lab2
 * ============================================================================ */

/*
 * lab2_open(): được gọi khi userspace mở /dev/lab2.
 *
 * Mục đích:
 *   - Tăng open_count khi một lần mở device thành công.
 *   - Dùng mutex để bảo vệ thao tác tăng biến dùng chung.
 *
 * Lưu ý: open_count là tổng số lần mở, không phải số file đang mở.
 */
static int lab2_open(struct inode *inode, struct file *file)
{
    mutex_lock(&lab2_mutex);
    open_count++;
    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: device opened (count=%d)\n", open_count);
    return 0;
}

/*
 * lab2_release(): được gọi khi kernel giải phóng file đã mở.
 *
 * Trong LAB-02 gốc, hàm chỉ ghi log; không giảm open_count.
 */
static int lab2_release(struct inode *inode, struct file *file)
{
    pr_info("lab2_driver: device closed\n");
    return 0;
}

/*
 * lab2_read(): chuyển dữ liệu từ device_buffer sang userspace.
 *
 * Trình tự:
 *   1. Kiểm tra vị trí đọc; nếu hết dữ liệu thì trả về 0 (EOF).
 *   2. Giới hạn số byte đọc theo lượng dữ liệu còn lại trong buffer.
 *   3. Dùng copy_to_user() để sao chép an toàn sang bộ nhớ userspace.
 *   4. Cập nhật *offset và trả về số byte đã đọc.
 *
 * Không dùng memcpy() trực tiếp với con trỏ __user, vì nó tham chiếu
 * đến bộ nhớ userspace và cần được truy cập qua kernel user-access API.
 */
static ssize_t lab2_read(struct file *file, char __user *buf,
                         size_t len, loff_t *offset)
{
    int bytes_read = 0;

    /* Nếu vị trí hiện tại đã ở cuối dữ liệu, báo EOF. */
    if (*offset >= buffer_len)
        return 0;

    /* Không đọc vượt quá phần dữ liệu hợp lệ còn lại. */
    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    /* Đưa dữ liệu từ kernel buffer sang userspace buffer. */
    if (copy_to_user(buf, device_buffer + *offset, len)) {
        pr_err("lab2_driver: copy_to_user failed\n");
        return -EFAULT;
    }

    /* Ghi nhận vị trí đọc mới và số byte đã trả về. */
    *offset += len;
    bytes_read = len;

    pr_info("lab2_driver: sent %d bytes to user\n", bytes_read);
    return bytes_read;
}

/*
 * lab2_write(): nhận dữ liệu từ userspace và lưu vào device_buffer.
 *
 * Trình tự:
 *   1. Giới hạn len ở mức BUFFER_SIZE - 1 để còn chỗ cho ký tự '\0'.
 *   2. Khóa mutex trước khi ghi vào buffer dùng chung.
 *   3. Dùng copy_from_user() để lấy dữ liệu từ userspace.
 *   4. Cập nhật buffer_len, thêm '\0' và mở khóa mutex.
 *
 * Mỗi lần ghi sẽ GHI ĐÈ nội dung buffer cũ, chưa hỗ trợ ghi nối tiếp.
 */
static ssize_t lab2_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *offset)
{
    int temperature_x100; /* [BÀI 21] Nhiệt độ giải mã từ lần ghi hiện tại. */

    /* Để lại một byte cuối cho ký tự kết thúc chuỗi '\0'. */
    if (len > BUFFER_SIZE - 1) {
        pr_warn("lab2_driver: write too large, truncating\n");
        len = BUFFER_SIZE - 1;
    }

    /* Bảo vệ thao tác ghi buffer khi nhiều process truy cập. */
    mutex_lock(&lab2_mutex);

    /* Sao chép dữ liệu từ userspace vào kernel buffer. */
    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }

    /* Cập nhật kích thước dữ liệu và kết thúc chuỗi ký tự. */
    buffer_len = len;
    device_buffer[buffer_len] = '\0';

    /* [BÀI 21] Chỉ xử lý NHIỆT ĐỘ sau khi copy_from_user() thành công. */
    if (lab2_parse_temperature_x100(device_buffer, buffer_len,
                                   &temperature_x100)) {
        if (!has_temperature_sample) {
            /* [BÀI 21] Mẫu đầu tiên là cả min lẫn max (không dùng mốc 0). */
            min_temperature_x100 = temperature_x100;
            max_temperature_x100 = temperature_x100;
            has_temperature_sample = true;
        } else {
            if (temperature_x100 < min_temperature_x100)
                min_temperature_x100 = temperature_x100; /* [BÀI 21] Cập nhật cực tiểu. */
            if (temperature_x100 > max_temperature_x100)
                max_temperature_x100 = temperature_x100; /* [BÀI 21] Cập nhật cực đại. */
        }
    }

    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: received %zu bytes: [%s]\n",
            len, device_buffer);
    return len;
}

/* ============================================================================
 * 05. ÁNH XẠ THAO TÁC VFS SANG CÁC CALLBACK CỦA DRIVER
 * ============================================================================ */
static struct file_operations lab2_fops = {
    .owner   = THIS_MODULE,   /* Module sở hữu các callback */
    .open    = lab2_open,     /* open("/dev/lab2", ...) */
    .release = lab2_release,  /* close() / giải phóng file */
    .read    = lab2_read,     /* read() */
    .write   = lab2_write,    /* write() */
};

/* ============================================================================
 * 06. PROCFS - XUẤT THÔNG TIN TỔNG HỢP TẠI /proc/lab2_info
 * ============================================================================ */

/*
 * seq_file hỗ trợ xuất dữ liệu văn bản qua giao diện đọc tuần tự.
 * single_open() gắn một callback show() cho file /proc, giúp userspace
 * có thể đọc thông tin bằng lệnh `cat /proc/lab2_info`.
 */
static int lab2_proc_show(struct seq_file *m, void *v)
{
    int local_min, local_max;        /* [BÀI 21] Bản sao trạng thái nhiệt độ. */
    bool local_has_sample;          /* [BÀI 21] Đã nhận mẫu hay chưa? */

    /* [BÀI 21] Lấy snapshot dưới cùng mutex với lab2_write(). */
    mutex_lock(&lab2_mutex);
    local_has_sample = has_temperature_sample;
    local_min = min_temperature_x100;
    local_max = max_temperature_x100;
    mutex_unlock(&lab2_mutex);

    seq_printf(m, "LAB-02 Driver Statistics\n");
    seq_printf(m, "======================\n");
    seq_printf(m, "Driver name   : %s\n", DEVICE_NAME);
    seq_printf(m, "Major number : %d\n", major_number);
    seq_printf(m, "Buffer size   : %d bytes\n", BUFFER_SIZE);
    seq_printf(m, "Data length   : %d bytes\n", buffer_len);
    seq_printf(m, "Open count    : %d\n", open_count);

    if (!local_has_sample) {
        /* [BÀI 21] Trước mẫu đầu tiên phải hiển thị N/A cả hai dòng. */
        seq_puts(m, "min_temperature: N/A\n");
        seq_puts(m, "max_temperature: N/A\n");
    } else {
        /* [BÀI 21] Trình bày fixed-point theo định dạng xx.xx (có dấu nếu âm). */
        int min_abs = local_min < 0 ? -local_min : local_min;
        int max_abs = local_max < 0 ? -local_max : local_max;

        seq_printf(m, "min_temperature: %s%d.%02d\n",
                   local_min < 0 ? "-" : "", min_abs / 100, min_abs % 100);
        seq_printf(m, "max_temperature: %s%d.%02d\n",
                   local_max < 0 ? "-" : "", max_abs / 100, max_abs % 100);
    }

    /* Nếu buffer có dữ liệu, in chuỗi được ghi gần nhất. */
    if (buffer_len > 0)
        seq_printf(m, "Last data     : [%s]\n", device_buffer);

    return 0;
}

/* Kết nối file /proc với hàm show() thông qua single_open(). */
static int lab2_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, lab2_proc_show, NULL);
}

/* Các thao tác đọc/giải phóng của file procfs. */
static const struct proc_ops lab2_proc_fops = {
    .proc_open    = lab2_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* ============================================================================
 * 07. SYSFS - CÁC THUỘC TÍNH CHỈ ĐỌC CỦA DEVICE
 * ============================================================================ */

/* Trả về số byte dữ liệu hợp lệ hiện có trong device_buffer. */
static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", buffer_len);
}

/* Trả về tổng số lần mở /dev/lab2 kể từ khi nạp module. */
static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", open_count);
}

/* Trả về chuỗi hiện tại trong buffer, hoặc '(empty)' nếu buffer rỗng. */
static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    if (buffer_len > 0)
        return sprintf(buf, "%s\n", device_buffer);

    return sprintf(buf, "(empty)\n");
}

/*
 * DEVICE_ATTR_RO(): tạo attribute chỉ đọc và liên kết với hàm *_show().
 * Mỗi attribute được đưa vào danh sách để tạo file tương ứng trong sysfs.
 */
static DEVICE_ATTR_RO(buffer_len);
static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(last_data);

/* Danh sách attribute sẽ được đăng ký cho đối tượng device. */
static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    NULL,
};

/* Gom các attribute thành một nhóm để đăng ký/gỡ bỏ cùng nhau. */
static struct attribute_group lab2_attr_group = {
    .attrs = lab2_attrs,
};

/* ============================================================================
 * 08. KHỞI TẠO MODULE - CHẠY KHI insmod
 * ============================================================================ */

/*
 * lab2_init(): tạo các tài nguyên cần thiết cho character device.
 *
 * Thu tu:
 *   1. Đăng ký major number và file_operations.
 *   2. Tạo device class.
 *   3. Tạo đối tượng device.
 *   4. Tạo /proc/lab2_info.
 *   5. Đăng ký nhóm sysfs attributes.
 */
static int __init lab2_init(void)
{
    int ret;

    pr_info("lab2_driver: initializing module\n");

    /* Đăng ký character device với major number cố định. */
    ret = register_chrdev(major_number, DEVICE_NAME, &lab2_fops);
    if (ret < 0) {
        pr_err("lab2_driver: register_chrdev failed: %d\n", ret);
        return ret;
    }

    /* Tạo device class để kết nối driver với Linux Device Model. */
    lab2_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(lab2_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_class);
    }

    /* Tạo device trong class, sử dụng major 240 và minor 0. */
    lab2_device = device_create(lab2_class, NULL,
                                MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(lab2_device)) {
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_device);
    }

    /* Tạo file thống kê /proc/lab2_info. */
    proc_entry = proc_create("lab2_info", 0444, NULL, &lab2_proc_fops);
    if (!proc_entry)
        pr_warn("lab2_driver: failed to create /proc/lab2_info\n");
    else
        pr_info("lab2_driver: /proc/lab2_info created\n");

    /* Đưa nhóm attribute vào sysfs thông qua kobject của device. */
    ret = sysfs_create_group(&lab2_device->kobj, &lab2_attr_group);
    if (ret)
        pr_warn("lab2_driver: sysfs_create_group failed\n");

    pr_info("lab2_driver: loaded, major=%d\n", major_number);
    return 0;
}

/* ============================================================================
 * 09. DỌN DẸP MODULE - CHẠY KHI rmmod
 * ============================================================================ */

/*
 * lab2_exit(): gỡ bỏ tài nguyên đã tạo khi nạp module.
 *
 * Thu tu:
 *   1. Gỡ file procfs nếu đã được tạo.
 *   2. Gỡ nhóm sysfs attributes.
 *   3. Hủy device và device class.
 *   4. Hủy đăng ký major number.
 */
static void __exit lab2_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);

    sysfs_remove_group(&lab2_device->kobj, &lab2_attr_group);
    device_destroy(lab2_class, MKDEV(major_number, 0));
    class_destroy(lab2_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    pr_info("lab2_driver: module unloaded\n");
}

/* ============================================================================
 * 10. ĐĂNG KÝ ĐIỂM VÀO/RA CỦA KERNEL MODULE
 * ============================================================================ */
module_init(lab2_init);
module_exit(lab2_exit);
