#include <stdio.h>
#include <sys/stat.h>

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_object.h"

#include "count.h"
#include "funcmap.h"
#include "util.h"

using namespace std;
using namespace Dyninst;

TargetFunc::TargetFunc(void) :
		func(NULL), index(UINT_MAX)
{
}

TargetFunc::TargetFunc(BPatch_function *f, unsigned int i) :
		func(f), index(i)
{
}

bool CountUtil::getTargetFuncs(void)
{
	BPatch_Vector<BPatch_object *> objs;

	getUserObjs(objs);

	for (unsigned i = 0; i < objs.size(); i++) {
		FuncMap fmap(objs[i]->pathName().c_str());

		LPROFILE_INFO("Load function map for %s",
						objs[i]->pathName().c_str());
		if (!fmap.load(false)) {
			LPROFILE_ERROR("Failed to load function map for %s",
							objs[i]->pathName().c_str());
			return false;
		}
		matchFuncs(objs[i], &fmap, pattern);
	}

	if (target_funcs.size() == 0) {
		LPROFILE_ERROR("Failed to find functions %s", pattern.c_str());
		return false;
	}

	LPROFILE_INFO("Found %lu functions for %s",
					target_funcs.size(), pattern.c_str());
	return true;
}

void CountUtil::addTargetFunc(BPatch_function *func, unsigned idx)
{
	target_funcs.push_back(TargetFunc(func, idx));
}

void CountUtil::matchFuncs(BPatch_object *obj,
				FuncMap *fmap, string pattern)
{
	BPatch_Vector<BPatch_function *> tfuncs;

	obj->findFunction(pattern.c_str(), tfuncs, false);
	for (unsigned j = 0; j < tfuncs.size(); j++) {
		unsigned int index = UINT_MAX;

		index = fmap->getFunctionID(tfuncs[j]->getName());
		if (index == UINT_MAX) {
			LPROFILE_ERROR("Cannot find %s in map %s",
							tfuncs[j]->getName().c_str(),
							obj->name().c_str());
			continue;
		}

		target_funcs.push_back(TargetFunc(tfuncs[j], index));
		LPROFILE_INFO("Target function %s:%s, ID %u",
						obj->name().c_str(),
						tfuncs[j]->getName().c_str(), index);
	}
}

#define OSLIBPATH_PREFIX "/lib/"
#define DYNINSTLIB_PREFIX "/home/rujia/project/profile/lprofile/build/lprofile/dyninst/lib/"
void CountUtil::getUserObjs(BPatch_Vector<BPatch_object *> &objs)
{
	BPatch_object *obj = NULL;
	string osprefix(OSLIBPATH_PREFIX);
	string diprefix(DYNINSTLIB_PREFIX);
	BPatch_Vector<BPatch_object *> allobjs;

	as->getImage()->getObjects(allobjs);
	for (unsigned i = 0; i < allobjs.size(); i++) {
		std::string path;

		obj = allobjs[i];
		path = obj->pathName();

		if (!path.compare(0, osprefix.size(), osprefix) ||
						!path.compare(0, diprefix.size(), diprefix)) {
			LPROFILE_INFO("Skip object %s", obj->name().c_str());
			continue;
		}
		objs.push_back(obj);
	}
}

bool CountUtil::insertCount(void)
{
	unsigned int fun_id = 0;

	for (unsigned i = 0; i < target_funcs.size(); i++) {
		TargetFunc *tf = &target_funcs[i];
		vector<BPatch_point *> *pentry = NULL, *pexit = NULL;
		BPatchSnippetHandle *handle = NULL;

		if (tf->index == UINT_MAX)
			continue;

		// find entry point
		pentry = tf->func->findPoint(BPatch_entry);
		if (!pentry) {
			LPROFILE_ERROR("Failed to find entry point of func %s",
							tf->func->getName().c_str());
			continue;
		}

		// find exit point
		pexit = tf->func->findPoint(BPatch_exit);
		if (!pexit) {
			LPROFILE_ERROR("Failed to find exit point of func %s",
							tf->func->getName().c_str());
			continue;
		}

		// create snippet
		vector<BPatch_snippet *> cnt_arg;
		BPatch_constExpr id(tf->index);

		cnt_arg.push_back(&id);

		BPatch_funcCallExpr pre_expr(*func_pre, cnt_arg);
		BPatch_funcCallExpr post_expr(*func_post, cnt_arg);

		// insert counting functions
		LPROFILE_INFO("Insert %s into %s", FUNC_PRE,
						tf->func->getName().c_str());
		handle = as->insertSnippet(
						pre_expr, *pentry, BPatch_callBefore);
		if (!handle) {
			LPROFILE_ERROR("Failed to insert func_pre to %s",
							tf->func->getName().c_str());
			return false;
		}

		LPROFILE_INFO("Insert %s into %s", FUNC_POST,
						tf->func->getName().c_str());
		handle = as->insertSnippet(
						post_expr, *pexit, BPatch_callAfter);
		if (!handle) {
			LPROFILE_ERROR("Failed to insert post_cnt to %s",
							tf->func->getName().c_str());
			return false;
		}
	}

	return true;
}

bool CountUtil::loadFunctions(void)
{
	BPatch_object *libcnt = NULL;

	// load lib
	LPROFILE_INFO("Load %s", LIBCNT);
	libcnt = as->loadLibrary(LIBCNT);
	if (!libcnt) {
		LPROFILE_ERROR("Failed to load %s", LIBCNT);
		return false;
	}

	// load functions
	LPROFILE_INFO("Load counting functions");
	func_pre = findFunction(libcnt, FUNC_PRE);
	func_post = findFunction(libcnt, FUNC_POST);
	if (!func_pre || !func_post) {
		LPROFILE_ERROR("Failed to load counting functions");
		return false;
	}

#ifndef USE_FUNCCNT
	LPROFILE_INFO("Load init function");
	func_init = findFunction(libcnt, FUNC_INIT);
	if (!func_init) {
		LPROFILE_ERROR("Failed to load init function");
		return false;
	}

	LPROFILE_INFO("Load exit function");
	func_exit = findFunction(libcnt, FUNC_EXIT);
	func_texit = findFunction(libcnt, FUNC_TEXIT);
	if (!func_exit || !func_texit) {
		LPROFILE_ERROR("Failed to load destroy function");
		return false;
	}
#endif
	return true;
}

BPatch_function *CountUtil::findFunction(
				BPatch_object *obj, string name)
{
	BPatch_Vector<BPatch_function *> tfuncs;

	obj->findFunction(name, tfuncs, false);
	if (tfuncs.size() == 0) {
		LPROFILE_ERROR("Cannot find fucntion %s in %s",
						name.c_str(), obj->pathName().c_str());
		return NULL;
	}

	return tfuncs[0];
}

#ifndef USE_FUNCCNT
bool CountUtil::insertExit(string filter)
{
	BPatch_Vector<BPatch_snippet *> exit_arg;
	BPatch_Vector<BPatch_point *> *exit_point = NULL;
	BPatchSnippetHandle *handle = NULL;
	BPatch_Vector<BPatch_function *> funcs;

	BPatch_funcCallExpr exit_expr(*func_exit, exit_arg);

	/* insert global exiting function */
	as->getImage()->findFunction("^_fini", funcs);
	for (unsigned int i = 0; i < funcs.size(); i++) {
		BPatch_module *mod = funcs[i]->getModule();

		if (filter.size() > 0 &&
				filter.compare(mod->getObject()->name().c_str()))
			continue;

		LPROFILE_INFO("Insert exit function to %s",
						mod->getObject()->name().c_str());
		handle = mod->getObject()->insertFiniCallback(exit_expr);
		if (!handle) {
			LPROFILE_ERROR("Failed to insert exit function to %s",
							mod->getObject()->name().c_str());
			return false;
		}
		target_funcs.push_back(TargetFunc(funcs[i], UINT_MAX));
	}

	/* insert per-thread exiting function */
	funcs.clear();
	as->getImage()->findFunction("^pthread_exit", funcs);
	if (funcs.size() == 0) {
		LPROFILE_INFO("No pthread_exit found");
		return true;
	}

	exit_point = funcs[0]->findPoint(BPatch_entry);
	if (!exit_point) {
		LPROFILE_ERROR("Failed to find point for pthread_exit");
		return true;
	}

	BPatch_funcCallExpr thread_exit_expr(*func_texit, exit_arg);

	handle = as->insertSnippet(thread_exit_expr,
					*exit_point, BPatch_callBefore);
	if (!handle) {
		LPROFILE_ERROR("Failed to insert thread exit function");
		return false;
	}

	return true;
}
#else
bool CountUtil::insertExit(LPROFILE_UNUSED string filter)
{
	return true;
}
#endif // USE_FUNCCNT

void CountUtil::calculateRange(void)
{
	unsigned i = 0, max = 0, min = UINT_MAX, idx = 0;

	for (i = 0; i < target_funcs.size(); i++) {
		idx = target_funcs[i].index;

		if (idx == UINT_MAX)
			continue;
		if (max < idx)
			max = idx;
		if (min > idx)
			min = idx;
	}

	func_id_range.min = min;
	func_id_range.max = max;
}

string CountUtil::getOptStr(void)
{
	return string(FUNCC_ARG);
}

string CountUtil::getUsageStr(void)
{
	string usage;

	usage = "\t-f <function_pattern>\n"
			"\t\tDefine the matching pattern (regex) for\n"
			"\t\tmonitored functions. Default is \"(.*)\",\n"
			"\t\tmatching all functions.\n"
#ifndef USE_FUNCCNT
			"\t-o <output_data_file>\n"
			"\t\tDefine the prefix of the name of the output\n"
			"\t\tdata file. The actual data file is per-thread,\n"
			"\t\tnamed as the form of <output_data_file>.<pid>.\n"
			"\t\tDefault is 'profile.data'\n"
			"\t-e <event_list>\n"
			"\t\tDefine a list of performance events to be\n"
			"\t\tmonitored. It follows the same syntax as Perf.\n"
			"\t\tYou can use the command 'perf list' of Perf to\n"
			"\t\tcheck supported events in your environment.\n"
			"\t\tDefault is 'cpu-cycles'\n".
			"\t-l <log_file>\n"
			"\t\tDefine the prefix of the log file. The actual\n"
			"\t\tlog file is per-thread, named as the form of\n"
			"\t\t<log_file>.<pid>. Default is 'profile.log'\n"
			"\t-F <sample_frequency>\n"
			"\t\tDefine the sample frequency of the monitoring.\n"
			"\t\tThe value must be an integer. For each monitored\n"
			"\t\tfunction, the tool records one execution per\n"
			"\t\t<sample_frequency> executions. Default is zero,\n"
			"\t\tthat means no sampling.\n"
#endif /* ifndef USE_FUNCCNT */
			;
	return usage;
}

bool CountUtil::parseOption(int opt, char *optarg)
{
	switch(opt) {
		/* function pattern */
		case 'f':
			pattern = optarg;
			break;

#ifndef USE_FUNCCNT
		/* Output file */
		case 'o':
			output = optarg;
			break;

		/* event list */
		case 'e':
			evlist = optarg;
			break;

		case 'F':
			freq = (unsigned)atoi(optarg);
			if (!freq) {
				LPROFILE_ERROR("Failed to parse sample frequency %s",
								optarg);
				return false;
			}
			break;

		case 'l':
			logfile = optarg;
			break;
#endif /* ifndef USE_FUNCCNT */
		default:
			return false;
	}
	return true;
}

#ifdef USE_FUNCCNT
void CountUtil::buildInitArgs(
				LPROFILE_UNUSED vector<BPatch_snippet *> &args)
{
}

#else
void CountUtil::buildInitArgs(vector<BPatch_snippet *> &args)
{
	if (evlist.size() == 0)
		evlist = EVLIST_DEF;
	args.push_back(new BPatch_constExpr(evlist));

	if (logfile.size() == 0)
		logfile = LOGFILE_DEF;
	args.push_back(new BPatch_constExpr(logfile));

	calculateRange();

	LPROFILE_DEBUG("min %u, max %u",
					func_id_range.min, func_id_range.max);
	args.push_back(new BPatch_constExpr(func_id_range.min));
	args.push_back(new BPatch_constExpr(func_id_range.max));

	args.push_back(new BPatch_constExpr(freq));
}
#endif // USE_FUNCCNT
