#include "util.h"
#include "evsel.h"
#include "evlist.h"
#include "array.h"
#include "pmu.h"
#include "threadmap.h"
#include "inst.h"

struct prof_evsel *
prof_evsel__new(struct perf_event_attr *attr,
				const char *name)
{
	struct prof_evsel *evsel = NULL;

	evsel = zalloc(sizeof(struct prof_evsel));
	if (evsel == NULL) {
		LOG_ERROR("Failed to alloc memory for prof_evsel");
		return NULL;
	}

	evsel->name = strdup(name);

	if (attr)
		prof_evsel__init(evsel, attr, 0);

	return evsel;
}

void
prof_evsel__init(struct prof_evsel *evsel,
				struct perf_event_attr *attr, int idx)
{
	if (evsel == NULL)
		return;

	evsel->idx = idx;
	evsel->attr = *attr;
	evsel->evlist = NULL;
	evsel->is_open = false;
	INIT_LIST_HEAD(&evsel->node);
}

void
prof_evsel__disable(struct prof_evsel *evsel, int nthreads)
{
	int thread;
	struct thread_data *data = NULL;

	if (!evsel->is_enable)
		return;

	for (thread = 0; thread < nthreads; thread++) {
		data = &(evsel->per_thread[thread]);
		if (data->fd < 0)
			continue;

		// unmap
		if (data->mm_page) {
			munmap(data->mm_page, PAGE_SIZE);
			data->mm_page = NULL;
		}

		// disable event
		ioctl(data->fd, PERF_EVENT_IOC_DISABLE, 0);
	}

	evsel->is_enable = 1;
}

int
prof_evsel__enable(struct prof_evsel *evsel, int nthreads)
{
	int thread, i;
	struct thread_data *data = NULL;
	struct perf_event_mmap_page * pc = NULL;

	if (!evsel->is_open) {
		LOG_ERROR("Wrong state: the event is not open");
		return 1;
	}

	if (evsel->is_enable)
		return 0;

	for (thread = 0; thread < nthreads; thread++) {
		data = &(evsel->per_thread[thread]);
		if (data->fd < 0)
			continue;

		// enable event
		ioctl(data->fd, PERF_EVENT_IOC_ENABLE, 0);

		// mmap
		data->mm_page = mmap(NULL, PAGE_SIZE, PROT_READ,
						MAP_SHARED, data->fd, 0);
		if (data->mm_page == MAP_FAILED) {
			LOG_ERROR("Failed to mmap perf event, err %d", errno);
			data->mm_page = NULL;
			goto out_failed;
		}

		// get hardware counter index
		pc = (struct perf_event_mmap_page *)data->mm_page;
		data->hwc_index = pc->index;

		LOG_INFO("Enable event %s for thread %d, hwc_index %u, cap_user_rdpmc %u",
						evsel->name, thread, data->hwc_index,
						pc->cap_user_rdpmc);
	}
	evsel->is_enable = 1;

	return 0;

out_failed:
	for (i = 0; i <= thread; i++) {
		data = &(evsel->per_thread[thread]);
		if (data->fd < 0)
			continue;

		if (data->mm_page) {
			munmap(data->mm_page, PAGE_SIZE);
			data->mm_page = NULL;
		}

		ioctl(data->fd, PERF_EVENT_IOC_DISABLE, 0);
	}
	return -1;
}

void
prof_evsel__close(struct prof_evsel *evsel)
{
	int thread, nthreads;
	struct thread_data *data = NULL;

	if (!evsel->is_open)
		return;

	nthreads = thread_map__nr(evsel->threads);

	if (evsel->is_enable)
		prof_evsel__disable(evsel, nthreads);

	for (thread = 0; thread < nthreads; ++thread) {
		data = &(evsel->per_thread[thread]);
		if (data->fd < 0)
			continue;

		close(data->fd);
		data->fd = -1;
	}

	evsel->is_open = 0;
}

void
prof_evsel__delete(struct prof_evsel *evsel)
{
	if (evsel->is_open)
		prof_evsel__close(evsel);

	evsel->threads = NULL;
	if (evsel->name)
		free(evsel->name);
	free(evsel->per_thread);
	free(evsel);
}

int
prof_evsel__open(struct prof_evsel *evsel,
				struct thread_map *threads)
{
	int nthread = 0, thread, pid = 0, err = 0;
	struct thread_data *info = NULL;
	enum {
		NO_CHANGE, SET_TO_MAX, INCREASED_MAX,
	} set_rlimit = NO_CHANGE;

	if (threads == NULL) {
		if (evsel->evlist && evsel->evlist->threads)
			evsel->threads = evsel->evlist->threads;
		else {
			LOG_ERROR("Threadmap is not found");
			return -EINVAL;
		}
	}
	else
		evsel->threads = threads;

	nthread = thread_map__nr(threads);

	// create per-thread data
	evsel->per_thread = zalloc(
					nthread * sizeof(struct thread_data));
	if (!evsel->per_thread) {
		LOG_ERROR("Failed to allocate memory for "
						"per-thread data");
		return -ENOMEM;
	}

	for (thread = 0; thread < nthread; thread++) {
		pid = PID(threads, thread);
		info = &(evsel->per_thread[thread]);

		LOG_INFO("Open event %s %u %lu for pid %d",
						evsel->name,
						(unsigned int)evsel->attr.type,
						(unsigned long)evsel->attr.config,
						pid);
retry_open:
		info->fd = syscall(__NR_perf_event_open,
						&evsel->attr, pid, -1, -1, 0);
		if (info->fd < 0) {
			err = -errno;
			if (errno != 2)
				LOG_ERROR("sys_perf_event_open failed, errno %d",
								errno);
			goto try_fallback;
		}

		set_rlimit = NO_CHANGE;
	}

	evsel->is_open = 1;
	return 0;

try_fallback:
	if (err == -EMFILE && set_rlimit < INCREASED_MAX) {
		struct rlimit l;
		int old_errno = errno;

		if (getrlimit(RLIMIT_NOFILE, &l) == 0) {
			if (set_rlimit == NO_CHANGE)
				l.rlim_cur = l.rlim_max;
			else {
				l.rlim_cur = l.rlim_max + 1000;
				l.rlim_max = l.rlim_cur;
			}

			if (setrlimit(RLIMIT_NOFILE, &l) == 0) {
				set_rlimit++;
				errno = old_errno;
				goto retry_open;
			}
		}
		errno = old_errno;
	}

	while (--thread >= 0) {
		if (FD(evsel, thread) < 0)
			continue;
		ioctl(FD(evsel, thread), PERF_EVENT_IOC_DISABLE, 0);
		close(FD(evsel, thread));
		FD(evsel, thread) = -1;
	}

	free(evsel->per_thread);
	evsel->per_thread = NULL;
	evsel->is_open = 0;
	return err;
}

/* Options:
 * - u: count in user space
 * - k: count in kernel space
 */
static void
__evsel__parse_opt(char *opt,
				struct perf_event_attr *attr)
{
	int i = 0, opt_len = 0;
	bool user = false, kernel = false;

	opt_len = strlen(opt);
	for (i = 0; i < opt_len; i++) {
		switch(opt[i]) {
		case 'u':
			user = true;
			break;
		case 'k':
			kernel = true;
			break;
		default:
			LOG_ERROR("Unknown argument %c", opt[i]);
		}
	}

	if (user && !kernel)
		attr->exclude_kernel = 1;
	else if (!user && kernel)
		attr->exclude_user = 1;
}

/* Usage: <event_name>:<opt1><opt2>... */
char *
prof_evsel__parse(const char *str, struct perf_event_attr *attr)
{
	char *s = NULL, *p = NULL;
	char *name = NULL, *opt = NULL;
	struct prof_pmu *pmu = NULL;

	s = strdup(str);
	p = strchr(s, ':');
	if (p != NULL) {
		if (strlen(p) > 1) {
			opt = p + 1;
		}
		*p = '\0';
	}

	name = s;
	pmu = prof_pmu__find(name);
	if (pmu == NULL) {
		free(s);
		LOG_ERROR("No event named %s", name);
		return NULL;
	}

	memset(attr, 0, sizeof(*attr));
	attr->type = pmu->type;
	attr->config = pmu->config;
	attr->size = sizeof(*attr);
	attr->disabled = 1;

	if (opt != NULL)
		__evsel__parse_opt(opt, attr);
	return name;
}

#if 1
uint64_t
prof_evsel__read(struct prof_evsel *evsel, int thread)
{
	uint64_t data;
	int fd = -1;

	if (!evsel->is_enable)
		return 0;

	fd = FD(evsel, thread);
	if (fd < 0) {
		LOG_ERROR("Wrong fd value %d", fd);
		return UINT64_MAX;
	}

	if (readn(fd, &data, sizeof(uint64_t)) < 0) {
		LOG_ERROR("Failed to read counter");
		return UINT64_MAX;
	}

	return data;
}
#endif

uint64_t
prof_evsel__rdpmc(struct prof_evsel *evsel, int thread)
{
	uint64_t value;
	uint32_t index;
//	struct perf_event_mmap_page *pc = NULL;

	if (!evsel->is_enable) {
		return 0;
	}

//	pc = (struct perf_event_mmap_page *)evsel->per_thread[thread].mm_page;
	index = evsel->per_thread[thread].hwc_index;

//	rdpmcl(index - 1, value);
	rdpmcl(index - 1, value);
//	fprintf(stdout, "hwc %u, value %lu\n", index, value);

	return value;
}
