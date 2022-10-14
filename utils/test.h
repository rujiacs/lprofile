#ifndef __TEST_H__
#define __TEST_H__

#include "BPatch.h"

enum {
	TEST_MODE_FUNCMAP = 0,
	TEST_MODE_PROF,
	TEST_MODE_WRAP,
	TEST_MODE_REPLACE,
	TEST_MODE_REPORT,
	TEST_MODE_SIMPLE,
	TEST_MODE_TRACER,
	TEST_MODE_HELP,
	TEST_MODE_NUM,
};

class Test {

	public:
		virtual ~Test(void) = default;

		virtual bool parseArgs(int argc, char **argv) = 0;
		virtual bool init(void) = 0;
		virtual bool process(void) = 0;
		virtual void destroy(void) = 0;
};

extern BPatch bpatch;

#endif // __TEST_H__
