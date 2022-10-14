#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_object.h"

#include "test.h"
#include "prof.h"
#include "funcmap.h"
#include "count.h"
#include "util.h"

using namespace std;
using namespace Dyninst;

Test *ProfTest::construct(void)
{
	return new ProfTest();
}

ProfTest::ProfTest(void)
{
	mode = LPROFILE_MODE_PROC;
}

ProfTest::~ProfTest(void)
{
	if (mode == LPROFILE_MODE_PROC)
		zfree(muttee_argv);
}

bool ProfTest::init(void)
{
	if (mode == LPROFILE_MODE_PROC) {	
		LPROFILE_DEBUG("create process %s, %d args",
						muttee_bin.c_str(), muttee_argc);

		LPROFILE_DEBUG("process creating");
		as = bpatch.processCreate(
						muttee_bin.c_str(), muttee_argv);
		if (!as) {
			LPROFILE_ERROR("Failed to create process for muttee");
			return false;
		}
	}
	else {
		LPROFILE_DEBUG("Open binary file %s",
						muttee_bin.c_str());
		as = bpatch.openBinary(muttee_bin.c_str());
		if (!as) {
			LPROFILE_ERROR("Failed tp open elf file");
			return false;
		}
	}
	/* init CountUtil */
	count.setAS(as);

	/* find target functions */
	if (!count.getTargetFuncs()) {
		LPROFILE_ERROR("Failed to find target functions (%s)",
						count.getPattern().c_str());
		return false;
	}
	return true;
}

void ProfTest::staticUsage(void)
{
	fprintf(stdout, "./lprofile %s -h\t\tShow the usage.\n",
					PROF_CMD);
	fprintf(stdout, "./lprofile %s [OPTIONS] <muttee> [<muttee_arg>]\n"
					"OPTIONS:\n"
					"\t-w output file (and path)\n%s\n",
					PROF_CMD, CountUtil::getUsageStr().c_str());
}

bool ProfTest::parseArgs(int argc, char **argv)
{
	int c;
	char buf[64] = {'\0'};
	string optstr = CountUtil::getOptStr() + "w:h";

	while ((c = getopt(argc, argv, optstr.c_str())) != -1) {
		switch(c) {
			case 'w':
				mode = LPROFILE_MODE_EDIT;
				output = optarg;
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

	argc -= optind;
	argv += optind;

	if (argc <= 0) {
		LPROFILE_ERROR("Wrong options");
		staticUsage();
		return false;
	}

	muttee_bin = argv[0];
	// check if muttee is valid.
	if (access(muttee_bin.c_str(), X_OK) < 0) {
		LPROFILE_ERROR("Muttee %s is invalid, err %d",
						muttee_bin.c_str(), errno);
		return false;
	}
	LPROFILE_DEBUG("Muttee %s", muttee_bin.c_str());

	if (mode == LPROFILE_MODE_EDIT) {
		muttee_argv = NULL;
		muttee_argc = 0;
		return true;
	}

	muttee_argc = argc;

	if (argc > 0) {
		muttee_argv = (const char **)malloc(sizeof(char*) * (argc+1));
		if (!muttee_argv) {
			LPROFILE_ERROR("Failed to allocate memory for muttee_argv");
			return false;
		}

		for (c = 0; c < argc; c++) {
			muttee_argv[c] = argv[c];
			LPROFILE_DEBUG("Muttee argv[%d]=%s", c, muttee_argv[c]);
		}
		muttee_argv[argc] = NULL;
	}
	else
		muttee_argv = NULL;

	return true;
}

BPatch_module *ProfTest::findMainModule(void)
{
	BPatch_Vector<BPatch_function *> funcs;

	as->getImage()->findFunction("^main", funcs);
	if (funcs.size() == 0) {
		LPROFILE_ERROR("Cannot find main()");
		return NULL;
	}

	return funcs[0]->getModule();
}

bool ProfTest::insertExit(void)
{
	BPatch_Vector<BPatch_snippet *> exit_arg;
	BPatch_Vector<BPatch_point *> *exit_point = NULL;
	BPatchSnippetHandle *handle = NULL;
	BPatch_Vector<BPatch_function *> funcs;
	BPatch_module *mainmod = NULL;
	string target_elf;

	BPatch_funcCallExpr exit_expr(*(count.getExit()), exit_arg);

	/* insert global exiting function */
	as->getImage()->findFunction("^_fini", funcs);
	
	if (mode == LPROFILE_MODE_PROC) {
		mainmod = findMainModule();
		if (mainmod == NULL) {
			LPROFILE_ERROR("Failed to find global exit point");
			return false;
		}
	}
	else {
		target_elf = muttee_bin.substr(muttee_bin.find_last_of('/') + 1);
	}

	for (unsigned int i = 0; i < funcs.size(); i++) {
		BPatch_module *mod = funcs[i]->getModule();

		if (mode == LPROFILE_MODE_PROC && mod != mainmod)
			continue;
		if (mode == LPROFILE_MODE_EDIT) {
			if (target_elf.compare(mod->getObject()->name()))
				continue;
		}

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

bool ProfTest::insertInit(void)
{
	BPatch_Vector<BPatch_function *> funcs;
	BPatch_Vector<BPatch_snippet *> init_arg;
	BPatch_Vector<BPatch_point *> *init_point = NULL;
	BPatchSnippetHandle *handle = NULL;
	bool ret = true;
	string target_elf;

	LPROFILE_DEBUG("Insert init function");

	count.buildInitArgs(init_arg);

	BPatch_funcCallExpr init_expr(*(count.getInit()),
					init_arg);

	if (mode == LPROFILE_MODE_PROC) {
		BPatch_process *proc = (BPatch_process *)as;
		int initret = 0;

		initret = (int)(uintptr_t)(proc->oneTimeCode(init_expr));
		if (initret) {
			LPROFILE_ERROR("Failed to execute init code, ret %d", initret);
			ret = false;
			goto free_arg;
		}
	}
	else {
		BPatch_binaryEdit *editor = (BPatch_binaryEdit *)as;

		editor->getImage()->findFunction("^_init", funcs);
		if (funcs.size() == 0) {
			LPROFILE_ERROR("No _init function is found.");
			ret = false;
			goto free_arg;
		}

		target_elf = muttee_bin.substr(muttee_bin.find_last_of('/') + 1);
		LPROFILE_DEBUG("Target ELF %s", target_elf.c_str());

		for (unsigned int i = 0; i < funcs.size(); i++) {
			BPatch_module *mod = funcs[i]->getModule();

			if (target_elf.compare(mod->getObject()->name()))
				continue;

			LPROFILE_INFO("Insert init function to %s",
							mod->getObject()->name().c_str());
			handle = mod->insertInitCallback(init_expr);
			if (!handle) {
				LPROFILE_ERROR("Failed to insert init function to %s",
								mod->getObject()->name().c_str());
				ret = false;
				goto free_arg;
			}
			count.addTargetFunc(funcs[i], UINT_MAX,
								mod->getObject()->name().c_str(), false);
			break;
		}
	}
	

free_arg:
	for (unsigned int i = 0; i < init_arg.size(); i++) {
		delete init_arg[i];
	}
	return ret;
}

bool ProfTest::process(void)
{
	// load counting functions
	if (!count.loadFunctions()) {
		LPROFILE_ERROR("Failed to load counting functions");
		return false;
	}

	// insert init function
	if (!insertInit()) {
		LPROFILE_ERROR("Failed to insert init function");
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

	if (!count.wrapPthreadFunc()) {
		LPROFILE_ERROR("Failed to wrap pthread functions");
		return false;
	}

	if (mode == LPROFILE_MODE_PROC) {
		BPatch_process *proc = (BPatch_process *)as;

		// start the muttee
		proc->continueExecution();

		// wait for the termination of muttee
		while (!proc->isTerminated()){
			bpatch.waitForStatusChange();
		}

		if (proc->terminationStatus() == ExitedNormally) {
			LPROFILE_INFO("Muttee exited with code %d", proc->getExitCode());
		} else if (proc->terminationStatus() == ExitedViaSignal)  {
			LPROFILE_INFO("!!! Muttee exited with signal %d",
							proc->getExitSignal());
		} else {
			LPROFILE_INFO("Unknown application exit");
		}
	}
	else {
		BPatch_binaryEdit *edit = (BPatch_binaryEdit *)as;	

		if (!edit->writeFile(output.c_str())) {
			LPROFILE_ERROR("Failed to write new file %s",
								output.c_str());
			return false;
		}
	}
	return true;
}
