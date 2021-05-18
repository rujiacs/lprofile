#ifndef __PROFILE_H__
#define __PROFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PROF_EVENT_MAX	10

#include "list.h"

struct prof_info {
	/* Tarce or summary */
	uint8_t is_trace;
	/* List of events */
	struct prof_evlist *evlist;
	/* Min index of traced function */
	unsigned min_index;
	/* Max index of traced function */
	unsigned max_index;
	/* Sample frequency */
	unsigned sample_freq;
	/* log file */
	FILE *flog;
	/* list of per-thread data.
	 * It's used for safely termination.
	 */
//	struct list_head thread_data;
};

enum {
	PROF_STATE_UNINIT = 0,
	PROF_STATE_RUNNING,
	PROF_STATE_STOP,
	PROF_STATE_ERROR,
};

#define PROF_FUNC_STACK_MAX	14

struct prof_func {
	uint32_t depth;
	uint32_t counter;
	uint32_t stack[PROF_FUNC_STACK_MAX];
};

#define PROF_RECORD_MAX	(1 << 22)
struct prof_record {
	uint16_t func_idx;
	uint8_t ev_idx;
	uint64_t count;
};

#define PROF_RECORD_CACHE (1 << 16)

// per-thread data
struct prof_tinfo {
//	struct list_head node;

	int pid;
	int tid;
	uint8_t state;

	FILE *output_file;

	/* Per-function counters
	 * Its length is (max_index + 1)
	 */
	struct prof_func *func_counters;

	uint32_t nb_record;
	struct prof_record records[PROF_RECORD_CACHE];
//	struct prof_record *records;
};

enum {
	PROF_LOG_ERROR = 0,
	PROF_LOG_INFO,
	PROF_LOG_WARN,
	PROF_LOG_DEBUG,
	PROF_LOG_NUM,
};

void prof_log(uint8_t level, const char *format, ...);

#define LOG_ERROR(format, ...) prof_log(PROF_LOG_ERROR, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) prof_log(PROF_LOG_INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) prof_log(PROF_LOG_WARN, format, ##__VA_ARGS__)

#ifdef PF_DEBUG
#define LOG_DEBUG(format, ...) prof_log(PROF_LOG_DEBUG, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif



void prof_dump_records(void);

#ifdef __cplusplus
}
#endif

#endif	// __PROFILE_H__
