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
prof_evsel__disable(struct prof_evsel *evsel)
{
	int thread;
	struct thread_data *data = NULL;

	if (!evsel->is_enable)
		return;
	
	if (evsel->fd < 0)
		return;

	// unmap
	if (evsel->mm_page) {
		munmap(evsel->mm_page, PAGE_SIZE);
		evsel->mm_page = NULL;
	}

	// disable event
	ioctl(evsel->fd, PERF_EVENT_IOC_DISABLE, 0);
	evsel->is_enable = 1;
}

int
prof_evsel__enable(struct prof_evsel *evsel)
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
	
	if (evsel->fd < 0)
		return 1;

	// enable event
	ioctl(evsel->fd, PERF_EVENT_IOC_ENABLE, 0);

	// mmap
	evsel->mm_page = mmap(NULL, PAGE_SIZE, PROT_READ,
						MAP_SHARED, evsel->fd, 0);
	if (evsel->mm_page == MAP_FAILED) {
		LOG_ERROR("Failed to mmap perf event, err %d", errno);
		evsel->mm_page = NULL;
		goto out_failed;
	}

	// get hardware counter index
	pc = (struct perf_event_mmap_page *)evsel->mm_page;
	evsel->hwc_index = pc->index;

	LOG_INFO("Enable event %s, fd %d, hwc_index %u, cap_user_rdpmc %u",
			evsel->name, evsel->fd, evsel->hwc_index, pc->cap_user_rdpmc);
	evsel->is_enable = 1;

	return 0;

out_failed:
	if (evsel->mm_page) {
		munmap(evsel->mm_page, PAGE_SIZE);
		evsel->mm_page = NULL;
	}
	ioctl(evsel->fd, PERF_EVENT_IOC_DISABLE, 0);
	return -1;
}

void
prof_evsel__close(struct prof_evsel *evsel)
{
	struct thread_data *data = NULL;

	if (!evsel->is_open)
		return;

	if (evsel->is_enable)
		prof_evsel__disable(evsel);

	if (evsel->fd < 0)
		return;

	close(evsel->fd);
	evsel->fd = -1;
	evsel->is_open = 0;
}

void
prof_evsel__delete(struct prof_evsel *evsel)
{
	if (evsel->is_open)
		prof_evsel__close(evsel);

	if (evsel->name)
		free(evsel->name);
	free(evsel);
}

int
prof_evsel__open(struct prof_evsel *evsel, int pid)
{
	int err = 0;
	enum {
		NO_CHANGE, SET_TO_MAX, INCREASED_MAX,
	} set_rlimit = NO_CHANGE;

	
retry_open:
	evsel->fd = syscall(__NR_perf_event_open,
						&evsel->attr, pid, -1, -1, 0);
	if (evsel->fd < 0) {
		err = -errno;
		if (errno != 2)
			LOG_ERROR("sys_perf_event_open failed, errno %s",
							strerror(errno));
		goto try_fallback;
	}

	set_rlimit = NO_CHANGE;

	evsel->is_open = 1;
	LOG_INFO("Open event %s %u %lu for pid %d, fd %d",
				evsel->name, (unsigned int)evsel->attr.type,
				(unsigned long)evsel->attr.config, pid, evsel->fd);
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

	if (evsel->fd >= 0) {
		ioctl(evsel->fd, PERF_EVENT_IOC_DISABLE, 0);
		close(evsel->fd);
		evsel->fd = -1;
	}

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
	attr->inherit = 0;
	attr->exclude_kernel = 1;

	if (opt != NULL)
		__evsel__parse_opt(opt, attr);
	return name;
}

uint64_t
prof_evsel__read(struct prof_evsel *evsel)
{
	uint64_t data;
	int fd = -1;

	if (!evsel->is_enable)
		return 0;

	fd = evsel->fd;
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

/* based on the code in include/uapi/linux/perf_event.h */
static inline unsigned long long mmap_read_self(
					struct perf_event_mmap_page *pc,
					uint64_t *en, uint64_t *ru) {
	uint32_t seq, time_mult, time_shift, index, width;
	int64_t count;
	uint64_t enabled, running;
	uint64_t cyc, time_offset;
	int64_t pmc = 0;
	uint64_t quot, rem;
	uint64_t delta = 0;


	do {
		/* The kernel increments pc->lock any time */
		/* perf_event_update_userpage() is called */
		/* So by checking now, and the end, we */
		/* can see if an update happened while we */
		/* were trying to read things, and re-try */
		/* if something changed */
		/* The barrier ensures we get the most up to date */
		/* version of the pc->lock variable */

		seq=pc->lock;
		barrier();

		/* For multiplexing */
		/* time_enabled is time the event was enabled */
		enabled = pc->time_enabled;
		/* time_running is time the event was actually running */
		running = pc->time_running;

		/* if cap_user_time is set, we can use rdtsc */
		/* to calculate more exact enabled/running time */
		/* for more accurate multiplex calculations */
		if ( (pc->cap_user_time) && (enabled != running)) {
			cyc = rdtsc();
			time_offset = pc->time_offset;
			time_mult = pc->time_mult;
			time_shift = pc->time_shift;

			quot=(cyc>>time_shift);
			rem = cyc & (((uint64_t)1 << time_shift) - 1);
			delta = time_offset + (quot * time_mult) +
				((rem * time_mult) >> time_shift);
		}
		enabled+=delta;

		/* actually do the measurement */

		/* Index of register to read */
		/* 0 means stopped/not-active */
		/* Need to subtract 1 to get actual index to rdpmc() */
		index = pc->index;

		/* count is the value of the counter the last time */
		/* the kernel read it */
		/* If we don't sign extend it, we get large negative */
		/* numbers which break if an IOC_RESET is done */
		width = pc->pmc_width;
		count = pc->offset;
		count<<=(64-width);
		count>>=(64-width);

		/* Only read if rdpmc enabled and event index valid */
		/* Otherwise return the older (out of date?) count value */
		if (pc->cap_user_rdpmc && index) {

			/* Read counter value */
			pmc = rdpmc(index-1);

			/* sign extend result */
			pmc<<=(64-width);
			pmc>>=(64-width);

			/* add current count into the existing kernel count */
			count+=pmc;

			/* Only adjust if index is valid */
			running+=delta;
		} else {
			/* Falling back because rdpmc not supported	*/
			/* for this event.				*/
			return 0xffffffffffffffffULL;
		}

		barrier();

	} while (pc->lock != seq);

	if (en) *en=enabled;
	if (ru) *ru=running;

	return count;
}

uint64_t
prof_evsel__rdpmc(struct prof_evsel *evsel)
{
	uint64_t value, enabled, running, adjusted;
	uint32_t index;
//	struct perf_event_mmap_page *pc = NULL;

	if (!evsel->is_enable) {
		return 0;
	}

// //	pc = (struct perf_event_mmap_page *)evsel->per_thread[thread].mm_page;
// 	index = evsel->hwc_index;

// //	rdpmcl(index - 1, value);
// 	value = rdpmc(index - 1);
// 	// fprintf(stdout, "hwc %u, value %lu\n", index, value);
	value = mmap_read_self(evsel->mm_page, &enabled, &running);
	// if (enabled == running) {
	// 	/* no adjustment needed */
	// }
	// else if (enabled && running) {
	// 	adjusted = (enabled * 128LL) / running;
	// 	adjusted = adjusted * value;
	// 	adjusted = adjusted / 128LL;
	// 	value = adjusted;
	// }

	return value;
}
