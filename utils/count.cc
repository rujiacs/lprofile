#include <stdio.h>
#include <sys/stat.h>

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_object.h"
#include "CodeSource.h"
#include "CodeObject.h"
#include "Symbol.h"
#include "Symtab.h"

#include "count.h"
#include "funcmap.h"
#include "util.h"

using namespace std;
using namespace Dyninst;

TargetFunc::TargetFunc(void) :
		func(NULL), index(UINT_MAX)
{
}

TargetFunc::TargetFunc(BPatch_function *f, unsigned int i, string e, bool b) :
		func(f), index(i), elf(e), is_user(b)
{
}

bool CountUtil::getTargetFuncs(bool is_wrap)
{
	BPatch_Vector<BPatch_object *> objs;

	getUserObjs(objs);

	for (unsigned i = 0; i < objs.size(); i++) {
		FuncMap fmap(objs[i]->pathName().c_str(), is_wrap);

		LPROFILE_INFO("Load function map for %s",
						objs[i]->pathName().c_str());
		if (!fmap.load(true)) {
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

void CountUtil::clearSnippet()
{
	size_t i = 0;

	for (i = 0; i < target_funcs.size(); i++) {
		target_funcs[i].func->removeInstrumentation(false);
	}
}

void CountUtil::addTargetFunc(BPatch_function *func, unsigned idx,
		std::string obj, bool user)
{
	elfmap_t::iterator it;

	it = elf_targets.find(obj);
	if (it == elf_targets.end()) {
		std::pair<elfmap_t::iterator, bool> ret;

		ret = elf_targets.insert(std::pair<std::string, vector<size_t>>(obj,
											std::vector<size_t>()));
		it = ret.first;
		LPROFILE_DEBUG("new target elf %s", obj.c_str());
	}

	target_funcs.push_back(TargetFunc(func, idx, obj, user));
	it->second.push_back(target_funcs.size() - 1);
}

void CountUtil::matchFuncs(BPatch_object *obj,
				FuncMap *fmap, string pattern)
{
	size_t pos = 0, cur = 0;

	pos = pattern.find(",");
	while (true) {
		string substr = pattern.substr(cur, (pos - cur));

		LPROFILE_INFO("pattern %s", substr.c_str());

		matchFunc(obj, fmap, substr.c_str());
		if (pos == string::npos ||
						(pos + 1) >= pattern.size())
			break;

		cur = pos + 1;
		pos = pattern.find(",", cur);
	}
}

void CountUtil::matchFunc(BPatch_object *obj,
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

		addTargetFunc(tfuncs[j], index, obj->name(), true);
	}

	elfmap_t::iterator it;
	for (it = elf_targets.begin(); it != elf_targets.end(); it++) {
		unsigned j = 0;

		for (j = 0; j < it->second.size(); j++) {
			TargetFunc *pf = &target_funcs[it->second[j]];

			LPROFILE_INFO("Target function %s:%p(%lu), ID %u",
							it->first.c_str(),
							(void*)pf->func, it->second[j], pf->index);

			// LPROFILE_INFO("Target function %s:%s(%s), ID %u",
			// 				it->first.c_str(),
			// 				pf->func->getName().c_str(),
			// 				pf->elf.c_str(), pf->index);
		}
	}
}



const static char *skiplib[] = {
	"libpapi", "libpfm", NULL
};

const static char *osprefix[] = {
	"/usr/lib64/", "/lib64/", "/usr/local/gcc/", NULL
};

void CountUtil::getUserObjs(BPatch_Vector<BPatch_object *> &objs)
{
	BPatch_object *obj = NULL;
	BPatch_Vector<BPatch_object *> allobjs;

	as->getImage()->getObjects(allobjs);
	for (unsigned i = 0; i < allobjs.size(); i++) {
		obj = allobjs[i];
		if (obj->isSystemLib()) {
			LPROFILE_INFO("Skip system lib %s", obj->pathName().c_str());
			continue;
		}

		bool is_skip = false;

		for(unsigned j = 0; ; j++) {
			const char *str = osprefix[j];

			if (!str)
				break;

			if (obj->pathName().compare(0, strlen(str), str) == 0) {
				is_skip = true;
				LPROFILE_INFO("Skip object %s", obj->pathName().c_str());
				break;
			}
		}
		if (is_skip)
			continue;

		for(unsigned j = 0; ; j++) {
			const char *str = skiplib[j];

			if (!str)
				break;

			if (obj->name().find(str) != std::string::npos) {
				is_skip = true;
				LPROFILE_INFO("Skip object %s", obj->pathName().c_str());
				break;
			}
		}
		if (!is_skip) {
			objs.push_back(obj);
		}
	}
}

bool CountUtil::insertCount(void)
{
	unsigned int fun_id = 0;

	for (unsigned i = 0; i < target_funcs.size(); i++) {
		TargetFunc *tf = &target_funcs[i];
		vector<BPatch_point *> *pentry = NULL, *pexit = NULL;
		BPatchSnippetHandle *handle = NULL;

		if (!tf->is_user)
			continue;

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
	libcnt = NULL;

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

	LPROFILE_INFO("Load init function");
	func_init = findFunction(libcnt, FUNC_INIT);
	func_tinit = findFunction(libcnt, WRAP_TINIT);
	if (!func_init || !func_tinit) {
		LPROFILE_ERROR("Failed to load init function");
		return false;
	}

	LPROFILE_INFO("Load exit function");
	func_exit = findFunction(libcnt, FUNC_EXIT);
	func_texit = findFunction(libcnt, WRAP_TEXIT);
	if (!func_exit || !func_texit) {
		LPROFILE_ERROR("Failed to load destroy function");
		return false;
	}
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

// bool CountUtil::insertExit(void)
// {
// 	BPatch_Vector<BPatch_snippet *> exit_arg;
// 	BPatch_Vector<BPatch_point *> *exit_point = NULL;
// 	BPatchSnippetHandle *handle = NULL;
// 	BPatch_Vector<BPatch_function *> funcs;
// 	BPatch_module *mainmod = NULL;

// 	BPatch_funcCallExpr exit_expr(*func_exit, exit_arg);

// 	/* insert global exiting function */
// 	as->getImage()->findFunction("^_fini", funcs);
// 	mainmod = findMainModule();

// 	if (mainmod == NULL) {
// 		return false;
// 	}

// 	for (unsigned int i = 0; i < funcs.size(); i++) {
// 		BPatch_module *mod = funcs[i]->getModule();

// 		if (mod->isSharedLib())
// 			continue;
		
// 		if (mod != mainmod)
// 			continue;

// 		LPROFILE_INFO("Insert exit function to %s",
// 						mod->getObject()->name().c_str());
// 		handle = mod->insertFiniCallback(exit_expr);
// 		if (!handle) {
// 			LPROFILE_ERROR("Failed to insert exit function to %s",
// 							mod->getObject()->name().c_str());
// 			return false;
// 		}
// 		target_funcs.push_back(TargetFunc(funcs[i], UINT_MAX, mod->getObject()));
// 		break;
// 	}

// 	// /* insert per-thread exiting function */
// 	// funcs.clear();
// 	// as->getImage()->findFunction("^pthread_exit", funcs);
// 	// if (funcs.size() == 0) {
// 	// 	LPROFILE_INFO("No pthread_exit found");
// 	// 	return true;
// 	// }

// 	// exit_point = funcs[0]->findPoint(BPatch_entry);
// 	// if (!exit_point) {
// 	// 	LPROFILE_ERROR("Failed to find point for pthread_exit");
// 	// 	return true;
// 	// }

// 	// BPatch_funcCallExpr thread_exit_expr(*func_texit, exit_arg);

// 	// handle = as->insertSnippet(thread_exit_expr,
// 	// 				*exit_point, BPatch_callBefore);
// 	// if (!handle) {
// 	// 	LPROFILE_ERROR("Failed to insert thread exit function");
// 	// 	return false;
// 	// }

// 	return true;
// }

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
			"\t\tDefault is 'cpu-cycles'\n"
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
	args.push_back(new BPatch_constExpr(evlist.c_str()));

	if (logfile.size() == 0)
		logfile = LOGFILE_DEF;
	args.push_back(new BPatch_constExpr(logfile.c_str()));

	calculateRange();

	LPROFILE_DEBUG("min %u, max %u",
					func_id_range.min, func_id_range.max);
	args.push_back(new BPatch_constExpr(func_id_range.min));
	args.push_back(new BPatch_constExpr(func_id_range.max));

	args.push_back(new BPatch_constExpr(freq));
}
#endif // USE_FUNCCNT

SymtabAPI::Symbol *CountUtil::findHookSymbol(BPatch_function *func_wrap, string orgname)
{

	SymtabAPI::Symtab *symtab = NULL;
	vector<SymtabAPI::Symbol *> symlist;
	int i = 0;
	ParseAPI::Function *ifunc = Dyninst::ParseAPI::convert(func_wrap);
	ParseAPI::SymtabCodeSource *src = dynamic_cast<ParseAPI::SymtabCodeSource *>(ifunc->obj()->cs());
	string symname = "HOOK_" + orgname;

	if (!src) {
		LPROFILE_ERROR("Error: wrapper function created from non-SymtabAPI code source");
		return NULL;
	}

	symtab = src->getSymtabObject();

	if (symtab->findSymbol(symlist, symname, SymtabAPI::Symbol::ST_UNKNOWN,
							SymtabAPI::anyName, false, false, true)) {
		return symlist[0];
	}

	LPROFILE_ERROR("Failed to find hook symbol %s", symname.c_str());
	return NULL;
}

bool CountUtil::wrapFunction(string funcstr, BPatch_function *__wrap)
{
	BPatch_function *func_orig = NULL, *func_wrap = NULL;
	BPatch_Vector<BPatch_function *> tfuncs;
	string tmp = "^" + funcstr;
	SymtabAPI::Symbol *hooksym = NULL;
	BPatch_Vector<BPatch_object *> tmods;

	LPROFILE_INFO("wrap function %s", funcstr.c_str());

	as->getImage()->findFunction(tmp.c_str(), tfuncs);
	if (tfuncs.size() == 0) {
		LPROFILE_ERROR("Func %s is not found", tmp.c_str());
		return false;
	}
	for (size_t i = 0; i < tfuncs.size(); i++) {
		LPROFILE_INFO("Find function %s", tfuncs[i]->getName().c_str());
	}
	func_orig = tfuncs[0];

	if (__wrap == NULL) {
		tmp = "^lprobe_" + funcstr;
		tfuncs.clear();
		libcnt->findFunction(tmp.c_str(), tfuncs);
		if (tfuncs.size() == 0) {
			LPROFILE_ERROR("Func %s is not found", tmp.c_str());
			return false;
		}
		func_wrap = tfuncs[0];
	}
	else {
		func_wrap = __wrap;
	}

	hooksym = findHookSymbol(func_wrap, funcstr);
	if (!hooksym) {
		LPROFILE_ERROR("Hook symbol for %s is not found",
				funcstr.c_str());
		return false;
	}

	getUserObjs(tmods);

	if (!as->wrapDynFunction(func_orig, func_wrap, hooksym, tmods)) {
		LPROFILE_ERROR("Failed to wrap function %s", funcstr.c_str());
		return false;
	}
	return true;
}

#define LIBPTHREAD_NAME "libpthread"

uint32_t CountUtil::findTIDOffset(void)
{
	BPatch_Vector<BPatch_object *> allobjs;
	BPatch_object *objpthread = NULL;
	BPatch_type *pthread_type = NULL;

	as->getImage()->getObjects(allobjs);
	for (unsigned i = 0; i < allobjs.size(); i++) {
		objpthread = allobjs[i];
		if (objpthread->name().compare(0, strlen(LIBPTHREAD_NAME), LIBPTHREAD_NAME)) {
			objpthread = NULL;
			continue;
		}
		else
			break;
	}

	if (!objpthread) {
		LPROFILE_INFO("No libpthread.so is found");
		return UINT32_MAX;
	}

	pthread_type = as->getImage()->findType("pthread");
	if (!pthread_type) {
		LPROFILE_INFO("Failed to find struct pthread in libpthread.so");
		return UINT32_MAX;
	}
	LPROFILE_INFO("Find pthread, type %u, size %u",
					pthread_type->getDataClass(), pthread_type->getSize());

	BPatch_Vector<BPatch_field *> *fields = pthread_type->getComponents();

	if (fields) {
		uint32_t offset = 0;
		for (unsigned i = 0; i < (*fields).size(); i++) {
			BPatch_field *field = (*fields)[i];

			if (strncmp(field->getName(), "tid", sizeof("tid")) == 0)
				break;

			LPROFILE_INFO("Field[%u]: %s, size %u, offset %d",
							i, field->getName(), field->getType()->getSize(),
							field->getOffset());
			offset += field->getType()->getSize();
		}
		LPROFILE_INFO("tid offset: %u", offset);
		return offset;
	}

	return UINT32_MAX;
}

bool CountUtil::wrapPthreadFunc(void)
{
	uint32_t offset = UINT32_MAX;

	offset = findTIDOffset();
	if (offset == UINT32_MAX) {
		LPROFILE_INFO("Cannot read PID offset in struct pthread, "
					"pthread_create will not be wrapped.");
		return true;
	}

	// write tid offset to libprobe.so
	vector<BPatch_module *> mods;
	BPatch_variableExpr *var = NULL;

	libcnt->modules(mods);
	var = mods[0]->findVariable(LIBTIDOFFSET);

	if (!var) {
		LPROFILE_ERROR("Failed to find global var %s in %s",
							LIBTIDOFFSET, LIBCNT);
		return false;
	}
	var->writeValue(&offset);

	if (!wrapFunction("pthread_create", func_tinit)) {
		LPROFILE_ERROR("Failed to wrap pthread_create");
		return false;
	}
	return true;
}