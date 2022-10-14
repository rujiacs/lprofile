#ifndef __REPORT_H__
#define __REPORT_H__

#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <cstdio>

#include <string>
#include <map>
#include <vector>

#include "test.h"

struct prof_record {
	uint64_t ts;
	uint16_t func_idx;
#define PROF_RECORD_POS_PRE 0
#define PROF_RECORD_POS_POST 1
	uint8_t pos;
	uint8_t ev_idx;
	uint64_t count;
};

#define REPORT_CMD "report"
class ReportTest: public Test {
	private:
		std::string datafile;
		bool is_count;

		bool processRecord(struct prof_record *record);

	public:
		ReportTest(void);
		~ReportTest(void);

		static void staticUsage(void);
		static Test *construct(void);

		void usage(void);
		bool parseArgs(int argc, char **argv);
		bool init(void);
		bool process(void);
		void destroy(void);
};

#endif // __REPORT_H__
