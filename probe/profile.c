#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "util.h"
#include "list.h"
#include "evlist.h"
#include "threadmap.h"
#include "profile.h"
#include "probe.h"

struct prof_info globalinfo = {
	.evlist = NULL,
	.min_index = UINT_MAX,
	.max_index = 0,
	.sample_freq = 0,
	.flog = NULL,
};

__thread struct prof_tinfo tinfo = {
	.pid = -1,
	.tid = -1,
	.state = PROF_STATE_UNINIT,
	.func_counters = NULL,
	.nb_record = 0,
//	.records = NULL,
};

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

	fprintf(fout, "[%s][%d]: ", log_tag[level], tinfo.pid);
	va_start(va, format);
	vfprintf(fout, format, va);
	va_end(va);
	fprintf(fout, "\n");

	fflush(fout);
}

static int __create_evlist(struct prof_info *info,
				char *evlist_str, int pid)
{
	struct prof_evlist *evlist = NULL;

	evlist = prof_evlist__new();
	if (!evlist) {
		LOG_ERROR("Failed to create evlist (%s) for process %d",
						evlist_str, pid);
		return -1;
	}

	// parse and add events
	if (prof_evlist__add_from_str(evlist, evlist_str) < 0) {
		LOG_ERROR("Wrong event list %s", evlist_str);
		goto fail_destroy_evlist;
	}
	LOG_INFO("Add %d events", evlist->nr_entries);

	// create threadmap
	if (prof_evlist__create_threadmap(evlist, pid) < 0) {
		LOG_ERROR("Failed to create thread map");
		goto fail_destroy_evlist;
	}
	LOG_INFO("%d threads detected.",
					thread_map__nr(evlist->threads));

	// start all events
	if (prof_evlist__start(evlist) < 0) {
		LOG_ERROR("Failed to start events");
		goto fail_destroy_evlist;
	}

	info->evlist = evlist;
	return 0;

fail_destroy_evlist:
	prof_evlist__delete(evlist);
	return -1;
}

#if 0
static int __init_record(struct prof_tinfo *local)
{
	int fd = -1, ret = 0;
	char buf[32] = {'\0'};
	size_t size;
	void *ptr = NULL;

	snprintf(buf, 32, "prof_%d.data", local->pid);

	size = sizeof(struct prof_record) * PROF_RECORD_MAX;

	fd = open(buf, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		LOG_ERROR("Failed to create record file %s, err %d",
						buf, errno);
		return -1;
	}

	ret = ftruncate(fd, size);
	if (ret < 0) {
		LOG_ERROR("Failed to resize the record file %s, err %d",
						buf, errno);
		close(fd);
		return -1;
	}

	ptr = mmap(0, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		LOG_ERROR("Failed to mmap record file %s, err %d",
						buf, errno);
		close(fd);
		return -1;
	}
	memset(ptr, 0, size);

	local->records = ptr;
	return 0;
}
#endif

static void __init_thread(void)
{
	struct prof_evlist *evlist = globalinfo.evlist;
	struct prof_tinfo *info = &tinfo;
	unsigned int nb_funcs = globalinfo.max_index - globalinfo.min_index + 1;

	info->pid = syscall(__NR_gettid);
	info->tid = thread_map__getindex(evlist->threads, info->pid);
	if (info->tid == -1) {
		LOG_ERROR("No thread %d found in threadmap", info->pid);
		info->state = PROF_STATE_ERROR;
		return;
	}

	// allocate counter
	info->func_counters = (struct prof_func *)malloc(
					sizeof(struct prof_func) * nb_funcs);
	if (!info->func_counters) {
		LOG_ERROR("Failed to allocate memory for function counters,"
				  "its desired length is %u",
				  nb_funcs);
		info->state = PROF_STATE_ERROR;
		return;
	}
	LOG_INFO("Create %u function counters [%u-%u]",
					nb_funcs,
					globalinfo.min_index,
					globalinfo.max_index);

	// open data file
	char buf[32] = {'\0'};

	sprintf(buf, "profile_%d.data", info->pid);
	info->output_file = fopen(buf, "w");
	if (!info->output_file) {
		free(info->func_counters);
		info->func_counters = NULL;
		info->state = PROF_STATE_ERROR;
		LOG_ERROR("Failed to open data file %s", buf);
		return;
	}
	LOG_INFO("Data file %s", buf);

	memset(info->func_counters, 0, sizeof(struct prof_func) * nb_funcs);

	memset(info->records, 0, sizeof(struct prof_record) * PROF_RECORD_CACHE);
//	// allocate record storage
//	if (__init_record(info) < 0) {
//		LOG_ERROR("Failed to init record storage");
//		free(info->func_counters);
//		info->func_counters = NULL;
//		info->state = PROF_STATE_ERROR;
//		return;
//	}


	tinfo.state = PROF_STATE_RUNNING;
	LOG_INFO("Thread %d index %d, address %p",
					tinfo.pid, tinfo.tid, info);
	return;
}

void lprobe_thread_init(void)
{
	struct prof_tinfo *info = &tinfo;

	info->pid = syscall(__NR_gettid);

	LOG_INFO("INIT thread %d", info->pid);
}

void *lprobe_init(char *evlist_str, char *logfile,
				unsigned min_id, unsigned max_id, unsigned freq)
{
	int pid;
	struct prof_info *info = &globalinfo;
	FILE *fp = NULL;
	char buf[32] = {'\0'};

	pid = getpid();
	tinfo.pid = syscall(__NR_gettid);
//	INIT_LIST_HEAD(&info->thread_data);
	info->max_index = max_id;
	info->min_index = min_id;
	info->sample_freq = freq;

	if (strlen(logfile) == 0)
		snprintf(buf, 32, "prof_%d.log", pid);
	else
		snprintf(buf, 32, "%s", logfile);

	fp = fopen(buf, "w");
	if (!fp) {
		LOG_ERROR("Failed to open log file %s, using stdout/stderr",
						buf);
		return (void *)-1;
	}
	LOG_INFO("Create log file %s", buf);
	info->flog = fp;

	LOG_INFO("Create event list %s", evlist_str);
	if (__create_evlist(info, evlist_str, pid) < 0) {
		LOG_ERROR("Failed to create event list %s", evlist_str);
		goto fail_close_log;
	}

//	// init current thread
//	__init_thread();

	return (void *)0;

fail_close_log:
	fclose(info->flog);
	info->flog = NULL;

	return (void *)-1;
}

#if 1
static void __test_print(struct prof_info *global, struct prof_tinfo *thread)
{
	struct prof_func *func = NULL;
	unsigned int i = 0, nb_func = global->max_index - global->min_index + 1;

	if (!thread->func_counters)
		return;

	for (i = 0; i < nb_func; i++) {
		func = &thread->func_counters[i];
		if (func->counter == 0)
			continue;

		LOG_INFO("func %u: %u", global->min_index + i, func->counter);
	}
}
#endif

void lprobe_thread_exit(void)
{
	struct prof_tinfo *local = &tinfo;

	LOG_INFO("Destroy per-thread data");
	if (local->func_counters) {
		__test_print(&globalinfo, local);
		free(local->func_counters);
		local->func_counters = NULL;
	}

	// write data to file
	LOG_INFO("%u records", local->nb_record);
	if (local->nb_record && local->output_file) {
		fwrite(local->records, sizeof(struct prof_record), local->nb_record,
						local->output_file);
		local->nb_record = 0;
	}

	// close output file
	if (local->output_file)
		fclose(local->output_file);
//	if (local->records) {
//		munmap(local->records, PROF_RECORD_MAX * sizeof(struct prof_record));
//		local->records = NULL;
//	}
}

static void __destroy_evlist(void)
{
	struct prof_evlist *evlist = globalinfo.evlist;

	if (!evlist)
		return;

	prof_evlist__delete(evlist);
	globalinfo.evlist = NULL;
}

void lprobe_exit(void)
{
	LOG_INFO("Profile exit.");

	//prof_thread_exit();
	__destroy_evlist();

	if (globalinfo.flog) {
		fclose(globalinfo.flog);
		globalinfo.flog = NULL;
	}
}

#if 1
static void __read_count(struct prof_evlist *evlist,
				unsigned int func_index, struct prof_tinfo *local)
{
	struct prof_evsel *evsel = NULL;
	uint8_t i = 0;

	evlist__for_each(evlist, evsel) {
//		prof_evsel__rdpmc(evsel, local->tid);
		local->records[local->nb_record].func_idx = func_index;
		local->records[local->nb_record].ev_idx = i;
		local->records[local->nb_record].count = prof_evsel__rdpmc(evsel, local->tid);
//		local->records[local->nb_record].count = prof_evsel__read(evsel, local->tid);
		i++;
		local->nb_record ++;
//		if (local->nb_record == PROF_RECORD_MAX) {
//			local->state = PROF_STATE_STOP;
//			break;
//		}
		if (local->nb_record == PROF_RECORD_CACHE) {
//			fwrite(local->records, sizeof(struct prof_record),
//							local->nb_record, local->output_file);
			local->nb_record = 0;
		}
	}
}
#endif

void lprobe_func_entry(unsigned int func_index)
{
	struct prof_info *global = &globalinfo;
	struct prof_tinfo *local = &tinfo;
//	struct prof_func *func = NULL;

	if (local->state == PROF_STATE_UNINIT)
		__init_thread();

	if (local->state != PROF_STATE_RUNNING)
		return;

//	func = &local->func_counters[func_index - global->min_index];
//	if (func->depth < PROF_FUNC_STACK_MAX) {
//		func->stack[func->depth] = func->counter;
//		func->counter ++;

		__read_count(global->evlist, func_index, local);
//	}
//	func->depth ++;
}

void lprobe_func_exit(unsigned int func_index)
{
	struct prof_info *global = &globalinfo;
	struct prof_tinfo *local = &tinfo;
//	struct prof_func *func = NULL;
//	unsigned int cnt = 0;

	if (local->state != PROF_STATE_RUNNING)
		return;

//	func = &local->func_counters[func_index - global->min_index];
//	func->depth --;
//	if (func->depth < PROF_FUNC_STACK_MAX) {
//		cnt = func->counter[func->deep];
		__read_count(global->evlist, func_index, local);
//	}
}

void prof_dump_records(void) {
	struct prof_tinfo *local = &tinfo;
	unsigned int i = 0;

	for (i = 0; i < local->nb_record; i++) {
		fprintf(stdout, "%u,%lu\n",
				local->records[i].func_idx, local->records[i].count);
	}
}
