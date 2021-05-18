#ifndef _PROFILE_PMU_H_
#define _PROFILE_PMU_H_

#include <stdint.h>
#include <linux/perf_event.h>

#define PROFILE_PMU_NAME_MAX 30

struct prof_pmu {
	char name[PROFILE_PMU_NAME_MAX];
	uint32_t type;
	uint64_t config;
	bool is_support;
};

struct prof_pmu *prof_pmu__find(char *str);

void prof_pmu__dump(void);

#endif /* _PROFILE_PMU_H_ */
