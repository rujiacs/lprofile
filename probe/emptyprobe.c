#include <stdio.h>
#include <sys/syscall.h>

#include "probe.h"
#include "util.h"

static __thread unsigned long thread_tid = -1;

void lprobe_func_entry_empty(
				LPROBE_UNUSED unsigned int func_index)
{
	LPROBE_DEBUG("[%lu]: func %d enter", thread_tid, func_index);
}

void lprobe_func_exit_empty(
				LPROBE_UNUSED unsigned int func_index)
{
	LPROBE_DEBUG("[%lu]: func %d exit", thread_tid, func_index);
}

void *lprobe_init_empty(void)
{
	LPROBE_DEBUG("global init");
	return (void*)0;
}

void lprobe_exit_empty(void)
{
	LPROBE_DEBUG("Global exit");
}

void lprobe_thread_init_empty(unsigned long tid)
{
	LPROBE_DEBUG("init thread %lu", tid);
	thread_tid = tid;
}

void lprobe_thread_exit_empty(void)
{
	LPROBE_DEBUG("[%lu]: exit", thread_tid);
}

int HOOK_pthread_create(unsigned long *p1, void *p2, void *p3, void *p4);

int lprobe_pthread_create(unsigned long *p1, void *p2, void *p3, void *p4)
{
	int ret = NULL;

	ret = HOOK_pthread_create(p1, p2, p3, p4);
	if (!ret) {
		lprobe_thread_init_empty(p1[0]);
	}
	return ret;
}

void HOOK_pthread_exit(void *p);

void lprobe_pthread_exit(void *p)
{
	HOOK_pthread_exit(p);
	lprobe_thread_exit_empty();
}