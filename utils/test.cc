#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#include "util.h"
#include "tracer.h"
#include "funcmap.h"
#include "prof.h"
#include "simple.h"
#include "test.h"

#include "BPatch.h"

using namespace std;
using namespace Dyninst;

struct testcmd {
	const char *cmd;
	Test *(*construct)(void);
	void (*usage)(void);
};

static void help_usage(void)
{
	fprintf(stdout, "./lprofile help\n");
}

static struct testcmd cmds[TEST_MODE_NUM] = {
//	[TEST_MODE_ATTACH] = {
//		.cmd = TRACER_CMD,
//		.construct = TracerTest::construct,
//		.usage = TracerTest::staticUsage,
//	},
	[TEST_MODE_FUNCMAP] = {
		.cmd = FUNCMAP_CMD,
		.construct = FuncMapTest::construct,
		.usage = FuncMapTest::staticUsage,
	},
	[TEST_MODE_PROF] = {
		.cmd = PROF_CMD,
		.construct = ProfTest::construct,
		.usage = ProfTest::staticUsage,
	},
	[TEST_MODE_SIMPLE] = {
		.cmd = SIMPLE_CMD,
		.construct = SimpleTest::construct,
		.usage = SimpleTest::staticUsage,
	},
//	[TEST_MODE_EDIT] = {
//		.cmd = EDIT_CMD,
//		.construct = EditTest::construct,
//		.usage = EditTest::staticUsage,
//	},
	[TEST_MODE_HELP] = {
		.cmd = "help",
		.construct = NULL,
		.usage = help_usage,
	},
};

// global BPatch instance
BPatch bpatch;

static void usage(void)
{
	LPROFILE_INFO("Usage:");
	for (unsigned i = 0; i < TEST_MODE_NUM; i++)
		cmds[i].usage();
}

int main(int argc, char **argv)
{
	Test *test = NULL;
	char *cmd = NULL;
	int ret = 0;

	if (argc < 2) {
		LPROFILE_ERROR("Wrong parameters");
		usage();
		return 1;
	}

	cmd = argv[1];
	argc --;
	argv ++;

	for (unsigned i = 0; i < TEST_MODE_NUM; i++) {
		if (strcmp(cmd, cmds[i].cmd) == 0) {
			if (i == TEST_MODE_HELP) {
				usage();
				return 0;
			}

			test = cmds[i].construct();
			break;
		}
	}
	if (!test) {
		LPROFILE_ERROR("Unknown command %s, usage: ", cmd);
		usage();
		return 1;
	}

	if (!test->parseArgs(argc, argv)) {
		LPROFILE_ERROR("Failed to parse arguments");
		ret = 1;
		goto delete_test;
	}

	if (!test->init()) {
		LPROFILE_ERROR("Failed to initialize test");
		ret = 1;
		goto delete_test;
	}

	if (!test->process()) {
		LPROFILE_ERROR("Failed to perform test");
		ret = 1;
	}

	test->destroy();

delete_test:
	delete test;

	return ret;
}
