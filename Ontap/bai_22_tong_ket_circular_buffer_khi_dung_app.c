/*
 * ============================================================================
 * BÀI 22/22 - THỐNG KÊ CIRCULAR BUFFER KHI ỨNG DỤNG DỪNG
 * ============================================================================
 * NGUỒN GỐC: Requirement 3 - Buffer summary when sms_app stops.
 * PHIÊN BẢN LUYỆN TẬP LAB-02, KHÔNG PHẢI BẢN VÁ sms_app GỐC:
 *   - Đây là ứng dụng USERSPACE C + POSIX threads, KHÔNG phải kernel module.
 *   - Producer ghi dữ liệu vào /dev/lab2 và đẩy mẫu vào circular buffer.
 *   - Consumer lấy mẫu từ circular buffer; số liệu đọc qua các getter.
 *   - Sau pthread_join() của CẢ HAI thread, main in ĐÚNG MỘT dòng:
 *       [SUMMARY] produced=<P> consumed=<C> size=<S> overflow=<O>
 *   - Hỗ trợ '-d N' (N>0: dừng sau N giây; N=0: dừng bằng Ctrl+C).
 *   - Hỗ trợ '--no-device' chỉ để kiểm thử pthread trên HOST; chế độ này
 *     KHÔNG kiểm tra giao tiếp thật với /dev/lab2.
 *   - Chặn SIGINT ở các thread; main nhận bằng sigwait()/sigtimedwait().
 *     Cách này tránh gọi printf()/pthread_join()/mutex từ signal handler.
 *   - Circular buffer KHÔNG ghi đè dữ liệu khi đầy: producer chờ consumer.
 *     Vì không làm rơi mẫu, overflow_count dự kiến bằng 0.
 *
 * CẢNH BÁO:
 *   Nếu đề chấm sms_app thật, cần thêm logic tương ứng vào main()/getter
 *   của PROJECT sms_app hiện hữu, không nộp app LAB-02 này thay thế.
 * ============================================================================
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define LAB2_DEVICE "/dev/lab2"
#define RING_CAPACITY 16U    /* [BÀI 22] Số mẫu tối đa trong circular buffer. */
#define SAMPLE_LEN 64U       /* [BÀI 22] Số byte cho mỗi mẫu và ký tự kết thúc. */

struct circular_buffer {
    char sample[RING_CAPACITY][SAMPLE_LEN]; /* [BÀI 22] Bộ nhớ vòng lưu dữ liệu. */
    size_t head;                             /* [BÀI 22] Vị trí lấy mẫu. */
    size_t tail;                             /* [BÀI 22] Vị trí ghi mẫu mới. */
    size_t size;                             /* [BÀI 22] Số mẫu đang chờ xử lý. */
    unsigned long long produced;             /* [BÀI 22] Số mẫu enqueue thành công. */
    unsigned long long consumed;             /* [BÀI 22] Số mẫu dequeue thành công. */
    unsigned long long overflow;             /* [BÀI 22] Số mẫu bị rơi do tràn: 0 khi chờ chỗ. */
    bool stopping;                           /* [BÀI 22] Yêu cầu dừng các thread. */
    pthread_mutex_t mutex;                   /* [BÀI 22] Bảo vệ mọi trạng thái trên. */
    pthread_cond_t not_empty;                /* [BÀI 22] Đánh thức consumer khi có mẫu. */
    pthread_cond_t not_full;                 /* [BÀI 22] Đánh thức producer khi còn chỗ. */
};

/* [BÀI 22] Shared circular buffer: tất cả getter phải đọc đúng đối tượng này. */
static struct circular_buffer shared = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER,
};

struct producer_args {
    int device_fd;
};

/* [BÀI 22] Yêu cầu dừng từ main, đồng thời đánh thức thread đang đợi. */
static void circular_buffer_request_stop(struct circular_buffer *b)
{
    pthread_mutex_lock(&b->mutex);
    b->stopping = true;
    pthread_cond_broadcast(&b->not_empty);
    pthread_cond_broadcast(&b->not_full);
    pthread_mutex_unlock(&b->mutex);
}

/* [BÀI 22] Đưa mẫu mới vào circular buffer, chờ nếu buffer đầy. */
static bool circular_buffer_push(struct circular_buffer *b, const char *sample)
{
    pthread_mutex_lock(&b->mutex);
    while (b->size == RING_CAPACITY && !b->stopping)
        pthread_cond_wait(&b->not_full, &b->mutex);

    if (b->stopping) {
        pthread_mutex_unlock(&b->mutex);
        return false;
    }

    snprintf(b->sample[b->tail], SAMPLE_LEN, "%s", sample);
    b->tail = (b->tail + 1U) % RING_CAPACITY; /* [BÀI 22] Quay vòng chỉ số. */
    b->size++;
    b->produced++; /* [BÀI 22] Chỉ tăng sau khi enqueue thành công. */
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
    return true;
}

/* [BÀI 22] Lấy một mẫu; khi dừng vẫn lấy hết mẫu đã có rồi mới thoát. */
static bool circular_buffer_pop(struct circular_buffer *b, char out[SAMPLE_LEN])
{
    pthread_mutex_lock(&b->mutex);
    while (b->size == 0 && !b->stopping)
        pthread_cond_wait(&b->not_empty, &b->mutex);

    if (b->size == 0 && b->stopping) {
        pthread_mutex_unlock(&b->mutex);
        return false;
    }

    memcpy(out, b->sample[b->head], SAMPLE_LEN);
    b->head = (b->head + 1U) % RING_CAPACITY;
    b->size--;
    b->consumed++; /* [BÀI 22] Chỉ tăng sau khi dequeue thành công. */
    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->mutex);
    return true;
}

/* [BÀI 22] Bốn getter riêng: trả về thống kê của SHARED BUFFER, không đếm ở main. */
static unsigned long long circular_buffer_get_total_produced(struct circular_buffer *b)
{
    unsigned long long value;
    pthread_mutex_lock(&b->mutex);
    value = b->produced;
    pthread_mutex_unlock(&b->mutex);
    return value;
}

static unsigned long long circular_buffer_get_total_consumed(struct circular_buffer *b)
{
    unsigned long long value;
    pthread_mutex_lock(&b->mutex);
    value = b->consumed;
    pthread_mutex_unlock(&b->mutex);
    return value;
}

static size_t circular_buffer_get_current_size(struct circular_buffer *b)
{
    size_t value;
    pthread_mutex_lock(&b->mutex);
    value = b->size;
    pthread_mutex_unlock(&b->mutex);
    return value;
}

static unsigned long long circular_buffer_get_overflow_count(struct circular_buffer *b)
{
    unsigned long long value;
    pthread_mutex_lock(&b->mutex);
    value = b->overflow;
    pthread_mutex_unlock(&b->mutex);
    return value;
}

/* [BÀI 22] Producer tạo 1 mẫu mỗi 500 ms; ghi device rồi enqueue vào buffer. */
static void *producer_thread(void *arg)
{
    const struct producer_args *options = arg;
    unsigned long long sequence = 0;
    char sample[SAMPLE_LEN];
    const struct timespec half_second = { .tv_sec = 0, .tv_nsec = 500000000L };

    for (;;) {
        struct timespec remaining = half_second;

        while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR)
            ;

        /* [BÀI 22] Kiểm tra cờ dừng dưới mutex. */
        pthread_mutex_lock(&shared.mutex);
        if (shared.stopping) {
            pthread_mutex_unlock(&shared.mutex);
            break;
        }
        pthread_mutex_unlock(&shared.mutex);

        snprintf(sample, sizeof(sample), "sample_%llu\n", ++sequence);

        if (options->device_fd >= 0) {
            size_t len = strlen(sample);
            ssize_t sent = write(options->device_fd, sample, len);
            if (sent != (ssize_t)len) {
                fprintf(stderr, "[BÀI 22] write(/dev/lab2) thất bại: %s\n",
                        sent < 0 ? strerror(errno) : "ghi thiếu byte");
                circular_buffer_request_stop(&shared);
                break;
            }
        }

        if (!circular_buffer_push(&shared, sample))
            break;
    }
    return NULL;
}

/* [BÀI 22] Consumer đọc mẫu ở shared buffer, không đọc nhầm /proc. */
static void *consumer_thread(void *arg)
{
    char sample[SAMPLE_LEN];
    (void)arg;

    while (circular_buffer_pop(&shared, sample)) {
        printf("[CONSUMED] %s", sample);
        fflush(stdout);
    }
    return NULL;
}

static void print_usage(const char *program)
{
    fprintf(stderr, "Sử dụng: %s -d <giây> [--no-device]\n", program);
    fprintf(stderr, "  -d 5: chạy 5 giây rồi in SUMMARY\n");
    fprintf(stderr, "  -d 0: chạy tới Ctrl+C rồi in SUMMARY\n");
    fprintf(stderr, "  --no-device: chỉ thử pthread trên HOST, không ghi /dev/lab2\n");
}

int main(int argc, char **argv)
{
    int duration = -1;
    bool no_device = false;
    int fd = -1;
    pthread_t producer, consumer;
    struct producer_args options;
    sigset_t blocked_signals;
    unsigned long long P, C, O;
    size_t S;
    int i, ret;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            char *end;
            long parsed;
            errno = 0;
            parsed = strtol(argv[++i], &end, 10);
            if (errno || end == argv[i] || *end != '\0' || parsed < 0 || parsed > 86400) {
                print_usage(argv[0]);
                return 2;
            }
            duration = (int)parsed;
        } else if (strcmp(argv[i], "--no-device") == 0) {
            no_device = true;
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }
    if (duration < 0) {
        print_usage(argv[0]);
        return 2;
    }

    /* [BÀI 22] Chặn SIGINT trước pthread_create để các worker kế thừa mask. */
    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGINT);
    ret = pthread_sigmask(SIG_BLOCK, &blocked_signals, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_sigmask: %s\n", strerror(ret));
        return 1;
    }

    if (!no_device) {
        fd = open(LAB2_DEVICE, O_WRONLY);
        if (fd < 0) {
            perror("open(/dev/lab2)");
            return 1;
        }
    }
    options.device_fd = fd;

    /* [BÀI 22] Khởi tạo 2 worker: producer và consumer. */
    ret = pthread_create(&producer, NULL, producer_thread, &options);
    if (ret != 0) {
        fprintf(stderr, "pthread_create producer: %s\n", strerror(ret));
        if (fd >= 0) close(fd);
        return 1;
    }
    ret = pthread_create(&consumer, NULL, consumer_thread, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_create consumer: %s\n", strerror(ret));
        circular_buffer_request_stop(&shared);
        pthread_join(producer, NULL);
        if (fd >= 0) close(fd);
        return 1;
    }

    /* [BÀI 22] Chờ hạn -d hoặc Ctrl+C (SIGINT) trong luồng main. */
    if (duration == 0) {
        int signal_received;
        sigwait(&blocked_signals, &signal_received);
    } else {
        const struct timespec time_limit = { .tv_sec = duration, .tv_nsec = 0 };
        int signal_received = sigtimedwait(&blocked_signals, NULL, &time_limit);
        if (signal_received < 0 && errno != EAGAIN)
            fprintf(stderr, "sigtimedwait: %s\n", strerror(errno));
    }

    /* [BÀI 22] Gửi lệnh dừng và đánh thức các thread đang ngủ/chờ. */
    circular_buffer_request_stop(&shared);

    /* [BÀI 22] YÊU CẦU ĐỀ: đợi TOÀN BỘ thread kết thúc TRƯỚC khi thống kê. */
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    if (fd >= 0)
        close(fd);

    /* [BÀI 22] Dữ liệu lấy DUY NHẤT từ getter của shared circular buffer. */
    P = circular_buffer_get_total_produced(&shared);
    C = circular_buffer_get_total_consumed(&shared);
    S = circular_buffer_get_current_size(&shared);
    O = circular_buffer_get_overflow_count(&shared);

    /* [BÀI 22] In CHÍNH XÁC một dòng tổng kết ở cuối stdout. */
    printf("[SUMMARY] produced=%llu consumed=%llu size=%zu overflow=%llu\n",
           P, C, S, O);

    /* [BÀI 22] Bất biến P = C + S vì buffer không làm mất mẫu. */
    if (P != C + (unsigned long long)S || O != 0ULL)
        return 1;

    return 0; /* [BÀI 22] Kết thúc bình thường hoặc SIGINT đều trả 0. */
}
