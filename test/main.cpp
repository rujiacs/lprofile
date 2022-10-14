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

#define DEFAULT_SIZE 100
#define DEFAULT_K   4

#define THREAD_NUM 1

#define RDTSC

static int size;
static int k;

#ifdef RDTSC
static inline unsigned long long rdtsc(void)
{
	unsigned hi, lo;

	asm volatile("rdtsc":"=a"(lo), "=d"(hi));
	return (((unsigned long long)lo) |
					(((unsigned long long)hi) << 32));
}
#endif

static inline void __setup(int *arr)
{
    // srand(time(0));

    int i = 0;

    for (i = 0; i < size; i++) {
        // arr[i] = rand() % 1000000;
        if (i%2)
            arr[i] = i;
        else
            arr[i] = size - i;
    }
}

static void *__routine(void *arg)
{
    int tid = 0, klarge = 0, i = 0;
    int *arr = NULL;
#ifndef RDTSC
    struct timeval tp1, tp2;
    float time_taken;
#else
    unsigned long long cyc_start, cyc_end;
#endif

    tid = syscall(__NR_gettid);

    fprintf(stdout, "[%d]: pthread %lu, k = %d, size = %d\n",
                    tid, pthread_self(), k, size);

    arr = (int *)malloc((size + 4) * sizeof(int));
    if (arr == NULL) {
        fprintf(stderr, "[%d]: Failed to malloc\n", tid);
        return NULL;
    }

//    fprintf(stdout, "[%d]: Setup random array\n", tid);

    __setup(arr);
    klarge = kth_largest_qs(k, arr, size);


    for (i = 0; i < 20; i++){
#ifndef RDTSC
    gettimeofday(&tp1, NULL);
#else
    cyc_start = rdtsc();
#endif
    klarge = kth_largest_qs(k, arr, size);
#ifndef RDTSC
    gettimeofday(&tp2, NULL);
#else
    cyc_end = rdtsc();
#endif

//    fprintf(stdout, "[%d]: size %d, %d th largest element = %d\n",
//                        tid, size, k, klarge);

#ifdef RDTSC
    fprintf(stdout, "%llu\n", (cyc_end - cyc_start));
//    fprintf(stdout, "[%d]: Time taken = %llu cycles\n", tid, (cyc_end - cyc_start));
#else
    time_taken = (tp2.tv_sec - tp1.tv_sec) + (tp2.tv_usec - tp1.tv_usec);
    fprintf(stdout, "%.0f\n", time_taken);
    // fprintf(stdout, "[%d]: Time taken (wall clock) = %f us\n", tid, time_taken);
#endif
    }

    return NULL;
}

int main(int argc, char **argv)
{
    // pthread_t threads[THREAD_NUM];
    // int i = 0;

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

    int tid = syscall(__NR_gettid);

    fprintf(stdout, "tid %d\n", tid);

    fprintf(stdout, "enter any key to start\n");
    int tmp;

    // __routine(NULL);

    fscanf(stdin, "%d", &tmp);

    __routine(NULL);

//    for (i = 0; i < 10; i++) {
//        sleep(2);
//        __routine(NULL);
//    }

    //  for (i = 0; i < THREAD_NUM; i++) {
    //      if (pthread_create(&threads[i], NULL, __routine, NULL)) {
    //          fprintf(stderr, "[master]: Failed to create thread %d\n", i);
    //          exit(1);
    //      }
    //      fprintf(stdout, "[master] create thread[%d], pthread_id = %lu\n", i, threads[i]);
    //  }

    //  for (i = 0; i < THREAD_NUM; i++)
    //      pthread_join(threads[i], NULL);

    return 0;   
}
