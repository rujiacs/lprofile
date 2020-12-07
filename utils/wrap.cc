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
#include "wrap.h"
#include "funcmap.h"
#include "count.h"
#include "util.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::SymtabAPI;

Test *WrapTest::construct(void)
{
	return new WrapTest();
}

WrapTest::WrapTest(void)
{
	mode = LPROFILE_MODE_PROC;
}

WrapTest::~WrapTest(void)
{
	zfree(muttee_argv);
}

bool WrapTest::init(void)
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
	if (!count.getTargetFuncs(true)) {
		LPROFILE_ERROR("Failed to find target functions (%s)",
						count.getPattern().c_str());
		return false;
	}
	return true;
}

void WrapTest::staticUsage(void)
{
	fprintf(stdout, "./lprofile %s\n",
					WRAP_CMD);
}

bool WrapTest::parseArgs(LPROFILE_UNUSED int argc,
				LPROFILE_UNUSED char **argv)
{
	int c;
	char buf[64] = {'\0'};
	string optstr = CountUtil::getOptStr() + "w:h";

	while ((c = getopt(argc, argv, optstr.c_str())) != -1) {
		switch(c) {
			case 'h':
				staticUsage();
				exit(0);

			case 'w':
				mode = LPROFILE_MODE_EDIT;
				output = optarg;
				break;

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
	if (muttee_argc > 0) {
		muttee_argv = (const char **)malloc(sizeof(char*) * (muttee_argc+1));
		if (!muttee_argv) {
			LPROFILE_ERROR("Failed to allocate memory for muttee_argv");
			return false;
		}

		for (c = 0; c < muttee_argc; c++) {
			muttee_argv[c] = argv[c];
			LPROFILE_DEBUG("Muttee argv[%d]=%s", c, muttee_argv[c]);
		}
		muttee_argv[muttee_argc] = NULL;
	}
	else
		muttee_argv = NULL;

	return true;
}

Symbol *WrapTest::findHookSymbol(BPatch_function *func_wrap, unsigned int index)
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

	if (symtab->findSymbol(symlist, FuncMap::getWrapSymbolName(index, FUNCMAP_SYMBOL_HOOK),
							Symbol::ST_UNKNOWN,
							anyName, false, false, true)) {
		return symlist[0];
	}
	return NULL;
}

void WrapTest::handleElf(string elf, funclist_t &targets)
{
	unsigned int i = 0;
	BPatch_object *libwrap = NULL;
	string wrap_path;

	wrap_path = FuncMap::getWrapFilePath(elf, FUNCMAP_FILE_LIB);
	LPROFILE_DEBUG("Load wrapper library %s", wrap_path.c_str());
	libwrap = as->loadLibrary(wrap_path.c_str());
	if (!libwrap) {
		LPROFILE_ERROR("Failed to load %s", wrap_path.c_str());
		return;
	}
	LPROFILE_DEBUG("%lu functions in %s", targets.size(), elf.c_str());
	for (i = 0; i < targets.size(); i++)
		handleFunction(libwrap, targets[i]);
}

void WrapTest::handleFunction(BPatch_object *libwrap, TargetFunc *func)
{
	BPatch_function *func_wrap = NULL;
	SymtabAPI::Symbol *hook_sym = NULL;

	LPROFILE_DEBUG("Load wrapper function for func %u", func->index);
	func_wrap = count.findFunction(libwrap,
					FuncMap::getWrapSymbolName(func->index, FUNCMAP_SYMBOL_WRAP));
	if (!func_wrap)
		return;
	hook_sym = findHookSymbol(func_wrap, func->index);

	if (!hook_sym) {
		LPROFILE_ERROR("Symbol for hook %u is not found", func->index);
		return;
	}
	LPROFILE_DEBUG("Replace %s with %s, orig code is stored in %s:%lx",
					func->func->getName().c_str(),
					func_wrap->getName().c_str(),
					hook_sym->getMangledName().c_str(),
					hook_sym->getOffset());

	if (!as->wrapFunction(func->func, func_wrap, hook_sym)) {
		LPROFILE_ERROR("Failed to replace function");
	}
}

bool WrapTest::process(void)
{
	elfmap_t elfs = count.getTargetLists();
	elfmap_t::iterator iter;
	BPatch_object *libcnt = NULL;

	// load probe library
	// load lib
	LPROFILE_DEBUG("Load %s", LIBCNT);
	libcnt = as->loadLibrary(LIBCNT);
	if (!libcnt) {
		LPROFILE_ERROR("Failed to load %s", LIBCNT);
		return false;
	}

	// handle target functions
	for (iter = elfs.begin(); iter != elfs.end(); iter++)
		handleElf(iter->first, iter->second);

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
			LPROFILE_INFO("!!! Muttee exited with signal %d", proc->getExitSignal());
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
