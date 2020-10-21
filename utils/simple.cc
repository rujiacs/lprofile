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
	if (!count.getTargetFuncs()) {
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
	std::vector<TargetFunc> target_funcs;
	SymtabAPI::Symbol *hook_sym = NULL;

	target_funcs = count.getTargetLists();
	if (target_funcs.size() != 1) {
		LPROFILE_ERROR("Wrong target function list");
		return false;
	}
	func_orig = target_funcs[0].func;

	LPROFILE_INFO("Load %s", SIMPLE_LIBWRAP);
	libwrap = proc->loadLibrary(SIMPLE_LIBWRAP);
	if (!libwrap) {
		LPROFILE_ERROR("Failed to load %s", SIMPLE_LIBWRAP);
		return false;
	}

	LPROFILE_INFO("Load wrap function %s", SIMPLE_WRAP_FUNC);
	func_wrap = count.findFunction(libwrap, SIMPLE_WRAP_FUNC);
	if (!func_wrap)
		return false;

	hook_sym = findHookSymbol(func_wrap);

	if (!hook_sym) {
		LPROFILE_ERROR("Symbol for hook %s is not found", SIMPLE_HOOK_FUNC);
		return false;
	}
	LPROFILE_INFO("Find hook symbol %s:%lx",
					hook_sym->getMangledName().c_str(),
					hook_sym->getOffset());

	LPROFILE_INFO("Replace %s with %s, orig code is stored in symbol %s",
					SIMPLE_FUNC, SIMPLE_WRAP_FUNC, SIMPLE_HOOK_FUNC);
	if (!proc->wrapFunction(func_orig, func_wrap, hook_sym)) {
		LPROFILE_ERROR("Failed to replace function");
		return false;
	}
//
//	LPROFILE_INFO("Load hook function %s", SIMPLE_HOOK_FUNC);
//	func_hook = count.findFunction(libwrap, SIMPLE_HOOK_FUNC);
//	if (!func_hook)
//		return false;
//
//	LPROFILE_INFO("%s: %lx", SIMPLE_WRAP_FUNC,
//					(Address)func_wrap->getBaseAddr());
//	LPROFILE_INFO("%s: %lx", SIMPLE_FUNC,
//					(Address)func_orig->getBaseAddr());
//	LPROFILE_INFO("%s: %lx", SIMPLE_HOOK_FUNC,
//					(Address)func_hook->getBaseAddr());
//
//	BPatch_Vector<BPatch_point *> call_points;
//	BPatch_point *hook_point = NULL;
//	int i = 0;
//
//	func_wrap->getCallPoints(call_points);
//	for (i = 0; i < call_points.size(); i++) {
//		BPatch_point *point = call_points[i];
//		Address callee;
//
//		callee = (Address)point->getCalledFunction()->getBaseAddr();
//		LPROFILE_INFO("Call point: %lx", callee);
//		if (callee == (Address)func_hook->getBaseAddr()) {
//			hook_point = point;
//			break;
//		}
//	}
//	if (!hook_point) {
//		LPROFILE_ERROR("Failed to find hook point");
//		return false;
//	}
//
//	LPROFILE_INFO("Replace call to %s with %s", SIMPLE_HOOK_FUNC, SIMPLE_FUNC);
//	if (!proc->replaceFunctionCall(*hook_point, *func_orig)) {
//		LPROFILE_ERROR("Failed to replace function");
//		return false;
//	}

	// start the muttee
	proc->continueExecution();

	// wait for the termination of muttee
	while (!proc->isTerminated()){
		bpatch.waitForStatusChange();
	}

	if (proc->terminationStatus() == ExitedNormally) {
		LPROFILE_INFO("Muttee exited with code %d", proc->getExitCode());
	} else if (proc->terminationStatus() == ExitedViaSignal)  {
		LPROFILE_INFO("!!! Muttee exited with signal %d", proc->getExitSignal());
	} else {
		LPROFILE_INFO("Unknown application exit");
	}

//	// write to file
//	if (!editor->writeFile(output.c_str())) {
//		LPROFILE_ERROR("Failed to write new file %s",
//						output.c_str());
//		return false;
//	}
	return true;
}
