#include <stdio.h>
#include <sys/stat.h>

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_process.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_object.h"

#include "test.h"
#include "tracer.h"
#include "funcmap.h"
#include "util.h"

using namespace std;
using namespace Dyninst;

TracerTest::~TracerTest(void)
{
}

Test *TracerTest::construct(void)
{
	return new TracerTest();
}

TracerTest::TracerTest(void) :
		pid(-1)
{
	proc = NULL;
}

void TracerTest::staticUsage(void)
{
	fprintf(stdout, "./lprofile %s -h\t\tShow the usage.\n",
					TRACER_CMD);
	fprintf(stdout, "./lprofile %s -p pid [OPTIONS]\n"
					"OPTIONS:\n"
					"%s\n",
					TRACER_CMD, CountUtil::getUsageStr().c_str());
}

bool TracerTest::parseArgs(int argc, char **argv)
{
	int c;
	char buf[64] = {'\0'};
	string optstr = CountUtil::getOptStr() + "p:h";

	while ((c = getopt(argc, argv, optstr.c_str())) != -1) {
		switch(c) {
			case 'p':
				// check whether the process exists
				sprintf(buf, "/proc/%s", optarg);
				if (access(buf, F_OK) != 0) {
					LPROFILE_ERROR("Process %s doesn't exist.", optarg);
					return false;
				}

				pid = atoi(optarg);
				break;

			case 'h':
				staticUsage();
				exit(0);

			default:
				if (!count.parseOption(c, optarg)) {
					LPROFILE_ERROR("Unknown option %c, usage:", c);
					staticUsage();
					return false;
				}
				break;
		}
	}

	if (pid == -1) {
		LPROFILE_ERROR("No process specified, usage:");
		staticUsage();
		return false;
	}

	return true;
}

bool TracerTest::init(void)
{
	BPatch_Vector<BPatch_object *> objs;

	LPROFILE_INFO("Attaching to process %d", pid);
	proc = bpatch.processAttach(NULL, pid);
	if (proc == NULL) {
		LPROFILE_ERROR("Failed to attach to process %d", pid);
		return false;
	}

	/* init CountUtil */
	count.setAS(proc);

	/* find target functions */
	if (!count.getTargetFuncs()) {
		LPROFILE_ERROR("Failed to find target functions (%s)",
						count.getPattern().c_str());
		return false;
	}
	return true;

detach_out:
	LPROFILE_INFO("Detaching process %d", pid);
	proc->detach(true);
	return false;
}

bool TracerTest::callInit(void)
{
	BPatch_Vector<BPatch_function *> funcs;
	BPatch_Vector<BPatch_snippet *> init_arg;
	BPatch_Vector<BPatch_point *> *init_point = NULL;
	BPatchSnippetHandle *handle = NULL;
	bool ret = true;
	string target_elf;
	int init_ret = 0;

	LPROFILE_DEBUG("Insert init function");

	count.buildInitArgs(init_arg);

	BPatch_funcCallExpr init_expr(*(count.getInit()),
					init_arg);

	init_ret = (int)(uintptr_t)(proc->oneTimeCode(init_expr));
	if (init_ret) {
		LPROFILE_ERROR("Failed to execute init code, ret %d", init_ret);
		ret = false;
	}

	for (unsigned int i = 0; i < init_arg.size(); i++) {
		delete init_arg[i];
	}
	return ret;
}

BPatch_module *TracerTest::findMainModule(void)
{
	BPatch_Vector<BPatch_function *> funcs;

	proc->getImage()->findFunction("^main", funcs);
	if (funcs.size() == 0) {
		LPROFILE_ERROR("Cannot find main()");
		return NULL;
	}

	return funcs[0]->getModule();
}

bool TracerTest::insertExit(void)
{
	BPatch_Vector<BPatch_snippet *> exit_arg;
	BPatch_Vector<BPatch_point *> *exit_point = NULL;
	BPatchSnippetHandle *handle = NULL;
	BPatch_Vector<BPatch_function *> funcs;
	BPatch_module *mainmod = NULL;

	BPatch_funcCallExpr exit_expr(*(count.getExit()), exit_arg);

	/* insert global exiting function */
	proc->getImage()->findFunction("^_fini", funcs);
	mainmod = findMainModule();
	if (mainmod == NULL) {
		LPROFILE_ERROR("Failed to find global exit point");
		return false;
	}

	for (unsigned int i = 0; i < funcs.size(); i++) {
		BPatch_module *mod = funcs[i]->getModule();

		if (mod != mainmod)
			continue;

		LPROFILE_INFO("Insert exit function to %s",
						mod->getObject()->name().c_str());
		handle = mod->insertFiniCallback(exit_expr);
		if (!handle) {
			LPROFILE_ERROR("Failed to insert exit function to %s",
							mod->getObject()->name().c_str());
			return false;
		}

		count.addTargetFunc(funcs[i], UINT_MAX,
								mod->getObject()->name().c_str(), false);
		break;
	}
	return true;
}

bool TracerTest::process(void)
{
	// load counting functions
	if (!count.loadFunctions()) {
		LPROFILE_ERROR("Failed to load counting functions");
		return false;
	}

	// insert init function
	if (!callInit()) {
		LPROFILE_ERROR("Failed to call init function");
		return false;
	}

	// insert counter
	if (!count.insertCount()) {
		LPROFILE_ERROR("Failed to insert counting functions");
		return false;
	}

	// insert exit function
	if (!insertExit()) {
		LPROFILE_ERROR("Failed to insert exit function");
		return false;
	}

	// continue the tracee
	if (!proc->continueExecution()) {
		LPROFILE_ERROR("Failed to continue execution");
		return false;
	}

	// wait for the termination of muttee
	while (!proc->isTerminated()){
		bpatch.waitForStatusChange();
	}

	// wait for termination of tracee
	if (proc->terminationStatus() == ExitedNormally) {
		LPROFILE_INFO("Muttee exited with code %d", proc->getExitCode());
	} else if (proc->terminationStatus() == ExitedViaSignal)  {
		LPROFILE_INFO("!!! Muttee exited with signal %d",
						proc->getExitSignal());
	} else {
		LPROFILE_INFO("Unknown application exit");
	}

	return true;
}

void TracerTest::clearSnippets(void)
{
	if (proc->isTerminated())
		return;

	while (!proc->isStopped()) {
		LPROFILE_INFO("Try to stop the tracee");
		proc->stopExecution();
	}

	LPROFILE_INFO("Clear all snippets");
	count.clearSnippet();
}

void TracerTest::destroy(void)
{
	if (!proc || proc->isTerminated())
		return;

	clearSnippets();

	LPROFILE_INFO("Detach process %d", pid);
	proc->detach(true);
}