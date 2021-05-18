#include "util.h"
#include "list.h"
#include "evlist.h"
#include "evsel.h"
#include "threadmap.h"

struct prof_evlist *prof_evlist__new(void)
{
	struct prof_evlist *evlist = NULL;

	evlist = zalloc(sizeof(struct prof_evlist));
	if (evlist != NULL)
		prof_evlist__init(evlist);

	return evlist;
}

void prof_evlist__init(struct prof_evlist *evlist)
{
	INIT_LIST_HEAD(&evlist->entries);
}

void prof_evlist__delete(struct prof_evlist *evlist)
{
	struct prof_evsel *evsel = NULL, *pos = NULL;
//	int n;

	evlist__for_each_reverse(evlist, evsel) {
		prof_evsel__close(evsel);
	}

	thread_map__free(evlist->threads);
	evlist->threads = NULL;

	evsel = NULL;
	evlist__for_each_safe(evlist, evsel, pos) {
		list_del_init(&pos->node);
		pos->evlist = NULL;
		prof_evsel__delete(pos);
	}
	evlist->nr_entries = 0;
	free(evlist);
	evlist = NULL;
}

static int
__prof_evlist__add(struct prof_evlist *evlist, char *str)
{
	struct perf_event_attr attr;
	char *name = NULL;
	struct prof_evsel *evsel = NULL;

	name = prof_evsel__parse(str, &attr);
	if (name == NULL) {
		LOG_ERROR("Failed to parse event %s", str);
	} else {
		evsel = prof_evsel__new(NULL, name);
		if (evsel == NULL) {
			LOG_ERROR("Failed to create prof_evsel for %s",
							name);
		} else {
			prof_evsel__init(evsel, &attr, evlist->nr_entries);
			evsel->evlist = evlist;
			list_add_tail(&evsel->node, &evlist->entries);
			evlist->nr_entries++;
			return 1;
		}
	}
	return 0;
}

/* Usage: <event1>:<opt_list1>,<event2>:<opt_list2>,...*/
int prof_evlist__add_from_str(struct prof_evlist *evlist,
				const char *str)
{
	char *pcur = NULL, *pnext = NULL, *tmp;
	int len = 0, added = 0;

	if (str == NULL || strlen(str) == 0)
		return -EINVAL;

	len = strlen(str);
	tmp = strdup(str);
	pcur = tmp;
	while ((pnext = strchr(pcur, ',')) != NULL) {
		*pnext = '\0';

		added += __prof_evlist__add(evlist, pcur);

		if ((pnext - tmp) == (len - 1))
			break;
		pcur = pnext + 1;
	}

	if ((pcur - tmp) < (len - 1))
		added += __prof_evlist__add(evlist, pcur);

	free(tmp);
	return added;
}

void prof_evlist__dump(struct prof_evlist *evlist)
{
	struct prof_evsel *evsel;

	evlist__for_each(evlist, evsel) {
		LOG_INFO("Event[%d] %s, type %u, config %lu",
						evsel->idx, evsel->name,
						evsel->attr.type,
						(unsigned long)evsel->attr.config);
	}
}

void prof_evlist__set_threads(struct prof_evlist *evlist,
				struct thread_map *threads)
{
	evlist->threads = threads;
}

static void prof_evlist__enable(struct prof_evlist *evlist,
				int nthreads)
{
	struct prof_evsel *evsel = NULL;

	evlist__for_each(evlist, evsel) {
		if (prof_evsel__enable(evsel, nthreads) < 0)
			LOG_ERROR("Failed to enable event %s", evsel->name);
	}
}

int prof_evlist__start(struct prof_evlist *evlist)
{
	struct prof_evsel *evsel = NULL;
	int nthreads = 0;

	if (!evlist->threads) {
		LOG_ERROR("Thread map is not set");
		return -1;
	}

	nthreads = thread_map__nr(evlist->threads);
	evlist__for_each(evlist, evsel) {
		if (!evsel->is_open &&
				prof_evsel__open(evsel, evlist->threads) < 0) {
			LOG_ERROR("Failed to open event %s", evsel->name);
			continue;
		}
	}

	prof_evlist__enable(evlist, nthreads);
	return 0;
}

void prof_evlist__stop(struct prof_evlist *evlist)
{
	struct prof_evsel *evsel = NULL;
	int nthreads = 0;

	nthreads = thread_map__nr(evlist->threads);
	evlist__for_each(evlist, evsel) {
		prof_evsel__disable(evsel, nthreads);
	}
}

int prof_evlist__counter_nr(struct prof_evlist *evlist)
{
//	int nthreads = 0;
//
//	nthreads = thread_map__nr(evlist->threads);
	return evlist->nr_entries;
}

/* pid == 0 => current process */
int
prof_evlist__create_threadmap(struct prof_evlist *evlist,
				int pid)
{
	struct thread_map *threads = NULL;

	if (pid == 0)
		threads = thread_map__new(getpid());
	else
		threads = thread_map__new(pid);

	if (threads == NULL)
		return -1;

	evlist->threads = threads;
	return 0;
}

//int prof_evlist__read_all(struct prof_evlist *evlist, uint64_t *vals)
//{
//	struct prof_evsel *evsel = NULL;
//	int nthreads = 0, i = 0;
//	uint64_t val = 0;
////	struct perf_count_values count;
//
//	nthreads = thread_map__nr(evlist->threads);
//
//	memset(vals, 0, sizeof(uint64_t) * evlist->nr_entries);
//
//	evlist__for_each(evlist, evsel) {
//		for (i = 0; i < nthreads; i++) {
//			prof_evsel__read(evsel, i, &val);
//			vals[evsel->idx] += val;
//		}
//	}
//
//	return evlist->nr_entries;
//}
