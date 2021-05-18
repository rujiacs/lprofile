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

MutteeFunc::MutteeFunc(BPatch_function *f, BPatch_object *o, unsigned i)
		: func(f), obj(o), index(i)
{
	is_shared = func->isSharedLib();
}


Test *SimpleTest::construct(void)
{
	return new SimpleTest();
}

SimpleTest::SimpleTest(void)
{
}

SimpleTest::~SimpleTest(void)
{
// 	zfree(muttee_argv);
}

static const char *syslib[] = {
	"libc.so",
	"libdl.so",
	"libpthread.so",
	NULL
};

bool SimpleTest::isSystemLib(BPatch_object *obj)
{
	if (obj->isSystemLib())
		return true;

	int i = 0;
	const char *str = syslib[i];

	while (str != NULL) {
		if (strncmp(str, obj->name().c_str(), strlen(str)) == 0)
			return true;
		i++;
		str = syslib[i];
	}
	return false;
}

void SimpleTest::findTarget(const char *targ)
{
	vector<BPatch_module *> mods;
	vector<BPatch_function *> funcs;

	proc->getImage()->getModules(mods);

	for (size_t i = 0; i < mods.size(); i++) {
		if (isSystemLib(mods[i]->getObject()))
			continue;

		LPROFILE_INFO("Searching obj %s",
						mods[i]->getObject()->name().c_str());

		funcs.clear();
		mods[i]->getProcedures(funcs, true);
		
		for (size_t j = 0; j < funcs.size(); j++) {
			LPROFILE_INFO("%s", funcs[j]->getName().c_str());
		}

		// allobjs[i]->findFunction(targ, funcs, false);

		// for (size_t j = 0; j < funcs.size(); j++) {
		// 	targets.push_back(MutteeFunc(funcs[j], allobjs[i]));

		// 	LPROFILE_INFO("Find target function %s in %s, shared %u",
		// 					targets[targets.size() - 1].func->getName().c_str(),
		// 					targets[targets.size() - 1].obj->name().c_str(),
		// 					targets[targets.size() - 1].is_shared);
		// }
	}
}

bool SimpleTest::findTargets(void)
{
	string pattern(SIMPLE_FUNC);
	size_t pos = 0, cur = 0;

	pos = pattern.find(",");
	while (true) {
		string substr = pattern.substr(cur, (pos - cur));

		LPROFILE_INFO("pattern %s", substr.c_str());

		findTarget(substr.c_str());
		if (pos == string::npos ||
						(pos + 1) >= pattern.size())
			break;

		cur = pos + 1;
		pos = pattern.find(",", cur);
	}

	if (targets.size() == 0) {
		LPROFILE_ERROR("No function matches the target pattern %s",
						pattern.c_str());
		return false;
	}
	return true;
}

bool SimpleTest::init(void)
{
	LPROFILE_INFO("create process %s", SIMPLE_MUTTEE);
	int i = 0;

	LPROFILE_INFO("process creating");
	proc = bpatch.processCreate(SIMPLE_MUTTEE, NULL);
	if (!proc) {
		LPROFILE_ERROR("Failed to create process for muttee");
		return false;
	}
	LPROFILE_INFO("process created");

	if (!findTargets()) {
		LPROFILE_ERROR("Failed to find target functions %s",
						SIMPLE_FUNC);
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

SymtabAPI::Symbol *SimpleTest::findHookSymbol(BPatch_function *func_wrap, string orgname)
{

	SymtabAPI::Symtab *symtab = NULL;
	vector<SymtabAPI::Symbol *> symlist;
	int i = 0;
	ParseAPI::Function *ifunc = Dyninst::ParseAPI::convert(func_wrap);
	ParseAPI::SymtabCodeSource *src = dynamic_cast<ParseAPI::SymtabCodeSource *>(ifunc->obj()->cs());
	string symname = "__" + orgname;

	if (!src) {
		LPROFILE_ERROR("Error: wrapper function created from non-SymtabAPI code source");
		return NULL;
	}

	symtab = src->getSymtabObject();

	if (symtab->findSymbol(symlist, symname, Symbol::ST_UNKNOWN,
							anyName, false, false, true)) {
		return symlist[0];
	}

	LPROFILE_ERROR("Failed to find hook symbol %s", symname.c_str());
	return NULL;
}


void SimpleTest::findCaller(MutteeFunc *tfunc)
{
	vector<BPatch_point *> caller_points;
	size_t i = 0;

	tfunc->func->getCallerPoints(caller_points);
	LPROFILE_INFO("Find %lu callers for %s",
					caller_points.size(), SIMPLE_FUNC);
	for (i = 0; i < caller_points.size(); i++) {
		BPatch_point *point = caller_points[i];

		if (point->getFunction()) {
			LPROFILE_INFO("CALL %s->%s",
							point->getFunction()->getName().c_str(),
							SIMPLE_FUNC);
		}
//		if (!proc->replaceFunctionCall(*point, *func_wrap)) {
//			LPROFILE_ERROR("Failed to modify function call: %s->%s",
//							point->getFunction()->getName().c_str(),
//							SIMPLE_FUNC);
//			return false;
//		}
	}
}

bool SimpleTest::process(void)
{
#if 0
	BPatch_object *libwrap = NULL;
	size_t i = 0;

	LPROFILE_INFO("Load %s", SIMPLE_LIBWRAP);
	libwrap = proc->loadLibrary(SIMPLE_LIBWRAP);
	if (!libwrap) {
		LPROFILE_ERROR("Failed to load %s", SIMPLE_LIBWRAP);
		return false;
	}

	for (i = 0; i < targets.size(); i++) {
		string wrap_str = "wrap_";
		BPatch_function *func_orig = targets[i].func;
		BPatch_function *func_wrap = NULL;
		vector<BPatch_function *> tfuncs;
		Symbol *hooksym = NULL;

		wrap_str += func_orig->getName();
		libwrap->findFunction(wrap_str, tfuncs, false);
		if (tfuncs.size() == 0) {
			LPROFILE_ERROR("Failed to find wrapper %s", wrap_str.c_str());
			continue;
		}

		func_wrap = tfuncs[0];

		hooksym = findHookSymbol(func_wrap, func_orig->getName());
		if (!hooksym)
			continue;

		if (!proc->replaceCallee(func_orig, func_wrap, hooksym)) {
			LPROFILE_ERROR("Failed to replace callee (%s->%s)",
							func_orig->getName().c_str(),
							func_wrap->getName().c_str());
		}
	}

#if 0
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

	LPROFILE_INFO("Load wrap function %s", SIMPLE_WRAP_FUNC);
	func_wrap = count.findFunction(libwrap, SIMPLE_WRAP_FUNC);
	if (!func_wrap)
		return false;

	LPROFILE_INFO("%s: %lx", SIMPLE_WRAP_FUNC,
					(Address)func_wrap->getBaseAddr());
	LPROFILE_INFO("%s: %lx", SIMPLE_FUNC,
					(Address)func_orig->getBaseAddr());

	LPROFILE_INFO("replace function %s with %s",
					SIMPLE_FUNC, SIMPLE_WRAP_FUNC);
//	if (!proc->replaceFunction(*func_orig, *func_wrap)) {
//		LPROFILE_INFO("Failed to replace %s", SIMPLE_FUNC);
//		return false;
//	}

	BPatch_Vector<BPatch_point *> call_points;
	BPatch_point *hook_point = NULL;
	int i = 0;

	func_orig->getCallerPoints(call_points);
	LPROFILE_INFO("Find %lu callers for %s",
					call_points.size(), SIMPLE_FUNC);
	for (i = 0; i < call_points.size(); i++) {
		BPatch_point *point = call_points[i];

		if (point->getFunction()) {
			LPROFILE_INFO("CALL %s->%s",
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

	LPROFILE_INFO("Replace call to %s with %s", SIMPLE_HOOK_FUNC, SIMPLE_FUNC);
	if (!proc->replaceFunctionCall(*hook_point, *func_orig)) {
		LPROFILE_ERROR("Failed to replace function");
		return false;
	}
#endif


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
#endif
//	// write to file
//	if (!editor->writeFile(output.c_str())) {
//		LPROFILE_ERROR("Failed to write new file %s",
//						output.c_str());
//		return false;
//	}
	return true;
}
