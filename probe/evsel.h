#ifndef _PROFILE_EVSEL_H_
#define _PROFILE_EVSEL_H_

//#include "util.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct prof_evlist;
struct prof_evsel;
struct thread_map;

/* thread private data */
struct thread_data {
	/* file descriptor of the perf event */
	int fd;
	/* hardware counter index */
	uint32_t hwc_index;
	/* pointer to the MMAP oage of each perf event */
	void *mm_page;
};

struct prof_evsel {
	struct list_head node;
	struct prof_evlist *evlist;
	struct perf_event_attr attr;
	// struct thread_map *threads;
	// struct thread_data *per_thread;

	/* file descriptor of the perf event */
	int fd;
	/* hardware counter index */
	uint32_t hwc_index;
	/* pointer to the MMAP oage of each perf event */
	void *mm_page;

	union {
		uint8_t state;
		struct {
			uint8_t is_open		: 1;
			uint8_t is_enable	: 1;
			uint8_t unused		: 6;
		};
	};

	int idx;
	char *name;
};

// #define FD(evsel, thread) 
// 		((evsel->per_thread[thread]).fd)

struct prof_evsel *prof_evsel__new(struct perf_event_attr *attr,
				const char *name);

void prof_evsel__init(struct prof_evsel *evsel,
				struct perf_event_attr *attr, int idx);

// int prof_evsel__enable(struct prof_evsel *evsel, int nthread);
// void prof_evsel__disable(struct prof_evsel *evsel, int nthread);
int prof_evsel__enable(struct prof_evsel *evsel);
void prof_evsel__disable(struct prof_evsel *evsel);

void prof_evsel__delete(struct prof_evsel *evsel);

int prof_evsel__open(struct prof_evsel *evsel, int pid);

// int prof_evsel__open(struct prof_evsel *evsel,
// 				struct thread_map *threads);

void prof_evsel__close(struct prof_evsel *evsel);

char *prof_evsel__parse(const char *str,
				struct perf_event_attr *attr);

// uint64_t prof_evsel__read(struct prof_evsel *evsel, int thread);
// uint64_t prof_evsel__rdpmc(struct prof_evsel *evsel, int thread);
uint64_t prof_evsel__read(struct prof_evsel *evsel);
uint64_t prof_evsel__rdpmc(struct prof_evsel *evsel);

#ifdef __cplusplus
}
#endif

#endif /* _PROFILE_EVSEL_H_ */
