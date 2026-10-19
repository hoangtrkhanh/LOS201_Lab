#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Bug 1: Null pointer dereference */
void process_data(int *data, int len) {
    int sum = 0;
    for (int i = 0; i <= len; i++) {   /* Bug: <= thay vi < */
        sum += data[i];                 /* Out-of-bounds khi i==len */
    }
    printf("Sum = %d\n", sum);
}

/* Bug 2: Use after free */
char *create_message(const char *text) {
    char *buf = malloc(strlen(text) + 1);
    strcpy(buf, text);
    free(buf);
    return buf;   /* Bug: tra ve pointer da bi free */
}

/* Bug 3: Integer overflow */
int calculate(int a, int b) {
    int result = a * b;   /* Bug: a*b co the overflow int */
    printf("calculate(%d, %d) = %d\n", a, b, result);
    return result;
}

int main(int argc, char *argv[]) {
    printf("=== Buggy App v1.0 – LAB-03 Debug Target ===\n");

    /* Test 1: array processing */
    int arr[5] = {10, 20, 30, 40, 50};
    printf("[TEST1] Processing array of 5 elements\n");
    process_data(arr, 5);   /* Bug: truyen len=5, loop den i=5 */

    /* Test 2: string message */
    printf("[TEST2] Creating message\n");
    char *msg = create_message("Hello Embedded Linux");
    printf("Message: %s\n", msg);  /* Use after free */

    /* Test 3: integer overflow */
    printf("[TEST3] Integer calculation\n");
    calculate(100000, 100000);  /* 10^10 > INT_MAX (2^31-1) */

    printf("Done.\n");
    return 0;
}
