#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#include "util.h"
#include "inst.h"
#include "list.h"
#include "evlist.h"
#include "threadmap.h"
#include "profile.h"
#include "probe.h"

unsigned tid_offset = UINT32_MAX;

struct prof_info globalinfo = {
	.is_trace = 0,
	.evlist_str = NULL,
	.is_error = 0,
	.min_index = UINT_MAX,
	.max_index = 0,
	.sample_freq = 0,
	.flog = NULL,
	.threads = {
		{
			.tid = 0,
			.state = PROF_STATE_UNINIT,
			.output_file = NULL,
			.func_counters = NULL,
			.nb_record = 0,
		}
	},
	.thread_nb = 0,
	.thread_nid = 0,
	.thread_lock = PTHREAD_MUTEX_INITIALIZER,
};

static __thread struct prof_tinfo *threadinfo = NULL;
static __thread uint8_t is_ignore = 0;
static __thread int threadid = -1;

static char *log_tag[PROF_LOG_NUM] = {
	[PROF_LOG_ERROR] = "ERROR",
	[PROF_LOG_INFO] = "INFO",
	[PROF_LOG_WARN] = "WARN",
	[PROF_LOG_DEBUG] = "DEBUG",
};

void prof_log(uint8_t level, const char *format, ...)
{
	va_list va;
	FILE *fout = NULL;

	if (globalinfo.flog)
		fout = globalinfo.flog;
	else if (level == PROF_LOG_ERROR)
		fout = stderr;
	else
		fout = stdout;

	fprintf(fout, "[%s][%d]: ", log_tag[level], threadid);
	va_start(va, format);
	vfprintf(fout, format, va);
	va_end(va);
	fprintf(fout, "\n");

	fflush(fout);
}

static inline void __update_threadnid(struct prof_info *info)
{
	unsigned cur_id = info->thread_nid;
	unsigned nxt_id = PROF_THREAD_MAX, i;

	if (info->thread_nb >= PROF_THREAD_MAX) {
		info->thread_nid = UINT_MAX;
		return;
	}
	
	for (i = 1; i < PROF_THREAD_MAX; i++) {
		nxt_id = (cur_id + i) % PROF_THREAD_MAX;

		if (info->threads[nxt_id].tid == 0) {
			info->thread_nid = nxt_id;
			return;
		}
	}
}

static inline struct prof_tinfo *__find_tinfo(int pid)
{
	int i;

	for (i = 0; i < PROF_THREAD_MAX; i++) {
		if (globalinfo.threads[i].tid == pid) {
			return &globalinfo.threads[i];
		}
	}
	return NULL;
}

static int __create_evlist(struct prof_info *info,
							struct prof_tinfo *tinfo)
{
	struct prof_evlist *evlist = NULL;

	evlist = prof_evlist__new(tinfo->tid);
	if (!evlist) {
		LOG_ERROR("Failed to create evlist (%s) for process %d",
						info->evlist_str, tinfo->tid);
		return -1;
	}

	// parse and add events
	if (prof_evlist__add_from_str(evlist, info->evlist_str) < 0) {
		LOG_ERROR("Wrong event list %s", info->evlist_str);
		goto fail_destroy_evlist;
	}
	LOG_DEBUG("Add %d events", evlist->nr_entries);

	// start all events
	if (prof_evlist__start(evlist) < 0) {
		LOG_ERROR("Failed to start events");
		goto fail_destroy_evlist;
	}

	tinfo->evlist = evlist;
	tinfo->state = PROF_STATE_RUNNING;
	return 0;

fail_destroy_evlist:
	prof_evlist__delete(evlist);
	return -1;
}

static void __init_thread(unsigned id)
{
	struct prof_info *info = &globalinfo;
	struct prof_tinfo *tinfo = &(info->threads[id]);
	LOG_DEBUG("init thread[%u] %d: %p", id, tinfo->tid, (void*)tinfo);

	if (__create_evlist(info, tinfo) != 0) {
		LOG_ERROR("Failed to create evlist");
		tinfo->state = PROF_STATE_ERROR;
	}
}

void lprobe_thread_init(void *p)
{
	pthread_t tid = ((pthread_t *)p)[0];
	void *ptr = (void *)(uintptr_t)tid;
	pid_t pid = ((pid_t *)(ptr + tid_offset))[0];

	struct prof_info *info = &globalinfo;
	struct prof_tinfo *tinfo = NULL;

	if (info->thread_nid == UINT32_MAX) {
		LOG_WARN("New thread %d arrived, but we don't have enough"
				"space for it.", pid);
		return;
	}

	pthread_mutex_lock(&info->thread_lock);
	tinfo = &(info->threads[info->thread_nid]);
	tinfo->tid = pid;
	info->thread_nb ++;
	__init_thread(info->thread_nid);
	__update_threadnid(info);
	pthread_mutex_unlock(&info->thread_lock);
}

int lprobe_pthread_create(void *p1, void *p2, void *p3, void *p4)
{
	int ret = NULL;

	ret = HOOK_pthread_create(p1, p2, p3, p4);
	if (!ret) {
		lprobe_thread_init(p1);
	}
	return ret;
}

/* Skip "." and ".." directories */
static int __filter(const struct dirent *dir)
{
	if (dir->d_name[0] == '.')
		return 0;
	else
		return 1;
}

void __scan_threads(int pid, struct prof_info *info)
{
	char name[256] = {'\0'};
	int items, nb_thread;
	struct dirent **namelist = NULL;
	int i;

	sprintf(name, "/proc/%d/task", pid);
	items = scandir(name, &namelist, __filter, NULL);
	if (items <= 0) {
		struct prof_tinfo *tinfo = NULL;

		tinfo = &(info->threads[info->thread_nid]);
		tinfo->tid = pid;
		info->thread_nb ++;
		__init_thread(info->thread_nid);
		__update_threadnid(info);
		
	}
	else {
		nb_thread = items;
		if (items > PROF_THREAD_MAX) {
			LOG_WARN("Detect %d threads, only the first %d will be measured",
						items, PROF_THREAD_MAX);
			nb_thread = PROF_THREAD_MAX;
		}

		for (i = 0; i < nb_thread; i++) {
			struct prof_tinfo *tinfo = NULL;

			tinfo = &(info->threads[info->thread_nid]);
			tinfo->tid = atoi(namelist[i]->d_name);
			if (tinfo->tid > 0) {
				info->thread_nb ++;
				__init_thread(info->thread_nid);
				__update_threadnid(info);
			}
		}
	}

	for (i = 0; i < items; i++)
		free(namelist[i]);
	free(namelist);
}

void *lprobe_init(char *evlist_str, char *logfile,
				unsigned min_id, unsigned max_id, unsigned freq)
{
	int pid;
	struct prof_info *info = &globalinfo;
	// FILE *fp = NULL;
	// char buf[32] = {'\0'};
	
	pid = getpid();
	threadid = pid;
	info->max_index = max_id;
	info->min_index = min_id;
	info->sample_freq = freq;
	info->evlist_str = strdup(evlist_str);
	// if (strlen(logfile) == 0)
	// 	snprintf(buf, 32, "prof_%d.log", pid);
	// else
	// 	snprintf(buf, 32, "%s", logfile);
	
	// fp = fopen(buf, "w");
	// if (!fp) {
	// 	LOG_ERROR("Failed to open log file %s, using stdout/stderr",
	// 					buf);
	// 	return (void *)-1;
	// }
	// LOG_INFO("Create log file %s", buf);
	// info->flog = fp;

	if (pthread_mutex_init(&info->thread_lock, NULL) != 0) {
		LOG_ERROR("Failed to init thread mutex");
		goto fail_close_log;
	}

	/* scan threads */
	__scan_threads(pid, info);

	threadinfo = __find_tinfo(pid);
	if (threadinfo->state == PROF_STATE_ERROR) {
		LOG_ERROR("Failed to init master thread %d", pid);
		goto fail_close_log;
	}


	// LOG_INFO("Create event list %s", evlist_str);
	// if (__create_evlist(info, evlist_str, pid) < 0) {
	// 	LOG_ERROR("Failed to create event list %s", evlist_str);
	// 	goto fail_close_log;
	// }

//	// init current thread
//	__init_thread();

	return (void *)0;

fail_close_log:
	fclose(info->flog);
	info->flog = NULL;
	info->is_error = 1;

	return (void *)-1;
}

void lprobe_thread_exit(void)
{

}

void lprobe_pthread_exit(void *p)
{
	HOOK_pthread_exit(p);
	lprobe_thread_exit();
}

static void __destroy_tinfo(struct prof_tinfo *tinfo)
{
	LOG_INFO("Destroy thread data for %d", tinfo->tid);
	if (tinfo->evlist) {
		prof_evlist__delete(tinfo->evlist);
		tinfo->evlist = NULL;
	}
	tinfo->tid = -1;
	tinfo->state = PROF_STATE_UNINIT;
}

void lprobe_exit(void)
{
	struct prof_info *info = &globalinfo;
	int i = 0;

	LOG_INFO("Profile exit.");
	if (info->is_error)
		return;

	if (info->flog) {
		fclose(info->flog);
		info->flog = NULL;
	}
	pthread_mutex_destroy(&info->thread_lock);

	for (i = 0; i < PROF_THREAD_MAX; i++) {
		struct prof_tinfo *tinfo = &info->threads[i];

		if (tinfo->tid == -1)
			continue;
		if (tinfo->state == PROF_STATE_RUNNING) {
			__destroy_tinfo(tinfo);
		}
	}
}

static void __read_count(unsigned int func_index, uint8_t is_post,
						struct prof_tinfo *local)
{
	struct prof_evsel *evsel = NULL;
	uint8_t i = 0;
	uint64_t count = 0, cyc = 0;

	cyc = rdtsc();
	LOG_DEBUG("cyc %llu", cyc);

	evlist__for_each(local->evlist, evsel) {
		count = prof_evsel__rdpmc(evsel);
		LOG_DEBUG("func[%u], %u, %lu, cyc %llu",
				func_index, is_post, count, cyc);
// 		local->records[local->nb_record].func_idx = func_index;
// 		local->records[local->nb_record].ev_idx = i;
// 		local->records[local->nb_record].count = prof_evsel__rdpmc(evsel, local->tid);
// //		local->records[local->nb_record].count = prof_evsel__read(evsel, local->tid);
// 		i++;
// 		local->nb_record ++;
// //		if (local->nb_record == PROF_RECORD_MAX) {
// //			local->state = PROF_STATE_STOP;
// //			break;
// //		}
// 		if (local->nb_record == PROF_RECORD_CACHE) {
// //			fwrite(local->records, sizeof(struct prof_record),
// //							local->nb_record, local->output_file);
// 			local->nb_record = 0;
// 		}
	}
}

void lprobe_func_entry(unsigned int func_index)
{
	if (is_ignore)
		return;
	
	if (threadinfo == NULL) {
		LOG_DEBUG("First enter");
		threadid = syscall(__NR_gettid);
		threadinfo = __find_tinfo(threadid);
		if (threadinfo == NULL) {
			is_ignore = 1;
			LOG_INFO("No tinfo found for %d", threadid);
			return;
		}
		is_ignore = 0;
		LOG_DEBUG("Thread %d start, tinfo %p", threadid, threadinfo);
	}

	if (threadinfo->state != PROF_STATE_RUNNING)
		return;
	LOG_DEBUG("FUNC %u enter", func_index);
	__read_count(func_index, 0, threadinfo);
}

void lprobe_func_exit(unsigned int func_index)
{
	if (is_ignore)
		return;
	
	if (threadinfo == NULL) {
		threadid = syscall(__NR_gettid);
		threadinfo = __find_tinfo(threadid);
		if (threadinfo == NULL) {
			is_ignore = 1;
			return;
		}
		is_ignore = 0;
		LOG_INFO("Thread %d start, tinfo %p", threadid, threadinfo);
	}
	if (threadinfo->state != PROF_STATE_RUNNING)
		return;
	LOG_INFO("FUNC %u exit", func_index);
	__read_count(func_index, 1, threadinfo);
}

void prof_dump_records(void) {

}
