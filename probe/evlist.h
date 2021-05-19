#ifndef _PROFILE_EVLIST_H_
#define _PROFILE_EVLIST_H_

#include <stdio.h>
#include "util.h"
#include "list.h"

struct thread_map;
struct prof_evsel;
struct list_head;

struct prof_evlist {
	struct list_head entries;

	int nr_entries;
//	bool enabled;

	int pid;
	// struct thread_map *threads;
	struct prof_evsel *selected;
};

struct prof_evlist *prof_evlist__new(int pid);

void prof_evlist__init(struct prof_evlist *evlist);

void prof_evlist__delete(struct prof_evlist *evlist);

int prof_evlist__add_from_str(struct prof_evlist *evlist,
				const char *str);

void prof_evlist__dump(struct prof_evlist *evlist);

// void prof_evlist__set_threads(struct prof_evlist *evlist,
// 				struct thread_map *threads);

int prof_evlist__start(struct prof_evlist *evlist);

void prof_evlist__stop(struct prof_evlist *evlist);

//int prof_evlist__read_all(struct prof_evlist *evlist,
//				uint64_t *vals);

int prof_evlist__counter_nr(struct prof_evlist *evlist);

// int prof_evlist__create_threadmap(struct prof_evlist *evlist, int pid);

static inline struct
prof_evsel *prof_evlist__first(struct prof_evlist *evlist)
{
	return list_entry(evlist->entries.next, struct prof_evsel, node);
}

/**
 * __evlist__for_each - iterate thru all the evsels
 * @list: list_head instance to iterate
 * @evsel: struct evsel iterator
 */
#define __evlist__for_each(list, evsel) \
        list_for_each_entry(evsel, list, node)

/**
 * evlist__for_each - iterate thru all the evsels
 * @evlist: evlist instance to iterate
 * @evsel: struct evsel iterator
 */
#define evlist__for_each(evlist, evsel) \
	__evlist__for_each(&(evlist)->entries, evsel)

/**
 * __evlist__for_each_continue - continue iteration thru all the evsels
 * @list: list_head instance to iterate
 * @evsel: struct evsel iterator
 */
#define __evlist__for_each_continue(list, evsel) \
        list_for_each_entry_continue(evsel, list, node)

/**
 * evlist__for_each_continue - continue iteration thru all the evsels
 * @evlist: evlist instance to iterate
 * @evsel: struct evsel iterator
 */
#define evlist__for_each_continue(evlist, evsel) \
	__evlist__for_each_continue(&(evlist)->entries, evsel)

/**
 * __evlist__for_each_reverse - iterate thru all the evsels in reverse order
 * @list: list_head instance to iterate
 * @evsel: struct evsel iterator
 */
#define __evlist__for_each_reverse(list, evsel) \
        list_for_each_entry_reverse(evsel, list, node)

/**
 * evlist__for_each_reverse - iterate thru all the evsels in reverse order
 * @evlist: evlist instance to iterate
 * @evsel: struct evsel iterator
 */
#define evlist__for_each_reverse(evlist, evsel) \
	__evlist__for_each_reverse(&(evlist)->entries, evsel)

/**
 * __evlist__for_each_safe - safely iterate thru all the evsels
 * @list: list_head instance to iterate
 * @tmp: struct evsel temp iterator
 * @evsel: struct evsel iterator
 */
#define __evlist__for_each_safe(list, tmp, evsel) \
        list_for_each_entry_safe(evsel, tmp, list, node)

/**
 * evlist__for_each_safe - safely iterate thru all the evsels
 * @evlist: evlist instance to iterate
 * @evsel: struct evsel iterator
 * @tmp: struct evsel temp iterator
 */
#define evlist__for_each_safe(evlist, tmp, evsel) \
	__evlist__for_each_safe(&(evlist)->entries, tmp, evsel)

#endif /* _PROFILE_EVLIST_H_ */
