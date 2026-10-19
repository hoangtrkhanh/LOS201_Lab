#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define ARRAY_SIZE  100000
#define ITERATIONS  50

/* Hàm tính toán nặng – CPU bound */
double heavy_compute(double *arr, int n) {
    double result = 0.0;
    for (int i = 0; i < n; i++) {
        result += sqrt(arr[i]) * log(arr[i] + 1.0);
    }
    return result;
}

/* Hàm tìm kiếm tuyến tính – O(n) */
int linear_search(int *arr, int n, int target) {
    for (int i = 0; i < n; i++)
        if (arr[i] == target) return i;
    return -1;
}

/* Hàm I/O – nhiều system call */
void io_intensive(int count) {
    FILE *f = fopen("/tmp/perf_test.txt", "w");
    for (int i = 0; i < count; i++)
        fprintf(f, "Line %d: measurement data\n", i);
    fclose(f);
}

int main(void) {
    printf("=== Perf Target App – LAB-03 ===\n");

    /* Khởi tạo array */
    double *darr = malloc(sizeof(double) * ARRAY_SIZE);
    int    *iarr = malloc(sizeof(int)    * ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        darr[i] = (double)(i + 1);
        iarr[i] = i;
    }

    printf("[1] CPU-intensive computation...\n");
    double total = 0;
    for (int iter = 0; iter < ITERATIONS; iter++)
        total += heavy_compute(darr, ARRAY_SIZE);
    printf("    Result: %f\n", total);

    printf("[2] Linear search (1000 times)...\n");
    for (int i = 0; i < 1000; i++)
        linear_search(iarr, ARRAY_SIZE, ARRAY_SIZE - 1);

    printf("[3] I/O intensive (write 10000 lines)...\n");
    io_intensive(10000);

    free(darr); free(iarr);
    printf("Done.\n");
    return 0;
}
