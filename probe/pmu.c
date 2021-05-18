#include "util.h"
#include "evsel.h"
#include "threadmap.h"
#include "pmu.h"

enum {
	PROF_PMU_CPU_CYCLES = 0,
	PROF_PMU_INSTRUCTIONS,
	PROF_PMU_CACHE_REFERENCES,
	PROF_PMU_CACHE_MISSES,
	PROF_PMU_BUS_CYCLES,
	PROF_PMU_BRANCH_INSTRUCTIONS,
	PROF_PMU_BRANCH_MISSES,
	PROF_PMU_L1D_READ_REFERENCES,
	PROF_PMU_L1D_READ_MISSES,
	PROF_PMU_L1D_WRITE_REFERENCES,
	PROF_PMU_L1D_WRITE_MISSES,
	PROF_PMU_L1D_PREFETCH_REFERENCES,
	PROF_PMU_L1D_PREFETCH_MISSES,
	PROF_PMU_L1I_READ_REFERENCES,
	PROF_PMU_L1I_READ_MISSES,
	PROF_PMU_L1I_WRITE_REFERENCES,
	PROF_PMU_L1I_WRITE_MISSES,
	PROF_PMU_L1I_PREFETCH_REFERENCES,
	PROF_PMU_L1I_PREFETCH_MISSES,
	PROF_PMU_LL_READ_REFERENCES,
	PROF_PMU_LL_READ_MISSES,
	PROF_PMU_LL_WRITE_REFERENCES,
	PROF_PMU_LL_WRITE_MISSES,
	PROF_PMU_LL_PREFETCH_REFERENCES,
	PROF_PMU_LL_PREFETCH_MISSES,
	PROF_PMU_DTLB_READ_REFERENCES,
	PROF_PMU_DTLB_READ_MISSES,
	PROF_PMU_DTLB_WRITE_REFERENCES,
	PROF_PMU_DTLB_WRITE_MISSES,
	PROF_PMU_DTLB_PREFETCH_REFERENCES,
	PROF_PMU_DTLB_PREFETCH_MISSES,
	PROF_PMU_ITLB_READ_REFERENCES,
	PROF_PMU_ITLB_READ_MISSES,
	PROF_PMU_ITLB_WRITE_REFERENCES,
	PROF_PMU_ITLB_WRITE_MISSES,
	PROF_PMU_ITLB_PREFETCH_REFERENCES,
	PROF_PMU_ITLB_PREFETCH_MISSES,
	PROF_PMU_MAX,
};

static struct prof_pmu pmu_list[PROF_PMU_MAX] = {
	[PROF_PMU_CPU_CYCLES] = {
		.name = "cpu-cycles",
		.type = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_CPU_CYCLES,
	},
	[PROF_PMU_INSTRUCTIONS] = {
		.name = "instructions",
		.type = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_INSTRUCTIONS,
	},
	[PROF_PMU_CACHE_REFERENCES] = {
		.name = "cache-references",
		.type = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_CACHE_REFERENCES,
	},
	[PROF_PMU_CACHE_MISSES] = {
		.name = "cache-misses",
		.type = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_CACHE_MISSES,
	},
	[PROF_PMU_BUS_CYCLES] = {
		.name = "bus-cycles",
		.type = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_BUS_CYCLES,
	},
	[PROF_PMU_BRANCH_INSTRUCTIONS] = {
		.name = "branch-instructions",
		.type = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
	},
	[PROF_PMU_BRANCH_MISSES] = {
		.name = "branch-misses",
		.type = PERF_TYPE_HARDWARE,
		.config = PERF_COUNT_HW_BRANCH_MISSES,
	},
	[PROF_PMU_L1D_READ_REFERENCES] = {
		.name = "l1d-read",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1D |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
				 	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_L1D_READ_MISSES] = {
		.name = "l1d-read-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1D |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
					(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_L1D_WRITE_REFERENCES] = {
		.name = "l1d-write",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1D |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_L1D_WRITE_MISSES] = {
		.name = "l1d-write-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1D |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_L1D_PREFETCH_REFERENCES] = {
		.name = "l1d-prefetch",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1D |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_L1D_PREFETCH_MISSES] = {
		.name = "l1d-prefetch-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1D |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_L1I_READ_REFERENCES] = {
		.name = "l1i-read",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1I |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_L1I_READ_MISSES] = {
		.name = "l1i-read-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1I |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_L1I_WRITE_REFERENCES] = {
		.name = "l1i-write",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1I |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_L1I_WRITE_MISSES] = {
		.name = "l1i-write-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1I |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_L1I_PREFETCH_REFERENCES] = {
		.name = "l1i-prefetch",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1I |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				 	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_L1I_PREFETCH_MISSES] = {
		.name = "l1i-prefetch-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_L1I |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_LL_READ_REFERENCES] = {
		.name = "ll-read",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_LL |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_LL_READ_MISSES] = {
		.name = "ll-read-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_LL |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_LL_WRITE_REFERENCES] = {
		.name = "ll-write",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_LL |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_LL_WRITE_MISSES] = {
		.name = "ll-write-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_LL |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_LL_PREFETCH_REFERENCES] = {
		.name = "ll-prefetch",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_LL |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_LL_PREFETCH_MISSES] = {
		.name = "ll-prefetch-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_LL |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_DTLB_READ_REFERENCES] = {
		.name = "dtlb-read",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_DTLB |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_DTLB_READ_MISSES] = {
		.name = "dtlb-read-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_DTLB |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_DTLB_WRITE_REFERENCES] = {
		.name = "dtlb-write",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_DTLB |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_DTLB_WRITE_MISSES] = {
		.name = "dtlb-write-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_DTLB |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_DTLB_PREFETCH_REFERENCES] = {
		.name = "dtlb-prefetch",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_DTLB |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_DTLB_PREFETCH_MISSES] = {
		.name = "dtlb-prefetch-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_DTLB |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_ITLB_READ_REFERENCES] = {
		.name = "itlb-read",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_ITLB |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_ITLB_READ_MISSES] = {
		.name = "itlb-read-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_ITLB |
					(PERF_COUNT_HW_CACHE_OP_READ << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_ITLB_WRITE_REFERENCES] = {
		.name = "itlb-write",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_ITLB |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_ITLB_WRITE_MISSES] = {
		.name = "itlb-write-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_ITLB |
					(PERF_COUNT_HW_CACHE_OP_WRITE << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
	[PROF_PMU_ITLB_PREFETCH_REFERENCES] = {
		.name = "itlb-prefetch",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_ITLB |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				  	(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
	},
	[PROF_PMU_ITLB_PREFETCH_MISSES] = {
		.name = "itlb-prefetch-misses",
		.type = PERF_TYPE_HW_CACHE,
		.config = PERF_COUNT_HW_CACHE_ITLB |
					(PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) |
				 	(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
	},
};

static bool pmu_is_init = false;

#if 0
static bool __pmu__is_supported(int event)
{
	bool ret = true;
	int open_ret = 0;
	struct prof_evsel *evsel = NULL;
	struct perf_event_attr attr;
	struct prof_pmu *pmu = NULL;
	struct {
		struct thread_map map;
		int threads[1];
	} tmap = {
		.map.nr = 1,
		.threads = {0},
	};

	if (event >= PROF_PMU_MAX) {
		return false;
	}

	pmu = &pmu_list[event];
	memset(&attr, 0, sizeof(attr));
	attr.type = pmu->type;
	attr.config = pmu->config;
	attr.disabled = 1;
	attr.size = sizeof(attr);

	evsel = prof_evsel__new(&attr, pmu->name);
	if (evsel) {
		open_ret = prof_evsel__open(evsel, &tmap.map);
		ret = open_ret >= 0;

		if (open_ret == -EACCES) {
			evsel->attr.exclude_kernel = 1;
			ret = prof_evsel__open(evsel, &tmap.map) >= 0;
		}
		prof_evsel__delete(evsel);
	}

	return ret;
}
#endif

static void __pmu__init(void)
{
//	int i = 0;
//
//	for (i = 0; i < PROF_PMU_MAX; i++) {
//		pmu_list[i].is_support = __pmu__is_supported(i);
//	}
	pmu_is_init = true;
}


struct prof_pmu *prof_pmu__find(char *str)
{
	int i = 0;
	struct prof_pmu *pmu = NULL;

	if (!pmu_is_init)
		__pmu__init();

	for (i = 0; i < PROF_PMU_MAX; i++) {
		pmu = &pmu_list[i];
		if (strcmp(str, pmu->name) == 0) {
//			if (!pmu->is_support) {
//				LOG_ERROR("Event %s is not supported", pmu->name);
//				return NULL;
//			}
			return pmu;
		}
	}

	LOG_ERROR("No event named %s", str);
	return NULL;
}

void prof_pmu__dump(void)
{
	int i = 0;
	struct prof_pmu *pmu = NULL;

	if (!pmu_is_init)
		__pmu__init();

	for (i = 0; i < PROF_PMU_MAX; i++) {
		pmu = &pmu_list[i];
		if (pmu->is_support) {
			if (pmu->type == PERF_TYPE_HARDWARE) {
				LOG_INFO("Event %s\t[HARDWARE]", pmu->name);
			} else if (pmu->type == PERF_TYPE_HW_CACHE) {
				LOG_INFO("Event %s\t[HARDWARE CACHE]", pmu->name);
			}
		}
	}
}
