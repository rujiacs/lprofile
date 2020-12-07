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
#include "CFG.h"
#include "CodeSource.h"
#include "CodeObject.h"
#include "Symbol.h"
#include "Symtab.h"

#include "test.h"
#include "simple.h"
#include "funcmap.h"
#include "count.h"
#include "util.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::SymtabAPI;

Test *SimpleTest::construct(void)
{
	return new SimpleTest();
}

SimpleTest::SimpleTest(void)
{
}

SimpleTest::~SimpleTest(void)
{
	zfree(muttee_argv);
}

bool SimpleTest::init(void)
{
	LPROFILE_DEBUG("create process %s", SIMPLE_MUTTEE);
	int i = 0;

	LPROFILE_DEBUG("process creating");
	proc = bpatch.processCreate(SIMPLE_MUTTEE, NULL);
	if (!proc) {
		LPROFILE_ERROR("Failed to create process for muttee");
		return false;
	}
	LPROFILE_DEBUG("process created");

	/* init CountUtil */
	count.setAS(proc);
	count.setPattern(SIMPLE_FUNC);

	/* find target functions */
	if (!count.getTargetFuncs(false)) {
		LPROFILE_ERROR("Failed to find target functions (%s)",
						count.getPattern().c_str());
		return false;
	}
	return true;
}

void SimpleTest::staticUsage(void)
{
	fprintf(stdout, "./lprofile %s\n",
					SIMPLE_CMD);
}

bool SimpleTest::parseArgs(LPROFILE_UNUSED int argc,
				LPROFILE_UNUSED char **argv)
{
	return true;
}

Symbol *SimpleTest::findHookSymbol(BPatch_function *func_wrap)
{
	SymtabAPI::Symtab *symtab = NULL;
	vector<SymtabAPI::Symbol *> symlist;
	int i = 0;
	ParseAPI::Function *ifunc = Dyninst::ParseAPI::convert(func_wrap);
	ParseAPI::SymtabCodeSource *src = dynamic_cast<ParseAPI::SymtabCodeSource *>(ifunc->obj()->cs());

	if (!src) {
		LPROFILE_ERROR("Error: wrapper function created from non-SymtabAPI code source");
		return NULL;
	}

	symtab = src->getSymtabObject();

	if (symtab->findSymbol(symlist, SIMPLE_HOOK_FUNC, Symbol::ST_UNKNOWN,
							anyName, false, false, true)) {
		return symlist[0];
	}
	return NULL;
}

bool SimpleTest::process(void)
{
	BPatch_object *libwrap = NULL;
	BPatch_function *func_wrap = NULL, *func_orig = NULL;
	vector<TargetFunc> target_funcs;
	SymtabAPI::Symbol *hook_sym = NULL;

	target_funcs = count.getTargets();
	if (target_funcs.size() != 1) {
		LPROFILE_ERROR("Wrong target function list");
		return false;
	}
	func_orig = target_funcs[0].func;

	LPROFILE_DEBUG("Load %s", SIMPLE_LIBWRAP);
	libwrap = proc->loadLibrary(SIMPLE_LIBWRAP);
	if (!libwrap) {
		LPROFILE_ERROR("Failed to load %s", SIMPLE_LIBWRAP);
		return false;
	}

	LPROFILE_DEBUG("Load wrap function %s", SIMPLE_WRAP_FUNC);
	func_wrap = count.findFunction(libwrap, SIMPLE_WRAP_FUNC);
	if (!func_wrap)
		return false;

	LPROFILE_DEBUG("%s: %lx", SIMPLE_WRAP_FUNC,
					(Address)func_wrap->getBaseAddr());
	LPROFILE_DEBUG("%s: %lx", SIMPLE_FUNC,
					(Address)func_orig->getBaseAddr());

	LPROFILE_DEBUG("replace function %s with %s",
					SIMPLE_FUNC, SIMPLE_WRAP_FUNC);
//	if (!proc->replaceFunction(*func_orig, *func_wrap)) {
//		LPROFILE_DEBUG("Failed to replace %s", SIMPLE_FUNC);
//		return false;
//	}

	BPatch_Vector<BPatch_point *> call_points;
	BPatch_point *hook_point = NULL;
	int i = 0;

	func_orig->getCallerPoints(call_points);
	LPROFILE_DEBUG("Find %lu callers for %s",
					call_points.size(), SIMPLE_FUNC);
	for (i = 0; i < call_points.size(); i++) {
		BPatch_point *point = call_points[i];

		if (point->getFunction()) {
			LPROFILE_DEBUG("CALL %s->%s",
							point->getFunction()->getName().c_str(),
							SIMPLE_FUNC);
		}
		if (!proc->replaceFunctionCall(*point, *func_wrap)) {
			LPROFILE_ERROR("Failed to modify function call: %s->%s",
							point->getFunction()->getName().c_str(),
							SIMPLE_FUNC);
			return false;
		}
	}
	call_points.clear();

	func_wrap->getCallPoints(call_points);
	for (i = 0; i < call_points.size(); i++) {
		BPatch_point *point = call_points[i];
		Address callee;

		if (point->getCalledFunction() == NULL) {
			hook_point = point;
			break;
		}
	}
	if (!hook_point) {
		LPROFILE_ERROR("Failed to find hook point");
		return false;
	}

	LPROFILE_DEBUG("Replace call to %s with %s", SIMPLE_HOOK_FUNC, SIMPLE_FUNC);
	if (!proc->replaceFunctionCall(*hook_point, *func_orig)) {
		LPROFILE_ERROR("Failed to replace function");
		return false;
	}

	// start the muttee
	proc->continueExecution();

	// wait for the termination of muttee
	while (!proc->isTerminated()){
		bpatch.waitForStatusChange();
	}

	if (proc->terminationStatus() == ExitedNormally) {
		LPROFILE_DEBUG("Muttee exited with code %d", proc->getExitCode());
	} else if (proc->terminationStatus() == ExitedViaSignal)  {
		LPROFILE_DEBUG("!!! Muttee exited with signal %d", proc->getExitSignal());
	} else {
		LPROFILE_DEBUG("Unknown application exit");
	}

//	// write to file
//	if (!editor->writeFile(output.c_str())) {
//		LPROFILE_ERROR("Failed to write new file %s",
//						output.c_str());
//		return false;
//	}
	return true;
}
