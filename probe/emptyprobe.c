#include <stdio.h>
#include <sys/syscall.h>

#include "probe.h"
#include "util.h"

#include <pthread.h>

static __thread pid_t thread_tid = 0;

void lprobe_func_entry_empty(
				LPROBE_UNUSED unsigned int func_index)
{
	LPROBE_DEBUG("[%d]: func %d enter", thread_tid, func_index);
}

void lprobe_func_exit_empty(
				LPROBE_UNUSED unsigned int func_index)
{
	LPROBE_DEBUG("[%d]: func %d exit", thread_tid, func_index);
}

void *lprobe_init_empty(void)
{
	LPROBE_INFO("global init");
	return (void*)0;
}

void lprobe_exit_empty(void)
{
	LPROBE_INFO("Global exit");
}

void lprobe_thread_init_empty(void *ptid)
{
	pthread_t tid = ((pthread_t *)ptid)[0];
	void *ptr = (void *)(uintptr_t)tid;
	pid_t *pid = (pid_t *)(ptr + tid_offset);

	thread_tid = *pid;
	LPROBE_INFO("init thread, tid_offset %u, tid %d, pthread_id %lu",
					tid_offset, thread_tid, tid);
}

void lprobe_thread_exit_empty(void)
{
	LPROBE_INFO("[%d]: exit", thread_tid);
}

int lprobe_empty_pthread_create(void *p1, void *p2, void *p3, void *p4)
{
	int ret = NULL;

	ret = HOOK_pthread_create(p1, p2, p3, p4);
	if (!ret) {
		lprobe_thread_init_empty(p1);
	}
	return ret;
}

void lprobe_empty_pthread_exit(void *p)
{
	HOOK_pthread_exit(p);
	lprobe_thread_exit_empty();
}