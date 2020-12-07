#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>

#include <cstring>
#include <string>
#include <set>

#include "BPatch.h"
#include "BPatch_object.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_function.h"
#include "BPatch_module.h"
#include "BPatch_type.h"

#include "util.h"
#include "funcmap.h"
#include "count.h"

using namespace std;
using namespace Dyninst;

static set<string> skiplist;

static void initSkipList(void)
{
	if (skiplist.size() > 0)
		return;

	skiplist.insert("_init");
	skiplist.insert("_fini");
	skiplist.insert("__popcountdi2");
	skiplist.insert("__do_global_dtors_aux");
}

FuncMap::FuncMap(std::string name, std::string path, bool wrap) :
		obj(NULL),
		wrapper(NULL)
{
	elf_path = path;
	elf_name = name;
	is_wrap = wrap;

	if (skiplist.size() == 0)
		initSkipList();
}

FuncMap::FuncMap(string path, bool wrap) :
		elf_path(path),
		is_wrap(wrap),
		obj(NULL),
		wrapper(NULL)
{
	size_t pos = 0;

	pos = elf_path.find_last_of('/');
	if (pos == (size_t)-1)
		elf_name = elf_path;
	else
		elf_name = elf_path.substr(pos + 1);
}

FuncMap::FuncMap(BPatch_object *target, bool wrap) :
		obj(target),
		is_wrap(wrap),
		wrapper(NULL)
{
	elf_path = obj->pathName();
	elf_name = obj->name();

	if (skiplist.size() == 0)
		initSkipList();
}

FuncMap::~FuncMap(void)
{
	if (wrapper) {
		fclose(wrapper);
		wrapper = NULL;
	}
}

uint8_t FuncMap::checkState(void)
{
	time_t last_update;
	struct stat fileinfo;
	int ret = 0;
	string cachefile(FUNCMAP_DIR);

	// check elf file
	ret = stat(elf_path.c_str(), &fileinfo);
	if (ret < 0)
		return FUNCMAP_STATE_ENOELF;
	last_update = fileinfo.st_mtime;

	// check cache file
	cachefile += elf_name;
	ret = stat(cachefile.c_str(), &fileinfo);
	if (ret < 0) {
		// if cache doesn't exist, create(update) it.
		if (errno == ENOENT)
			return FUNCMAP_STATE_UPDATE;
		// Otherwise, error
		return FUNCMAP_STATE_ECACHE;
	}
	// if cache exists, check its last-updated time
	else {
		// if cache is out-of-date, update
		if (difftime(last_update, fileinfo.st_mtime) > 0)
			return FUNCMAP_STATE_UPDATE;
		// if cache is empty, update
		else if (fileinfo.st_size == 0)
			return FUNCMAP_STATE_UPDATE;
		else
			return FUNCMAP_STATE_CACHED;
	}
}

unsigned FuncMap::addFunction(const char *func)
{
	string funcname(func);
	unsigned index = 0;

	// check if exists
	if (func_indices.count(funcname))
		return UINT_MAX;

	// filter
	if (skiplist.count(funcname))
		return UINT_MAX;

	index = funcs.size();
	funcs.push_back(funcname);
	func_indices.insert(pair<string, unsigned int>(funcname, index));

//	LPROFILE_DEBUG("[%s] Function %u: %s",
//					elf_path.c_str(), index, func);
	return index;
}

std::string FuncMap::getWrapFilePath(std::string elf, unsigned type)
{
	string name(FUNCMAP_DIR);

	name += FUNCMAP_PREFIX;
	name += elf;
	if (type == FUNCMAP_FILE_SRC)
		name += ".c";
	else
		name += ".so";
	return name;
}

std::string FuncMap::getWrapSymbolName(unsigned index, unsigned type)
{
	string name;

	switch (type) {
		case FUNCMAP_SYMBOL_HOOK:
			name = "LPROFILE_HOOK_func" + to_string(index);
			break;
		case FUNCMAP_SYMBOL_WRAP:
			name = "LPROFILE_WRAP_func" + to_string(index);
			break;
		default:
			LPROFILE_ERROR("Wrong func type");
			break;
	}
	return name;
}

std::string FuncMap::generateWrapDef(
				unsigned index, unsigned type, bool is_proto,
				unsigned nb_params, bool has_ret)
{
	string funcname;

	funcname = FuncMap::getWrapSymbolName(index, type) + "(";

	if (is_proto) {
		if (has_ret)
			funcname = "void *" + funcname;
		else
			funcname = "void " + funcname;
	}

	for (unsigned i = 0; i < nb_params; i++) {
		if (i > 0)
			funcname += ", ";
		if (is_proto)
			funcname += "void *p" + to_string(i);
		else
			funcname += "p" + to_string(i);
	}
	funcname += ")";
	return funcname;
}

bool FuncMap::generateWrapper(BPatch_function *func, unsigned index)
{
	vector<BPatch_localVar *> *params = NULL;
	bool has_ret = true;
	unsigned i = 0;
	string tmpstr;

	LPROFILE_DEBUG("Generate wrapper file");

	if (!wrapper) {
		string wrap_file;
		wrap_file = getWrapFilePath(elf_name, FUNCMAP_FILE_SRC);
		LPROFILE_DEBUG("Wrap file %s", wrap_file.c_str());

		wrapper = fopen(wrap_file.c_str(), "w");
		if (!wrapper) {
			LPROFILE_ERROR("Failed to open wrapper file %s",
							wrap_file.c_str());
			return false;
		}

		tmpstr = "#include <stdio.h>\n";
		tmpstr += "void " + string(FUNC_PRE) + "(unsigned int);\n";
		tmpstr += "void " + string(FUNC_POST) + "(unsigned int);\n";
		fprintf(wrapper, "%s", tmpstr.c_str());
	}

	params = func->getParams();
	if (!func->getReturnType() || func->getReturnType()->getDataClass()
					== BPatch_dataNullType)
		has_ret = false;
	else
		has_ret = true;
	LPROFILE_DEBUG("Function %s: %lu params, return %u",
					func->getName().c_str(),
					params->size(), has_ret);

	// generate an empty symbol hook_funcX as a hook
	tmpstr = generateWrapDef(index, FUNCMAP_SYMBOL_HOOK, true,
					params->size(), has_ret) + ";\n";

	/* generate the wrap function
	 * void * wrap_funcX(...) {
	 *     void *ret;
	 *     probe(X);
	 *     ret = hook_funcX(...);
	 *     probe(X);
	 *     return ret;
	 * }
	 */
	tmpstr += generateWrapDef(index, FUNCMAP_SYMBOL_WRAP, true,
					params->size(), has_ret) + " {\n";
	if (has_ret)
		tmpstr += "\tvoid *ret;\n";

	tmpstr += "\tfprintf(stdout, \"enter " + to_string(index) + "\\n\");\n";
//	tmpstr += "\t" + string(FUNC_PRE);
//	tmpstr += "(" + to_string(index) + ");\n";

	if (has_ret)
		tmpstr += "\tret = ";
	else
		tmpstr += "\t";

	tmpstr += generateWrapDef(index, FUNCMAP_SYMBOL_HOOK, false,
					params->size(), has_ret) + ";\n";

	tmpstr += "\tfprintf(stdout, \"exit " + to_string(index) + "\\n\");\n";
//	tmpstr += "\t" + string(FUNC_POST) + "(" + to_string(index) + ");\n";

	if (has_ret)
		tmpstr += "\treturn ret;\n";
	tmpstr += "}\n";

	fprintf(wrapper, "%s", tmpstr.c_str());

	return true;
}

bool FuncMap::compileWrapper(void)
{
	string srcfile, libfile;
	string cmd;
	int ret = 0;

	if (wrapper) {
		fclose(wrapper);
		wrapper = NULL;

		srcfile = getWrapFilePath(elf_name, FUNCMAP_FILE_SRC);
		libfile = getWrapFilePath(elf_name, FUNCMAP_FILE_LIB);
//		cmd = "gcc -g -o " + libfile + " -shared -fPIC " + srcfile + " -lprobe";
		cmd = "gcc -g -o " + libfile + " -shared -fPIC " + srcfile;

		ret = system(cmd.c_str());
		LPROFILE_DEBUG("CMD %s, ret %d", cmd.c_str(), ret);
		if (!ret)
			return true;

		LPROFILE_ERROR("Failed to compile %s, ret %d", srcfile.c_str(), ret);
		return false;
	}
	return true;
}

bool FuncMap::loadFromCache(void)
{
	FILE *file = NULL;
	string cachefile(FUNCMAP_DIR);
	char str[FUNC_NAME_MAX] = {'\0'};
	unsigned len = 0;

	cachefile += elf_name;
	file = fopen(cachefile.c_str(), "r");
	if (file == NULL) {
		LPROFILE_ERROR("Failed to open cache file %s, err %d",
						cachefile.c_str(), errno);
		return false;
	}

	while (!feof(file)) {
		if (fgets(str, FUNC_NAME_MAX, file) != NULL) {
			len = strlen(str);
			if (str[len - 1] == '\n')
				str[len - 1] = '\0';
			addFunction(str);
		}
	}
	return true;
}

bool FuncMap::buildDir(void)
{
	string path(FUNCMAP_DIR);
	ssize_t pos = 0;

	while (1) {
		pos = path.find_first_of('/', pos);
		if (pos == 0) {
			pos++;
			continue;
		}
		else if (pos == (ssize_t)-1)
			break;

		string subpath = path.substr(0, pos);

		// check if exists
		if (access(subpath.c_str(), F_OK) != 0) {
			if (mkdir(subpath.c_str(), 0755) != 0) {
				LPROFILE_ERROR("Failed to create directory %s, err %d",
								subpath.c_str(), errno);
				return false;
			}
		}
		pos++;
	}
	return true;
}

bool FuncMap::updateCache(void)
{
	FILE *file = NULL;
	string cachefile(FUNCMAP_DIR);

	// check and create the dictories
	if (!buildDir())
		return false;

	cachefile += elf_name;
	file = fopen(cachefile.c_str(), "w");
	if (file == NULL) {
		LPROFILE_ERROR("Failed to open cache file %s",
						cachefile.c_str());
		return false;
	}

	LPROFILE_INFO("Update cache %s", cachefile.c_str());
	for (unsigned i = 0; i < funcs.size(); i++)
		fprintf(file, "%s\n", funcs[i].c_str());
	return true;
}

bool FuncMap::loadFromELF(void)
{
	/* if there is no existing open BPatch_object for this ELF, open
	 * this ELF by BPatch_binaryEdit.
	 */
	if (obj == NULL) {
		BPatch_Vector<BPatch_function *> funclist;
		BPatch_binaryEdit *binary = NULL;

		binary = bpatch.openBinary(elf_path.c_str(), false);
		if (!binary) {
			LPROFILE_ERROR("Failed to open ELF file %s",
							elf_path.c_str());
			return false;
		}

		if (!(binary->getImage()->getProcedures(funclist, false))) {
			LPROFILE_ERROR("Failed to get function list from "
					  "BPatch_binaryEdit(%s)", elf_path.c_str());
			return false;
		}

		// generate function map		
		for (unsigned i = 0; i < funclist.size(); i++) {
			unsigned func_idx = addFunction(funclist[i]->getName().c_str());

			if (is_wrap && func_idx != UINT_MAX) {
				if (!generateWrapper(funclist[i], func_idx))
					return false;
			}
		}
	}
	// get the function list directly from BPatch_object
	else {
		BPatch_Vector<BPatch_module *> mods;

		obj->modules(mods);
		for (unsigned i = 0; i < mods.size(); i++) {
			BPatch_Vector<BPatch_function *> tmp_funcs;

			mods[i]->getProcedures(tmp_funcs, false);
			for (unsigned j = 0; j < tmp_funcs.size(); j++) {
				unsigned func_idx = addFunction(tmp_funcs[i]->getName().c_str());

				if (is_wrap && func_idx != UINT_MAX) {
					if (!generateWrapper(tmp_funcs[i], func_idx))
						return false;
				}
			}
		}
	}

	if (is_wrap && funcs.size() > 0) {
		// compile wrapper library
		if (!compileWrapper()) {
			LPROFILE_DEBUG("Failed to compile wrapper library");
			return false;
		}
	}

	// update cache
	if (!updateCache()) {
		LPROFILE_ERROR("Failed to update cache");
		return false;
	}
	return true;
}

bool FuncMap::load(bool force_update)
{
	uint8_t state;

	if (force_update) {
		LPROFILE_INFO("Force updating. Load function map from ELF file.");
		return loadFromELF();
	}

	// check the state of cache
	state = checkState();
	switch (state) {
		case FUNCMAP_STATE_CACHED:
			LPROFILE_INFO("Load function map from cache");
			return loadFromCache();
		case FUNCMAP_STATE_UPDATE:
			LPROFILE_INFO("Load function map from ELF file");
			return loadFromELF();
		default:
			LPROFILE_ERROR("Error state: %d", state);
			return false;
	}
}

unsigned int FuncMap::getFunctionID(string func)
{
	map<string, unsigned>::iterator iter;

	iter = func_indices.find(func);
	if (iter == func_indices.end())
		return UINT_MAX;
	return iter->second;
}

unsigned FuncMap::getFunctionID(const char *func)
{
	return getFunctionID(string(func));
}

void FuncMap::printAll(void)
{
//	map<string, unsigned>::iterator iter;
//
//	for (iter = func_indices.begin(); iter != func_indices.end(); iter++)
//		LPROFILE_INFO("FUNC[%u]: %s", iter->second, iter->first.c_str());
//	LPROFILE_INFO("Total %lu functions", func_indices.size());
	size_t i = 0;

	for (i = 0; i < funcs.size(); i++) {
		LPROFILE_INFO("FUNC[%lu]: %s", i, funcs[i].c_str());
	}
	LPROFILE_INFO("Total %lu functions", funcs.size());
}
