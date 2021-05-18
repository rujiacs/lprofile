#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "klargest.hpp"

#define DEFAULT_SIZE 10000
#define DEFAULT_K   4

static inline void __setup(int *arr, int size)
{
    srand(time(0));

    int i = 0;

    for (i = 0; i < size; i++) {
        arr[i] = rand();
    }
}

int main(int argc, char **argv)
{
    int klarge, k, size;
    struct timeval tp1, tp2;
    float time_taken;
    int *arr = NULL;

    if (argc > 2) {
        size = atoi(argv[1]);
        k = atoi(argv[2]);
        if (k <= 0 || size <= 0 || k > size) {
            fprintf(stderr, "Wrong parameters, k = %d, size = %d\n", k, size);
            exit(1);
        }
    }
    else {
        size = DEFAULT_SIZE;
        k = DEFAULT_K;
    }

    fprintf(stdout, "k = %d, size = %d\n", k, size);

    arr = (int *)malloc((size + 4) * sizeof(int));
    if (arr == NULL) {
        fprintf(stderr, "Failed to malloc\n");
        exit(1);
    }

    fprintf(stdout, "Setup random array\n");
    __setup(arr, size);

    gettimeofday(&tp1, NULL);
    klarge = kth_largest_qs(k, arr, size);
    gettimeofday(&tp2, NULL);

    fprintf(stdout, "size %d, %d th largest element = %d\n", size, k, klarge);
    time_taken = (tp2.tv_sec - tp1.tv_sec) + (tp2.tv_usec - tp1.tv_usec) * 1e-6;
    fprintf(stdout, "Time taken (wall clock) = %f secs\n", time_taken);
}