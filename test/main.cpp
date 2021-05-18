#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#include "klargest.hpp"

#define DEFAULT_SIZE 10000
#define DEFAULT_K   4

#define THREAD_NUM 2

static int size;
static int k;

static inline void __setup(int *arr)
{
    srand(time(0));

    int i = 0;

    for (i = 0; i < size; i++) {
        arr[i] = rand() % 1000000;
    }
}

static void *__routine(void *arg)
{
    int tid = 0, klarge = 0;
    int *arr = NULL;
    struct timeval tp1, tp2;
    float time_taken;

    tid = syscall(__NR_gettid);

    fprintf(stdout, "[%d]: pthread %lu, k = %d, size = %d\n",
                    tid, pthread_self(), k, size);

    arr = (int *)malloc((size + 4) * sizeof(int));
    if (arr == NULL) {
        fprintf(stderr, "[%d]: Failed to malloc\n", tid);
        return NULL;
    }

    fprintf(stdout, "[%d]: Setup random array\n", tid);
    __setup(arr);

    gettimeofday(&tp1, NULL);
    klarge = kth_largest_qs(k, arr, size);
    gettimeofday(&tp2, NULL);

    fprintf(stdout, "[%d]: size %d, %d th largest element = %d\n",
                        tid, size, k, klarge);
    time_taken = (tp2.tv_sec - tp1.tv_sec) + (tp2.tv_usec - tp1.tv_usec) * 1e-6;
    fprintf(stdout, "[%d]: Time taken (wall clock) = %f secs\n", tid, time_taken);

    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t threads[THREAD_NUM];
    int i = 0;

    if (argc > 2) {
        size = atoi(argv[1]);
        k = atoi(argv[2]);
        if (k <= 0 || size <= 0 || k > size) {
            fprintf(stderr, "[master]: Wrong parameters, k = %d, size = %d\n", k, size);
            exit(1);
        }
    }
    else {
        size = DEFAULT_SIZE;
        k = DEFAULT_K;
    }

    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_create(&threads[i], NULL, __routine, NULL)) {
            fprintf(stderr, "[master]: Failed to create thread %d\n", i);
            exit(1);
        }
        fprintf(stdout, "[master] create thread[%d], pthread_id = %lu\n", i, threads[i]);
    }

    for (i = 0; i < THREAD_NUM; i++)
        pthread_join(threads[i], NULL);

    return 0;   
}